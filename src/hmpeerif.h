/*
 * hmpeerif.h
 *
 *  Created on: 29-Apr-2015
 *      Author: anshul
 */

#ifndef SRC_HMPEERIF_H_
#define SRC_HMPEERIF_H_

/*******************************NOTE****************************************/
/* Since we cannot be sure if the Cluster Communication will be TCP or UDP */
/* the PDU size of each message is non-variable.						   */
/* 																		   */
/***************************************************************************/
#define HM_PEER_NUM_TLVS_PER_UPDATE				((uint32_t)5)

/***************************************************************************/
/* Message Types to be used in msg_type field of HM_MSG_HEADER			   */
/***************************************************************************/
#define HM_PEER_MSG_TYPE_INIT					((uint32_t) 1) /* INIT Message*/
#define HM_PEER_MSG_TYPE_KEEPALIVE				((uint32_t) 2) /* Keepalive Tick */
#define HM_PEER_MSG_TYPE_PROCESS_UPDATE			((uint32_t) 3) /* Process Create/Destroy Update to HM */
#define HM_PEER_MSG_TYPE_NODE_UPDATE			((uint32_t) 4) /* Node Create/Destroy update to HM */
#define HM_PEER_MSG_TYPE_HA_UPDATE				((uint32_t) 5) /* Updates on HA roles */
#define HM_PEER_MSG_TYPE_REPLAY					((uint32_t) 6) /* Updates on HA roles */
/***************************************************************************/

/***************************************************************************/
/* Group: TLV Update Type												   */
/***************************************************************************/
#define HM_PEER_REPLAY_UPDATE_TYPE_NODE			((uint32_t) 1)
#define HM_PEER_REPLAY_UPDATE_TYPE_PROC			((uint32_t) 2)

/***************************************************************************/
/* Group: Possible status values of node and process types				   */
/***************************************************************************/
#define HM_PEER_ENTITY_STATUS_INACTIVE			((uint32_t) 0)
#define HM_PEER_ENTITY_STATUS_ACTIVE			((uint32_t) 1)

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_HEADER											 */
/*                                                                           */
/* Name:      hm_peer_msg_hdr			 									 */
/*                                                                           */
/* Textname:  Hardware Manager Peer Message Header                           */
/*                                                                           */
/* Description: The commone Message Header that must be placed on top of each*/
/* Peer Message.															 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_hdr
{
	/***************************************************************************/
	/* Message Type															   */
	/***************************************************************************/
	uint8_t msg_type[4];

	/***************************************************************************/
	/* Hardware Manager Index												   */
	/***************************************************************************/
	uint8_t hw_id[4];

	/***************************************************************************/
	/* Timestamp of sending													   */
	/* A time_t variable is a long int, so 8 bytes should suffice for most 	   */
	/* purposes.															   */
	/***************************************************************************/
	uint8_t timestamp[8];

} HM_PEER_MSG_HEADER ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_INIT												 */
/*                                                                           */
/* Name:      hm_peer_msg_init			 									 */
/*                                                                           */
/* Textname:  HM Peer INIT Message                                           */
/*                                                                           */
/* Description: INIT Message Telling about HM Parameters.		             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_init
{
	/***************************************************************************/
	/* HM Header															   */
	/***************************************************************************/
	HM_PEER_MSG_HEADER hdr;

} HM_PEER_MSG_INIT ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_KEEPALIVE											 */
/*                                                                           */
/* Name:      hm_peer_msg_keepalive			 								 */
/*                                                                           */
/* Textname:  HM Peer Keepalive Update Message                               */
/*                                                                           */
/* Description: Keepalive Tick sending a summary of own status and timestamps*/
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_keepalive
{
	/***************************************************************************/
	/* HM Header															   */
	/***************************************************************************/
	HM_PEER_MSG_HEADER hdr;

	/***************************************************************************/
	/* Port on which listening												   */
	/***************************************************************************/
	uint8_t listen_port[4];

	/***************************************************************************/
	/* Number of Active Nodes												   */
	/***************************************************************************/
	uint8_t num_nodes[4];

	/***************************************************************************/
	/* Number of active processes											   */
	/***************************************************************************/
	uint8_t num_proc[4];

} HM_PEER_MSG_KEEPALIVE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_REPLAY_TLV											 */
/*                                                                           */
/* Name:      hm_peer_replay_tlv			 								 */
/*                                                                           */
/* Textname:  HM Peer Replay TLV Chunk                                       */
/*                                                                           */
/* Description: A single chunk of information relayed.			             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_replay_tlv
{
	/***************************************************************************/
	/* Notify Type: Process/Node											   */
	/***************************************************************************/
	uint8_t update_type[4];

	/***************************************************************************/
	/* Node ID: Since a Process can be created on many nodes.				   */
	/***************************************************************************/
	uint8_t node_id[4];

	/***************************************************************************/
	/* ID of Process													   	   */
	/***************************************************************************/
	uint8_t pid[4];

	/***************************************************************************/
	/* Process Type/ Node Group Type										   */
	/***************************************************************************/
	uint8_t group[4];

	/***************************************************************************/
	/* Role	Primary/Back: Only relevant for Nodes.							   */
	/***************************************************************************/
	uint8_t role[4];
} HM_PEER_REPLAY_TLV ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_REPLAY											 */
/*                                                                           */
/* Name:      hm_peer_msg_replay			 							     */
/*                                                                           */
/* Textname:  HM Peer Replay Message	                                     */
/*                                                                           */
/* Description: Replay Message							            		 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_replay
{
	/***************************************************************************/
	/* HM Header															   */
	/***************************************************************************/
	HM_PEER_MSG_HEADER hdr;

	/***************************************************************************/
	/* Last in sequence														   */
	/* Signals that it is the last message and contains no other data.		   */
	/***************************************************************************/
	uint8_t last[4];

	/***************************************************************************/
	/* Number of TLVs filled in												   */
	/***************************************************************************/
	uint8_t num_tlvs[4];


	/***************************************************************************/
	/* Array of 5 TLVs														   */
	/***************************************************************************/
	HM_PEER_REPLAY_TLV tlv[HM_PEER_NUM_TLVS_PER_UPDATE];

} HM_PEER_MSG_REPLAY ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_NODE_UPDATE										 */
/*                                                                           */
/* Name:      hm_peer_msg_node_update			 							 */
/*                                                                           */
/* Textname:  HM Peer Node Update Message                                    */
/*                                                                           */
/* Description: Node Update Message								             */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_node_update
{
	/***************************************************************************/
	/* HM Header															   */
	/***************************************************************************/
	HM_PEER_MSG_HEADER hdr;

	/***************************************************************************/
	/* Node Status: UP(1)/DOWN(0)											   */
	/***************************************************************************/
	uint8_t status[4];

	/***************************************************************************/
	/* Node ID																   */
	/***************************************************************************/
	uint8_t node_id[4];

	/***************************************************************************/
	/* Node Group															   */
	/***************************************************************************/
	uint8_t node_group[4];

	/***************************************************************************/
	/* Node Role															   */
	/***************************************************************************/
	uint8_t node_role[4];

} HM_PEER_MSG_NODE_UPDATE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_PROCESS_UPDATE								     */
/*                                                                           */
/* Name:      hm_peer_msg_proc_update			 							 */
/*                                                                           */
/* Textname:  HM Peer Process Update Message                                 */
/*                                                                           */
/* Description: Process Update Message.						            	 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_proc_update
{
	/***************************************************************************/
	/* HM Header															   */
	/***************************************************************************/
	HM_PEER_MSG_HEADER hdr;

	/***************************************************************************/
	/* Process Status: UP(1)/DOWN(0)										   */
	/***************************************************************************/
	uint8_t status[4];

	/***************************************************************************/
	/* Process ID															   */
	/***************************************************************************/
	uint8_t proc_id[4];

	/***************************************************************************/
	/* Process Type															   */
	/***************************************************************************/
	uint8_t proc_type[4];

	/***************************************************************************/
	/* Node ID																   */
	/***************************************************************************/
	uint8_t node_id[4];
} HM_PEER_MSG_PROCESS_UPDATE ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: HM_PEER_MSG_UNION												 */
/*                                                                           */
/* Name:      hm_peer_msg_union			 									 */
/*                                                                           */
/* Textname:  HM Peer Message Union                                          */
/*                                                                           */
/* Description: A Union of all Peer Interface Messages. This is mainly to    */
/* implicitly determine the maximum occupancy among them all which would be	 */
/* useful in receiving bytes into buffer.									 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_peer_msg_union
{
	HM_PEER_MSG_HEADER peer_header;
	HM_PEER_MSG_INIT peer_init;
	HM_PEER_MSG_KEEPALIVE peer_tick;
	HM_PEER_MSG_REPLAY peer_replay;
	HM_PEER_MSG_NODE_UPDATE peer_node_update;
	HM_PEER_MSG_PROCESS_UPDATE peer_proc_update;
} HM_PEER_MSG_UNION ;
/**STRUCT-********************************************************************/

#endif /* SRC_HMPEERIF_H_ */
