#pragma once
#include "event_manager.h"
#include "event_manager_impl_epoll.h"
#include "message.h"
#include "thread.h"
#include <iostream>
#include <map>
#include "log.h"
#include "timewrap.h"
using namespace std;

static const int tcp_read_timeout = 1000;

class server
{
protected:
	address _addr;
	event_handler *_listener_handler;
	event_manager _event_manager;
	buffer _evt_read_buf;
public:
	server() : _event_manager(new event_manager_impl_epoll())
	{
		_listener_handler = 0;
		
	}

	virtual ~server()
	{
		LOG_DEBUG_VA("destroy  server1\n");
		
		LOG_DEBUG_VA("destroy  server2\n");
	}
	
	void set_listener_address(const address &addr)
	{
			_addr = addr;
	}

	void set_listener_handler(event_handler *eh)
	{
		_listener_handler = eh;
	}

	int add_client_socket_event(socket_event *se)
	{
		//LOG_DEBUG_VA("add client event: %x\n", se);
		return _event_manager.register_event(se);
	}

	int remove_client_socket_event(socket_event *se)
	{
		//LOG_DEBUG_VA("remove client event: %x\n", se);
		return _event_manager.unregister_event(se);
	}

	int start()
	{
		LOG_DEBUG_VA("server::start()\n\n");
		int svr_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (svr_sock < 0) {
			LOG_ERROR_VA("create server socket failed\n");
			return -1;
		}
		int flag = 1;
		setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
		if (bind(svr_sock, &_addr, _addr.length()) < 0) {
			LOG_ERROR_VA("bind address failed\n");
			return -1;
		}
		
		int sockflag = fcntl(svr_sock, F_GETFL, 0);
		fcntl(svr_sock, F_SETFL, sockflag | O_NONBLOCK);
		
		if (listen(svr_sock, 100) < 0) {
			LOG_ERROR_VA("listen failed\n");
			return -1;
		}
		socket_event *listener = new socket_event(socket_event::read);
		listener->set_handle(svr_sock);
		listener->set_event_handler(_listener_handler);

		_event_manager.register_event(listener);

		LOG_DEBUG_VA("server::start() event_manager start event loop\n\n");
		_event_manager.start_event_loop();
	}

	buffer &get_buffer() { return _evt_read_buf; }
};
