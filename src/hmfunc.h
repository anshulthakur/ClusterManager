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

/* hmcbs.c */
HM_CONFIG_CB * hm_alloc_config_cb();
HM_TRANSPORT_CB * hm_init_transport_cb(	uint32_t ,
										const char *,
										uint16_t
									);
HM_SOCKET_CB * hm_alloc_sock_cb(struct sockaddr_storage *);

/* hmlocmgmt.c */
int32_t hm_init_location_cb(HM_LOCATION_CB *);

/* hmutil2.c */
int32_t hm_compare_ulong(void *, void *);
int32_t hm_compare_2_ulong(void *, void *);
#endif /* SRC_HMFUNC_H_ */
