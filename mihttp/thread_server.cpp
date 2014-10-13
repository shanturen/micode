#include "thread_server.h"
#include <sys/sysinfo.h>

int thread_server2::listener_handler::handle_event(socket_event *se)
{
	//LOG_INFO_VA("handle listener event\n");
	address addr;
	socklen_t length = addr.length();
	while (1) {
		int sock = accept(se->get_handle(), &addr, &length);
		if (sock < 0) {
			if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
				return 3103;
			} else {
				LOG_ERROR_VA("accept error : %s\n", strerror(errno));
				return -1;
			}
		}
		//LOG_INFO_VA("accept <%s,%d>\n", addr.ip().c_str(), addr.port());
		socket_event *client_se = new socket_event(socket_event::read);
		client_se->set_handle(sock);
		client_se->set_event_handler(new thread_server2::client_event_handler(_svr));
		//if (_svr->add_client_socket_event(client_se) != 0) {
		client_se->set_event_manager2(se->get_event_manager2());
		if (se->get_event_manager2()->register_event(client_se) < 0) {
			delete client_se;
			LOG_ERROR_VA("add client error, maybe event-pool full\n");
		}
	}
	return 3103;
}

/*
thread_server2::thread_server2(const string &ip, int port)
{
	//_work_thread_number = 1;
	_work_thread_number = get_nprocs();
	set_listener_address(address(ip, port));
	thread_server2::listener_handler *lh = new thread_server2::listener_handler();
	lh->set_server(this);
	set_listener_handler(lh);

}
*/
thread_server2::thread_server2()
{
	_work_thread_number = get_nprocs();
}

int thread_server2::thread_func()
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

	LOG_ERROR_VA("thread server2 setup\n");

	thread_server2::listener_handler *lh = new thread_server2::listener_handler();
	lh->set_server(this);
	set_listener_handler(lh);


	for (int i = 0; i != _work_thread_number; i++) {
		LOG_ERROR_VA("thread server2 thread setup for %d\n", i);
		thread_server2::work_thread *worker = new work_thread();

		socket_event *listener = new socket_event(socket_event::read);
		listener->set_handle(svr_sock);
		listener->set_event_handler(_listener_handler);
		event_manager2 *m = worker->get_event_manager2();
		listener->set_event_manager2(m);
		m->register_event(listener);

		_work_threads.push_back(worker);
	}

	for (int i = 0; i != _work_thread_number; i++) {
		LOG_ERROR_VA("thread server2 work thread %d start\n", i);
		_work_threads[i]->start();
	}

	for (int i = 0; i != _work_thread_number; i++) {
		LOG_ERROR_VA("thread server2 work thread %d wait\n", i);
		_work_threads[i]->wait();
	}
}

int thread_server2::work_thread::thread_func()
{
	_event_manager.start_event_loop();
}
