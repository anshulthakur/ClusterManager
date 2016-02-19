/**
 *  @file hmcluster.c
 *  @brief Cluster Messaging Routines
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None
 */
#include <hmincl.h>

/**
 *  @brief Checks if the Location ID contained in the Message is known.
 *
 *  Purpose: Checks if the Location ID contained in the Message is known. If
 *  not, then initiate connection with it. If yes, verify connection is up.
 *
 *  @param *msg Message received on Transport Layer
 *  @param *sender Address of the sender (#SOCKADDR)
 *  @return void
 */
void hm_cluster_check_location(HM_MSG *msg, SOCKADDR *sender)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_GLOBAL_LOCATION_CB *glob_cb = NULL;
  HM_LOCATION_CB *loc_cb = NULL;
  HM_SOCKET_CB *sock_cb = NULL;

  HM_PEER_MSG_KEEPALIVE *keepalive_msg = NULL;

  HM_SOCKADDR_UNION send_addr;


  int32_t loc_id, port_id;
  uint32_t temp_var;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(msg != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  keepalive_msg = msg->msg;

  memcpy(&send_addr.sock_addr, sender, sizeof(SOCKADDR));
  HM_GET_LONG(loc_id, keepalive_msg->hdr.hw_id);
  glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIND(LOCAL.locations_tree,
                          &loc_id,
                          locations_tree_by_db_id );
  if(glob_cb == NULL)
  {
    TRACE_INFO(("New Location Detected"));
    /***************************************************************************/
    /* Create a Location CB and fill in the details.               */
    /***************************************************************************/
    loc_cb = hm_alloc_location_cb();
    loc_cb->fsm_state = HM_PEER_FSM_STATE_NULL;
    HM_GET_LONG(loc_cb->index, keepalive_msg->hdr.hw_id);
    /***************************************************************************/
    /* It is going to be an outgoing connection                   */
    /***************************************************************************/
    loc_cb->peer_listen_cb = hm_alloc_transport_cb(HM_TRANSPORT_TCP_OUT);
    /***************************************************************************/
    /* Fix pointers                                 */
    /***************************************************************************/
    loc_cb->peer_listen_cb->location_cb = loc_cb;

    /***************************************************************************/
    /* Modify the destination port as per request message             */
    /***************************************************************************/
    HM_GET_LONG(port_id, keepalive_msg->listen_port);
    TRACE_INFO(("Remote listen port: %d",ntohs(port_id)));

    send_addr.in_addr.sin_port = port_id;
    sock_cb = hm_tprt_open_connection(HM_TRANSPORT_TCP_OUT, (void *)&send_addr);
    if(sock_cb == NULL)
    {
      TRACE_ERROR(("Error creating outgoing connection!"));
      hm_free_transport_cb(loc_cb->peer_listen_cb);
      loc_cb->peer_listen_cb = NULL;
      hm_free_location_cb(loc_cb);
      loc_cb = NULL;
      goto EXIT_LABEL;
    }

    loc_cb->peer_listen_cb->sock_cb = sock_cb;
    sock_cb->tprt_cb = loc_cb->peer_listen_cb;

    hm_peer_fsm(HM_PEER_FSM_CONNECT, loc_cb);
  }
  else
  {
    TRACE_DETAIL(("Keepalive Message from known location."));
    if(glob_cb->status != HM_STATUS_RUNNING)
    {
      TRACE_WARN(("Receiving Tick from an Inactive Node. State inconsistent!"));
      TRACE_ASSERT(FALSE);
    }
    /***************************************************************************/
    /* Decrement the Keepalive missed counter                   */
    /***************************************************************************/
    if(glob_cb->loc_cb->keepalive_missed > 0)
    {
      glob_cb->loc_cb->keepalive_missed--;
    }
    /***************************************************************************/
    /* Check if the number of processes and nodes cited in Keepalive are consi-*/
    /* -stent with what we have. If not, re-initiate replay.           */
    /***************************************************************************/
    if(!glob_cb->loc_cb->replay_in_progress)
    {
      TRACE_DETAIL(("Replay not in progress. Check consistency!"));
      HM_GET_LONG(temp_var, keepalive_msg->num_nodes);
      TRACE_DETAIL(("Active Nodes reported: %d", temp_var));
      if(temp_var != glob_cb->loc_cb->active_nodes)
      {
        TRACE_WARN(("States inconsistent, initiate replay."));
        hm_cluster_replay_info(glob_cb->loc_cb->peer_listen_cb);
        glob_cb->loc_cb->replay_in_progress = TRUE;
      }

      temp_var = 0;
      HM_GET_LONG(temp_var, keepalive_msg->num_proc);
      TRACE_DETAIL(("Active Processes reported: %d", temp_var));
      if(temp_var != glob_cb->loc_cb->active_processes)
      {
        TRACE_WARN(("States inconsistent, initiate replay."));
        hm_cluster_replay_info(glob_cb->loc_cb->peer_listen_cb);
        glob_cb->loc_cb->replay_in_progress = TRUE;
      }
    }
    else
    {
      TRACE_INFO(("Peer is in replay mode. Stats must converge soon."));
    }
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
}/* hm_cluster_check_location */


/**
 *  @brief Sends out a Keepalive message on the multicast socket
 *
 *  @param None
 *  @return void
 */
void hm_cluster_send_tick()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_PEER_MSG_KEEPALIVE *tick_msg = NULL;
  HM_MSG *msg = NULL;

  SOCKADDR_IN *addr = NULL;
  time_t current_time;

  uint32_t msg_type = HM_PEER_MSG_TYPE_KEEPALIVE;
#ifdef I_WANT_TO_DEBUG
  int32_t i = 0;
#endif
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Get a bufffer for Keepalive messages.                   */
  /* As an optimization, it can be done that the memory for a Tick is      */
  /* allocated only once, at the start of the day, and then reused.       */
  /* That is not being done now, because we still want to queue and call      */
  /* hm_free_buffer which won't be possible there.               */
  /* The use of HM_MSG buffer type restricts us from doing that for now.     */
  /***************************************************************************/
  msg = hm_get_buffer(sizeof(HM_PEER_MSG_KEEPALIVE));
  if(msg == NULL)
  {
    TRACE_ERROR(("Error allcating buffer for Keepalive Message."));
    TRACE_ASSERT(FALSE);
  }
  tick_msg = (HM_PEER_MSG_KEEPALIVE *)msg->msg;

  HM_PUT_LONG(tick_msg->hdr.hw_id, LOCAL.local_location_cb.index);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->hdr.hw_id);i++)
  {
    TRACE_DETAIL(("Index[%d]: %x", i,tick_msg->hdr.hw_id[i]));
  }
#endif

  HM_PUT_LONG(tick_msg->hdr.msg_type, msg_type);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->hdr.msg_type);i++)
  {
    TRACE_DETAIL(("Msg Type[%d]: %x", i,tick_msg->hdr.msg_type[i]));
  }
#endif

  current_time = hm_hton64(time(NULL));
  TRACE_DETAIL(("Time value: 0x%x", (unsigned int)current_time));
  memcpy(tick_msg->hdr.timestamp, &current_time, sizeof(current_time));
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->hdr.timestamp);i++)
  {
    TRACE_DETAIL(("Timestamp[%d]: %x", i,tick_msg->hdr.timestamp[i]));
  }
#endif

  addr = (SOCKADDR_IN *)&(LOCAL.local_location_cb.peer_listen_cb->address.address);
  TRACE_DETAIL(("Port Value: %d", ntohs(addr->sin_port)));
  TRACE_DETAIL(("Port Value[Network]: 0x%x", addr->sin_port));
  HM_PUT_LONG(tick_msg->listen_port, addr->sin_port);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->listen_port);i++)
  {
    TRACE_DETAIL(("Listen Port[%d]: %x",i, tick_msg->listen_port[i]));
  }
#endif

  HM_PUT_LONG(tick_msg->num_nodes, LOCAL.local_location_cb.active_nodes);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->num_nodes);i++)
  {
    TRACE_DETAIL(("Number of Nodes[%d]: %x", i,tick_msg->num_nodes[i]));
  }
#endif

  HM_PUT_LONG(tick_msg->num_proc, LOCAL.local_location_cb.active_processes);
#ifdef I_WANT_TO_DEBUG
  for(i=0;i<sizeof(tick_msg->num_proc);i++)
  {
    TRACE_DETAIL(("Number of Processes[%d]: %x", i,tick_msg->num_proc[i]));
  }
#endif
  /***************************************************************************/
  /* Message created. Now, add it to outgoing queue and try to send       */
  /***************************************************************************/
  if(hm_queue_on_transport(msg, LOCAL.mcast_addr, FALSE)!= HM_OK)
  {
    TRACE_ERROR(("Error occured while sending Tick"));
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
}/* hm_cluster_send_tick */


/**
 *  @brief Send an INIT request to the cluster location
 *
 *  @param *tprt_cb Transport Control Block of Peer to whom we would be sending
 *  this INIT message (#HM_TRANSPORT_CB)
 *
 *  @return HM_OK if successful, HM_ERR otherwise.
 */
int32_t hm_cluster_send_init(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_PEER_MSG_INIT *init_msg = NULL;
  HM_MSG *msg = NULL;

  int32_t ret_val= HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(tprt_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  msg = hm_get_buffer(sizeof(HM_PEER_MSG_INIT));
  if(msg == NULL)
  {
    TRACE_ERROR(("Error allocating buffer for INIT message."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  init_msg = (HM_PEER_MSG_INIT *)msg->msg;

  HM_PUT_LONG(init_msg->hdr.hw_id, LOCAL.local_location_cb.index);
  HM_PUT_LONG(init_msg->hdr.msg_type, HM_PEER_MSG_TYPE_INIT);
#ifdef I_WANT_TO_DEBUG
  {
    uint32_t test;
    HM_GET_LONG(test, init_msg->hdr.msg_type);
    TRACE_DETAIL(("Sending %d", (uint32_t)(init_msg->hdr.msg_type)));
    TRACE_DETAIL(("Sending %x", test));
  }
#endif
  HM_PUT_LONG(init_msg->hdr.timestamp, 0);

  HM_PUT_LONG(init_msg->request, TRUE);
  HM_PUT_LONG(init_msg->response_ok, FALSE);

  if(hm_queue_on_transport(msg, tprt_cb, TRUE)!= HM_OK)
  {
    TRACE_ERROR(("Error sending message to peer!"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_cluster_send_init */


/**
 *  @brief Replay complete local information to a peer.
 *
 *  @param tprt_cb  Transport Control Block of Peer to whom we would be sending
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_cluster_replay_info(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK; /* I am very optimistic! */

  HM_NODE_CB *node_cb = NULL;
  HM_PROCESS_CB *proc_cb = NULL;

  int32_t index_filled = 0, chunks_needed = 0, i;

  HM_MSG *msg = NULL;
  HM_PEER_MSG_REPLAY *replay_msg = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Chunks needed is minimum number of Blocks of HM_PEER_NUM_TLVS_PER_UPDATE*/
  /* that can transmit all Node info.                       */
  /***************************************************************************/
  chunks_needed =
      ((LOCAL.local_location_cb.total_nodes)/HM_PEER_NUM_TLVS_PER_UPDATE);
  if(((LOCAL.local_location_cb.total_nodes)%HM_PEER_NUM_TLVS_PER_UPDATE) != 0)
  {
  chunks_needed +=
      (
        ((LOCAL.local_location_cb.total_nodes)%HM_PEER_NUM_TLVS_PER_UPDATE)
        /
        ((LOCAL.local_location_cb.total_nodes)%HM_PEER_NUM_TLVS_PER_UPDATE)
       ); /* or simply 1 */
  }
  TRACE_DETAIL(("Chunks Needed = %d", chunks_needed));
  if(chunks_needed == 0)
  {
    /***************************************************************************/
    /* Send end of replay message, since if there are no nodes, there can't be */
    /* processes.                                 */
    /***************************************************************************/
    TRACE_DETAIL(("Send End of Replay. We have nothing to send."));
    if(hm_cluster_send_end_of_replay(tprt_cb)!= HM_OK)
    {
      TRACE_ERROR(("Error while sending End of Replay Message"));
      ret_val = HM_ERR;
    }
    /***************************************************************************/
    /* Nothing more to do. Time to return                     */
    /***************************************************************************/
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* First, build a message for Nodes                       */
  /***************************************************************************/
  index_filled = 0;
  i = 0;
  for(node_cb = (HM_NODE_CB *)HM_AVL3_FIRST(LOCAL.local_location_cb.node_tree,
                            nodes_tree_by_node_id);
        node_cb != NULL;
        node_cb = (HM_NODE_CB *)HM_AVL3_NEXT(node_cb->index_node, nodes_tree_by_node_id))
  {
    TRACE_DETAIL(("Filling out Node %d information.", node_cb->index));
    if(index_filled ==0)
    {
      /***************************************************************************/
      /* Allocate a message buffer if we know that currently, no indices have    */
      /* been used. This implies a new message must be constructed.         */
      /***************************************************************************/
#ifdef I_WANT_TO_DEBUG
      i++; /* keep score of number of allocs so far */
      TRACE_ASSERT(i<=chunks_needed);
#endif
      /***************************************************************************/
      /* Allocate a buffer for update                         */
      /***************************************************************************/
      msg = hm_get_buffer(sizeof(HM_PEER_MSG_REPLAY));
      if(msg == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Replay Message."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }

      replay_msg = (HM_PEER_MSG_REPLAY *)msg->msg;
      if(replay_msg == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Replay Message."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      memset(replay_msg, 0, sizeof(HM_PEER_MSG_REPLAY));
      HM_PUT_LONG(replay_msg->hdr.hw_id, LOCAL.local_location_cb.index);
      HM_PUT_LONG(replay_msg->hdr.msg_type, HM_PEER_MSG_TYPE_REPLAY);
      HM_PUT_LONG(replay_msg->hdr.timestamp, 0);

      /* Not the last message */
      HM_PUT_LONG(replay_msg->last, 0);
    }

    /***************************************************************************/
    /* It is necessary to communicate information of all the nodes that we know*/
    /* about, running or not.                           */
    /* This is because, later, if the node fails to start (during INIT), we    */
    /* send out an update of FAILED Node, and the remote HM would not find an  */
    /* entry for that node, causing trouble.                    */
    /***************************************************************************/
    HM_PUT_LONG(replay_msg->tlv[index_filled].group,
            (uint32_t)node_cb->group);
    HM_PUT_LONG(replay_msg->tlv[index_filled].node_id,
            (uint32_t)node_cb->index);
    HM_PUT_LONG(replay_msg->tlv[index_filled].update_type,
            HM_PEER_REPLAY_UPDATE_TYPE_NODE);
    if(node_cb->role != NODE_ROLE_NONE)
    {
      HM_PUT_LONG(replay_msg->tlv[index_filled].role,
                    (uint32_t)node_cb->current_role);
    }
    else
    {
      TRACE_DETAIL(("Node role resolution not completed. Use desired role"));
      HM_PUT_LONG(replay_msg->tlv[index_filled].role,
                    (uint32_t)node_cb->role);
    }
    HM_PUT_LONG(replay_msg->tlv[index_filled].running, (node_cb->fsm_state));

    index_filled++;

    /***************************************************************************/
    /* If we've filled enough TLVs at this point, send the message and prepare */
    /* a new buffer for filling up.                         */
    /***************************************************************************/
    if(index_filled==HM_PEER_NUM_TLVS_PER_UPDATE)
    {
      HM_PUT_LONG(replay_msg->num_tlvs, index_filled);
      /***************************************************************************/
      /* Made message, now queue on transport and send if possible         */
      /***************************************************************************/
      if(hm_queue_on_transport(msg, tprt_cb, FALSE)!= HM_OK)
      {
        TRACE_ERROR(("Error sending message to peer!"));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      hm_free_buffer(msg);
      index_filled = 0;
    }
  }//Node loop

  TRACE_ASSERT(i==chunks_needed);
  /***************************************************************************/
  /* At this point, either all node information has been sent, or there still*/
  /* remains the last incomplete TLV block message.               */
  /* Now, we will fill it with Process information.               */
  /***************************************************************************/
  /* Now, build a message for Processes                     */
  /***************************************************************************/
  chunks_needed =
      ((LOCAL.local_location_cb.active_processes)/HM_PEER_NUM_TLVS_PER_UPDATE);
  if(((LOCAL.local_location_cb.active_processes)%HM_PEER_NUM_TLVS_PER_UPDATE) != 0)
  {
    chunks_needed += 1;
  }
  if(chunks_needed == 0)
  {
    /***************************************************************************/
    /* If no more chunks are needed, verify that we've sent our previous msg   */
    /***************************************************************************/
    if(index_filled != 0)
    {
      TRACE_INFO(("No more message need be sent. Send the pending buffer and EOR!"));
      HM_PUT_LONG(replay_msg->num_tlvs, index_filled);
      if(hm_queue_on_transport(msg, tprt_cb, FALSE)!= HM_OK)
      {
        TRACE_ERROR(("Error sending message to peer!"));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
    }
    /***************************************************************************/
    /* Send end of replay message.                         */
    /***************************************************************************/
    TRACE_DETAIL(("Send End of Replay. We have nothing more to send."));
    if(hm_cluster_send_end_of_replay(tprt_cb)!= HM_OK)
    {
      TRACE_ERROR(("Error while sending End of Replay Message"));
      ret_val = HM_ERR;
    }
    /***************************************************************************/
    /* Nothing more to do. Time to return                     */
    /***************************************************************************/
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Now, loop through all the nodes collecting their running processes and  */
  /* making an update.                             */
  /***************************************************************************/
  i = 0;
  for(node_cb = (HM_NODE_CB *)HM_AVL3_FIRST(LOCAL.local_location_cb.node_tree,
                          nodes_tree_by_node_id);
      node_cb != NULL;
      node_cb = (HM_NODE_CB *)HM_AVL3_NEXT(node_cb->index_node, nodes_tree_by_node_id))
  {
    TRACE_DETAIL(("Filling out Node %d information.", node_cb->index));
    /***************************************************************************/
    /* Fill at max five process updates                       */
    /***************************************************************************/
    for(proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->process_tree,
                        node_process_tree_by_proc_type_and_pid);
      proc_cb != NULL;
      proc_cb = (HM_PROCESS_CB *)HM_AVL3_NEXT(proc_cb->node,node_process_tree_by_proc_type_and_pid))
    {
      TRACE_DETAIL(("Filling out Process %d information.", proc_cb->type));
      if(index_filled ==0)
      {
        /***************************************************************************/
        /* Allocate a message buffer if we know that currently, no indices have    */
        /* been used. This implies a new message must be constructed.         */
        /***************************************************************************/
#ifdef I_WANT_TO_DEBUG
        i++; /* keep score of number of allocs so far */
        TRACE_ASSERT(i<=chunks_needed);
#endif
        /***************************************************************************/
        /* Allocate a buffer for update                         */
        /***************************************************************************/
        msg = hm_get_buffer(sizeof(HM_PEER_MSG_REPLAY));
        if(msg == NULL)
        {
          TRACE_ERROR(("Error allocating memory for Replay Message."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }

        replay_msg = (HM_PEER_MSG_REPLAY *)msg->msg;
        if(replay_msg == NULL)
        {
          TRACE_ERROR(("Error allocating memory for Replay Message."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        memset(replay_msg, 0, sizeof(HM_PEER_MSG_REPLAY));
        HM_PUT_LONG(replay_msg->hdr.hw_id, LOCAL.local_location_cb.index);
        HM_PUT_LONG(replay_msg->hdr.msg_type, HM_PEER_MSG_TYPE_REPLAY);
        HM_PUT_LONG(replay_msg->hdr.timestamp, 0);

        /* Not the last message */
        HM_PUT_LONG(replay_msg->last, 0);
      }
      if(proc_cb->running != FALSE)
      {
        HM_PUT_LONG(replay_msg->tlv[index_filled].group,
                (uint32_t)proc_cb->type);
        HM_PUT_LONG(replay_msg->tlv[index_filled].node_id,
                (uint32_t)node_cb->index);
        HM_PUT_LONG(replay_msg->tlv[index_filled].update_type,
                HM_PEER_REPLAY_UPDATE_TYPE_PROC);
        HM_PUT_LONG(replay_msg->tlv[index_filled].pid,
                        (uint32_t)proc_cb->pid);

        index_filled++;
      }
      else
      {
        /***************************************************************************/
        /* Process is not running. Exclude it from update.               */
        /***************************************************************************/
        TRACE_DETAIL(("Exclude from update. Not running!"));
        continue;
      }
      /***************************************************************************/
      /* If we've filled enough TLVs at this point, send the message and prepare */
      /* a new buffer for filling up.                         */
      /***************************************************************************/

      if(index_filled==HM_PEER_NUM_TLVS_PER_UPDATE)
      {
        HM_PUT_LONG(replay_msg->num_tlvs, index_filled);
        /***************************************************************************/
        /* Made message, now queue on transport and send if possible         */
        /***************************************************************************/
        if(hm_queue_on_transport(msg, tprt_cb, FALSE)!= HM_OK)
        {
          TRACE_ERROR(("Error sending message to peer!"));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        hm_free_buffer(msg);
        index_filled = 0;
      }
    }//processes loop
    /***************************************************************************/
    /* If we've broken out before all TLVs were filled, then the process list  */
    /* is exhausted for this node and we still have space for more.        */
    /* So, move on to next node.                         */
    /***************************************************************************/
    if(index_filled != 0)
    {
      TRACE_DETAIL(("Move to next Node tree."));
      continue;
    }
  }//Node loop
  /***************************************************************************/
  /* If we've reached here and indexes filled is less than max., that mean   */
  /* we've exhausted all processes in all nodes and this is the penultimate  */
  /* message to be sent.                             */
  /***************************************************************************/
  if(index_filled != 0)
  {
    HM_PUT_LONG(replay_msg->num_tlvs, index_filled);
    /***************************************************************************/
    /* Made message, now queue on transport and send if possible         */
    /***************************************************************************/
    if(hm_queue_on_transport(msg, tprt_cb, FALSE)!= HM_OK)
    {
      TRACE_ERROR(("Error sending message to peer!"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    hm_free_buffer(msg);
  }

  TRACE_DETAIL(("Send End of Replay. We have nothing more to send."));
  if(hm_cluster_send_end_of_replay(tprt_cb)!= HM_OK)
  {
    TRACE_ERROR(("Error while sending End of Replay Message"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_cluster_replay_info */


/**
 *  @brief Sends the end of replay message
 *
 *  @param *tprt_cb Transport Control Block of Peer to whom we would be sending
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_cluster_send_end_of_replay(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_MSG *msg = NULL;
  HM_PEER_MSG_REPLAY *replay_msg = NULL;

  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(tprt_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  msg = hm_get_buffer(sizeof(HM_PEER_MSG_REPLAY));
  if(msg == NULL)
  {
    TRACE_ERROR(("Error allocating memory for Replay Message."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  replay_msg = (HM_PEER_MSG_REPLAY *)msg->msg;
  if(replay_msg == NULL)
  {
    TRACE_ERROR(("Error allocating memory for Replay Message."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  memset(replay_msg, 0, sizeof(HM_PEER_MSG_REPLAY));
  HM_PUT_LONG(replay_msg->hdr.hw_id, LOCAL.local_location_cb.index);
  HM_PUT_LONG(replay_msg->hdr.msg_type, HM_PEER_MSG_TYPE_REPLAY);
  HM_PUT_LONG(replay_msg->hdr.timestamp, 0);

  /* Not the last message */
  HM_PUT_LONG(replay_msg->last, 1);

  if(hm_queue_on_transport(msg, tprt_cb, FALSE)!= HM_OK)
  {
    TRACE_ERROR(("Error sending message to peer!"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  hm_free_buffer(msg);

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_cluster_send_end_of_replay */


/**
 *  @brief Receives and processes a cluster message
 *
 *  @param *sock_cb Socket Control Block on which a Cluster message was received (#HM_SOCKET_CB)
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_receive_cluster_message(HM_SOCKET_CB *sock_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;

  HM_PEER_MSG_HEADER *msg_header = NULL;
  int32_t msg_type;
  int32_t hw_id;

  HM_PEER_MSG_INIT *init_msg = NULL;
  HM_PEER_MSG_REPLAY *replay_msg = NULL;
  HM_PEER_MSG_NODE_UPDATE *node_update = NULL;
  HM_PEER_MSG_PROCESS_UPDATE *proc_update = NULL;
  //HM_PEER_MSG_UNION *msg = NULL; /* Use this instead of many pointers */

  HM_LOCATION_CB *loc_cb = NULL;
  HM_NODE_CB *node_cb = NULL;
  HM_PROCESS_CB *proc_cb = NULL;

  HM_MSG *msg = NULL;

  int32_t node_id;
  int32_t proc_key[2];
  int32_t status;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(sock_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  msg_header = (HM_PEER_MSG_HEADER *)sock_cb->tprt_cb->in_buffer;

  loc_cb = sock_cb->tprt_cb->location_cb;

  HM_GET_LONG(hw_id, msg_header->hw_id);
  HM_GET_LONG(msg_type, msg_header->msg_type);
  TRACE_DETAIL(("Received message of type %d from Location %d", msg_type, hw_id));

  /***************************************************************************/
  /* Since the message can be received over TCP or UDP, the incoming buffer  */
  /* is kept as large as possible while keeping the PDU size on the UDP     */
  /* socket limited.                               */
  /* Thus, here, we won't need to allocate more buffers. We already have a   */
  /* complete message.                             */
  /***************************************************************************/
  switch(msg_type)
  {
  case HM_PEER_MSG_TYPE_INIT:
    TRACE_DETAIL(("Received INIT message."));
    init_msg = (HM_PEER_MSG_INIT *)sock_cb->tprt_cb->in_buffer;
    HM_GET_LONG(hw_id, init_msg->hdr.hw_id);
    HM_GET_LONG(node_id, init_msg->request);
    if(node_id == TRUE)
    {
      TRACE_DETAIL(("Received INIT request."));
      HM_PUT_LONG(init_msg->response_ok, TRUE);
      HM_PUT_LONG(init_msg->request, TRUE);
      msg = hm_get_buffer(sizeof(HM_PEER_MSG_INIT));
      if(msg == NULL)
      {
        TRACE_ERROR(("Error allocating buffer for INIT response."));
        TRACE_ASSERT(msg != NULL);
      }
      memcpy(msg->msg, init_msg, sizeof(HM_PEER_MSG_INIT));
      /***************************************************************************/
      /* Queue on port                               */
      /***************************************************************************/
      if(hm_queue_on_transport(msg, sock_cb->tprt_cb, TRUE)!= HM_OK)
      {
        TRACE_ERROR(("Error while sending INIT response."));
        TRACE_ASSERT(FALSE);
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      hm_free_buffer(msg);
    }
    else
    {
      TRACE_DETAIL(("Received INIT Response."));
      HM_GET_LONG(node_id, init_msg->response_ok);
      if(node_id != TRUE)
      {
        TRACE_ERROR(("Response is not OK"));
        TRACE_ASSERT(FALSE);
      }
    }
    hm_peer_fsm(HM_PEER_FSM_INIT_RCVD, sock_cb->tprt_cb->location_cb);
    break;

  case HM_PEER_MSG_TYPE_REPLAY:
    TRACE_DETAIL(("Received Replay Message Sequence"));
    /***************************************************************************/
    /* Location cannot be inactive while doing replay               */
    /***************************************************************************/
    TRACE_ASSERT(sock_cb->tprt_cb->location_cb->fsm_state
                          == HM_PEER_FSM_STATE_ACTIVE);
    replay_msg = (HM_PEER_MSG_REPLAY *)sock_cb->tprt_cb->in_buffer;
    if( (ret_val = hm_cluster_process_replay(replay_msg, loc_cb))!= HM_OK)
    {
      TRACE_ERROR(("Error occurred while processing Replay Message."));
      goto EXIT_LABEL;
    }
    break;


  case HM_PEER_MSG_TYPE_NODE_UPDATE:
    TRACE_DETAIL(("Received Node Update"));
    /***************************************************************************/
    /* Location cannot be inactive while getting a node update              */
    /***************************************************************************/
    TRACE_ASSERT(sock_cb->tprt_cb->location_cb->fsm_state
                              == HM_PEER_FSM_STATE_ACTIVE);
    node_update = (HM_PEER_MSG_NODE_UPDATE *)sock_cb->tprt_cb->in_buffer;

    /***************************************************************************/
    /* Find the node_cb in HM                           */
    /***************************************************************************/
    HM_GET_LONG(node_id, node_update->node_id);
    node_cb = (HM_NODE_CB *)HM_AVL3_FIND(loc_cb->node_tree,
                        &node_id,
                        nodes_tree_by_node_id);
    if(node_cb == NULL)
    {
      TRACE_ERROR(("Node %d does not exist in DB and its update was received.", node_id));
      TRACE_ASSERT(FALSE);
      /* This is temporary. The code is unhittable, but currently on upgrade path */
      node_cb = hm_alloc_node_cb(FALSE);
      if(node_cb == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Remote Node"));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      HM_GET_LONG(node_cb->index, node_update->node_id);
      HM_GET_LONG(node_cb->group, node_update->node_group);
      HM_GET_LONG(node_cb->role, node_update->node_role);
      HM_GET_LONG(status, node_update->status);
      if(status == HM_PEER_ENTITY_STATUS_ACTIVE)
      {
        TRACE_DETAIL(("Node %d has become active.", node_id));
        node_cb->fsm_state = HM_NODE_FSM_STATE_ACTIVE;
      }
      else
      {
        node_cb->fsm_state = HM_NODE_FSM_STATE_FAILING;
      }
      /***************************************************************************/
      /* Add node to system                             */
      /***************************************************************************/
      if(hm_node_add(node_cb, loc_cb) != HM_OK)
      {
        TRACE_ERROR(("Error adding node to system."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* We've parsed the whole thing. No need to go any further with this msg.  */
      /***************************************************************************/
      break;
    }
    HM_GET_LONG(status, node_update->status);
    TRACE_DETAIL(("Current state of node: %d",node_cb->fsm_state));
    if((status == HM_PEER_ENTITY_STATUS_ACTIVE) && (node_cb->fsm_state != HM_NODE_FSM_STATE_ACTIVE))
    {
      TRACE_DETAIL(("Node %d has become active.", node_id));
      node_cb->fsm_state = HM_NODE_FSM_STATE_ACTIVE;
      TRACE_ASSERT(node_cb->parent_location_cb->active_nodes >=0);
      node_cb->parent_location_cb->active_nodes++;
      //TODO: Let remote nodes also be managed by the same FSM
      //hm_node_fsm(HM_NODE_FSM_TERM, node_cb);
    }
    else if((status == HM_PEER_ENTITY_STATUS_ACTIVE) && (node_cb->fsm_state == HM_NODE_FSM_STATE_ACTIVE))
    {
      TRACE_DETAIL(("Node %d has become active. But we know it already", node_id));
    }
    else if((status == HM_PEER_ENTITY_STATUS_INACTIVE) && (node_cb->fsm_state == HM_NODE_FSM_STATE_WAITING))
    {
      TRACE_DETAIL(("Node %d has failed to start.", node_id));
      node_cb->fsm_state = HM_NODE_FSM_STATE_FAILING;
    }
    else if((status == HM_PEER_ENTITY_STATUS_INACTIVE) && (node_cb->fsm_state != HM_NODE_FSM_STATE_FAILED))
    {
      TRACE_DETAIL(("Node %d has failed.", node_id));
      node_cb->fsm_state = HM_NODE_FSM_STATE_FAILING;
      node_cb->parent_location_cb->active_nodes--;
    }
    else if((status == HM_PEER_ENTITY_STATUS_INACTIVE) && (node_cb->fsm_state == HM_NODE_FSM_STATE_FAILED))
    {
      TRACE_DETAIL(("Node %d has failed. But we know it already", node_id));
    }
    else
    {
      TRACE_DETAIL(("Something happened to Node %d, but fail anyway.", node_id));
      node_cb->fsm_state = HM_NODE_FSM_STATE_FAILING;
      node_cb->parent_location_cb->active_nodes--;
    }
    hm_global_node_update(node_cb, HM_UPDATE_RUN_STATUS);
    break;

  case HM_PEER_MSG_TYPE_PROCESS_UPDATE:
    TRACE_DETAIL(("Received Process Update Message"));
    /***************************************************************************/
    /* Location cannot be inactive while getting a process update         */
    /***************************************************************************/
    TRACE_ASSERT(sock_cb->tprt_cb->location_cb->fsm_state
                              == HM_PEER_FSM_STATE_ACTIVE);
    proc_update = (HM_PEER_MSG_PROCESS_UPDATE *)sock_cb->tprt_cb->in_buffer;
    /***************************************************************************/
    /* Find the node_cb in HM                           */
    /***************************************************************************/
    HM_GET_LONG(node_id, proc_update->node_id);
    node_cb = (HM_NODE_CB *)HM_AVL3_FIND(loc_cb->node_tree,
                        &node_id,
                        nodes_tree_by_node_id);
    if(node_cb == NULL)
    {
      TRACE_ERROR(("Node %d does not exist in DB and its update was received.", node_id));
      ret_val = HM_ERR;
      break;
    }
    HM_GET_LONG(proc_key[0], proc_update->proc_type);
    HM_GET_LONG(proc_key[1], proc_update->proc_id);

    /***************************************************************************/
    /* Find the Process in the Nodes tree                     */
    /***************************************************************************/
    proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIND(node_cb->process_tree,
                        &proc_key,
                        node_process_tree_by_proc_type_and_pid);
    if(proc_cb == NULL)
    {
      TRACE_WARN(("Process CB not found!"));
      proc_cb = hm_alloc_process_cb();
      if(proc_cb == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Process CBs"));
        /***************************************************************************/
        /* Maybe, try again later?                           */
        /***************************************************************************/
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Find a node_cb                               */
      /***************************************************************************/

      proc_cb->parent_node_cb = node_cb;
      proc_cb->pid =  proc_key[1];
      proc_cb->type = proc_key[0];

      HM_GET_LONG(status, proc_update->status);
      if(status == HM_PEER_ENTITY_STATUS_ACTIVE)
      {
        TRACE_DETAIL(("Proces is active"));
        proc_cb->running = HM_STATUS_RUNNING;
        //TODO: Let remote nodes also be managed by the same FSM
        //hm_node_fsm(HM_NODE_FSM_TERM, node_cb);
      }
      else
      {
        TRACE_DETAIL(("Process is not running."));
        proc_cb->running = HM_STATUS_DOWN;
      }

      if(hm_process_add(proc_cb, proc_cb->parent_node_cb)!= HM_OK)
      {
        TRACE_ERROR(("Error occurred while adding process info to HM."));
        hm_free_process_cb(proc_cb);
        proc_cb = NULL;
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      TRACE_DETAIL(("Added Process!"));
      break;
    }
    /***************************************************************************/
    /* Found Process CB                                 */
    /***************************************************************************/
    HM_GET_LONG(status, proc_update->status);
    if(status == HM_PEER_ENTITY_STATUS_ACTIVE)
    {
      TRACE_DETAIL(("Process %d has become active.", proc_key[1]));
      proc_cb->running = HM_STATUS_RUNNING;
      //TODO: Let remote nodes also be managed by the same FSM
      //hm_node_fsm(HM_NODE_FSM_TERM, node_cb);
    }
    else
    {
      TRACE_DETAIL(("Process %d is not running.", proc_key[1]));
      proc_cb->running = HM_STATUS_DOWN;
    }
    hm_process_update(proc_cb);
    break;

  case HM_PEER_MSG_TYPE_HA_UPDATE:
    TRACE_DETAIL(("Received HA Update Message"));
    msg = (HM_PEER_MSG_UNION *)sock_cb->tprt_cb->in_buffer;
    if(hm_cluster_recv_ha_update((HM_PEER_MSG_HA_UPDATE *)msg, loc_cb)!=HM_OK)
    {
      TRACE_ERROR(("Error processing HA Update!"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    break;

  case HM_PEER_MSG_TYPE_BINDING:
    TRACE_DETAIL(("Received Subscription binding message"));
    msg = (HM_PEER_MSG_UNION *)sock_cb->tprt_cb->in_buffer;
    if(hm_cluster_recv_binding((HM_PEER_MSG_BINDING *)msg, loc_cb)!=HM_OK)
    {
      TRACE_ERROR(("Error processing Subscription bidning!"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    break;

  default:
    TRACE_WARN(("Unknown Message type."));
    TRACE_ASSERT(FALSE);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_receive_cluster_message */


/**
 *  @brief Processes an incoming Replay Message
 *
 *  @param *msg an #HM_PEER_MSG_REPLAY type message received on Transport
 *  @param *loc_cb Location Control Block (#HM_LOCATION_CB) of the Location from
 *  where message was received.
 *
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_cluster_process_replay(HM_PEER_MSG_REPLAY *msg, HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t end = FALSE;

  int32_t tlv_type, num_tlvs = 0;

  /***************************************************************************/
  /* Use of i is necessary. We MUST traverse update in order only. Process   */
  /* updates before nodes will fail if nodes do not exist.           */
  /***************************************************************************/
  int32_t node_id, i;

  HM_NODE_CB *node_cb= NULL;
  HM_PROCESS_CB *proc_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(msg != NULL);
  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  HM_GET_LONG(end, msg->last);
  if(end)
  {
    TRACE_DETAIL(("End of replay message received."));
    loc_cb->replay_in_progress = FALSE;
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* We expect more messages. Parse the current one               */
  /***************************************************************************/
  HM_GET_LONG(num_tlvs, msg->num_tlvs);
  TRACE_DETAIL(("%d TLVs present.", num_tlvs));
  for(i = 0;i< num_tlvs;i++)
  {
    HM_GET_LONG(tlv_type, msg->tlv[i].update_type);
    switch(tlv_type)
    {
    case HM_PEER_REPLAY_UPDATE_TYPE_NODE:
      TRACE_DETAIL(("Node Update"));
      node_cb = hm_alloc_node_cb(FALSE);
      if(node_cb == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Remote Node"));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      HM_GET_LONG(node_cb->index, msg->tlv[i].node_id);
      TRACE_DETAIL(("Node ID: %d", node_cb->index));
      HM_GET_LONG(node_cb->group, msg->tlv[i].group);
      HM_GET_LONG(node_cb->current_role, msg->tlv[i].role);

      HM_GET_LONG(node_cb->fsm_state, msg->tlv[i].running);
      if(node_cb->fsm_state != HM_NODE_FSM_STATE_ACTIVE)
      {
        TRACE_DETAIL(("Remote node not yet active. Do not trigger subscriptions yet!"));
        node_cb->fsm_state = HM_NODE_FSM_STATE_WAITING;/* It may or may not succeed. */
      }

      /***************************************************************************/
      /* Add node to system                             */
      /***************************************************************************/
      if(hm_node_add(node_cb, loc_cb) != HM_OK)
      {
        TRACE_ERROR(("Error adding node to system."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Resolve active-backup if we have suitable candidates                    */
      /***************************************************************************/
      hm_ha_resolve_active_backup(node_cb);
      break;

    case HM_PEER_REPLAY_UPDATE_TYPE_PROC:
      TRACE_DETAIL(("Process Update"));
      HM_GET_LONG(node_id, msg->tlv[i].node_id);
      /***************************************************************************/
      /* Find the node first                             */
      /***************************************************************************/
      node_cb = (HM_NODE_CB *)HM_AVL3_FIND(loc_cb->node_tree,
                          &node_id,
                          nodes_tree_by_node_id);
      if(node_cb== NULL)
      {
        TRACE_ERROR(("Could not find node in Location CB. Out of Order Replays?"));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      TRACE_DETAIL(("Found Node. Update Process Entries"));
      proc_cb = hm_alloc_process_cb();
      if(proc_cb == NULL)
      {
        TRACE_ERROR(("Error allocating memory for Process CBs"));
        /***************************************************************************/
        /* Maybe, try again later?                           */
        /***************************************************************************/
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Find a node_cb                               */
      /***************************************************************************/

      proc_cb->parent_node_cb = node_cb;
      HM_GET_LONG(proc_cb->pid, msg->tlv[i].pid);
      HM_GET_LONG(proc_cb->type, msg->tlv[i].group);
      proc_cb->running = TRUE;

      if(hm_process_add(proc_cb, proc_cb->parent_node_cb)!= HM_OK)
      {
        TRACE_ERROR(("Error occurred while adding process info to HM."));
        hm_free_process_cb(proc_cb);
        proc_cb = NULL;
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      TRACE_DETAIL(("Added Process!"));
      break;

    default:
      TRACE_DETAIL(("Unknown type %d", tlv_type));
      TRACE_ASSERT(FALSE);
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_cluster_process_replay */


/**
 *  @brief Sends out an update on the status of this entry in DB
 *
 *  @param *cb Control block for which Update needs to be sent.
 *  It is of type @c void to allow some polymorphism in the type of Control Blocks
 *
 *  @return HM_OK if successful, HM_ERR otherwise.
 */
int32_t hm_cluster_send_update(void *cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_SUBSCRIBER glob_cb;

  HM_GLOBAL_LOCATION_CB *loc_cb;

  HM_PEER_MSG_NODE_UPDATE *node_update = NULL;
  HM_PEER_MSG_PROCESS_UPDATE *proc_update = NULL;

  HM_MSG *msg = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  glob_cb.void_cb = cb;
  /* Value at address saved at memory location(
   *    treating the pointer as that of a uint32_t(
   *      treating the pointer cb as a byte, advance it by sizeof uint32_t
   *      )
   *    )
   */
  switch(*(uint32_t *)((char *)cb+ sizeof(uint32_t)))
  {
  case HM_TABLE_TYPE_NODES:
    TRACE_DETAIL(("Nodes Table Update"));
    msg = hm_get_buffer(sizeof(HM_PEER_MSG_NODE_UPDATE));
    if(msg == NULL)
    {
      TRACE_ERROR(("Error allocating buffer for Node Update"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    node_update = (HM_PEER_MSG_NODE_UPDATE *)msg->msg;
    memset(node_update, 0, sizeof(HM_PEER_MSG_NODE_UPDATE));
    HM_PUT_LONG(node_update->hdr.hw_id, LOCAL.local_location_cb.index);
    HM_PUT_LONG(node_update->hdr.msg_type, HM_PEER_MSG_TYPE_NODE_UPDATE);
    HM_PUT_LONG(node_update->hdr.timestamp, 0);

    HM_PUT_LONG(node_update->node_group, glob_cb.node_cb->group_index);
    HM_PUT_LONG(node_update->node_id, glob_cb.node_cb->index);
    HM_PUT_LONG(node_update->node_role, glob_cb.node_cb->role);
    if(glob_cb.node_cb->status == HM_NODE_FSM_STATE_ACTIVE)
    {
      HM_PUT_LONG(node_update->status, HM_PEER_ENTITY_STATUS_ACTIVE);
    }
    else
    {
      HM_PUT_LONG(node_update->status, HM_PEER_ENTITY_STATUS_INACTIVE);
    }
    break;

  case HM_TABLE_TYPE_PROCESS:
    TRACE_DETAIL(("Processes Table"));
    msg = hm_get_buffer(sizeof(HM_PEER_MSG_PROCESS_UPDATE));
    if(msg == NULL)
    {
      TRACE_ERROR(("Error allocating buffer for Process Update"));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    proc_update = (HM_PEER_MSG_PROCESS_UPDATE *)msg->msg;
    memset(proc_update, 0, sizeof(HM_PEER_MSG_PROCESS_UPDATE));
    HM_PUT_LONG(proc_update->hdr.hw_id, LOCAL.local_location_cb.index);
    HM_PUT_LONG(proc_update->hdr.msg_type, HM_PEER_MSG_TYPE_PROCESS_UPDATE);
    HM_PUT_LONG(proc_update->hdr.timestamp, 0);

    HM_PUT_LONG(proc_update->proc_type, glob_cb.process_cb->type);
    HM_PUT_LONG(proc_update->proc_id, glob_cb.process_cb->pid);
    HM_PUT_LONG(proc_update->node_id, glob_cb.process_cb->node_index);

    if(glob_cb.process_cb->status == HM_STATUS_RUNNING)
    {
      HM_PUT_LONG(proc_update->status, HM_PEER_ENTITY_STATUS_ACTIVE);
    }
    else
    {
      HM_PUT_LONG(proc_update->status, HM_PEER_ENTITY_STATUS_INACTIVE);
    }

    break;

  case HM_TABLE_TYPE_LOCATION:
    TRACE_DETAIL(("Location Table"));

    break;
  case HM_TABLE_TYPE_NODES_LOCAL:
    TRACE_ERROR(("Local Node Structure. Find its Global Entry."));
    TRACE_ASSERT(FALSE);
    break;
  case HM_TABLE_TYPE_LOCATION_LOCAL:
    TRACE_ERROR(("Local Location Structure. Find its Global Entry."));
    TRACE_ASSERT(FALSE);
    break;
  case HM_TABLE_TYPE_PROCESS_LOCAL:
    TRACE_ERROR(("Local Process Structure. Find its Global Entry."));
    TRACE_ASSERT(FALSE);
    break;
  default:
    TRACE_WARN(("Unknown type of subscriber"));
    TRACE_ASSERT((FALSE));
  }
  if(msg != NULL)
  {
    TRACE_DETAIL(("Try to send message to cluster."));
    /***************************************************************************/
    /* Later, it might be sending update only on the multicast port, but right */
    /* now, we must loop through all Location CBs and send to each if its      */
    /* connection in active.                           */
    /***************************************************************************/
    for(loc_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIRST(LOCAL.locations_tree, locations_tree_by_db_id);
        loc_cb != NULL;
        loc_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_NEXT(loc_cb->node, locations_tree_by_db_id))
    {
      if(loc_cb->loc_cb->index == LOCAL.local_location_cb.index)
      {
        /***************************************************************************/
        /* Don't send to itself!                           */
        /***************************************************************************/
        continue;
      }
      if(loc_cb->loc_cb->peer_listen_cb->sock_cb == NULL ||
          loc_cb->loc_cb->peer_listen_cb->sock_cb->sock_fd == -1)
      {
        /***************************************************************************/
        /* Peer is not connected.                           */
        /***************************************************************************/
        TRACE_DETAIL(("Skipping %d", loc_cb->index));
      }
      ret_val = hm_queue_on_transport(msg, loc_cb->loc_cb->peer_listen_cb, FALSE);
    }
    /***************************************************************************/
    /* We've sent/queued message on all transports. Release buffer on our side */
    /***************************************************************************/
    hm_free_buffer(msg);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_cluster_send_update */

/**
 *  hm_cluster_exchange_binding
 *  @brief Sends out subscription bindings on the cluster
 *
 *  @param *src_cb A global table entry pointer for the local entity which subscribed.
 *  @param num_bindings Number of bindings to be propagated
 *  @param *reg_msg Register Message from which to extract requests
 *
 *  @return Nothing
 */
void hm_cluster_exchange_binding(void *cb, uint32_t num_bindings, void *reg_msg)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  HM_PEER_MSG_BINDING *bind_msg = NULL;
  HM_PEER_BINDING_CB *binding_cb = NULL;
  HM_REGISTER_TLV_CB *tlv = NULL;
  int32_t i =0, subscriber_type, reqd;
  HM_SUBSCRIBER subscriber;

  int32_t packed = 0, total_packed = 0;

  HM_MSG *msg=NULL;
  HM_GLOBAL_LOCATION_CB *loc_cb;
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(cb !=NULL);
  TRACE_ASSERT(reg_msg != NULL);
  TRACE_ASSERT(num_bindings > 0);

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  reqd = (num_bindings/HM_PEER_NUM_TLVS_PER_UPDATE);
  if((num_bindings%HM_PEER_NUM_TLVS_PER_UPDATE) != 0)
  {
    reqd +=
      (
        (num_bindings%HM_PEER_NUM_TLVS_PER_UPDATE)
        /
        (num_bindings%HM_PEER_NUM_TLVS_PER_UPDATE)
       ); /* or simply 1 */
  }

  TRACE_DETAIL(("Require %d messages", reqd));
  /* Determine subscriber type to set subscriber ID */
  subscriber_type =*(int32_t *)((char *)cb+ (uint32_t)(sizeof(int32_t)));
  subscriber.void_cb = cb;
  tlv = (HM_REGISTER_TLV_CB *)((HM_REGISTER_MSG *)reg_msg)->data;

  while(reqd>0)
  {
    bind_msg = NULL;
    msg = hm_get_buffer(sizeof(HM_PEER_MSG_BINDING));
    if(msg == NULL)
    {
      TRACE_ASSERT(msg!=NULL);
      TRACE_ERROR(("Error allocating binding message buffer."));
      goto EXIT_LABEL;
    }
    bind_msg = (HM_PEER_MSG_BINDING *)msg->msg;

    memset(bind_msg, 0, sizeof(HM_PEER_MSG_BINDING));
    HM_PUT_LONG(bind_msg->hdr.hw_id, LOCAL.local_location_cb.index);
    HM_PUT_LONG(bind_msg->hdr.msg_type, HM_PEER_MSG_TYPE_BINDING);
    HM_PUT_LONG(bind_msg->hdr.timestamp, 0);

    packed = 0;
    switch(subscriber_type)
    {
    case HM_TABLE_TYPE_NODES_LOCAL:
      TRACE_DETAIL(("Local Node Structure."));
      HM_PUT_LONG(bind_msg->subscriber_id, subscriber.proper_node_cb->index);
      HM_PUT_LONG(bind_msg->subscriber_type, HM_PEER_REPLAY_UPDATE_TYPE_NODE);
      break;

    case HM_TABLE_TYPE_PROCESS_LOCAL:
      TRACE_DETAIL(("Local Process Structure."));
      HM_PUT_LONG(bind_msg->subscriber_id, subscriber.proper_process_cb->pid);
      HM_PUT_LONG(bind_msg->subscriber_type, HM_PEER_REPLAY_UPDATE_TYPE_PROC);
      break;

    default:
      TRACE_WARN(("Unknown type of subscriber %d", subscriber_type));
      TRACE_ASSERT((FALSE));
    }

    binding_cb = bind_msg->bindings;
    while(i<((HM_REGISTER_MSG *)reg_msg)->num_register
                                          && packed < HM_PEER_NUM_TLVS_PER_UPDATE)
    {
      tlv = tlv+i;

      if(tlv->cross_bind && packed < HM_PEER_NUM_TLVS_PER_UPDATE)
      {
        HM_PUT_LONG(binding_cb->subscription_id, tlv->id);
        switch(((HM_REGISTER_MSG *)reg_msg)->type)
        {
          case HM_REG_SUBS_TYPE_GROUP:
            TRACE_DETAIL(("Registering for Node Group"));
            HM_PUT_LONG(binding_cb->subscription_type, HM_REG_SUBS_TYPE_GROUP);
            break;
          case HM_REG_SUBS_TYPE_PROC:
            TRACE_DETAIL(("Registering for Process Types"));
            HM_PUT_LONG(binding_cb->subscription_type, HM_REG_SUBS_TYPE_PROC);
            break;
          default:
            TRACE_ERROR(("Registering for unsupported type: %d", ((HM_REGISTER_MSG *)reg_msg)->type));
            TRACE_ASSERT(0!=0);
            break;
        }
        packed++;
        total_packed++;
        binding_cb = binding_cb+1;
      }
      i++;
    }


    TRACE_DETAIL(("Try to send message to cluster."));
    HM_PUT_LONG(bind_msg->num_bindings, packed);
    /***************************************************************************/
    /* Later, it might be sending update only on the multicast port, but right */
    /* now, we must loop through all Location CBs and send to each if its      */
    /* connection in active.                           */
    /***************************************************************************/
    for(loc_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIRST(LOCAL.locations_tree, locations_tree_by_db_id);
        loc_cb != NULL;
        loc_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_NEXT(loc_cb->node, locations_tree_by_db_id))
    {
      if(loc_cb->loc_cb->index == LOCAL.local_location_cb.index)
      {
        /***************************************************************************/
        /* Don't send to itself!                           */
        /***************************************************************************/
        continue;
      }
      if(loc_cb->loc_cb->peer_listen_cb->sock_cb == NULL ||
          loc_cb->loc_cb->peer_listen_cb->sock_cb->sock_fd == -1)
      {
        /***************************************************************************/
        /* Peer is not connected.                           */
        /***************************************************************************/
        TRACE_DETAIL(("Skipping %d", loc_cb->index));
      }
      hm_queue_on_transport(msg, loc_cb->loc_cb->peer_listen_cb, FALSE);
    }
    /***************************************************************************/
    /* We've sent/queued message on all transports. Release buffer on our side */
    /***************************************************************************/
    hm_free_buffer(msg);
    reqd--;
  }//end while

  if(total_packed != num_bindings)
  {
    TRACE_ERROR(("Unequal bindings and packing values."));
    TRACE_ASSERT(packed==num_bindings);
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return;
} /* hm_cluster_exchange_binding */


/**
 *  hm_cluster_recv_binding
 *  @brief Receive a subscription binding from the cluster
 *
 *  @param *msg Subscription binding message.
 *  @return True if successful, else FALSE
 *
 *  A subscription binding can never arrive before the create message for the
 *  entity that generated it was received. Consequently, it is guaranteed that
 *  the initiator has a cb present here.
 *
 *  Proceed from here as if it were a regular subscribe request received from
 *  a regular local node.
 */
uint32_t hm_cluster_recv_binding(HM_PEER_MSG_BINDING *msg, HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  uint32_t ret_val = TRUE;
  uint32_t subscriber_id, subscriber_type, i;
  uint32_t reg_type, value, num_bindings;
  HM_SUBSCRIBER subscriber;

  HM_PEER_BINDING_CB *binding_cb = NULL;

  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(msg!= NULL);
  TRACE_ASSERT(loc_cb!=NULL);

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  HM_GET_LONG(subscriber_id, msg->subscriber_id);
  HM_GET_LONG(subscriber_type, msg->subscriber_type);
  HM_GET_LONG(num_bindings, msg->num_bindings);
  /* First find the corresponding subscriber. */

  switch(subscriber_type)
  {
    case HM_PEER_REPLAY_UPDATE_TYPE_NODE:
      TRACE_DETAIL(("Node subscriber"));
      /* There will always be a node */
      subscriber.proper_node_cb = (HM_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
                              &subscriber_id,
                              nodes_tree_by_db_id);
      if(subscriber.proper_node_cb == NULL)
      {
        TRACE_ERROR(("Binding received from a Node that we don't know about!"));
        TRACE_ASSERT(FALSE);
      }
      break;
    case HM_PEER_REPLAY_UPDATE_TYPE_PROC:
      TRACE_DETAIL(("Process subscriber"));
      subscriber.proper_process_cb = (HM_NODE_CB *)HM_AVL3_FIND(LOCAL.process_tree,
                              &subscriber_id,
                              process_tree_by_db_id);
      if(subscriber.proper_process_cb == NULL)
      {
        TRACE_ERROR(("Binding received from a Node that we don't know about!"));
        TRACE_ASSERT(FALSE);
      }
      break;

    default:
      TRACE_ERROR(("Unsupported type: %d", subscriber_type));
      TRACE_ASSERT(0!=0);
      break;
  }
  binding_cb = msg->bindings;
  for(i=0; i< num_bindings; i++)
  {
    binding_cb = binding_cb+i;
    HM_GET_LONG(reg_type, binding_cb->subscription_type);
    TRACE_DETAIL(("Registration Type: %d", reg_type));
    TRACE_DETAIL(("Subscription ID: %d", value));
    HM_GET_LONG(value, binding_cb->subscription_id);
    /* We've received it over a cluster. Don't need cross-bind. It already is.*/
    if(hm_subscribe(reg_type, value, subscriber.void_cb, FALSE) != HM_OK)
    {
      TRACE_ERROR(("Error creating subscriptions."));
      ret_val = FALSE;
      break;
    }
  }
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
} /* hm_cluster_recv_binding */
