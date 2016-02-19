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
 *  hm_ha_role_update_callback
 *  @brief Updates roles of Nodes as active/backup depending on information
 *
 *  @param *location_cb Location CB of the Location for which timer was set
 *  @return #HM_OK on success, #HM_ERR otherwise
 *
 *  @detail Timer has popped to send HA role updates. Look through the local
 *  nodes tree and see if its current role has been updated from the clusters.
 *  If not, then we are assuming that what is written in desired role is to
 *  be accepted as is and send a role update to the location.
 *  If it has been set, then a notification must already have been sent. In that
 *  case, do nothing for that node.
 */
int32_t hm_ha_role_update_callback(void *location_cb)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_LOCATION_CB *loc_cb = NULL;
  HM_NODE_CB *node_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  loc_cb = (HM_LOCATION_CB *)loc_cb;

  /***************************************************************************/
  /* Loop through each node and determine what status to set.                */
  /***************************************************************************/
  for(node_cb = (HM_NODE_CB *)HM_AVL3_FIRST(LOCAL.local_location_cb.node_tree,
                              nodes_tree_by_node_id);
      node_cb != NULL;
      node_cb = (HM_NODE_CB *)HM_AVL3_NEXT(node_cb->index_node, nodes_tree_by_node_id))
  {
    if(node_cb->current_role == NODE_ROLE_NONE)
    {
      TRACE_DETAIL(("Node %d did not receive any update on cluster.", node_cb->index));
      /* Set role to that set in desired role field (role) if it is active. */
      /* If it is set to passive, then we cannot promote it to active.      */
      if(node_cb->role == NODE_ROLE_PASSIVE)
      {
        TRACE_WARN(("Cannot set Node to passive without any active."));
        continue;
      }
      else
      {
        node_cb->current_role = node_cb->role;
      }
    }
    /* Else if it was updated, we'll send that value only.*/
    TRACE_DETAIL(("Set node as %s",
                  (node_cb->current_role==NODE_ROLE_PASSIVE?"Passive":"Active/None")));
    /***************************************************************************/
    /* Check if the node is running or not.                                    */
    /* If running, then we will send it an HA Role update.                     */
    /* Otherwise, it will get the update when it connects (if it does in window*/
    /* period).                                                                */
    /***************************************************************************/
    if(hm_global_node_update(node_cb, HM_UPDATE_NODE_ROLE)!= HM_OK)
    {
       TRACE_ERROR(("Error propagating node update"));
       ret_val = HM_ERR;
       goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Send update on cluster about its updated role.                          */
    /***************************************************************************/
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Stop the timer                                                          */
  /***************************************************************************/
  HM_TIMER_STOP(LOCAL.local_location_cb.ha_timer_cb);
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
} /* hm_ha_role_update_callback */

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
        addr_info = (HM_ADDRESS_INFO *)(BYTE *)ha_msg + ha_msg->offset;
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
  if(hm_queue_on_transport(msg, dest_node->transport_cb, FALSE)!= HM_OK)
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

/**
 *  hm_ha_resolve_active_backup
 *  @brief Resolves candidature of active-backups if any exists in the DB
 *
 *  @param *node_cb a #HM_NDOE_CB type Node Control block
 *  @return Nothing
 *
 *  @detail Search through the global database to see if we have a master/slave
 *  for the same group and if it exists (no matter its current state), update
 *  information in both.
 *
 *  @note Currently only one backup per active is supported.
 */
void hm_ha_resolve_active_backup(HM_NODE_CB *node_cb)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  HM_GLOBAL_NODE_CB *glob_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  for(glob_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIRST(LOCAL.nodes_tree, nodes_tree_by_db_id);
      glob_cb != NULL;
      glob_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_NEXT(glob_cb->node, nodes_tree_by_db_id))
  {
    if(glob_cb->index != node_cb->index)
    {
      if (glob_cb->node_cb->group == node_cb->group)
      {
        TRACE_INFO(("Found a candidate node on location %d, node id %d",
              glob_cb->node_cb->parent_location_cb->index,
              glob_cb->index));
        TRACE_INFO(("Candidate's Role: %s", (glob_cb->role == NODE_ROLE_ACTIVE)?"active":"passive"));
        TRACE_INFO(("Node's Role: %s", (node_cb->current_role == NODE_ROLE_ACTIVE)?"active":"passive"));
        /* Resolve only if roles are not NONE */
        /* Or, if both are running on same location, and desired_roles are not NONE */
        if(
            (
              (glob_cb->node_cb->parent_location_cb->index
                                          == LOCAL.local_location_cb.index) &&
              ( node_cb->parent_location_cb->index
                                          == LOCAL.local_location_cb.index) &&
              (glob_cb->node_cb->role != NODE_ROLE_NONE)
            )
            ||(
                ( node_cb->parent_location_cb->index
                            != LOCAL.local_location_cb.index) &&
                (glob_cb->role != NODE_ROLE_NONE)
               )
           )
        {
          /* Edge case: Active and Backup on same hardware (A poor choice, though)*/
          if(glob_cb->node_cb->parent_location_cb->index==
                                    node_cb->parent_location_cb->index)
          {
            TRACE_INFO(("Active and backup on same location."));
            /* Here, chances are the current roles are NONE. In that case, compare desired roles */
            if(glob_cb->role == node_cb->role &&
                              glob_cb->node_cb->current_role== NODE_ROLE_NONE)
            {
              TRACE_ERROR(("Two Nodes on the same system want to take same roles."));
              TRACE_ASSERT(FALSE);
            }
            else if(glob_cb->role == node_cb->role &&
                              glob_cb->node_cb->current_role!= NODE_ROLE_NONE)
            {
              /* Both want same roles, but one is late. Grant the alternative role*/
              /* This means that no other remote node has reported roles          */
              /* Note that this has nothing to do with their running status       */
              TRACE_WARN(("Two Nodes on the same system want to take same roles."));
              node_cb->current_role = (glob_cb->node_cb->current_role == NODE_ROLE_ACTIVE)?
                                                        NODE_ROLE_PASSIVE:NODE_ROLE_ACTIVE;
              TRACE_DETAIL(("Set Node %d as %s.", node_cb->index,
                  (node_cb->current_role == NODE_ROLE_ACTIVE)?"active":"passive"));
              TRACE_DETAIL(("Set Node %d as %s.", glob_cb->index,
                      (glob_cb->node_cb->current_role == NODE_ROLE_ACTIVE)?"active":"passive"));
            }
          }
          else if(glob_cb->role == node_cb->current_role)
          {
            TRACE_WARN(("Contingency on Node roles for %s",
                (glob_cb->role==NODE_ROLE_ACTIVE)?"active":"passive"));
            if(glob_cb->node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
            {
              /* Own node. Conflict must be resolved soon */
              TRACE_WARN(("Conflict with our node! OYE!"));
              /* By virtue of current Protocol Sequence, we must oblige */
              glob_cb->node_cb->current_role = (node_cb->current_role == NODE_ROLE_ACTIVE)?
                                                        NODE_ROLE_PASSIVE:NODE_ROLE_ACTIVE;
            }
          }
          else
          {
            TRACE_DETAIL(("No contingency."));
            /* No conflicts. Grant desired role */
            glob_cb->node_cb->current_role = glob_cb->node_cb->role;
          }
          /* We should be reaching here only if the nodes are similar */
          /* By the end of this, if a similar node was found, the current_roles of   */
          /* both the parties will be updated with proper values.                    */
          /***************************************************************************/
          /* Fixup pointers of partner                                               */
          /***************************************************************************/
          node_cb->partner = glob_cb->node_cb;
          glob_cb->node_cb->partner = node_cb;

          TRACE_DETAIL(("Set Node %d as %s.", node_cb->index,
              (node_cb->current_role == NODE_ROLE_ACTIVE)?"active":"passive"));
          TRACE_DETAIL(("Set Node %d as %s.", glob_cb->index,
              (glob_cb->node_cb->current_role == NODE_ROLE_ACTIVE)?"active":"passive"));

          if(glob_cb->node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
          {
            /* If the other node is a local node, setup subscriptions too.*/
            hm_subscribe(HM_REG_SUBS_TYPE_NODE, node_cb->id, (void *)glob_cb, FALSE);
            if(hm_global_node_update(glob_cb->node_cb, HM_UPDATE_NODE_ROLE)!=HM_OK)
            {
              TRACE_ERROR(("Error updating global node."));
            }
          }
        }
      }
      /* Do not look any further */
      TRACE_DETAIL(("Partner found. Break search."));
      break;
    }
  }
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
} /* hm_ha_resolve_active_backup */
