#pragma once
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>


class spin_lock
{
	int _lock;
public:
	spin_lock() : _lock(0) {}
	~spin_lock() {unlock();}
	void lock()
	{
		while (!__sync_bool_compare_and_swap(&_lock, 0, 1));
	}
	void unlock()
	{
		__sync_bool_compare_and_swap(&_lock, 1, 0);
	}
};

