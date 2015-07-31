/**
 *  @file hmdef.h
 *  @brief Hardware Manager Macros and Defines
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None
 */
#ifndef SRC_HMDEF_H_
#define SRC_HMDEF_H_

/***************************************************************************/
/* Maximum number of pending connections on a listen socket				   */
/***************************************************************************/
#define HM_MAX_PENDING_CONNECT_REQ 							((uint32_t)128)
#define HM_MCAST_BASE_ADDRESS								((uint32_t)200)
/***************************************************************************/
/* Group HM_NODE_ROLES: Node Roles 										   */
/***************************************************************************/
#define NODE_ROLE_PASSIVE									((uint16_t)	0)
#define NODE_ROLE_ACTIVE									((uint16_t)	1)

/***************************************************************************/
/* Group Socket Type													   */
/***************************************************************************/
#define HM_TRANSPORT_SOCK_TYPE_TCP							((uint32_t) 0)
#define HM_TRANSPORT_SOCK_TYPE_UDP							((uint32_t) 1)

/***************************************************************************/
/* Group HM_TRANSPORT_ADDR_TYPE: Transport Types						   */
/***************************************************************************/
#define HM_TRANSPORT_TCP_LISTEN								((uint16_t)	0)
#define HM_TRANSPORT_TCP_IN									((uint16_t)	1)
#define HM_TRANSPORT_TCP_OUT								((uint16_t)	2)
#define HM_TRANSPORT_UDP									((uint16_t)	3)
#define HM_TRANSPORT_MCAST									((uint16_t)	4)
#define HM_TRANSPORT_SCTP									((uint16_t)	5)
/* IPv6 variants */
#define HM_TRANSPORT_TCP_IPv6_LISTEN						((uint16_t)	6)
#define HM_TRANSPORT_TCP_IPv6_IN							((uint16_t)	7)
#define HM_TRANSPORT_TCP_IPv6_OUT							((uint16_t)	8)
#define HM_TRANSPORT_UDP_IPv6								((uint16_t)	9)
#define HM_TRANSPORT_MCAST_IPv6								((uint16_t)10)
#define HM_TRANSPORT_SCTP_IPv6								((uint16_t)11)

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
#define HM_NOTIFICATION_LOCATION_ACTIVE						((uint16_t) 9)
#define HM_NOTIFICATION_LOCATION_INACTIVE					((uint16_t) 10)

/***************************************************************************/
/* Group HM Node FSM States												   */
/***************************************************************************/
#define HM_NODE_FSM_STATE_NULL								((uint16_t) 0)
#define HM_NODE_FSM_STATE_WAITING							((uint16_t) 1)
#define HM_NODE_FSM_STATE_ACTIVE							((uint16_t) 2)
#define HM_NODE_FSM_STATE_FAILING							((uint16_t) 3)
#define HM_NODE_FSM_STATE_FAILED							((uint16_t) 4)

/***************************************************************************/
/* Number of Node FSM States											   */
/***************************************************************************/
#define HM_NODE_FSM_NUM_STATES								((uint32_t) 5)

/***************************************************************************/
/* Node FSM Signals														   */
/***************************************************************************/
#define HM_NODE_FSM_CREATE									((uint32_t) 0)
#define HM_NODE_FSM_INIT									((uint32_t) 1)
#define HM_NODE_FSM_DATA									((uint32_t) 2)
#define HM_NODE_FSM_TERM									((uint32_t) 3)
#define HM_NODE_FSM_CLOSE									((uint32_t) 4)
#define HM_NODE_FSM_TIMER_POP								((uint32_t) 5)
#define HM_NODE_FSM_TIMEOUT									((uint32_t) 6)
#define HM_NODE_FSM_FAILED									((uint32_t) 7)
#define HM_NODE_FSM_ACTIVE									((uint32_t) 8)
#define HM_NODE_FSM_NULL										0xFF
/***************************************************************************/
/* Number of Node FSM Signals											   */
/***************************************************************************/
#define HM_NODE_FSM_NUM_SIGNALS								((uint32_t) 9)

/***************************************************************************/
/* Group HM Peer FSM States												   */
/***************************************************************************/
#define HM_PEER_FSM_STATE_NULL								((uint16_t) 0)
#define HM_PEER_FSM_STATE_INIT								((uint16_t) 1)
#define HM_PEER_FSM_STATE_ACTIVE							((uint16_t) 2)
#define HM_PEER_FSM_STATE_FAILED							((uint16_t) 3)

#define HM_PEER_FSM_NUM_STATES								((uint32_t) 4)
/***************************************************************************/
/* Group: Peer FSM Signals												   */
/***************************************************************************/
#define HM_PEER_FSM_CONNECT									((uint32_t) 0)
#define HM_PEER_FSM_INIT_RCVD								((uint32_t) 1)
#define HM_PEER_FSM_LOOP									((uint32_t) 2)
#define HM_PEER_FSM_CLOSE									((uint32_t) 3)
#define HM_PEER_FSM_CLOSED									((uint32_t) 4)
#define HM_PEER_FSM_TIMER_POP								((uint32_t) 5)
/***************************************************************************/
/* Number of Peer FSM Signals											   */
/***************************************************************************/
#define HM_PEER_FSM_NUM_SIGNALS								((uint32_t) 6)

/***************************************************************************/
/* FSM Paths															   */
/***************************************************************************/
#define ACT_NO   0
#define FSM_ERR   0xFF

#define ACT_A    1
#define ACT_B    2
#define ACT_C    3
#define ACT_D    4
#define ACT_E    5
#define ACT_F    6
#define ACT_G    7
#define ACT_H    8
#define ACT_I    9
#define ACT_J    10
#define ACT_K    11
#define ACT_L    12
#define ACT_M    13
#define ACT_N    14
#define ACT_O    15
#define ACT_P    16
#define ACT_Q    17
#define ACT_R    18
#define ACT_S    19
#define ACT_T    20
#define ACT_U    21
#define ACT_V    22
#define ACT_W    23
#define ACT_X    24
#define ACT_Y    25
#define ACT_Z    26

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
/* Attribute values vocabulary											   */
/***************************************************************************/
/* Timer Resolutions */
#define HM_CONFIG_ATTR_RES_MIL_SEC							((uint32_t) 1)
#define HM_CONFIG_ATTR_RES_SEC								((uint32_t) 2)
/* Heartbeat Scopes */
#define HM_CONFIG_ATTR_SCOPE_NODE							((uint32_t) 3)
#define HM_CONFIG_ATTR_SCOPE_CLUSTER						((uint32_t) 4)
/* IP Type*/
#define HM_CONFIG_ATTR_IP_TYPE_TCP							((uint32_t) 5)
#define HM_CONFIG_ATTR_IP_TYPE_UDP							((uint32_t) 6)
#define HM_CONFIG_ATTR_IP_TYPE_MCAST						((uint32_t) 7)
/* Address Type */
#define HM_CONFIG_ATTR_ADDR_TYPE_LOCAL						((uint32_t) 8)
#define HM_CONFIG_ATTR_ADDR_TYPE_CLUSTER					((uint32_t) 9)
/* IP Versions */
#define HM_CONFIG_ATTR_IP_VERSION_4							((uint32_t) 10)
#define HM_CONFIG_ATTR_IP_VERSION_6							((uint32_t) 11)
/* Subscription types */
#define HM_CONFIG_ATTR_SUBS_TYPE_GROUP						((uint32_t) 12)
#define HM_CONFIG_ATTR_SUBS_TYPE_PROC						((uint32_t) 13)
#define HM_CONFIG_ATTR_SUBS_TYPE_IF							((uint32_t) 14)
#define HM_CONFIG_ATTR_SUBS_TYPE_LOCATION					((uint32_t) 15)
#define HM_CONFIG_ATTR_SUBS_TYPE_NODE						((uint32_t) 16)



/***************************************************************************/
/* Scope of configuration												   */
/***************************************************************************/
#define HM_CONFIG_SCOPE_CLUSTER								((uint32_t) 1)
#define HM_CONFIG_SCOPE_NODE								((uint32_t) 2)

/***************************************************************************/
/* Default values of some parameters									   */
/***************************************************************************/
#define HM_CONFIG_DEFAULT_NODE_KICKOUT						((uint32_t) 3)
#define HM_CONFIG_DEFAULT_PEER_KICKOUT						((uint32_t) 3)
#define HM_CONFIG_DEFAULT_NODE_TICK_TIME					((uint32_t) 1000)
#define HM_CONFIG_DEFAULT_PEER_TICK_TIME					((uint32_t) 1000)

#define HM_DEFAULT_TCP_LISTEN_PORT							((uint32_t) 0x8000)
#define HM_DEFAULT_UDP_COMM_PORT							((uint32_t) 0x8001)
#define HM_DEFAULT_MCAST_COMM_PORT							((uint32_t) 0x8002)
#define HM_DEFAULT_MCAST_GROUP								((uint32_t) 3)


/***************************************************************************/
/* Table Types															   */
/***************************************************************************/
#define HM_TABLE_TYPE_NODES									((uint32_t) 1)
#define HM_TABLE_TYPE_PROCESS								((uint32_t) 2)
#define HM_TABLE_TYPE_IF									((uint32_t) 3)
#define HM_TABLE_TYPE_LOCATION								((uint32_t) 4)
#define HM_TABLE_TYPE_GROUP									((uint32_t) 5)

#define HM_TABLE_TYPE_NODES_LOCAL							((uint32_t) 6)
#define HM_TABLE_TYPE_PROCESS_LOCAL							((uint32_t) 7)
#define HM_TABLE_TYPE_IF_LOCAL								((uint32_t) 8)
#define HM_TABLE_TYPE_LOCATION_LOCAL						((uint32_t) 9)


/***************************************************************************/
/* Process Status														   */
/***************************************************************************/
#define HM_STATUS_DOWN										((uint32_t) 0)
#define HM_STATUS_PENDING									((uint32_t) 1)
#define HM_STATUS_RUNNING									((uint32_t) 2)

#endif /* SRC_HMDEF_H_ */
