/**
 *  @file hmnodeif.h
 *  @brief Hardware Manager Local Nodes Interface Declarations
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */
#ifndef SRC_HMNODEIF_H_
#define SRC_HMNODEIF_H_

/***************************************************************************/
/* Message Types to be used in msg_type field of HM_MSG_HEADER             */
/***************************************************************************/
#define HM_MSG_TYPE_INIT                ((uint32_t) 1) /* INIT Message*/
#define HM_MSG_TYPE_KEEPALIVE           ((uint32_t) 2) /* Keepalive Tick */
#define HM_MSG_TYPE_PROCESS_CREATE      ((uint32_t) 3) /* Process Create Update to HM */
#define HM_MSG_TYPE_PROCESS_DESTROY     ((uint32_t) 4) /* Process Destroy update to HM */
#define HM_MSG_TYPE_REGISTER            ((uint32_t) 5) /* Register for Notifications from HM*/
#define HM_MSG_TYPE_UNREGISTER          ((uint32_t) 6) /* UnRegister for Notifications from HM*/
#define HM_MSG_TYPE_HA_UPDATE           ((uint32_t) 7) /* Updates on HA roles */
#define HM_MSG_TYPE_HA_NOTIFY           ((uint32_t) 8) /* Notification Message */
/***************************************************************************/

/***************************************************************************/
/* Registration Types                                                      */
/***************************************************************************/
#define HM_REG_SUBS_TYPE_GROUP            ((uint32_t) 12)
#define HM_REG_SUBS_TYPE_PROC             ((uint32_t) 13)
#define HM_REG_SUBS_TYPE_IF               ((uint32_t) 14)
#define HM_REG_SUBS_TYPE_LOCATION         ((uint32_t) 15)
#define HM_REG_SUBS_TYPE_NODE             ((uint32_t) 16)

/***************************************************************************/
/* Notification Types                                                      */
/***************************************************************************/
#define HM_NOTIFY_TYPE_PROC_AVAILABLE       ((uint32_t) 1) /* Process available */
#define HM_NOTIFY_TYPE_PROC_GONE            ((uint32_t) 2) /* Process has gone down */
#define HM_NOTIFY_TYPE_NODE_UP              ((uint32_t) 3) /* Node has come up */
#define HM_NOTIFY_TYPE_NODE_DOWN            ((uint32_t) 4) /* Node has gone down */
#define HM_NOTIFY_TYPE_IF_PARTNER_AVAILABLE ((uint32_t) 5) /* Interface Partner is available */
#define HM_NOTIFY_TYPE_IF_PARTNER_GONE      ((uint32_t) 6) /* Interface Partner gone */
/***************************************************************************/

/***************************************************************************/
/* HA Roles used in HA_UPDATE_MSG                       */
/***************************************************************************/
#define HM_HA_NODE_ROLE_NONE                ((uint32_t) 0) /* Node must become active */
#define HM_HA_NODE_ROLE_ACTIVE              ((uint32_t) 1) /* Node must become active */
#define HM_HA_NODE_ROLE_PASSIVE             ((uint32_t) 2) /* Node must become passive */
/***************************************************************************/

/***************************************************************************/
/* Address Types                               */
/***************************************************************************/
#define HM_NOTIFY_ADDR_TYPE_TCP_v4          ((uint32_t) AF_INET)
#define HM_NOTIFY_ADDR_TYPE_TCP_v6          ((uint32_t) AF_INET6)


/***************************************************************************/
/* Node status codes.                                                      */
/***************************************************************************/
#define HM_NODE_STATUS_ACTIVE                ((uint32_t) 1)

/**
 * @brief HM Common Message Header.
 *
 * Common Message Header Segment that must be on top of each mesage from the cluster
 */
typedef struct hm_msg_header
{
  /***************************************************************************/
  /* Message Type of the payload. (UINT)                                     */
  /***************************************************************************/
  uint32_t msg_type;

  /***************************************************************************/
  /* Message Length (UINT)                                                   */
  /***************************************************************************/
  uint32_t msg_len;

  /***************************************************************************/
  /* Message ID.                                                             */
  /***************************************************************************/
  uint32_t msg_id;

  /***************************************************************************/
  /* Is request? True/False (UINT)                                           */
  /***************************************************************************/
  uint32_t request;

  /***************************************************************************/
  /* Response Code: OK/NOT OK                                                */
  /***************************************************************************/
  uint32_t response_ok;

} HM_MSG_HEADER ;
/**STRUCT-********************************************************************/


/**
 * @brief Hardware Manager Node INIT Message.
 *
 * INIT Message to be sent by Local Nodes to their HMs for requesting monitoring support.
 */
typedef struct hm_node_init_msg
{
  /***************************************************************************/
  /* Message Header                                                          */
  /***************************************************************************/
  HM_MSG_HEADER hdr;

  /***************************************************************************/
  /* Location Information: Node Number.                                      */
  /***************************************************************************/
  uint32_t index;

  /***************************************************************************/
  /* Service Group Number                                                    */
  /***************************************************************************/
  uint32_t service_group_index;

  /***************************************************************************/
  /* Hardware Location Number: To be filled by HM in response                */
  /***************************************************************************/
  uint32_t hardware_num;

  /***************************************************************************/
  /* Keepalive Period Requested.  (ms)                                       */
  /* Cab be overridden by HM in response. Node must set value from response. */
  /***************************************************************************/
  uint32_t keepalive_period;

  /***************************************************************************/
  /* Location Status: Active/Passive: To be filled by HM in response.        */
  /***************************************************************************/
  uint32_t location_status;

} HM_NODE_INIT_MSG ;
/**STRUCT-********************************************************************/


/**
 * @brief Hardware Manager Keepalive Message.
 *
 * Keepalive messages exchanged between nodes in the cluster.
 */
typedef struct hm_keepalive_msg
{
  HM_MSG_HEADER hdr;
} HM_KEEPALIVE_MSG ;
/**STRUCT-********************************************************************/

/**
 * @brief Process Update Message
 *
 * Update Message from HM Stub to HM regarding change in status of a process.
 * Process may be created/destroyed.
 */
typedef struct hm_process_update_msg
{
  /***************************************************************************/
  /* Message header                                                          */
  /***************************************************************************/
  HM_MSG_HEADER hdr;

  /***************************************************************************/
  /* Process Type                                                            */
  /***************************************************************************/
  uint32_t proc_type;

  /***************************************************************************/
  /* PID of process                                                          */
  /***************************************************************************/
  uint32_t pid;

  /***************************************************************************/
  /* Name of Process                                                         */
  /***************************************************************************/
  char name[25];

  /***************************************************************************/
  /* Number of slave interfaces supported                                    */
  /* Should be 0 in case of destroy message.                                 */
  /***************************************************************************/
  uint32_t num_if;

  /***************************************************************************/
  /* Offset from beginning of this structure where interfaces array starts   */
  /* Interface array is currently expected to be of uint32_t values.         */
  /* Should be 0 in case of destroy message.                                 */
  /***************************************************************************/
  uint32_t if_offset;

} HM_PROCESS_UPDATE_MSG ;
/**STRUCT-********************************************************************/


/**
 * @brief Register TLV Block
 *
 * A registration TLV Block defining the type of registration and subscribers
 * information, if applicable.
 */
typedef struct hm_register_tlv_cb
{
  /***************************************************************************/
  /* Register for what value                                                 */
  /* If registering for a node, node number                                  */
  /* if for group, then group index                                          */
  /* if for interface, then interface ID                                     */
  /***************************************************************************/
  uint32_t id;

} HM_REGISTER_TLV_CB ;
/**STRUCT-********************************************************************/


/**
 * @brief Register Message
 *
 * Generic register message to subscribe to notifications to a group, or PCT_TYPE,
 * or a location index in particular
 */
typedef struct hm_register_msg
{
  /***************************************************************************/
  /* Message Header                                                          */
  /***************************************************************************/
  HM_MSG_HEADER hdr;

  /***************************************************************************/
  /* Subscriber PID                                                          */
  /* Required for logistics purposes even if HM Stub keeps track itself.     */
  /*                                                                         */
  /* If the overall node is subscribing, set it to zero.                     */
  /***************************************************************************/
  uint32_t subscriber_pid;

  /***************************************************************************/
  /* Registration Type                                                       */
  /***************************************************************************/
  uint32_t type;

  /***************************************************************************/
  /* Number of registrations                           */
  /* It must be followed by an array of HM_REGISTER_TLV_CB for each        */
  /* registration.                               */
  /***************************************************************************/
  uint32_t num_register;
         
  uint32_t data[1];

} HM_REGISTER_MSG ;
/**STRUCT-********************************************************************/


/**
 * @brief Unregister Message
 *
 * Unregister from further notifications.
 */
typedef struct hm_unregister_msg
{
  /***************************************************************************/
  /* Header                                                                  */
  /***************************************************************************/
  HM_MSG_HEADER hdr;
        
  /***************************************************************************/
  /* Subscriber PID                                                          */
  /* Required for logistics purposes even if HM Stub keeps track itself.     */
  /*                                                                         */
  /* If the overall node is subscribing, set it to zero.                     */
  /***************************************************************************/
  uint32_t subscriber_pid;

  uint32_t type;

  /***************************************************************************/
  /* Number of un-registrations                                              */
  /* It must be followed by an array of HM_REGISTER_TLV_CB for each          */
  /* unregistration.                                                         */
  /***************************************************************************/
  uint32_t num_unregister;

  uint32_t data[1];

} HM_UNREGISTER_MSG ;
/**STRUCT-********************************************************************/


/**
 * @brief Address Information
 *
 * Address Information of a node.
 */
typedef struct hm_address_info
{
  /***************************************************************************/
  /* Address type                                                            */
  /***************************************************************************/
  uint32_t addr_type;

  /***************************************************************************/
  /* Address                                                                 */
  /* Is large enough to contain IPv6 addresses.                              */
  /***************************************************************************/
  char addr[128];

  /***************************************************************************/
  /* Port information                                                        */
  /***************************************************************************/
  uint32_t port;

  /***************************************************************************/
  /* Node Index                                                              */
  /***************************************************************************/
  uint32_t node_id;

  /***************************************************************************/
  /* Node Group                                                              */
  /***************************************************************************/
  uint32_t group;

  /***************************************************************************/
  /* Hardware index                                                          */
  /***************************************************************************/
  uint32_t hw_index;
} HM_ADDRESS_INFO ;
/**STRUCT-********************************************************************/


/**
 * @brief Notification Message
 *
 * Notification sent from HM to HM Stub
 */
typedef struct hm_notification_msg
{
  /***************************************************************************/
  /* Header                                                                  */
  /***************************************************************************/
  HM_MSG_HEADER hdr;

  /***************************************************************************/
  /* Notification Type                                                       */
  /***************************************************************************/
  uint32_t type;

  /***************************************************************************/
  /* Process Type of Process being notified                                  */
  /***************************************************************************/
  uint32_t proc_type;

  /***************************************************************************/
  /* PID of destination Process to be notified                               */
  /***************************************************************************/
  uint32_t subs_pid;

  /***************************************************************************/
  /* PID of reported process                                                 */
  /* For Node notifications, look into the addr_info fields.                 */
  /* They are compulsorily provided for constructive Process Updates too.    */
  /***************************************************************************/
  uint32_t id;

  /***************************************************************************/
  /* Interface Type of reported process                                      */
  /* used only in interface messages.                                        */
  /* Also used as a flag for presence of information in addr_info in case of */
  /* HA Updates.                                                             */
  /***************************************************************************/
  uint32_t if_id;

  /***************************************************************************/
  /* Transport information for the reported node.                            */
  /***************************************************************************/
  HM_ADDRESS_INFO addr_info;

} HM_NOTIFICATION_MSG ;
/**STRUCT-********************************************************************/

/**
 * @brief HA Status Update Message
 *
 * Master/Slave updates for overall nodes.
 *
 * @detail This message is a unidirectional message that is sent by a HM-Partner
 * to the HM informing that the User has set the role explicitly.
 *
 * For HA Role updates from HM to HM Interface users, Notification is used.
 */
typedef struct hm_ha_status_update_msg
{
  /***************************************************************************/
  /* Header                                                                  */
  /***************************************************************************/
  HM_MSG_HEADER hdr;

  /***************************************************************************/
  /* Node role                                                               */
  /***************************************************************************/
  uint32_t node_role;

  /***************************************************************************/
  /* Node information for complementary node provided? (Passive for ACTIVE or*/
  /* viceversa                                                               */
  /***************************************************************************/
  unsigned char node_info_provided; /* 0: False, 1: True  */

  /***************************************************************************/
  /* if provided, offset to HM_ADDRESS_INFO for target node.                 */
  /* Offset computed from the beginning of this structure.                   */
  /***************************************************************************/
  uint32_t offset;
} HM_HA_STATUS_UPDATE_MSG ;
/**STRUCT-********************************************************************/
#endif /* SRC_HMNODEIF_H_ */
