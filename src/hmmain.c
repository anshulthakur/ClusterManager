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
	/***************************************************************************/
	ret_val = hm_init_location_layer();
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Location Layer Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}


EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* main */


/***************************************************************************/
/* Name:	hm_init_local 									*/
/* Parameters: Input - 	config_cb: Configuration structure read									*/
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
	HM_NODE_CB *node_cb = NULL;
	SOCKADDR_IN *sock_addr = NULL;
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
	/* Timer table */
	HM_AVL3_INIT_TREE(global_timer_table, timer_table_by_handle);
	/* Aggregate Nodes Tree	*/
	HM_AVL3_INIT_TREE(LOCAL.locations_tree, locations_tree_by_hardware_id);

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

	/***************************************************************************/
	/* Fill in Local Location CB information								   */
	/***************************************************************************/
	LOCAL.local_location_cb.id = config_cb->instance_info.index;
	LOCAL.transport_bitmask = 0;

	LOCAL.local_location_cb.timer_cb = NULL;

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
			//TODO
			//Add Node to tree
		}
	}

	LOCAL.local_location_cb.fsm_state = HM_PEER_FSM_STATE_NULL;
	LOCAL.local_location_cb.keepalive_missed = 0;
	LOCAL.local_location_cb.keepalive_period = LOCAL.peer_keepalive_period;

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
