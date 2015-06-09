/*
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */


/* Copyright (c) 2015 Wang Ke.
 * pthreadself@gmail.com
 * http://github.com/pthreadself
 * All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */
 
#include	"sock.h"

#define IP_MTU 14  /* funny that IP_MTU can only be found in <linux/in.h>,
                      which we can not include */

int
cliopen(char *host, char *port)
{
	int					fd, i, on, val, vallen;
	char				*protocol;
	unsigned long		inaddr;
	struct sockaddr_in	cli_addr, serv_addr;
	struct servent		*sp;
	struct hostent		*hp;

	protocol = udp ? "udp" : "tcp";

		/* initialize socket address structure */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

		/* see if "port" is a service name or number */
	if ( (i = atoi(port)) == 0) {
		if ( (sp = getservbyname(port, protocol)) == NULL)
			err_quit("getservbyname() error for: %s/%s", port, protocol);

		serv_addr.sin_port = sp->s_port;
	} else
		serv_addr.sin_port = htons(i);

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */

	if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
						/* it's dotted-decimal */
		bcopy((char *) &inaddr, (char *) &serv_addr.sin_addr, sizeof(inaddr));

	} else {
		if ( (hp = gethostbyname(host)) == NULL)
			err_quit("gethostbyname() error for: %s", host);

		bcopy(hp->h_addr_list[0], (char *) &serv_addr.sin_addr, hp->h_length);
	}

	if ( (fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0)
		err_sys("socket() error");

	if (reuseaddr) {
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
										(char *) &on, sizeof (on)) < 0)
			err_sys("setsockopt of SO_REUSEADDR error");
	}

	if (dontfragment) {
		val = IP_PMTUDISC_DO;
		if (setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER,
										(char *) &val, sizeof (val)) < 0)
			err_sys("setsockopt of IP_MTU_DISCOVER error");

	}

	/*
	 * User can specify port number for client to bind.  Only real use
	 * is to see a TCP connection initiated by both ends at the same time.
	 * Also, if UDP is being used, we specifically call bind() to assign
	 * an ephemeral port to the socket.
	 */

	if (bindport != 0 || udp) {
		bzero((char *) &cli_addr, sizeof(cli_addr));
		cli_addr.sin_family      = AF_INET;
		cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/* wildcard */
		cli_addr.sin_port        = htons(bindport);

		if (bind(fd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
			err_sys("bind() error");
	}

	/* Need to allocate buffers before connect(), since they can affect
	 * TCP options (window scale, etc.).
	 */

	buffers(fd);
	sockopts(fd, 0);	/* may also want to set SO_DEBUG */

	/*
	 * Connect to the server.  Required for TCP, optional for UDP.
	 */

	if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_sys("connect() error");

	if (verbose) {
			/* Call getsockname() to find local address bound to socket:
			   TCP ephemeral port was assigned by connect() or bind();
			   UDP ephemeral port was assigned by bind(). */
		i = sizeof(cli_addr);
		if (getsockname(fd, (struct sockaddr *) &cli_addr, &i) < 0)
			err_sys("getsockname() error");

					/* Can't do one fprintf() since inet_ntoa() stores
					   the result in a static location. */
		fprintf(stderr, "connected on %s.%d ",
					INET_NTOA(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		fprintf(stderr, "to %s.%d\n",
					INET_NTOA(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
		if (getsockopt(fd, IPPROTO_IP, IP_MTU, &val, (socklen_t*)&vallen) < 0)
		    err_sys("getsockname() error");
		fprintf(stderr, "current path MTU is %d\n", val);
	}

	sockopts(fd, 1);	/* some options get set after connect() */

	return(fd);
}
