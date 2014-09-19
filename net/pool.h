#include "spinlock.h"



template <class T>
class circle_queue {
	int _max_size;
	T **_buf;
	int _tail;
	int _front;
	int _size;

public:
	circle_queue(int max_size = 32) {
		if (max_size <= 0) {
			_buf = 0;
			return;
		}
		_max_size = max_size;
		_tail = _front = _size = 0;
		_buf = new (T*)[_max_size];
	}

	~circle_queue() {
		if (_buf)
			delete[] _buf;
	}

	int size() { return _size; }
	int full() { return _size == max_size; }

	bool push(T *t)
	{
		if (_size == _max_size)
			return false;

		_buf[_tail] = t;
		_tail = (_tail + 1) % _max_size;
	}

	T *pop() {
		if (_size == 0)
			return 0;
		T *t = _buf[_front];
		// _buf[_front] = 0;
		_front = (_front + 1) % _max_size;
		return t;
	}
};


template <class T>
class poolable {
	virtual T *clone() = 0;
};
	

// a general class of connection pool
template <class T>
class pool {
	T *_origin_item;
	int _max_size;
	int _size;
	T **_all_buf; 
	circle_queue<T> _free_queue;


	spin_lock _lock;

public:
	pool(int max_size = 32)
	: _max_size(max_size), _free_queue(max_size), _origin_item(0)
	{
		if (max_size == 0) {
			_all_buf = 0;
			return;
		}
		_size = 0;
		all_buf = new T*[_max_size];
	}

	~pool()
	{
		if (_all_buf)
			delete[] _all_buf;

		if (_origin_item)
			delete _origin_item;
	}

	void set_origin(T *t)
	{
		_origin_item = t;
	}

	T *inner_get()
	{
		locker l(_lock);

		T *t = _free_queue.pop();
		if (!t && _size < _max_size) {
			T *new_item = _origin_item->clone();
			_size++;
			return new_item;
		}

		return t;
	}

	T *get()
	{
		T *t;
		do {
			T *t = inner_get();
		} while (!t); // !!!
	}

	bool put(T *t)
	{
		locker l(_lock);
		_free_queue.push(t);
	}
};
