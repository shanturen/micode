#pragma once

#include <pthread.h>
#include "safe_time.h"
#include <errno.h>

class event
{
	bool _auto;
	bool _flag;
	pthread_mutex_t _mtx;
	pthread_cond_t _cond;
	pthread_condattr_t _attr;
	
public:
	event(bool flag = false, bool automation=true) : _flag(false), _auto(automation)
	{
		pthread_mutex_init(&_mtx, NULL);
		pthread_condattr_init(&_attr);
		pthread_condattr_setclock(&_attr, CLOCK_REALTIME);
		pthread_cond_init(&_cond, &_attr);
	}
	~event()
	{
		pthread_mutex_destroy(&_mtx);
		pthread_cond_destroy(&_cond);
	}
	void signal()
	{
		pthread_mutex_lock(&_mtx);
		if (!_flag) {
			_flag = true;
			pthread_cond_signal(&_cond);
		}
		pthread_mutex_unlock(&_mtx);
	}
	void  timedwait(int timeout)
	{
		timeval now;
		safe_gettimeofday(&now, 0);
		timespec t;
		t.tv_nsec = now.tv_usec * 1000 + (timeout % 1000) * 1000000; 
		t.tv_sec = now.tv_sec + timeout / 1000 + t.tv_nsec / 1000000000;
		t.tv_nsec = t.tv_nsec % 1000000000;
		
		//t.tv_sec = now.tv_sec + timeout / 1000;
		//t.tv_nsec = now.tv_usec * 1000;

		pthread_mutex_lock(&_mtx);
		while (!_flag) {
			int ret = pthread_cond_timedwait(&_cond, &_mtx, &t);
			if (ret == ETIMEDOUT)
				break;
		}

		if (_auto)
			_flag = false;
		pthread_mutex_unlock(&_mtx);
	}
	void  wait()
	{
		pthread_mutex_lock(&_mtx);
		while (!_flag) {
			pthread_cond_wait(&_cond, &_mtx);
		}
		
		if (_auto)
			_flag = false;
		pthread_mutex_unlock(&_mtx);
	}
};
