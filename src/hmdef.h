/*
 * hmdef.h
 *
 *	Hardware Manager Macros and Defines
 *
 *  Created on: 29-Apr-2015
 *      Author: anshul
 */

#ifndef SRC_HMDEF_H_
#define SRC_HMDEF_H_

/***************************************************************************/
/* Maximum number of pending connections on a listen socket				   */
/***************************************************************************/
#define HM_MAX_PENDING_CONNECT_REQ 							((uint32_t)128)

/***************************************************************************/
/* Group HM_NODE_ROLES: Node Roles 										   */
/***************************************************************************/
#define NODE_ROLE_PASSIVE									((uint16_t)	0)
#define NODE_ROLE_ACTIVE									((uint16_t)	1)

/***************************************************************************/
/* Group HM_TRANSPORT_ADDR_TYPE: Transport Types						   */
/***************************************************************************/
#define HM_TRANSPORT_TCP_LISTEN								((uint16_t)	0)
#define HM_TRANSPORT_TCP_IO									((uint16_t)	1)
#define HM_TRANSPORT_UDP									((uint16_t)	2)
#define HM_TRANSPORT_MCAST									((uint16_t)	3)
#define HM_TRANSPORT_SCTP									((uint16_t)	4)
/* IPv6 variants */
#define HM_TRANSPORT_TCP_IPv6_LISTEN						((uint16_t)	5)
#define HM_TRANSPORT_TCP_IPv6_IO							((uint16_t)	6)
#define HM_TRANSPORT_UDP_IPv6								((uint16_t)	7)
#define HM_TRANSPORT_MCAST_IPv6								((uint16_t)	8)
#define HM_TRANSPORT_SCTP_IPv6								((uint16_t)	9)

#define HM_TRANSPORT_DEFAULT						HM_TRANSPORT_TCP_LISTEN

/***************************************************************************/
/* Group HM_TRANSPORT_CONNECTION_STATES: 								   */
/***************************************************************************/
#define HM_TPRT_CONN_NULL									((uint16_t) 0)
#define HM_TPRT_CONN_INIT									((uint16_t) 1)
#define HM_TPRT_CONN_ACTIVE									((uint16_t) 2)
#define HM_TPRT_CONN_DOWN									((uint16_t) 3)

/***************************************************************************/
/* Group HM_NOTIFICATION_TYPES											   */
/***************************************************************************/
#define HM_NOTIFICATION_NODE_ACTIVE							((uint16_t) 1)
#define HM_NOTIFICATION_NODE_INACTIVE						((uint16_t) 2)
#define HM_NOTIFICATION_PROCESS_CREATED						((uint16_t) 3)
#define HM_NOTIFICATION_PROCESS_DESTROYED					((uint16_t) 4)
#define HM_NOTIFICATION_INTERFACE_ADDED						((uint16_t) 5)
#define HM_NOTIFICATION_INTERFACE_DELETE					((uint16_t) 6)
#define HM_NOTIFICATION_SUBSCRIBE							((uint16_t) 7)
#define HM_NOTIFICATION_UNSUBSCRIBE							((uint16_t) 8)


/***************************************************************************/
/* Group HM Node FSM States												   */
/***************************************************************************/
#define HM_NODE_FSM_STATE_NULL								((uint16_t) 0)
#define HM_NODE_FSM_STATE_STARTING							((uint16_t) 1)
#define HM_NODE_FSM_STATE_WAITING							((uint16_t) 2)
#define HM_NODE_FSM_STATE_INIT								((uint16_t) 3)
#define HM_NODE_FSM_STATE_ACTIVE							((uint16_t) 4)
#define HM_NODE_FSM_STATE_FAILED							((uint16_t) 5)


/***************************************************************************/
/* Group HM Peer FSM States												   */
/***************************************************************************/
#define HM_PEER_FSM_STATE_NULL								((uint16_t) 0)
#define HM_PEER_FSM_STATE_WAITING							((uint16_t) 1)
#define HM_PEER_FSM_STATE_SYNC								((uint16_t) 2)
#define HM_PEER_FSM_STATE_ACTIVE							((uint16_t) 3)
#define HM_PEER_FSM_STATE_FAILED							((uint16_t) 4)


/***************************************************************************/
/* Parsing utility 														   */
/***************************************************************************/
#define 	MAX_STACK_SIZE									((uint32_t)127)

/***************************************************************************/
/* Node types															   */
/***************************************************************************/

#define 	HM_CONFIG_ROOT									((uint32_t) 1)
#define 	HM_CONFIG_HM_INSTANCE							((uint32_t) 2)
#define 	HM_CONFIG_HEARTBEAT								((uint32_t) 3)
#define 	HM_CONFIG_ADDRESS								((uint32_t) 4)
#define 	HM_CONFIG_NODE_TREE								((uint32_t) 5)
#define 	HM_CONFIG_NODE_INSTANCE							((uint32_t) 6)
#define 	HM_CONFIG_SUBSCRIPTION_TREE						((uint32_t) 7)
#define 	HM_CONFIG_PERIOD								((uint32_t) 8)
#define 	HM_CONFIG_THRESHOLD								((uint32_t) 9)
#define 	HM_CONFIG_INDEX									((uint32_t) 10)
#define 	HM_CONFIG_NAME									((uint32_t) 11)
#define 	HM_CONFIG_IP									((uint32_t) 12)
#define 	HM_CONFIG_PORT									((uint32_t) 13)
#define 	HM_CONFIG_GROUP									((uint32_t) 14)
#define 	HM_CONFIG_ROLE									((uint32_t) 15)
#define 	HM_CONFIG_SUBSCRIPTION_INSTANCE					((uint32_t) 16)


/***************************************************************************/
/* Hearbeat Scope														   */
/***************************************************************************/
#define HM_CONFIG_HEARTBEAT_SCOPE_CLUSTER					((uint32_t) 1)
#define HM_CONFIG_HEARTBEAT_SCOPE_NODE						((uint32_t) 2)

#endif /* SRC_HMDEF_H_ */
