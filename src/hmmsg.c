/**
 *  @file hmmsg.c
 *  @brief Transport Layer Message Processing Methods
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */

#include <hmincl.h>


/**
 *  @brief Determines where to send the incoming message.
 *  It assumes that a message header has been received in the in_buffer.
 *
 *  @param *sock_cb Socket Control Block #HM_SOCKET_CB) on which incoming message
 *      notification is received
 *
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_route_incoming_message(HM_SOCKET_CB *sock_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_MSG_HEADER *msg_hdr = NULL;
  int32_t bytes_rcvd = 0;
  HM_MSG *msg_buf = NULL;
  int32_t ret_val = HM_OK;
  int32_t size = 0;

  SOCKADDR *udp_sender;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(sock_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Transport will have at least the space for a Message Header. Get bytes  */
  /* into that buffer and then invoke the layer specific code.               */
  /***************************************************************************/
  sock_cb->tprt_cb->in_buffer = (char *)&sock_cb->tprt_cb->header;

  /***************************************************************************/
  /* If associated transport has a node_cb associated with it, it is a Node  */
  /* connection. Else, it is a cluster connection/Location Connection.       */
  /***************************************************************************/
  if (sock_cb->tprt_cb->node_cb != NULL)
  {
    TRACE_DETAIL(("Message must be from Node"));
    /***************************************************************************/
    /* First, receive the message header and verify that it is an INIT message */
    /***************************************************************************/
    sock_cb->tprt_cb->in_bytes = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                 sock_cb->sock_type,
                                 (BYTE *)sock_cb->tprt_cb->in_buffer,
                                 sizeof(HM_MSG_HEADER),
                                 NULL
                                                       );

    if ((sock_cb->tprt_cb->in_bytes != sizeof(HM_MSG_HEADER)))
    {
      TRACE_DETAIL(("Message Length of %u was expected, %d was received",
                    sizeof(HM_MSG_HEADER), sock_cb->tprt_cb->in_bytes));
      hm_tprt_handle_improper_read(sock_cb->tprt_cb->in_bytes, sock_cb->tprt_cb);
      //ret_val = HM_ERR;
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Route the message to its appropriate handler                 */
    /***************************************************************************/
    msg_hdr = (HM_MSG_HEADER *)sock_cb->tprt_cb->in_buffer;

    /***************************************************************************/
    /* Depending on the incoming message type, allocate Buffers for receiving  */
    /* the message.                                 */
    /***************************************************************************/
    switch (msg_hdr->msg_type)
    {
      case HM_MSG_TYPE_KEEPALIVE:
        TRACE_DETAIL(("Keepalive Message."));

        /***************************************************************************/
        /* Received Keepalive. Validate and Decrement Keepalive Ticks          */
        /* KEEPALIVE is just a header.                         */
        /***************************************************************************/
        if (sock_cb->tprt_cb->node_cb->keepalive_missed > 0)
        {
          sock_cb->tprt_cb->node_cb->keepalive_missed--;
        }

        sock_cb->tprt_cb->in_buffer = NULL;
        break;

      case HM_MSG_TYPE_REGISTER:
        TRACE_DETAIL(("Registration Message received."));
        msg_buf = hm_get_buffer(sizeof(HM_REGISTER_MSG));

        if (msg_buf == NULL)
        {
          TRACE_ERROR(("Error allocating buffers for Incoming Message."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }

        /***************************************************************************/
        /* Got Buffer. Set incoming buffer pointer to it and receive.         */
        /***************************************************************************/
        /* First copy the header from in_buffer                     */
        memcpy(msg_buf->msg, sock_cb->tprt_cb->in_buffer, sizeof(HM_MSG_HEADER));
        memset(&sock_cb->tprt_cb->header, 0, sizeof(sock_cb->tprt_cb->header));

        /* Now align the in_buffer pointer to it and receive message         */
        sock_cb->tprt_cb->in_buffer = (char *)((char *)msg_buf->msg + sizeof(
            HM_MSG_HEADER));
        size  = sizeof(HM_REGISTER_MSG) - sizeof(HM_MSG_HEADER);
        bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                            sock_cb->sock_type,
                                            (BYTE *)sock_cb->tprt_cb->in_buffer,
                                            size,
                                            &udp_sender
                                           );

        if (bytes_rcvd < size)
        {
          TRACE_WARN(("Bytes received (%d) less than expected %d",
                      bytes_rcvd, size));
          hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
          goto EXIT_LABEL;
        }

        if (hm_recv_register(msg_buf, sock_cb->tprt_cb) != HM_OK)
        {
          TRACE_ERROR(("Error occurred while REGISTER Processing."));
          TRACE_ASSERT(FALSE);
          goto EXIT_LABEL;
        }

        /* Cannot call free here as realloc may change pointers */
        //hm_free_buffer(msg_buf);
        break;

      case HM_MSG_TYPE_UNREGISTER:
        TRACE_DETAIL(("Received Unregister Message"));
        //TODO
        break;

      case HM_MSG_TYPE_PROCESS_CREATE:
        TRACE_DETAIL(("Received Process Creation Message"));
        msg_buf = hm_get_buffer(sizeof(HM_PROCESS_UPDATE_MSG));

        if (msg_buf == NULL)
        {
          TRACE_ERROR(("Error allocating buffers for Incoming Message."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }

        /***************************************************************************/
        /* Got Buffer. Set incoming buffer pointer to it and receive.         */
        /***************************************************************************/
        /* First copy the header from in_buffer                     */
        memcpy(msg_buf->msg, sock_cb->tprt_cb->in_buffer, sizeof(HM_MSG_HEADER));
        /* Now align the in_buffer pointer to it and receive message         */
        sock_cb->tprt_cb->in_buffer = (char *)((char *)msg_buf->msg + sizeof(
            HM_MSG_HEADER));
        size  = sizeof(HM_PROCESS_UPDATE_MSG) - sizeof(HM_MSG_HEADER);
        bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                            sock_cb->sock_type,
                                            (BYTE *)sock_cb->tprt_cb->in_buffer,
                                            size,
                                            &udp_sender);

        if (bytes_rcvd < size)
        {
          TRACE_WARN(("Bytes received (%d) less than expected %d",
                      bytes_rcvd, size));
          hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
          goto EXIT_LABEL;
        }

        if (hm_recv_proc_update(msg_buf, sock_cb->tprt_cb) != HM_OK)
        {
          TRACE_ERROR(("Error occurred while REGISTER Processing."));
          TRACE_ASSERT(FALSE);
          goto EXIT_LABEL;
        }

        hm_free_buffer(msg_buf);
        break;

      case HM_MSG_TYPE_PROCESS_DESTROY:
        TRACE_DETAIL(("Received Process Destruction Message"));
        //TODO
        break;

      case HM_MSG_TYPE_HA_UPDATE:
        TRACE_DETAIL(("Received HA Role Updates from user."));
        msg_buf = hm_get_buffer(sizeof(HM_PROCESS_UPDATE_MSG));

        if (msg_buf == NULL)
        {
          TRACE_ERROR(("Error allocating buffers for Incoming Message."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }

        /***************************************************************************/
        /* Got Buffer. Set incoming buffer pointer to it and receive.              */
        /***************************************************************************/
        memcpy(msg_buf->msg, sock_cb->tprt_cb->in_buffer, sizeof(HM_MSG_HEADER));
        sock_cb->tprt_cb->in_buffer = (char *)((char *)msg_buf->msg + sizeof(
            HM_MSG_HEADER));
        size  = sizeof(HM_HA_STATUS_UPDATE_MSG) - sizeof(HM_MSG_HEADER);
        bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                            sock_cb->sock_type,
                                            (BYTE *)sock_cb->tprt_cb->in_buffer,
                                            size,
                                            &udp_sender);

        if (bytes_rcvd < size)
        {
          TRACE_WARN(("Bytes received (%d) less than expected %d",
                      bytes_rcvd, size));
          hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
          goto EXIT_LABEL;
        }

        if (hm_recv_ha_update(msg_buf, sock_cb->tprt_cb) != HM_OK)
        {
          TRACE_ERROR(("Error occurred while HA Update Processing."));
          TRACE_ASSERT(FALSE);
          goto EXIT_LABEL;
        }

        hm_free_buffer(msg_buf);
        break;

      default:
        TRACE_WARN(("Unknown Message Type %d", msg_hdr->msg_type));
        TRACE_ASSERT(FALSE);
    }
  }//If message is from Node
  else
  {
    TRACE_DETAIL(("Received Message from a peer."));
    /***************************************************************************/
    /* Irrespective of TCP or UDP, we'll be receiving full message chunks only */
    /***************************************************************************/
    sock_cb->tprt_cb->in_bytes = hm_tprt_recv_on_socket(sock_cb->sock_fd,
                                 sock_cb->sock_type,
                                 (BYTE *)sock_cb->tprt_cb->in_buffer,
                                 sizeof(HM_PEER_MSG_UNION),
                                 &udp_sender
                                                       );

    if (sock_cb->tprt_cb->in_bytes < (int32_t) sizeof(HM_PEER_MSG_HEADER))
    {
      TRACE_DETAIL(("Message Length of at least %d was expected, %d was received",
                    sizeof(HM_PEER_MSG_HEADER), bytes_rcvd));
      hm_tprt_handle_improper_read(sock_cb->tprt_cb->in_bytes, sock_cb->tprt_cb);
      /***************************************************************************/
      /* Shh.. We've handled the network error. It is OK now.             */
      /***************************************************************************/
      ret_val = HM_OK;
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Process message received from cluster                   */
    /***************************************************************************/
    if (hm_receive_cluster_message(sock_cb) != HM_OK)
    {
      TRACE_ERROR(("Error occurred while handling cluster message."));
      ret_val = HM_ERR;
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_route_incoming_message */


/**
 *  @brief Handles the event of unsuccessful read on socket.
 *
 *  Evaluates the errno property in conjunction with bytes read to determine what to do.
 *
 *  @param bytes_rcvd Number of bytes received on the transport.
 *  @param *tprt_cb Transport Control Block (#HM_TRANSPORT_CB) on which error condition was raised.
 *  @return #HM_OK on successful handling of error condition, #HM_ERR otherwise
 */
int32_t hm_tprt_handle_improper_read(int32_t bytes_rcvd,
                                     HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(tprt_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if (bytes_rcvd == 0)
  {
    TRACE_WARN(("Remote side has disconnected."));
    tprt_cb->in_buffer = NULL;

    if (tprt_cb->node_cb != NULL)
    {
      hm_node_fsm(HM_NODE_FSM_TERM, tprt_cb->node_cb);
    }
    else
    {
      hm_peer_fsm(HM_PEER_FSM_CLOSE, tprt_cb->location_cb);
    }
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_tprt_handle_improper_read */

/**
 *  @brief Receives a REGISTER (#HM_REGISTER_MSG) message and routes it to Node or Process Layer
 *
 *  @param *msg #HM_MSG type buffer received from the transport layer
 *  @param *tprt_cb #HM_TRANSPORT_CB type of Transport Control Block on which message was received
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_recv_register(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_REGISTER_MSG *reg = NULL;
  int32_t ret_val = HM_OK, bytes_rcvd = 0;
  uint32_t i;

  int32_t msg_size = sizeof(HM_REGISTER_MSG);

  void *subscriber = NULL;

  HM_PROCESS_CB *proc_cb = NULL;

  SOCKADDR *udp_sender;

  HM_REGISTER_TLV_CB *tlv = NULL;
  int32_t num_binding = 0; /* Number of bindings to propagate to cluster. */
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(tprt_cb != NULL);
  TRACE_ASSERT(msg != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  reg = (HM_REGISTER_MSG *)msg->msg;

  if (reg->subscriber_pid != 0)
  {
    /***************************************************************************/
    /* PID has been provided. Pass to Process Management.                      */
    /* For now, even if a process is subscribing to notifications, they will be*/
    /* dispatched on a nodal level. This is because more than one processes on */
    /* a node may subscribe to the same notifications and in such a case rather*/
    /* than sending multiple notifications from HM itself, the distribution of */
    /* a single notification into multiple is done at the HM-Stub end.         */
    /* Thus, PID becomes irrelevant right now.                                 */
    /* It can however be changed easily by calling hm_subscribe on the subscri-*/
    /*-ption entity of the process rather than the node.                       */
    /***************************************************************************/
    TRACE_DETAIL(("Subscriber PID: 0x%x", reg->subscriber_pid));

    /* Find subscribing process */
    for (proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIRST(tprt_cb->node_cb->process_tree,
                   node_process_tree_by_proc_id);
         proc_cb != NULL;
         proc_cb = (HM_PROCESS_CB *)HM_AVL3_NEXT(proc_cb->node,
                   node_process_tree_by_proc_id))
    {
      if (proc_cb->pid == reg->subscriber_pid)
      {
        TRACE_DETAIL(("Found process."));
        subscriber = (void *)proc_cb;
        break;
      }
    }

    TRACE_ASSERT(proc_cb != NULL);

    if (proc_cb == NULL)
    {
      TRACE_WARN(("Process %x not found on Node.", reg->subscriber_pid));
      /* Register to Node instead. In release only. */
      subscriber = (void *)tprt_cb->node_cb;
    }
  }
  else
  {
    subscriber = (void *)tprt_cb->node_cb;
  }

  if (reg->num_register == 0)
  {
    TRACE_WARN(("Number of register TLVs value is malformed. Reject Register"));
    reg->hdr.response_ok = FALSE;
    reg->hdr.request = FALSE;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Allocate Memory to receive the rest of Register TLVs             */
  /***************************************************************************/
  TRACE_DETAIL(("Need extra memory for %u registers.[%u/block]",
                reg->num_register , sizeof(HM_REGISTER_TLV_CB)));

  /* -1 is to account for the 1 uint_32 already in the header to mark start of data */
  if ((int32_t)(msg_size + ((reg->num_register) * sizeof(HM_REGISTER_TLV_CB))
                - 1) > msg_size)
  {
    msg_size = msg_size + ((reg->num_register) * sizeof(HM_REGISTER_TLV_CB)) - 1;
    msg = hm_grow_buffer(msg, msg_size);

    if (msg == NULL)
    {
      TRACE_ERROR(("Error allocating buffers for Incoming Message."));
      ret_val = HM_ERR;
      /* THIS is a terminal error. ABORT! ABORT! ABORT!*/
      TRACE_ASSERT(FALSE);
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Got Buffer. Set incoming buffer pointer to it and receive.         */
    /***************************************************************************/
    reg = (HM_REGISTER_MSG *)msg->msg;
    tprt_cb->in_buffer = (char *)msg->msg + sizeof(HM_REGISTER_MSG);
    bytes_rcvd = hm_tprt_recv_on_socket(tprt_cb->sock_cb->sock_fd,
                                        tprt_cb->sock_cb->sock_type,
                                        (uint8_t *)tprt_cb->in_buffer,
                                        ((reg->num_register) * sizeof(HM_REGISTER_TLV_CB)) - 1,
                                        &udp_sender);

    if (bytes_rcvd < (int32_t)((reg->num_register * sizeof(HM_REGISTER_TLV_CB))
                               - 1))
    {
      TRACE_WARN(("Bytes received (%d) less than expected %u",
                  bytes_rcvd, (reg->num_register * sizeof(HM_REGISTER_TLV_CB)) - 1));
      hm_tprt_handle_improper_read(bytes_rcvd, tprt_cb);
      goto EXIT_LABEL;
    }
  }

  /***************************************************************************/
  /* Hold transport from sending out any subscription notifications before   */
  /* response.                                                               */
  /***************************************************************************/
  TRACE_INFO(("Acquire Transport Lock!"));
  tprt_cb->hold = TRUE;

  /***************************************************************************/
  /* Have TLVs. Handle Subscription.                                         */
  /***************************************************************************/
  for (i = 0; i < reg->num_register; i++)
  {
    tlv = (HM_REGISTER_TLV_CB *)reg->data + i;
    TRACE_INFO(("Subscribe to 0X%x", tlv->id));

    if (hm_subscribe(reg->type, tlv->id, subscriber, tlv->cross_bind) != HM_OK)
    {
      TRACE_ERROR(("Error creating subscriptions."));
      ret_val = HM_ERR;
      /* Now how do we selectively tell that this particular subscription failed? */
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Also propagate update to cluster if it is a cross binding               */
    /***************************************************************************/
    if (tlv->cross_bind)
    {
      TRACE_DETAIL(("Bidirectional subscriber. Propagate binding to cluster."));
      /* First pack the updates together. */
      num_binding++;
    }
  }

  /***************************************************************************/
  /* Successful subscriptions! Return Register with OK Response         */
  /***************************************************************************/
  reg->hdr.response_ok = TRUE;
  reg->hdr.request = FALSE;

  /***************************************************************************/
  /* Setup message to send to cluster                                        */
  /***************************************************************************/
  if (num_binding > 0)
  {
    hm_cluster_exchange_binding((void *)subscriber, num_binding, reg);
  }

EXIT_LABEL:

  if (ret_val == HM_OK)
  {
    TRACE_DETAIL(("Send Response"));
    ret_val = hm_queue_on_transport(msg, tprt_cb, TRUE);
    tprt_cb->in_buffer = NULL;
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_recv_register */


/**
 *  @brief Receives update on Process Creation/Destruction
 *
 *  @param *msg #HM_MSG type of message buffer on which #HM_PROCESS_UPDATE_MSG was received
 *  @param &tprt_cb #HM_TRANSPOR_CB structure of the transport on which message was received
 *  @return #HM_OK on success, #HM_ERR on failure.
 */
int32_t hm_recv_proc_update(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_PROCESS_UPDATE_MSG *proc_msg = NULL;
  int32_t ret_val = HM_OK;

  int32_t key[2];

  HM_PROCESS_CB *proc_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(msg != NULL);
  TRACE_ASSERT(tprt_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  proc_msg = msg->msg;

  if (proc_msg->proc_type == 0)
  {
    TRACE_WARN(("Invalid Process Type"));
    proc_msg->hdr.response_ok = FALSE;
    proc_msg->hdr.request = FALSE;
    goto EXIT_LABEL;
  }

  if (proc_msg->pid == 0)
  {
    TRACE_WARN(("Invalid Process ID"));
    proc_msg->hdr.response_ok = FALSE;
    proc_msg->hdr.request = FALSE;
    goto EXIT_LABEL;
  }

  if (proc_msg->hdr.msg_type == HM_MSG_TYPE_PROCESS_CREATE)
  {
    TRACE_INFO(("Process 0x%x Created", proc_msg->pid));
    /***************************************************************************/
    /* Allocate a Process Control Block                       */
    /***************************************************************************/
    proc_cb = hm_alloc_process_cb();

    if (proc_cb == NULL)
    {
      TRACE_ERROR(("Error allocating memory for Process CBs"));
      /***************************************************************************/
      /* Maybe, try again later?                           */
      /***************************************************************************/
      proc_msg->hdr.response_ok = FALSE;
      proc_msg->hdr.request = FALSE;
      goto EXIT_LABEL;
    }

    proc_cb->parent_node_cb = tprt_cb->node_cb;
    proc_cb->pid = proc_msg->pid;
    proc_cb->type = proc_msg->proc_type;
    proc_cb->running = TRUE;
    snprintf((char *)proc_cb->name, sizeof(proc_cb->name), "%s", proc_msg->name);

    if (hm_process_add(proc_cb, proc_cb->parent_node_cb) != HM_OK)
    {
      TRACE_ERROR(("Error occurred while adding process info to HM."));
      proc_msg->hdr.response_ok = FALSE;
      proc_msg->hdr.request = FALSE;
      hm_free_process_cb(proc_cb);
      proc_cb = NULL;
      goto EXIT_LABEL;
    }
  }
  else if (proc_msg->hdr.msg_type == HM_MSG_TYPE_PROCESS_DESTROY)
  {
    TRACE_INFO(("Process Destroyed"));
    key[0] = proc_msg->proc_type;
    key[2] = proc_msg->pid;

    proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIND(tprt_cb->node_cb->process_tree,
                                            &key,
                                            node_process_tree_by_proc_type_and_pid);

    if (proc_cb == NULL)
    {
      TRACE_ERROR(("Process CB not found!"));
      TRACE_ASSERT(FALSE);
      proc_msg->hdr.response_ok = FALSE;
      proc_msg->hdr.request = FALSE;
      goto EXIT_LABEL;
    }

    TRACE_DETAIL(("Found Process CB. Update status to down."));
    proc_cb->running = FALSE;
    hm_process_update(proc_cb);
  }

  /***************************************************************************/
  /* Everything went fine. Send a proper response.               */
  /***************************************************************************/
  proc_msg->hdr.response_ok = TRUE;
  proc_msg->hdr.request = FALSE;

EXIT_LABEL:

  if (ret_val == HM_OK)
  {
    TRACE_DETAIL(("Send Response"));
    ret_val = hm_queue_on_transport(msg, tprt_cb, TRUE);
    tprt_cb->in_buffer = NULL;
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_recv_proc_update */



/**
 *  @brief Receives the message header into the buffer provided
 *
 *  @param *buf Message Byte Buffer into which the incoming message header is to be written
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_receive_msg_hdr(char *buf)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(buf != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_receive_msg_hdr */


/**
 *  @brief Sends a response to incoming INIT request
 *
 *  @param *node_cb #HM_NODE_CB structure for which #HM_NODE_INIT_MSG was received
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_node_send_init_rsp(HM_NODE_CB *node_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;

  HM_MSG *msg = NULL;
  HM_NODE_INIT_MSG *init_msg = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(node_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  msg = (HM_MSG *)node_cb->transport_cb->in_buffer;

  TRACE_ASSERT(msg != NULL);
  /***************************************************************************/
  /* We are using this message and will free it. Increment ref_count       */
  /***************************************************************************/
  msg->ref_count++;
  init_msg = (HM_NODE_INIT_MSG *) msg->msg;
  TRACE_ASSERT(init_msg != NULL);
  /***************************************************************************/
  /* Set the message as INIT Response                       */
  /***************************************************************************/
  init_msg->hdr.request = FALSE;
  init_msg->hdr.response_ok = TRUE;

  /***************************************************************************/
  /* Set the Hardware Location Number of the Location                        */
  /***************************************************************************/
  init_msg->hardware_num = LOCAL.local_location_cb.index;
  TRACE_INFO(("Hardware index: %d", init_msg->hardware_num));
  init_msg->location_status = node_cb->current_role;

  init_msg->keepalive_period = node_cb->keepalive_period;

  /***************************************************************************/
  /* Made message, now queue on transport and send if possible               */
  /***************************************************************************/
  ret_val = hm_queue_on_transport(msg, node_cb->transport_cb, TRUE);
  node_cb->transport_cb->in_buffer = NULL;


  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();

  return (ret_val);

}/* hm_node_send_init_rsp */


/**
 *  @brief Queue the message on the given transport for sending.
 *
 *  @param *msg Message to be sent (of type #HM_MSG)
 *  @param *tprt_cb Transport CB (#HM_TRANSPORT_CB) to which message is to be sent
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_queue_on_transport(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb,
                              uint32_t priority)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_LIST_BLOCK *block = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(msg != NULL);
  TRACE_ASSERT(tprt_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Message created. Now, add it to outgoing queue and try to send       */
  /***************************************************************************/
  block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));

  if (block == NULL)
  {
    TRACE_ASSERT(FALSE);
    TRACE_ERROR(("Error allocating memory for Register response queuing!"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  HM_INIT_LQE(block->node, block);

  block->target = msg;
  msg->ref_count++;

  /***************************************************************************/
  /* INIT Response should be the first thing that is sent on this queue. So, */
  /* add it to the head, not the tail.                       */
  /***************************************************************************/
  if (priority)
  {
    HM_INSERT_AFTER(tprt_cb->pending, block->node);
    TRACE_INFO(("Release Transport Lock!"));
    tprt_cb->hold = FALSE;
  }
  else
  {
    HM_INSERT_BEFORE(tprt_cb->pending, block->node);
  }

  tprt_cb->in_buffer = NULL;

  /***************************************************************************/
  /* Try to send                                 */
  /***************************************************************************/
  ret_val = hm_tprt_process_outgoing_queue(tprt_cb);

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_queue_on_transport */


/**
 *  @brief Tries to send all pending messages on the outgoing queue
 *
 *  @param *tprt_cb Transport CB (#HM_TRANSPORT_CB) whose pending outgoing messages need to be processed.
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_tprt_process_outgoing_queue(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_MSG *msg = NULL;

  int32_t ret_val = HM_OK;

  HM_LIST_BLOCK *block = NULL, *temp = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(tprt_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if (tprt_cb->sock_cb == NULL)
  {
    TRACE_DETAIL(("No connection exists yet. Keep in pending buffers"));
    ret_val = HM_OK;
    goto EXIT_LABEL;
  }

  if (tprt_cb->hold == TRUE)
  {
    TRACE_DETAIL(("Message sending locked on transport. Keep in pending buffers."));
    ret_val = HM_OK;
    goto EXIT_LABEL;
  }

  for (block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(tprt_cb->pending);
       block != NULL;)
  {
    msg = (HM_MSG *)block->target;

    /***************************************************************************/
    /* For TCP, this is a no-brainer. We do not actually need to send the SOCK-*/
    /* -ADDR structure. For UDP and Multicast, we do.               */
    /* Even in case of Multicast, it isn't problematic, since the sender addr  */
    /* always stays the same.                           */
    /* For UDP, however, the case is different. Because, we can use the same   */
    /* port and socket for communicating with different hosts. Thus the address*/
    /* passed to the lower API must be of destination.                */
    /* But considering that Unicast UDP is meant only for Peers, each peer will*/
    /* have a separate TRANSPORT_CB and consequently, the address will be diff-*/
    /*-erent. The reverse-relation from sock_cb to tprt_cb will be obviously   */
    /* wrong should someone try to use it in case of UDP, since it would point */
    /* to the Transport of Local Location. But forward path stays correct.     */
    /***************************************************************************/
    if (hm_tprt_send_on_socket((SOCKADDR *)&tprt_cb->address.address,
                               tprt_cb->sock_cb->sock_fd, tprt_cb->type,
                               msg->msg, msg->msg_len) == HM_ERR)
    {
      TRACE_ERROR(("Could not send on socket."));
    }

    /***************************************************************************/
    /* Remove from list.                             */
    /***************************************************************************/
    temp = block;
    block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(block->node);
    HM_REMOVE_FROM_LIST(temp->node);
    hm_free_buffer(msg);
    free(temp);
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_tprt_process_outgoing_queue */
