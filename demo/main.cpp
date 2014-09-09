#include "utils/myxmlwrap.h"
#include "servers.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "Log.h"
#include "misc_configs.h"

using namespace std;

vector<string> get_local_ip()
{
	int s;
	struct ifconf conf;
	struct ifreq *ifr;
	char buff[BUFSIZ];
	int num;
	int i;
	vector<string> ip_vec;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = BUFSIZ;
	conf.ifc_buf = buff;

	ioctl(s, SIOCGIFCONF, &conf);
	num = conf.ifc_len / sizeof(struct ifreq);
	ifr = conf.ifc_req;

	for(i=0;i < num;i++) {
		struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

		ioctl(s, SIOCGIFFLAGS, ifr);
		if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)) {
			ip_vec.push_back(inet_ntoa(sin->sin_addr));
		}
		ifr++;
	}
	close(s);
	return ip_vec;
}

bool ip_in_vec(const string &ip, const vector<string> &vec)
{
	for (vector<string>::const_iterator it = vec.begin(); it != vec.end(); it++) {
		if (*it == ip)
			return true;
	}
	return false;
}

int xml_get_node_address(lightxquery &xqry, const string &xpath, address &addr)
{
	string ip;
	int port;
	if (xqry.get_attr_value(xpath, "ip", ip) != 0 || xqry.get_attr_value(xpath, "port", port) != 0)
		return -1;
	addr = address(ip, port);
	return 0;
}

int xml_get_node_ip(lightxquery &xqry, const string &xpath, address &addr)
{
	string ip;
	int port;
	if (xqry.get_attr_value(xpath, "ip", ip) != 0)
		return -1;
	addr = address(ip, -1);
	return 0;
}

int setup_calc_server(lightxquery &xqry, vector<message_server *> &svr_vec)
{
	int num_calc_server = xqry.how_many("/dumpcalc/nodes/calc");
	if (0 == num_calc_server) {
		LOG_ERROR_VA("calc error");
		return -1;
	}

	vector<string> local_ip_vec = get_local_ip();
	int num_ip = local_ip_vec.size();
	string svr_ip;
	char xpathbuf[500];
	address addr;
	string path;
	int num_seted_servers = 0;
	
	calc_server *svr = new calc_server();

	for(int i = 0;i != num_calc_server;i++){
		snprintf(xpathbuf, 499, "/dumpcalc/nodes/calc[%d]",i + 1);
		if (xml_get_node_address(xqry, xpathbuf, addr) != 0){
			LOG_ERROR_VA("calc addr error");
			continue;
		}
		if (ip_in_vec(addr.ip(), local_ip_vec)){

			if (xqry.get_attr_value(xpathbuf, "gf_cluster_path", path) != 0) {
				LOG_ERROR_VA("gf_data cluster_center_path error ");
				return -1;
			}
			svr->set_gf_cluster_center_path(path);
	
			if (xqry.get_attr_value(xpathbuf, "lf_cluster_path", path) != 0) {
				LOG_ERROR_VA("lf_data cluster_center_path error ");
				return -1;
			}
			svr->set_lf_cluster_center_path(path);

			if (xqry.get_attr_value(xpathbuf, "tl_cluster_path", path) != 0) {
				LOG_ERROR_VA("text_label cluster_center_path error ");
				return -1;
			}
			svr->set_tl_cluster_center_path(path);

			svr->set_listener_address(addr);
			if (svr->setup() == 0) {
				svr_vec.push_back(svr);
				num_seted_servers++;
			} else 
				delete svr;
		}
	}
	return num_seted_servers;
}

int start_servers(lightxquery &xqry, vector<message_server *> &svr_vec)
{
	if (setup_calc_server(xqry, svr_vec) < 0) {
		LOG_ERROR_VA("server setup failed");
		return -1;
	}
	
	int num_threads;
	if (xqry.get_attr_value("/dumpcalc/public/thread", "number_of_work_threads", num_threads) != 0) {
		LOG_ERROR_VA("thread number configure error");
		return -1;
	}

	for (vector<message_server *>::iterator it = svr_vec.begin(); it != svr_vec.end(); it++)
		(*it)->get_thread_pool()->set_max_threads(num_threads);

	for (vector<message_server *>::iterator it = svr_vec.begin(); it != svr_vec.end(); it++)
		(*it)->startup();

	for (vector<message_server *>::iterator it = svr_vec.begin(); it != svr_vec.end(); it++) {
		//(*it)->stop();
		//delete *it;
		(*it)->waitend();
	}
}

vector<message_server *> g_svr_vec;
void sig_stop(int signo)
{
	LOG_INFO_VA("stop servers in SIGINT ");
	for (vector<message_server *>::iterator it = g_svr_vec.begin(); it != g_svr_vec.end(); it++) {
		(*it)->stop();
		delete *it;
	}
	exit(0);
}

int misc_config(lightxquery &xqry)
{

	if (xqry.get_attr_value("/dumpcalc/miscs/user_image_save_path", "value", the_misc_configs.user_image_save_path) != 0) {
		LOG_ERROR_VA("misc config user_iamge_save_path error");
		return -1;
	}
	LOG_INFO_VA("user save image path: %s", the_misc_configs.user_image_save_path.c_str());
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		cout << "no configure file !" << endl;
		return -1;
	}

	xmlInitParser();
	lightxquery xqry;
	if (!xqry.load(argv[1])) {
		cout << "load config xml file error, please check " << argv[1] << endl;
		return -1;
	}
	
	// init logger
	string path, log_level;
	if (xqry.get_attr_value("/dumpcalc/public/log[@path]", "path", path) != 0) {
		cout << "log path configure error" << endl;
		return -1;
	}
	if (xqry.get_attr_value("/dumpcalc/public/log[@level]", "level", log_level) != 0) {
		cout << "log level configure error" << endl;
		return -1;
	}

	if (!init_logger(path, log_level)) {
		cout << "logger init failed, check /public/log[@path | @level]" << endl;
		return -1;
	}

	if (misc_config(xqry) != 0)
		return -1;

	signal(SIGPIPE,SIG_IGN);
	signal(SIGINT, sig_stop);

	start_servers(xqry, g_svr_vec);
	
	return 0;
}
