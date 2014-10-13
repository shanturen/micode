#pragma once
#include "socket_event.h"
#include "spinlock.h"
#include <queue>
#include <list>
#include "timewrap.h"
#include "log.h"

class server; // define in server.h
class event_manager_impl
{
	server *_svr;
public:
	virtual ~event_manager_impl() {};
	virtual int register_event(socket_event &evt) = 0;
	virtual int unregister_event(socket_event &evt) = 0;
	virtual socket_event *get_event() = 0;
	void set_server(server *svr) { _svr = svr; }
	server *get_server() { return _svr; }
};

class event_manager
{
	//server *_svr;
	event_manager_impl *_impl;
	spin_lock _lock;
	int dispatch(socket_event *e)
	{
		return e->get_event_handler()->handle_event(e);
	}

private:
	std::vector<socket_event *> _delayed_close_events;
	static const int delayed_msecs = 2000;
	void add_delayed_event(socket_event *evt);
	void close_expired_events();
public:
	event_manager(event_manager_impl *impl) : _impl(impl) {}
	~event_manager();
	int register_event(socket_event &evt);
	int unregister_event(socket_event &evt);
	socket_event *get_event() { return _impl->get_event(); }
	void start_event_loop();
	//void set_server(server *svr) { _svr = svr; _impl->set_server(svr); }
	//server *get_server() { return _svr; }

};

class event_manager2 {
	event_manager_impl *_impl;
public:
	event_manager2(event_manager_impl *impl) : _impl(impl) {}
	~event_manager2();
	int register_event(socket_event *evt) {return _impl->register_event(*evt); }
	int unregister_event(socket_event *evt) {return _impl->unregister_event(*evt); }
	socket_event *get_event() { return _impl->get_event(); }
	void start_event_loop();
};
