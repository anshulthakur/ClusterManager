/*
 * hmnodemgmt.c
 *
 *	Purpose: Node Management Layer
 *
 *  Created on: 07-May-2015
 *      Author: anshul
 */

#include <hmincl.h>

/***************************************************************************/
/* Name:	hm_node_add 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Adds a node to its parent Location and does the subscription   */
/* triggers.															   */
/***************************************************************************/
int32_t hm_node_add(HM_NODE_CB *node_cb, HM_LOCATION_CB *location_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	HM_NODE_CB *insert_cb = NULL;
	HM_LIST_BLOCK *temp_node = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(node_cb != NULL);
	TRACE_ASSERT(location_cb !=NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	node_cb->parent_location_cb = location_cb;
	/***************************************************************************/
	/* The generic stuff													   */
	/***************************************************************************/
	/***************************************************************************/
	/* Add to Location CBs Tree												   */
	/***************************************************************************/
	TRACE_DETAIL(("Add Node with ID %d to Location %d", node_cb->index, location_cb->index));

	insert_cb = (HM_NODE_CB *)HM_AVL3_INSERT_OR_FIND(location_cb->node_tree,
										node_cb->index_node,
										nodes_tree_by_node_id);
	 if(insert_cb != NULL)
	 {
		 TRACE_WARN(("Found an existing entry with same parameters. Overwriting fields"));
		 /***************************************************************************/
		 /* Validate that the two are one and the same thing. Maybe it had gone down*/
		 /* and is coming back up again?											*/
		 /***************************************************************************/
		 TRACE_ASSERT(insert_cb->index==node_cb->index);
		 /* FSM State field determines if the node is active or not */
		 /* For Local Nodes, this field is governed by an FSM.	    */
		 /* For Remote field, it is directly modified				*/
		 /* In any case, a new ADD request must only arrive if it   */
		 /* went down once.											*/
		 if(insert_cb->fsm_state == HM_NODE_FSM_STATE_ACTIVE)
		 {
			 TRACE_ERROR(("Existing node already in active state."));
			 ret_val = HM_ERR;
			 goto EXIT_LABEL;
		 }
		 TRACE_DETAIL(("Initial transport CB (%p) written with (%p)",
				 	 insert_cb->transport_cb, node_cb->transport_cb));
		 insert_cb->transport_cb = node_cb->transport_cb;
	 }

	 if(node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
	 {
		 TRACE_INFO(("Local Node added. Initialize timer processing."));
		/***************************************************************************/
		/* Alter its timers to desired values.									   */
		/***************************************************************************/
		HM_TIMER_MODIFY(node_cb->timer_cb, LOCAL.node_keepalive_period);
		node_cb->keepalive_period = LOCAL.node_keepalive_period;
		/***************************************************************************/
		/* Arm the timer to receive a INIT request.							    */
		/***************************************************************************/
		//TODO: Move it to FSM later.: Go into WAIT State
		HM_TIMER_START(node_cb->timer_cb);
	 }
	 else
	 {
		 TRACE_INFO(("Remote Node information added."));
		 /***************************************************************************/
		 /* Update FSM State of old state (if it has gone up)						*/
		 /***************************************************************************/
		 insert_cb->fsm_state = node_cb->fsm_state;
	 }

	 /***************************************************************************/
	 /* Add node to global tables for subscriptions and stuff.				    */
	 /***************************************************************************/
	 if(hm_global_node_add(node_cb) != HM_OK)
	 {
		 TRACE_ERROR(("Error updating node statistics in system"));
		 ret_val = HM_ERR;
		 if(hm_node_remove(node_cb)!= HM_OK)
		 {
			 TRACE_ERROR(("Error removing node from system."));
			 ret_val = HM_ERR;
			 goto EXIT_LABEL;
		 }
	 }
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_node_add */

/***************************************************************************/
/* Name:	hm_node_remove 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Removes the node from the system			*/
/***************************************************************************/
int32_t hm_node_remove(HM_NODE_CB *node_cb	)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(node_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	if(node_cb->id > 0)
	{
		TRACE_DETAIL(("Remove node from global tables too."));
		hm_global_node_remove(node_cb);
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_node_remove */


/***************************************************************************/
/* Name:	hm_node_keepalive_callback 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	void									*/
/* Purpose: Keepalive callback for Keepalive timer pop			*/
/***************************************************************************/
int32_t hm_node_keepalive_callback(void *cb)
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
}/* hm_node_keepalive_callback */
