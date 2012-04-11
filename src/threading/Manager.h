
#ifndef THREADING_MANAGER_H
#define THREADING_MANAGER_H

#include <list>

#include "IOSource.h"

#include "BasicThread.h"
#include "MsgThread.h"

namespace input { namespace reader {
	class Benchmark;
}}

namespace threading {

/**
 * The thread manager coordinates all child threads. Once a BasicThread is
 * instantitated, it gets addedd to the manager, which will delete it later
 * once it has terminated.
 *
 * In addition to basic threads, the manager also provides additional
 * functionality specific to MsgThread instances. In particular, it polls
 * their outgoing message queue on a regular basis and feeds data sent into
 * the rest of Bro. It also triggers the regular heartbeats.
 */
class Manager : public IOSource
{
public:
	/**
	 * Constructor. Only a single instance of the manager must be
	 * created.
	 */
	Manager();

	/**
	 * Destructir.
	 */
	~Manager();

	/**
	 * Terminates the manager's processor. The method signals all threads
	 * to terminates and wait for them to do so. It then joins them and
	 * returns to the caller. Afterwards, no more thread instances may be
	 * created.
	 */
	void Terminate();

	/**
	 * Returns True if we are currently in Terminate() waiting for 
	 * threads to exit.
	 */
	bool Terminating() const	{ return terminating; }

	/**
	 * Immediately kills all child threads. It does however not yet join
	 * them, one still needs to call Terminate() for that.
	 *
	 * This method is safe to call from a signal handler, and can in fact
	 * be called while Terminate() is already in progress.
	 */
	void KillThreads();

	typedef std::list<std::pair<string, MsgThread::Stats> > msg_stats_list;

	/**
	 * Returns statistics from all current MsgThread instances.
	 *
	 * @return A list of statistics, with one entry for each MsgThread.
	 * Each entry is a tuple of thread name and statistics. The list
	 * reference remains valid until the next call to this method (or
	 * termination of the manager).
	 */
	const msg_stats_list& GetMsgThreadStats();

	/**
	 * Returns the number of currently active threads. This counts all
	 * threads that are not yet joined, includingt any potentially in
	 * Terminating() state.
	 */
	int NumThreads() const { return all_threads.size(); }

protected:
	friend class BasicThread;
	friend class MsgThread;
	friend class input::reader::Benchmark; // needs heartbeat

	/**
	 * Registers a new basic thread with the manager. This is
	 * automatically called by the thread's constructor.
	 *
	 * @param thread The thread.
	 */
	void AddThread(BasicThread* thread);

	/**
	 * Registers a new message thread with the manager. This is
	 * automatically called by the thread's constructor. This must be
	 * called \a in \a addition to AddThread(BasicThread* thread). The
	 * MsgThread constructor makes sure to do so. 
	 *
	 * @param thread The thread.
	 */
	void AddMsgThread(MsgThread* thread);

	/**
	 * Part of the IOSource interface.
	 */
	virtual void GetFds(int* read, int* write, int* except);

	/**
	 * Part of the IOSource interface.
	 */
	virtual double NextTimestamp(double* network_time);

	/**
	 * Part of the IOSource interface.
	 */
	virtual void Process();

	/**
	 * Part of the IOSource interface.
	 */
	virtual const char* Tag()	{ return "threading::Manager"; }
	
private:
	int heart_beat_interval;

	typedef std::list<BasicThread*> all_thread_list;
	all_thread_list all_threads;

	typedef std::list<MsgThread*> msg_thread_list;
	msg_thread_list msg_threads;

	bool did_process;	// True if the last Process() found some work to do.
	double next_beat;	// Timestamp when the next heartbeat will be sent.
	bool terminating;	// True if we are in Terminate().

	msg_stats_list stats;
};

}

/**
 * A singleton instance of the thread manager. All methods must only be
 * called from Bro's main thread.
 */
extern threading::Manager* thread_mgr;

#endif
