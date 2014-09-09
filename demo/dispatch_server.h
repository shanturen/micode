#include "dce.h"

class message_dispatcher
{
	server_proxy_cluster
};

class pure_message : public message
{
	buffer _body_buf
public:
	void set_buffer(const buffer &buf)
	{
		_body_buf = buf
	}
	int marshal(buffer &buf)
	{
		return 0;
	}
	int unmarshal(buffer &buf)
	{
		return 0;
	}
};

class listenner_handler : public listener_handler
{
public:
	int read(socket_event *se);
};

class dispatch_server : public server, private pthread
{
	thread_pool _tp;
public:
	
};

