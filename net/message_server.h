#include "server.h"

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
};
