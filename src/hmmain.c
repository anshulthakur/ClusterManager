/*
 * hmmain.c
 *
 *  Created on: 07-May-2015
 *      Author: anshul
 */

#define HM_MAIN_DEFINE_VARS
#include <hmincl.h>

/***************************************************************************/
/* Name:	main 														   */
/* Parameters: Input - 	Stdargs											   */
/*			   Input/Output -											   */
/* Return:	int															   */
/* Purpose: Main routine												   */
/***************************************************************************/
int32_t main(int32_t argc, char **argv)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	extern char *optarg;
	extern int32_t optind;

	struct sigaction sa;

	int32_t cmd_opt;
	int32_t ret_val = HM_OK;

	char *config_file = "config.xml";
	HM_CONFIG_CB *config_cb = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Setup signal handler for Ctrl+C										   */
	/***************************************************************************/
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = hm_interrupt_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        printf("Failed to setup signal handling for hardware manager.\n");
        ret_val = HM_ERR;
        goto EXIT_LABEL;
    }
    /***************************************************************************/
	/* Setup signal handler for timers										   */
	/***************************************************************************/
    sa.sa_sigaction = hm_base_timer_handler;
    sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
	{
		TRACE_ERROR(("Failed to setup signal handling for timers"));
		goto EXIT_LABEL;
	}

	/* Timer table */
	HM_AVL3_INIT_TREE(global_timer_table, timer_table_by_handle);

	/***************************************************************************/
	/* Initialize Logging													   */
	/***************************************************************************/

	/***************************************************************************/
	/* Allocate Configuration Control Block									   */
	/* This will load default options everywhere							   */
	/***************************************************************************/
	config_cb = hm_alloc_config_cb();
	TRACE_ASSERT(config_cb != NULL);
	if(config_cb == NULL)
	{
		TRACE_ERROR(("Error allocating Configuration Control Block. System will quit."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Parse command line arguments to find the name of configuration file 	   */
	/***************************************************************************/
	while ((cmd_opt=getopt(argc, argv, "c:h")) !=-1)
	{
		switch(cmd_opt)
		{
		case 'c':
			config_file = optarg;
			break;

		case 'h':
		default:
			printf("\nUsage:\n <cmd> [-c <config file name>]\n");
			break;
		}
	}
	/***************************************************************************/
	/* If file is present, read the XML Tree into the configuration	CB		   */
	/* NOTE: access() is POSIX standard, but it CAN cause race conditions.	   */
	/* replace if necessary.												   */
	/***************************************************************************/
	if(access(config_file, F_OK) != -1)
	{
		TRACE_INFO(("Using %s for Configuration", config_file));
		ret_val = hm_parse_config(config_cb, config_file);
	}
	else
	{
		TRACE_WARN(("File %s does not exist. Use default configuration", config_file));
		/***************************************************************************/
		/* We need not parse anything now, since we already loaded up defaults 	   */
		/***************************************************************************/
	}
	/***************************************************************************/
	/* By now, we will be aware of all things we need to start the HM		   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize HM Local structure										   */
	/***************************************************************************/
	ret_val = hm_init_local(config_cb);
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Local Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Initialize Transport Layer											   */
	/***************************************************************************/
	ret_val = hm_init_transport();
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Transport Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Start Hardware Location Layer										   */
	/* Cluster Layer: Discovery and other routines							   */
	/***************************************************************************/
	ret_val = hm_init_location_layer();
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Location Layer Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Start Eternal Select Loop											   */
	/***************************************************************************/
	hm_run_main_thread();

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* main */


/***************************************************************************/
/* Name:	hm_init_local 									*/
/* Parameters: Input - 	config_cb: Configuration structure read			   */
/*																		   */
/* Return:	int32_t														   */
/* Purpose: Initializes local data strcuture which houses all Global Data  */
/***************************************************************************/
int32_t hm_init_local(HM_CONFIG_CB *config_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	HM_CONFIG_NODE_CB *node_config_cb = NULL;

	SOCKADDR_IN *sock_addr = NULL;
	HM_CONFIG_SUBSCRIPTION_CB *subs = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(config_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize Trees														   */
	/***************************************************************************/
	/* Aggregate Nodes Tree	*/
	HM_AVL3_INIT_TREE(LOCAL.locations_tree, locations_tree_by_hardware_id);
	LOCAL.next_loc_tree_id = 1;

	/* Aggregate Process Tree */
	HM_AVL3_INIT_TREE(LOCAL.nodes_tree, nodes_tree_by_db_id);
	LOCAL.next_node_tree_id = 1;

	/* Aggregate PID Tree	*/
	HM_AVL3_INIT_TREE(LOCAL.pid_tree, process_tree_by_db_id);
	LOCAL.next_pid_tree_id = 1;

	/* Aggregate Interfaces Tree */
	HM_AVL3_INIT_TREE(LOCAL.interface_tree, interface_tree_by_db_id);
	LOCAL.next_pid_tree_id = 1;

	/* Active Joins Tree */
	HM_AVL3_INIT_TREE(LOCAL.active_subscriptions_tree, NULL);

	/* Broken/Pending Joins Tree */
	HM_AVL3_INIT_TREE(LOCAL.pending_subscriptions_tree, NULL);
	LOCAL.next_pending_tree_id = 1;

	/* Notifications Queue */
	HM_INIT_ROOT(LOCAL.notification_queue);

	/* Wildcard Queue */
	HM_INIT_ROOT(LOCAL.table_root_subscribers);

	/***************************************************************************/
	/* Initialize the connection list										   */
	/***************************************************************************/
	HM_INIT_ROOT(LOCAL.conn_list);

	/***************************************************************************/
	/* Fill in Local Location CB information								   */
	/***************************************************************************/
	LOCAL.local_location_cb.id = config_cb->instance_info.index;
	LOCAL.local_location_cb.index = config_cb->instance_info.index;
	LOCAL.local_location_cb.table_type = HM_TABLE_TYPE_LOCATION_LOCAL;

	TRACE_DETAIL(("Hardware Index: %d", LOCAL.local_location_cb.index));

	LOCAL.transport_bitmask = 0;

	LOCAL.local_location_cb.timer_cb = NULL;
	LOCAL.node_keepalive_period = config_cb->instance_info.node.timer_val;
	TRACE_INFO(("Node Keepalive Period: %d(ms)",LOCAL.node_keepalive_period));
	LOCAL.node_kickout_value = config_cb->instance_info.node.threshold;
	TRACE_INFO(("Node Keepalive Threshold: %d",LOCAL.node_kickout_value));

	LOCAL.peer_keepalive_period = config_cb->instance_info.node.timer_val;
	TRACE_INFO(("Peer Keepalive Period: %d(ms)",LOCAL.peer_keepalive_period));
	LOCAL.peer_kickout_value = config_cb->instance_info.node.threshold;
	TRACE_INFO(("Peer Keepalive Threshold: %d",LOCAL.peer_kickout_value));

	/* TCP Info */
	if(config_cb->instance_info.tcp != NULL)
	{
		TRACE_DETAIL(("TCP Information is provided."));
		memcpy(	&LOCAL.local_location_cb.tcp_addr,
				&(config_cb->instance_info.tcp->address),
				sizeof(HM_INET_ADDRESS));
		sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.tcp_addr.address;
		TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.tcp_addr.type));
#ifdef I_WANT_TO_DEBUG
		{
			char tmp[100];
			TRACE_INFO(("Address IP: %s", inet_ntop(AF_INET, &sock_addr->sin_addr, tmp, sizeof(tmp))));
		}
#endif
		TRACE_DETAIL(("Port: %d", sock_addr->sin_port));
		/***************************************************************************/
		/* Set LSB to 1															   */
		/***************************************************************************/
		TRACE_DETAIL(("Set TCP in BIT Mask"));
		LOCAL.transport_bitmask = (LOCAL.transport_bitmask | (1 << HM_TRANSPORT_TCP_LISTEN));
		TRACE_DETAIL(("0x%x", LOCAL.transport_bitmask));
	}
	LOCAL.local_location_cb.node_listen_cb = NULL;

	/* UDP Info */
	if(config_cb->instance_info.udp != NULL)
	{
		TRACE_DETAIL(("UDP Information is provided."));
		memcpy(	&LOCAL.local_location_cb.udp_addr,
				&(config_cb->instance_info.udp->address),
				sizeof(HM_INET_ADDRESS));
		sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.udp_addr.address;
		TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.udp_addr.type));
#ifdef I_WANT_TO_DEBUG
		{
			char tmp[100];
			TRACE_INFO(("Address IP: %s", inet_ntop(AF_INET,
					&sock_addr->sin_addr, tmp, sizeof(tmp))));
		}
#endif
		TRACE_DETAIL(("Port: %d", sock_addr->sin_port));
		TRACE_DETAIL(("Set UDP in transport bitmask"));
		LOCAL.transport_bitmask = (LOCAL.transport_bitmask | (1 << HM_TRANSPORT_UDP));
		TRACE_DETAIL(("0x%x", LOCAL.transport_bitmask));
	}
	LOCAL.local_location_cb.peer_listen_cb = NULL;

	/* Multicast Info */
	if(config_cb->instance_info.mcast != NULL)
	{
		TRACE_DETAIL(("Multicast Information is provided."));
		memcpy(	&LOCAL.local_location_cb.mcast_addr,
				&(config_cb->instance_info.mcast->address),
				sizeof(HM_INET_ADDRESS));
		sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.mcast_addr.address;
		TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.mcast_addr.type));
#ifdef I_WANT_TO_DEBUG
		{
			char tmp[100];
			TRACE_INFO(("Address IP: %s", inet_ntop(AF_INET,
					&sock_addr->sin_addr, tmp, sizeof(tmp))));
		}
#endif
		TRACE_DETAIL(("Port: %d", sock_addr->sin_port));
		LOCAL.local_location_cb.mcast_addr.mcast_group = config_cb->instance_info.mcast_group;
		TRACE_DETAIL(("Multicast Group: %d", LOCAL.local_location_cb.mcast_addr.mcast_group));
		TRACE_DETAIL(("Set Multicast in transport bitmask"));
		LOCAL.transport_bitmask = (LOCAL.transport_bitmask | (1 << HM_TRANSPORT_MCAST));
		TRACE_DETAIL(("0x%x", LOCAL.transport_bitmask));
	}
	LOCAL.local_location_cb.peer_broadcast_cb = NULL;

	if(HM_NEXT_IN_LIST(config_cb->instance_info.addresses) != NULL)
	{
		TRACE_INFO(("Relative Peer information has been provided."));
		//TODO: Allocate a Location CB and fill its transport

	}

	/* FIXME: Timer resolution must be fixed for seconds */
	LOCAL.node_keepalive_period = config_cb->instance_info.node.timer_val;
	TRACE_INFO(("Node Keepalive period: %d", LOCAL.node_keepalive_period));

	LOCAL.node_kickout_value = config_cb->instance_info.node.threshold;
	TRACE_INFO(("Node Kickout Value: %d", LOCAL.node_kickout_value));

	/* FIXME: Timer resolution must be fixed for seconds */
	LOCAL.peer_keepalive_period = config_cb->instance_info.cluster.timer_val;
	TRACE_INFO(("Peer Keepalive period: %d", LOCAL.node_keepalive_period));

	LOCAL.peer_kickout_value = config_cb->instance_info.cluster.threshold;
	TRACE_INFO(("Peer Kickout Value: %d", LOCAL.peer_kickout_value));

	LOCAL.config_data = config_cb;

	/***************************************************************************/
	/* Add the location to global locations tree							   */
	/***************************************************************************/
	if(hm_global_location_add(&LOCAL.local_location_cb, HM_STATUS_RUNNING)!= HM_OK)
	{
		TRACE_ERROR(("Error initializing local location"));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	TRACE_DETAIL(("ID returned: %d", LOCAL.local_location_cb.id));

	/***************************************************************************/
	/* Initialize the Node tree and fill it up								   */
	/***************************************************************************/
	HM_AVL3_INIT_TREE(LOCAL.nodes_tree, NULL);
	if(HM_NEXT_IN_LIST(config_cb->node_list) != NULL)
	{
		for(node_config_cb = (HM_CONFIG_NODE_CB *)(HM_NEXT_IN_LIST(config_cb->node_list));
				node_config_cb != NULL;
				node_config_cb = (HM_CONFIG_NODE_CB *)(HM_NEXT_IN_LIST(node_config_cb->node)))
		{
			/***************************************************************************/
			/* Initialize a node (and it must also make entries to alter subscriptions)*/
			/***************************************************************************/
			TRACE_DETAIL(("Found node %s", node_config_cb->node_cb->name));
			/***************************************************************************/
			/* Add node to system													   */
			/***************************************************************************/
			if(hm_node_add(node_config_cb->node_cb, &LOCAL.local_location_cb) != HM_OK)
			{
				TRACE_ERROR(("Error adding node to system."));
				ret_val = HM_ERR;
				goto EXIT_LABEL;
			}
			/***************************************************************************/
			/* Subscribe to its dependency list.									   */
			/***************************************************************************/
			TRACE_DETAIL(("Making subscriptions."));
			for(subs = (HM_CONFIG_SUBSCRIPTION_CB *)HM_NEXT_IN_LIST(node_config_cb->subscriptions);
				subs != NULL;
				subs = (HM_CONFIG_SUBSCRIPTION_CB *)HM_NEXT_IN_LIST(subs->node))
			{
				if(hm_subscribe(subs->subs_type, subs->value, (void *)node_config_cb->node_cb) != HM_OK)
				{
					TRACE_ERROR(("Error creating subscriptions."));
					ret_val = HM_ERR;
					goto EXIT_LABEL;
				}
			}
		}
	}

	LOCAL.local_location_cb.fsm_state = HM_PEER_FSM_STATE_NULL;
	LOCAL.local_location_cb.keepalive_missed = 0;
	LOCAL.local_location_cb.keepalive_period = LOCAL.peer_keepalive_period;

	TRACE_ASSERT(global_timer_table.root != NULL);
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_local */

/***************************************************************************/
/* Name:	hm_init_transport 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Initializes Transport Layer									   */
/* 1. Set SIGIGN for dead sockets										   */
/***************************************************************************/
int32_t hm_init_transport()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	HM_TRANSPORT_CB *tprt_cb = NULL;

	struct sigaction action;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/* Allocate resources and pre-provision operations like binding and listen */
	/* on designated sockets.												   */
	/* Connection requests will be queued until we start polling. So, should   */
	/* not be a problem.													   */
	/***************************************************************************/
	/***************************************************************************/
	/* On writing to dead Sockets, do not panic.                               */
	/***************************************************************************/
	action.sa_handler = SIG_IGN;
  	sigaction(SIGPIPE, &action, NULL);

	/***************************************************************************/
	/* Set the global FD set 												   */
	/***************************************************************************/
	FD_ZERO(&hm_tprt_conn_set);

	/***************************************************************************/
	/* TCP Connection Setup													   */
	/***************************************************************************/
	TRACE_DETAIL(("Check for TCP: %d",(1 & LOCAL.transport_bitmask >> HM_TRANSPORT_TCP_LISTEN)));
	if((1 & LOCAL.transport_bitmask >> HM_TRANSPORT_TCP_LISTEN)== TRUE)
	{
		TRACE_INFO(("Start TCP IPv4 Listener"));
		tprt_cb = hm_alloc_transport_cb(HM_TRANSPORT_TCP_LISTEN);
		if(tprt_cb == NULL)
		{
			TRACE_ERROR(("Error allocating Transport structures for listening."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		tprt_cb->location_cb = &LOCAL.local_location_cb;

		tprt_cb->sock_cb =
				hm_tprt_open_connection(tprt_cb->type, (void *)&LOCAL.local_location_cb.tcp_addr);
		if(tprt_cb->sock_cb == NULL)
		{
			TRACE_ERROR(("Error initializing Listen socket"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}

		tprt_cb->sock_cb->tprt_cb = tprt_cb;
		LOCAL.local_location_cb.node_listen_cb = tprt_cb;
	}

	/***************************************************************************/
	/* UDP Connection Setup													   */
	/***************************************************************************/
	TRACE_DETAIL(("Check for UDP: %d",(1 & (LOCAL.transport_bitmask >> HM_TRANSPORT_UDP))));
	if((1 & (LOCAL.transport_bitmask >> HM_TRANSPORT_UDP))== TRUE)
	{
		TRACE_INFO(("Start UDP Server"));
		tprt_cb = hm_alloc_transport_cb(HM_TRANSPORT_UDP);
		if(tprt_cb == NULL)
		{
			TRACE_ERROR(("Error allocating Transport structures for UDP Server."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		tprt_cb->location_cb = &LOCAL.local_location_cb;

		tprt_cb->sock_cb =
				hm_tprt_open_connection(tprt_cb->type, (void *)&LOCAL.local_location_cb.udp_addr);
		if(tprt_cb->sock_cb == NULL)
		{
			TRACE_ERROR(("Error initializing UDP server socket"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		LOCAL.local_location_cb.peer_listen_cb = tprt_cb;
		tprt_cb->sock_cb->tprt_cb = tprt_cb;
	}

	/***************************************************************************/
	/* Multicast Connection Setup											   */
	/***************************************************************************/
	TRACE_DETAIL(("Check for Mcast: %d",(1 & (LOCAL.transport_bitmask >> HM_TRANSPORT_MCAST))));
	if((1 & (LOCAL.transport_bitmask >> HM_TRANSPORT_MCAST))== TRUE)
	{
		TRACE_INFO(("Start Multicast Service"));
		tprt_cb = hm_alloc_transport_cb(HM_TRANSPORT_MCAST);
		if(tprt_cb == NULL)
		{
			TRACE_ERROR(("Error allocating Transport structures for Multicast service."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		tprt_cb->location_cb = &LOCAL.local_location_cb;

		tprt_cb->sock_cb =
				hm_tprt_open_connection(tprt_cb->type, (void *)&LOCAL.local_location_cb.mcast_addr);
		if(tprt_cb->sock_cb == NULL)
		{
			TRACE_ERROR(("Error initializing Multicast socket"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		LOCAL.local_location_cb.peer_broadcast_cb = tprt_cb;
		tprt_cb->sock_cb->tprt_cb = tprt_cb;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	if(ret_val == HM_ERR)
	{
		if(tprt_cb != NULL)
		{
			hm_free_transport_cb(tprt_cb);
		}
	}
	TRACE_EXIT();
	return ret_val;
}/* hm_init_transport */

/***************************************************************************/
/* Name:	hm_run_main_thread 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	void														   */
/* Purpose: The main select loop of this program that monitors incoming    */
/* requests and/or schedules the rest of things.						   */
/***************************************************************************/
void hm_run_main_thread()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t nready; //Number of ready descriptors

	HM_SOCKET_CB *sock_cb;
	extern fd_set hm_tprt_conn_set;
	extern int32_t max_fd;
	struct timeval select_timeout;

	fd_set read_set;

	HM_MSG *buf = NULL;

	HM_NODE_INIT_MSG *init_msg = NULL;
	int32_t bytes_rcvd = HM_ERR;

	HM_SUBSCRIBER node_cb;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Start Endless loop to process incoming data							   */
	/***************************************************************************/
	while(1)
	{
		/*start loop to check for incoming events till kingdom come! 			   */
		/***************************************************************************/
		/* Check if the signal has been blocked. If it has been, proceed with      */
		/* select. Else, block it and unblock at the end of while. This is done to */
		/* ensure that there are no concurrency problems arising from the          */
		/* interrupt routines changing the values of variables being accessed by   */
		/* ongoing routine/callback.											   */
		/***************************************************************************/
		select_timeout.tv_sec = 0;
		select_timeout.tv_usec = /*300000*/0;

		read_set = hm_tprt_conn_set;
		/***************************************************************************/
		/* Block the signal without seeing what was in previously.   			   */
		/***************************************************************************/
	    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
	    {
			TRACE_PERROR(("Error blocking signals before querying descriptors."));
	    }

		/***************************************************************************/
		/* Check for listen/read events											   */
		/***************************************************************************/
		nready = select(max_fd+1, &read_set, NULL, NULL, &select_timeout);
		if(nready == 0)
		{
			/***************************************************************************/
			/* No descriptors are ready												   */
			/***************************************************************************/
			continue;
		}
		else if(nready == -1)
		{
			TRACE_PERROR(("Error Occurred on select() call."));
			continue;
		}

		/***************************************************************************/
		/* Check for incoming connections										   */
		/***************************************************************************/
		if(FD_ISSET(LOCAL.local_location_cb.node_listen_cb->sock_cb->sock_fd, &read_set))
		{
			/***************************************************************************/
			/* Accept the connection only if we have not exceeded top-load benchmark   */
			/***************************************************************************/
			TRACE_INFO(("Listen Port has requests."));

			sock_cb = hm_tprt_accept_connection(
								LOCAL.local_location_cb.node_listen_cb->sock_cb->sock_fd);
			if(sock_cb == NULL)
			{
				TRACE_ERROR(("Error accepting connection request."));
				goto EXIT_LABEL;
			}
			/***************************************************************************/
			/* If select returned 1, and it was a listen socket, it makes sense to poll*/
			/* again by breaking out and use select again.							   */
			/***************************************************************************/
			if(--nready <=0)
			{
				TRACE_DETAIL(("No more incoming requests."));
				continue;
			}
		}//end select on listenfd
		/***************************************************************************/
		/* If Cluster port is also listen type TCP, then accept connection on it   */
		/* NOTE: We must abstract this entire thing from the upper layers. They    */
		/* don't need to know whether it is TCP or UDP or anything, they just need */
		/* to know that they have a request and their level of PDU.				   */
		/***************************************************************************/
		if((LOCAL.local_location_cb.peer_listen_cb->type == HM_TRANSPORT_TCP_LISTEN)||
				(LOCAL.local_location_cb.peer_listen_cb->type == HM_TRANSPORT_TCP_IPv6_LISTEN))
		{
			/***************************************************************************/
			/* Accept the connection only if we have not exceeded top-load benchmark   */
			/***************************************************************************/
			TRACE_INFO(("Cluster Port has requests."));
			sock_cb = hm_tprt_accept_connection(
								LOCAL.local_location_cb.peer_listen_cb->sock_cb->sock_fd);
			if(sock_cb == NULL)
			{
				TRACE_ERROR(("Error accepting connection request."));
				goto EXIT_LABEL;
			}
			/***************************************************************************/
			/* If select returned 1, and it was a listen socket, it makes sense to poll*/
			/* again by breaking out and use select again.							   */
			/***************************************************************************/
			if(--nready <=0)
			{
				TRACE_DETAIL(("No more incoming requests."));
				continue;
			}
		}
		/***************************************************************************/
		/* There are more read events waiting to be processed.					   */
		/***************************************************************************/
		for(sock_cb = (HM_SOCKET_CB *)HM_NEXT_IN_LIST(LOCAL.conn_list);
				sock_cb != NULL;
				sock_cb = HM_NEXT_IN_LIST(sock_cb->node))
		{
			if(FD_ISSET(sock_cb->sock_fd, &read_set))
			{
				/***************************************************************************/
				/* The connection is valid and read event has occured here.			       */
				/***************************************************************************/
				TRACE_DETAIL(("FD %d has data",sock_cb->sock_fd));

				/***************************************************************************/
				/* If its parent transport CB has been set, then we can pass the message   */
				/* notification to the upper layer. If not, then we are expecting an INIT  */
				/* message.																   */
				/***************************************************************************/
				if(sock_cb->tprt_cb == NULL)
				{
					TRACE_DETAIL(("Socket is currently not associated with any Node."));
					/***************************************************************************/
					/* We'll need to receive the message into a random buffer which must be as */
					/* large as at least the INIT message.									   */
					/***************************************************************************/
					buf = hm_get_buffer(sizeof(HM_NODE_INIT_MSG));
					TRACE_ASSERT(buf != NULL);
					if(buf == NULL)
					{
						TRACE_ERROR(("Error allocating buffer for incoming message."));
						goto EXIT_LABEL;
					}
					/***************************************************************************/
					/* First, receive the message header and verify that it is an INIT message */
					/***************************************************************************/
					if((bytes_rcvd = hm_tprt_recv_on_socket(	sock_cb->sock_fd,
												HM_TRANSPORT_TCP_IN,
												buf->msg,
												sizeof(HM_MSG_HEADER)
												))== HM_ERR)
					{
						TRACE_ERROR(("Error receiving message on socket."));
						TRACE_ASSERT(FALSE);
						//It is an error, but we'll still run (in release mode)
						//FIXME
						break;
					}
					if(bytes_rcvd == sizeof(HM_MSG_HEADER))
					{
						init_msg = (HM_NODE_INIT_MSG *)buf->msg;
						if((init_msg->hdr.msg_type != HM_MSG_TYPE_INIT) || (init_msg->hdr.request != TRUE))
						{
							TRACE_WARN(("Message type is not INIT request. Ignore."));
							/***************************************************************************/
							/* Receive the rest of the message and discard the buffer.				   */
							/***************************************************************************/
							if((bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
																	HM_TRANSPORT_TCP_IN,
																	(BYTE *)buf->msg + bytes_rcvd,
																	sizeof(HM_NODE_INIT_MSG)
																	- sizeof(HM_MSG_HEADER)
																	))== HM_ERR)
							{
								TRACE_ERROR(("Error receiving rest of data"));
								TRACE_ASSERT(FALSE);
								//TODO: Error handling? Exit or continue?
							}
							hm_free_buffer(buf);
						}
						else
						{
							/***************************************************************************/
							/* It is an INIT request. Read the rest of the message and associate with  */
							/* a node.																   */
							/***************************************************************************/
							TRACE_INFO(("INIT request received. Associate with a Node"));
							if((bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
																		HM_TRANSPORT_TCP_IN,
																		(BYTE *)buf->msg + bytes_rcvd,
																		sizeof(HM_NODE_INIT_MSG)
																		- sizeof(HM_MSG_HEADER)
																		))== HM_ERR)
							{
								TRACE_ERROR(("Error receiving rest of data"));
								TRACE_ASSERT(FALSE);
								//TODO: Error handling? Exit or continue?
							}
							TRACE_INFO(("INIT Request from Node Index: %d, Group %d",
													init_msg->index, init_msg->service_group_index));
							/***************************************************************************/
							/* Find a node with this index and group in current table				   */
							/* NOTE: Since indexes are currently guaranteed to be unique, the lookup is*/
							/* based on node index only, and group number checking is then done here   */
							/* itself.																   */
							/***************************************************************************/
							//TODO
							node_cb.proper_node_cb = (HM_NODE_CB *)HM_AVL3_FIND(
															LOCAL.local_location_cb.node_tree,
															&init_msg->index,
															nodes_tree_by_node_id
															);
							if(node_cb.proper_node_cb == NULL)
							{
								TRACE_ERROR(("No such node found in Local CB"));
								hm_free_buffer(buf);
								/***************************************************************************/
								/* Continue receiving on other sockets if any							   */
								/***************************************************************************/
								continue;
							}
							if(node_cb.proper_node_cb->group != init_msg->service_group_index)
							{
								TRACE_ERROR(("The Group index %d of node in system does not match with reported",
										node_cb.proper_node_cb->group));
								hm_free_buffer(buf);
								/***************************************************************************/
								/* Continue receiving on other sockets if any							   */
								/***************************************************************************/
								continue;
							}
							TRACE_ASSERT(node_cb.proper_node_cb->transport_cb != NULL);
							/***************************************************************************/
							/* Put message as the input buffer of the transport.					   */
							/***************************************************************************/
							node_cb.proper_node_cb->transport_cb->in_buffer = (char *)buf;

							/***************************************************************************/
							/* Fix pointers															   */
							/***************************************************************************/
							node_cb.proper_node_cb->transport_cb->sock_cb = sock_cb;
							sock_cb->tprt_cb = node_cb.proper_node_cb->transport_cb;

							/***************************************************************************/
							/* Call into Node FSM signifying an INIT receive message.				   */
							/***************************************************************************/
							hm_node_fsm(HM_NODE_FSM_INIT, node_cb.proper_node_cb);

							/***************************************************************************/
							/* We no longer need this buffer. If someone else is using it, the ref_cnt */
							/* would have increased and the other consumer can still use it.		   */
							/***************************************************************************/
							hm_free_buffer(buf);
							//TODO
						}
					}
				}
				else
				{
					TRACE_DETAIL(("Socket associated with Transport of Location %d",
													sock_cb->tprt_cb->location_cb->index));
					/***************************************************************************/
					/* Transport will have at least the space for a Message Header. Get bytes  */
					/* into that buffer and then invoke the layer specific code.			   */
					/***************************************************************************/
					/***************************************************************************/
					/* We'll need to receive the message into a random buffer which must be as */
					/* large as at least the INIT message.									   */
					/***************************************************************************/
					sock_cb->tprt_cb->in_buffer = (char *)&sock_cb->tprt_cb->header;
					/***************************************************************************/
					/* First, receive the message header and verify that it is an INIT message */
					/***************************************************************************/
					bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
												HM_TRANSPORT_TCP_IN,
												sock_cb->tprt_cb->in_buffer,
												sizeof(HM_MSG_HEADER)
												);
					if(bytes_rcvd != sizeof(HM_MSG_HEADER))
					{
						TRACE_DETAIL(("Message Length of %d was expected, %d was received",
										sizeof(HM_MSG_HEADER), bytes_rcvd));
						hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
						//TODO: Quit or Continue?
					}
					else
					{
						//TODO
						//Pass to message router to handle message accordingly
						hm_route_incoming_message(sock_cb);
						//Based on the type of message, process it.
						//PROC_CREATE/DESTROY handled by Process layer

					}
					/* Don't need to free the buffer, because it is a statically allocated memory */
				}

				/***************************************************************************/
				/* Read the incoming message. This FD is now processed.				 	   */
				/***************************************************************************/
				if(--nready <=0)
				{
					TRACE_DETAIL(("No more incoming requests."));
					break;	//No more descriptors to process. Break out and poll again.
				}// end if --nready <=0
			}// end if FD_ISSET(sockfd, &rset)
		}//end for

		/***************************************************************************/
		/* Unblock the signal.													   */
		/***************************************************************************/
	    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
	    {
	    		TRACE_PERROR(("Error unblocking signals"));
	    		//goto EXIT_LABEL;
	    }
	}//end while(1)
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return;
}/* hm_run_main_thread */

/***************************************************************************/
/* Name:	hm_init_location_layer 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Initialize Hardware Location Layer			*/
/***************************************************************************/
int32_t hm_init_location_layer()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize Local Location CB											   */
	/***************************************************************************/
	/* Initialize FD_SET */

	/***************************************************************************/
	/* Initialize Transport CB for Listen									   */
	/***************************************************************************/
	/*
    * Initialize Socket Connection CB:
        * Set Connection Type
        * Open Socket
        * Set Transport Location CB Pointer
        * Set Node CB to NULL
        * Add Sock to FD_SET
    * Start listen on TCP port
	*/

	/***************************************************************************/
	/* Initialize Transport CB for Unicast									   */
	/***************************************************************************/
	/*
	 * Initialize TCB for Unicast Port
        * Initialize Socket Connection CB:
            * Open UDP Unicast port
    */

	/***************************************************************************/
	/* Initialize Cluster Specific Function									   */
	/***************************************************************************/
	/*
    * If Cluster enabled:
        * Initialize TCB for Multicast Port
            * Open UDP Multicast port
            * Join Multicast Group
     */

	//hm_init_location_cb(HM_LOCATION_CB &LOCAL.local_location_cb)

	/***************************************************************************/
	/* For each node in Configuration, initialize Node CB and Fill it up	   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_location_layer */


/**PROC+**********************************************************************/
/* Name:     hm_interrupt_handler  		                                     */
/*                                                                           */
/* Purpose:  Invoked when Ctrl+C or any event that triggers SIGINT happens.  */
/*			 Closes the system properly.									 */
/*                                                                           */
/* Returns:   VOID  :											             */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 							                                 */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/

void hm_interrupt_handler(int32_t sig, siginfo_t *info, void *data)
{
	TRACE_ENTRY();

	if ( info == NULL )
	{
		TRACE_INFO(("Signal Received: [%d]",sig));
		TRACE_INFO(("Shutting Down"));
	}
	hm_terminate();
	/***************************************************************************/
	/* We're not getting here.												   */
	/***************************************************************************/
	TRACE_EXIT();
	return;
} /* hm_interrupt_handler */

/**PROC+**********************************************************************/
/* Name:     hm_terminate  		                                             */
/*                                                                           */
/* Purpose:  Initializes the closure of the module.                          */
/*                                                                           */
/* Returns:   VOID  :											             */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 							                                 */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: Calls into each of the sub-modules terminate functions, if     */
/*			any, and later, exits from the code.							 */
/*                                                                           */
/**PROC-**********************************************************************/

void hm_terminate()
{
	TRACE_ENTRY();

	/***************************************************************************/
	/* Close the socket connections.										   */
	/***************************************************************************/
	//hm_tprt_terminate();

	TRACE_EXIT();
	exit(0);
} /* hm_terminate */
