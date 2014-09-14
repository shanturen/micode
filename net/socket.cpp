#include "system.h"
#include "socket.h"
#include "message.h"
#include <iostream>
#include "log.h"
using namespace std;

static int tcp_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
			struct timeval *timeout)
{
	int  val;
  again:
	val = select(nfds, readfds, writefds, exceptfds, timeout);
	if (val < 0) {
                if (errno == EINTR) {
                        goto again;
                }
		LOG_ERROR_VA("select() call error : %d, %s\n", errno, strerror(errno));
        }

        if (val == 0) {
                errno = ETIMEDOUT;
        }

        return (val);
}
static int tcp_read(int fd, void *ptr, size_t nbytes, struct timeval *tv)
{
	int err;
	fd_set fs;

	FD_ZERO(&fs);
	FD_SET(fd, &fs);
	
	err = tcp_select(fd+1, &fs, NULL, NULL, tv);
	if (err > 0) {
		int ret =read(fd, ptr, nbytes);
		return ret;
	} else if (err == 0) {
		return -2;
	} else {
		return -3;
        }
}

static ssize_t tcp_write(int fd, void *ptr, size_t nbytes, struct timeval *tv) 
{
	int err;
	fd_set fs;

	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	err = tcp_select(fd+1, NULL, &fs, NULL, tv);

	if(err < 0) 
	{
		return -1;
	} 
	else if (err == 0)
	{
		return -2;
	}

	return write(fd, ptr, nbytes);	
}

int tcp_read_ms(int fd, void *ptr1, int nbytes, int msecs)
{
	ssize_t  nread;
	int 	 sockflag;
	size_t   nleft = nbytes;
	char *ptr = (char *)ptr1;
	struct  timeval tv;
	struct	timeval *tp;

	if(msecs >= 0)
	{
        	tv.tv_sec = msecs / 1000;
        	tv.tv_usec = (msecs % 1000) * 1000;
		tp = &tv;
	}
	else
	{
		tp = NULL;
	}

        sockflag = fcntl(fd, F_GETFL, 0);
        if(sockflag < 0)
        {
                return -1;
        }

        if(!(sockflag & O_NONBLOCK))
        {
                if(fcntl(fd, F_SETFL, sockflag | O_NONBLOCK) < 0)
                {
                        return -1;
                }
        }

	while(nleft > 0)
	{
	  again:
		nread = tcp_read(fd, ptr, nleft, tp);
		if(nread < 0)
		{
			printf("read <= 0 %d\n", nread);
			if((nread == -1) && (errno == EINTR))
			{
				goto again;
			}
			else if(nread == -2)
			{
				errno = ETIMEDOUT;
				return -1;
			}
			else if(nread == -3)
			{
				errno = EIO;
				return -1;
			}
			else
			{
				return -1;
			}
		} 
		else if(nread == 0)
		{
			break;
		}
		else 
		{
			printf("read > 0 %d\n", nread);
			ptr += nread;
			nleft -= nread;
		}
	}

	return (ssize_t)(nbytes - nleft);
}


int tcp_write_ms(int fd, const void *data, int length, int msecs)
{
	int  sockflag;
	ssize_t  n;
	size_t nleft = length;
	char *ptr = (char *)data;
	struct  timeval tv;
	struct  timeval *tp;

	if(msecs >= 0)
	{
		tv.tv_sec = msecs / 1000;
		tv.tv_usec = (msecs % 1000) * 1000;
		tp = &tv;
	}
	else
	{
		tp = NULL;
	}

	sockflag = fcntl(fd, F_GETFL, 0);
	if(sockflag < 0)
	{
		return -1;
	}

	if(!(sockflag & O_NONBLOCK))
	{
		if(fcntl(fd, F_SETFL, sockflag | O_NONBLOCK) < 0)
		{
			return -1;
		}
	}

	while(nleft > 0)
	{
		n = tcp_write(fd, ptr, (size_t)nleft, tp);
		if((n == -1) && (errno == EINTR))
		{
			continue;
		}
		if(n <= 0)
		{
			if(n == -2)
			{
				errno = ETIMEDOUT;
			}

			if(!(sockflag & O_NONBLOCK))
			{
				if(fcntl(fd, F_SETFL, sockflag) < 0)
				{
					return -1;
				}
			}

			return -1;
		}
		nleft -= n;
		ptr += n;
	}
	if(!(sockflag & O_NONBLOCK))
	{
		if(fcntl(fd, F_SETFL, sockflag) < 0)
		{
			return -1;
		}
	}

	return (ssize_t)(length - nleft);
}

int tcp_read_ms(int sock, buffer &buf, int length, int msecs)
{
	buf.ensure_compacity(length);
	buf.drain_all_data();
	int ret = tcp_read_ms(sock, buf.get_data_buf(), length, msecs);
	if (ret > 0) 
		buf.set_used_size(ret);
	return ret;
}

int tcp_read_ms_once(int sock, buffer &buf, int length, int msecs)
{
	buf.ensure_compacity(length);
	buf.drain_all_data();

	struct  timeval tv;
	struct  timeval *tp;

	if(msecs >= 0)
	{
		tv.tv_sec = msecs / 1000;
		tv.tv_usec = (msecs % 1000) * 1000;
		tp = &tv;
	}
	else
	{
		tp = NULL;
	}

	int nread = tcp_read(sock, buf.get_data_buf(), length, tp);
	if (nread > 0)
		buf.set_used_size(nread);
	
	return nread;
}

int send_msg(int sock, message &msg, int msecs)
{
	buffer buf;
	msg.marshal(buf);
	return tcp_write_ms(sock, buf.get_data_buf(), buf.get_size(), msecs);
}

int recv_msg(int sock, message &msg, int msecs)
{
	head msg_head;
	int ret = tcp_read_ms(sock, &msg_head, sizeof(msg_head), msecs);
	if (ret != sizeof(msg_head)) {
		LOG_ERROR_VA("receive msg head error\n");
		return -1;
	}
	msg_head.ntoh();

	LOG_INFO_VA("msg head, cmd:%d, st:%d, len:%d, vr:%d\n", msg_head.command, msg_head.status, msg_head.length, msg_head.version);
	buffer buf;
	if (tcp_read_ms(sock, buf, msg_head.length, msecs) != msg_head.length) {
		return -1;
	}

	msg.set_head(msg_head);
	if (msg.unmarshal(buf) != 0) {
		LOG_ERROR_VA("unmarshal failed\n");
		return -1;
	}
	return (ret + msg_head.length);
}

int check_alive(int sockno)
{
	if (sockno <= 0)
		return 0;
	fd_set	fs;
	int	maxfd;
	int	ret;
	
	const timeval TV_TIMEOUT = { 0, 0 };
	timeval  tv;
	
	FD_ZERO(&fs);
	FD_SET(sockno, &fs);
	
	maxfd = sockno + 1;
	
	tv = TV_TIMEOUT;
	ret = select(maxfd, NULL, &fs, NULL, &tv);
	if(ret < 0)
	{
		return ret;
	}
	if(FD_ISSET(sockno, &fs))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

