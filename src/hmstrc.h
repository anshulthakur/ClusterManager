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

typedef struct hm_tprt_fsm_entry
{
	uint32_t next_state;
	uint32_t path;
}HM_TPRT_FSM_ENTRY;

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
/* Structure: HM_AVL3_GEN_NODE												 */
/*                                                                           */
/* Name:      hm_avl3_gen_node			 									 */
/*                                                                           */
/* Textname:  AVL3 Generic Node                                              */
/*                                                                           */
/* Description: A generic Node to enable having a single node in multiple    */
/* trees without having to put a AVL3_NODE explicitly in the structure.		 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_avl3_gen_node
{
	/***************************************************************************/
	/* Node in tree															   */
	/***************************************************************************/
	HM_AVL3_NODE tree_node;

	/***************************************************************************/
	/* Pointer to the actual data-structure									   */
	/***************************************************************************/
	void *parent;

	/***************************************************************************/
	/* Pointer to Key value													   */
	/***************************************************************************/
	void *key;
} HM_AVL3_GEN_NODE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_LIST_BLOCK													 */
/*                                                                           */
/* Name:      hm_list_block			 										 */
/*                                                                           */
/* Textname:  HM List Block	                                                 */
/*                                                                           */
/* Description: A generic LQE block which can contain pointer to a different */
/* location.																 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_list_block
{
	HM_LQE node;

	void * target;

	/***************************************************************************/
	/* Opaque data for freestyle saving.									   */
	/* Currently used in Notifications to mark as sent.						   */
	/***************************************************************************/
	void *opaque;
} HM_LIST_BLOCK ;
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
    struct sockaddr_storage address;
    char addr_str[128];
	/***************************************************************************/
	/* One of HM_TRANSPORT_ADDR_TYPE 										   */
	/***************************************************************************/
    uint16_t type;
    uint16_t mcast_group;
} HM_INET_ADDRESS ;
/**STRUCT-********************************************************************/

typedef int32_t(HM_TIMER_CALLBACK)(void *timer_cb);

/**STRUCT+********************************************************************/
/* Structure: HM_TIMER_CB													 */
/*                                                                           */
/* Name:      hm_timer_cb			 										 */
/*                                                                           */
/* Textname:  Timer Control Block                                            */
/*                                                                           */
/* Description: Represents a timer in the HM					             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_timer_cb
{
	/***************************************************************************/
	/* Timer Handle value													   */
	/***************************************************************************/
	uint32_t handle; /* Contains timer_t */

	/***************************************************************************/
	/* Pointer to parent of this timer.										   */
	/***************************************************************************/
	void *parent;

	/***************************************************************************/
	/* Callback routine														   */
	/***************************************************************************/
	HM_TIMER_CALLBACK *callback;

	/***************************************************************************/
	/* Timer in OS															   */
	/***************************************************************************/
	timer_t timerID;

	/***************************************************************************/
	/* Repeat timer															   */
	/***************************************************************************/
	uint32_t repeat;

	/***************************************************************************/
	/* Currently running?													   */
	/***************************************************************************/
	uint32_t running;

	/***************************************************************************/
	/* Timer value															   */
	/***************************************************************************/
	uint32_t period;

	/***************************************************************************/
	/* Node in Timer Tree													   */
	/***************************************************************************/
	HM_AVL3_NODE node;
} HM_TIMER_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_SOCKADDR_UNION												 */
/*                                                                           */
/* Name:      hm_sockaddr_union			 									 */
/*                                                                           */
/* Textname:  Union of Sockaddr Types	                                     */
/*                                                                           */
/* Description: 						            						 */
/*                                                                           */
/*****************************************************************************/
typedef union hm_sockaddr_union
{
	struct sockaddr sock_addr;
	struct sockaddr_in in_addr;
	struct sockaddr_in6 in6_addr;
} HM_SOCKADDR_UNION ;
/**STRUCT-********************************************************************/
/***************************************************************************/
/* Hardware Manager Implementation Specific Structures					   */
/***************************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_MSG														 */
/*                                                                           */
/* Name:      hm_msg			 											 */
/*                                                                           */
/* Textname:  Hardware Manager internal queuing message header.	             */
/*                                                                           */
/* Description: 						            */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_msg
{
	/***************************************************************************/
	/* Node to queue into buffer.											   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Number of references still active for this message					   */
	/* Will be freed only if references drops to 0							   */
	/***************************************************************************/
	uint32_t ref_count;

	/***************************************************************************/
	/* Length of the message appended.										   */
	/***************************************************************************/
	uint32_t msg_len;

	/***************************************************************************/
	/* Pointer to message. (It might be a remote pointer, or at offset from    */
	/* the beginning of this structure.										   */
	/***************************************************************************/
	void *msg;

} HM_MSG ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_SUBSCRIBER													 */
/*                                                                           */
/* Name:      hm_subscriber			 										 */
/*                                                                           */
/* Textname:  HM Subscriber	                                                 */
/*                                                                           */
/* Description: This is not a structure, but a union of subscriber types     */
/* It exists only for convenience of casting.								 */
/*                                                                           */
/*****************************************************************************/
typedef union hm_subscriber
{
	/***************************************************************************/
	/* Just a void															   */
	/***************************************************************************/
	void *void_cb;

	/***************************************************************************/
	/* Global Location CB													   */
	/***************************************************************************/
	struct hm_global_location_cb *location_cb;

	/***************************************************************************/
	/* Global Node CB Type													   */
	/***************************************************************************/
	struct hm_global_node_cb *node_cb;

	/***************************************************************************/
	/* Global Process CB													   */
	/***************************************************************************/
	struct hm_global_process_cb *process_cb;

	/***************************************************************************/
	/* Location CB															   */
	/***************************************************************************/
	struct hm_location_cb *proper_location_cb;

	/***************************************************************************/
	/* Node CB Type															   */
	/***************************************************************************/
	struct hm_node_cb *proper_node_cb;

	/***************************************************************************/
	/* Process CB															   */
	/***************************************************************************/
	struct hm_process_cb *proper_process_cb;
} HM_SUBSCRIBER ;
/**STRUCT-********************************************************************/


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
	/* Node element in a global connections list							   */
	/***************************************************************************/
	HM_LQE node;

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
	/* Socket Type															   */
	/* Although we could have fetched it from Kernel by getsockopt(SOCK_TYPE)  */
	/***************************************************************************/
	int32_t sock_type;

	/***************************************************************************/
	/* Connection State (in transport layer, application independent)		   */
	/* One of: HM_TRANSPORT_CONNECTION_STATES								   */
	/***************************************************************************/
	uint32_t conn_state;

	/***************************************************************************/
	/* Parent Transport Connection CB										   */
	/***************************************************************************/
	struct hm_transport_cb *tprt_cb;
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
	/* Transport Address													   */
	/* Note that this structure must be replaced by a union of all transport   */
	/* structures if providing multiple transport types						   */
	/***************************************************************************/
	HM_INET_ADDRESS address;

	/***************************************************************************/
	/* Parent Location CB. Cannot be NULL 									   */
	/* Would be a Listen Socket, or P2P socket.								   */
	/***************************************************************************/
	struct hm_location_cb *location_cb;

	/***************************************************************************/
	/* Parent Node CB.														   */
	/***************************************************************************/
	struct hm_node_cb *node_cb;

	/***************************************************************************/
	/* Pointer to incoming buffer											   */
	/***************************************************************************/
	char *in_buffer;

	/***************************************************************************/
	/* Number of bytes pending on In Buffer									   */
	/***************************************************************************/
	int32_t in_bytes;

	/***************************************************************************/
	/* Pointer to outgoing buffer											   */
	/***************************************************************************/
	char *out_buffer;

	/***************************************************************************/
	/* Outgoing buffers (Pending). HM_MSG structures must be appended on top.  */
	/***************************************************************************/
	HM_LQE pending;

	/***************************************************************************/
	/* A single MSG_HEADER structure pre-allocated so that we always have some */
	/* memory available to read the message header. Saves two small mallocs on */
	/* demand																   */
	/* When descriptor is set, we write the header from socket into this memory*/
	/* and then, on demand, we allocate the mentioned data size memory and copy*/
	/* this message header into the beginning of that memory and rest of the   */
	/* message in the remaining buffer.	This is better than malloc-ing a header*/
	/* sized memory when data arrives, and then a bigger chunk later when the  */
	/* header is parsed.													   */
	/***************************************************************************/
	union msg_header{
		HM_MSG_HEADER node_header;
		HM_PEER_MSG_UNION peer_msg;
	} header;

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
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Pointer to its CB in DB												   */
	/***************************************************************************/
	void *db_ptr;

	/***************************************************************************/
	/* Hardware Location Index for this Node.								   */
	/***************************************************************************/
	uint32_t index;

	/***************************************************************************/
	/* Parameters for Local Node Address  of this node				   		   */
	/***************************************************************************/
	HM_TRANSPORT_CB *node_listen_cb;

	/***************************************************************************/
	/* Parameters for Cluster Address (cluster peer) of this node.		   	   */
	/***************************************************************************/
	HM_TRANSPORT_CB *peer_listen_cb;

	/***************************************************************************/
	/* Parameters for Cluster Address (cluster general) Multicast of this node.*/
	/***************************************************************************/
	HM_TRANSPORT_CB *peer_broadcast_cb;

	/***************************************************************************/
	/* List of other known peers											   */
	/***************************************************************************/
	HM_LQE peer_list;

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
	/* Timer to send keepalive on cluster (For local only)					   */
	/***************************************************************************/
	HM_TIMER_CB *timer_cb;

	/***************************************************************************/
	/* Missed Keepalive Count												   */
	/***************************************************************************/
	uint32_t keepalive_missed;

	/***************************************************************************/
	/* Number of active nodes												   */
	/***************************************************************************/
	uint32_t active_nodes;

	/***************************************************************************/
	/* Number of active processes											   */
	/***************************************************************************/
	uint32_t active_processes;

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
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Pointer to its CB in DB												   */
	/***************************************************************************/
	void *db_ptr;

	/***************************************************************************/
	/* Location index														   */
	/***************************************************************************/
	uint32_t index;

	/***************************************************************************/
	/* Node in Location CB Tree												   */
	/***************************************************************************/
	HM_AVL3_NODE index_node;

	/***************************************************************************/
	/* Location Group Index													   */
	/***************************************************************************/
	uint32_t group;

	/***************************************************************************/
	/* Active/Passive													       */
	/***************************************************************************/
	uint32_t role;

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
	uint32_t fsm_state;

	/***************************************************************************/
	/* Keepalive Period														   */
	/***************************************************************************/
	uint32_t keepalive_period;

	/***************************************************************************/
	/* Timer to send keepalive to node (For local only)					   	   */
	/* Before going into Active state, it will be used for INIT timeout.	   */
	/***************************************************************************/
	HM_TIMER_CB *timer_cb;

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
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Pointer to its CB in DB												   */
	/***************************************************************************/
	void *db_ptr;

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
	/* Tree Node in Node Process Tree										   */
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
	/* Running or not														   */
	/***************************************************************************/
	int32_t running;

} HM_PROCESS_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_INTERFACE_CB												 */
/*                                                                           */
/* Name:      hm_interface_cb											 	 */
/*                                                                           */
/* Textname:  Interface Control Block                                        */
/*                                                                           */
/* Description: 						            						 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_interface_cb
{
	/***************************************************************************/
	/* ID in DB																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Pointer to its CB in DB												   */
	/***************************************************************************/
	void *db_ptr;

	/***************************************************************************/
	/* Interface Type														   */
	/***************************************************************************/
	uint32_t if_type;

	/***************************************************************************/
	/* List element in parent Process CB									   */
	/***************************************************************************/
	HM_LQE list_node;

	/***************************************************************************/
	/* Parent Process CB													   */
	/***************************************************************************/
	HM_PROCESS_CB *parent_cb;

} HM_INTERFACE_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_GLOBAL_LOCATION_CB											 */
/*                                                                           */
/* Name:      hm_global_location_cb			 								 */
/*                                                                           */
/* Textname:  Global Location Control Block	                                 */
/*                                                                           */
/* Description: Represents a row in the Global Location CB Table             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_global_location_cb
{
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Node in Aggregate Locations table									   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Location ID															   */
	/***************************************************************************/
	uint32_t index;

	/***************************************************************************/
	/* Pointer to CB in system												   */
	/***************************************************************************/
	HM_LOCATION_CB *loc_cb;

	/***************************************************************************/
	/* Status	RUNNING/DOWN												   */
	/***************************************************************************/
	uint32_t status;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE subscriptions;

	/***************************************************************************/
	/* Pointer to subscription (for fast access)							   */
	/***************************************************************************/
	struct hm_subscription_cb *sub_cb;

} HM_GLOBAL_LOCATION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_GLOBAL_NODE_CB												 */
/*                                                                           */
/* Name:      hm_global_node_cb			 									 */
/*                                                                           */
/* Textname:  Global Node Control Block                                      */
/*                                                                           */
/* Description: Represents a Node entry in Global DB						 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_global_node_cb
{
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Node in Aggregate Nodes table									   	   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Location index														   */
	/***************************************************************************/
	uint32_t index;

	/***************************************************************************/
	/* Group Index															   */
	/***************************************************************************/
	uint32_t group_index;

	/***************************************************************************/
	/* Pointer to Node CB													   */
	/***************************************************************************/
	HM_NODE_CB *node_cb;

	/***************************************************************************/
	/* Location Role														   */
	/***************************************************************************/
	uint32_t role;

	/***************************************************************************/
	/* Location Status														   */
	/***************************************************************************/
	uint32_t status;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE subscriptions;

	/***************************************************************************/
	/* Pointer to subscription (for fast access)							   */
	/***************************************************************************/
	struct hm_subscription_cb *sub_cb;

} HM_GLOBAL_NODE_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_GLOBAL_PROCESS_CB											 */
/*                                                                           */
/* Name:      hm_global_process_cb			 								 */
/*                                                                           */
/* Textname:  Global Process Control Block	                                 */
/*                                                                           */
/* Description: Represents an entry in the global process tables.	         */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_global_process_cb
{
	/***************************************************************************/
	/* DB Index																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Node in Aggregate Process Tree										   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Process type															   */
	/***************************************************************************/
	uint32_t type;

	/***************************************************************************/
	/* Process Identifier (PID) of the process								   */
	/***************************************************************************/
	uint32_t pid;

	/***************************************************************************/
	/* Node Index of Parent Node											   */
	/***************************************************************************/
	uint32_t node_index;

	/***************************************************************************/
	/* Pointer to Process CB												   */
	/***************************************************************************/
	HM_PROCESS_CB *proc_cb;

	/***************************************************************************/
	/* Process status														   */
	/***************************************************************************/
	uint32_t status;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE subscriptions;

	/***************************************************************************/
	/* Pointer to subscription (for fast access)							   */
	/***************************************************************************/
	struct hm_subscription_cb *sub_cb;

} HM_GLOBAL_PROCESS_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_GLOBAL_INTERFACE_CB										 */
/*                                                                           */
/* Name:      hm_global_interface_cb			 							 */
/*                                                                           */
/* Textname:  Global Interface Control Block                                 */
/*																			 */
/* Description: Represents an entry in global interface tree.	             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_global_interface_cb
{
	/***************************************************************************/
	/* ID in DB																   */
	/***************************************************************************/
	uint32_t id;

	/***************************************************************************/
	/* Type of Structure. For dynamic Typecasting information				   */
	/***************************************************************************/
	int32_t table_type;

	/***************************************************************************/
	/* Entry in DB Interfaces tree											   */
	/***************************************************************************/
	HM_AVL3_NODE node;

	/***************************************************************************/
	/* Interface Type														   */
	/***************************************************************************/
	uint32_t if_type;

	/***************************************************************************/
	/* PID of parent														   */
	/***************************************************************************/
	uint32_t pid;

	/***************************************************************************/
	/* Pointer to Interface CB												   */
	/***************************************************************************/
	HM_INTERFACE_CB *if_cb;

	/***************************************************************************/
	/* List of Nodes which this node would be notified about.				   */
	/***************************************************************************/
	HM_LQE subscriptions;

	/***************************************************************************/
	/* Pointer to subscription (for fast access)							   */
	/***************************************************************************/
	struct hm_subscription_cb *sub_cb;

} HM_GLOBAL_INTERFACE_CB ;
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
	/* Value of subscription												   */
	/***************************************************************************/
	uint32_t value;

	/***************************************************************************/
	/* ID of Row on which subscription is being made						   */
	/***************************************************************************/
	uint32_t row_id;

	/***************************************************************************/
	/* Pointer to the row for fast access									   */
	/***************************************************************************/
	HM_SUBSCRIBER row_cb;

	/***************************************************************************/
	/* Number of subscribers												   */
	/***************************************************************************/
	uint32_t num_subscribers;

	/***************************************************************************/
	/* List of subscribers													   */
	/* NOTE: WHY NOT USE A PURE RELATIONAL DB MODEL and accessing by walking   */
	/* the table when subscribers need to be notified?						   */
	/* Because notifications need to be sent fast, and traversing a linked list*/
	/* would be faster than a tree traversal.								   */
	/***************************************************************************/
	HM_LQE subscribers_list;

	/***************************************************************************/
	/* Subscription is active or not?										   */
	/***************************************************************************/
	int32_t live;

} HM_SUBSCRIPTION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_SUBSCRIBER_WILDCARD										 */
/*                                                                           */
/* Name:      hm_subscriber_wildcard			 							 */
/*                                                                           */
/* Textname:  Wild Card Subscriber	                                         */
/*																			 */
/* Description: Wildcard Subscribers. Those which are subscribed to every    */
/* thing of a particular type.												 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_subscriber_wildcard
{
	/***************************************************************************/
	/* Link in Subscription list											   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Type of subscription													   */
	/***************************************************************************/
	uint32_t subs_type;

	/***************************************************************************/
	/* Subscription value											 		   */
	/***************************************************************************/
	uint32_t value;

	/***************************************************************************/
	/* Pointer to the parent subscriber CB.									   */
	/***************************************************************************/
	HM_SUBSCRIBER subscriber;

} HM_SUBSCRIBER_WILDCARD ;
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
	HM_SUBSCRIBER node_cb;

	/***************************************************************************/
	/* Number of references to this notifications							   */
	/* When this number drops to zero, we can free this notification.		   */
	/***************************************************************************/
	uint32_t ref_count;

	/***************************************************************************/
	/* Notification State: User specific.									   */
	/***************************************************************************/
	void *custom_data;

} HM_NOTIFICATION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_CONFIG_ADDRESS_CB		*/
/*                                                                           */
/* Name:      hm_config_address_cb			 */
/*                                                                           */
/* Textname:  Configuration Address CB	                                     */
/*                                                                           */
/* Description: 						            */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_config_address_cb
{
	/***************************************************************************/
	/* Address read from config												   */
	/***************************************************************************/
	HM_INET_ADDRESS address;

	/***************************************************************************/
	/* Scope of address	: Local or Remote									   */
	/***************************************************************************/
	uint32_t scope;

	/***************************************************************************/
	/* Port 																   */
	/***************************************************************************/
	uint32_t port;

	/***************************************************************************/
	/* Scope of communication: node or cluster								   */
	/***************************************************************************/
	uint32_t comm_scope;

	/***************************************************************************/
	/* Node in list															   */
	/***************************************************************************/
	HM_LQE node;
} HM_CONFIG_ADDRESS_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_CONFIG_SUBSCRIPTION_CB										 */
/*                                                                           */
/* Name:      hm_config_subscription_cb			 							 */
/*                                                                           */
/* Textname:  Configuration Block for Subscription CB                        */
/*                                                                           */
/* Description: 						            						 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_config_subscription_cb
{
	/***************************************************************************/
	/* Link in Subscription list											   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Type of subscription													   */
	/***************************************************************************/
	uint32_t subs_type;

	/***************************************************************************/
	/* Value in configuration												   */
	/***************************************************************************/
	uint32_t value;
} HM_CONFIG_SUBSCRIPTION_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_CONFIG_NODE_CB												 */
/*                                                                           */
/* Name:      hm_config_node_cb			 									 */
/*                                                                           */
/* Textname:  Configuration of a Node                                        */
/*                                                                           */
/* Description: 						            */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_config_node_cb
{
	/***************************************************************************/
	/* Node in List															   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Node CB																   */
	/***************************************************************************/
	HM_NODE_CB	*node_cb;

	HM_LQE subscriptions;
} HM_CONFIG_NODE_CB ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_HEARTBEAT_CONFIG		*/
/*                                                                           */
/* Name:      hm_heartbeat_config			 */
/*                                                                           */
/* Textname:  Heartbeat configuration structure                              */
/*                                                                           */
/* Description: 						            */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_heartbeat_config {
	uint32_t scope;
	uint32_t resolution;
	uint32_t timer_val;
	uint32_t threshold;
} HM_HEARTBEAT_CONFIG ;
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
		/***************************************************************************/
		/* Hardware Index of the HM												   */
		/***************************************************************************/
		uint32_t index;

		/***************************************************************************/
		/* Heartbeat configuration CBs											   */
		/***************************************************************************/
		HM_HEARTBEAT_CONFIG node, cluster;

		/***************************************************************************/
		/* Information on local TCP/UDP/Multicast Transports					   */
		/***************************************************************************/
		HM_CONFIG_ADDRESS_CB *node_addr;
		HM_CONFIG_ADDRESS_CB *cluster_addr;
		HM_CONFIG_ADDRESS_CB *mcast;

		uint32_t mcast_group;
		/***************************************************************************/
		/* List of address CBs 													   */
		/***************************************************************************/
		HM_LQE addresses;
	} instance_info;

	/***************************************************************************/
	/* List of Node CBs														   */
	/***************************************************************************/
	HM_LQE node_list;
} HM_CONFIG_CB ;
/**STRUCT-********************************************************************/


typedef struct attribute_map {
	/***************************************************************************/
	/* Character representation of attribute								   */
	/***************************************************************************/
	char attribute[25];
	/***************************************************************************/
	/* Type of attribute													   */
	/***************************************************************************/
	uint32_t type;
} HM_ATTRIBUTE_MAP;

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
	uint32_t next_loc_tree_id;

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
	/* Will contain only actively running instances.						   */
	/***************************************************************************/
	HM_AVL3_TREE pid_tree;
	uint32_t next_pid_tree_id;

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
	/* Wildcard subscribers list											   */
	/* These are required because they'll be subscribing to everything that    */
	/* does not even exist without regard to its name.						   */
	/***************************************************************************/
	HM_LQE table_root_subscribers;

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

	/***************************************************************************/
	/* Bitmask to check TCP Connection features							       */
	/***************************************************************************/
	uint32_t transport_bitmask;

	/***************************************************************************/
	/* List of all socket connections									       */
	/***************************************************************************/
	HM_LQE conn_list;

	/***************************************************************************/
	/* Address structure for Multicast sending								   */
	/***************************************************************************/
	HM_TRANSPORT_CB *mcast_addr;

} HM_GLOBAL_DATA ;
/**STRUCT-********************************************************************/

#endif /* SRC_HMSTRC_H_ */
