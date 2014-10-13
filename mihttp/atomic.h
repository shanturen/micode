#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

static int count = 0;

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



class mutex
{
	pthread_mutex_t _mutex;
public:
	mutex() {pthread_mutex_init(&_mutex, NULL);}
	~mutex() {pthread_mutex_unlock(&_mutex);}
	void lock()
	{
		pthread_mutex_lock(&_mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&_mutex);
	}
};
	

spin_lock cnt_lock;
//mutex cnt_lock;
int thread_count=20;
int count_count = 100000000;
void *test_func(void *arg)
{
        int i=0;
        for(i=0;i<count_count;++i){
                __sync_fetch_and_add(&count,1);
        }
        return NULL;
}

int  atomic_inc(int &count)
{

