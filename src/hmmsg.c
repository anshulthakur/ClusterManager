/*
 * hmmsg.c
 *
 *  Created on: 09-Jun-2015
 *      Author: anshul
 */

#include <hmincl.h>
/***************************************************************************/
/* Name:	hm_recv_register 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Receives a REGISTER message and routes it to Node or		   */
/* Process Layer.														   */
/***************************************************************************/
int32_t hm_recv_register(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_REGISTER_MSG *reg = NULL;
	int32_t ret_val = HM_OK, bytes_rcvd = 0, i;
	HM_LIST_BLOCK *block = NULL;
	int32_t msg_size = sizeof(HM_REGISTER_MSG);

	HM_REGISTER_TLV_CB *tlv = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(tprt_cb != NULL);
	TRACE_ASSERT(msg != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	reg = (HM_REGISTER_MSG *)msg->msg;

	if(reg->subscriber_pid != 0)
	{
		/***************************************************************************/
		/* PID has been provided. Pass to Process Management.					   */
		/* For now, even if a process is subscribing to notifications, they will be*/
		/* dispatched on a nodal level. This is because more than one processes on */
		/* a node may subscribe to the same notifications and in such a case rather*/
		/* than sending multiple notifications from HM itself, the distribution of */
		/* a single notification into multiple is done at the HM-Stub end.		   */
		/* Thus, PID becomes irrelevant right now. 								   */
		/* It can however be changed easily by calling hm_subscribe on the subscri-*/
		/*-ption entity of the process rather than the node.					   */
		/***************************************************************************/
		TRACE_DETAIL(("Subscriber PID: 0x%x", reg->subscriber_pid));
	}
	if(reg->num_register ==0)
	{
		TRACE_WARN(("Number of register TLVs value is malformed. Reject Register"));
		reg->hdr.response_ok = FALSE;
		reg->hdr.request = FALSE;
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Allocate Memory to receive the rest of Register TLVs					   */
	/***************************************************************************/
	TRACE_DETAIL(("Need extra memory for %d registers.[%d/block]",
								reg->num_register,sizeof(HM_REGISTER_TLV_CB) ));

	msg_size = msg_size + (reg->num_register * sizeof(HM_REGISTER_TLV_CB));
	msg = hm_grow_buffer(msg, msg_size);
	if(msg == NULL)
	{
		TRACE_ERROR(("Error allocating buffers for Incoming Message."));
		ret_val = HM_ERR;
		/* THIS is a terminal error. ABORT! ABORT! ABORT!*/
		TRACE_ASSERT(FALSE);
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Got Buffer. Set incoming buffer pointer to it and receive.			   */
	/***************************************************************************/
	reg = (HM_REGISTER_MSG *)msg->msg;
	tprt_cb->in_buffer = (char *)msg->msg+ sizeof(HM_REGISTER_MSG);
	bytes_rcvd = hm_tprt_recv_on_socket(tprt_cb->sock_cb->sock_fd,
										HM_TRANSPORT_TCP_IN,
										tprt_cb->in_buffer,
										(reg->num_register * sizeof(HM_REGISTER_TLV_CB))
										);
	if(bytes_rcvd < (reg->num_register + sizeof(HM_REGISTER_TLV_CB)))
	{
		TRACE_WARN(("Bytes received (%d) less than expected %d",
								bytes_rcvd, sizeof(HM_REGISTER_MSG)));
		hm_tprt_handle_improper_read(bytes_rcvd, tprt_cb);
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Have TLVs. Handle Subscription.										   */
	/***************************************************************************/
	for(i=0; i< reg->num_register; i++)
	{
		tlv = (HM_REGISTER_TLV_CB *)tprt_cb->in_buffer + i;
		if(hm_subscribe(tlv->type, tlv->id, (void *)tprt_cb->node_cb) != HM_OK)
		{
			TRACE_ERROR(("Error creating subscriptions."));
			ret_val = HM_ERR;
			/* Now how do we selectively tell that this particular subscription failed? */
			goto EXIT_LABEL;
		}
	}

	/***************************************************************************/
	/* Successful subscriptions! Return Register with OK Response			   */
	/***************************************************************************/
	reg->hdr.response_ok = TRUE;
	reg->hdr.request = FALSE;

EXIT_LABEL:
	if(ret_val == HM_OK)
	{
		TRACE_DETAIL(("Send Response"));
		/***************************************************************************/
		/* Message created. Now, add it to outgoing queue and try to send		   */
		/***************************************************************************/
		block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
		if(block == NULL)
		{
			TRACE_ASSERT(FALSE);
			TRACE_ERROR(("Error allocating memory for Register response queuing!"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		HM_INIT_LQE(block->node, block);

		block->target = msg;
		/***************************************************************************/
		/* INIT Response should be the first thing that is sent on this queue. So, */
		/* add it to the head, not the tail.								       */
		/***************************************************************************/
		HM_INSERT_BEFORE(tprt_cb->pending, block->node);
		tprt_cb->in_buffer = NULL;

		ret_val = hm_tprt_process_outgoing_queue(tprt_cb);
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_recv_register */

/***************************************************************************/
/* Name:	hm_recv_proc_update 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Receives update on Process Creation/Destruction			*/
/***************************************************************************/
int32_t hm_recv_proc_update(HM_MSG *msg, HM_TRANSPORT_CB *tprt_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_LIST_BLOCK *block = NULL;
	HM_PROCESS_UPDATE_MSG *proc_msg = NULL;
	int32_t ret_val = HM_OK, i=0;

	int32_t key[2];

	HM_PROCESS_CB *proc_cb = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(msg != NULL);
	TRACE_ASSERT(tprt_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	proc_msg = msg->msg;

	if(proc_msg->proc_type == 0)
	{
		TRACE_WARN(("Invalid Process Type"));
		proc_msg->hdr.response_ok = FALSE;
		proc_msg->hdr.request = FALSE;
		goto EXIT_LABEL;
	}
	if(proc_msg->pid == 0)
	{
		TRACE_WARN(("Invalid Process ID"));
		proc_msg->hdr.response_ok = FALSE;
		proc_msg->hdr.request = FALSE;
		goto EXIT_LABEL;
	}

	if(proc_msg->hdr.msg_type == HM_MSG_TYPE_PROCESS_CREATE)
	{
		TRACE_INFO(("Process 0x%x Created", proc_msg->pid));
		/***************************************************************************/
		/* Allocate a Process Control Block										   */
		/***************************************************************************/
		proc_cb = hm_alloc_process_cb();
		if(proc_cb == NULL)
		{
			TRACE_ERROR(("Error allocating memory for Process CBs"));
			/***************************************************************************/
			/* Maybe, try again later?												   */
			/***************************************************************************/
			proc_msg->hdr.response_ok = FALSE;
			proc_msg->hdr.request = FALSE;
			goto EXIT_LABEL;
		}

		proc_cb->parent_node_cb = tprt_cb->node_cb;
		proc_cb->pid = proc_msg->pid;
		proc_cb->type = proc_msg->proc_type;
		proc_cb->running = TRUE;
		snprintf(proc_cb->name, sizeof(proc_cb->name),"%s",proc_msg->name);

		if(hm_process_add(proc_cb, proc_cb->parent_node_cb)!= HM_OK)
		{
			TRACE_ERROR(("Error occurred while adding process info to HM."));
			proc_msg->hdr.response_ok = FALSE;
			proc_msg->hdr.request = FALSE;
			hm_free_process_cb(proc_cb);
			proc_cb = NULL;
			goto EXIT_LABEL;
		}
	}
	else if(proc_msg->hdr.msg_type == HM_MSG_TYPE_PROCESS_DESTROY)
	{
		TRACE_INFO(("Process Destroyed"));
		key[0] = proc_msg->proc_type;
		key[2] = proc_msg->pid;

		proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIND(tprt_cb->node_cb->process_tree,
												&key,
										node_process_tree_by_proc_type_and_pid);
		if(proc_cb == NULL)
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
	/* Everything went fine. Send a proper response.						   */
	/***************************************************************************/
	proc_msg->hdr.response_ok = TRUE;
	proc_msg->hdr.request = FALSE;

EXIT_LABEL:
	if(ret_val == HM_OK)
	{
		TRACE_DETAIL(("Send Response"));
		/***************************************************************************/
		/* Message created. Now, add it to outgoing queue and try to send		   */
		/***************************************************************************/
		block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
		if(block == NULL)
		{
			TRACE_ASSERT(FALSE);
			TRACE_ERROR(("Error allocating memory for Register response queuing!"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		HM_INIT_LQE(block->node, block);

		block->target = msg;
		/***************************************************************************/
		/* INIT Response should be the first thing that is sent on this queue. So, */
		/* add it to the head, not the tail.								       */
		/***************************************************************************/
		HM_INSERT_BEFORE(tprt_cb->pending, block->node);
		tprt_cb->in_buffer = NULL;

		ret_val = hm_tprt_process_outgoing_queue(tprt_cb);
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
}/* hm_recv_proc_update */

/***************************************************************************/
/* Name:	hm_route_incoming_message 									   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t													 	   */
/* Purpose: Determines where to send the incoming message. It assumes that */
/* a message header has been received in the in_buffer. 				   */
/***************************************************************************/
int32_t hm_route_incoming_message(HM_SOCKET_CB *sock_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG_HEADER *msg_hdr = NULL;
	int32_t bytes_rcvd = 0;
	HM_MSG *msg_buf = NULL;
	int32_t ret_val = HM_OK;
	int32_t size = 0;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(sock_cb != NULL);
	TRACE_ASSERT(sock_cb->tprt_cb->in_buffer != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	msg_hdr = sock_cb->tprt_cb->in_buffer;

	/***************************************************************************/
	/* Depending on the incoming message type, allocate Buffers for receiving  */
	/* the message.															   */
	/***************************************************************************/
	switch(msg_hdr->msg_type)
	{
	case HM_MSG_TYPE_KEEPALIVE:
		TRACE_DETAIL(("Keepalive Message."));
		/***************************************************************************/
		/* Received Keepalive. Validate and Decrement Keepalive Ticks 			   */
		/* KEEPALIVE is just a header.											   */
		/***************************************************************************/
		sock_cb->tprt_cb->node_cb->keepalive_missed--;
		sock_cb->tprt_cb->in_buffer = NULL;
		break;

	case HM_MSG_TYPE_REGISTER:
		TRACE_DETAIL(("Registration Message received."));
		msg_buf = hm_get_buffer(sizeof(HM_REGISTER_MSG));
		if(msg_buf == NULL)
		{
			TRACE_ERROR(("Error allocating buffers for Incoming Message."));
			ret_val =HM_ERR;
			goto EXIT_LABEL;
		}
		/***************************************************************************/
		/* Got Buffer. Set incoming buffer pointer to it and receive.			   */
		/***************************************************************************/
		/* First copy the header from in_buffer									   */
		memcpy(msg_buf->msg, sock_cb->tprt_cb->in_buffer, sizeof(HM_MSG_HEADER));
		/* Now align the in_buffer pointer to it and receive message			   */
		sock_cb->tprt_cb->in_buffer = (char *)((char *)msg_buf->msg + sizeof(HM_MSG_HEADER));
		size  = sizeof(HM_REGISTER_MSG)-sizeof(HM_MSG_HEADER);
		bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
											HM_TRANSPORT_TCP_IN,
											sock_cb->tprt_cb->in_buffer,
											size
											);
		if(bytes_rcvd < size)
		{
			TRACE_WARN(("Bytes received (%d) less than expected %d",
									bytes_rcvd, size));
			hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
			goto EXIT_LABEL;
		}
		if(hm_recv_register(msg_buf, sock_cb->tprt_cb)!= HM_OK)
		{
			TRACE_ERROR(("Error occurred while REGISTER Processing."));
			TRACE_ASSERT(FALSE);
			goto EXIT_LABEL;
		}
		break;

	case HM_MSG_TYPE_UNREGISTER:
		TRACE_DETAIL(("Received Unregister Message"));
		//TODO
		break;

	case HM_MSG_TYPE_PROCESS_CREATE:
		TRACE_DETAIL(("Received Process Creation Message"));
		msg_buf = hm_get_buffer(sizeof(HM_PROCESS_UPDATE_MSG));
		if(msg_buf == NULL)
		{
			TRACE_ERROR(("Error allocating buffers for Incoming Message."));
			ret_val =HM_ERR;
			goto EXIT_LABEL;
		}
		/***************************************************************************/
		/* Got Buffer. Set incoming buffer pointer to it and receive.			   */
		/***************************************************************************/
		/* First copy the header from in_buffer									   */
		memcpy(msg_buf->msg, sock_cb->tprt_cb->in_buffer, sizeof(HM_MSG_HEADER));
		/* Now align the in_buffer pointer to it and receive message			   */
		sock_cb->tprt_cb->in_buffer = (char *)((char *)msg_buf->msg + sizeof(HM_MSG_HEADER));
		size  = sizeof(HM_PROCESS_UPDATE_MSG)-sizeof(HM_MSG_HEADER);
		bytes_rcvd = hm_tprt_recv_on_socket(sock_cb->sock_fd,
											HM_TRANSPORT_TCP_IN,
											sock_cb->tprt_cb->in_buffer,
											size
											);
		if(bytes_rcvd < size)
		{
			TRACE_WARN(("Bytes received (%d) less than expected %d",
									bytes_rcvd, size));
			hm_tprt_handle_improper_read(bytes_rcvd, sock_cb->tprt_cb);
			goto EXIT_LABEL;
		}
		if(hm_recv_proc_update(msg_buf, sock_cb->tprt_cb)!= HM_OK)
		{
			TRACE_ERROR(("Error occurred while REGISTER Processing."));
			TRACE_ASSERT(FALSE);
			goto EXIT_LABEL;
		}
		break;
	case HM_MSG_TYPE_PROCESS_DESTROY:
		TRACE_DETAIL(("Received Process Destruction Message"));
		//TODO
		break;
	default:
		TRACE_WARN(("Unknown Message Type"));
		TRACE_ASSERT(FALSE);
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_route_incoming_message */

/***************************************************************************/
/* Name:	hm_tprt_handle_improper_read 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Handles the event of unsuccessful read on socket. Evaluates the*/
/* errno property in conjunction with bytes read to determine what to do.  */
/***************************************************************************/
int32_t hm_tprt_handle_improper_read(int32_t bytes_rcvd, HM_TRANSPORT_CB *tprt_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(tprt_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(bytes_rcvd == 0)
	{
		TRACE_WARN(("Remote Peer has disconnected."));
		tprt_cb->in_buffer = NULL;
		hm_node_fsm(HM_NODE_FSM_TERM, tprt_cb->node_cb);
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
}/* hm_tprt_handle_improper_read */

/***************************************************************************/
/* Name:	hm_receive_msg_hdr 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Receives the message header into the buffer provided		   */
/***************************************************************************/
int32_t hm_receive_msg_hdr(char *buf)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(buf != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_receive_msg_hdr */

/***************************************************************************/
/* Name:	hm_receive_msg 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Receives the message in the given buffer					   */
/***************************************************************************/
int32_t hm_receive_msg(char *buf)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(buf != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_receive_msg */

/***************************************************************************/
/* Name:	hm_node_send_init_rsp 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Sends a response to incoming INIT request			*/
/***************************************************************************/
int32_t hm_node_send_init_rsp(HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;

	HM_MSG *msg = NULL;
	HM_NODE_INIT_MSG *init_msg = NULL;

	HM_LIST_BLOCK *block = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(node_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	msg = (HM_MSG *)node_cb->transport_cb->in_buffer;

	TRACE_ASSERT(msg != NULL);
	/***************************************************************************/
	/* We are using this message and will free it. Increment ref_count		   */
	/***************************************************************************/
	msg->ref_count++;
	init_msg = (HM_NODE_INIT_MSG *) msg->msg;
	TRACE_ASSERT(init_msg != NULL);
	/***************************************************************************/
	/* Set the message as INIT Response										   */
	/***************************************************************************/
	init_msg->hdr.request = FALSE;
	init_msg->hdr.response_ok = TRUE;

	/***************************************************************************/
	/* Set the Hardware Location Number of the Location						   */
	/***************************************************************************/
	init_msg->hardware_num = LOCAL.local_location_cb.index;
	init_msg->location_status = TRUE;

	init_msg->keepalive_period = node_cb->keepalive_period;

	/***************************************************************************/
	/* Message created. Now, add it to outgoing queue and try to send		   */
	/***************************************************************************/
	block = (HM_LIST_BLOCK *)malloc(sizeof(HM_LIST_BLOCK));
	if(block == NULL)
	{
		TRACE_ASSERT(FALSE);
		TRACE_ERROR(("Error allocating memory for INIT response queuing!"));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	HM_INIT_LQE(block->node, block);

	block->target = msg;
	/***************************************************************************/
	/* INIT Response should be the first thing that is sent on this queue. So, */
	/* add it to the head, not the tail.								       */
	/***************************************************************************/
	HM_INSERT_AFTER(node_cb->transport_cb->pending, block->node);
	node_cb->transport_cb->in_buffer = NULL;

	ret_val = hm_tprt_process_outgoing_queue(node_cb->transport_cb);
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();

	return(ret_val);

}/* hm_node_send_init_rsp */


/***************************************************************************/
/* Name:	hm_tprt_process_outgoing_queue 								   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Tries to send all pending messages on the outgoing queue	   */
/***************************************************************************/
int32_t hm_tprt_process_outgoing_queue(HM_TRANSPORT_CB *tprt_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG *msg = NULL;

	int32_t ret_val = HM_OK;

	HM_LIST_BLOCK *block = NULL, *temp= NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(tprt_cb);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(tprt_cb->sock_cb == NULL)
	{
		TRACE_DETAIL(("No connection exists yet. Keep in pending buffers"));
		ret_val = HM_OK;
		goto EXIT_LABEL;
	}
	for(block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(tprt_cb->pending);
			block != NULL;)
	{
		msg = (HM_MSG *)block->target;
		if(hm_tprt_send_on_socket(NULL, tprt_cb->sock_cb->sock_fd, tprt_cb->type,
				msg->msg, msg->msg_len) == HM_ERR)
		{
			TRACE_ERROR(("Could not send on socket."));
		}
		/***************************************************************************/
		/* Remove from list.													   */
		/***************************************************************************/
		temp = block;
		block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(block->node);
		HM_REMOVE_FROM_LIST(temp->node);
		hm_free_buffer(msg);
		free(temp);
	}
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_tprt_process_outgoing_queue */
