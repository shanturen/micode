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

	LOG_INFO_VA("start msg task, msg id:%d, sn:%llu-%llu\n", _msg->get_id(), _msg->get_head().sn_high, _msg->get_head().sn_low);

	int ret = _msg->response(wkr);
	
	

	//if (_msg->get_socket_event()->client_closed()) {
	//	LOG_FATAL_VA("close event in task %x\n", _msg->get_socket_event());
	//	_msg->get_socket_event()->unregister();
	//}

	return ret;
}
