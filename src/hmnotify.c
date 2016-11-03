/**
 *  @file hmnotify.c
 *  @brief Hardware Manager Notification Generating Routines
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */
#include <hmincl.h>

/**
 *  @brief Services the notifications queued in the Notifications Queue
 *
 *  @note This routine may be driven by a conditionally timed semaphore such that
 *  every time a notification is added, a consumer thread is invoked.
 *
 *  @param None
 *  @return #HM_OK on success, #HM_ERR on failure.
 */
int32_t hm_service_notify_queue()
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_NOTIFICATION_CB *notify_cb = NULL, *temp = NULL;
  HM_NOTIFICATION_MSG *notify_msg = NULL;
  HM_TRANSPORT_CB *tprt_cb = NULL;

  HM_SUBSCRIBER affected_node, subscriber;
  HM_MSG *msg = NULL;
  int32_t ret_val = HM_OK;
  uint32_t *processed = NULL;
  HM_LIST_BLOCK *list_member = NULL, *block = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  for(notify_cb = (HM_NOTIFICATION_CB *)HM_NEXT_IN_LIST(LOCAL.notification_queue);
      notify_cb != NULL;
      )
  {
    TRACE_DETAIL(("Found Notification of type %d", notify_cb->notification_type));
    affected_node.void_cb = notify_cb->node_cb.void_cb;

    switch (notify_cb->notification_type)
    {
    case HM_NOTIFICATION_NODE_ACTIVE:
      TRACE_INFO(("Node %d Active on Location %d", affected_node.node_cb->index,
          affected_node.node_cb->node_cb->parent_location_cb->index));
      if(affected_node.node_cb->sub_cb->num_subscribers == 0)
      {
        TRACE_DETAIL(("No subscriber!"));
        break;
        //TODO: We still need to send a cluster update
      }
      else
      {
        TRACE_DETAIL(("Number of subscribers: %d",
            affected_node.node_cb->sub_cb->num_subscribers));
      }
      msg = hm_build_notify_message(notify_cb);
      if(msg == NULL)
      {
        TRACE_ERROR(("Error building Notification."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Have message, update reference count                     */
      /***************************************************************************/
      notify_cb->ref_count = affected_node.node_cb->sub_cb->num_subscribers;
      /***************************************************************************/
      /* We are ready to send this message to various subscribers           */
      /***************************************************************************/
      for(list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(
                  affected_node.node_cb->sub_cb->subscribers_list);
          list_member != NULL;
        list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(list_member->node))
      {
        processed = (uint32_t *)list_member->opaque;
        TRACE_ASSERT(processed != NULL);
        if( *processed >= notify_cb->id)
        {
          TRACE_DETAIL(("Subscriber has been serviced!"));
          continue;
        }
        subscriber.void_cb = (void *)list_member->target;
        switch(*(int32_t *)((char *)subscriber.void_cb+ (uint32_t)(sizeof(int32_t))))
        {
          case HM_TABLE_TYPE_NODES:
            TRACE_DETAIL(("Global Node type"));
            tprt_cb = subscriber.node_cb->node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS:
            TRACE_DETAIL(("Global Process type"));
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.process_cb->pid;
            tprt_cb = subscriber.process_cb->proc_cb->parent_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_NODES_LOCAL:
            TRACE_DETAIL(("Node Structure."));
            tprt_cb = subscriber.proper_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS_LOCAL:
            TRACE_DETAIL(("Local Process Structure."));
            TRACE_ASSERT(subscriber.proper_process_cb->parent_node_cb != NULL);
            /* Fixup PID values in message and then, fixup subscriber pointer */
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.proper_process_cb->pid;
            tprt_cb = subscriber.proper_process_cb->parent_node_cb->transport_cb;
            break;

          default:
            TRACE_WARN(("Unknown type of subscriber"));
            TRACE_ASSERT((FALSE));
        }

        /***************************************************************************/
        /* Append the message to the outgoing message queue on the transport of the*/
        /* node.                                                                   */
        /* Send only if it is a local dispatch.                                    */
        /***************************************************************************/
        if(tprt_cb != NULL)
        {
          if(tprt_cb->location_cb->index == LOCAL.local_location_cb.index)
          {
            block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
            if(block == NULL)
            {
              TRACE_ASSERT(FALSE);
              TRACE_ERROR(("Error allocating memory for Process creation notificaton queuing!"));
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            HM_INIT_LQE(block->node, block);

            block->target = msg;

            HM_INSERT_BEFORE(tprt_cb->pending, block->node);
            msg->ref_count++; /* This is more important */
            hm_tprt_process_outgoing_queue(tprt_cb);
          }
          else
          {
            TRACE_DETAIL(("Remote peer binding. Discard!"));
          }
          *processed = notify_cb->id;
        }
        else
        {
          TRACE_INFO(("Subscribers are inactive. No notifications!"));
        }
      }
      break;

    case HM_NOTIFICATION_NODE_INACTIVE:
      TRACE_INFO(("Node %d inactive on Location %d", affected_node.node_cb->index,
          affected_node.node_cb->node_cb->parent_location_cb->index));
      if(affected_node.node_cb->sub_cb->num_subscribers == 0)
      {
        TRACE_DETAIL(("No subscriber!"));
        break;
        //TODO: We still need to send a cluster update
      }
      else
      {
        TRACE_DETAIL(("Number of subscribers: %d",
            affected_node.node_cb->sub_cb->num_subscribers));
      }
      msg = hm_build_notify_message(notify_cb);
      if(msg == NULL)
      {
        TRACE_ERROR(("Error building Notification."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Have message, update reference count                     */
      /***************************************************************************/
      notify_cb->ref_count = affected_node.node_cb->sub_cb->num_subscribers;
      /***************************************************************************/
      /* We are ready to send this message to various subscribers           */
      /***************************************************************************/
      for(list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(
                  affected_node.node_cb->sub_cb->subscribers_list);
          list_member != NULL;
        list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(list_member->node))
      {
        processed = (uint32_t *)list_member->opaque;
        TRACE_ASSERT(processed != NULL);
        if( *processed >= notify_cb->id)
        {
          TRACE_DETAIL(("Subscriber has been serviced!"));
          continue;
        }
        subscriber.void_cb = (void *)list_member->target;
        switch(*(int32_t *)((char *)subscriber.void_cb+ (uint32_t)(sizeof(int32_t))))
        {
          case HM_TABLE_TYPE_NODES:
            TRACE_DETAIL(("Global Node type"));
            tprt_cb = subscriber.node_cb->node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS:
            TRACE_DETAIL(("Global Process type"));
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.process_cb->pid;
            tprt_cb = subscriber.process_cb->proc_cb->parent_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_NODES_LOCAL:
            TRACE_DETAIL(("Node Structure."));
            tprt_cb = subscriber.proper_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS_LOCAL:
            TRACE_DETAIL(("Local Process Structure."));
            TRACE_ASSERT(subscriber.proper_process_cb->parent_node_cb != NULL);
            /* Fixup PID values in message and then, fixup subscriber pointer */
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.proper_process_cb->pid;
            tprt_cb = subscriber.proper_process_cb->parent_node_cb->transport_cb;
            break;

          default:
            TRACE_WARN(("Unknown type of subscriber"));
            TRACE_ASSERT((FALSE));
        }

        /***************************************************************************/
        /* Append the message to the outgoing message queue on the transport of the*/
        /* node.                                                                   */
        /* Send only if it is a local dispatch.                                    */
        /***************************************************************************/
        if(tprt_cb != NULL)
        {
          if(tprt_cb->location_cb->index == LOCAL.local_location_cb.index)
          {
            block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
            if(block == NULL)
            {
              TRACE_ASSERT(FALSE);
              TRACE_ERROR(("Error allocating memory for Process creation notificaton queuing!"));
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            HM_INIT_LQE(block->node, block);

            block->target = msg;

            HM_INSERT_BEFORE(tprt_cb->pending, block->node);
            msg->ref_count++; /* This is more important */
            hm_tprt_process_outgoing_queue(tprt_cb);
          }
          else
          {
            TRACE_DETAIL(("Remote peer binding. Discard!"));
          }
          *processed = notify_cb->id;
        }
      }
      break;

    case HM_NOTIFICATION_PROCESS_CREATED:
      TRACE_INFO(("Process 0x%x Created on Node %d of Location %d",
          affected_node.process_cb->pid,
          affected_node.process_cb->proc_cb->parent_node_cb->index,
          affected_node.process_cb->proc_cb->parent_node_cb->parent_location_cb->index));
      if(affected_node.process_cb->sub_cb->num_subscribers == 0)
      {
        TRACE_DETAIL(("No subscriber!"));
        break;
        //TODO: We still need to send a cluster update
      }
      else
      {
        TRACE_DETAIL(("Number of subscribers: %d",
            affected_node.process_cb->sub_cb->num_subscribers));
      }
      msg = hm_build_notify_message(notify_cb);
      if(msg == NULL)
      {
        TRACE_ERROR(("Error building Notification."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Have message, update reference count                     */
      /***************************************************************************/
      notify_cb->ref_count = affected_node.process_cb->sub_cb->num_subscribers;
      /***************************************************************************/
      /* We are ready to send this message to various subscribers           */
      /***************************************************************************/
      for(list_member =(HM_LIST_BLOCK *)HM_NEXT_IN_LIST(
                  affected_node.process_cb->sub_cb->subscribers_list);
        list_member != NULL;
        list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(list_member->node))
      {
        processed = (uint32_t *)list_member->opaque;
        TRACE_ASSERT(processed != NULL);
        if( *processed >= notify_cb->id)
        {
          TRACE_DETAIL(("Subscriber has been serviced!"));
          continue;
        }
        subscriber.void_cb = (void *)list_member->target;
        switch(*(int32_t *)((char *)subscriber.void_cb+ (uint32_t)(sizeof(int32_t))))
        {
          case HM_TABLE_TYPE_NODES:
            TRACE_DETAIL(("Global Node type"));
            tprt_cb = subscriber.node_cb->node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS:
            TRACE_DETAIL(("Global Process type"));
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.process_cb->pid;
            tprt_cb = subscriber.process_cb->proc_cb->parent_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_NODES_LOCAL:
            TRACE_DETAIL(("Node Structure."));
            tprt_cb = subscriber.proper_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS_LOCAL:
            TRACE_DETAIL(("Local Process Structure."));
            TRACE_ASSERT(subscriber.proper_process_cb->parent_node_cb != NULL);
            /* Fixup PID values in message and then, fixup subscriber pointer */
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.proper_process_cb->pid;
            tprt_cb = subscriber.proper_process_cb->parent_node_cb->transport_cb;
            break;

          default:
            TRACE_WARN(("Unknown type of subscriber"));
            TRACE_ASSERT((FALSE));
        }

        /***************************************************************************/
        /* Append the message to the outgoing message queue on the transport of the*/
        /* node.                                                                   */
        /* Send only if it is a local dispatch.                                    */
        /***************************************************************************/
        if(tprt_cb != NULL)
        {
          if(tprt_cb->location_cb->index == LOCAL.local_location_cb.index)
          {
            block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
            if(block == NULL)
            {
              TRACE_ASSERT(FALSE);
              TRACE_ERROR(("Error allocating memory for Process creation notificaton queuing!"));
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            HM_INIT_LQE(block->node, block);

            block->target = msg;

            HM_INSERT_BEFORE(tprt_cb->pending, block->node);
            msg->ref_count++; /* This is more important */
            hm_tprt_process_outgoing_queue(tprt_cb);
          }
          else
          {
            TRACE_DETAIL(("Remote peer binding. Discard!"));
          }
          *processed = notify_cb->id;
        }
      }
      break;
    case HM_NOTIFICATION_PROCESS_DESTROYED:
      TRACE_INFO(("Process Destroyed"));
      TRACE_INFO(("Process 0x%x Deleted on Node %d of Location %d",
                affected_node.process_cb->pid,
                affected_node.process_cb->proc_cb->parent_node_cb->index,
                affected_node.process_cb->proc_cb->parent_node_cb->parent_location_cb->index));
      if(affected_node.process_cb->sub_cb->num_subscribers == 0)
      {
        TRACE_DETAIL(("No subscriber!"));
        break;
        //TODO: We still need to send a cluster update
      }
      else
      {
        TRACE_DETAIL(("Number of subscribers: %d",
            affected_node.process_cb->sub_cb->num_subscribers));
      }
      msg = hm_build_notify_message(notify_cb);
      if(msg == NULL)
      {
        TRACE_ERROR(("Error building Notification."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Have message, update reference count                                    */
      /***************************************************************************/
      notify_cb->ref_count = affected_node.process_cb->sub_cb->num_subscribers;
      /***************************************************************************/
      /* We are ready to send this message to various subscribers           */
      /***************************************************************************/
      for(list_member =(HM_LIST_BLOCK *)HM_NEXT_IN_LIST(
                  affected_node.process_cb->sub_cb->subscribers_list);
        list_member != NULL;
        list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(list_member->node))
      {
        //TODO
        processed = (uint32_t *)list_member->opaque;
        TRACE_ASSERT(processed != NULL);
        if( *processed >= notify_cb->id)
        {
          TRACE_DETAIL(("Subscriber has been serviced!"));
          continue;
        }

        subscriber.void_cb = (void *)list_member->target;
        switch(*(int32_t *)((char *)subscriber.void_cb+ (uint32_t)(sizeof(int32_t))))
        {
          case HM_TABLE_TYPE_NODES:
            TRACE_DETAIL(("Global Node type"));
            tprt_cb = subscriber.node_cb->node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS:
            TRACE_DETAIL(("Global Process type"));
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.process_cb->pid;
            tprt_cb = subscriber.process_cb->proc_cb->parent_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_NODES_LOCAL:
            TRACE_DETAIL(("Node Structure."));
            tprt_cb = subscriber.proper_node_cb->transport_cb;
            break;

          case HM_TABLE_TYPE_PROCESS_LOCAL:
            TRACE_DETAIL(("Local Process Structure."));
            TRACE_ASSERT(subscriber.proper_process_cb->parent_node_cb != NULL);
            /* Fixup PID values in message and then, fixup subscriber pointer */
            notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
            notify_msg->subs_pid = subscriber.proper_process_cb->pid;
            tprt_cb = subscriber.proper_process_cb->parent_node_cb->transport_cb;
            break;

          default:
            TRACE_WARN(("Unknown type of subscriber"));
            TRACE_ASSERT((FALSE));
        }

        /***************************************************************************/
        /* Append the message to the outgoing message queue on the transport of the*/
        /* node.                                                                   */
        /* Send only if it is a local dispatch.                                    */
        /***************************************************************************/
        if(tprt_cb != NULL)
        {
          if(tprt_cb->location_cb->index == LOCAL.local_location_cb.index)
          {
            block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
            if(block == NULL)
            {
              TRACE_ASSERT(FALSE);
              TRACE_ERROR(("Error allocating memory for Process creation notificaton queuing!"));
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            HM_INIT_LQE(block->node, block);

            block->target = msg;

            HM_INSERT_BEFORE(tprt_cb->pending, block->node);
            msg->ref_count++; /* This is more important */
            hm_tprt_process_outgoing_queue(tprt_cb);
          }
          else
          {
            TRACE_DETAIL(("Remote peer binding. Discard!"));
          }
          *processed = notify_cb->id;
        }
      }
      break;

    case HM_NOTIFICATION_INTERFACE_ADDED:
      TRACE_INFO(("Interface Added"));
      break;
    case HM_NOTIFICATION_INTERFACE_DELETE:
      TRACE_INFO(("Interface Deleted"));
      break;

    case HM_NOTIFICATION_LOCATION_ACTIVE:
      TRACE_INFO(("Location Active"));

      break;
    case HM_NOTIFICATION_LOCATION_INACTIVE:
      TRACE_INFO(("Location Inactive"));
      break;

    case HM_NOTIFICATION_NODE_ROLE_ACTIVE:
    case HM_NOTIFICATION_NODE_ROLE_PASSIVE:
      TRACE_INFO(("Location Role Notification."));
      /* This message is not to be sent to subscribers but to node itself */
      msg = hm_build_notify_message(notify_cb);
      if(msg == NULL)
      {
        TRACE_ERROR(("Error building Notification."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      /***************************************************************************/
      /* Have message, update reference count                     */
      /***************************************************************************/
      notify_cb->ref_count = 1 ;

      subscriber.void_cb = (void *)affected_node.void_cb;
      switch(*(int32_t *)((char *)subscriber.void_cb+ (uint32_t)(sizeof(int32_t))))
      {
        case HM_TABLE_TYPE_NODES:
          TRACE_DETAIL(("Global Node type"));
          tprt_cb = subscriber.node_cb->node_cb->transport_cb;
          break;

        case HM_TABLE_TYPE_PROCESS:
          TRACE_DETAIL(("Global Process type"));
          notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
          notify_msg->subs_pid = subscriber.process_cb->pid;
          tprt_cb = subscriber.process_cb->proc_cb->parent_node_cb->transport_cb;
          break;

        case HM_TABLE_TYPE_NODES_LOCAL:
          TRACE_DETAIL(("Node Structure."));
          tprt_cb = subscriber.proper_node_cb->transport_cb;
          break;

        case HM_TABLE_TYPE_PROCESS_LOCAL:
          TRACE_DETAIL(("Local Process Structure."));
          TRACE_ASSERT(subscriber.proper_process_cb->parent_node_cb != NULL);
          /* Fixup PID values in message and then, fixup subscriber pointer */
          notify_msg = (HM_NOTIFICATION_MSG *)msg->msg;
          notify_msg->subs_pid = subscriber.proper_process_cb->pid;
          tprt_cb = subscriber.proper_process_cb->parent_node_cb->transport_cb;
          break;

        default:
          TRACE_WARN(("Unknown type of subscriber"));
          TRACE_ASSERT((FALSE));
      }

      /***************************************************************************/
      /* Append the message to the outgoing message queue on the transport of the*/
      /* node.                                                                   */
      /* Send only if it is a local dispatch.                                    */
      /***************************************************************************/
      if(tprt_cb != NULL)
      {
        if(tprt_cb->location_cb->index == LOCAL.local_location_cb.index)
        {
          block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
          if(block == NULL)
          {
            TRACE_ASSERT(FALSE);
            TRACE_ERROR(("Error allocating memory for Process creation notificaton queuing!"));
            ret_val = HM_ERR;
            goto EXIT_LABEL;
          }
          HM_INIT_LQE(block->node, block);

          block->target = msg;

          HM_INSERT_BEFORE(tprt_cb->pending, block->node);
          msg->ref_count++; /* This is more important */
          hm_tprt_process_outgoing_queue(tprt_cb);
        }
        else
        {
          TRACE_DETAIL(("Remote peer binding. Discard!"));
        }
      }
      break;

    default:
      TRACE_WARN(("Unknown Notification Type"));
      TRACE_ASSERT(FALSE);
    }
    /***************************************************************************/
    /* This notification has either been processed according to the notifier.  */
    /* it may happen that it is pending in some transport, but Notify CB can be*/
    /* freed now.                                 */
    /***************************************************************************/
    TRACE_DETAIL(("Freeing Notification CB"));
    temp = notify_cb;
    notify_cb = (HM_NOTIFICATION_CB *)HM_NEXT_IN_LIST(notify_cb->node);
    HM_REMOVE_FROM_LIST(temp->node);
    hm_free_notify_cb(temp);
    temp = NULL;
  }
EXIT_LABEL:

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_service_notify_queue */


/**
 *  @brief Builds a notification Message
 *
 *  Builds a notification Message that must be sent to various subscribers
 *  of the subscription point. The message stays the same so we will store it
 *  in the opaque section of the notification CB until it has been transmitted
 *  to all the subscribers.
 *
 *  @param *notify_cb Notification CB (#HM_NOTIFICATION_CB) for which message should be created
 *  @return Pointer to a fully formed #HM_MSG message containing information on this notification.
 */
HM_MSG * hm_build_notify_message(HM_NOTIFICATION_CB *notify_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_MSG *msg = NULL;
  HM_NOTIFICATION_MSG *notify_msg = NULL;
  HM_SUBSCRIBER affected_node;
  HM_SOCKADDR_UNION *addr=NULL;

  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(notify_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  msg  = hm_get_buffer(sizeof(HM_NOTIFICATION_MSG));
  if(msg == NULL)
  {
    TRACE_ERROR(("Error allocating buffer"));
    goto EXIT_LABEL;
  }
  notify_msg = msg->msg;

  notify_msg->hdr.msg_id = 0;
  notify_msg->hdr.msg_len = sizeof(HM_NOTIFICATION_MSG);
  notify_msg->hdr.msg_type = HM_MSG_TYPE_HA_NOTIFY;
  notify_msg->hdr.request = FALSE;
  notify_msg->hdr.response_ok = FALSE;

  affected_node.node_cb = notify_cb->node_cb.node_cb;

  /***************************************************************************/
  /* WICKED CODE. Please refrain from using so many levels of pointer.     */
  /***************************************************************************/
  switch(notify_cb->notification_type)
  {
  case HM_NOTIFICATION_NODE_ACTIVE:
    TRACE_INFO(("Node %d Active on Location %d", affected_node.node_cb->index,
        affected_node.node_cb->node_cb->parent_location_cb->index));
    notify_msg->type = HM_NOTIFY_TYPE_NODE_UP;
    notify_msg->id = 0;
    notify_msg->if_id = 0;
    notify_msg->proc_type = 0;

    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb != NULL);
    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb->location_cb != NULL);

    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb->sock_cb != NULL);

    addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->transport_cb->sock_cb->addr);

    if(affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IN
      ||
      affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_OUT)
    {
      TRACE_DETAIL(("Node has IPv4 type of connection."));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;

      memcpy(    notify_msg->addr_info.addr,
            &addr->in_addr.sin_addr.s_addr,
            sizeof(struct in_addr)
          );

      notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
#ifdef I_WANT_TO_DEBUG
    {
    char address_value[128];
    inet_ntop(addr->in_addr.sin_family,
          notify_msg->addr_info.addr,
          address_value, sizeof(address_value));
    TRACE_INFO(("IP Address filled: %s:%d",address_value, notify_msg->addr_info.port));
    }
#endif
    }
    else if(affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IPv6_IN)
    {
      TRACE_DETAIL(("Node has IPv6 type of connection"));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
      memcpy(notify_msg->addr_info.addr,
        &addr->in6_addr.sin6_addr,
        sizeof(struct in6_addr));
      notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
    }
    else
    {
      TRACE_WARN(("Unknown Transport Type %d", affected_node.node_cb->node_cb->transport_cb->type));
    }
    notify_msg->addr_info.group = affected_node.node_cb->group_index;
    notify_msg->addr_info.hw_index = affected_node.node_cb->node_cb->parent_location_cb->index;
    notify_msg->addr_info.node_id = affected_node.node_cb->index;

    break;

  case HM_NOTIFICATION_NODE_INACTIVE:
    TRACE_INFO(("Node %d inactive on Location %d", affected_node.node_cb->index,
        affected_node.node_cb->node_cb->parent_location_cb->index));
    notify_msg->type = HM_NOTIFY_TYPE_NODE_DOWN;
    notify_msg->id = 0;
    notify_msg->if_id = 0;
    notify_msg->proc_type = 0;

    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb != NULL);

    if(affected_node.node_cb->node_cb->transport_cb->sock_cb != NULL)
    {
      addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->transport_cb->sock_cb->addr);
      if((affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IN) ||
        (affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_OUT))
      {
        TRACE_DETAIL(("Node has IPv4 type of connection."));
        notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
        memcpy(notify_msg->addr_info.addr,
            &addr->in_addr.sin_addr.s_addr,
          sizeof(struct in_addr));
        notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
      }
      else if((affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IPv6_IN)||
          (affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IPv6_OUT))
      {
        TRACE_DETAIL(("Node has IPv6 type of connection"));
        notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
        memcpy(notify_msg->addr_info.addr,
          &addr->in6_addr.sin6_addr,
          sizeof(struct in6_addr));
        notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
      }
      else
      {
        TRACE_WARN(("Unknown Transport Type"));
      }
    }
    else
    {
      TRACE_WARN(("Node never connected. Nothing to put in its transport profile notification."));
    }

    notify_msg->addr_info.group = affected_node.node_cb->group_index;
    notify_msg->addr_info.hw_index = affected_node.node_cb->node_cb->parent_location_cb->index;
    notify_msg->addr_info.node_id = affected_node.node_cb->index;

    break;

  case HM_NOTIFICATION_PROCESS_CREATED:

    notify_msg->type = HM_NOTIFY_TYPE_PROC_AVAILABLE;
    notify_msg->id = affected_node.process_cb->pid;
    notify_msg->if_id = 0;
    notify_msg->proc_type = affected_node.process_cb->type;
    TRACE_INFO(("Process  %x Created", notify_msg->id ));
    TRACE_ASSERT(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb != NULL);

    addr = (HM_SOCKADDR_UNION *)&(
        affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->sock_cb->addr);
    if(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->type
              == HM_TRANSPORT_TCP_IN)
    {
      TRACE_DETAIL(("Node has IPv4 type of connection."));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
      memcpy(notify_msg->addr_info.addr,
          &addr->in_addr.sin_addr.s_addr,
        sizeof(struct in_addr));
      notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
    }
    else if(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->type
              == HM_TRANSPORT_TCP_IPv6_IN)
    {
      TRACE_DETAIL(("Node has IPv6 type of connection"));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
      memcpy(notify_msg->addr_info.addr,
        &addr->in6_addr.sin6_addr,
        sizeof(struct in6_addr));
      notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
    }
    else
    {
      TRACE_WARN(("Unknown Transport Type"));
    }
    notify_msg->addr_info.group = affected_node.process_cb->proc_cb->parent_node_cb->group;
    notify_msg->addr_info.hw_index =
          affected_node.process_cb->proc_cb->parent_node_cb->parent_location_cb->index;
    notify_msg->addr_info.node_id = affected_node.process_cb->proc_cb->parent_node_cb->index;
    notify_msg->addr_info.role = affected_node.process_cb->proc_cb->parent_node_cb->role;

    break;
  case HM_NOTIFICATION_PROCESS_DESTROYED:
    TRACE_INFO(("Process Destroyed"));
    notify_msg->type = HM_NOTIFY_TYPE_PROC_GONE;
    notify_msg->id = affected_node.process_cb->pid;
    notify_msg->if_id = 0;
    notify_msg->proc_type = affected_node.process_cb->type;
    TRACE_ASSERT(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb != NULL);
    addr = (HM_SOCKADDR_UNION *)&(
        affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->sock_cb->addr);
    if(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->type
              == HM_TRANSPORT_TCP_IN)
    {
      TRACE_DETAIL(("Node has IPv4 type of connection."));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
      memcpy(notify_msg->addr_info.addr,
          &addr->in_addr.sin_addr.s_addr,
        sizeof(struct in_addr));
      notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
    }
    else if(affected_node.process_cb->proc_cb->parent_node_cb->transport_cb->type
              == HM_TRANSPORT_TCP_IPv6_IN)
    {
      TRACE_DETAIL(("Node has IPv6 type of connection"));
      notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
      memcpy(notify_msg->addr_info.addr,
        &addr->in6_addr.sin6_addr,
        sizeof(struct in6_addr));
      notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
    }
    else
    {
      TRACE_WARN(("Unknown Transport Type"));
    }
    notify_msg->addr_info.group = affected_node.process_cb->proc_cb->parent_node_cb->group;
    notify_msg->addr_info.hw_index =
          affected_node.process_cb->proc_cb->parent_node_cb->parent_location_cb->index;
    notify_msg->addr_info.node_id = affected_node.process_cb->proc_cb->parent_node_cb->index;
    notify_msg->addr_info.role = affected_node.process_cb->proc_cb->parent_node_cb->role;
    break;

  case HM_NOTIFICATION_INTERFACE_ADDED:
    TRACE_INFO(("Interface Added"));
    break;
  case HM_NOTIFICATION_INTERFACE_DELETE:
    TRACE_INFO(("Interface Deleted"));
    break;

  case HM_NOTIFICATION_NODE_ROLE_ACTIVE:
    TRACE_INFO(("Node Role Active"));
    TRACE_INFO(("Node %d role change to %s", affected_node.node_cb->index,
        (affected_node.node_cb->role==NODE_ROLE_PASSIVE)?"Passive":"Active/None"));
    notify_msg->hdr.msg_type = HM_MSG_TYPE_HA_UPDATE;

    if(affected_node.node_cb->role != HM_HA_NODE_ROLE_NONE)
    {
      notify_msg->type = (affected_node.node_cb->role==NODE_ROLE_ACTIVE)?
                            HM_HA_NODE_ROLE_ACTIVE:HM_HA_NODE_ROLE_PASSIVE;
    }
    else
    {
      notify_msg->type = HM_HA_NODE_ROLE_NONE;
    }
    notify_msg->id = 0;
    notify_msg->proc_type = 0;

    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb != NULL);
    /* For active node, there may or may not be a passive node on cluster */
    if(affected_node.node_cb->node_cb->partner != NULL)
    {
      TRACE_DETAIL(("Node partner present."));
      if(affected_node.node_cb->node_cb->partner->transport_cb->sock_cb != NULL)
      {
        addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->partner->transport_cb->sock_cb->addr);
        if((affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IN) ||
          (affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_OUT))
        {
          TRACE_DETAIL(("Node has IPv4 type of connection."));
          notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
          memcpy(notify_msg->addr_info.addr,
              &addr->in_addr.sin_addr.s_addr,
            sizeof(struct in_addr));
          notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
        }
        else if((affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IPv6_IN)||
            (affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IPv6_OUT))
        {
          TRACE_DETAIL(("Node has IPv6 type of connection"));
          notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
          memcpy(notify_msg->addr_info.addr,
            &addr->in6_addr.sin6_addr,
            sizeof(struct in6_addr));
          notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
        }
        else
        {
          TRACE_WARN(("Unknown Transport Type"));
        }
      }
      else
      {
        /* Possible point of bug. */
        TRACE_WARN(("Node Partner present without transport connection."));
        TRACE_ASSERT(FALSE);
      }
      notify_msg->addr_info.group = affected_node.node_cb->node_cb->partner->group;
      notify_msg->addr_info.hw_index = affected_node.node_cb->node_cb->partner->parent_location_cb->index;
      notify_msg->addr_info.node_id = affected_node.node_cb->node_cb->partner->index;
      notify_msg->if_id = TRUE;
    }
    else
    {
      TRACE_DETAIL(("Node partner not present"));
      notify_msg->if_id = FALSE;
    }

    break;

  case HM_NOTIFICATION_NODE_ROLE_PASSIVE:
    TRACE_INFO(("Node Role Passive or None"));
    TRACE_INFO(("Node %d role change to %s", affected_node.node_cb->index,
        (affected_node.node_cb->role==NODE_ROLE_PASSIVE)?"Passive":"Active/None"));
    notify_msg->hdr.msg_type = HM_MSG_TYPE_HA_UPDATE;

    if(affected_node.node_cb->role != HM_HA_NODE_ROLE_NONE)
    {
      notify_msg->type = (affected_node.node_cb->role==NODE_ROLE_ACTIVE)?
                            HM_HA_NODE_ROLE_ACTIVE:HM_HA_NODE_ROLE_PASSIVE;
    }
    else
    {
      notify_msg->type = HM_HA_NODE_ROLE_NONE;
    }
    notify_msg->id = 0;
    notify_msg->if_id = 0;
    notify_msg->proc_type = 0;

    TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb != NULL);
    /* For a passive node, there must always be an active node on the cluster */
    TRACE_ASSERT(affected_node.node_cb->node_cb->partner != NULL);
    TRACE_DETAIL(("Node partner present."));
    if(affected_node.node_cb->node_cb->partner->transport_cb->sock_cb != NULL)
    {
      addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->partner->transport_cb->sock_cb->addr);
      if((affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IN) ||
        (affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_OUT))
      {
        TRACE_DETAIL(("Node has IPv4 type of connection."));
        notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
        memcpy(notify_msg->addr_info.addr,
            &addr->in_addr.sin_addr.s_addr,
          sizeof(struct in_addr));
        notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
      }
      else if((affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IPv6_IN)||
          (affected_node.node_cb->node_cb->partner->transport_cb->type == HM_TRANSPORT_TCP_IPv6_OUT))
      {
        TRACE_DETAIL(("Node has IPv6 type of connection"));
        notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v6;
        memcpy(notify_msg->addr_info.addr,
          &addr->in6_addr.sin6_addr,
          sizeof(struct in6_addr));
        notify_msg->addr_info.port = (uint32_t)addr->in6_addr.sin6_port;
      }
      else
      {
        TRACE_WARN(("Unknown Transport Type"));
      }
    }
    else
    {
      /* Possible point of bug. */
      TRACE_WARN(("Node Partner present without transport connection."));
      TRACE_ASSERT(FALSE);
    }

    notify_msg->addr_info.group = affected_node.node_cb->node_cb->partner->group;
    notify_msg->addr_info.hw_index = affected_node.node_cb->node_cb->partner->parent_location_cb->index;
    notify_msg->addr_info.node_id = affected_node.node_cb->node_cb->partner->index;

    notify_msg->if_id = TRUE;
    break;

  default:
    TRACE_ERROR(("Unknown notification type"));
    TRACE_ASSERT(FALSE);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (msg);
}/* hm_build_notify_message */
