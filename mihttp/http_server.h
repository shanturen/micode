#pragma once

#include "server.h"
#include <vector>
#include <stdio.h>
#include "buffer.h"
#include <string>
#include <map>
#include "thread_server.h"

class http_request
{
public:
	string method;
	string uri; // uri = [host:port]/[path][?query]
	string version;

	string path;
	string query;

	string header(const string &name) const;
	string form(const string &key) const;

	static const int max_request_size = 8192;

private:
	std::map<std::string, std::string> _headers;
	std::map<std::string, std::string> _form_datas;
	buffer _body_content;

public:
	// request parser 
	static bool parse_request(buffer &buf, http_request &req);
	static bool read_request_buffer(socket_event *se, buffer &buf);
	static int parse_request_line(buffer &buf, http_request &req);
	static int parse_header(buffer &buf, http_request &req);
	static int parse_body(buffer &buf, http_request &req);

	bool parse_form();

private:
	void insert_form_data_if_not_exist(const string &k, const string &v);
};

class http_response
{
	// status line: HTTP-version SP status SP reason-phrase CRLF
	// [header] CRLF
	// CRLF
	// [body]
	string _version;
	string _status;
	string _phrase;
	std::map<std::string, std::string> _headers;
	buffer _body_buf;
public:
	friend class response_writer;
	http_response() {
		_version = "HTTP/1.1";
		_status = "200";
		_phrase= "OK";
		set_header("Server", "mihttp 0.1");
		set_header("Content-Length", "0");
		set_header("Connection", "keep-alive");
	}
	void set_version(const string &version) { _version = version; }
	void set_status(const string &st, const string &phrase)
	{
		_status = st;
		_phrase = phrase;
	}
	void set_header(const string &name, const string &value)
	{
		if (_headers.find(name) != _headers.end())
			_headers[name] = value;
		else
			_headers.insert(map<string, string>::value_type(name, value));
	}
	void set_body(const buffer &buf)
	{
		_body_buf = buf;
		_body_buf.append_string("\r\n");
		if (_body_buf.get_size() != 0) {
			char cbuf[20];
			sprintf(cbuf, "%d", _body_buf.get_size());
			//printf("body modify header, Content-Length: %s\n", cbuf);
			set_header("Content-Length", cbuf);
		}
	}

	void set_body_string(const string &s)
	{
		buffer b;
		b.append_string(s);
		set_body(b);
	}
};

class response_writer
{
	socket_event *_se;
public:
	response_writer(socket_event *se) :_se(se) {}
	int write(const http_response &r);
};


class http_server : public thread_server2
{
	typedef int (*path_handler)(const http_request &, response_writer &);
	class path_and_handler {
	public:
		string path;
		path_handler hdr;
	};
	typedef std::vector<path_and_handler> path_handler_vec_t;

	std::vector<path_and_handler> _path_handlers;

public:
	http_server(const string &ip, int port) { set_listener_address(address(ip, port)); }
	int handle_client_event(socket_event *se);
	void register_path_handler(const string &path, path_handler hdr);
};
