#include "http_server.h"
#include <stdlib.h>
#include <ctype.h>

using namespace std;

int http_server::handle_client_event(socket_event *se)
{
	buffer buf;
	if (!http_request::read_request_buffer(se, buf)) {
		return -1;
	}
	http_request req;
	
	if (!http_request::parser_request(buf, req)) {
		return -1;
	}
	http_server::path_handler_vec_t::iterator it;
	for (it = _path_handlers.begin(); it != _path_handlers.end(); it++) {
		if ((*it).path == req->path) {
			response_writer wr(se)
			(*it).second(*req, wr);
			break;
		}
	}

	if (it == _path_handlers.end()) {
		return -1;
	}
}

static bool http_request::read_buffer(socket_event *se, buffer &buf)
{
	bool header_finished = false;
	int header_max_size = 2048;
	int n = 0;
	do {
		int ret = tcp_read_ms(se->get_handle(), &c, 1, 10);
		if (ret == 1) {
			buf.append_data(&c, 1);
			if (++n == header_max_size) {
				return false;
			}
		} else {
			return false;
		}
		if (buf.get_size() > 4) {
			char *p = buf.get_data_buf();
			int l = buf.get_size();
			header_finished = ( *(p+l-4) == '\r' && *(p+l-3) == '\n' *(p+l-2) == '\r' && *(p+l-1) == '\n' )
		}
	} while (!header_finished);

	// get the length of body if present
	string upstr;
	int body_length = 0;
	char *p = buf.get_data_buf();
	for (int i = 0; i !=  buf.get_size(); i++) {
		upstr += tolower(*p++);
	}

	size_t pos = upstr.find("content-length:");
	if (pos != string::npos) {
		lenstr = upstr.substr(pos + 15);
		size_t pos2 = lenstr.find("\r\n");
		if (pos2 != string::npos) {
			body_length = atoi(lenstr.substr(0, pos2));
		}
	}

	if (body_length != 0) {
		buffer body_buf;
		int n = tcp_read_ms(se->get_handle(), body_buf, body_length, 10);
		if (n != body_length) {
			return false;
		}
		buf.append_data(body_buf.get_data_buf(), n);
	}
	return true;
}

static bool http_request::parse_request(buffer &buf, http_request &req)
{
	// a simple, non-standart parse of http request
	parse_state_t state = parse_request_line;

	while (state != parse_ok && state != parse_fail) {
		switch(state) {
		case parse_request_line :
			if (parse_request_line(buf, req) < 0) {
				state = parse_fail;
			} else {
				state = parse_header;
			}
			break
		case parse_header :
			int ret = parse_header(buf, req);
			if (ret < 0) {
				state = parse_fail;
			} else if (ret == 0) { // a empty line parsed
				if (req.header("content-length") == "") {
					state = parse_ok;
				} else {
					state = parse_body;
				}
			}
			break
		case parse_body :
			//int len = atoi(req.header("Content-Length").cstr());
			if (parse_body(buf, req) < 0) {
				state = parse_ok;
			} else {
				state = parse_fail;
			}
			break;
		}
	}

	if (state != parse_ok || buf->get_size() != 0) {
		return false;
	}

	return true;
}

static void skip_sp(buffer &buf, char sp)
{
	char cbuf[32];
	int l = 0;
	int buf_len = buf.get_size();
	char *p = (char *)buf->get_data_buf();
	bool skiped = false;
	

	do {
		for (l = 0; *p++ == sp && l != 32 && l < buf_len ; l++);
		if (l == 0 || l == buf_len) {
			skiped = true;
		}

		if (l != 0) {
			buf.drain_data(cbuf, l);
			buf_len -= l;
		}
	} while(!skiped);
}

static inline bool drop_crlf(buffer &buf)
{
	char cbuf[2];
	char *p = (char *)buf.get_data_buf();
	if (*p != '\r' || *(p+1) != '\n') {
		return false;
	}
	buf.drain_data(cbuf, 2);
	return true;
}

static int http_request::parse_request_line(buffer &buf, http_request *req)
{
	char cbuf[2048];
	char sp = ' ';
	int buf_len = buf.get_size();
	int l = 0;
	char *p = (char *)buf.get_data_buf();

	// method
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != sp && l != buf.get_size() && l != 30; l++);
	if (l == 30|| l == buf_len) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	req->method = string(cbuf);

	// uri
	skip_sp(buf, ' ');
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != sp && l != buf.get_size() && l != 2048; l++);
	if (l == 2048 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	req->uri = string(cbuf);

	// version
	skip_sp(buf, ' ');
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != '\r' && l != buf.get_size() && l != 20; l++);
	if (l == 20 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	req->uri = string(cbuf);
	
	if (!drop_crlf(buf))
		return -1;
	return 0;
}

// token:what ever value\r\n 
// or
// \r\n
//
// return 1 for general header
// return 0 for empty line(\r\n)
// return -1 for bad format
static int http_request::parse_header(buffer &buf, http_request *req)
{
	char cbuf[256];
	char *p;
	int l = 0;


	p = buf.get_data_buf();
	if (buf.get_size() > 2) {
		if (*p == '\r' && *(p+1) == '\n') {
			buf.drain_data(cbuf, 2);
			return 0;
		}
	}

	// name 
	string name, value;
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != ':' && l != buf.get_size() && l != 64; l++);
	if (l == 64 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	for (int i = 0; i != l; i++) {
		cbuf[i] = tolower(cbuf[i]);
	}
	name = string(cbuf);
	skip_sp(buf, ':');

	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != '\r' && l != buf.get_size() && l != 256; l++);
	if (l == 256 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	value = string(cbuf);

	req->_headers.insert(map<string, string>::value_type(name, value));

	if (!drop_crlf(buf))
		return -1;

	return 1;
}

static int http_request::parse_body(buffer &buf, http_request *req)
{
	int body_len = atoi(req->header("content-length"));
	if (buf.get_size() != body_len){
		return -1;
	}
	req->_body_content = buf;
	return 0;
}

string http_request::header(const string &name)
{
	map<string, string>::iterator it = _headers.find(name);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}
