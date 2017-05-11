/*
 * hmmain.c
 *
 *  Created on: 07-May-2015
 *      Author: anshul
 */
/**
 *  @file hmmain.c
 *  @brief Main entry point routines and loop methods of module
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None
 */
#define HM_MAIN_DEFINE_VARS
#include <hmincl.h>

/**
 *  @brief Main routine
 *
 *  @param argc
 *  @param **argv
 *  @return #HM_OK on normal exit, #HM_ERR on error
 */
int32_t main(int32_t argc, char **argv)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  extern char *optarg;
  extern int32_t optind;

  struct sigaction sa;

  int32_t cmd_opt;
  int32_t ret_val = HM_OK;

  char *config_file = HM_DEFAULT_CONFIG_FILE_NAME;
  HM_CONFIG_CB *config_cb = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Setup signal handler for Ctrl+C                       */
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
  /* Setup signal handler for timers                       */
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
  /* Initialize Logging                                                      */
  /***************************************************************************/

  /***************************************************************************/
  /* Allocate Configuration Control Block                     */
  /* This will load default options everywhere                 */
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
  /* Parse command line arguments to find the name of configuration file      */
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
  /* If file is present, read the XML Tree into the configuration  CB       */
  /* NOTE: access() is POSIX standard, but it CAN cause race conditions.     */
  /* replace if necessary.                           */
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
    /* We need not parse anything now, since we already loaded up defaults      */
    /***************************************************************************/
  }
  /***************************************************************************/
  /* By now, we will be aware of all things we need to start the HM       */
  /***************************************************************************/
  LOCAL.config_data = config_cb;
  /***************************************************************************/
  /* Initialize HM Local structure                       */
  /***************************************************************************/
  ret_val = hm_init_local(config_cb);
  if(ret_val != HM_OK)
  {
    TRACE_ERROR(("Hardware Manager Local Initialization Failed."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Initialize Transport Layer                         */
  /***************************************************************************/
  ret_val = hm_init_transport();
  if(ret_val != HM_OK)
  {
    TRACE_ERROR(("Hardware Manager Transport Initialization Failed."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Start Hardware Location Layer                       */
  /* Cluster Layer: Discovery and other routines                 */
  /***************************************************************************/
  ret_val = hm_init_location_layer();
  if(ret_val != HM_OK)
  {
    TRACE_ERROR(("Hardware Manager Location Layer Initialization Failed."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Start Eternal Select Loop                         */
  /***************************************************************************/
  hm_run_main_thread();

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  hm_terminate();
  TRACE_EXIT();
  return ret_val;
}/* main */


/**
 *  @brief Initializes local data strcuture which houses all Global Data
 *
 *  @param *config_cb A well formed Configuration CB obtained after parsing
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_init_local(HM_CONFIG_CB *config_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_CONFIG_NODE_CB *node_config_cb = NULL;

  SOCKADDR_IN *sock_addr = NULL;
  HM_CONFIG_SUBSCRIPTION_CB *subs = NULL;

  HM_TRANSPORT_CB *tprt_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(config_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Initialize Trees                               */
  /***************************************************************************/
  /* Aggregate Locations Tree  */
  HM_AVL3_INIT_TREE(LOCAL.locations_tree, locations_tree_by_hardware_id);
  LOCAL.next_loc_tree_id = 1;

  /* Aggregate Nodes Tree */
  HM_AVL3_INIT_TREE(LOCAL.nodes_tree, nodes_tree_by_db_id);
  LOCAL.next_node_tree_id = 1;

  /* Aggregate Processes Tree */
  HM_AVL3_INIT_TREE(LOCAL.process_tree, global_process_tree_by_id);
  LOCAL.next_process_tree_id = 1;

  /* Aggregate PID Tree  */
  HM_AVL3_INIT_TREE(LOCAL.pid_tree, process_tree_by_db_id);
  LOCAL.next_pid_tree_id = 1;

  /* Aggregate Interfaces Tree */
  HM_AVL3_INIT_TREE(LOCAL.interface_tree, interface_tree_by_db_id);
  LOCAL.next_interface_tree_id = 1;

  /* Active Joins Tree */
  HM_AVL3_INIT_TREE(LOCAL.active_subscriptions_tree, NULL);

  /* Broken/Pending Joins Tree */
  HM_AVL3_INIT_TREE(LOCAL.pending_subscriptions_tree, NULL);
  LOCAL.next_pending_tree_id = 1;

  /* Notifications Queue */
  HM_INIT_ROOT(LOCAL.notification_queue);
  LOCAL.next_notification_id = 1;

  /* Wildcard list */
  HM_INIT_ROOT(LOCAL.table_root_subscribers);

  /***************************************************************************/
  /* Initialize the connection list                       */
  /***************************************************************************/
  HM_INIT_ROOT(LOCAL.conn_list);

  /***************************************************************************/
  /* Fill in Local Location CB information                   */
  /***************************************************************************/
  LOCAL.local_location_cb.id = config_cb->instance_info.index;
  LOCAL.local_location_cb.index = config_cb->instance_info.index;
  LOCAL.local_location_cb.table_type = HM_TABLE_TYPE_LOCATION_LOCAL;

  TRACE_DETAIL(("Hardware Index: %d", LOCAL.local_location_cb.index));

  LOCAL.transport_bitmask = 0;
  LOCAL.mcast_addr = NULL;

  LOCAL.local_location_cb.node_listen_cb = NULL;
  LOCAL.local_location_cb.peer_broadcast_cb = NULL;
  LOCAL.local_location_cb.peer_listen_cb = NULL;


  LOCAL.local_location_cb.timer_cb = NULL;

  LOCAL.node_keepalive_period = config_cb->instance_info.node.timer_val;
  TRACE_INFO(("Node Keepalive Period: %d(ms)",LOCAL.node_keepalive_period));
  LOCAL.node_kickout_value = config_cb->instance_info.node.threshold;
  TRACE_INFO(("Node Keepalive Threshold: %d",LOCAL.node_kickout_value));

  LOCAL.peer_keepalive_period = config_cb->instance_info.node.timer_val;
  TRACE_INFO(("Peer Keepalive Period: %d(ms)",LOCAL.peer_keepalive_period));
  LOCAL.peer_kickout_value = config_cb->instance_info.node.threshold;
  TRACE_INFO(("Peer Keepalive Threshold: %d",LOCAL.peer_kickout_value));

  LOCAL.local_location_cb.ha_timer_wait_interval = config_cb->instance_info.ha_role.timer_val;
  TRACE_INFO(("Wait for %d ms for HA Role update", LOCAL.local_location_cb.ha_timer_wait_interval));
  /* TCP Info */
  if(config_cb->instance_info.node_addr != NULL)
  {
    TRACE_DETAIL(("Transport Information for Local Nodes is provided."));
    tprt_cb = hm_alloc_transport_cb(config_cb->instance_info.node_addr->address.type);
    if(tprt_cb == NULL)
    {
      TRACE_ERROR(("Error allocating Transport structures for nodes."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->location_cb = &LOCAL.local_location_cb;
    LOCAL.local_location_cb.node_listen_cb = tprt_cb;

    memcpy(  &LOCAL.local_location_cb.node_listen_cb->address,
        &(config_cb->instance_info.node_addr->address),
        sizeof(HM_INET_ADDRESS));
    sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.node_listen_cb->address.address;
    TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.node_listen_cb->address.type));
#ifdef I_WANT_TO_DEBUG
    {
      char tmp[100];
      TRACE_INFO(("Address IP: %s", inet_ntop(AF_INET, &sock_addr->sin_addr, tmp, sizeof(tmp))));
    }
#endif
    TRACE_DETAIL(("Port: %d", sock_addr->sin_port));
    /***************************************************************************/
    /* Set LSB to 1                                 */
    /***************************************************************************/
    TRACE_DETAIL(("Set TCP in BIT Mask"));
    LOCAL.transport_bitmask = (LOCAL.transport_bitmask | (1 << HM_TRANSPORT_TCP_LISTEN));
    TRACE_DETAIL(("0x%x", LOCAL.transport_bitmask));
  }

  tprt_cb = NULL;
  /* Cluster Info */
  if(config_cb->instance_info.cluster_addr != NULL)
  {
    TRACE_DETAIL(("Cluster Communication Information is provided."));
    tprt_cb = hm_alloc_transport_cb(config_cb->instance_info.cluster_addr->address.type);
    if(tprt_cb == NULL)
    {
      TRACE_ERROR(("Error allocating Transport structures for cluster peers."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->location_cb = &LOCAL.local_location_cb;
    LOCAL.local_location_cb.peer_listen_cb = tprt_cb;

    memcpy(  &LOCAL.local_location_cb.peer_listen_cb->address,
        &(config_cb->instance_info.cluster_addr->address),
        sizeof(HM_INET_ADDRESS));
    sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.peer_listen_cb->address.address;
    TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.peer_listen_cb->address.type));
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
  tprt_cb = NULL;

  /* Multicast Info */
  if(config_cb->instance_info.mcast != NULL)
  {
    TRACE_DETAIL(("Multicast Information is provided."));
    tprt_cb = hm_alloc_transport_cb(config_cb->instance_info.mcast->address.type);
    if(tprt_cb == NULL)
    {
      TRACE_ERROR(("Error allocating Transport structures for listening."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->location_cb = &LOCAL.local_location_cb;
    LOCAL.local_location_cb.peer_broadcast_cb = tprt_cb;

    memcpy(  &LOCAL.local_location_cb.peer_broadcast_cb->address,
        &(config_cb->instance_info.mcast->address),
        sizeof(HM_INET_ADDRESS));
    sock_addr = (SOCKADDR_IN *)&LOCAL.local_location_cb.peer_broadcast_cb->address.address;
    TRACE_DETAIL(("Address Type: %d", LOCAL.local_location_cb.peer_broadcast_cb->address.type));
#ifdef I_WANT_TO_DEBUG
    {
      char tmp[100];
      TRACE_INFO(("Address IP: %s", inet_ntop(AF_INET,
          &sock_addr->sin_addr, tmp, sizeof(tmp))));
    }
#endif
    TRACE_DETAIL(("Port: %d", sock_addr->sin_port));
    LOCAL.local_location_cb.peer_broadcast_cb->address.mcast_group =
                          config_cb->instance_info.mcast_group;
    TRACE_DETAIL(("Multicast Group: %d",
            LOCAL.local_location_cb.peer_broadcast_cb->address.mcast_group));
    TRACE_DETAIL(("Set Multicast in transport bitmask"));
    LOCAL.transport_bitmask = (LOCAL.transport_bitmask | (1 << HM_TRANSPORT_MCAST));
    TRACE_DETAIL(("0x%x", LOCAL.transport_bitmask));

    LOCAL.mcast_addr = hm_alloc_transport_cb(config_cb->instance_info.mcast->address.type);
    if(LOCAL.mcast_addr == NULL)
    {
      TRACE_ERROR(("Error allocating memory for Multicast Address."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
  }
  tprt_cb = NULL;

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
  /* Initialize Node Tree                             */
  /***************************************************************************/
  HM_AVL3_INIT_TREE(LOCAL.local_location_cb.node_tree, NULL);

  /***************************************************************************/
  /* Add the location to global locations tree                 */
  /***************************************************************************/
  if(hm_global_location_add(&LOCAL.local_location_cb, HM_STATUS_RUNNING)!= HM_OK)
  {
    TRACE_ERROR(("Error initializing local location"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  TRACE_DETAIL(("ID returned: %d", LOCAL.local_location_cb.id));

  /***************************************************************************/
  /* Initialize the Node tree and fill it up                   */
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
      /* Add node to system                             */
      /***************************************************************************/
      if(hm_node_add(node_config_cb->node_cb, &LOCAL.local_location_cb) != HM_OK)
      {
        TRACE_ERROR(("Error adding node to system."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Subscribe to its dependency list.                     */
      /***************************************************************************/
      TRACE_DETAIL(("Making subscriptions."));
      for(subs = (HM_CONFIG_SUBSCRIPTION_CB *)HM_NEXT_IN_LIST(node_config_cb->subscriptions);
        subs != NULL;
        subs = (HM_CONFIG_SUBSCRIPTION_CB *)HM_NEXT_IN_LIST(subs->node))
      {
        if(hm_subscribe(subs->subs_type, subs->value, (void *)node_config_cb->node_cb, FALSE) != HM_OK)
        {
          TRACE_ERROR(("Error creating subscriptions."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
      }
      /***************************************************************************/
      /* Try to resolve active-backup dependencies if any                        */
      /***************************************************************************/
      hm_ha_resolve_active_backup(node_config_cb->node_cb);
    }
  }

  LOCAL.local_location_cb.fsm_state = HM_PEER_FSM_STATE_ACTIVE;
  LOCAL.local_location_cb.keepalive_missed = 0;
  LOCAL.local_location_cb.keepalive_period = LOCAL.peer_keepalive_period;

  TRACE_ASSERT(global_timer_table.root != NULL);
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_init_local */

/**
 *  @brief Initializes Transport Layer
 *
 * 1. Set SIGIGN for dead sockets
 *
 *  @param None
 *  @return #HM_OK on success, #HM_ERR otherwise.
 */
int32_t hm_init_transport()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_TRANSPORT_CB *tprt_cb = NULL;

  struct sigaction action;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /* Allocate resources and pre-provision operations like binding and listen */
  /* on designated sockets.                           */
  /* Connection requests will be queued until we start polling. So, should   */
  /* not be a problem.                             */
  /***************************************************************************/
  /***************************************************************************/
  /* On writing to dead Sockets, do not panic.                               */
  /***************************************************************************/
  action.sa_handler = SIG_IGN;
  sigemptyset(&action.sa_mask);
    sigaction(SIGPIPE, &action, NULL);

  /***************************************************************************/
  /* Set the global FD set                            */
  /***************************************************************************/
  FD_ZERO(&hm_tprt_conn_set);

  /***************************************************************************/
  /* Local Nodes Connection Setup                         */
  /***************************************************************************/
  if(LOCAL.local_location_cb.node_listen_cb != NULL)
  {
    TRACE_INFO(("Start Transport for Local Nodes"));

    tprt_cb = LOCAL.local_location_cb.node_listen_cb;
    tprt_cb->sock_cb =
        hm_tprt_open_connection(tprt_cb->type,
                (void *)&LOCAL.local_location_cb.node_listen_cb->address);
    if(tprt_cb->sock_cb == NULL)
    {
      TRACE_ERROR(("Error initializing Listen socket"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->sock_cb->tprt_cb = tprt_cb;
  }
  else
  {
    TRACE_INFO(("No transport information provided for Nodal Communication."));
  }

  /***************************************************************************/
  /* Cluster Peers Connection Setup                       */
  /***************************************************************************/
  if(LOCAL.local_location_cb.peer_listen_cb != NULL)
  {
    TRACE_INFO(("Start Peer Communication Transport"));
    tprt_cb = LOCAL.local_location_cb.peer_listen_cb;

    tprt_cb->sock_cb =
        hm_tprt_open_connection(tprt_cb->type,
            (void *)&LOCAL.local_location_cb.peer_listen_cb->address);
    if(tprt_cb->sock_cb == NULL)
    {
      TRACE_ERROR(("Error initializing UDP server socket"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->sock_cb->tprt_cb = tprt_cb;
  }

  /***************************************************************************/
  /* Multicast Connection Setup                         */
  /***************************************************************************/
  if(LOCAL.local_location_cb.peer_broadcast_cb != NULL)
  {
    TRACE_INFO(("Start Multicast Service"));
    tprt_cb = LOCAL.local_location_cb.peer_broadcast_cb;

    tprt_cb->sock_cb =
        hm_tprt_open_connection(tprt_cb->type,
            (void *)&LOCAL.local_location_cb.peer_broadcast_cb->address);
    if(tprt_cb->sock_cb == NULL)
    {
      TRACE_ERROR(("Error initializing Multicast socket"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    tprt_cb->sock_cb->tprt_cb = tprt_cb;

    /***************************************************************************/
    /* Start the Cluster Timer too                         */
    /***************************************************************************/
    LOCAL.local_location_cb.timer_cb = HM_TIMER_CREATE(LOCAL.peer_keepalive_period, TRUE,
            hm_peer_keepalive_callback, (void *)&LOCAL.local_location_cb);
    HM_TIMER_START(LOCAL.local_location_cb.timer_cb);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
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


/**
 *  @brief The main select loop of this program that monitors incoming requests
 *  and/or schedules the rest of things.
 *
 *  @param None
 *  @return @c void
 */
void hm_run_main_thread()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t nready; //Number of ready descriptors

  int32_t val;
  uint32_t msg_type;
  HM_SOCKET_CB *sock_cb;
  extern fd_set hm_tprt_conn_set;
  extern int32_t max_fd;
  struct timeval select_timeout;

  fd_set read_set, write_set, except_set;

  HM_MSG *buf = NULL;

  HM_NODE_INIT_MSG *init_msg = NULL;
  HM_PEER_MSG_INIT *peer_init_msg = NULL;
  HM_PEER_MSG_KEEPALIVE *peer_tick_msg = NULL;
  HM_LOCATION_CB *loc_cb = NULL;

  HM_GLOBAL_LOCATION_CB *glob_cb = NULL;
  int32_t loc_id;

  int32_t bytes_rcvd = HM_ERR;

  SOCKADDR *udp_sender = NULL;

  HM_SUBSCRIBER node_cb;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Send Announce on Cluster marking its availability             */
  /***************************************************************************/
  hm_cluster_send_tick();

  /***************************************************************************/
  /* Start HA Role timer too                                                 */
  /***************************************************************************/

  TRACE_INFO(("Starting HA Role Wait timer."));
  LOCAL.local_location_cb.ha_timer_cb =
      HM_TIMER_CREATE(LOCAL.local_location_cb.ha_timer_wait_interval,
                      TRUE,
                      hm_ha_role_update_callback,
                      (void *)&LOCAL.local_location_cb);
  HM_TIMER_START(LOCAL.local_location_cb.ha_timer_cb);

  /***************************************************************************/
  /* Start Endless loop to process incoming data                 */
  /***************************************************************************/
  while(1)
  {
    /*start loop to check for incoming events till kingdom come!          */
    /***************************************************************************/
    /* Check if the signal has been blocked. If it has been, proceed with      */
    /* select. Else, block it and unblock at the end of while. This is done to */
    /* ensure that there are no concurrency problems arising from the          */
    /* interrupt routines changing the values of variables being accessed by   */
    /* ongoing routine/callback.                         */
    /***************************************************************************/
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 250000; /* 250ms */

    read_set = hm_tprt_conn_set;
    write_set = hm_tprt_write_set;
    except_set = hm_tprt_write_set;
    /***************************************************************************/
    /* Unblock the signal.                             */
    /***************************************************************************/
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
        TRACE_PERROR(("Error unblocking signals"));
        //goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Check for listen/read events                         */
    /***************************************************************************/
    nready = select(max_fd+1, &read_set, &write_set, &except_set, &select_timeout);
    if(nready == 0)
    {
      /***************************************************************************/
      /* No descriptors are ready                           */
      /***************************************************************************/
      continue;
    }
    else if(nready == -1)
    {
      TRACE_PERROR(("Error Occurred on select() call."));
      continue;
    }

    /***************************************************************************/
    /* Block the signal without seeing what was in previously.            */
    /***************************************************************************/
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
    TRACE_PERROR(("Error blocking signals before querying descriptors."));
    }
    /***************************************************************************/
    /* Check for incoming connections                       */
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
      /* again by breaking out and use select again.                 */
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
    /* to know that they have a request and their level of PDU.           */
    /***************************************************************************/
    if((LOCAL.local_location_cb.peer_listen_cb->type == HM_TRANSPORT_TCP_LISTEN)||
        (LOCAL.local_location_cb.peer_listen_cb->type == HM_TRANSPORT_TCP_IPv6_LISTEN))
    {
      if(FD_ISSET(LOCAL.local_location_cb.peer_listen_cb->sock_cb->sock_fd, &read_set))
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
        /* again by breaking out and use select again.                 */
        /***************************************************************************/
        if(--nready <=0)
        {
          TRACE_DETAIL(("No more incoming requests."));
          continue;
        }
      }//end select on listenfd
    }
    /***************************************************************************/
    /* Check on Multicast socket too                       */
    /***************************************************************************/
    TRACE_DETAIL(("Check Multicast port %d!",
            LOCAL.local_location_cb.peer_broadcast_cb->sock_cb->sock_fd));
    if(LOCAL.local_location_cb.peer_broadcast_cb->sock_cb->sock_fd != -1)
    {
      TRACE_DETAIL(("Check Multicast port!"));
      if(FD_ISSET(LOCAL.local_location_cb.peer_broadcast_cb->sock_cb->sock_fd, &read_set))
      {
        /***************************************************************************/
        /* Accept the connection only if we have not exceeded top-load benchmark   */
        /***************************************************************************/
        TRACE_INFO(("Cluster Muticast Port has requests."));
        /***************************************************************************/
        /* We'll need to receive the message into a random buffer which must be as */
        /* large as at least the INIT message of peers.                */
        /***************************************************************************/
        buf = hm_get_buffer( sizeof(HM_PEER_MSG_KEEPALIVE));
        TRACE_ASSERT(buf != NULL);
        if(buf == NULL)
        {
          TRACE_ERROR(("Error allocating buffer for incoming message."));
          goto EXIT_LABEL;
        }
        /***************************************************************************/
        /* First, receive the message header and verify that it is an INIT message */
        /* Since we don't know yet if it is a Nodal Socket or Peer socket as no    */
        /* association exists yet, we'll allocate the buffer which ever is larger  */
        /* and try to receive data.                           */
        /* It HAS to be an INIT request message for such connections or we can      */
        /* discard it.                                 */
        /***************************************************************************/
        if((bytes_rcvd = hm_tprt_recv_on_socket(
                LOCAL.local_location_cb.peer_broadcast_cb->sock_cb->sock_fd,
                LOCAL.local_location_cb.peer_broadcast_cb->sock_cb->sock_type,
                      buf->msg,
                      sizeof(HM_PEER_MSG_KEEPALIVE),
                      &(udp_sender)
                      ))== HM_ERR)
        {
          TRACE_ERROR(("Error receiving message on socket."));
          TRACE_ASSERT(FALSE);
          //It is an error, but we'll still run (in release mode)
          //FIXME
          break;
        }
        TRACE_ASSERT(udp_sender != NULL);
        if(bytes_rcvd == sizeof(HM_PEER_MSG_KEEPALIVE))
        {
          peer_tick_msg = (HM_PEER_MSG_KEEPALIVE *)buf->msg;
          HM_GET_LONG(bytes_rcvd, peer_tick_msg->hdr.msg_type);
          if( bytes_rcvd != HM_PEER_MSG_TYPE_KEEPALIVE)
          {
            TRACE_WARN(("Message type is not Keepalive request. Ignore."));
            hm_free_buffer(buf);
          }
          else
          {
            /***************************************************************************/
            /* It is an INIT request. Read the rest of the message and check if we know*/
            /* about this location already.                         */
            /***************************************************************************/
            TRACE_INFO(("Keepalive message received. Check location awareness"));
#ifdef I_WANT_TO_DEBUG
            {
              char ip_addr[128];
              int32_t hw_id;
              int32_t length = 128;
              TRACE_ASSERT(udp_sender != NULL);
              inet_ntop(AF_INET, &((SOCKADDR_IN *)udp_sender)->sin_addr, ip_addr, length);
              TRACE_DETAIL(("%s:%d", ip_addr, ntohs(((SOCKADDR_IN *)udp_sender)->sin_port)));
              HM_GET_LONG(hw_id, peer_tick_msg->hdr.hw_id);
              TRACE_DETAIL(("HW ID: %d", hw_id));
            }
#endif
            hm_cluster_check_location(  buf,
                          udp_sender);
            hm_free_buffer(buf);
            //TODO
          }
        }

        /***************************************************************************/
        /* If select returned 1, and it was a listen socket, it makes sense to poll*/
        /* again by breaking out and use select again.                 */
        /***************************************************************************/
        if(--nready <=0)
        {
          TRACE_DETAIL(("No more incoming requests."));
          continue;
        }
      }//end select on listenfd
    }
    /***************************************************************************/
    /* Check for Exception events on any socket                     */
    /***************************************************************************/
    for(sock_cb = (HM_SOCKET_CB *)HM_NEXT_IN_LIST(LOCAL.conn_list);
        sock_cb != NULL;
        sock_cb = HM_NEXT_IN_LIST(sock_cb->node))
    {
      if(FD_ISSET(sock_cb->sock_fd, &except_set))
      {
        /***************************************************************************/
        /* The connection is valid and read event has occurred here.           */
        /***************************************************************************/
        TRACE_DETAIL(("FD %d has a exception event",sock_cb->sock_fd));

        /***************************************************************************/
        /* Connection connect() request must have failed.                  */
        /* Check if its state was INIT.                         */
        /***************************************************************************/
        if(sock_cb->conn_state == HM_TPRT_CONN_INIT)
        {
          TRACE_DETAIL(("Connect failed!"));
          TRACE_ASSERT(sock_cb->tprt_cb->location_cb != NULL);
          loc_cb = sock_cb->tprt_cb->location_cb;
          /***************************************************************************/
          /* Retry request                               */
          /***************************************************************************/
          if((val = connect(sock_cb->sock_fd, &sock_cb->addr, sizeof(SOCKADDR_IN)))!=0)
          {
            if(errno == EINPROGRESS)
            {
              TRACE_DETAIL(("Connect will complete asynchronously."));
            }
            else
            {
              TRACE_PERROR(("Connect failed on socket %d", sock_cb->sock_fd));
              FD_CLR(sock_cb->sock_fd, &hm_tprt_write_set);
              close(sock_cb->sock_fd);
              sock_cb->sock_fd = -1;
              goto EXIT_LABEL;
            }
          }
          else
          {
            TRACE_DETAIL(("Connect succeeded!"));
            /* Directly move connection to active state */
            sock_cb->conn_state = HM_TPRT_CONN_ACTIVE;
          }
        }
        if(--nready <=0)
        {
          TRACE_DETAIL(("No more incoming requests."));
          /*
           * FIXME: Possible breakpoint. If a socket has both read and write fd set
           * is it counted in twice?
           */
          continue;
        }
      }
    }
    /***************************************************************************/
    /* Check for WRITE events on any socket                     */
    /***************************************************************************/
    for(sock_cb = (HM_SOCKET_CB *)HM_NEXT_IN_LIST(LOCAL.conn_list);
        sock_cb != NULL;
        sock_cb = HM_NEXT_IN_LIST(sock_cb->node))
    {
      if(FD_ISSET(sock_cb->sock_fd, &write_set))
      {
        /***************************************************************************/
        /* The connection is valid and read event has occurred here.           */
        /***************************************************************************/
        TRACE_DETAIL(("FD %d has a write event",sock_cb->sock_fd));

        /***************************************************************************/
        /* Connection connect() request must have completed successfully.       */
        /* Check if its state was INIT.                         */
        /***************************************************************************/
        if(sock_cb->conn_state == HM_TPRT_CONN_INIT)
        {
          TRACE_DETAIL(("Connect succeeded!"));
          TRACE_ASSERT(sock_cb->tprt_cb->location_cb != NULL);
          loc_cb = sock_cb->tprt_cb->location_cb;
          /***************************************************************************/
          /* Send an INIT request to the Peer                      */
          /***************************************************************************/
          hm_cluster_send_init(sock_cb->tprt_cb);
          /***************************************************************************/
          /* Remove it from global FD write set now.                   */
          /***************************************************************************/

          FD_CLR(sock_cb->sock_fd, &hm_tprt_write_set);
        }
        if(--nready <=0)
        {
          TRACE_DETAIL(("No more incoming requests."));
          /*
           * FIXME: Possible breakpoint. If a socket has both read and write fd set
           * is it counted in twice?
           */
          continue;
        }
      }
    }

    /***************************************************************************/
    /* There are READ events waiting to be processed.                  */
    /***************************************************************************/
    for(sock_cb = (HM_SOCKET_CB *)HM_NEXT_IN_LIST(LOCAL.conn_list);
        sock_cb != NULL;
        sock_cb = HM_NEXT_IN_LIST(sock_cb->node))
    {
      if(FD_ISSET(sock_cb->sock_fd, &read_set))
      {
        /***************************************************************************/
        /* The connection is valid and read event has occured here.             */
        /***************************************************************************/
        TRACE_DETAIL(("FD %d has data",sock_cb->sock_fd));

        /***************************************************************************/
        /* If its parent transport CB has been set, then we can pass the message   */
        /* notification to the upper layer. If not, then we are expecting an INIT  */
        /* message.                                   */
        /***************************************************************************/
        if(sock_cb->tprt_cb == NULL)
        {
          TRACE_DETAIL(("Socket is currently not associated with any Node or Peer."));

          /***************************************************************************/
          /* We'll need to receive the message into a random buffer which must be as */
          /* large as at least the INIT message of peers. This is because we are      */
          /* currently not concerned if the Socket is TCP Type or UDP, so we're      */
          /* preparing for a UDP socket too (provisioning a full buffer read because */
          /* partial reading is not allowed on UDP).                   */
          /* Even if it is TCP Socket, that is not a problem.               */
          /***************************************************************************/
          buf = hm_get_buffer( MAX(sizeof(HM_NODE_INIT_MSG),sizeof(HM_PEER_MSG_INIT)) );
          TRACE_ASSERT(buf != NULL);
          if(buf == NULL)
          {
            TRACE_ERROR(("Error allocating buffer for incoming message."));
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* First, receive the message header and verify that it is an INIT message */
          /* Since we don't know yet if it is a Nodal Socket or Peer socket as no    */
          /* association exists yet, we'll allocate the buffer which ever is larger  */
          /* and try to receive data.                           */
          /* It HAS to be an INIT request message for such connections or we can      */
          /* discard it.                                 */
          /* Update:                                    */
          /* We will have to try and receive Full INIT message on the socket because */
          /* the INIT message of Nodes is bigger, and since both are TCP sockets     */
          /* we cannot (we can but don't want to) make a distinction between a Node  */
          /* socket and Location Socket.                         */
          /***************************************************************************/
          if((bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                        sock_cb->sock_type,
                        buf->msg,
                        MAX(sizeof(HM_NODE_INIT_MSG),
                          sizeof(HM_PEER_MSG_INIT)),
                        &udp_sender
                        ))== HM_ERR)
          {
            TRACE_ERROR(("Error receiving message on socket."));
            TRACE_ASSERT(FALSE);
            //It is an error, but we'll still run (in release mode)
            //FIXME
            break;
          }
          /***************************************************************************/
          /* Depending on the number of bytes read, we can infer if it is a Peer INIT*/
          /* Or a Node INIT                               */
          /***************************************************************************/
          //FIXME: There is a chance that size of HM_MSG_HDR and that of Peer Messages is same.
          //Consider differentiating recv on basis of sock type
          if(bytes_rcvd == sizeof(HM_NODE_INIT_MSG))
          {
            init_msg = (HM_NODE_INIT_MSG *)buf->msg;
            if((init_msg->hdr.msg_type != HM_MSG_TYPE_INIT) || (init_msg->hdr.request != TRUE))
            {
              TRACE_WARN(("Message type is not INIT request. Ignore."));
              /***************************************************************************/
              /* Receive the rest of the message and discard the buffer.           */
              /* This can be problematic because the size of the buffer is fixed, but    */
              /* the message that arrived on socket might be larger, due to which, the   */
              /* socket descriptor will be set as long as there is data on that buffer.  */
              /***************************************************************************/
              if((bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                  sock_cb->sock_type,
                                  (BYTE *)buf->msg + bytes_rcvd,
                                  sizeof(HM_NODE_INIT_MSG)
                                  - sizeof(HM_MSG_HEADER),
                                  &udp_sender
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
              init_msg = (HM_NODE_INIT_MSG *)buf->msg;
              TRACE_INFO(("INIT Request from Node Index: %d, Group %d",
                          init_msg->index, init_msg->service_group_index));
              /***************************************************************************/
              /* Find a node with this index and group in current table           */
              /* NOTE: Since indexes are currently guaranteed to be unique, the lookup is*/
              /* based on node index only, and group number checking is then done here   */
              /* itself.                                   */
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
                /* Continue receiving on other sockets if any                 */
                /***************************************************************************/
                continue;
              }
              if(node_cb.proper_node_cb->group != init_msg->service_group_index)
              {
                TRACE_ERROR(("The Group index %d of node in system does not match with reported",
                    node_cb.proper_node_cb->group));
                hm_free_buffer(buf);
                /***************************************************************************/
                /* Continue receiving on other sockets if any                 */
                /***************************************************************************/
                continue;
              }
              TRACE_ASSERT(node_cb.proper_node_cb->transport_cb != NULL);
              /***************************************************************************/
              /* Put message as the input buffer of the transport.             */
              /***************************************************************************/
              node_cb.proper_node_cb->transport_cb->in_buffer = (char *)buf;

              /***************************************************************************/
              /* Fix pointers                                 */
              /***************************************************************************/
              node_cb.proper_node_cb->transport_cb->sock_cb = sock_cb;
              sock_cb->tprt_cb = node_cb.proper_node_cb->transport_cb;

              /***************************************************************************/
              /* Call into Node FSM signifying an INIT receive message.                  */
              /***************************************************************************/
              hm_node_fsm(HM_NODE_FSM_INIT, node_cb.proper_node_cb);

              /***************************************************************************/
              /* We no longer need this buffer. If someone else is using it, the ref_cnt */
              /* would have increased and the other consumer can still use it.       */
              /***************************************************************************/
              hm_free_buffer(buf);
              //TODO
            }
          }
          else if(bytes_rcvd == sizeof(HM_PEER_MSG_INIT))
          {
            buf = hm_shrink_buffer(buf, sizeof(HM_PEER_MSG_INIT));
            if(buf == NULL)
            {
              TRACE_ERROR(("Error shrinking buffer!"));
              TRACE_ASSERT(FALSE);
            }
            peer_init_msg = (HM_PEER_MSG_INIT *)buf->msg;
            /***************************************************************************/
            /* Necessary to initialize msg_type to 0.                   */
            /***************************************************************************/
            msg_type = 0;
            /***************************************************************************/
            /* FIXME Keepalive message is still referenced.                 */
            /***************************************************************************/
            HM_GET_LONG(msg_type, peer_init_msg->hdr.msg_type);

            if( msg_type != HM_PEER_MSG_TYPE_INIT)
            {
              TRACE_WARN(("Message type %d is not INIT request. Ignore.", msg_type));
              hm_free_buffer(buf);
            }
            else
            {
              /***************************************************************************/
              /* It is an INIT request. Read the rest of the message and associate with  */
              /* a location.                                 */
              /***************************************************************************/
              TRACE_INFO(("INIT request received. Associate with a Location"));
              {
                int32_t location_id;
                HM_GET_LONG(location_id, peer_init_msg->hdr.hw_id);
                TRACE_INFO(("INIT Request from Peer, indexed: %d",
                                    location_id));
              }
              /***************************************************************************/
              /* Try to find the location in tree first. If found, this is error       */
              /* TODO: Move it into hm_cluster_check_location()               */
              /***************************************************************************/
              HM_GET_LONG(loc_id, peer_init_msg->hdr.hw_id);
              glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIND(LOCAL.locations_tree,
                                        &loc_id,
                                        locations_tree_by_db_id );
              if(glob_cb != NULL)
              {
                TRACE_ERROR(("Received INIT on TCP Port of a known location"));
                hm_peer_fsm(HM_PEER_FSM_INIT_RCVD, glob_cb->loc_cb);
              }
              else
              {
                /***************************************************************************/
                /* Create a Location CB and fill in the details.               */
                /***************************************************************************/
                loc_cb = hm_alloc_location_cb();
                loc_cb->index =loc_id;
                if(sock_cb->sock_type == HM_TRANSPORT_SOCK_TYPE_TCP)
                {
                  loc_cb->peer_listen_cb = hm_alloc_transport_cb(HM_TRANSPORT_TCP_IN);
                }
                else
                {
                  loc_cb->peer_listen_cb = hm_alloc_transport_cb(HM_TRANSPORT_UDP);
                }
                /***************************************************************************/
                /* Fix pointers                                 */
                /***************************************************************************/
                loc_cb->peer_listen_cb->location_cb = loc_cb;
                loc_cb->peer_listen_cb->sock_cb = sock_cb;
                sock_cb->tprt_cb = loc_cb->peer_listen_cb;

                /***************************************************************************/
                /* Send a response of INIT first, and then commence Replay           */
                /***************************************************************************/
                HM_PUT_LONG(peer_init_msg->request, FALSE );
                HM_PUT_LONG(peer_init_msg->response_ok, TRUE);

                if(hm_queue_on_transport(buf, sock_cb->tprt_cb, TRUE)!= HM_OK)
                {
                  TRACE_ERROR(("Error sending INIT response!"));
                  TRACE_ASSERT((FALSE));
                  //TODO?
                }
                /***************************************************************************/
                /* Add the location to global locations tree                 */
                /***************************************************************************/
                if(hm_peer_fsm(HM_PEER_FSM_INIT_RCVD,loc_cb)!= HM_OK)
                {
                  TRACE_ERROR(("Error initializing location %d", loc_cb->index));
                  //TODO: What to do now?
                }
              }

              //TODO
              hm_free_buffer(buf);
            }
          }
        }
        else
        {
          TRACE_DETAIL(("Socket associated with Transport of Location %d",
                          sock_cb->tprt_cb->location_cb->index));
          if(hm_route_incoming_message(sock_cb)!= HM_OK)
          {
            TRACE_ERROR(("Some error occured while processing message!"));
            TRACE_ASSERT(FALSE);
            break;
            //FIXME: What now?
          }
          /* Don't need to free the buffer, because it is a statically allocated memory */
        }

        /***************************************************************************/
        /* Read the incoming message. This FD is now processed.              */
        /***************************************************************************/
        if(--nready <=0)
        {
          TRACE_DETAIL(("No more incoming requests."));
          break;  //No more descriptors to process. Break out and poll again.
        }// end if --nready <=0
      }// end if FD_ISSET(sockfd, &rset)
    }//end for

  }//end while(1)
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return;
}/* hm_run_main_thread */


/**
 *  @brief Initializes the Hardware Location Layer
 *
 *  @param None
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_init_location_layer()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Initialize Local Location CB                         */
  /***************************************************************************/
  /* Initialize FD_SET */

  /***************************************************************************/
  /* Initialize Transport CB for Listen                     */
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
  /* Initialize Transport CB for Unicast                     */
  /***************************************************************************/
  /*
   * Initialize TCB for Unicast Port
        * Initialize Socket Connection CB:
            * Open UDP Unicast port
    */

  /***************************************************************************/
  /* Initialize Cluster Specific Function                     */
  /***************************************************************************/
  /*
    * If Cluster enabled:
        * Initialize TCB for Multicast Port
            * Open UDP Multicast port
            * Join Multicast Group
     */

  //hm_init_location_cb(HM_LOCATION_CB &LOCAL.local_location_cb)

  /***************************************************************************/
  /* For each node in Configuration, initialize Node CB and Fill it up     */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_init_location_layer */


/**PROC+**********************************************************************/
/* Name:     hm_interrupt_handler                                            */
/*                                                                           */
/* Purpose:  Invoked when Ctrl+C or any event that triggers SIGINT happens.  */
/*       Closes the system properly.                                         */
/*                                                                           */
/* Returns:   VOID  :                                                        */
/*                                                                           */
/*                                                                           */
/* Params:    IN                                                             */
/*            IN/OUT                                                         */
/*                                                                           */
/* Operation:                                                                */
/*                                                                           */
/**PROC-**********************************************************************/
/**
 *  @brief Invoked when Ctrl+C or any event that triggers SIGINT happens. Closes the system properly.
 *
 *  @param sig Signal received from Kernel
 *  @param *info @siginfo_t structure passed along
 *  @param *data Additional data associated
 *
 *  @return @c void
 */
void hm_interrupt_handler(int32_t sig, siginfo_t *info, void *data)
{
  TRACE_ENTRY();
  UNUSED(data);

  if ( info == NULL )
  {
    TRACE_INFO(("Signal Received: [%d]",sig));
    TRACE_INFO(("Shutting Down"));
  }
  hm_terminate();
  /***************************************************************************/
  /* We're not getting here.                           */
  /***************************************************************************/
  TRACE_EXIT();
  return;
} /* hm_interrupt_handler */

/**
 *  @brief Calls into each of the sub-modules terminate functions, if any,
 *  and later, exits from the code.
 *
 *  @param None
 *  @return @c void
 */
void hm_terminate()
{
  TRACE_ENTRY();

  /***************************************************************************/
  /* Close the socket connections.                       */
  /***************************************************************************/
  //hm_tprt_terminate();
  /***************************************************************************/
  /* Do something about the config data also. Free all memory!               */
  /***************************************************************************/
  //TODO
  TRACE_EXIT();
  exit(0);
} /* hm_terminate */
