#pragma once
#include "buffer.h"
#include "thread.h"
#include "socket.h"
#include "socket_event.h"
#include <iostream>
#include "event_manager.h"
#include "log.h"
#include "safe_time.h"
using namespace std;

static bool is_big_endian()
{
	unsigned int i = 0x1;
	unsigned char *p = reinterpret_cast<unsigned char *>(&i);
	return p[0] == 0;
}

static void reverse_bytes(unsigned char *bytes, int nbytes) 
{
	int left = 0, right = nbytes - 1;
	while (left < right) {
		unsigned char c = bytes[left];
		bytes[left++] = bytes[right];
		bytes[right--] = c;
	}
}

static unsigned long long htonll(unsigned long long ll)
{
	if (!is_big_endian())
		reverse_bytes(reinterpret_cast<unsigned char *>(&ll), sizeof(unsigned long long));
	return ll;
}

static unsigned long long ntohll(unsigned long long ll)
{
	if (!is_big_endian())
		reverse_bytes(reinterpret_cast<unsigned char *>(&ll), sizeof(unsigned long long));
	return ll;
}

class octet_ostream
{
	buffer _buf;
public:
	octet_ostream() : _buf(128) {};
	octet_ostream &operator<< (int i) { int oi = htonl(i); _buf.append_data(&oi, sizeof(int)); return *this; }
	octet_ostream &operator<< (char c) { _buf.append_data(&c, sizeof(char)); return *this; }
	octet_ostream &operator<< (unsigned long long ll) { unsigned long long nll =  htonll(ll); _buf.append_data(&nll, sizeof(nll)); return *this;}
	octet_ostream &operator<< (float f) { _buf.append_data(&f, sizeof(f)) ; return *this; }
	octet_ostream &operator<< (double d) { _buf.append_data(&d, sizeof(d)); return *this; }
	octet_ostream &operator<< (const char *cstr) { _buf.append_data(cstr, strlen(cstr) + 1); return *this; }
	octet_ostream &operator<< (const string &str) { return *this << str.c_str(); return *this; }
	octet_ostream &append_data(const void *data, int length) { _buf.append_data(data, length); return *this; }
	buffer &get_buffer() { return _buf; }
};

class octet_istream 
{
	buffer *_buf;
public:
	octet_istream(buffer &buf) { _buf = &buf; }
	octet_istream &operator>> (char &c) { if (_buf->drain_data(&c, sizeof(char)) != 0) cout << "error"; return *this; }
	octet_istream &operator>> (int &i) { _buf->drain_data(&i, sizeof(int)); i = ntohl(i); return *this; }
	octet_istream &operator>> (unsigned long long &ll) { _buf->drain_data(&ll, sizeof(long long));  ll = ntohll(ll);  return *this; }
	octet_istream &operator>> (float &f) { _buf->drain_data(&f, sizeof(float)); return *this; }
	octet_istream &operator>> (double &d) { _buf->drain_data(&d, sizeof(double)); return *this; }
	octet_istream &operator>> (string &str) { str = "unsupport"; return *this; }
	octet_istream & drain_data(void *data, int length) { _buf->drain_data(data, length); return *this; }
};


#pragma pack(push)
#pragma pack(1)
class head
{
public:
	int version;
	int command;
	char password[8];
	int type;
	int status;
	int src;
	int dest;
	unsigned long long sn_high;
	unsigned long long sn_low;
	int length;

	head()
	{
		version = 400;
		command = 0;
		memset(password ,'\0',8);
		type = 0;
		status = 0;
		src = 0;
		dest = 0;
		sn_high = 0;
		sn_low = 0;
		length = 0;
	}

	void ntoh()
	{
		version = ntohl(version);
		command = ntohl(command);
		type = ntohl(type);
		status = ntohl(status);
		src = ntohl(src);
		dest = ntohl(dest);
		length = ntohl(length);
		sn_high = ntohll(sn_high);
		sn_low = ntohll(sn_low);
	}

	
};
#pragma pack(pop)

class server;
octet_ostream &operator<<(octet_ostream &oos, head &h);

class message
{
public:
	enum {type_request, type_response, type_notify};

private:
	socket_event *_sock_event;
	head _head;
public:
	message() { _sock_event = 0; }
	virtual ~message() { _sock_event = 0; }
	int get_id() { return _head.command; }
	int get_version() { return _head.version; }
	int get_length() { return _head.length; }
	int get_status() { return _head.status; }
	void set_id(int mid) { _head.command = mid; }
	void set_version(int ver) { _head.version = ver; }
	void set_length(int length) { _head.length = length; }
	void set_status(int s) { _head.status = s; }
	void set_sn_high(unsigned long long sh) { _head.sn_high = sh; }
	void set_sn_low(unsigned long long sl) { _head.sn_low = sl; }
	void set_type(int i) { _head.type = 1; }

	int get_handle() { return _sock_event->get_handle(); }
	void set_head(const head &h) { _head = h; }
	const head &get_head() const { return _head; }
	head &get_head() { return _head; }
	void set_socket_event(socket_event *evt) { _sock_event = evt; evt->set_delayed(); }
	socket_event *get_socket_event() { return _sock_event; }
	//server *get_server() {return _sock_event->get_event_manager()->get_server(); }
	virtual int marshal(buffer &buf) = 0;
	virtual int unmarshal(buffer &buf) = 0;
	virtual int response(worker *wrk_env) = 0;
	int response_error(const string &str_err, int code = -1)
	{
		LOG_INFO_VA("error response : %s\n", str_err.c_str());
		octet_ostream ots;
		set_status(-1);
		set_type(1);
		set_length(1 + str_err.size());
		ots << _head << str_err;
		int ret = send(get_handle(), ots.get_buffer().get_data_buf(), ots.get_buffer().get_size(), 0);
		return ret;
	}
};

class response_message : public message
{
	string _error_str;
public:
	response_message()
	{
		get_head().type = message::type_response;
		get_head().status = 0;
	}
	virtual ~response_message() {}
	bool unnormal() { return get_head().status != 0; }

	string get_error_str() { return _error_str; }
	void set_error_str(const string &e) { _error_str = e; set_length(e.size() + 1); }

	int marshal(buffer &buf)
	{
		if (message::get_head().status != 0) {
			octet_ostream ots;
			set_length(_error_str.size() + 1);
			ots << get_head() << _error_str;
			buf = ots.get_buffer();
			return 0;
		} else 
			return response_marshal(buf);
	}
	int unmarshal(buffer &buf)
	{
		if (message::get_head().status != 0) {
			const char *errstr = static_cast<const char *>(buf.get_data_buf());
			_error_str = errstr;
			set_length(_error_str.size() + 1);
			return 0;
		} else 
			return response_unmarshal(buf);
	}

	int response(worker *) { return 0; }
	virtual int response_marshal(buffer &buf) = 0;
	virtual int response_unmarshal(buffer &buf) = 0;
	virtual void show() { cout << "show in class response_message" << endl;};
};

class error_response_message : public response_message
{
public:
	error_response_message() 
	{
		get_head().status = -1;
	}	
	int response_marshal(buffer &buf) {return 0; }
	int response_unmarshal(buffer &buf) { return 0; }
};

class message_builder_base
{
public:
	virtual ~message_builder_base() {};
	virtual message *build_message(const head &msg_head, buffer &buf) = 0;
};

template <class msg_type>
class message_builder : public message_builder_base
{
public:
	virtual message *build_message(const head &msg_head, buffer &buf)
	{
		msg_type *msg = new msg_type();
		msg->set_head(msg_head);
		if (msg->unmarshal(buf) != 0) {
			LOG_ERROR_VA("messsage unmarshal failed\n");
			delete msg;
			return 0;
		}
		return msg;
	}
};

class msg_handle_task : public task
{
	message *_msg;
public:
	msg_handle_task(message *msg) : _msg(msg) { }
	~msg_handle_task() { delete _msg; }
	int execute(worker *wkr);
};

class message_worker : public worker
{
public:
	virtual int handle_message(message *msg) { /* design changed */ return 0; };
};

int send_msg(int sock, message &msg, int msecs=1000);
int recv_msg(int sock, message &msg, int msecs=10000);
