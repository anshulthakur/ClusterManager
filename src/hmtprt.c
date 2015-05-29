/*
 * hmtprt.c
 *
 *	Transport Functions
 *
 *  Created on: 06-May-2015
 *      Author: anshul
 */

#include <hmincl.h>

/***************************************************************************/
/* Name:	hm_open_socket 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Opens a non-blocking socket			*/
/***************************************************************************/
static int32_t hm_open_socket(struct addrinfo *res)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t sock_fd = -1;
	int32_t val;
	int32_t option_val;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(res != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock_fd < 0)
	{
		TRACE_PERROR(("Error opening socket."));
		sock_fd = -1;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Socket Opened. Now set its options		   						       */
	/***************************************************************************/
	TRACE_DETAIL(("Setting the socket to non-blocking mode"));
	val = fcntl(sock_fd, F_GETFL, 0);
	val = fcntl(sock_fd, F_SETFL, val | O_NONBLOCK);

	if (val == -1)
	{
		TRACE_PERROR(("Error setting socket to non-blocking mode"));
		close(sock_fd);
		sock_fd = -1;
		goto EXIT_LABEL;
	} //end if val == -1

	/***************************************************************************/
	/* Allow the descriptors to be reusable									   */
	/***************************************************************************/
    option_val = (int)TRUE;

	/*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
	/*************************************************************/
    TRACE_DETAIL(("Setting socket descriptor as reusable"));
	val = setsockopt(sock_fd, SOL_SOCKET,  SO_REUSEADDR,
	                   (char *)&option_val, sizeof(option_val));
	if (val < 0)
	{
		TRACE_PERROR(("Error settting socket descriptor to reusable"));
		close(sock_fd);
	    sock_fd = -1;
	    goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Add the descriptor to global descriptor set							   */
	/***************************************************************************/
	if(max_fd < sock_fd)
	{
		TRACE_DETAIL(("Update maximum socket descriptor value to %d", sock_fd));
		max_fd = sock_fd;
	}
	TRACE_DETAIL(("Add FD to set"));
	FD_SET(sock_fd, &hm_tprt_conn_set);

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return sock_fd;
}/* hm_open_socket */

/**PROC+**********************************************************************/
/* Name:     hm_tprt_create_connection 		                                 */
/*                                                                           */
/* Purpose:   Creates a non-blocking socket and returns its descriptor.      */
/*                                                                           */
/* Returns:   int32_t sock_fd :	Socket File descriptor of socket created.    */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 		: target: IP Address/Domain Name of target           */
/*			  IN		: service: Port										 */
/*			  IN		: conn_type: Connection Type requested.				 */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/
HM_SOCKET_CB * hm_tprt_open_connection(uint32_t conn_type, void * params )
{
	/***************************************************************************/
	/* Descriptor of the socket created. Default value denotes error condition */
	/***************************************************************************/
	int32_t sock_fd = -1;
	int32_t val;
	int32_t option_val;

	HM_SOCKET_CB *sock_cb = NULL;

	char target[128], service[128];

	int32_t ret_val;

	struct addrinfo hints, *res, *ressave;

	TRACE_ENTRY();

	TRACE_ASSERT(params != NULL);
	/***************************************************************************/
	/* Create a socket control block										   */
	/***************************************************************************/
	sock_cb = hm_alloc_sock_cb();
	if(sock_cb == NULL)
	{
		TRACE_ERROR(("Resource allocation failed for socket control block"));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Depending on the connection type, determine the sockaddr strcuture	   */
	switch(conn_type)
	{
	case HM_TRANSPORT_TCP_LISTEN:
		TRACE_INFO(("IPv4 Listen Socket"));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
		break;

	case HM_TRANSPORT_TCP_OUT:
		TRACE_INFO(("IPv4 Outgoing Socket"));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
		break;

	case HM_TRANSPORT_TCP_IN:
		TRACE_INFO(("IPv4 Incoming Socket"));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		break;

	case HM_TRANSPORT_UDP:
		TRACE_INFO(("IPv4 UDP Socket"));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
		break;

	case HM_TRANSPORT_MCAST:
		TRACE_INFO(("IPv4 Multicast Socket"));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
		break;

	case HM_TRANSPORT_SCTP:
		TRACE_INFO(("IPv4 SCTP Socket"));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_SCTP;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
		break;

	case HM_TRANSPORT_TCP_IPv6_LISTEN:
		TRACE_INFO(("IPv6 Listen Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
		break;

	case HM_TRANSPORT_TCP_IPv6_OUT:
		TRACE_INFO(("IPv6 I/O Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
		break;

	case HM_TRANSPORT_TCP_IPv6_IN:
		TRACE_INFO(("IPv6 I/O Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		break;

	case HM_TRANSPORT_UDP_IPv6:
		TRACE_INFO(("IPv6 UDP Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_DGRAM;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
		break;

	case HM_TRANSPORT_MCAST_IPv6:
		TRACE_INFO(("IPv6 Multicast Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_DGRAM;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
		break;

	case HM_TRANSPORT_SCTP_IPv6:
		TRACE_INFO(("IPv6 SCTP Socket"));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
		snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
		break;

	default:
		TRACE_ERROR(("Unknown connection Type %d", conn_type));
		TRACE_ASSERT(0==1);
		goto EXIT_LABEL;
	}

	switch(conn_type)
	{
	case HM_TRANSPORT_TCP_IN:
	case HM_TRANSPORT_TCP_IPv6_IN:
		TRACE_INFO(("Accept incoming connection"));
		goto EXIT_LABEL;

	default:
		TRACE_INFO(("One of Outgoing Connections Type %d", conn_type));
	}

	/***************************************************************************/
	/* If unsatisfactory, this code chunk may be replaced by a more elaborate  */
	/* memset(0) and filling of sin_addr structures.						   */
	/***************************************************************************/
	TRACE_DETAIL(("AI_FAMILY: %d", hints.ai_family));
	TRACE_DETAIL(("Target: %s:%s", target, service));
	if((ret_val = getaddrinfo(target, service, &hints, &res)) !=0)
	{
		TRACE_GAI_ERROR(("Error getting information on target %s:%s", target, service),ret_val);
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	ressave = res;
	/***************************************************************************/
	/* Right now, we are only expecting a single address in response.		   */
	/* Set appropriate socket options depending on the type of socket.		   */
	/***************************************************************************/
	TRACE_DETAIL(("Opening Socket"));
	sock_fd = hm_open_socket(res);
	if(sock_fd == -1)
	{
		TRACE_ERROR(("Error opening socket"));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Other specific options and processing.								   */
	/***************************************************************************/
	switch (conn_type)
	{
	case HM_TRANSPORT_TCP_LISTEN:
		/***************************************************************************/
		/* Set SOCKOPTS to TCP_NODELAY to disable Nagle's algorithm				   */
		/***************************************************************************/
		 val = 	setsockopt(sock_fd,
		                      IPPROTO_TCP,
	        	              TCP_NODELAY,
	                	      &option_val,
		                      sizeof(option_val));
		 if(val == -1)
		 {
			 TRACE_PERROR(("Error Setting TCP_NODELAY"));
			 close(sock_fd);
			 sock_fd = -1;
			 goto EXIT_LABEL;
		 }
		/***************************************************************************/
		/* Bind to address														   */
		/***************************************************************************/
		TRACE_DETAIL(("Binding to address"));
		if(bind(sock_fd, res->ai_addr, res->ai_addrlen) != 0)
		{
			TRACE_PERROR(("Error binding to port"));
			close(sock_fd);
			sock_fd = -1;
			goto EXIT_LABEL;
		}

		TRACE_DETAIL(("Start Listen on Port"));
		if(listen(sock_fd, HM_MAX_PENDING_CONNECT_REQ) != 0)
		{
			TRACE_PERROR(("Listen on socket %d failed.", sock_fd));
			close(sock_fd);
			sock_fd = -1;
			goto EXIT_LABEL;
		}
		break;

	case HM_TRANSPORT_TCP_OUT:
		TRACE_INFO(("Trying to connect"));
		if(connect(sock_fd, res->ai_addr, res->ai_addrlen)!=0)
		{
			TRACE_PERROR(("Connect failed on socket %d", sock_fd));
			close(sock_fd);
			sock_fd = -1;
			goto EXIT_LABEL;
		}
		break;
	case HM_TRANSPORT_UDP:
		/***************************************************************************/
		/* Bind to address														   */
		/***************************************************************************/
		TRACE_DETAIL(("Binding to address"));
		if(bind(sock_fd, res->ai_addr, res->ai_addrlen) != 0)
		{
			TRACE_PERROR(("Error binding to port"));
			close(sock_fd);
			sock_fd = -1;
			goto EXIT_LABEL;
		}
        break;

	case HM_TRANSPORT_MCAST:
		/***************************************************************************/
		/* Bind to address														   */
		/***************************************************************************/
		TRACE_DETAIL(("Binding to address"));
		if(bind(sock_fd, res->ai_addr, res->ai_addrlen) != 0)
		{
			TRACE_PERROR(("Error binding to port"));
			close(sock_fd);
			sock_fd = -1;
			goto EXIT_LABEL;
		}
		//TODO: Join multicast group
		break;

	default:
		TRACE_WARN(("Unknown Connection type"));
		break;
	}
	/***************************************************************************/
	/* All went well. Set the sock_fd as that of sock_cb					   */
	/***************************************************************************/
	sock_cb->sock_fd = sock_fd;

EXIT_LABEL:
	if (ret_val == 0)
	{
		/***************************************************************************/
		/* Free the address structures that were allocated in getaddrinfo in kernel*/
		/* NOTE: We are not saving the struct sockaddr, we will fetch it later 	   */
		/* using getaddrinfo()													   */
		/***************************************************************************/
		freeaddrinfo(ressave);
		/***************************************************************************/
		/* Add the socket descriptor to the global FD set.						   */
		/***************************************************************************/
		//FIXME
	}
	else if(ret_val == HM_ERR)
	{
		hm_free_sock_cb(sock_cb);
		sock_cb = NULL;
	}

	TRACE_EXIT();
	return (sock_cb);
} /* hm_tprt_create_connection */

#if 0
/**PROC+**********************************************************************/
/* Name:     hm_tprt_send_on_socket	                                         */
/*                                                                           */
/* Purpose: Send Message on socket					                         */
/*                                                                           */
/* Returns:   int32_t success :	Boolean TRUE/FALSE			                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN : sock_fd - Descriptor on which to sent data                */
/*            IN : msg_buffer - Buffer which must be sent.	                 */
/*			  IN: length - Length of the data to be sent.					 */
/*                                                                           */
/* Operation: Retry until all is sent.					                     */
/*                                                                           */
/**PROC-**********************************************************************/

int32_t hm_tprt_send_on_socket(struct sockaddr* ip,int32_t sock_fd,
						uint32_t sock_type,BYTE *msg_buffer, uint32_t length )
{
	/***************************************************************************/
	/* Local variables														   */
	/***************************************************************************/
	int32_t total_bytes_sent=0;
	int32_t bytes_sent=0;
	int32_t success = FALSE;
	BYTE *buf = NULL;
	int32_t os_error;
	struct sockaddr_in addr;

	TRACE_ENTRY();

	printf("\n sock type %d", sock_type);
	printf("\nAttempt to send %d bytes of data on socket %d",
	    							length, sock_fd);
	fflush(stdout);
	buf = msg_buffer;

	do
	{
		if(sock_type== HM_TRANSPORT_TCP_IO)
		{
			printf("\n data to be sent on TCP connection ");
			bytes_sent = send(sock_fd,
							  buf,
							  (length - bytes_sent),
							  0);
		}

		else if(sock_type==HM_TRANSPORT_UDP)
		{
			printf("\n data to be sent on UDP Ucast connection ");
			bytes_sent = sendto(sock_fd,
                                 buf,
                                 (length - bytes_sent),
                                 0,
                                 ip,
                                 sizeof(struct sockaddr_in));
		}

		else if(sock_type==HM_TRANSPORT_MCAST)
		{
			printf("\n data to be sent on UDP Mcast connection ");

			bytes_sent = sendto(sock_fd,
                                 buf,
                                 (length - bytes_sent),
                                 0,
                                 (struct sockaddr *) &tprt_local.mcast_send_addr,
                                 sizeof(struct sockaddr_in));
        }

		if(bytes_sent == length)
		{
			printf("\nMessage sent in full");
			buf = NULL;
			success = TRUE;
			break;
		}
		else if (bytes_sent == -1)
		{
			/***********************************************************************/
			/* An error was returned - check for retryable socket error values.    */
			/***********************************************************************/
			os_error = errno;
			printf("\nSend failed on socket %d, error %d",
			    		sock_fd, os_error);

			perror(" Error : ");
			fflush(stdout);

			if ((os_error == EWOULDBLOCK) ||
			          (os_error == ENOMEM) ||
			          (os_error == ENOSR))
			{
				/*********************************************************************/
			    /* This seems to be a flow control condition - clear the bytes sent  */
			    /* value to show that no data was written.                           */
			    /*********************************************************************/
			    printf("Resource shortage - try again");
			    bytes_sent = 0;
			}
			else if (os_error == Eint32_tR)
			{
				printf("\nSend interrupted - loop round again");
			    bytes_sent = 0;
			}
			else
			{
				/*********************************************************************/
			    /* Socket failed so work source needs to be unregistered.            */
			    /*********************************************************************/
			    printf("\nSocket failed");
			    break;
			}
		}
		/***************************************************************************/
		/* Advance current pointer to remaining part of data					   */
		/***************************************************************************/
		buf += bytes_sent;
		total_bytes_sent +=bytes_sent;
	}while(total_bytes_sent< length);

	if(total_bytes_sent == length)
	{
		printf("\nMessage sent in full");
		success = TRUE;
	}

	TRACE_EXIT();
	return (success);
} /* hm_tprt_send_on_socket */

/**PROC+**********************************************************************/
/* Name:     hm_tprt_recv_on_socket  	                                     */
/*                                                                           */
/* Purpose:   Receives data from socket				                         */
/*                                                                           */
/* Returns:   int32_t total_read_bytes : Number of bytes read from socket.       */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	: sock_fd - Socket on which data is to be received.      */
/*			  IN	: msg_buffer - Buffer to place data into.		         */
/*			  IN	: length - Length of data to be received.			     */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/

uint32_t hm_tprt_recv_on_socket(int32_t sock_fd,HM_TPRT_CONN_CB* conn_cb ,
							BYTE * msg_buffer, uint32_t length)
{
	/***************************************************************************/
	/* Local variables														   */
	/***************************************************************************/
	int32_t total_bytes_rcvd=0;
	uint32_t bytes_rcvd=0;
	int32_t os_error;
	int32_t op_complete = FALSE;
	BYTE *buf = msg_buffer;
	struct sockaddr *src_addr;
	extern fd_set hm_tprt_conn_set;

	TRACE_ENTRY("hm_tprt_recv_on_socket");
	SOCKADDR_IN* ip_addr = (SOCKADDR_IN*)MALLOC(sizeof(SOCKADDR_IN));
	src_addr = (SOCKADDR_IN *)MALLOC(sizeof(SOCKADDR_IN));
    /***************************************************************************/
	/* Now try to receive data												   */
	/***************************************************************************/
	do
	{
		printf("\nTry to receive %d bytes on Socket %d", (length - bytes_rcvd), sock_fd);

		if(conn_cb->sock_cb.sock_type==TCP_CONNECTION)
		{
			bytes_rcvd = recv(sock_fd,
							  buf,
							  (length - bytes_rcvd),
							  0);
		}

		else if(conn_cb->sock_cb.sock_type==UDP_UCAST_CONNECTION)
		{
			 bytes_rcvd = recvfrom(sock_fd,
                                      buf,
                                      (length - bytes_rcvd),
                                       0,
                                       (struct sockaddr *)&src_addr	,
                                       (socklen_t *)sizeof(struct sockaddr_in));

			 ip_addr=(SOCKADDR_IN *)src_addr;

			 MEMCPY(&conn_cb->ip,&ip_addr,(sizeof(SOCKADDR_IN)));
		}

		else if(conn_cb->sock_cb.sock_type==UDP_MCAST_CONNECTION)
		{
			 bytes_rcvd = recvfrom(sock_fd,
                                     buf,
                                     (length - bytes_rcvd),
                                      0,
                                      (struct sockaddr *)&src_addr,
                                      (socklen_t *) sizeof (struct sockaddr_in));

			 ip_addr=(SOCKADDR_IN *)src_addr;
			 MEMCPY(&conn_cb->ip,&ip_addr,(sizeof(SOCKADDR_IN)));
		}

		if(bytes_rcvd == length)
		{
			printf("\nMessage received in full");
			buf = NULL;
			op_complete = TRUE;
			total_bytes_rcvd +=bytes_rcvd;
			break;
		}
		else if (bytes_rcvd == -1)
		{
			/***********************************************************************/
			/* An error was returned - check for retryable socket error values.    */
			/***********************************************************************/
			printf("\nRecv failed on socket");

			perror(" Error : ");
			os_error = errno;
			if ((os_error == EWOULDBLOCK))
			{
				/*********************************************************************/
			    /* This seems to be a flow control condition - clear the bytes sent  */
			    /* value to show that no data was written.                           */
			    /*********************************************************************/
				printf("\nResource shortage - try again");
			    bytes_rcvd = 0;
			 }
			else if (os_error == EINTR)
			{
				printf("\nSend interrupted - loop round again");
			    bytes_rcvd = 0;
			}
			else
			{
				/*********************************************************************/
			    /* Socket failed so work source needs to be unregistered.            */
			    /*********************************************************************/
				printf("\nSocket failed");
				FD_CLR(sock_fd, &hm_tprt_conn_set);
			    op_complete = TRUE;
			    break;
			}
		}
		else if(bytes_rcvd == 0)
		{
			printf("\nThe peer has disconnected");
			op_complete = TRUE;
			FD_CLR(sock_fd, &hm_tprt_conn_set);
			break;
		}
		/***************************************************************************/
		/* Advance current pointer to remaining part of data					   */
		/***************************************************************************/
		buf += bytes_rcvd;
		total_bytes_rcvd +=bytes_rcvd;
	}while(total_bytes_rcvd< length);

	if(op_complete == TRUE)
	{
		if(total_bytes_rcvd == length)
		{
			printf("\nMessage received in full");
			buf = NULL;
		}
		else
		{
			printf("\nSome error happened");
			buf = NULL;
		}
	}

	TRACE_EXIT();
	return (total_bytes_rcvd);
} /* hm_tprt_recv_on_socket */
#endif
