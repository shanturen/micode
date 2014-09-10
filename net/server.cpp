#include "server.h"
#include "log.h"

int listener_handler::read(socket_event *se)
{
	address addr;
	socklen_t length = addr.length();
	while (1) {
		int sock = accept(se->get_handle(), &addr, &length);
		if (sock < 0) {
			if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
				return 3103;
			} else {
				LOG_ERROR_VA("accept error");
				return -1;
			}
		}
		LOG_INFO_VA("accept <%s,%d>", addr.ip().c_str(), addr.port());
		socket_event *client_sock_event = new socket_event(socket_event::read);
		client_sock_event->set_handle(sock);
		client_sock_event->set_event_handler(new message_read_event_handler(_svr));
		if (_svr->add_client_socket_event(client_sock_event) != 0) {
			delete client_sock_event;
			LOG_ERROR_VA("add client error, maybe event-pool full");
		}
	}
	return 3103;
}

int message_read_event_handler::read(socket_event *se)
{
	error_response_message emsg;
	LOG_INFO_VA("new request message coming in...");
	head msg_head;
	int sock = se->get_handle();
	int ret = tcp_read_ms(sock, &msg_head, sizeof(msg_head), tcp_read_timeout);
	if (ret <= 0) {
		LOG_INFO_VA("read head error or client closed");
		return ret;
	}
	if (ret != sizeof(msg_head)) {
		LOG_ERROR_VA("bad request, refused to service");
		return -1;
	}
	msg_head.ntoh();
	LOG_DEBUG_VA("likly msg head, cmd:%d, vr:%d, src:%d, st:%d, len:%d, sn:%llu-%llu", 
	msg_head.command, msg_head.version, msg_head.src, msg_head.status, msg_head.length, msg_head.sn_high, msg_head.sn_low);

	message_builder_base *msg_builder = _svr->find_message_builder(msg_head.command);
	if (!msg_builder) {
		LOG_ERROR_VA("unsupporeted message command %d", msg_head.command);
		emsg.set_error_str("bad msg format");
		send_msg(se->get_handle(), emsg);
		return -1;
	}

	buffer &buf = _svr->get_buffer();
	ret = tcp_read_ms(sock, buf, msg_head.length, tcp_read_timeout);
	if (ret != msg_head.length) {
		LOG_ERROR_VA("message body receive failed, recv:%d bytes, expect:%d, maybe bad format",
		ret, msg_head.length);
		emsg.set_error_str("bad msg body length");
		send_msg(se->get_handle(), emsg);
		return -1;
	}


	//message *msg = _svr.get_message_builder()->build_message(msg_head, buf);
	message *msg = msg_builder->build_message(msg_head, buf);
	if (!msg) {
		LOG_ERROR_VA("message unmarshal failed");
		emsg.set_error_str("bad msg body");
		send_msg(se->get_handle(), emsg);
		return -1;
	}
	msg->set_socket_event(se);
	task *tsk = new msg_handle_task(msg);
	LOG_DEBUG_VA("create msg task for msg %d", msg->get_id());
	tsk->start(_svr->get_thread_pool());
	return 3103;
}

message_server::~message_server()
{
	LOG_DEBUG_VA("destroy message_server");
	for(std::map<int, message_builder_base *>::iterator it = _msg_builder_map.begin(); it != _msg_builder_map.end(); it++) {
		delete it->second;
	}
	LOG_DEBUG_VA("destroyed message_server");
}

int thread_server::listener_handler::handle_event(socket_event *se)
{
	address addr;
	socklen_t length = addr.length();
	while (1) {
		int sock = accept(se->get_handle(), &addr, &length);
		if (sock < 0) {
			if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
				return 3103;
			} else {
				LOG_ERROR_VA("accept error");
				return -1;
			}
		}
		LOG_INFO_VA("accept <%s,%d>", addr.ip().c_str(), addr.port());
		socket_event *client_sock_event = new socket_event(socket_event::read);
		client_sock_event->set_handle(sock);
		client_sock_event->set_event_handler(new thread_server::client_event_handler(_svr));
		if (_svr->add_client_socket_event(client_sock_event) != 0) {
			delete client_sock_event;
			LOG_ERROR_VA("add client error, maybe event-pool full");
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
