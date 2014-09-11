#pragma once

#include "server.h"
#include <vector>

class http_server : public thread_server
{
public:
	typedef int (*path_handler)(const http_request &, response_writer &);
	typedef std::vector<path_and_handler> path_handler_vec_t;

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
	std::vector<path_and_handler> _path_handlers;
	register_path_handler(const string &path, path_handler hdr);
};

class http_request
{
public:
	string method;
	string uri; // uri = [host:port]/[path][?query]
	string version;

	string path;
	string query;

	string header(const string &name);

private:
	std::map<string><string> _headers
	buffer _body_content;

public:
	// request parser 
	typedef enum {parse_request_line = 0, parse_headers, parse_body, parse_ok, parse_fail} parse_state_t;
	static bool parse_request(socket_event *se, http_request &req);
	static bool read_request_buffer(socket_event *se, buffer &buf);
	static int parse_request_line(buffer &buf, http_request *req); 
	static int parse_header(buffer &buf, http_request *req);
	static int parse_body(buffer &buf, http_request *req);
};

class http_response
{
	// status line: HTTP-version SP status SP reason-phrase CRLF
	// [header] CRLF
	// CRLF
	// [body]
};

class response_writer
{
	socket_event *_se;
public:
	response_wirter(socket_event *se) :_se(se) {}
	int write(const http_response &r);
};

http_listener
