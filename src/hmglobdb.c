/*
 * hmglobdb.c
 *
 *  Created on: 29-May-2015
 *      Author: anshul
 */

#include <hmincl.h>

/***************************************************************************/
/* Name:	hm_global_location_add 										   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Adds a Location to global trees for subscription processing	   */
/***************************************************************************/
int32_t hm_global_location_add(HM_LOCATION_CB *loc_cb, uint32_t status)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_GLOBAL_LOCATION_CB *glob_cb = NULL;
	int32_t ret_val = HM_OK;
	int32_t trigger = TRUE;
	HM_SUBSCRIBER_WILDCARD *greedy = NULL;

	HM_SUBSCRIPTION_CB *sub_cb = NULL;
	HM_LIST_BLOCK *list_member = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(loc_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/***************************************************************************/
	/* First check if we don't have a node with same location ID in tree 	   */
	/***************************************************************************/
	/* NOTE: Currently, we're keying and sorting it by HW ID only. This is so  */
	/* because the system has a constraint that HW ID will always be unique.   */
	/* The commented code segment would be useful when sifting criteria is not */
	/* on the basis of keys.												   */
	/*
	for(glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIRST(LOCAL.locations_tree,
														locations_tree_by_db_id);
		glob_cb != NULL;
		glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_NEXT(glob_cb->node,
														locations_tree_by_db_id))
	{

	}
	*/
	glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIND(LOCAL.locations_tree,
										&loc_cb->index, locations_tree_by_db_id);
	if(glob_cb != NULL)
	{
		/* THE DESIRED BEHAVIOR IF THIS CONDITION OCCURS IS NOT THOUGHT AS OF YET */
		/* FIXME:																  */
		/* It is hard to say that such a condition will not arise. This is because*/
		/* we are operating on a cluster, and conflicts may arise.				  */
		/* But because, at the given moment, we are in control of the cluster 	  */
		/* topology (closed system design), we'll handle it as a non-recoverable  */
		/* error.																  */

		TRACE_WARN(("Found a previous node with the same index."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;

		/* Currently ineffective and unhittable. This is just an evolution path.  */
		if(glob_cb->status == status)
		{
			TRACE_DETAIL(("States are same. Do not trigger notifications!"));
			trigger = FALSE;
		}
	}
	else
	{
		/***************************************************************************/
		/* Allocate a global location CB										   */
		/***************************************************************************/
		glob_cb = (HM_GLOBAL_LOCATION_CB *)malloc(sizeof(HM_GLOBAL_LOCATION_CB));
		TRACE_ASSERT(glob_cb != NULL);
		if(glob_cb == NULL)
		{
			TRACE_ERROR(("Error allocating resources for Location Entry."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		glob_cb->id = LOCAL.next_loc_tree_id++;
		glob_cb->index =  loc_cb->index;
		HM_AVL3_INIT_NODE(glob_cb->node, glob_cb);
		glob_cb->sub_cb = NULL;
		glob_cb->table_type = HM_TABLE_TYPE_LOCATION;
	}
	glob_cb->loc_cb = loc_cb;
	glob_cb->status = status;

	/***************************************************************************/
	/* Try inserting it into the tree										   */
	/***************************************************************************/
	if(!HM_AVL3_IN_TREE(glob_cb->node))
	{
		TRACE_DETAIL(("Inserting"));
		if(HM_AVL3_INSERT(LOCAL.locations_tree, glob_cb->node, locations_tree_by_db_id) != TRUE)
		{
			TRACE_ERROR(("Error inserting into global trees."));
			ret_val = HM_ERR;
			free(glob_cb);
			glob_cb = NULL;
			goto EXIT_LABEL;
		}
		/***************************************************************************/
		/* Create a subscription entry											   */
		/***************************************************************************/
		if((sub_cb = hm_create_subscription_entry(HM_TABLE_TYPE_LOCATION, loc_cb->index,
														 (void *)glob_cb))== NULL)
		{
			TRACE_ERROR(("Error creating subscription."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		glob_cb->sub_cb = sub_cb;
	}
	/***************************************************************************/
	/* Update pointers on Location CB too.									   */
	/***************************************************************************/
	loc_cb->id = glob_cb->id;
	loc_cb->db_ptr = (void *)glob_cb;

	/***************************************************************************/
	/* Find out if there is some greedy (wildcard) subscriber.				   */
	/* If present, subscribe to this location implicitly.					   */
	/***************************************************************************/
	for(greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
			greedy != NULL;
			greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(greedy->node))
	{
		if((greedy->subs_type == HM_CONFIG_ATTR_SUBS_TYPE_LOCATION) &&
				((greedy->value == 0)||(greedy->value==loc_cb->index)))
		{
			TRACE_DETAIL(("Found wildcard subscriber."));
			/***************************************************************************/
			/* Allocate Node														   */
			/***************************************************************************/
			list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK));
			if(list_member == NULL)
			{
				TRACE_ERROR(("Error allocating resources for Subscriber list element."));
				ret_val = HM_ERR;
				goto EXIT_LABEL;
			}
			HM_INIT_LQE(list_member->node, list_member);
			list_member->target = greedy->subscriber.node_cb;

			if(hm_subscription_insert(sub_cb, list_member) != HM_OK)
			{
				TRACE_ERROR(("Error inserting subscription to its entity"));
				ret_val = HM_ERR;
				hm_free_subscription_cb(sub_cb);
				free(list_member);
				sub_cb = NULL;
				list_member = NULL;
				goto EXIT_LABEL;
			}
		}
	}

	if(trigger == TRUE)
	{
		TRACE_DETAIL(("Trigger notification to existing subscribers."));
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_global_location_add */

/***************************************************************************/
/* Name:	hm_global_location_update 									   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Looks into the Location structure of the Location for updates  */
/*			and perform updates.										   */
/* All spectacular things happen here.									   */
/***************************************************************************/
int32_t hm_global_location_update(HM_LOCATION_CB *loc_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_global_location_update */

/***************************************************************************/
/* Name:	hm_global_location_remove 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Removes the Location CB from global DB			*/
/***************************************************************************/
int32_t hm_global_location_remove(HM_GLOBAL_LOCATION_CB *loc_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(loc_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_global_location_remove */

/***************************************************************************/
/* Name:	hm_global_node_add 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Adds the node to global aggregate trees and handles the 	   */
/*	subscriptions processing.											   */
/***************************************************************************/
int32_t hm_global_node_add(HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_GLOBAL_NODE_CB *insert_cb = NULL;
	int32_t ret_val = HM_OK;
	HM_SUBSCRIPTION_CB *sub_cb = NULL;
	HM_SUBSCRIBER_WILDCARD *greedy = NULL;
	HM_LIST_BLOCK *list_member = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(node_cb!= NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/***************************************************************************/
	/* First search for a node with same node ID. If not found, grant this CB  */
	/* a new ID, and insert it in the tree.									   */
	/*																		   */
	/* NOTE: We are currently using node index for global keying, because it is*/
	/* guaranteed to be unique in current design.							   */
	/***************************************************************************/
	insert_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
											&node_cb->index,
											nodes_tree_by_db_id);
	if(insert_cb != NULL)
	{
		/***************************************************************************/
		/* Same node found.														   */
		/***************************************************************************/
		TRACE_ERROR(("Same node found. This shouldn't happen!"));
		/* THE DESIRED BEHAVIOR IF THIS CONDITION OCCURS IS NOT THOUGHT AS OF YET */
		/* FIXME:																  */
		/* It is hard to say that such a condition will not arise. This is because*/
		/* we are operating on a cluster, and conflicts may arise.				  */
		/* But because, at the given moment, we are in control of the cluster 	  */
		/* topology (closed system design), we'll handle it as a non-recoverable  */
		/* error.																  */
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	else
	{
		/***************************************************************************/
		/* Allocate a global location CB										   */
		/***************************************************************************/
		insert_cb = (HM_GLOBAL_NODE_CB *)malloc(sizeof(HM_GLOBAL_NODE_CB));
		TRACE_ASSERT(insert_cb != NULL);
		if(insert_cb == NULL)
		{
			TRACE_ERROR(("Error allocating resources for Location Entry."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}

		insert_cb->id = LOCAL.next_node_tree_id++;
		HM_AVL3_INIT_NODE(insert_cb->node, insert_cb);
		HM_INIT_ROOT(insert_cb->subscriptions);
		insert_cb->index =  node_cb->index;
		insert_cb->group_index = node_cb->group;
		insert_cb->table_type = HM_TABLE_TYPE_NODES;
	}

	TRACE_DETAIL(("Node ID: %d", insert_cb->id));
	insert_cb->node_cb = node_cb;
	insert_cb->role = node_cb->role;
	insert_cb->status = node_cb->fsm_state;

	/***************************************************************************/
	/* Try inserting it into the tree										   */
	/***************************************************************************/
	if(!HM_AVL3_IN_TREE(insert_cb->node))
	{
		TRACE_DETAIL(("Inserting"));
		if(HM_AVL3_INSERT(LOCAL.nodes_tree, insert_cb->node, nodes_tree_by_db_id) != TRUE)
		{
			TRACE_ERROR(("Error inserting into global trees."));
			ret_val = HM_ERR;
			free(insert_cb);
			insert_cb = NULL;
			goto EXIT_LABEL;
		}
		/***************************************************************************/
		/* Create a subscription entry											   */
		/***************************************************************************/
		if((sub_cb = hm_create_subscription_entry(HM_TABLE_TYPE_NODES, insert_cb->index,
														 (void *)insert_cb))== NULL)
		{
			TRACE_ERROR(("Error creating subscription."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		insert_cb->sub_cb = sub_cb;
	}
	/***************************************************************************/
	/* Update pointers on Location CB too.									   */
	/***************************************************************************/
	node_cb->id = insert_cb->id;
	node_cb->db_ptr = (void *)insert_cb;

	/***************************************************************************/
	/* Find out if there is some greedy (wildcard) subscriber.				   */
	/* If present, subscribe to this location implicitly.					   */
	/***************************************************************************/
	for(greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
			greedy != NULL;
			greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(greedy->node))
	{
		if((greedy->subs_type == HM_CONFIG_ATTR_SUBS_TYPE_GROUP) &&
				((greedy->value == 0)||(greedy->value==node_cb->group)))
		{
			TRACE_DETAIL(("Found wildcard subscriber."));
			/***************************************************************************/
			/* Allocate Node														   */
			/***************************************************************************/
			list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK));
			if(list_member == NULL)
			{
				TRACE_ERROR(("Error allocating resources for Subscriber list element."));
				ret_val = HM_ERR;
				goto EXIT_LABEL;
			}
			HM_INIT_LQE(list_member->node, list_member);
			list_member->target = greedy->subscriber.node_cb;

			if(hm_subscription_insert(sub_cb, list_member) != HM_OK)
			{
				TRACE_ERROR(("Error inserting subscription to its entity"));
				ret_val = HM_ERR;
				hm_free_subscription_cb(sub_cb);
				free(list_member);
				sub_cb = NULL;
				list_member = NULL;
				goto EXIT_LABEL;
			}
		}
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_global_node_add */

/***************************************************************************/
/* Name:	hm_global_node_update 										   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Updates the node control block and notifies the rest.		   */
/* All spectacular things happen here.									   */
/***************************************************************************/
int32_t hm_global_node_update(HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	int32_t notify = FALSE;
	HM_GLOBAL_NODE_CB *glob_cb = NULL;
	HM_NOTIFICATION_CB *notify_cb = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(node_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Look into the node state in its FSM variable to determine if an update  */
	/* is needed or not.													   */
	/***************************************************************************/
	switch(node_cb->fsm_state)
	{
	case HM_NODE_FSM_STATE_ACTIVE:
		TRACE_DETAIL(("Node moved to active state. Send Notifications."));
		notify = HM_NOTIFICATION_NODE_ACTIVE;
		break;

	case HM_NODE_FSM_STATE_FAILING:
		TRACE_DETAIL(("Node is no longer active. Send Notifications."));
		notify = HM_NOTIFICATION_NODE_INACTIVE;
		break;

	default:
		TRACE_WARN(("Unknown state of Node %d", node_cb->fsm_state));
		TRACE_ASSERT(FALSE);
	}

	if(!notify)
	{
		TRACE_DETAIL(("No updates."));
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* We need to send updates. These work irrespective of previous state. This*/
	/* means that the caller must ensure if a notification must be issued or   */
	/* not. If old and new states are same ('Up', and 'Up' somehow), two same  */
	/* notifications would be sent.											   */
	/***************************************************************************/
	/***************************************************************************/
	/* Find the node in DB.													   */
	/***************************************************************************/
	glob_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
											&node_cb->id, nodes_tree_by_db_id);
	if(glob_cb == NULL)
	{
		TRACE_ERROR(("No Global DB Entry found for this node. This shouldn't happen."));
		TRACE_ASSERT(glob_cb != NULL);
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* If subscription was not live, it is now.								   */
	/***************************************************************************/
	/***************************************************************************/
	/* Move this subscription node into the active subscriptions tree.		   */
	/* And also notify subscribers.											   */
	/***************************************************************************/
	if(glob_cb->sub_cb->live == FALSE)
	{
		TRACE_DETAIL(("Subscription Now Active."));
		HM_AVL3_DELETE(LOCAL.pending_subscriptions_tree, glob_cb->sub_cb->node);
		if(HM_AVL3_INSERT(LOCAL.active_subscriptions_tree, glob_cb->sub_cb->node,
														subs_tree_by_db_id)!= TRUE)
		{
			TRACE_ERROR(("Error activating subscription."));
			goto EXIT_LABEL;
		}

		/***************************************************************************/
		/* Mark the subscription as active.										   */
		/***************************************************************************/
		glob_cb->sub_cb->live = TRUE;
	}

	/***************************************************************************/
	/* Allocate a Notification CB and Add that notification CB to Notify Queue */
	/***************************************************************************/
	notify_cb =  hm_alloc_notify_cb();
	if(notify_cb == NULL)
	{
		TRACE_ERROR(("Error creating Notification CB"));
		TRACE_ERROR(("Update could not be propagated."));
		ret_val = HM_ERR;
	}
	notify_cb->node_cb.node_cb = glob_cb;
	notify_cb->notification_type = notify;
	/***************************************************************************/
	/* Queue the notification CB 											   */
	/***************************************************************************/
	HM_INSERT_BEFORE(LOCAL.notification_queue ,notify_cb->node);

	//FIXME: Move it to a separate thread later
	hm_service_notify_queue();
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_global_node_update */

/***************************************************************************/
/* Name:	hm_global_node_remove 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Removes the node from global tables and carries out subscript- */
/* -ion routines.														   */
/***************************************************************************/
int32_t hm_global_node_remove(HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return(ret_val);
}/* hm_global_node_remove */


/***************************************************************************/
/* Name:	hm_create_subscription_entry 								   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Makes an entry in subscription tree if no previous entry exists*/
/***************************************************************************/
HM_SUBSCRIPTION_CB * hm_create_subscription_entry(uint32_t subs_type, uint32_t value, void *row)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_SUBSCRIPTION_CB *tree_node = NULL;
	HM_SUBSCRIPTION_CB *sub_cb = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Row may be void. In case the subscriber comes before subscription       */
	/***************************************************************************/

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	TRACE_DETAIL(("Subscription Entry Requested: Type: %d Value: %d",
			subs_type, value));
	 /***************************************************************************/
	 /* A node added to any aggregate tree can be a subscription. Add a 		*/
	 /* subscription point in the pending subscription tables.				    */
	 /* It is not currently subscribed to.									    */
	 /***************************************************************************/
	sub_cb = hm_alloc_subscription_cb();
	if(sub_cb== NULL)
	{
		TRACE_ERROR(("Error creating subscription point."));
		/***************************************************************************/
		/* Remove from global node tree											   */
		/***************************************************************************/
		goto EXIT_LABEL;
	}
	 /***************************************************************************/
	 /* We are a node_table type entry											*/
	 /***************************************************************************/
	 sub_cb->table_type = subs_type;
	 sub_cb->value = value;
	 sub_cb->row_cb.void_cb = row;

	 if(row != NULL)
	 {
		 sub_cb->row_id = *(int32_t *)row;
		 TRACE_DETAIL(("Subscription row ID: %d", sub_cb->row_id));
	 }

	/***************************************************************************/
	/* First check the pending list if we have a matching subscription waiting */
	/* This may happen if a subscription was made before its provider could be */
	/* provisioned.															   */
	/***************************************************************************/
	for(tree_node =
			(HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.pending_subscriptions_tree,
															subs_tree_by_db_id);
		tree_node != NULL;
		tree_node =
			(HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(tree_node->node,
															subs_tree_by_db_id))
	{
		if((tree_node->table_type == sub_cb->table_type) &&
											(tree_node->value == sub_cb->value))
		{
			TRACE_INFO(("Found a pending subscription."));

#ifdef I_WANT_TO_DEBUG
			if(tree_node->row_cb.void_cb != NULL)
			{
				TRACE_DETAIL(("Previous subscription point has a NON NULL row"));
				TRACE_ASSERT(tree_node->row_id == sub_cb->row_id);
				TRACE_ASSERT(tree_node->row_cb.void_cb == sub_cb->row_cb.void_cb);
			}
#endif
			tree_node->row_cb.void_cb = sub_cb->row_cb.void_cb;
			tree_node->row_id = sub_cb->row_id;
			//TODO
#if 0
			/***************************************************************************/
			/* Move this subscription node into the active subscriptions tree.		   */
			/* And also notify subscribers.											   */
			/***************************************************************************/
			HM_AVL3_DELETE(LOCAL.pending_subscriptions_tree, tree_node->node);
			if(HM_AVL3_INSERT(LOCAL.active_subscriptions_tree, tree_node->node, subs_tree_by_db_id)!= TRUE)
			{
				TRACE_ERROR(("Error activating subscription."));
				goto EXIT_LABEL;
			}

			/***************************************************************************/
			/* Mark the subscription as active.										   */
			/***************************************************************************/
			tree_node->live = TRUE;

			//TODO: Notify subscribers
			TRACE_DETAIL(("Notifying Subscribers"));
#endif
			/***************************************************************************/
			/* Found the node, no need to look any further							   */
			/***************************************************************************/
			hm_free_subscription_cb(sub_cb);
			goto EXIT_LABEL;
		}
	}
	/***************************************************************************/
	/* Previous entry not found. Create a new one.							   */
	/***************************************************************************/
	sub_cb->id = LOCAL.next_pending_tree_id++;
	TRACE_DETAIL(("Subscription ID: %d", sub_cb->id));

	if(HM_AVL3_INSERT(LOCAL.pending_subscriptions_tree, sub_cb->node, subs_tree_by_db_id) != TRUE)
	{
		TRACE_ERROR(("Error creating subscription point."));
		LOCAL.next_pending_tree_id -=1;
		hm_free_subscription_cb(sub_cb);
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return sub_cb;
}/* hm_create_subscription_entry */

/***************************************************************************/
/* Name:	hm_update_subscribers 										   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Updates the subscribers for a particular subscription point	   */
/***************************************************************************/
int32_t hm_update_subscribers(HM_SUBSCRIPTION_CB *subs_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_LIST_BLOCK *greedy_node = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(subs_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
}/* hm_update_subscribers */

/***************************************************************************/
/* Name:	hm_subscribe 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Adds a subscription											   */
/* Registers for a particular subscription type.						   */
/* First traverse the active and pending subscription trees to see if we   */
/* already have that kind of subscription point ready. If not, we make one */
/* in the pending tree.													   */
/***************************************************************************/
int32_t hm_subscribe(uint32_t subs_type, uint32_t value, void *cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_SUBSCRIBER_WILDCARD *subscriber;
	HM_SUBSCRIPTION_CB *subscription = NULL;
	HM_SUBSCRIBER global_cb;
	HM_LIST_BLOCK *list_member = NULL;
	int32_t ret_val = HM_OK;

	int32_t table_type;
	int32_t keys[2];
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/***************************************************************************/
	/* Depending on the subscription type, choose the table					   */
	/***************************************************************************/
	switch(subs_type)
	{
	case HM_CONFIG_ATTR_SUBS_TYPE_GROUP:
		table_type = HM_TABLE_TYPE_NODES;
		break;
	case HM_CONFIG_ATTR_SUBS_TYPE_PROC:
		table_type = HM_TABLE_TYPE_PROCESS;
		break;
	case HM_CONFIG_ATTR_SUBS_TYPE_IF:
		table_type = HM_TABLE_TYPE_IF;
		break;
	case HM_CONFIG_ATTR_SUBS_TYPE_LOCATION:
		table_type = HM_TABLE_TYPE_LOCATION;
		break;
	case HM_CONFIG_ATTR_SUBS_TYPE_NODE:
		table_type = HM_TABLE_TYPE_NODES;
		break;
	default:
		TRACE_ERROR(("Unknown subscription group"));
		TRACE_ASSERT(0==1);
	}
	//FIXME
	if((subs_type == HM_CONFIG_ATTR_SUBS_TYPE_GROUP)
		|| (subs_type == HM_CONFIG_ATTR_SUBS_TYPE_PROC)
		|| (subs_type == HM_CONFIG_ATTR_SUBS_TYPE_IF)
		||	value == 0)
	{
		TRACE_DETAIL(("Subscribe to all nodes."));
		/* You can't really be a wildcard which does not have an entity yet */
		TRACE_ASSERT(cb != NULL);
		subscriber = (HM_SUBSCRIBER_WILDCARD *)malloc(sizeof(HM_SUBSCRIBER_WILDCARD));
		if(subscriber==NULL)
		{
			TRACE_ERROR(("Error allocating resources for Greedy subscriber."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		HM_INIT_LQE(subscriber->node, subscriber);
		subscriber->subs_type = subs_type;
		subscriber->value = value;
		subscriber->subscriber.void_cb = cb;
		global_cb.void_cb= cb;
		/***************************************************************************/
		/* Determine its table type and accordingly, find its global table entry   */
		/***************************************************************************/
		if(cb != NULL)
		{
			TRACE_DETAIL(("Checking type %d", *(int32_t *)((char *)cb+ (uint32_t)(sizeof(int32_t)))));
			switch(*(int32_t *)((char *)cb+ (uint32_t)(sizeof(int32_t))))
			{
			case HM_TABLE_TYPE_NODES_LOCAL:
				TRACE_DETAIL(("Local Node Structure. Find its Global Entry."));
				TRACE_DETAIL(("ID in Global Node Table %d", *(int32_t *)(global_cb.void_cb)));
				subscriber->subscriber.void_cb = global_cb.proper_node_cb->db_ptr;
				TRACE_DETAIL(("Subscriber type %d", *(int32_t *)(
						(char *)subscriber->subscriber.void_cb+ (uint32_t)(sizeof(int32_t)))));
				break;
			case HM_TABLE_TYPE_LOCATION_LOCAL:
				TRACE_DETAIL(("Local Location Structure. Find its Global Entry."));
				subscriber->subscriber.void_cb = global_cb.proper_location_cb->db_ptr;
				break;
			case HM_TABLE_TYPE_PROCESS_LOCAL:
				TRACE_DETAIL(("Local Process Structure. Find its Global Entry."));
				subscriber->subscriber.void_cb = global_cb.proper_process_cb->db_ptr;
				break;
			default:
				TRACE_WARN(("Unknown type of subscriber"));
				TRACE_ASSERT((FALSE));
			}
		}

		/***************************************************************************/
		/* Insert to wildcard list.												   */
		/***************************************************************************/
		TRACE_DETAIL(("Add to greedy list."));
		HM_INSERT_BEFORE(LOCAL.table_root_subscribers, subscriber->node);

		/***************************************************************************/
		/* Insert it as a subscriber to every existing node right now.			   */
		/***************************************************************************/
		//TODO: It is not going to work this way. To implement partial matching, we
		// need different kind of trees.
		/*
		 * Right now, we can traverse and check each node for type
		 */
		for(subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.active_subscriptions_tree,
														subs_tree_by_db_id );
				subscription != NULL;
				subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
														subs_tree_by_db_id))
		{
			if(subscription->table_type == table_type)
			{
				if((subscriber->value == 0) || (subscriber->value == subscription->value))
				{
					TRACE_DETAIL(("Found a node. Make subscription."));
					/***************************************************************************/
					/* Do not subscribe to itself											   */
					/***************************************************************************/
					if(subscription->row_cb.void_cb == subscriber->subscriber.void_cb)
					{
						TRACE_DETAIL(("Do not subscribe to itself!"));
						continue;
					}
					/***************************************************************************/
					/* Allocate Node.														   */
					/***************************************************************************/
					list_member = NULL;
					list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK));
					if(list_member == NULL)
					{
						TRACE_ERROR(("Error allocating resources for Subscriber list element."));
						ret_val = HM_ERR;
						goto EXIT_LABEL;
					}
					HM_INIT_LQE(list_member->node, list_member);
					list_member->target = subscriber->subscriber.void_cb;
					/***************************************************************************/
					/* Insert into List														   */
					/***************************************************************************/
					if(hm_subscription_insert(subscription, list_member) != HM_OK)
					{
						TRACE_ERROR(("Error inserting subscription to its entity"));
						ret_val = HM_ERR;
						free(list_member);
						list_member = NULL;
						goto EXIT_LABEL;
					}
				}
			}
		}
		for(subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.pending_subscriptions_tree,
																subs_tree_by_db_id );
						subscription != NULL;
						subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
																subs_tree_by_db_id))
		{
			TRACE_DETAIL(("Types: Node: %d Current Value: %d",subscription->table_type, subs_type));
			if(subscription->table_type == table_type)
			{
				if((subscriber->value == 0) || (subscriber->value == subscription->value))
				{
					/***************************************************************************/
					/* Do not subscribe to itself											   */
					/***************************************************************************/
					if(subscription->row_cb.void_cb == subscriber->subscriber.void_cb)
					{
						TRACE_DETAIL(("Do not subscribe to itself!"));
						continue;
					}

					TRACE_DETAIL(("Found a node. Make subscription."));
					/***************************************************************************/
					/* Allocate Node.														   */
					/***************************************************************************/
					list_member = NULL;
					list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK));
					if(list_member == NULL)
					{
						TRACE_ERROR(("Error allocating resources for Subscriber list element."));
						ret_val = HM_ERR;
						goto EXIT_LABEL;
					}
					HM_INIT_LQE(list_member->node, list_member);
					list_member->target = subscriber->subscriber.void_cb;
					/***************************************************************************/
					/* Insert into List														   */
					/***************************************************************************/
					if(hm_subscription_insert(subscription, list_member) != HM_OK)
					{
						TRACE_ERROR(("Error inserting subscription to its entity"));
						ret_val = HM_ERR;
						free(list_member);
						list_member = NULL;
						goto EXIT_LABEL;
					}
				}
			}
		}
	}
	/***************************************************************************/
	/* Else, it is a singular subscription. Find it, or create one.			   */
	/***************************************************************************/
	else
	{
		/***************************************************************************/
		/* Look for a subscription point, first in Active subscription trees	   */
		/***************************************************************************/
		keys[0]=table_type;
		keys[1]=value;
		/***************************************************************************/
		/* Allocate Node														   */
		/***************************************************************************/
		list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK));
		if(list_member == NULL)
		{
			TRACE_ERROR(("Error allocating resources for Subscriber list element."));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
		HM_INIT_LQE(list_member->node, list_member);
		list_member->target = cb;

		subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIND(LOCAL.active_subscriptions_tree,
															keys,
															subs_tree_by_subs_type );
		if(subscription == NULL)
		{
			TRACE_DETAIL(("Try to find in Pending subscriptions"));
			/* FIXME: Possibly redundant. We always check in pending list in create_entry method */
			subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIND(LOCAL.pending_subscriptions_tree,
																	keys,
																	subs_tree_by_subs_type );
		}
		if(subscription == NULL)
		{
			TRACE_DETAIL(("Subscription not found in active or pending lists. Create one."));
			/***************************************************************************/
			/* Create subscription now.												   */
			/***************************************************************************/
			if((subscription = hm_create_subscription_entry(subs_type, value, NULL))== NULL)
			{
				TRACE_ERROR(("Error creating subscription!"));
				ret_val = HM_ERR;
				free(list_member);
				subscription = NULL;
				list_member = NULL;
				goto EXIT_LABEL;
			}
		}
		/***************************************************************************/
		/* Now, either we've found a previous subscription, or created a new one.  */
		/* Add subscriber to a future node in subscription tree					   */
		/***************************************************************************/
		if(hm_subscription_insert(subscription, list_member) != HM_OK)
		{
			TRACE_ERROR(("Error inserting subscription to its entity"));
			ret_val = HM_ERR;
			hm_free_subscription_cb(subscription);
			free(list_member);
			subscription = NULL;
			list_member = NULL;
			goto EXIT_LABEL;
		}
	}
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_subscribe */

/***************************************************************************/
/* Name:	hm_subscription_insert 										   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Inserts the subscription CB into the list, and triggers 	   */
/* Notifications if appropriate.										   */
/***************************************************************************/
int32_t hm_subscription_insert(HM_SUBSCRIPTION_CB *subs_cb, HM_LIST_BLOCK *node)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(subs_cb != NULL);
	TRACE_ASSERT(node != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	HM_INSERT_BEFORE(subs_cb->subscribers_list, node->node);

	/***************************************************************************/
	/* Increment the subscribers count										   */
	/***************************************************************************/
	subs_cb->num_subscribers++;
	/***************************************************************************/
	/* If subscription is active, create a notification response to the subscr-*/
	/* -iber. Other subscribers already know it. This one might need update.   */
	/***************************************************************************/
	if(subs_cb->live)
	{
		//TODO
		TRACE_DETAIL(("Send Notification to Subscriber."));

	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_subscription_insert */

