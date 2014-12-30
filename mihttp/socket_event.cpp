#include "socket_event.h"
#include "event_manager.h"
#include <iostream>
using namespace std;

socket_event::~socket_event()
{
	_slot = -1;
	_event_manager = 0;
	if (_io_handle >= 0) {
		close(_io_handle);
		_io_handle = -1;
	}
	if (_event_handler) {
		delete _event_handler;
		_event_handler = 0;
	}
}

int socket_event::unregister()
{
	return _event_manager->unregister_event(this);
}
