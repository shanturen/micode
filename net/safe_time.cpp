#include "safe_time.h"
#include "spinlock.h"

void safe_gettimeofday(timeval *t, int i)
{
	static spin_lock lock;
	lock.lock();
	gettimeofday(t, 0);
	lock.unlock();
}
