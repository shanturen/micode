#include "message.h"
#include "server.h"

octet_ostream &operator<<(octet_ostream &oos, head &h)
{
	oos << h.version << h.command;
	oos.append_data(h.password, 8);
	oos << h.type << h.status << h.src << h.dest << h.sn_high << h.sn_low << h.length;
	return oos;
}

int msg_handle_task::execute(worker *wkr)
{
	timeval begin, end;
	safe_gettimeofday(&begin, 0);

	LOG_INFO_VA("start msg task, msg id:%d, sn:%llu-%llu\n", _msg->get_id(), _msg->get_head().sn_high, _msg->get_head().sn_low);

	int ret = _msg->response(wkr);
	
	
	safe_gettimeofday(&end, 0);
	LOG_INFO_VA("request message task serve time: %d us, more acumulate req time: %d us"
	, (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec)
	, timee::now() - _msg->get_socket_event()->get_active_time());
	_msg->get_socket_event()->unset_delayed();

	//if (_msg->get_socket_event()->client_closed()) {
	//	LOG_FATAL_VA("close event in task %x\n", _msg->get_socket_event());
	//	_msg->get_socket_event()->unregister();
	//}

	return ret;
}
