#pragma once

#include "server.h"
#include <vector>

// 他妈的，写的真jb烂，极其得难用，
class http_server : public thread_server
{
public:
	typedef int (*path_handler)(const http_request *, response_writer *);

	// read http request form se, 
	// find the path_handler by url in request,
	// call the handler
	// done
	int handle_client_event(socket_event *se);
private:
	class path_and_handler {
	public:
		string path;
		path_handler hdr
	};
	std::vector<path_and_handler> path_handlers;
	register_path_handler(const string &path, path_handler hdr);
};

request *read_request(socket_event *);

class http_request
{
	
};

class http_response
{
};

class response_writer
{
	socket_event *se;
	int write(const http_response &r);
};

http_listener
