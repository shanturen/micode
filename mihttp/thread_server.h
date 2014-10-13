#pragma once

#include "server.h"
#include <vector>
#include <string>
using std::string;
using std::vector;

// new framework: using epoll in multithread,
// each thread has its own event_manager,
class thread_server2 : public server, public pthread {
public:
	class work_thread : public pthread {
		thread_server2 *_svr;

		// each work thread has their own event_manager
		// all event managers share one same event: the listenner
		event_manager2 _event_manager; 
	public:
		work_thread() : _event_manager(new event_manager_impl_epoll()) {}
		event_manager2 *get_event_manager2() { return &_event_manager; }
		int thread_func();
	};

	class listener_handler : public event_handler {
		thread_server2 *_svr;
		event_manager2 *_event_manager;
	public:
		void set_server(thread_server2 *svr) { _svr = svr; }
		int handle_event(socket_event *);
	};

	class client_event_handler: public event_handler{
		thread_server2 *_svr;
	public:
		client_event_handler(thread_server2 *s) : _svr(s) {}
		int handle_event(socket_event *se) { return _svr->handle_client_event(se); }
	};
	

	//thread_server2(const string &ip, int port);
	thread_server2();

	void set_work_thread_number(int n) { _work_thread_number = n; }

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
		for (int i = 0; i != _work_threads.size(); i++) {
			_work_threads[i]->kill();
		}
		return pthread::kill();
	}

	int thread_func();

private:
	int _work_thread_number;
	vector<work_thread *> _work_threads;
	virtual int handle_client_event(socket_event *se) = 0;
};


