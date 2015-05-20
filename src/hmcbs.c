/*
 * hmcbs.c
 *
 *  Purpose: Hardware Manager Control Block Allocation and Deallocation routines
 *
 *  Created on: 07-May-2015
 *      Author: anshul
 */
#include <hmincl.h>

/***************************************************************************/
/* Name:	hm_alloc_config_cb 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Allocates and initializes Configuration CB for HM to defaults  */
/***************************************************************************/
HM_CONFIG_CB * hm_alloc_config_cb()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_CONFIG_CB *config_cb = NULL;

	SOCKADDR_IN *addr = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/***************************************************************************/
	/* Allocate Config Structure											   */
	/***************************************************************************/
	config_cb = (HM_CONFIG_CB *)malloc(sizeof(HM_CONFIG_CB));
	if(config_cb == NULL)
	{
		TRACE_PERROR(("Error allocating User Configuration CB"));
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Load default configuration					   						   */
	/***************************************************************************/
	config_cb->instance_info.index = 0;
	/***************************************************************************/
	/* Initialize Addresses List for HM Instance							   */
	/***************************************************************************/
	HM_INIT_ROOT(config_cb->instance_info.addresses);
	/***************************************************************************/
	/* Initialize Node List													   */
	/***************************************************************************/
	HM_INIT_ROOT(config_cb->node_list);
	/***************************************************************************/
	/* Set defaults for Node and Cluster Hearbeats							   */
	/***************************************************************************/
	config_cb->instance_info.node.scope = HM_CONFIG_SCOPE_NODE;
	config_cb->instance_info.node.threshold = HM_CONFIG_DEFAULT_NODE_KICKOUT;
	config_cb->instance_info.node.timer_val = HM_CONFIG_DEFAULT_NODE_TICK_TIME;

	config_cb->instance_info.cluster.scope = HM_CONFIG_SCOPE_CLUSTER;
	config_cb->instance_info.cluster.threshold = HM_CONFIG_DEFAULT_PEER_KICKOUT;
	config_cb->instance_info.cluster.timer_val = HM_CONFIG_DEFAULT_PEER_TICK_TIME;

	/***************************************************************************/
	/* Initialize Transport Defauls											   */
	/***************************************************************************/
	config_cb->instance_info.mcast_group = HM_DEFAULT_MCAST_GROUP;


EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return(config_cb);
}/* hm_alloc_config_cb */


/***************************************************************************/
/* Name:	hm_alloc_transport_cb 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	HM_TRANSPORT_CB *									*/
/* Purpose: Allocates a Transport Connection Control Block			*/
/***************************************************************************/
HM_TRANSPORT_CB * hm_alloc_transport_cb(uint32_t conn_type,
										const char *ip,
										uint16_t port
										)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_TRANSPORT_CB *transport_cb = NULL;
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
	return transport_cb;
}/* hm_alloc_transport_cb */

/***************************************************************************/
/* Name:	hm_free_transport_cb 										   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Frees the Transport Connection Control Block				   */
/* If the transport connection CB is being used by a location as well as   */
/* node, and a node requests freeing the resources, do not free them.	   */
/* Only the location may free the transport CB							   */
/* This must be ensured by the calling function.						   */
/***************************************************************************/
int32_t hm_free_transport_cb(HM_TRANSPORT_CB *tprt_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int ret_val = HM_OK;
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
}/* hm_free_transport_cb */

/***************************************************************************/
/* Name:	hm_alloc_sock_cb 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	HM_SOCKET_CB *									*/
/* Purpose: purpose			*/
/***************************************************************************/
HM_SOCKET_CB * hm_alloc_sock_cb(struct sockaddr_storage *address)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_SOCKET_CB *sock_cb = NULL;
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
	return sock_cb;
}/* hm_alloc_sock_cb */

/***************************************************************************/
/* Name:	hm_alloc_node_cb 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	HM_NODE_CB *												   */
/* Purpose: Allocates a Node Control Block structure and initializes the   */
/* default values.														   */
/***************************************************************************/
HM_NODE_CB * hm_alloc_node_cb()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_NODE_CB *node_cb = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	node_cb = (HM_NODE_CB *)malloc(sizeof(HM_NODE_CB));
	if(node_cb == NULL)
	{
		TRACE_ERROR(("Error allocating resources for Node Control Block"));
		node_cb = NULL;
		goto EXIT_LABEL;
	}
	node_cb->id = 0;
	node_cb->index = 0;
	node_cb->group = 0;
	node_cb->role = NODE_ROLE_PASSIVE;
	memset(&node_cb->name, '\0', sizeof(node_cb->name));
	node_cb->transport_cb = NULL;
	node_cb->parent_location_cb = NULL;

	HM_INIT_ROOT(node_cb->subscribers);
	HM_INIT_ROOT(node_cb->subscriptions);

	/* We don't need to specify the TREE_INFO right now*/
	HM_AVL3_INIT_TREE(node_cb->process_tree, NULL);
	HM_AVL3_INIT_TREE(node_cb->interface_tree, NULL);

	node_cb->partner = NULL;

	node_cb->fsm_state = HM_NODE_FSM_STATE_NULL;

	node_cb->keepalive_missed = 0;
	node_cb->keepalive_period = HM_CONFIG_DEFAULT_NODE_TICK_TIME;

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (node_cb);
}/* hm_alloc_node_cb */

/***************************************************************************/
/* Name:	hm_free_node_cb 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Free the Node Control Block and its associated resources	   */
/***************************************************************************/
int32_t hm_free_node_cb(HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	HM_PROCESS_CB *tree_node = NULL;
	HM_TRANSPORT_CB *tprt_cb = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(node_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/***************************************************************************/
	/* Close connections to the node if any									   */
	/***************************************************************************/
	/* FIXME: Is not required for Nodes, as Location CBs will clean it up      */

	/***************************************************************************/
	/* Empty the process tree and destroy									   */
	/* Since we are removing each process from the tree, we need not traverse  */
	/* the entire tree, but only its first. (It autobalances)			       */
	/***************************************************************************/
	for(tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(
							node_cb->process_tree, node_process_tree_by_proc_id);
			tree_node != NULL;
		tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->process_tree,
													node_process_tree_by_proc_id))
	{
		if((ret_val = hm_free_process_cb(tree_node)) != HM_OK)
		{
			TRACE_ERROR(("Error freeing Process CB"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
	}
	/***************************************************************************/
	/* Empty the interface tree and destroy									   */
	/***************************************************************************/
	for(tree_node = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->interface_tree, node_process_tree_by_proc_id);
			tree_node != NULL;
		tree_node = (HM_PROCESS_CB *) HM_AVL3_FIRST(node_cb->interface_tree, node_process_tree_by_proc_id))
	{
		if((ret_val = hm_free_process_cb(tree_node)) != HM_OK)
		{
			TRACE_ERROR(("Error freeing Process CB"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}
	}
	/***************************************************************************/
	/* Empty the subscriber list											   */
	/***************************************************************************/
	//TODO
	/***************************************************************************/
	/* Empty the subscription list											   */
	/***************************************************************************/
	//TODO
EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_free_node_cb */


/***************************************************************************/
/* Name:	hm_free_process_cb 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Frees the Process Control Block for a process			*/
/***************************************************************************/
int32_t hm_free_process_cb(HM_PROCESS_CB *proc_cb)
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
	return (ret_val);
}/* hm_free_process_cb */
