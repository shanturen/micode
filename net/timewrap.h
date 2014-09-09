#pragma once

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include "safe_time.h"
class timee
{
	// _msecs: 
	timeval _tv;
public:
	timee() { _tv.tv_sec = 0; _tv.tv_usec = 0; }
	timee &operator=(const timee &rhs) { _tv = rhs._tv; }
	long long operator -(const timee &rhs)
	{
		return (_tv.tv_sec - rhs._tv.tv_sec) * 1000000 + _tv.tv_usec - rhs._tv.tv_usec;
	}
	static timee now()
	{
		timee t;
		safe_gettimeofday(&(t._tv), 0);
		return t;
	}
};

class time_impl
{
	int64_t rdc;
public:
	long long operator -(const time_impl &rhs) {};
private:
	static int get_rdtsc()
	{
		asm("rdtsc");
	}
	static  int64_t rdtsc(void)
	{
		int i, j;
		asm volatile ("rdtsc" : "=a"(i), "=d"(j) : );
		return ((unsigned int64_t)j<<32) + (int64_t)i;
	}
};
	
class date
{
};
