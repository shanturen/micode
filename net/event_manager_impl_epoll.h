#pragma once
#include "event_manager.h"
#include <sys/epoll.h>
#include <queue>
using std::queue;

class event_pool
{
	static const int _number_of_slots = 2048;
	socket_event *_event_slots[_number_of_slots];
	int _slot_use_marks[_number_of_slots];
	int _mark_index_of_slot[_number_of_slots];
	int _number_of_used_slots;
public:
	event_pool(); 
	~event_pool();
	socket_event *find_by_slot(int slot) { return _event_slots[slot]; }
	int insert(socket_event *e);
	int remove_by_slot(int slot);
	int remove(socket_event *e) { return e ? remove_by_slot(e->get_slot()) : 0; }
	int get_next_empty_slot() { return _number_of_used_slots == _number_of_slots ? -1 : _slot_use_marks[_number_of_used_slots]; }
	int get_mark_index_of_slot(int slot) { return _mark_index_of_slot[slot]; };
};

class event_manager_impl_epoll : public event_manager_impl
{
	static const int max_number_of_events = 2048;
	int _epfd;
	event_pool _event_pool;
	std::queue<socket_event *> _ready_events;
public : 
	event_manager_impl_epoll();
	int register_event(socket_event &evt);
	int unregister_event(socket_event &evt);
	socket_event *get_event();
	int wait_for_events();
};

