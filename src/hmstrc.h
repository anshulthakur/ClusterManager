/*
 * hmstrc.h
 *
 *  Purpose: Structures for Hardware Manager.
 *
 *  Created on: 29-Apr-2015
 *      Author: Anshul
 *
 */

#ifndef SRC_HMSTRC_H_
#define SRC_HMSTRC_H_

/***************************************************************************/
/* Generic Utility Structure Composites									   */
/***************************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_STACK														 */
/*                                                                           */
/* Name:      hm_stack					 									 */
/*                                                                           */
/* Textname:  Stack Element	                                                 */
/*                                                                           */
/* Description: 						            						 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_stack
{
	/***************************************************************************/
	/* Pointer to structure saved											   */
	/***************************************************************************/
	void *self;

	/***************************************************************************/
	/* Index of top of stack												   */
	/***************************************************************************/
	int32_t top;

	/***************************************************************************/
	/* Size of stack														   */
	/***************************************************************************/
	int32_t size;

	/***************************************************************************/
	/* Stack of pointers. 													   */
	/***************************************************************************/
	void **stack;
} HM_STACK ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_CONFIG_NODE												 */
/*                                                                           */
/* Name:      hm_config_node			 									 */
/*                                                                           */
/* Textname:  Configuration Node                                             */
/*                                                                           */
/* Description: Configuration Node in XML file					             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_config_node
{
	/***************************************************************************/
	/* Type of Node															   */
	/***************************************************************************/
	uint32_t type;

	/***************************************************************************/
	/* Pointer to element													   */
	/***************************************************************************/
	void *self;

	/***************************************************************************/
	/* Opaque data for keeping some information								   */
	/***************************************************************************/
	void *opaque;

} HM_CONFIG_NODE ;
/**STRUCT-********************************************************************/

/*****************************************************************************/
/* compare function for trees.                                               */
/*****************************************************************************/
typedef int32_t(AVL3_COMPARE)(void *, void *);

/**STRUCT+********************************************************************/
/* Structure: HM_LQE														 */
/*                                                                           */
/* Name:      hm_lqe			 										 	 */
/*                                                                           */
/* Textname:  Linear Queue Element	                                         */
/*                                                                           */
/* Description: A linear linked list element for circular queues.            */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_lqe
{
    void *self;
    struct hm_lqe *next;
    struct hm_lqe *prev;

} HM_LQE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_AVL3_NODE													 */
/*                                                                           */
/* Name:      hm_avl3_node			 										 */
/*                                                                           */
/* Textname:  AVL Tree Node	                                                 */
/*                                                                           */
/* Description: A Balanced Binary Tree node.					             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_avl3_node {
	struct hm_avl3_node *parent;

	struct hm_avl3_node *left;
	int32_t left_height;

	struct hm_avl3_node *right;
	int32_t right_height;

	void *self;
} HM_AVL3_NODE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_AVL3_TREE													 */
/*                                                                           */
/* Name:      hm_avl3_tree			 										 */
/*                                                                           */
/* Textname:  AVLL Tree root                                                 */
/*                                                                           */
/* Description: AVL Tree node							            		 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_avl3_tree {
	HM_AVL3_NODE *root;
	HM_AVL3_NODE *first;
	HM_AVL3_NODE *last;
} HM_AVL3_TREE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_AVL3_TREE_INFO                                              */
/*                                                                           */
/* Description: AVL3 tree information.                                       */
/*****************************************************************************/

typedef struct hm_avl3_tree_info
{
  AVL3_COMPARE *compare;
  uint16_t key_offset;
  uint16_t node_offset;
} HM_AVL3_TREE_INFO;

/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_INET_ADDRESS												 */
/*                                                                           */
/* Name:      hm_inet_address			 									 */
/*                                                                           */
/* Textname:  Internet Address	                                             */
/*                                                                           */
/* Description: Internet address structure for internal consumption in a     */
/* a generic manner.														 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_inet_address
{
	/***************************************************************************/
	/* One of HM_TRANSPORT_ADDR_TYPE 										   */
	/***************************************************************************/
    uint16_t type;
    struct sockaddr_storage address;
} HM_INET_ADDRESS ;
/**STRUCT-********************************************************************/

/***************************************************************************/
/* Hardware Manager Implementation Specific Structures					   */
/***************************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_SOCKET_CB													 */
/*                                                                           */
/* Name:      hm_socket_cb			 										 */
/*                                                                           */
/* Textname:  Socket Connection Control Block.	                             */
/*                                                                           */
/* Description: This entity represents a socket connection CB. 				 */
/*                                                                           */
/*****************************************************************************/

typedef struct hm_socket_cb {
	/***************************************************************************/
	/* Socket Descriptor: Can be negative									   */
	/***************************************************************************/
	int32_t sock_fd;

	/***************************************************************************/
	/* Socket Address Structure												   */
	/* It must be IPv4/v6 agnostic as a structure.							   */
	/***************************************************************************/
	struct sockaddr addr;

	/***************************************************************************/
	/* Connection State (in transport layer, application independent)		   */
	/* One of: HM_TRANSPORT_CONNECTION_STATES								   */
	/***************************************************************************/
	uint32_t conn_state;

	/***************************************************************************/
	/* Parent Transport Connection CB										   */
	/***************************************************************************/
	struct transport_cb *tprt_cb;

} HM_SOCKET_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_TRANSPORT_CB												 */
/*                                                                           */
/* Name:      hm_transport_cb			 									 */
/*                                                                           */
/* Textname:  Transport Connection Control Block                             */
/*                                                                           */
/* Description: This structure represents a network communication endpoint   */
/* for rest of the application. There is at least a common Methods API which */
/* is supported on all types of interface.									 */
/* Further, it contains all the information about the current state of the   */
/* connection.																 */
/*                                                                           */
/*****************************************************************************/

typedef struct hm_transport_cb {
	/***************************************************************************/
	/* Type of Transport: One of HM_TRANSPORT_ADDR_TYPE						   */
	/***************************************************************************/
	uint16_t type;

	/***************************************************************************/
	/* Socket Connection Control Block, the lower level CB 					   */
	/***************************************************************************/
	HM_SOCKET_CB *sock_cb;

	/***************************************************************************/
	/* Parent Location CB. Cannot be NULL 									   */
	/* Would be a Listen Socket, or P2P socket.								   */
	/***************************************************************************/
	struct hm_location_cb *location_cb;

	/***************************************************************************/
	/* Parent Node CB.														   */
	/***************************************************************************/
	struct hm_node_cb *node_cb;

} HM_TRANSPORT_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_LOCATION_CB												 */
/*                                                                           */
/* Name:      hm_location_cb			 									 */
/*                                                                           */
/* Textname:  Hardware Location Control Block	                             */
/*                                                                           */
/* Description: A Hardware Location Control Block represents a Physical      */
/* Hardware Location on the cluster network.								 */
/* A Hardware Location is uniquely determined by its Hardware Index			 */
/* Which may be derived from the Slot ID on the board, or some Platform 	 */
/* specific API. It MUST be unique for proper function and the user must 	 */
/* ensure that.																 */
/* A hardware location can contain many nodes, which are running software 	 */
/* instances of processes of a certain service group.						 */
/* These groups can be, for a routing solution, Control Plane, Management	 */
/* Plane, Data Plane Software, Data Plane Platform, Fault Monitoring, etc.   */
/* These nodes may be active or backup, based on the configuration			 */
/* but usually, it would be the Role of the Management Plane which would	 */
/* determine the role of its subordinate nodes.								 */
/*                                                                           */
/*****************************************************************************/

typedef struct hm_location_cb {
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Hardware Location Index for this Node.								   */
	/***************************************************************************/
	uint8_t index;

	/***************************************************************************/
	/* Parameters for TCP Address (listen type) of this node				   */
	/***************************************************************************/
	HM_INET_ADDRESS tcp_addr; /* TCP Address */
	uint16_t tcp_port;
	HM_TRANSPORT_CB *node_listen_cb;

	/***************************************************************************/
	/* Parameters for UDP Address (cluster peer unicast) of this node.		   */
	/***************************************************************************/
	HM_INET_ADDRESS udp_addr; /* UDP Address */
	uint16_t udp_port;
	HM_TRANSPORT_CB *peer_listen_cb;

	/***************************************************************************/
	/* Parameters for UDP Address (cluster general) Multicast of this node.	   */
	/***************************************************************************/
	HM_INET_ADDRESS mcast_addr; /* Multicast Address */
	uint16_t mcast_port;
	uint16_t mcast_group;
	HM_TRANSPORT_CB *peer_broadcast_cb;

	/***************************************************************************/
	/* List of other known peers											   */
	/***************************************************************************/
	HM_LQE *peer_list;

	/***************************************************************************/
	/* List of Transport Connection CBs that belong to this Location.		   */
	/* These mainly belong to the Node CBs running on this Location			   */
	/***************************************************************************/
	HM_LQE transport_cb_list;

	/***************************************************************************/
	/* Nodes on this location 												   */
	/***************************************************************************/
	HM_AVL3_TREE node_tree;

	/***************************************************************************/
	/* Peer FSM	State														   */
	/***************************************************************************/
	uint16_t fsm_state;

	/***************************************************************************/
	/* Keepalive Period														   */
	/***************************************************************************/
	uint32_t keepalive_period;

	/***************************************************************************/
	/* Missed Keepalive Count												   */
	/***************************************************************************/
	uint32_t keepalive_missed;

} HM_LOCATION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_NODE_CB													 */
/*                                                                           */
/* Name:      hm_node_cb			 										 */
/*                                                                           */
/* Textname:  HM Node Control Block	                                         */
/*                                                                           */
/* Description: Each Hardware Location CB can contain many Nodes on it which */
/* are called Node Control Blocks. Each Node CB represents a Binary running  */
/* NBASE instance. The nodes can overall be active/backups, though the role  */
/* is governed by the Management Plane Node for now.						 */
/* It is possible that a node on one Hardware Location has its MP on a 		 */
/* different Hardware Location.												 */
/* For each Node, the HM will have only one TCP connection with it over which*/
/* it is going to pass control queries as well as heartbeats for liveness	 */
/* detection.																 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_node_cb {
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Location index														   */
	/***************************************************************************/
	uint16_t index;

	/***************************************************************************/
	/* Location Group Index													   */
	/***************************************************************************/
	uint16_t group;

	/***************************************************************************/
	/* Active/Passive													       */
	/***************************************************************************/
	uint8_t role;

	/***************************************************************************/
	/* Node String name													   	   */
	/***************************************************************************/
	unsigned char name[25];

	/***************************************************************************/
	/* Control Block representing Transport Connection to this node.		   */
	/***************************************************************************/
	HM_TRANSPORT_CB *transport_cb;

	/***************************************************************************/
	/* Parent Hardware Location Control Block pointer						   */
	/***************************************************************************/
	HM_LOCATION_CB *parent_location_cb;

	/***************************************************************************/
	/* List of Nodes which must be notified if something happens to this node  */
	/***************************************************************************/
	HM_LQE *subscribers;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE *subscriptions;

	/***************************************************************************/
	/* Tree of Processes Running on this Node								   */
	/***************************************************************************/
	HM_AVL3_TREE process_tree;

	/***************************************************************************/
	/* Tree of Interfaces supported by Processes running on this node.		   */
	/***************************************************************************/
	HM_AVL3_TREE interface_tree;

	/***************************************************************************/
	/* Direct Partner (redundancy)											   */
	/* Could be the active for backup, or backup for active					   */
	/***************************************************************************/
	struct hm_node_cb *partner;

	/***************************************************************************/
	/* Node FSM	State														   */
	/***************************************************************************/
	uint16_t fsm_state;

	/***************************************************************************/
	/* Keepalive Period														   */
	/***************************************************************************/
	uint32_t keepalive_period;

	/***************************************************************************/
	/* Missed Keepalive Count												   */
	/***************************************************************************/
	uint32_t keepalive_missed;



} HM_NODE_CB ;
/**STRUCT-********************************************************************/


/**STRUCT+********************************************************************/
/* Structure: HM_PROCESS_CB													 */
/*                                                                           */
/* Name:      hm_process_cb			 										 */
/*                                                                           */
/* Textname:  Process Control Block                                          */
/*                                                                           */
/* Description: Represents a Process Entity in Hardware Manager Tables.      */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_process_cb
{
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Process type															   */
	/***************************************************************************/
	uint32_t type;

	/***************************************************************************/
	/* Process Identifier (PID) of the process								   */
	/***************************************************************************/
	uint32_t pid;

	/***************************************************************************/
	/* Process String name													   */
	/***************************************************************************/
	unsigned char name[25];

	/***************************************************************************/
	/* Parent Node 															   */
	/***************************************************************************/
	HM_NODE_CB *parent_node_cb;

	/***************************************************************************/
	/* Node in Process Tree													   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Interfaces list 														   */
	/***************************************************************************/
	HM_LQE interfaces_list;

	/***************************************************************************/
	/* Active/Backup. This value is usually the same as that in Node CB		   */
	/***************************************************************************/
	uint8_t role;

	/***************************************************************************/
	/* Direct Partner (redundancy)											   */
	/* Could be the active for backup, or backup for active					   */
	/***************************************************************************/
	struct hm_process_cb *partner;

	/***************************************************************************/
	/* List of Nodes which must be notified if something happens to this node  */
	/***************************************************************************/
	HM_LQE *subscribers;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE *subscriptions;

} HM_PROCESS_CB ;
/**STRUCT-********************************************************************/


/**STRUCT+********************************************************************/
/* Structure: HM_SUBSCRIPTION_CB											 */
/*                                                                           */
/* Name:      hm_subscription_cb			 								 */
/*                                                                           */
/* Textname:  Hardware Manager Subscription Control Block                    */
/*                                                                           */
/* Description: This structure represents a typical Relational Database Row  */
/* It is Keyed by the TypeOfTable and idInTable fields which identify the 	 */
/* subscription point on which other nodes/processes may subscribe on.		 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_subscription_cb
{
	/***************************************************************************/
	/* DB Index (used in pending subscription tree only)					   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Tree Node															   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Table on which subscription is being made							   */
	/***************************************************************************/
	uint32_t table_type;

	/***************************************************************************/
	/* ID of Row on which subscription is being made						   */
	/***************************************************************************/
	uint32_t row_id;

	/***************************************************************************/
	/* Pointer to the row for fast access									   */
	/***************************************************************************/
	void *row_cb;

	/***************************************************************************/
	/* List of subscribers													   */
	/* NOTE: WHY NOT USE A PURE RELATIONAL DB MODEL and accessing by walking   */
	/* the table when subscribers need to be notified?						   */
	/* Because notifications need to be sent fast, and traversing a linked list*/
	/* would be faster than a tree traversal.								   */
	/***************************************************************************/
	HM_LQE subscribers_list;

} HM_SUBSCRIPTION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_NOTIFICATION_CB											 */
/*                                                                           */
/* Name:      hm_notification_cb			 								 */
/*                                                                           */
/* Textname:  Notification Control Block                                     */
/*                                                                           */
/* Description: A Notification Event which needs to be processed. This would */
/* be used differently in different perspectives. The same notification can  */
/* be used to broadcast an update to the peers and a notification to the 	 */
/* subscribed nodes.														 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_notification_cb
{
	/***************************************************************************/
	/* Node in list															   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Notification Type													   */
	/***************************************************************************/
	uint16_t notification_type;

	/***************************************************************************/
	/* Affected CB															   */
	/***************************************************************************/
	void *node_cb;

	/***************************************************************************/
	/* Notification State: User specific.									   */
	/***************************************************************************/
	void *custom_data;

} HM_NOTIFICATION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_ADDRESS_CB		*/
/*                                                                           */
/* Name:      hm_address_cb			 */
/*                                                                           */
/* Textname:  Configuration Address CB	                                                 */
/*                                                                           */
/* Description: 						            */
/*                                                                           */
/*****************************************************************************/
#define HM_ADDRESS_SCOPE_LOCAL		1
#define HM_ADDRESS_SCOPE_REMOTE		2
typedef struct hm_address_cb
{
	HM_INET_ADDRESS address;
	uint32_t scope;
	uint32_t port;
} HM_ADDRESS_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_CONFIG_CB													 */
/*                                                                           */
/* Name:      hm_config_cb			 										 */
/*                                                                           */
/* Textname:  HM Configuration Control Block                                 */
/*                                                                           */
/* Description: The data structure representing user configuration set.      */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_config_cb
{
	struct hm_instance_info {
		uint32_t index;

		struct hm_heartbeat_config {
			uint32_t scope;
			uint32_t timer_val;
			uint32_t threshold;
		} node, cluster;


	} instance_info;
} HM_CONFIG_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_GLOBAL_DATA												 */
/*                                                                           */
/* Name:      hm_global_data			 									 */
/*                                                                           */
/* Textname:  Hardware Manager Global Data                                   */
/*                                                                           */
/* Description: Global Tables that serve as overall statistics and 			 */
/* subscription base. These tables reflect the aggregated state information	 */
/* of the overall system.													 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_global_data
{
	/***************************************************************************/
	/* Locations Tree: Keyed by Hardware Location Indices					   */
	/***************************************************************************/
	HM_AVL3_TREE locations_tree;

	/***************************************************************************/
	/* Nodes Tree: Keyed by Node Location indices (MUST BE UNIQUE)			   */
	/***************************************************************************/
	HM_AVL3_TREE nodes_tree;
	uint32_t next_node_tree_id;

	/***************************************************************************/
	/* Process Instances Tree: Keyed by PCT_Types. The processes may or may not*/
	/* be running at the time of entry. 									   */
	/***************************************************************************/
	HM_AVL3_TREE process_tree;
	uint32_t next_process_tree_id;

	/***************************************************************************/
	/* Process Instances Tree: Keyed by PIDs of Processes					   */
	/* Will contain only activly running instances.							   */
	/***************************************************************************/
	HM_AVL3_TREE pid_tree;

	/***************************************************************************/
	/* Tree of all interfaces supported by processes.						   */
	/* Keyed by IF_ID, Location ID, PID										   */
	/***************************************************************************/
	HM_AVL3_TREE interface_tree;
	uint32_t next_interface_tree_id;

	/***************************************************************************/
	/* Tree of all P2P relationships that are currently active. 			   */
	/***************************************************************************/
	HM_AVL3_TREE active_subscriptions_tree;

	/***************************************************************************/
	/* Tree of all P2P relationships which are currently inactive			   */
	/***************************************************************************/
	HM_AVL3_TREE pending_subscriptions_tree;
	uint32_t next_pending_tree_id;

	/***************************************************************************/
	/* Queue of Notifications that need to be sent							   */
	/***************************************************************************/
	HM_LQE notification_queue;

	/***************************************************************************/
	/* Local Location is represented as a static structure.					   */
	/***************************************************************************/
	HM_LOCATION_CB local_location_cb;

	/***************************************************************************/
	/* Hardware Manager Configuration										   */
	/***************************************************************************/
	uint32_t node_keepalive_period; /* Configured/Default Keepalive period in ms */
	uint32_t node_kickout_value;	/* After how many ticks to declare node down */

	uint32_t peer_keepalive_period; /* Configured/Default Keepalive period in ms */
	uint32_t peer_kickout_value;	/* After how many ticks to declare node down */

	/***************************************************************************/
	/* User Configuration Data												   */
	/***************************************************************************/
	HM_CONFIG_CB *config_data;

} HM_GLOBAL_DATA ;
/**STRUCT-********************************************************************/

#endif /* SRC_HMSTRC_H_ */
