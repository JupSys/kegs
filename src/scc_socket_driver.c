/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_scc_socket_driver_c[] = "@(#)$KmKId: scc_socket_driver.c,v 1.2 2003-09-20 15:02:27-04 kentd Exp $";

/* This file contains the Unix socket calls */

#include "defc.h"
#include "scc.h"

extern Scc scc_stat[2];

void
scc_socket_init(int port)
{
#ifdef SCC_SOCKETS
	Scc	*scc_ptr;
	struct sockaddr_in sa_in;
	int	ret;
	int	sockfd;
	int	inc;

	int	on;

	inc = 0;

	scc_ptr = &(scc_stat[port]);

	scc_ptr->state = -1;		/* mark as failed for now */
	scc_ptr->host_aux1 = sizeof(struct sockaddr_in);
	scc_ptr->host_handle = malloc(scc_ptr->host_aux1);
	memset(scc_ptr->host_handle, 0, scc_ptr->host_aux1);

	while(1) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) {
			printf("socket ret: %d, errno: %d\n", sockfd, errno);
			return;
		}
		/* printf("socket ret: %d\n", sockfd); */

		on = 1;
		ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
					(char *)&on, sizeof(on));
		if(ret < 0) {
			printf("setsockopt REUSEADDR ret: %d, err:%d\n",
				ret, errno);
			return;
		}

		memset(&sa_in, 0, sizeof(sa_in));
		sa_in.sin_family = AF_INET;
		sa_in.sin_port = htons(6501 + port + inc);
		sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

		ret = bind(sockfd, (struct sockaddr *)&sa_in, sizeof(sa_in));

		if(ret < 0) {
			printf("bind ret: %d, errno: %d\n", ret, errno);
			inc++;
			close(sockfd);
			printf("Trying next port: %d\n", 6501 + port + inc);
			if(inc >= 10) {
				printf("Too many retries, quitting\n");
				return;
			}
		} else {
			break;
		}
	}

	printf("SCC port %d is at unix port %d\n", port, 6501 + port + inc);

	ret = listen(sockfd, 1);

	on = 1;
# ifdef FIOSNBIO
	ret = ioctl(sockfd, FIOSNBIO, (char *)&on);
# else
	ret = ioctl(sockfd, FIONBIO, (char *)&on);
# endif
	if(ret == -1) {
		printf("ioctl ret: %d, errno: %d\n", ret,errno);
		return;
	}

	scc_ptr->accfd = sockfd;
	scc_ptr->state = 1;		/* successful socket */
#endif	/* SCC_SOCKETS */
}

void
scc_socket_change_params(int port)
{
#ifdef SCC_SOCKETS
#endif
}

void
scc_accept_socket(int port)
{
#ifdef SCC_SOCKETS
	Scc	*scc_ptr;
	int	rdwrfd;

	scc_ptr = &(scc_stat[port]);

	if(scc_ptr->rdwrfd <= 0) {
		rdwrfd = accept(scc_ptr->accfd, scc_ptr->host_handle,
			&(scc_ptr->host_aux1));
		if(rdwrfd < 0) {
			return;
		}
		scc_ptr->rdwrfd = rdwrfd;
	}
#endif
}

void
scc_socket_fill_readbuf(int port, double dcycs)
{
#ifdef SCC_SOCKETS
	byte	tmp_buf[256];
	Scc	*scc_ptr;
	int	rdwrfd;
	int	i;
	int	ret;

	scc_ptr = &(scc_stat[port]);

	/* Accept socket if not already open */
	scc_accept_socket(port);

	rdwrfd = scc_ptr->rdwrfd;
	if(rdwrfd < 0) {
		return;
	}

	/* Try reading some bytes */
	ret = read(rdwrfd, tmp_buf, 256);
	if(ret > 0) {
		for(i = 0; i < ret; i++) {
			if(tmp_buf[i] == 0) {
				/* Skip null chars */
				continue;
			}
			scc_add_to_readbuf(port, tmp_buf[i], dcycs);
		}
	}
#endif
}

void
scc_socket_empty_writebuf(int port)
{
#ifdef SCC_SOCKETS
	Scc	*scc_ptr;
	int	rdptr;
	int	wrptr;
	int	rdwrfd;
	int	done;
	int	ret;
	int	len;

	scc_ptr = &(scc_stat[port]);

	scc_accept_socket(port);

	rdwrfd = scc_ptr->rdwrfd;
	if(rdwrfd < 0) {
		return;
	}

	/* Try writing some bytes */
	done = 0;
	while(!done) {
		rdptr = scc_ptr->out_rdptr;
		wrptr = scc_ptr->out_wrptr;
		if(rdptr == wrptr) {
			done = 1;
			break;
		}
		len = wrptr - rdptr;
		if(len < 0) {
			len = SCC_OUTBUF_SIZE - rdptr;
		}
		if(len > 32) {
			len = 32;
		}
		if(len <= 0) {
			done = 1;
			break;
		}
		ret = write(rdwrfd, &(scc_ptr->out_buf[rdptr]), len);
		if(ret <= 0) {
			done = 1;
			break;
		} else {
			rdptr = rdptr + ret;
			if(rdptr >= SCC_OUTBUF_SIZE) {
				rdptr = rdptr - SCC_OUTBUF_SIZE;
			}
			scc_ptr->out_rdptr = rdptr;
		}
	}
#endif
}
