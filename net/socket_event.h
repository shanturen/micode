#pragma once
#include "system.h"
#include "socket.h"
#include "thread.h"
#include "timewrap.h"

class event_manager;
class event_manager_impl;
class event_handler;
class socket_event
{
public:
	friend class event_manager;
	friend class event_manager_impl;
	typedef enum {read, write, error} event_type_t;
	socket_event(event_type_t type)
	{
		_type = type;
		_slot = -1;
		_io_handle = -1;
		_is_delayed = false;
		_event_handler = 0;
		_event_manager = 0;
		_is_first_closure = true;
	}
	~socket_event(); 
	void set_event_handler(event_handler *h) { _event_handler = h; }
	event_handler *get_event_handler() { return _event_handler; }
	void set_handle(int fd) { _io_handle = fd; }
	int get_handle() { return _io_handle; }
	int get_slot() { return _slot; }
	int set_slot(int slot) { _slot = slot; }
	int get_type() { return _type; }
	
	void set_active_time(const timee &t) { _active_time = t; }
	timee &get_active_time() { return _active_time; }
	void set_event_manager(event_manager *mgr) { _event_manager = mgr; }
	bool is_delayed() { return _is_delayed; }
	void set_delayed() { _is_delayed = true; }
	void unset_delayed() { _is_delayed = false; }
	void unset_first() { _is_first_closure = false; }
	bool is_first() { return _is_first_closure; }
	event_manager *get_event_manager() { return _event_manager; }
	int unregister();
private:
	event_type_t _type;
	int _slot;
	int _io_handle;
	timee _active_time;
	bool _is_delayed;
	bool _is_first_closure;
	event_handler *_event_handler;
	event_manager *_event_manager;
};

class event_handler
{
public:
	virtual int handle_event(socket_event *e) = 0;
};

class read_event_handler : public event_handler
{
public:
	
	int handle_event(socket_event *e)
	{
		int ret = read(e);
		if (ret <= 0) {
			/* 
			 * 
			 * if close would cause a error event, then handle unregister in that event 
			 *
			 * // expect event manager feel the close, test report that it can't feel
			 * close(e->get_handle());  
			 *
			 */
			return -1;
		}
		return ret;
	}
	
	// return : <= 0, represent client close or invalid message, which would cause event manager destroy this event
	virtual int read(socket_event *e) = 0;
};


class write_event_handler : public socket_event
{
public:	
	int handle_event() {};
};

