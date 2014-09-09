#pragma once
#include "event_manager.h"
#include "event_manager_impl_epoll.h"
#include "message.h"
#include "thread.h"
#include <iostream>
#include <map>
#include "log.h"
using namespace std;

static const int tcp_read_timeout = 1000;

class server
{
	socket_event *_svr_listener;
	event_manager _event_manager;
	buffer _evt_read_buf;
public:
	server() : _event_manager(new event_manager_impl_epoll())
	{
		_svr_listener = 0;
		_event_manager.set_server(this);
		
	}
	//int set_sock_address(const address &s) { _svr_addr = s; }
	//address &get_sock_address() { return _svr_addr; }
	virtual ~server()
	{
		LOG_DEBUG_VA("destroy  server1");
		/** _svr_listener release in _event_manager::~_event_manager
		if (_svr_listener)
			_event_manager.unregister_event(*_svr_listener);
		*/
		LOG_DEBUG_VA("destroy  server2");
	}
	
	int add_client_socket_event(socket_event *se)
	{
		LOG_DEBUG_VA("add client event: %x", se);
		return _event_manager.register_event(*se);
	}

	void set_listener_event(socket_event *evt)
	{
		_svr_listener = evt;
	}

	int startup()
	{
		_event_manager.register_event(*_svr_listener);
		_event_manager.start_event_loop();
	}

	buffer &get_buffer() { return _evt_read_buf; }
};

class message_server;
class message_read_event_handler : public read_event_handler
{
	message_server *_svr;
public:
	message_read_event_handler(message_server *svr) : _svr(svr) {}
	int set_server(message_server *svr) { _svr = svr; }
	int read(socket_event *se);
};

class message_server;
class listener_handler : public read_event_handler
{
	message_server *_svr;
public:
	int set_server(message_server *svr) { _svr = svr; }
	int read(socket_event *se);
};

class message_server : public server, private pthread
{
	thread_pool _msg_thread_pool;
	//server_read_event_handler *_server_read_event_handler;
	std::map<int, message_builder_base *> _msg_builder_map;
	address _addr;
public:
	virtual ~message_server();
	void set_listener_address(const address &addr) { _addr = addr; }
	thread_pool *get_thread_pool() { return &_msg_thread_pool; }

	void register_message_builder(int command, message_builder_base *mb)
	{
		_msg_builder_map.insert(std::map<int, message_builder_base *>::value_type(command, mb));
	}

	message_builder_base *find_message_builder(int command)
	{
		std::map<int, message_builder_base *>::iterator it = _msg_builder_map.find(command);
		if (it == _msg_builder_map.end())
			return 0;
		return it->second;
	}

	void set_msg_worker(message_worker *msgwkr)
	{
		_msg_thread_pool.set_worker(msgwkr);
	}
	
	int thread_func() 
	{
		int svr_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (svr_sock < 0) {
			LOG_ERROR_VA("create server socket failed");
			return -1;
		}
		int flag = 1;
		setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
		if (bind(svr_sock, &_addr, _addr.length()) < 0) {
			LOG_ERROR_VA("bind address failed");
			return -1;
		}
		
		int sockflag = fcntl(svr_sock, F_GETFL, 0);
		fcntl(svr_sock, F_SETFL, sockflag | O_NONBLOCK);
		
		if (listen(svr_sock, 100) < 0) {
			LOG_ERROR_VA("listen failed");
			return -1;
		}
		socket_event *listener = new socket_event(socket_event::read);
		listener->set_handle(svr_sock);
		listener_handler *eh = new listener_handler();
		eh->set_server(this);
		listener->set_event_handler(eh);
		set_listener_event(listener);
		server::startup();
	}

	int startup()
	{
		return pthread::start();
	}
	
	int waitend()
	{
		return pthread::wait();
	}
	
	int stop()
	{
		return pthread::kill();
	}
};
