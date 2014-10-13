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
		_buf = new T*[_max_size];
	}

	~circle_queue() {
		if (_buf) {
			/* item was deleted in class pool
			for (int i = _tail; i != _front; i = (++i) % _max_size) {
				delete _buf[i];
			} */
			delete[] _buf;
		}
	}

	int size() { return _size; }
	bool full() { return _size == _max_size; }

	bool push(T *t)
	{
		if (_size == _max_size)
			return false;

		_buf[_tail] = t;
		_tail = (_tail + 1) % _max_size;
		_size++;
	}

	T *pop() {
		if (_size == 0)
			return 0;
		T *t = _buf[_front];
		// _buf[_front] = 0;
		_front = (_front + 1) % _max_size;
		_size--;
		return t;
	}
};

template <class T> class pool;

template <typename T>
class poolable {
	pool<T> *_p;
public:
	virtual T *clone() = 0;
	void set_pool(pool<T> *p) { _p = p; }
	pool<T> *get_pool() { return _p; }
	void put() { if (_p) _p->put(dynamic_cast<T *>(this)); }
};
	
template <class T> class autoput_pointer;

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
		_all_buf = new T*[_max_size];
	}

	~pool()
	{
		for (int i = 0; i != _size; i++) {
			delete _all_buf[i];
		}

		if (_all_buf)
			delete[] _all_buf;

		if (_origin_item)
			delete _origin_item;
	}

	void set_origin(T *t)
	{
		_origin_item = t;
		T *new_item = _origin_item->clone();
		put(new_item);
		_all_buf[0] = new_item;
	}

	T *inner_get()
	{
		locker<spin_lock> l(_lock);

		T *t = _free_queue.pop();
		if (!t && _size < _max_size) {
			T *new_item = _origin_item->clone();
			new_item->set_pool(this);
			_size++;
			_all_buf[_size] = new_item;
			return new_item;
		}

		return t;
	}

	T *get()
	{
		//printf("pool get()\n");
		T *t;
		do {
			t = inner_get();
		} while (!t); // !!!
		//printf("pool got()\n");
		return t;
	}

	/*
	autoput_pointer<T> get_to_be_continue_method()
	{
		T *t = get();
		return autoput_pointer<T>(t, this);
	} */

	bool put(T *t)
	{
		locker<spin_lock> l(_lock);
		//printf("pool put\n");
		_free_queue.push(t);
		//printf("pool puted\n");
	}
};

template <typename T>
class autoput_pointer 
{
	T *_t;
	pool<T>  *_p;
	int _ref_cnt;
public:
	autoput_pointer() : _t(0), _p(0), _ref_cnt(0) {}
	autoput_pointer(T *t, pool<T> *p) : _t(t), _p(p), _ref_cnt(1) {}
	autoput_pointer(autoput_pointer &rhs)
	{
	   	this->_t = rhs._t;
		this->_p = rhs._p;
		this->_ref_cnt = 1;
		rhs->_ref_cnt++;
	}

	autoput_pointer &operator=(autoput_pointer &rhs)
	{
		if (this->_ref_cnt == 1 && this->_p) {
			_p->put(_t);
		}
		this->_t = rhs._t;
		this->_p = rhs._p;
		this->_ref_cnt = 1;
		rhs->_ref_cnt++;
		return *this;
	}

	~autoput_pointer()
	{
		if (_ref_cnt == 1 && _t && _p)
			_p->put(_t);
	}

	T *operator->() { return _t; }
	T &operator*() { return *_t; }
	bool operator !() { return _ref_cnt >= 1 && _t && _p; }
	//T *operator T*() { return _t; }
};
