#include "event_manager.h"
#include <iostream>
#include "log.h"
using namespace std;

void event_manager::start_event_loop()
{
	static int count = 0;
	socket_event *e;
	//LOG_DEBUG_VA("getting event\n");
	while (e = get_event()) {
		//LOG_DEBUG_VA("get event %x\n", e);
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
					//LOG_DEBUG_VA("client close %x\n", e);
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
	//printf("register event %x\n", &evt);
	int ret = _impl->register_event(evt);
	_lock.unlock(); 
	return ret; 
}

int event_manager::unregister_event(socket_event &evt)
{
	/* simple version, can not solve twice close event, see
	 * http://stackoverflow.com/questions/4724137/epoll-wait-receives-socket-closed-twice-read-recv-returns-0
	 */
#if 0
	_lock.lock();
	printf("unregister event %x\n", &evt);
	int ret = _impl->unregister_event(evt);
	_lock.unlock();
	return ret;
#else
	_lock.lock();
	if (evt.is_first()) {
		evt.unset_first();
		//if (evt.is_delayed()) {
			add_delayed_event(&evt);
			LOG_DEBUG_VA("client close %x\n", &evt);
	//	} else {
	//		_lock.unlock();
	//		return evt.unregister();
	//	}
	} else {
		LOG_INFO_VA("twice close, what the fuck, tell me why\n");
	}
	_lock.unlock();
	return 0;
#endif
}

void event_manager::add_delayed_event(socket_event *evt)
{
	LOG_INFO_VA("add delayed close event %x\n", evt);
	evt->set_active_time(timee::now());
	_delayed_close_events.push_back(evt);
}

void event_manager::close_expired_events()
{
	static int i = 0;

	// if (i++ % 10 == 0)
	//	LOG_INFO_VA("%d delayed events in queue\n", _delayed_close_events.size());

	/*
	   while (!_delayed_close_events.empty()) {
	   socket_event *e = _delayed_close_events.front();
	   if (((timee::now() - e->get_active_time()) > (delayed_msecs * 1000) && (!(e->is_delayed())))) {
	   LOG_INFO_VA("delayed close client event %x\n", e);
	   _delayed_close_events.pop();
	   e->unregister();
	   } else 
	   break;
	   }
	   */
	_lock.lock();
	timee t = timee::now();
	int n = _delayed_close_events.size();
	int index = 0;
	for (int i = 0; i != n; i++) {
		socket_event *e = *(_delayed_close_events.begin() + index);
		if (t - e->get_active_time() >= 50000) { // 50ms
			LOG_INFO_VA("delayed close client event %x\n", e);
			_delayed_close_events.erase(_delayed_close_events.begin() + index);
			//e->unregister();
			_impl->unregister_event(*e);
			index--;
		} else {
			break;
		}
		index++;
	}
	_lock.unlock();
}

void event_manager2::start_event_loop() {
	socket_event *e;
	while (e = get_event()) {
		event_handler *eh = e->get_event_handler();
		if (eh) {
			if (eh->handle_event(e) < 0) {
				unregister_event(e);
			}
		}
	}
	LOG_DEBUG_VA("event loop end\n");
}

event_manager2::~event_manager2()
{
	if (_impl)
		delete _impl;
}
