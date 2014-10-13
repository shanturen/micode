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
	//socket_event *_svr_listener;
	event_handler *_listener_handler;
	event_manager _event_manager;
	buffer _evt_read_buf;
public:
	server() : _event_manager(new event_manager_impl_epoll())
	{
		_listener_handler = 0;
		//_event_manager.set_server(this);
		
	}
	//int set_sock_address(const address &s) { _svr_addr = s; }
	//address &get_sock_address() { return _svr_addr; }
	virtual ~server()
	{
		LOG_DEBUG_VA("destroy  server1\n");
		/** _svr_listener release in _event_manager::~_event_manager
		if (_svr_listener) // delegate the delete to event_manager
			_event_manager.unregister_event(*_svr_listener);
		*/
		LOG_DEBUG_VA("destroy  server2\n");
	}
	
	void set_listener_address(const address &addr)
	{
			_addr = addr;
	}

	/*
	void set_listener_event(socket_event *evt)
	{
		_svr_listener = evt;
	}
	*/
	void set_listener_handler(event_handler *eh)
	{
		_listener_handler = eh;
		//_listener_handler->set_server(this);
	}

	int add_client_socket_event(socket_event *se)
	{
		//LOG_DEBUG_VA("add client event: %x\n", se);
		return _event_manager.register_event(*se);
	}

	int remove_client_socket_event(socket_event *se)
	{
		//LOG_DEBUG_VA("remove client event: %x\n", se);
		return _event_manager.unregister_event(*se);
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

		_event_manager.register_event(*listener);

		LOG_DEBUG_VA("server::start() event_manager start event loop\n\n");
		_event_manager.start_event_loop();
	}

	buffer &get_buffer() { return _evt_read_buf; }
};

class thread_server : public server, private pthread
{
	thread_pool _tp;

	class listener_handler : public event_handler {
		thread_server *_svr;
	public:
		int set_server(thread_server *svr) { _svr = svr; }
		int handle_event(socket_event *se); // accept and register client event
	};

	class client_task : public task {
		socket_event *_se;
		thread_server *_svr;
	public:
		client_task() : _se(0), _svr(0) {}
		void set_socket_event(socket_event *se) { _se = se; }
		void set_thread_server(thread_server *svr) { _svr = svr; }
		int execute(worker *wkr) {
			_se->t3 = timee::now();
			if (_svr->handle_client_event(_se) < 0) {
				_svr->remove_client_socket_event(_se);
			}
		}
	};

	class client_event_handler : public event_handler
	{
		thread_server *_svr;
	public:
		client_event_handler(thread_server *svr) : _svr(svr) {}
		void set_server(thread_server *svr) { _svr = svr; }
		int handle_event(socket_event *se);
	};
public:
	//thread_server(const string &ip = "", int port = 0) {
	//	set_listener_address(address(ip, port));
	//}

	// each thread server handle their client event differently,
	// the method was called at worker thread.
	virtual int handle_client_event(socket_event *se) = 0;

	int start()
	{
		return pthread::start();
	}
	
	int wait()
	{
		return pthread::wait();
	}
	
	int stop()
	{
		return pthread::kill();
	}

	thread_pool *get_thread_pool()
	{
		return &_tp;
	}

	void set_work_thread_number(int n) { _tp.set_max_threads(n); }

	int thread_func();
};

