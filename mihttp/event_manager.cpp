#include "event_manager.h"
#include <iostream>
#include "log.h"
using namespace std;

void event_manager::start_event_loop() {
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

event_manager::~event_manager()
{
	if (_impl)
		delete _impl;
}
