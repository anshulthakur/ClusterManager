/*
 * hmfunc.h
 *
 * 	Hardware Manager Function Prototypes
 *
 *  Created on: 29-Apr-2015
 *      Author: anshul
 */

#ifndef SRC_HMFUNC_H_
#define SRC_HMFUNC_H_

/* hmmain.c */
int32_t hm_init_local(HM_CONFIG_CB *);
int32_t hm_init_transport();
int32_t hm_init_location_layer();
void hm_run_main_thread();
void hm_interrupt_handler(int32_t , siginfo_t *, void *);
void hm_terminate();

/* hmconf.c */
int32_t hm_parse_config(HM_CONFIG_CB *, char *);

/* hmcbs.c */
HM_CONFIG_CB * hm_alloc_config_cb();
HM_TRANSPORT_CB * hm_alloc_transport_cb(uint32_t);
int32_t hm_free_transport_cb(HM_TRANSPORT_CB *);
HM_SOCKET_CB * hm_alloc_sock_cb();
void hm_free_sock_cb(HM_SOCKET_CB *);
HM_LOCATION_CB * hm_alloc_location_cb();
int32_t hm_free_location_cb(HM_LOCATION_CB *);
HM_NODE_CB * hm_alloc_node_cb(uint32_t);
int32_t hm_free_node_cb(HM_NODE_CB *);
HM_PROCESS_CB * hm_alloc_process_cb();
int32_t hm_free_process_cb(HM_PROCESS_CB *);
HM_SUBSCRIPTION_CB * hm_alloc_subscription_cb();
void hm_free_subscription_cb(HM_SUBSCRIPTION_CB *);
HM_NOTIFICATION_CB * hm_alloc_notify_cb();
void hm_free_notify_cb(HM_NOTIFICATION_CB *);
//HM_TRANSPORT_CB * hm_init_transport_cb(	uint32_t );

/* hmlocmgmt.c */
int32_t hm_peer_fsm(uint32_t , HM_LOCATION_CB *);
int32_t hm_location_add(HM_LOCATION_CB *);
int32_t hm_location_update(HM_LOCATION_CB *);
int32_t hm_location_remove(HM_LOCATION_CB *);
int32_t hm_peer_keepalive_callback(void *);

/* hmcluster.c */
void hm_cluster_check_location(HM_MSG *, SOCKADDR *);
void hm_cluster_send_tick();
int32_t hm_cluster_replay_info(HM_TRANSPORT_CB *);
int32_t hm_cluster_send_end_of_replay(HM_TRANSPORT_CB *);
int32_t hm_receive_cluster_message(HM_SOCKET_CB *);
int32_t hm_cluster_process_replay(HM_PEER_MSG_REPLAY *, HM_LOCATION_CB *);
int32_t hm_cluster_send_update(void *);
int32_t hm_cluster_send_init(HM_TRANSPORT_CB *);


/* hmnodemgmt.c */
int32_t hm_node_fsm(uint32_t, HM_NODE_CB *);
int32_t hm_node_add(HM_NODE_CB *, HM_LOCATION_CB *);
int32_t hm_node_remove(HM_NODE_CB *);
int32_t hm_node_keepalive_callback(void *);

/* hmprocmgmt.c */
int32_t hm_process_add(HM_PROCESS_CB *, HM_NODE_CB *);
int32_t hm_process_update(HM_PROCESS_CB *);

/* hmglobdb.c*/
int32_t hm_global_location_add(HM_LOCATION_CB *, uint32_t);
int32_t hm_global_location_update(HM_LOCATION_CB *);
int32_t hm_global_location_remove(HM_GLOBAL_LOCATION_CB *);
int32_t hm_global_node_add(HM_NODE_CB *);
int32_t hm_global_node_update(HM_NODE_CB *);
int32_t hm_global_node_remove(HM_NODE_CB *);
int32_t hm_global_process_add(HM_PROCESS_CB *);
int32_t hm_global_process_update(HM_PROCESS_CB *);
int32_t hm_global_process_remove(HM_PROCESS_CB *);
HM_SUBSCRIPTION_CB * hm_create_subscription_entry(uint32_t, uint32_t, void *);
int32_t hm_update_subscribers(HM_SUBSCRIPTION_CB *);
int32_t hm_subscribe(uint32_t, uint32_t , void *);
int32_t hm_subscription_insert(HM_SUBSCRIPTION_CB *, HM_LIST_BLOCK *);
int32_t hm_compare_proc_tree_keys(void *, void *);

/* hmnotify.c */
int32_t hm_service_notify_queue();
HM_MSG * hm_build_notify_message(HM_NOTIFICATION_CB *);

/* hmmsg.c */
int32_t hm_recv_register(HM_MSG *, HM_TRANSPORT_CB *);
int32_t hm_recv_proc_update(HM_MSG *, HM_TRANSPORT_CB *);
int32_t hm_route_incoming_message(HM_SOCKET_CB *);

int32_t hm_tprt_handle_improper_read(int32_t , HM_TRANSPORT_CB *);
int32_t hm_receive_msg_hdr(char *);
int32_t hm_receive_msg(char *);
int32_t hm_node_send_init_rsp(HM_NODE_CB *);
int32_t hm_queue_on_transport(HM_MSG *, HM_TRANSPORT_CB *);
int32_t hm_tprt_process_outgoing_queue(HM_TRANSPORT_CB *);

/* hmtprt.c */
HM_SOCKET_CB * hm_tprt_accept_connection(int32_t);
HM_SOCKET_CB * hm_tprt_open_connection(uint32_t, void *);
int32_t hm_tprt_send_on_socket(struct sockaddr* ,int32_t ,
						uint32_t, uint8_t *, uint32_t );
int32_t hm_tprt_recv_on_socket(uint32_t , uint32_t ,
							uint8_t * , uint32_t, struct sockaddr ** );
int32_t hm_tprt_close_connection(HM_TRANSPORT_CB *);
void  hm_close_sock_connection(HM_SOCKET_CB *);

/* hmutil.c */
void avl3_balance_tree(HM_AVL3_TREE *, HM_AVL3_NODE *);
void avl3_rebalance(HM_AVL3_NODE **);
void avl3_rotate_right(HM_AVL3_NODE **);
void avl3_rotate_left(HM_AVL3_NODE **);
void avl3_swap_right_most(HM_AVL3_TREE *,
						  HM_AVL3_NODE *,
						  HM_AVL3_NODE *);
void avl3_swap_left_most(HM_AVL3_TREE *,
                                 HM_AVL3_NODE *,
                                 HM_AVL3_NODE *);
void avl3_verify_tree(HM_AVL3_TREE *,
                              const HM_AVL3_TREE_INFO *);

/* hmutil2.c */
int64_t hm_hton64(int64_t);
int64_t hm_ntoh64(int64_t);

void hm_base_timer_handler(int32_t , siginfo_t *, void *);

int32_t hm_compare_ulong(void *, void *);
int32_t hm_compare_2_ulong(void *, void *);
int32_t hm_aggregate_compare_node_id(void *, void *);
int32_t hm_aggregate_compare_pid(void *, void *);
int32_t hm_aggregate_compare_if_id(void *, void *);
HM_MSG * hm_get_buffer(uint32_t);
HM_MSG * hm_grow_buffer(HM_MSG *, uint32_t);
HM_MSG * hm_shrink_buffer(HM_MSG *, uint32_t);
int32_t hm_free_buffer(HM_MSG *);

#endif /* SRC_HMFUNC_H_ */
