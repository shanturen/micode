#include "mihttp/http_server.h"
#include <string>

using namespace std;



int benchmark_handler(const http_request &req, response_writer &w)
{
	http_response r;
	buffer buf;
	buf.append_string("hello");
	r.set_body(buf);
	w.write(r);
	//printf("handler run\n");
	return 0;
}

int main(int argc, char **argv)
{
	http_server svr("127.0.0.1", 3101);

	int work_thread_count = 0;
	if (work_thread_count > 0) {
		svr.set_work_thread_number(work_thread_count);
	}

	svr.register_path_handler("/benchmark", benchmark_handler);

	svr.start();
	svr.wait();
}

