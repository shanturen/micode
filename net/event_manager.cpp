#include "event_manager.h"
#include <iostream>
#include "log.h"
using namespace std;

void event_manager::start_event_loop()
{
	static int count = 0;
	socket_event *e;
	LOG_DEBUG_VA("getting event\n");
	while (e = get_event()) {
		LOG_DEBUG_VA("get event %x\n", e);
		e->set_active_time(timee::now());
		if (dispatch(e) < 0) {
			// why can't directly unregister here:
			//  when client close the socket(timeout, or net error, or with no reason), 
			//  some msg_handle_task may running on this event, dce-framework was 
			//  designed so weak that it requires this socket_event alive during 
			//  the task processing, unregister(delete) this event soon(while tsk running) 
			//  may cause program crash
			//e->set_client_closed();
			//e->unregister();
			//continue;

			// epoll may notify the closure event twice, see the url:
			// http://s352.codeinspot.com/q/1551538
			// just ignore the second notify
			if (e->is_first()) {
				e->unset_first();
				if (e->is_delayed()) {
					add_delayed_event(e);
					LOG_DEBUG_VA("client close %x\n", e);
				} else {
					e->unregister();
				}
			} else {
				LOG_INFO_VA("twice close, what the fuck, tell me why\n");
			}
		}
		close_expired_events();
	}
	cout << "get event error, event loop end" << endl;
}

event_manager::~event_manager()
{
	if (_impl)
		delete _impl;
}

int event_manager::register_event(socket_event &evt)
{ 
	_lock.lock(); 
	evt.set_event_manager(this);
	int ret = _impl->register_event(evt);
	_lock.unlock(); 
	return ret; 
}

int event_manager::unregister_event(socket_event &evt)
{
	_lock.lock();
	int ret = _impl->unregister_event(evt);
	_lock.unlock();
	return ret;
}
