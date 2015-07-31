/**
 *  @file hmprocmgmt.c
 *  @brief Hardware Manager Process Monitoring and Management Routines
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */
#include <hmincl.h>


/**
 *  @brief Adds the Process Control Block into its parent Node CB
 *
 *  @param *proc_cb Process Control Block (#HM_PROCESS_CB) to be added
 *  @param *node_cb Node Control Block (#HM_NODE_CB) to which Process is to be added
 *  @return #HM_OK if successful, #HM_ERR otherwise.
 */
int32_t hm_process_add(HM_PROCESS_CB *proc_cb, HM_NODE_CB *node_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	HM_PROCESS_CB *insert_cb = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(proc_cb != NULL);
	TRACE_ASSERT(node_cb != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	proc_cb->parent_node_cb = node_cb;

	/***************************************************************************/
	/* Try insertion into the Node Process Tree.							   */
	/* If a copy is found, verify that the first instance was not running.	   */
	/* Now, there is one major worry with keeping old data marked as stale in  */
	/* the Database, that being, if each node keeps on creating Processes and  */
	/* destroying them, and each time a new instance is created, it is given a */
	/* new PID. Then, the DB Trees here will get un-necessarily big with mostly*/
	/* stale data. That is a condition of a very jittery system, though and is */
	/* not expected. Hence, we are okay with it.							   */
	/***************************************************************************/
	insert_cb = (HM_PROCESS_CB *)HM_AVL3_INSERT_OR_FIND(
								node_cb->process_tree,
								proc_cb->node,
								node_process_tree_by_proc_type_and_pid);
	if(insert_cb != NULL)
	{
		TRACE_INFO(("Process entry already existed. Validate and Update"));
		if(insert_cb->running == TRUE)
		{
			TRACE_WARN(("Previous Process was also running. This is erroneous!"));
			ret_val = HM_ERR;
			goto EXIT_LABEL;
		}

		/***************************************************************************/
		/* Free the current Process CB											   */
		/***************************************************************************/
		TRACE_DETAIL(("Freeing new Process CB"));
		hm_free_process_cb(proc_cb);
		proc_cb = NULL;
	}
	else
	{
		/***************************************************************************/
		/* Fixup pointers. We'll use insert_cb only from now on.				   */
		/***************************************************************************/
		insert_cb = proc_cb;
		/***************************************************************************/
		/* Increment it's parent location's number of active processes count	   */
		/***************************************************************************/
		proc_cb->parent_node_cb->parent_location_cb->active_processes++;
		TRACE_INFO(("Parent location %d's active processes now %d",
				proc_cb->parent_node_cb->parent_location_cb->index,
				proc_cb->parent_node_cb->parent_location_cb->active_processes));
	}
	/***************************************************************************/
	/* Inherit the role of its parent node.									   */
	/***************************************************************************/
	insert_cb->role = insert_cb->parent_node_cb->role;

	/***************************************************************************/
	/* Create a Global Process CB Entry										   */
	/***************************************************************************/
	if(hm_global_process_add(insert_cb)!= HM_OK)
	{
		TRACE_ERROR(("Error adding Process to Global Tables."));
		HM_AVL3_DELETE(node_cb->process_tree, insert_cb->node);
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Send an update of the process being added to subscribers				   */
	/* Processes are not pre-provisioned. So, if this routine is being hit 	   */
	/* means that process actually has been created, in that case, subscribers */
	/* must know.															   */
	/***************************************************************************/
	hm_global_process_update(insert_cb);

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_process_add */

/**
 *  @brief Updates the state of Process and causes triggers.
 *
 *  @param *proc_cb Process CB that has been updated (#HM_PROCESS_CB)
 *  @return #HM_OK on success, #HM_ERR otherwise.
 */
int32_t hm_process_update(HM_PROCESS_CB *proc_cb)
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
	/* If it is no longer running, decrement the counter.					   */
	/* The number of locations from where this may be called while creation is */
	/* not controlled (yet), and hence only deactivation is updated here.	   */
	/***************************************************************************/
	if(!proc_cb->running)
	{
		/***************************************************************************/
		/* Decrement it's parent location's number of active processes count	   */
		/***************************************************************************/
		proc_cb->parent_node_cb->parent_location_cb->active_processes--;
		TRACE_INFO(("Parent location %d's active processes now %d",
				proc_cb->parent_node_cb->parent_location_cb->index,
				proc_cb->parent_node_cb->parent_location_cb->active_processes));
	}
	/***************************************************************************/
	/* Update the Global Tables												   */
	/***************************************************************************/
	hm_global_process_update(proc_cb);

	/***************************************************************************/
	/* TODO: UPDATE Interfaces tables too									   */
	/***************************************************************************/

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return(ret_val);
}/* hm_process_update */

