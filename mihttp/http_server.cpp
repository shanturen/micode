#include "http_server.h"
#include <stdlib.h>
#include <ctype.h>
#include "url.h"

using namespace std;

// -1 on non-http request or client close
// 0 on ok or logical error
int http_server::handle_client_event(socket_event *se)
{
	buffer buf;
	//timee t1 = timee::now();
	if (!http_request::read_request_buffer(se, buf)) {
		//printf("read http request buffer error or client closed\n");
		return -1;
	}
	//printf("request read cost : %d us\n", timee::now() - t1);
	http_request req;
	
	if (!http_request::parse_request(buf, req)) {
		return -1;
	}

	if (!req.parse_form()) {
		return -1;
	}

	http_server::path_handler_vec_t::iterator it;
	for (it = _path_handlers.begin(); it != _path_handlers.end(); it++) {
		if ((*it).path == req.path) {
			response_writer wr(se);
			(*it).hdr(req, wr);
			break;
		}
	}

//	se->t4 = timee::now();
//	printf("t1 + %d + %d + %d\n", se->t2 - se->t1, se->t3 - se->t2, se->t4 - se->t3);

	if (it == _path_handlers.end()) {
		return -1;
	}

	// close if client need no keep-alive, 
	if (req.header("connection") != "keep-alive") {
		return -1;
	}

	return 0;
}

bool http_request::read_request_buffer(socket_event *se, buffer &buf)
{
	//int ret = tcp_read_ms_once(se->get_handle(), buf, http_request::max_request_size, 5);
	//return ret > 0;

	// can't stand the low performance
	bool header_finished = false;
	int header_max_size = 2048;
	int n = 0;
	do {
		char c;
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
			char *p = (char *)buf.get_data_buf();
			int l = buf.get_size();
			header_finished = (*(p+l-4) == '\r' && *(p+l-3) == '\n' && *(p+l-2) == '\r' && *(p+l-1) == '\n');
		}
	} while (!header_finished);

	// get the length of body if present
	string upstr;
	int body_length = 0;
	char *p = (char *)buf.get_data_buf();
	for (int i = 0; i !=  buf.get_size(); i++) {
		upstr += tolower(*p++);
	}

	size_t pos = upstr.find("content-length:");
	if (pos != string::npos) {
		string lenstr = upstr.substr(pos + 15);
		size_t pos2 = lenstr.find("\r\n");
		if (pos2 != string::npos) {
			body_length = atoi(lenstr.substr(0, pos2).c_str());
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

bool http_request::parse_request(buffer &buf, http_request &req)
{
	// a simple, non-standart parse of http request
	enum {state_request_line = 0, state_headers, state_body, state_ok, state_fail} state = state_request_line;
	int ret = 0;

	while (state != state_ok && state != state_fail) {
		switch(state) {
		case state_request_line :
			if (parse_request_line(buf, req) < 0) {
				state = state_fail;
			} else {
				state = state_headers;
			}
			break;
		case state_headers :
			ret = parse_header(buf, req);
			if (ret < 0) {
				state = state_fail;
			} else if (ret == 0) { // a empty line parsed
				if (req.header("content-length") == "") {
					state = state_ok;
				} else {
					state = state_body;
				}
			}
			break;
		case state_body :
			//int len = atoi(req.header("Content-Length").cstr());
			if (parse_body(buf, req) < 0) {
				state = state_fail;
			} else {
				state = state_ok;
			}
			break;
		}
	}

	if (state != state_ok || buf.get_size() != 0) {
		return false;
	}

	return true;
}

static void skip_sp(buffer &buf, char sp)
{
	char cbuf[32];
	int l = 0;
	int buf_len = buf.get_size();
	char *p = (char *)buf.get_data_buf();
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

int http_request::parse_request_line(buffer &buf, http_request &req)
{
	//printf("buf:%s\n", (char *)buf.get_data_buf());
	char cbuf[http_request::max_request_size];
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
	req.method = string(cbuf);

	// uri
	skip_sp(buf, ' ');
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != sp && l != buf.get_size() && l != http_request::max_request_size; l++);
	if (l == http_request::max_request_size|| l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	req.uri = string(cbuf);

	// version
	skip_sp(buf, ' ');
	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != '\r' && l != buf.get_size() && l != 20; l++);
	if (l == 20 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	req.version = string(cbuf);
	
	if (!drop_crlf(buf))
		return -1;

	size_t pos = req.uri.find("?");
	if (pos == string::npos) {
		req.path = req.uri;
	} else {
		req.path = req.uri.substr(0, pos);
		req.query = req.uri.substr(pos+1);
	}

	return 0;
}

// token:[what ever value]\r\n 
//
// return 1 for general header
// return 0 for empty line(\r\n)
// return -1 for bad format
int http_request::parse_header(buffer &buf, http_request &req)
{
	char cbuf[256];
	char *p;
	int l = 0;


	p = (char *)buf.get_data_buf();
	if (buf.get_size() >= 2) {
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
	skip_sp(buf, ' ');

	p = (char *)buf.get_data_buf();
	for (l = 0; *p++ != '\r' && l != buf.get_size() && l != 256; l++);
	if (l == 256 || l == buf.get_size()) {
		return -1;
	}
	buf.drain_data(cbuf, l);
	cbuf[l] = '\0';
	value = string(cbuf);

	req._headers.insert(map<string, string>::value_type(name, value));

	if (!drop_crlf(buf))
		return -1;

	return 1;
}

int http_request::parse_body(buffer &buf, http_request &req)
{
	int body_len = atoi(req.header("content-length").c_str());
	if (buf.get_size() != body_len){
		return -1;
	}
	req._body_content = buf;
	buf.drain_all_data();
	return 0;
}

string http_request::header(const string &name) const
{
	map<string, string>::const_iterator it = _headers.find(name);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}

string http_request::form(const string &name) const
{
	map<string, string>::const_iterator it = _form_datas.find(name);
	if (it != _form_datas.end()) {
		return it->second;
	}
	return "";
}

bool http_request::parse_form()
{
	const char *data = 0;
	int size = 0;
	char buf[http_request::max_request_size];
	string key, value;
	if (this->method == "GET") {
		if (this->query.empty())
			return true;
		data = this->query.c_str();
		size = this->query.size();
	}

	if (this->method == "POST" && this->header("content-type") == "application/x-www-form-urlencoded") {
		if (this->_body_content.get_size() == 0)
			return true;
		data = (const char *)(this->_body_content.get_data_buf());
		size = this->_body_content.get_size();
		//printf("form data: %s\n", data);
	}

	char k_or_v = 'k';
	int i = 0, bi = 0;
	// data = "k1=v1&k2=v2" with size long 
	// valid format : &&k=v&k=&&&k=
	for (i = 0; i != size; i++) {
		if (k_or_v == 'k') {
			if (data[i] == '&') {
				if (bi == 0) 
					continue; // ignore the &
				else {
					break;
				}
			} else if (data[i] == '=') {
				if (bi == 0) {
					break;
				} else {
					key = string(buf, buf + bi);
					k_or_v = 'v';
					bi = 0;
				}
			} else {
				buf[bi++] = data[i];
			}
		} else if (k_or_v == 'v') { // v
			if (data[i] == '&') {
				if (bi != 0) {
					value = string(buf, buf + bi);
				} else {
					value = "";
				}
				k_or_v = 'k';		
				bi = 0;
				insert_form_data_if_not_exist(key, value);
			} else if (data[i] == '=') {
				break;
			} else {
				buf[bi++] = data[i];
			}
		} else {
			break;
		}
	}

	if (i != size)
		return false;

	// for the last k=v
	if (k_or_v == 'v') {
		if (bi == 0) {
			value = "";
		} else {
			value = string(buf, buf + bi);
			insert_form_data_if_not_exist(key, value);
		}
	} else { // k=v&...k=v&kkkkkkkkk
		return false;
	}

	/*
	for (map<string, string>::const_iterator it = _form_datas.begin(); it != _form_datas.end(); it++) {
		printf("%s=%s\n", it->first.c_str(), it->second.c_str());
	}
	*/
	return true;
}

void http_request::insert_form_data_if_not_exist(const string &k, const string &v)
{
	if (_form_datas.find(URLDecode(k)) == _form_datas.end())
		_form_datas.insert(map<string, string>::value_type(URLDecode(k), URLDecode(v)));
}

int response_writer::write(const http_response &r)
{
	buffer buf;

	// status line
	buf.append_string(r._version);
	buf.append_string(" ");
	buf.append_string(r._status);
	buf.append_string(" ");
	buf.append_string(r._phrase);
	buf.append_string("\r\n");

	// headers
	for (map<string, string>::const_iterator it = r._headers.begin(); it != r._headers.end(); it++) {
		buf.append_string(it->first);
		buf.append_string(": ");
		buf.append_string(it->second);
		buf.append_string("\r\n");
		//printf("write header, %s : %s\n", it->first.c_str(), it->second.c_str());
	}
	buf.append_string("\r\n");

	// body
	if (r._body_buf.get_size() > 0) {
		buf.append_data(r._body_buf.get_data_buf(), r._body_buf.get_size());
	}

	//printf("write : %s\n", (char *)buf.get_data_buf());

	int n = tcp_write_ms(_se->get_handle(), buf.get_data_buf(), buf.get_size(), 10);
	if (n != buf.get_size()) {
		return -1;
	}
	return n;
}

void http_server::register_path_handler(const string &path, path_handler hdr)
{
	path_and_handler ph;
	ph.path = path;
	ph.hdr = hdr;
	_path_handlers.push_back(ph);
}
