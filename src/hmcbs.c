/** @file hmcbs.c
 *
 *  @brief Hardware Manager Control Block Allocation and Deallocation routines
 *
 *  @date 07-May-2015
 *  @author Anshul
 *  @bug None detected so far.
 */

#include <hmincl.h>

/**
 *  @brief Allocates and initializes configuration control blocks for HM to defaults
 *
 *  @param None
 *  @return Pointer to Control block of type #HM_CONFIG_CB
 */
HM_CONFIG_CB * hm_alloc_config_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_CONFIG_CB *config_cb = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Allocate Config Structure                         */
  /***************************************************************************/
  config_cb = (HM_CONFIG_CB *)malloc(sizeof(HM_CONFIG_CB));
  if(config_cb == NULL)
  {
    TRACE_PERROR(("Error allocating User Configuration CB"));
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* Load default configuration                            */
  /***************************************************************************/
  config_cb->instance_info.index = 0;
  /***************************************************************************/
  /* Initialize Addresses List for HM Instance                 */
  /***************************************************************************/
  HM_INIT_ROOT(config_cb->instance_info.addresses);
  /***************************************************************************/
  /* Initialize Node List                             */
  /***************************************************************************/
  HM_INIT_ROOT(config_cb->node_list);
  /***************************************************************************/
  /* Set defaults for Node and Cluster Hearbeats                 */
  /***************************************************************************/
  config_cb->instance_info.node.scope = HM_CONFIG_SCOPE_NODE;
  config_cb->instance_info.node.threshold = HM_CONFIG_DEFAULT_NODE_KICKOUT;
  config_cb->instance_info.node.timer_val = HM_CONFIG_DEFAULT_NODE_TICK_TIME;

  config_cb->instance_info.cluster.scope = HM_CONFIG_SCOPE_CLUSTER;
  config_cb->instance_info.cluster.threshold = HM_CONFIG_DEFAULT_PEER_KICKOUT;
  config_cb->instance_info.cluster.timer_val = HM_CONFIG_DEFAULT_PEER_TICK_TIME;

  /***************************************************************************/
  /* Initialize Transport Defauls                         */
  /***************************************************************************/
  config_cb->instance_info.mcast_group = HM_DEFAULT_MCAST_GROUP;


EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(config_cb);
}/* hm_alloc_config_cb */


/**
 *  @brief Allocates a Transport Connections Control Block
 *
 *  @param conn_type Connection Type: Whether it is a Listen/TCP/UDP type connection
 *  @return Pointer to control block allocated of type #HM_TRANSPORT_CB , or NULL
 */
HM_TRANSPORT_CB * hm_alloc_transport_cb(uint32_t conn_type)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_TRANSPORT_CB *transport_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  transport_cb = (HM_TRANSPORT_CB *)malloc(sizeof(HM_TRANSPORT_CB));
  if(transport_cb == NULL)
  {
    TRACE_ERROR(("Error allocating memory for transport CB"));
    goto EXIT_LABEL;
  }

  /* Got TCB */
  memset(transport_cb, 0, sizeof(HM_TRANSPORT_CB));

  transport_cb->type = conn_type;
  transport_cb->address.type = conn_type;

  transport_cb->location_cb = NULL;
  transport_cb->node_cb = NULL;
  transport_cb->sock_cb = NULL;

  transport_cb->in_buffer = NULL;
  transport_cb->out_buffer = NULL;
  transport_cb->hold = FALSE;

  HM_INIT_ROOT(transport_cb->pending);

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (transport_cb);
}/* hm_alloc_transport_cb */


/**
 *  @brief Frees the Transport Connection Control Block
 *
 * If the transport connection CB is being used by a location as well as
 * node, and a node requests freeing the resources, do not free them.
 * Only the location may free the transport CB
 * This must be ensured by the calling function.
 *
 *  @param tprt_cb Pointer to transport control block (#HM_TRANSPORT_CB)
 *  @return HM_OK for success, otherwise HM_ERR
 */
int32_t hm_free_transport_cb(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(tprt_cb !=NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  //TODO: Check if the connections are closed.

  free(tprt_cb);
  tprt_cb = NULL;
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_free_transport_cb */


/**
 *  @brief Allocates a socket control block #HM_SOCKET_CB and initializes to defaults
 *
 *  @param None
 *  @return Pointer to CB of type #HM_SOCKET_CB
 */
HM_SOCKET_CB * hm_alloc_sock_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_SOCKET_CB *sock_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  sock_cb = (HM_SOCKET_CB *)malloc(sizeof(HM_SOCKET_CB));
  if(sock_cb == NULL)
  {
    TRACE_ERROR(("Error allocating memory for socket control block"));
    goto EXIT_LABEL;
  }
  memset(sock_cb, 0, sizeof(HM_SOCKET_CB));

  sock_cb->conn_state = HM_TPRT_CONN_NULL;
  sock_cb->tprt_cb = NULL;
  sock_cb->sock_fd = -1;

  HM_INIT_LQE(sock_cb->node, sock_cb);

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return sock_cb;
}/* hm_alloc_sock_cb */


/**
 *  @brief Frees the socket connection CB
 *
 *  @param sock_cb Socket Control Block to be freed (type #HM_SOCKET_CB)
 *  @return void
 */
void hm_free_sock_cb(HM_SOCKET_CB *sock_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(sock_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if(HM_IN_LIST(sock_cb->node))
  {
    HM_REMOVE_FROM_LIST(sock_cb->node);
  }
  if(sock_cb->sock_fd != -1)
  {
    TRACE_WARN(("Socket was not closed."));
    if(FD_ISSET(sock_cb->sock_fd, &hm_tprt_conn_set))
    {
      TRACE_DETAIL(("Remove from Read set."));
      FD_CLR(sock_cb->sock_fd, &hm_tprt_conn_set);
    }
    if(FD_ISSET(sock_cb->sock_fd, &hm_tprt_write_set))
    {
      TRACE_DETAIL(("Remove from write set."));
      FD_CLR(sock_cb->sock_fd, &hm_tprt_write_set);
    }
    close(sock_cb->sock_fd);
  }

  free(sock_cb);
  sock_cb = NULL;
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
}/* hm_free_sock_cb */

/**
 *  @brief Allocates a location control block of type #HM_LOCATION_CB
 *
 *  @param none
 *  @return pointer to CB of type #HM_LOCATION_CB
 */
HM_LOCATION_CB * hm_alloc_location_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_LOCATION_CB *loc_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  loc_cb = (HM_LOCATION_CB *)malloc(sizeof(HM_LOCATION_CB));
  if(loc_cb == NULL)
  {
    TRACE_ERROR(("Error allocating memory for Location CB"));
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* Got Memory. Initialize defaults                       */
  /***************************************************************************/
  loc_cb->id = 0;
  loc_cb->table_type = HM_TABLE_TYPE_LOCATION_LOCAL;
  loc_cb->db_ptr = NULL;

  loc_cb->index = 0;
  HM_AVL3_INIT_TREE(loc_cb->node_tree, NULL);
  loc_cb->fsm_state = HM_PEER_FSM_STATE_NULL; /* Since it is being added. */

  loc_cb->timer_cb = HM_TIMER_CREATE(LOCAL.peer_keepalive_period, TRUE,
        hm_peer_keepalive_callback, (void *)loc_cb );
  if(loc_cb->timer_cb == NULL)
  {
    TRACE_ERROR(("Error creating timer for node"));
    free(loc_cb);
    loc_cb = NULL;
    goto EXIT_LABEL;
  }
  loc_cb->keepalive_missed = 0;
  loc_cb->keepalive_period = LOCAL.peer_keepalive_period;
  loc_cb->peer_listen_cb = NULL;

  loc_cb->active_nodes = 0;
  loc_cb->total_nodes = 0;
  loc_cb->active_processes = 0;
  loc_cb->replay_in_progress = FALSE;
  /***************************************************************************/
  /* The following fields are necessary only for a local node. Maybe I would */
  /* remove them later.                             */
  /***************************************************************************/
  loc_cb->node_listen_cb = NULL;
  loc_cb->peer_broadcast_cb = NULL;
  HM_INIT_ROOT(loc_cb->peer_list);
  HM_INIT_ROOT(loc_cb->transport_cb_list);



EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(loc_cb);
}/* hm_alloc_location_cb */


/**
 *  @brief Frees the resources of a location control block
 *
 *  @param loc_cb Location Control Block (#HM_LOCATION_CB)
 *  @return HM_OK on success and HM_ERR on failure
 */
int32_t hm_free_location_cb(HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Free Nodes tree if not free.                         */
  /***************************************************************************/

  /***************************************************************************/
  /* Free memory                                 */
  /***************************************************************************/
  free(loc_cb);
  loc_cb = NULL;

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();

  return(ret_val);
}/* hm_free_location_cb */


/**
 *  @brief Allocates a Node Control Block structure and initializes the default values.
 *
 *  @param local Flag whether the node is Local or remote (True/False)
 *  @return Control Block to allocated Node #HM_NODE_CB
 */
HM_NODE_CB * hm_alloc_node_cb(uint32_t local)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_NODE_CB *node_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  node_cb = (HM_NODE_CB *)malloc(sizeof(HM_NODE_CB));
  if(node_cb == NULL)
  {
    TRACE_ERROR(("Error allocating resources for Node Control Block"));
    node_cb = NULL;
    goto EXIT_LABEL;
  }
  node_cb->id = 0;
  node_cb->db_ptr = NULL;
  node_cb->table_type = HM_TABLE_TYPE_NODES_LOCAL;

  node_cb->index = 0;
  HM_AVL3_INIT_NODE(node_cb->index_node, node_cb);

  node_cb->group = 0;
  node_cb->role = NODE_ROLE_PASSIVE;
  node_cb->current_role = NODE_ROLE_NONE;
  memset(&node_cb->name, '\0', sizeof(node_cb->name));
  node_cb->transport_cb = NULL;
  node_cb->parent_location_cb = NULL;

  /* We don't need to specify the TREE_INFO right now*/
  HM_AVL3_INIT_TREE(node_cb->process_tree, NULL);
  HM_AVL3_INIT_TREE(node_cb->interface_tree, NULL);

  node_cb->partner = NULL;

  node_cb->fsm_state = HM_NODE_FSM_STATE_NULL;

  node_cb->keepalive_period = HM_CONFIG_DEFAULT_NODE_TICK_TIME;
  node_cb->timer_cb = NULL;
  node_cb->keepalive_missed = 0;
  /***************************************************************************/
  /* Create a Timer                               */
  /***************************************************************************/
  if(local == TRUE)
  {
    node_cb->timer_cb = HM_TIMER_CREATE(node_cb->keepalive_period, TRUE,
          hm_node_keepalive_callback, (void *)node_cb );
    if(node_cb->timer_cb == NULL)
    {
      TRACE_ERROR(("Error creating timer for node"));
      free(node_cb);
      node_cb = NULL;
      goto EXIT_LABEL;
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (node_cb);
}/* hm_alloc_node_cb */


/**
 *  @brief Free the Node Control Block and its associated resources
 *
 *  @param node_cb Node Control Block #HM_NODE_CB
 *  @return HM_OK on success, HM_ERR on failure
 */
int32_t hm_free_node_cb(HM_NODE_CB *node_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_PROCESS_CB *tree_node = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(node_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Close connections to the node if any                     */
  /***************************************************************************/
  /* FIXME: Is not required for Nodes, as Location CBs will clean it up      */

  /***************************************************************************/
  /* Empty the process tree and destroy                     */
  /* Since we are removing each process from the tree, we need not traverse  */
  /* the entire tree, but only its first. (It autobalances)             */
  /***************************************************************************/
  for(tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(
              node_cb->process_tree, node_process_tree_by_proc_id);
      tree_node != NULL;
    tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->process_tree,
                          node_process_tree_by_proc_id))
  {
    if((ret_val = hm_free_process_cb(tree_node)) != HM_OK)
    {
      TRACE_ERROR(("Error freeing Process CB"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
  }
  /***************************************************************************/
  /* Empty the interface tree and destroy                     */
  /***************************************************************************/
  for(tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->interface_tree, node_process_tree_by_proc_id);
      tree_node != NULL;
    tree_node = (HM_PROCESS_CB *) HM_AVL3_FIRST(node_cb->interface_tree, node_process_tree_by_proc_id))
  {
    if((ret_val = hm_free_process_cb(tree_node)) != HM_OK)
    {
      TRACE_ERROR(("Error freeing Process CB"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
  }

  /***************************************************************************/
  /* Close any node keepalive timers                       */
  /***************************************************************************/
  if(node_cb->timer_cb != NULL)
  {
    HM_TIMER_DELETE(node_cb->timer_cb);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_free_node_cb */


/**
 *  @brief Allocates a Process Control Block
 *
 *  @param None
 *  @return Control Block of allocated CB of type #HM_PROCESS_CB
 */
HM_PROCESS_CB * hm_alloc_process_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_PROCESS_CB *proc_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  proc_cb = (HM_PROCESS_CB *)malloc(sizeof(HM_PROCESS_CB));
  if(proc_cb == NULL)
  {
    TRACE_ERROR(("Error allocating memory for Process Control Block"));
    goto EXIT_LABEL;
  }
  proc_cb->db_ptr = NULL;
  proc_cb->id = 0;
  proc_cb->table_type = HM_TABLE_TYPE_PROCESS_LOCAL;

  proc_cb->type = 0;
  proc_cb->pid = 0;
  memset(proc_cb->name, '\0', sizeof(proc_cb->name));
  proc_cb->parent_node_cb = NULL;
  HM_AVL3_INIT_NODE(proc_cb->node, proc_cb);
  HM_INIT_ROOT(proc_cb->interfaces_list);
  proc_cb->running = FALSE;

  proc_cb->role = NODE_ROLE_PASSIVE;
  proc_cb->partner = NULL;

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(proc_cb);
}/* hm_alloc_process_cb */


/**
 *  @brief Frees the Process Control Block for a process
 *
 *  @param proc_cb Process CB (#HM_PROCESS_CB) to be freed
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_free_process_cb(HM_PROCESS_CB *proc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;

  HM_INTERFACE_CB *list_node = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(proc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Empty the interfaces list                         */
  /***************************************************************************/
  if(!HM_EMPTY_LIST(proc_cb->interfaces_list))
  {
    TRACE_DETAIL(("Emptying Interface list"));
    for(list_node = (HM_INTERFACE_CB *)HM_NEXT_IN_LIST(proc_cb->interfaces_list);
        list_node != NULL;
        list_node = (HM_INTERFACE_CB *)HM_NEXT_IN_LIST(proc_cb->interfaces_list))
    {
      HM_REMOVE_FROM_LIST(list_node->list_node);
      free(list_node);
    }
  }

  /***************************************************************************/
  /* Remove from Nodes tree                           */
  /***************************************************************************/
  if(proc_cb->node.parent != NULL)
  {
    TRACE_DETAIL(("Removing from Tree"));
    HM_AVL3_DELETE(proc_cb->parent_node_cb->interface_tree, proc_cb->node);
  }

  free(proc_cb);
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/

  TRACE_EXIT();
  return (ret_val);
}/* hm_free_process_cb */


/**
 *  @brief Allocates a subscription CB
 *
 *  @param None
 *  @return Pointer to allocated CB of type #HM_SUBSCRIPTION_CB
 */
HM_SUBSCRIPTION_CB * hm_alloc_subscription_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_SUBSCRIPTION_CB *subscription = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  subscription = (HM_SUBSCRIPTION_CB *)malloc(sizeof(HM_SUBSCRIPTION_CB));
   if(subscription== NULL)
   {
     TRACE_ERROR(("Error creating subscription point."));
     goto EXIT_LABEL;
   }
   /***************************************************************************/
   /* Since this variable is accessed only internally, we can increment it    */
   /* as only a single thread will always access it.              */
   /* Likewise, it may be decremented on failure.                */
   /* Subscription points may not be removed, (currently) ever.        */
   /***************************************************************************/
   subscription->id = 0;
   subscription->row_cb.void_cb = NULL;
   subscription->row_id = 0;
   HM_INIT_ROOT(subscription->subscribers_list);
   HM_AVL3_INIT_NODE(subscription->node, subscription);
   subscription->table_type = 0;
   subscription->value = 0;
   subscription->live = FALSE;
   subscription->num_subscribers = 0;

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return subscription;
}/* hm_alloc_subscription_cb */


/**
 *  @brief Frees a subscription CB
 *
 *  @param sub_cb Subscription to be freed (#HM_SUBSCRIPTION_CB)
 *  @return void
 */
void hm_free_subscription_cb(HM_SUBSCRIPTION_CB *sub_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(sub_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if(HM_AVL3_IN_TREE(sub_cb->node))
  {
    if(sub_cb->live)
    {
      TRACE_DETAIL(("Delete from active subscriptions tree"));
      HM_AVL3_DELETE(LOCAL.active_subscriptions_tree, sub_cb->node);
    }
    else
    {
      TRACE_DETAIL(("Delete from pending subscriptions tree"));
      HM_AVL3_DELETE(LOCAL.pending_subscriptions_tree, sub_cb->node);

    }
  }
  free(sub_cb);
  sub_cb = NULL;
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
}/* hm_free_subscription_cb */


/**
 *  @brief Allocates and initializes a notification CB
 *
 *  @param None
 *  @return CB of allocated Notification CB (#HM_NOTIFICATION_CB)
 */
HM_NOTIFICATION_CB * hm_alloc_notify_cb()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_NOTIFICATION_CB *notify_cb = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  notify_cb = (HM_NOTIFICATION_CB *)malloc(sizeof(HM_NOTIFICATION_CB));
  if(notify_cb == NULL)
  {
    TRACE_ERROR(("Error allocating resource for notification"));
    goto EXIT_LABEL;
  }
  notify_cb->custom_data = NULL;
  HM_INIT_LQE(notify_cb->node, notify_cb);
  notify_cb->notification_type = 0;
  notify_cb->node_cb.void_cb = NULL;

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return notify_cb;
}/* hm_alloc_notify_cb */


/**
 *  @brief Frees resources allocated for notification
 *
 *  @param notify_cb Notification CB (#HM_NOTIFICATION_CB)
 *  @return void
 */
void hm_free_notify_cb(HM_NOTIFICATION_CB *notify_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(notify_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if(HM_IN_LIST(notify_cb->node))
  {
    TRACE_DETAIL(("Removing from Notify list"));
    HM_REMOVE_FROM_LIST(notify_cb->node);
  }
  if(notify_cb->custom_data != NULL)
  {
    TRACE_DETAIL(("Blindly freeing custom data."));
    free(notify_cb->custom_data);
    notify_cb->custom_data = NULL;
  }
  free(notify_cb);
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
}/* hm_free_notify_cb */
