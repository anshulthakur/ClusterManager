/** 
 *  @file hmha.c
 *  @brief High Avaialbility specific messages
 *  
 *  @author Anshul
 *  @date 07-Dec-2015
 *  @bug 
 */

#include <hmincl.h>

/**
 *  @brief Receives a High Availability Update from user interface.
 *
 *  @detail Checks if the message sets the node as master or slave. If set as
 *  master, and the current status in config is also master, send explicit
 *  passive notice to the slave node. If the node has been set as slave, first
 *  check if the selected active node is healthy. If yes, then send fail-over
 *  signal to the other side. If not, deny change by sending a negative ack to
 *  the user.
 *
 *  @param msg_buf Stream Buffer containing the message
 *  @param &tprt_cb #HM_TRANSPOR_CB structure of the transport on which message was received
 *
 *  @return #HM_OK on successful update, #HM_ERR on error
 */
int32_t hm_recv_ha_update(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t changed = FALSE;

  HM_NODE_CB *node_cb = NULL;
  HM_HA_STATUS_UPDATE_MSG *ha_msg = NULL;
  HM_ADDRESS_INFO *addr_info = NULL;
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(msg!=NULL);
  TRACE_ASSERT(tprt_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  TRACE_ASSERT(tprt_cb->node_cb!=NULL);
  ha_msg = (HM_HA_STATUS_UPDATE_MSG *)msg->msg;
  node_cb = tprt_cb->node_cb;
  TRACE_DETAIL(("HA Message from Node index %d", node_cb->index));

  /***************************************************************************/
  /* Parse the message                                                       */
  /***************************************************************************/
  switch(ha_msg->node_role)
  {
    case HM_HA_NODE_ROLE_ACTIVE:
      TRACE_DETAIL(("Node requested to be Active."));

      /* Write preferred status to config cb and to file */
      node_cb->role = NODE_ROLE_ACTIVE;
      if(ha_msg->node_info_provided!=FALSE)
      {
        /* Send message to passive node.  */
        node_cb=NULL;
        node_cb =  (HM_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
                                              &(addr_info->node_id),
                                              nodes_tree_by_db_id);
        if(node_cb == NULL)
        {
          TRACE_WARN(("Backup Node (%d) not found!", addr_info->node_id));
          /* Not a critical error. */
          ret_val = HM_OK;
        }
        else
        {
          /* Both must have same group indices */
          TRACE_ASSERT(tprt_cb->node_cb->group== node_cb->group);
          /* Also, partner cb pointer must match */
          TRACE_ASSERT(tprt_cb->node_cb->partner == node_cb);
          /* Send message to backup */
          if(hm_cluster_send_ha_update(tprt_cb->node_cb, node_cb, node_cb)!=HM_OK)
          {
            TRACE_ERROR(("Error sending HA Update to peer."));
            ret_val = HM_ERR;
            goto EXIT_LABEL;
          }
        }
      }
      else if(node_cb->partner!=NULL)
      {
        TRACE_DETAIL(("Have an autoresolved partner at %d", node_cb->partner->index));
        node_cb = node_cb->partner;
        /* Send message to backup */
        if(hm_cluster_send_ha_update(tprt_cb->node_cb, node_cb, node_cb)!=HM_OK)
        {
          TRACE_ERROR(("Error sending HA Update to peer."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        /*
         * FIXME: Does an entry in table ensure it is alive?
         * Is it appropriately dereferenced on termination?
         */
      }
      changed = TRUE;
      break;

    case HM_HA_NODE_ROLE_PASSIVE:
      TRACE_DETAIL(("Node requested to be Passive"));
      /* For now, the partner information is mandatory if node is set to passive */
      TRACE_ASSERT(ha_msg->node_info_provided == TRUE);
      addr_info = (HM_ADDRESS_INFO *)((BYTE *)msg + ha_msg->offset);

      /* The value must also have been auto-resolved? */
      TRACE_ASSERT(node_cb->partner!=NULL);
      TRACE_ASSERT(node_cb->partner->index==addr_info->node_id);
      node_cb->role = NODE_ROLE_ACTIVE;
      /* Send message to backup, which would now become active*/
      if(hm_cluster_send_ha_update(node_cb, tprt_cb->node_cb, node_cb)!=HM_OK)
      {
        TRACE_ERROR(("Error sending HA Update to peer."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /* Initiate planned Fail-Over. */
      /* Check if it can be done, by checking the status of node */
      changed=TRUE;

      break;

    default:
      TRACE_ASSERT(FALSE);
      break;
  }
  if(changed)
  {
    /* Assuming only the group 1 node may currently send such signals */
    if(hm_write_config_file(HM_DEFAULT_CONFIG_FILE_NAME)!=HM_OK)
    {
      TRACE_ERROR(("Error writing configuration file!"));
      ret_val = HM_ERR;
    }
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
} /* hm_recv_ha_update */

/**
 *  @brief Sends HA Update message to peer
 *
 *  @param slave_node Pointer to slave node cb
 *  @param master_node Pointer to the master node cb
 *  @param dest_node Destination node
 *
 *  @return #HM_OK on success, #HM_ERR on failure
 */
int32_t hm_cluster_send_ha_update(HM_NODE_CB *master_node, HM_NODE_CB *slave_node,
                                  HM_NODE_CB *dest_node)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_PEER_MSG_HA_UPDATE *ha_msg = NULL;
  HM_MSG *msg = NULL;
  time_t current_time;
#ifdef I_WANT_TO_DEBUG
  int32_t i = 0;
#endif
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(master_node !=NULL);
  TRACE_ASSERT(slave_node !=NULL);

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  msg  = hm_get_buffer(sizeof(HM_PEER_MSG_HA_UPDATE));
  if(msg == NULL)
  {
    TRACE_ERROR(("Error allocating buffer"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  ha_msg = (HM_PEER_MSG_HA_UPDATE *)msg->msg;
  HM_PUT_LONG(ha_msg->hdr.hw_id, LOCAL.local_location_cb.index);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(ha_msg->hdr.hw_id);i++)
  {
    TRACE_DETAIL(("Index[%d]: %x", i,ha_msg->hdr.hw_id[i]));
  }
#endif

  HM_PUT_LONG(ha_msg->hdr.msg_type, HM_PEER_MSG_TYPE_HA_UPDATE);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(ha_msg->hdr.msg_type);i++)
  {
    TRACE_DETAIL(("Msg Type[%d]: %x", i,ha_msg->hdr.msg_type[i]));
  }
#endif

  current_time = hm_hton64(time(NULL));
  TRACE_DETAIL(("Time value: 0x%x", (unsigned int)current_time));
  memcpy(ha_msg->hdr.timestamp, &current_time, sizeof(current_time));
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(ha_msg->hdr.timestamp);i++)
  {
    TRACE_DETAIL(("Timestamp[%d]: %x", i,ha_msg->hdr.timestamp[i]));
  }
#endif

  HM_PUT_LONG(ha_msg->group, slave_node->group);
  HM_PUT_LONG(ha_msg->slave_node, slave_node->index);
  HM_PUT_LONG(ha_msg->master_node, master_node->index);

  /***************************************************************************/
  /* Message created. Now, add it to outgoing queue and try to send          */
  /***************************************************************************/
  if(hm_queue_on_transport(msg, dest_node->transport_cb)!= HM_OK)
  {
    TRACE_ERROR(("Error occured while sending HA Update"));
    ret_val = HM_ERR;
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
} /* hm_cluster_send_ha_update */

/**
 *  hm_cluster_recv_ha_update
 *  @brief Receives a HA Update from the cluster node
 *
 *  @param *msg #HM_PEER_MSG_HA_UPDATE message received on Transport
 *  @param *loc_cb #HM_LOCATION_CB structure of sender.
 *
 *  @return #HM_OK on success, #HM_ERR on failure
 */
int32_t hm_cluster_recv_ha_update(HM_PEER_MSG_HA_UPDATE *ha_msg,
                                                        HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_GLOBAL_NODE_CB *node_cb = NULL;
  int32_t node_id;
#ifdef I_WANT_TO_DEBUG
  int32_t group_id;
#endif
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(ha_msg != NULL);
  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  HM_GET_LONG(node_id, ha_msg->master_node);
  node_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
                                           &node_id,
                                           nodes_tree_by_db_id);
  if(node_cb==NULL)
  {
    TRACE_ERROR(("Node %d not found in tree. Out of sync?", node_id));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
#ifdef I_WANT_TO_DEBUG
  HM_GET_LONG(group_id, ha_msg->group);
  TRACE_ASSERT(group_id== node_cb->group_index);
#endif
  /* Found node. Is it us, or is it a remote node? */
  if(node_cb->node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
  {
    TRACE_DETAIL(("We're promoted as primary. Fail-over message awaited."));
    /* Update config and write to file */
    node_cb->role = NODE_ROLE_ACTIVE;
  }
  else
  {
    node_cb->role = NODE_ROLE_PASSIVE;
  }
  if(hm_write_config_file(HM_DEFAULT_CONFIG_FILE_NAME)!=HM_OK)
  {
    TRACE_ERROR(("Error writing configuration file!"));
    ret_val = HM_ERR;
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
} /* hm_cluster_recv_ha_update */
