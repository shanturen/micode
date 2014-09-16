#include "thread.h"
#include <iostream>
#include "log.h"
using namespace std;

int task::start(thread_pool *tp)
{
	tp->add_task(this);
	return 0;
}

thread_pool::thread_pool()
{
	_worker = 0;
	_max_number_of_threads = 4;
	_number_of_free_work_threads = 0;
	_number_of_work_threads = 0;
}

thread_pool::~thread_pool()
{
	for (list<pthread *>::iterator it = _thread_list.begin(); it != _thread_list.end(); it++) {
		pool_work_thread *_work_thread = static_cast<pool_work_thread *>(*it);
		_work_thread->stop();
		LOG_DEBUG_VA("wait thread end\n");
		_work_thread->wait();
		LOG_DEBUG_VA("thread ended\n");
		delete *it;
	}

	if (_worker)
		delete _worker;
}

int thread_pool::add_task(task *tsk)
{
	_task_mutex.lock();
	_task_queue.push(tsk);
	_task_mutex.unlock();
	if (_number_of_free_work_threads == 0 && _number_of_work_threads < _max_number_of_threads) {
		pool_work_thread *new_work_thread = new pool_work_thread(this);
		_thread_list.push_back(new_work_thread);
		new_work_thread->start();
		_number_of_work_threads++;
		LOG_INFO_VA("new work thread started, total:%d, free:%d\n", _number_of_work_threads, _number_of_free_work_threads);
	}
	_task_event.signal();
}

int thread_pool::set_worker(worker *wk)
{
	_worker = wk;
	_worker->set_thread_pool(this);
}

int worker::work()
{
	while (!_stoped) {
		task *tsk = _tp->get_task();
		if (!tsk) {
			_tp->_number_lock.lock();
			_tp->_number_of_free_work_threads++;
			_tp->_number_lock.unlock();
			_tp->_task_event.timedwait(1000);
			// if wait time out
			//	delete this worker;
			_tp->_number_lock.lock();
			_tp->_number_of_free_work_threads--;
			_tp->_number_lock.unlock();
		} else {
			tsk->execute(this);
			delete tsk;
		}
	}
	LOG_DEBUG_VA("worker stoped work\n");
}

thread_pool::pool_work_thread::~pool_work_thread()
{
	if (_wkr)
		delete _wkr;
}

task *thread_pool::get_task()
{
	task *t = NULL;
	_task_mutex.lock();
	if (!_task_queue.empty()) {
		t = _task_queue.front();
		_task_queue.pop();
	}
	_task_mutex.unlock();
	return t;
}

int thread_pool::pool_work_thread::thread_func()
{
	_wkr = _tp->new_worker();
	_wkr->work();
}

void thread_pool::pool_work_thread::stop()
{
	_wkr->stop();
}

worker *thread_pool::new_worker()
{
	if (!_worker) {
		_worker = new worker();
		_worker->set_thread_pool(this);
	}
	worker *wkr = _worker->clone();
	return wkr;
}
