/*
 * hmmsg.c
 *
 *  Created on: 09-Jun-2015
 *      Author: anshul
 */

#include <hmincl.h>

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
