#pragma once
#include <pthread.h>
#include <queue>
#include <list>
#include "mutex.h"
#include "spinlock.h"
#include <string.h>
#include "event.h"
using std::queue;
using std::list;

class pthread
{
	static void *_func(void *obj)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
		pthread *o = static_cast<pthread*>(obj);
		o->thread_func();
	}
	pthread_t _tid;
	pthread_attr_t _attr;
public:
	static int this_thread_id() { return pthread_self(); }
	pthread()
	{
		pthread_attr_init (&_attr);
		pthread_attr_setdetachstate (&_attr, PTHREAD_CREATE_DETACHED);
		_tid = -1;
	}

	virtual ~pthread()
	{
		if (_tid != -1) {
			pthread_cancel(_tid);
		}
		pthread_attr_destroy (&_attr);
	}
	
	int start()
	{
		//int ret = pthread_create(&_tid, &_attr, _func, this);
		int ret = pthread_create(&_tid, 0, _func, this);
		if (ret != 0) {
			printf("%s\n", strerror(errno));
		}
		return ret;
	}
	int kill()
	{
		if (_tid != -1) {
			pthread_cancel(_tid);
			_tid = -1;
		}
	}
	int wait() 
	{
		if (_tid != -1) {
			pthread_join(_tid, 0);
			_tid = -1;
		}
	}
	virtual int thread_func() = 0;
	int get_thread_id() { return *reinterpret_cast<int *>(&_tid); }
};

class thread_pool;
class worker;
class task 
{
public:
	virtual ~task() {}
	int start(thread_pool *tp);
	virtual int execute(worker *wkr) = 0;
};

class thread_pool
{
	friend class worker;
	
	class pool_work_thread : public pthread
	{
		thread_pool *_tp;
		worker *_wkr;
	public:
		pool_work_thread(thread_pool *tp) : _tp(tp), _wkr(0) {}
		~pool_work_thread();
		void stop();
		int thread_func();
	};

	worker *_worker;
	std::queue<task *> _task_queue;
	std::list<pthread *> _thread_list;
	int _number_of_work_threads;
	int _number_of_free_work_threads;
	int _max_number_of_threads;
	mutex _task_mutex;
	event _task_event;
	spin_lock _number_lock;
public:
	thread_pool(); 
	~thread_pool(); 
	void set_max_threads(int i) { _max_number_of_threads = i; }
	task *get_task();
	int add_task(task *thd);
	int set_worker(worker *wk);
	worker* new_worker();
	int delete_worker(worker *idle_worker);
	int kill(pthread *pthd);
};

class worker
{
	thread_pool *_tp;
	bool _stoped;
public:
	worker() { _tp = 0; _stoped = false; }
	virtual ~worker() {};
	void set_thread_pool(thread_pool *tp) { _tp = tp; }
	int work();
	void stop() { _stoped = true; }
	virtual worker *clone()
	{
		worker *wrk = new worker();
		wrk->set_thread_pool(_tp);
		return wrk;
	}
};

