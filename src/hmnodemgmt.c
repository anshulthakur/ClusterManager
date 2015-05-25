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
int32_t hm_node_add(HM_NODE_CB *node_cb,
					HM_LOCATION_CB *location_cb)
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
		 TRACE_ASSERT(insert_cb->id==node_cb->id);
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
		 /*
		  * Subscriptions are made with either config file, or dynamically
		  * So, anyways, its subscriptions list will be reconstructed from
		  * aggregate tables. So, flush out the old one.
		  */
		 for(temp_node = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(insert_cb->subscriptions);
				 temp_node != NULL;
				 temp_node = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(insert_cb->subscriptions))
		 {
			 HM_REMOVE_FROM_LIST(temp_node->node);
			 free(temp_node);
		 }
	 }

	 if(node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
	 {
		 TRACE_INFO(("Local Node added. Initialize timer processing."));
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
	/* Do subscription triggers												   */
	/***************************************************************************/
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_node */


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
}/* hm_node_keepalive_callback */
