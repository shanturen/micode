#pragma once
#include "server.h"

// 他妈的，写的真jb烂，极其得难用，
class http_server : public server, private pthread
{
	thread_pool _msg_thread_pool;
	address _addr;
};

