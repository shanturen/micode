#include "net/http_server.h"
#include <iostream>
#include <stdio.h>
using namespace std;

int hello_handler(const http_request &req, response_writer &w)
{
	http_response r;
	//cout << "the handler called" << endl;
	w.write(r);
	return 0;
}

int main(int argc, char **argv)
{
	printf("jfkdlajfklajk\n");
	http_server svr;
	svr.set_listener_address(address("127.0.0.1", 7070));
	svr.register_path_handler("/hello", hello_handler);
	svr.start();
	svr.wait();
}
