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
int32_t hm_parse_config(HM_CONFIG_CB *, char *);
void hm_interrupt_handler(int32_t , siginfo_t *, void *);
void hm_terminate();

/* hmconf.c */
int32_t hm_parse_config(HM_CONFIG_CB *, char *);

/* hmcbs.c */
HM_CONFIG_CB * hm_alloc_config_cb();
HM_TRANSPORT_CB * hm_init_transport_cb(	uint32_t );
int32_t hm_free_transport_cb(HM_TRANSPORT_CB *);
HM_SOCKET_CB * hm_alloc_sock_cb();
void hm_free_sock_cb(HM_SOCKET_CB *);
HM_NODE_CB * hm_alloc_node_cb(uint32_t);
int32_t hm_free_node_cb(HM_NODE_CB *);
int32_t hm_free_process_cb(HM_PROCESS_CB *);
HM_TRANSPORT_CB * hm_alloc_transport_cb(uint32_t);

/* hmlocmgmt.c */
int32_t hm_init_location_cb(HM_LOCATION_CB *);

/* hmnodemgmt.c */
int32_t hm_node_add(HM_NODE_CB *, HM_LOCATION_CB *);
int32_t hm_node_keepalive_callback(void *);

/* hmtprt.c */
HM_SOCKET_CB * hm_tprt_open_connection(uint32_t, void *);

/* hmutil2.c */
int32_t hm_compare_ulong(void *, void *);
int32_t hm_compare_2_ulong(void *, void *);
int32_t hm_aggregate_compare_node_id(void *, void *);
int32_t hm_aggregate_compare_pid(void *, void *);
int32_t hm_aggregate_compare_if_id(void *, void *);

#endif /* SRC_HMFUNC_H_ */
