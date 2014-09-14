#include "server.h"
#include "log.h"

int thread_server::listener_handler::handle_event(socket_event *se)
{
	LOG_INFO_VA("handle listener event\n");
	address addr;
	socklen_t length = addr.length();
	while (1) {
		int sock = accept(se->get_handle(), &addr, &length);
		if (sock < 0) {
			if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
				return 3103;
			} else {
				LOG_ERROR_VA("accept error\n");
				return -1;
			}
		}
		LOG_INFO_VA("accept <%s,%d>\n", addr.ip().c_str(), addr.port());
		socket_event *client_sock_event = new socket_event(socket_event::read);
		client_sock_event->set_handle(sock);
		client_sock_event->set_event_handler(new thread_server::client_event_handler(_svr));
		if (_svr->add_client_socket_event(client_sock_event) != 0) {
			delete client_sock_event;
			LOG_ERROR_VA("add client error, maybe event-pool full\n");
		}
	}
	return 3103;
}

int thread_server::client_event_handler::handle_event(socket_event *se)
{
	// create client_task 
	// add to server thread pool, and waiting for executing 
	
	thread_server::client_task *tsk = new thread_server::client_task();
	tsk->set_socket_event(se);
	tsk->set_thread_server(_svr);
	tsk->start(_svr->get_thread_pool());
	return 0;
}

int thread_server::thread_func()
{
	LOG_DEBUG_VA("server thread start\n");
	thread_server::listener_handler *eh = new thread_server::listener_handler();
	eh->set_server(this);
	set_listener_handler(eh);
	server::start();
}
