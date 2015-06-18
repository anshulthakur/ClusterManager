/*
 * hmglob.h
 *
 *  Purpose: Global Variables and Usable Information
 *  Created on: 29-Apr-2015
 *      Author: anshul
 */

#ifndef SRC_HMGLOB_H_
#define SRC_HMGLOB_H_

/***************************************************************************/
/* Hardware Manager Global Data											   */
/***************************************************************************/

#ifdef HM_MAIN_DEFINE_VARS
HM_ATTRIBUTE_MAP attribute_map[] = {
		{"ms", 								HM_CONFIG_ATTR_RES_MIL_SEC},
		{"s",								HM_CONFIG_ATTR_RES_SEC},
		{"node",							HM_CONFIG_ATTR_HB_SCOPE_NODE},
		{"cluster",							HM_CONFIG_ATTR_HB_SCOPE_CLUSTER},
		{"tcp",								HM_CONFIG_ATTR_IP_TYPE_TCP},
		{"udp",								HM_CONFIG_ATTR_IP_TYPE_UDP},
		{"mcast",							HM_CONFIG_ATTR_IP_TYPE_MCAST},
		{"local",							HM_CONFIG_ATTR_ADDR_TYPE_LOCAL},
		{"remote",							HM_CONFIG_ATTR_ADDR_TYPE_CLUSTER},
		{"group",							HM_CONFIG_ATTR_SUBS_TYPE_GROUP},
		{"process",							HM_CONFIG_ATTR_SUBS_TYPE_PROC},
		{"interface",						HM_CONFIG_ATTR_SUBS_TYPE_IF},
		{"4",								HM_CONFIG_ATTR_IP_VERSION_4},
		{"6",								HM_CONFIG_ATTR_IP_VERSION_6},
};

uint32_t size_of_map = (sizeof(attribute_map)/sizeof(attribute_map[0]));
HM_GLOBAL_DATA global;

#else

extern HM_GLOBAL_DATA global;
extern HM_ATTRIBUTE_MAP attribute_map[];
extern uint32_t size_of_map;
#endif

#define LOCAL global

#ifdef HM_MAIN_DEFINE_VARS
HM_AVL3_TREE_INFO node_process_tree_by_proc_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_PROCESS_CB, pid)	, /* key offset*/
	HM_OFFSETOF(HM_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO node_process_tree_by_proc_type_and_pid = {
	hm_compare_2_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_PROCESS_CB, type)	, /* key offset*/
	HM_OFFSETOF(HM_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO global_process_tree_by_id = {
	hm_compare_proc_tree_keys, /* pointer to function*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO locations_tree_by_hardware_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_LOCATION_CB, index)	, /* key offset*/
	HM_OFFSETOF(HM_LOCATION_CB, hm_id_node)  /* node offset */
};
HM_AVL3_TREE_INFO locations_tree_by_db_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_GLOBAL_LOCATION_CB, index)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_LOCATION_CB, node)  /* node offset */
};
//TODO: Change to DB ID later
HM_AVL3_TREE_INFO nodes_tree_by_db_id = {
	/* hm_aggregate_compare_node_id,*/ /* pointer to function*/
	hm_compare_ulong,
	HM_OFFSETOF(HM_GLOBAL_NODE_CB, index)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_NODE_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO nodes_tree_by_node_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_NODE_CB, index)	, /* key offset*/
	HM_OFFSETOF(HM_NODE_CB, index_node)  /* node offset */
};
HM_AVL3_TREE_INFO process_tree_by_db_id = {
	hm_aggregate_compare_pid, /* pointer to function*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO process_tree_by_pid = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_PROCESS_CB, pid)	, /* key offset*/
	HM_OFFSETOF(HM_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO interface_tree_by_db_id = {
	hm_aggregate_compare_if_id, /* pointer to function*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO interface_tree_by_if_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_GLOBAL_PROCESS_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO subs_tree_by_db_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO subs_tree_by_subs_type_and_val = {
	hm_compare_2_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, table_type)	, /* key offset*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, node)  /* node offset */
};
HM_AVL3_TREE_INFO subs_tree_by_subs_type = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, table_type)	, /* key offset*/
	HM_OFFSETOF(HM_SUBSCRIPTION_CB, node)  /* node offset */
};
/***************************************************************************/
/* Signal Mask															   */
/***************************************************************************/
sigset_t mask;
/***************************************************************************/
/* Maximum descriptor value in the binary								   */
/***************************************************************************/
int32_t max_fd;

/***************************************************************************/
/* Global Socket descriptor set											   */
/***************************************************************************/
fd_set hm_tprt_conn_set;

/***************************************************************************/
/* Timers table															   */
/***************************************************************************/
HM_AVL3_TREE global_timer_table;
HM_AVL3_TREE_INFO timer_table_by_handle = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_TIMER_CB, handle), /* key offset*/
	HM_OFFSETOF(HM_TIMER_CB, node)  /* node offset */
};

int32_t *var;
#else

extern HM_AVL3_TREE_INFO node_process_tree_by_proc_id;
extern HM_AVL3_TREE_INFO node_process_tree_by_proc_type_and_pid;
extern HM_AVL3_TREE_INFO global_process_tree_by_id;
extern HM_AVL3_TREE_INFO locations_tree_by_hardware_id;
extern HM_AVL3_TREE_INFO locations_tree_by_db_id;
extern HM_AVL3_TREE_INFO nodes_tree_by_db_id;
extern HM_AVL3_TREE_INFO nodes_tree_by_node_id;
extern HM_AVL3_TREE_INFO process_tree_by_pid;
extern HM_AVL3_TREE_INFO process_tree_by_db_id;
extern HM_AVL3_TREE_INFO interface_tree_by_db_id;
extern HM_AVL3_TREE_INFO interface_tree_by_if_id;
extern HM_AVL3_TREE_INFO subs_tree_by_db_id;
extern HM_AVL3_TREE_INFO subs_tree_by_subs_type_and_val;
extern HM_AVL3_TREE_INFO subs_tree_by_subs_type;

extern sigset_t mask;

extern int32_t  max_fd;
extern fd_set hm_tprt_conn_set;
extern HM_AVL3_TREE global_timer_table;
extern HM_AVL3_TREE_INFO timer_table_by_handle;

extern int32_t *var;
#endif

#endif /* SRC_HMGLOB_H_ */
