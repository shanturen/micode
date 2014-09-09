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
	server *_svr;
	event_manager_impl *_impl;
	spin_lock _lock;
	int dispatch(socket_event *e)
	{
		return e->get_event_handler()->handle_event(e);
	}

private:
	std::vector<socket_event *> _delayed_close_events;
	static const int delayed_msecs = 2000;
	void add_delayed_event(socket_event *evt)
	{
		LOG_INFO_VA("add delayed close event %x", evt);
		evt->set_active_time(timee::now());
		_delayed_close_events.push_back(evt);
	}
	void close_expired_events()
	{
		static int i = 0;
		if (i++ % 10 == 0)
			LOG_INFO_VA("%d delayed events in queue", _delayed_close_events.size());

		/*
		while (!_delayed_close_events.empty()) {
			socket_event *e = _delayed_close_events.front();
			if (((timee::now() - e->get_active_time()) > (delayed_msecs * 1000) && (!(e->is_delayed())))) {
				LOG_INFO_VA("delayed close client event %x", e);
				_delayed_close_events.pop();
				e->unregister();
			} else 
				break;
		}
		*/
		int n = _delayed_close_events.size();
		int index = 0;
		for (int i = 0; i != n; i++) {
			socket_event *e = *(_delayed_close_events.begin() + index);
			if (!e->is_delayed()) {
				LOG_INFO_VA("delayed close client event %x", e);
				_delayed_close_events.erase(_delayed_close_events.begin() + index);
				e->unregister();
				index--;
			}
			index++;
		}
	}
public:
	event_manager(event_manager_impl *impl) : _impl(impl) {}
	~event_manager();
	int register_event(socket_event &evt);
	int unregister_event(socket_event &evt);
	socket_event *get_event() { return _impl->get_event(); }
	void start_event_loop();
	void set_server(server *svr) { _svr = svr; _impl->set_server(svr); }
	server *get_server() { return _svr; }

};
