/*
 * hmnotify.c
 *
 *  Created on: 08-Jun-2015
 *      Author: anshul
 */

#include <hmincl.h>

/***************************************************************************/
/* Name:	hm_service_notify_queue 									   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Services the notifications queued in the Notifications Queue   */
/* NOTE:																   */
/* This routine may be driven by a conditionally timed semaphore such that */
/* every time a notification is added, a consumer thread is invoked.	   */
/***************************************************************************/
int32_t hm_service_notify_queue()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_NOTIFICATION_CB *notify_cb = NULL, *temp = NULL;
	HM_SUBSCRIBER affected_node, subscriber;
	HM_MSG *msg = NULL;
	int32_t ret_val = HM_OK;
	HM_LIST_BLOCK *list_member = NULL, *block = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	/***************************************************************************/
	/* Main Routine															   */
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
			/* Have message, update reference count									   */
			/***************************************************************************/
			notify_cb->ref_count = affected_node.node_cb->sub_cb->num_subscribers;
			/***************************************************************************/
			/* We are ready to send this message to various subscribers				   */
			/***************************************************************************/
			for(list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(
									affected_node.node_cb->sub_cb->subscribers_list);
					list_member != NULL;
				list_member = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(list_member->node))
			{

				//TODO
				/***************************************************************************/
				/* For Now, I am just assuming that only nodes subscribe to nodes and hence*/
				/* will be notified.													   */
				/* Later, this must be improved (by using switch case on the table_type    */
				/* field, to allow location to node, or node to locations subs.			   */
				/***************************************************************************/
				subscriber.node_cb = (HM_GLOBAL_NODE_CB *)list_member->target;

				TRACE_ASSERT(subscriber.node_cb != NULL);
				TRACE_ASSERT(subscriber.node_cb->node_cb != NULL);

				/***************************************************************************/
				/* Append the message to the outgoing message queue on the transport of the*/
				/* node.																   */
				/***************************************************************************/
				if(subscriber.node_cb->node_cb->transport_cb != NULL)
				{
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

					HM_INSERT_BEFORE(subscriber.node_cb->node_cb->transport_cb->pending, block->node);
					msg->ref_count++; /* This is more important */
					hm_tprt_process_outgoing_queue(subscriber.node_cb->node_cb->transport_cb);
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
			/* Have message, update reference count									   */
			/***************************************************************************/
			notify_cb->ref_count = affected_node.node_cb->sub_cb->num_subscribers;
			msg->ref_count = notify_cb->ref_count; /* This is more important */

			/***************************************************************************/
			/* We are ready to send this message to various subscribers				   */
			/***************************************************************************/

			break;

		case HM_NOTIFICATION_PROCESS_CREATED:
			TRACE_INFO(("Process Created"));
			break;
		case HM_NOTIFICATION_PROCESS_DESTROYED:
			TRACE_INFO(("Process Destroyed"));
			break;
		case HM_NOTIFICATION_INTERFACE_ADDED:
			TRACE_INFO(("Interface Added"));
			break;
		case HM_NOTIFICATION_INTERFACE_DELETE:
			TRACE_INFO(("Interface Deleted"));
			break;
		default:
			TRACE_WARN(("Unknown Notification Type"));
			TRACE_ASSERT(FALSE);
		}
		/***************************************************************************/
		/* This notification has either been processed according to the notifier.  */
		/* it may happen that it is pending in some transport, but Notify CB can be*/
		/* freed now.															   */
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
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_service_notify_queue */

/***************************************************************************/
/* Name:	hm_build_notify_message 									   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	HM_MSG *													   */
/* Purpose: Builds a notification Message that must be sent to various 	   */
/* subscribers of the subscription point. The message stays the same so    */
/* we will store it in the opaque section of the notification CB until it  */
/* has been transmitted to all the subscribers.							   */
/***************************************************************************/
HM_MSG * hm_build_notify_message(HM_NOTIFICATION_CB *notify_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG *msg = NULL;
	HM_NOTIFICATION_MSG *notify_msg = NULL;
	HM_SUBSCRIBER affected_node;
	HM_SOCKADDR_UNION *addr=NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(notify_cb != NULL);

	/***************************************************************************/
	/* Main Routine															   */
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
	/* WICKED CODE. Please refrain from using so many levels of pointer.	   */
	/***************************************************************************/
	switch(notify_cb->notification_type)
	{
	case HM_NOTIFICATION_NODE_ACTIVE:
		TRACE_INFO(("Node %d Active on Location %d", affected_node.node_cb->index,
				affected_node.node_cb->node_cb->parent_location_cb->index));
		notify_msg->type = HM_NOTIFY_TYPE_NODE_UP;
		notify_msg->id = 0;
		notify_msg->if_id = 0;
		addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->transport_cb->sock_cb->addr);
		if(affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IN)
		{
			TRACE_DETAIL(("Node has IPv4 type of connection."));
			notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
			memcpy(notify_msg->addr_info.addr,
					&addr->in_addr.sin_addr.s_addr,
				sizeof(struct in_addr));
			notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
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
			TRACE_WARN(("Unknown Transport Type"));
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
		TRACE_ASSERT(affected_node.node_cb->node_cb->transport_cb != NULL);
		addr = (HM_SOCKADDR_UNION *)&(affected_node.node_cb->node_cb->transport_cb->sock_cb->addr);
		if(affected_node.node_cb->node_cb->transport_cb->type == HM_TRANSPORT_TCP_IN)
		{
			TRACE_DETAIL(("Node has IPv4 type of connection."));
			notify_msg->addr_info.addr_type = HM_NOTIFY_ADDR_TYPE_TCP_v4;
			memcpy(notify_msg->addr_info.addr,
					&addr->in_addr.sin_addr.s_addr,
				sizeof(struct in_addr));
			notify_msg->addr_info.port = (uint32_t)addr->in_addr.sin_port;
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
			TRACE_WARN(("Unknown Transport Type"));
		}
		notify_msg->addr_info.group = affected_node.node_cb->group_index;
		notify_msg->addr_info.hw_index = affected_node.node_cb->node_cb->parent_location_cb->index;
		notify_msg->addr_info.node_id = affected_node.node_cb->index;

		break;

	case HM_NOTIFICATION_PROCESS_CREATED:
		TRACE_INFO(("Process Created"));
		notify_msg->type = HM_NOTIFY_TYPE_PROC_AVAILABLE;
		notify_msg->id = affected_node.process_cb->pid;
		notify_msg->if_id = 0;
		break;
	case HM_NOTIFICATION_PROCESS_DESTROYED:
		TRACE_INFO(("Process Destroyed"));
		notify_msg->type = HM_NOTIFY_TYPE_PROC_GONE;
		break;
	case HM_NOTIFICATION_INTERFACE_ADDED:
		TRACE_INFO(("Interface Added"));
		break;
	case HM_NOTIFICATION_INTERFACE_DELETE:
		TRACE_INFO(("Interface Deleted"));
		break;
	default:
		TRACE_ERROR(("Unknown notification type"));
		TRACE_ASSERT(FALSE);
	}

	notify_msg->id = 0;
	notify_msg->if_id = 0;
	notify_msg->type = HM_NOTIFY_TYPE_NODE_UP;

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (msg);
}/* hm_build_notify_message */