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

extern HM_GLOBAL_DATA global;

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
HM_AVL3_TREE_INFO node_process_tree_by_id = {
	hm_compare_ulong, /* pointer to function*/
	HM_OFFSETOF(HM_PROCESS_CB, id)	, /* key offset*/
	HM_OFFSETOF(HM_PROCESS_CB, node)  /* node offset */
};

#else

extern HM_AVL3_TREE_INFO node_process_tree_by_proc_id;
extern HM_AVL3_TREE_INFO node_process_tree_by_proc_id;
extern HM_AVL3_TREE_INFO node_process_tree_by_proc_id;

#endif

#endif /* SRC_HMGLOB_H_ */
