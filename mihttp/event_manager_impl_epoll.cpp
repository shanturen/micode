#include "event_manager_impl_epoll.h"
#include <iostream>
#include "log.h"
#include "server.h"
#include "timewrap.h"
#include <errno.h>
using namespace std;

int event_manager_impl_epoll::wait_for_events()
{
	epoll_event evs[256];
	//LOG_INFO_VA("epoll %d wait for event...\n", _epfd);
	int n = 0;
	do {
		n = epoll_wait(_epfd, evs, 256, -1);
		if ( n < 0 && errno == EINTR) {
			continue;
		}
		if (n <= 0)
			return -1;
		break;
	} while (1);


	//LOG_INFO_VA("epoll %d get %d events\n", _epfd, n);
	for (int i = 0; i != n; i++) {
		int slot = evs[i].data.fd;
		socket_event *e = _event_pool.find_by_slot(slot);
		if (!e) {
			LOG_INFO_VA("find event from event-pool error, at slot %d\n", slot);
	//		return 0;
		}
		
		/*
		if (evs[i].events & EPOLLRDHUP) {
			LOG_INFO_VA("event %x EPOLLRDHUP\n", e);
		}
		if (evs[i].events & EPOLLIN) {
			LOG_INFO_VA("event %x EPOLLIN\n", e);
		}
		if (evs[i].events & EPOLLOUT) {
			LOG_INFO_VA("event %x EPOLLOUT\n", e);
		}
		if (evs[i].events & EPOLLERR) {
			LOG_INFO_VA("event %x EPOLLERR\n", e);
		}
		if (evs[i].events & EPOLLHUP) {
			LOG_INFO_VA("event %x EPOLLHUP\n", e);
		}
		*/
		
		/*
		if (evs[i].events & EPOLLRDHUP) {
			cout << "EPOLLRDHUP, " ;
			cout << "we get an EPOLLRDHUP event of " << evs[i].data.fd << endl;
			close(evs[i].data.fd);
		}
		*/
		//e->t1 = timee::now();
		_ready_events.push(e);
	}
	return n;
}

event_pool::event_pool()
{
	for (int i = 0; i != _number_of_slots; i++) {
		_event_slots[i] = 0;
		_slot_use_marks[i] = i;
		_mark_index_of_slot[i] = i;
	}
	_number_of_used_slots = 0;
}

event_pool::~event_pool()
{
	while(_number_of_used_slots != 0) {
		socket_event *e = _event_slots[_slot_use_marks[0]];
		e->get_event_manager()->unregister_event(e);
	}
}

int event_pool::insert(socket_event *e)
{
	if (_number_of_used_slots == _number_of_slots)
		return -1;
	int slot = get_next_empty_slot();
	e->set_slot(slot);
	_event_slots[slot] = e;
	_slot_use_marks[_number_of_used_slots] = slot;
	_mark_index_of_slot[slot] = _number_of_used_slots;
	_number_of_used_slots++;
	//LOG_INFO_VA("[event_pool] used slots:%d\n", _number_of_used_slots);
	return 0;
}

int event_pool::remove_by_slot(int slot)
{
	_event_slots[slot] = 0;
	int mark_index = _mark_index_of_slot[slot];
	int last_used_slot = _slot_use_marks[_number_of_used_slots - 1];
	_slot_use_marks[mark_index] = last_used_slot;
	_mark_index_of_slot[last_used_slot] = mark_index;
	_slot_use_marks[_number_of_used_slots - 1] = slot;
	_number_of_used_slots--;
	return 0;
}

event_manager_impl_epoll::event_manager_impl_epoll()
{
	_epfd = epoll_create(2048);
}

int event_manager_impl_epoll::register_event(socket_event &e)
{
	epoll_event ee;
	int op = EPOLL_CTL_ADD;
	ee.data.u64 = 0UL;
	ee.events = EPOLLET;
	ee.data.fd = _event_pool.get_next_empty_slot();
	if (ee.data.fd == -1)
		return -1;
	if (e.get_type() == socket_event::read)
		ee.events |= EPOLLIN | EPOLLRDHUP;
	if (e.get_type() == socket_event::write)
		ee.events |= EPOLLOUT;
	if (e.get_type() == socket_event::error)
		ee.events |= EPOLLERR;

	//LOG_INFO_VA("epoll %d add %x sock %d at slot %d\n", _epfd, &e, e.get_handle(), ee.data.fd);
	if (epoll_ctl(_epfd, op, e.get_handle(), &ee) != 0)
		return -1;
	_event_pool.insert(&e);
	return 0;
}

int event_manager_impl_epoll::unregister_event(socket_event &e)
{
	int op = EPOLL_CTL_DEL;
	int slot = e.get_slot();
	epoll_event ee;
	ee.data.u64 = 0UL;
	ee.events = 0;
	if (e.get_type() == socket_event::read)
		ee.events &= (~EPOLLIN);
	if (e.get_type() == socket_event::write)
		ee.events &= (~EPOLLOUT);
	if (e.get_type() == socket_event::error)
		ee.events &= (~EPOLLERR);
	//LOG_INFO_VA("epoll %d del sock %d at slot %d\n", _epfd, e.get_handle(), slot);
	if (epoll_ctl(_epfd, op, e.get_handle(), &ee) < 0) {
		LOG_ERROR_VA("epoll_ctl for event %x failed %d, %s\n", &e, errno, strerror(errno));
		return -1;
	}
	_event_pool.remove(&e);
	
	//LOG_INFO_VA("delete event %x sock %d at slot %d\n", &e, e.get_handle(), slot);
	delete &e;
	return 0;
}

socket_event *event_manager_impl_epoll::get_event()
{
	if (_ready_events.empty()) {
		if (wait_for_events() <= 0)
			return 0;
	}
	socket_event *evt = _ready_events.front();
	if (!evt) { 
		LOG_ERROR_VA("bad design\n");
		return 0;
	}
	_ready_events.pop();
	return evt;
}
