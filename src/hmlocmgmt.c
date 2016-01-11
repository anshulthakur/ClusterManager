/**
 *  @file hmlocmgmt.c
 *  @brief Location Management Layer Routines
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None
 */
#include <hmincl.h>


/********************************************************************************/
/* FSM Description:                                   */
/*                                        */
/*FSM States:                                    */
/*-----------------------------------------------------------------------------  */
/*| S.No.  |  State Name        |    Remark               |*/
/*-----------------------------------------------------------------------------  */
/*|  0  |  HM_PEER_FSM_STATE_NULL  |  No connection exists.           |  */
/*|  1  |  HM_PEER_FSM_STATE_INIT  |  A connect request has been sent to  |   */
/*|    |              |  the Peer.                 |  */
/*|    |              |  Waiting for Connect to complete    |  */
/*|  2  |  HM_PEER_FSM_STATE_ACTIVE|  Received INIT request/response.    |  */
/*|    |              |  This is (should be) the first      |  */
/*|    |              |  message that is received.       |  */
/*|    |              |  The Node is now an active member  |  */
/*|  3  |  HM_PEER_FSM_STATE_FAILED|  Peer is down.              |  */
/*------------------------------------------------------------------------------*/
/*State Table:                                  */
/*-----------------------------------------------                */
/*|State    |NULL[0]|INIT[1]|ACTV[2]|FAIL[3]|                */
/*|Input    |    |    |    |    |                */
/*|          Next State via Path      |                */
/* ----------------------------------------------                */
/*|CONNECT    | 1    A  |--- ERR|--- ERR|--- ERR|                */
/*|INIT_RCVD  | 2   B | 2    B |--- ERR| 2   B |                */
/*|LOOP      |--- ERR|---  C |---  C |---  C |                */
/*|CLOSE    |--- ERR| 3   D | 3   D |--- ERR|                */
/*|CLOSED     |--- ERR|--- ERR|--- ERR| 0  ---|                */
/*|POP      |--- ERR|--- ERR|---  E |--- ERR|                */
/*-----------------------------------------------                */
/*                                        */
/*Path Descriptions:                              */
/*                                        */
/*A.  Peer Discovered on Multicast:                      */
/*  Connect request has been sent and we were waiting for it to complete.    */
/* No update from Global DB will be made right now.                  */
/* If connect fails, the peer does not exist.                  */
/*B.  Received INIT :                              */
/*  INIT Message was received on transport, or Connect was successful.      */
/* The Peer is now considered active and we may commence transactions with it.  */
/*C.  Loop:                                  */
/*  The state has changed and we need to react to that by sending out possible  */
/* updates. So, we are looping in to call the update method on updated states.  */
/* THIS MUST BE CALLED INTERNALLY ONLY.                      */
/*D.  Peer Disconnected:                            */
/*  The connection was either terminated by the other party or a request for  */
/*  disconnection was made by a local sub-module or keepalive threshold was   */
/*  breached.                                  */
/*  Send out updates if necessary.                        */
/*E. Timer Pop:                                    */
/* Send Keepalive if threshold has not been breached.              */
/*                                         */
/********************************************************************************/


HM_TPRT_FSM_ENTRY hm_peer_fsm_table[HM_PEER_FSM_NUM_SIGNALS][HM_PEER_FSM_NUM_STATES] =
{
    //HM_PEER_FSM_CONNECT SIGNAL Received
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_INIT    ,  ACT_A  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  }
  },

  //HM_PEER_FSM_INIT_RCVD
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_ACTIVE  ,  ACT_B  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_ACTIVE  ,  ACT_B  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_ACTIVE  ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_ACTIVE  ,  ACT_B  }
  },

  //HM_PEER_FSM_LOOP
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_INIT    ,  ACT_C  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_ACTIVE  ,  ACT_C  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_FAILED  ,  ACT_C  }
  },

  //HM_PEER_FSM_CLOSE
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_INIT    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_FAILED  ,  ACT_D  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_FAILED  ,  ACT_D  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  }
  },

  //HM_PEER_FSM_CLOSED
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_INIT    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_ACTIVE  ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_NULL    ,  ACT_NO  }
  },

  //HM_PEER_FSM_TIMER_POP
  {                  //Next State        Path
/* HM_PEER_FSM_STATE_NULL  */    {  HM_PEER_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_INIT */   {  HM_PEER_FSM_STATE_INIT    ,  FSM_ERR  },
/* HM_PEER_FSM_STATE_ACTIVE  */    {  HM_PEER_FSM_STATE_ACTIVE  ,  ACT_E  },
/* HM_PEER_FSM_STATE_FAILED*/    {  HM_PEER_FSM_STATE_FAILED  ,  FSM_ERR  }
  }
};

/**
 *  @brief FSM Routines for a local node on this HM.
 *
 *
 * FSM Description:
 *
 *FSM States:
 *-----------------------------------------------------------------------------
 *| S.No.  |  State Name        |    Remark               |
 *-----------------------------------------------------------------------------
 *|  0  |  HM_PEER_FSM_STATE_NULL  |  No connection exists.           |
 *|  1  |  HM_PEER_FSM_STATE_INIT  |  A connect request has been sent to  |
 *|    |              |  the Peer.                 |
 *|    |              |  Waiting for Connect to complete    |
 *|  2  |  HM_PEER_FSM_STATE_ACTIVE|  Received INIT request/response.    |
 *|    |              |  This is (should be) the first      |
 *|    |              |  message that is received.       |
 *|    |              |  The Node is now an active member  |
 *|  3  |  HM_PEER_FSM_STATE_FAILED|  Peer is down.              |
 *------------------------------------------------------------------------------
 *State Table:
 *-----------------------------------------------
 *|State    |NULL[0]|INIT[1]|ACTV[2]|FAIL[3]|
 *|Input    |    |    |    |    |
 *|          Next State via Path      |
 * ----------------------------------------------
 *|CONNECT    | 1    A  |--- ERR|--- ERR|--- ERR|
 *|INIT_RCVD  | 2   B | 2    B |--- ERR| 2   B |
 *|LOOP      |--- ERR|---  C |---  C |---  C |
 *|CLOSE    |--- ERR| 3   D | 3   D |--- ERR|
 *|CLOSED     |--- ERR|--- ERR|--- ERR| 0  ---|
 *|POP      |--- ERR|--- ERR|---  E |--- ERR|
 *-----------------------------------------------
 *
 *Path Descriptions:
 *
 *A.  Peer Discovered on Multicast:
 *  Connect request has been sent and we were waiting for it to complete.
 * No update from Global DB will be made right now.
 * If connect fails, the peer does not exist.
 *B.  Received INIT :
 *  INIT Message was received on transport, or Connect was successful.
 * The Peer is now considered active and we may commence transactions with it.
 *C.  Loop:
 *  The state has changed and we need to react to that by sending out possible
 * updates. So, we are looping in to call the update method on updated states.
 * THIS MUST BE CALLED INTERNALLY ONLY.
 *D.  Peer Disconnected:
 *  The connection was either terminated by the other party or a request for
 *  disconnection was made by a local sub-module or keepalive threshold was
 *  breached.
 *  Send out updates if necessary.
 *E. Timer Pop:
 * Send Keepalive if threshold has not been breached.
 *
 *  @param input_signal Signal into the FSM
 *  @param *loc_cb Location CB whose FSM is running.
 *
 *  @return #HM_OK if successful, #HM_ERR otherwise
 */
int32_t hm_peer_fsm(uint32_t input_signal, HM_LOCATION_CB * loc_cb)
{
  uint32_t next_input = input_signal;
  uint32_t action;
  int32_t ret_val = HM_OK;;

  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);

  /***************************************************************************/
  /* Run the FSM now.                               */
  /***************************************************************************/
  while(next_input != HM_NODE_FSM_NULL)
  {
    TRACE_DETAIL(("Signal : %d", next_input));
    input_signal = next_input;
    action = hm_peer_fsm_table[next_input][loc_cb->fsm_state].path;
    next_input = HM_NODE_FSM_NULL; /* Set unless specifically changed later. */
    /***************************************************************************/
    /* Loop until the inputs are processed properly.               */
    /***************************************************************************/
    switch(action)
    {
    case ACT_A:
      TRACE_DETAIL(("Act A"));
      TRACE_DETAIL(("A new node has been created on discovery."));
      break;

    case ACT_B:
      TRACE_DETAIL(("Act B"));
      TRACE_DETAIL(("Peer is now active."));
      /***************************************************************************/
      /* Arm its Keepalive timer to check for Keepalive timeouts on timely basis */
      /* NOTE that this is current implementation specific where Peers talk on   */
      /* TCP Socket.                                  */
      /* In Multicast UDP Case, there will be just a local timer at which the    */
      /* Location emits a pulse and also checks if any of the other location      */
      /* timers have been missed.                          */
      /* These timers may simply be a bitmask value, array of 'N' bitmasks,      */
      /* where N is the kickout count.                       */
      /* So, for a peer, if one Tick is missed, its bit mask is set.          */
      /* If at the second timer pop, it is still set, then next array mask is set*/
      /* If N masks are set, then the Peer is declared dead.             */
      /***************************************************************************/
      loc_cb->keepalive_missed = 0;
      HM_TIMER_START(loc_cb->timer_cb);

      /***************************************************************************/
      /* Update its global DB                             */
      /***************************************************************************/
      if(hm_global_location_add(loc_cb, HM_STATUS_PENDING) == HM_ERR)
      {
        TRACE_ERROR(("Error occurred while adding Hardware Location to HM."));
        ret_val = HM_ERR;
        TRACE_ASSERT(FALSE);
      }
      /***************************************************************************/
      /* Also, send complete Location, Node and Process Information of this loc  */
      /* to the peer.                                 */
      /* Queue on transport, don't actually send. We might not have sent the INIT*/
      /* response yet.                               */
      /*                                       */
      /* We're expecting the peer to do the same as soon as it receives INIT Rsp.*/
      /* Send END OF REPLAY at the end of it.                     */
      /***************************************************************************/
      loc_cb->replay_in_progress = TRUE;
      if(hm_cluster_replay_info(loc_cb->peer_listen_cb) != HM_OK)
      {
        TRACE_ERROR(("Error sending replay messages to peer."));
        TRACE_ASSERT(FALSE);
      }

      next_input = HM_PEER_FSM_LOOP;

      break;

    case ACT_C:
      TRACE_DETAIL(("Act C"));
      if(hm_location_update(loc_cb) == HM_ERR)
      {
        TRACE_ERROR(("Error occurred while updating Hardware Location."));
        ret_val = HM_ERR;
        TRACE_ASSERT(FALSE);
      }
      break;

    case ACT_D:
      TRACE_DETAIL(("Act D"));
      TRACE_WARN(("Peer disconnected."));
      /***************************************************************************/
      /* Stop timers                                   */
      /***************************************************************************/
      HM_TIMER_STOP(loc_cb->timer_cb);
      next_input = HM_PEER_FSM_LOOP;

      break;

    case ACT_E:
      TRACE_DETAIL(("Act E"));
      TRACE_DETAIL(("Keepalive Timer Popped!"));
      /***************************************************************************/
      /* For a Local Location CB, this is an indication to send KA Ticks.       */
      /***************************************************************************/
      if(loc_cb->index == LOCAL.local_location_cb.index)
      {
        TRACE_DETAIL(("Send Keepalive on Cluster"));
        hm_cluster_send_tick();
      }
      else if(++loc_cb->keepalive_missed > LOCAL.peer_kickout_value)
      {
        TRACE_WARN(("Peer has exceeded Kickout Threshold. Mark it as down now!"));
        next_input = HM_PEER_FSM_CLOSE;
      }
      /***************************************************************************/
      /* I'm sending Keepalive Ticks on the Multicast Socket only. This is due to*/
      /* the fact that TCP Connections would usually know if something went wrong*/
      /* but I'd need to send a separate message to each peer separately when the*/
      /* respective timers pop.                           */
      /* So, rather than doing that, since the Timeout value is going to stay    */
      /* consistent throughout the cluster, it is very much the case that one    */
      /* tick message must have arrived in between two pops.             */
      /***************************************************************************/
      break;

    case ACT_NO:
      /***************************************************************************/
      /* Do nothing. Just advance the state to next.                 */
      /***************************************************************************/
      TRACE_DETAIL(("Act NO"));
      break;

    default:
      TRACE_WARN(("Invalid FSM Branch Hit. Error!"));
      TRACE_WARN(("Current State: %d: Signal Received: %d", loc_cb->fsm_state, input_signal));
      TRACE_ASSERT(FALSE);
      break;
    }//end switch

    /***************************************************************************/
    /* Update the state of the connection                     */
    /***************************************************************************/
    if(loc_cb != NULL)
    {
      TRACE_DETAIL(("Current State is: %d", loc_cb->fsm_state));
      if(loc_cb->fsm_state != hm_peer_fsm_table[input_signal][loc_cb->fsm_state].next_state)
      {
        loc_cb->fsm_state = hm_peer_fsm_table[input_signal][loc_cb->fsm_state].next_state;
        /***************************************************************************/
        /* Update global tables.                           */
        /* NOTE:                                   */
        /* The updation of global tables is disparate from node function. This is  */
        /* just to keep the codes separate and node code as independent from global*/
        /* tables code.                                 */
        /***************************************************************************/
        //hm_global_node_update(node_cb);
        TRACE_DETAIL(("New State is: %d", loc_cb->fsm_state));
      }
    }
    else
    {
      TRACE_DETAIL(("Connection was closed and cleaned up."));
    }
  }//end while
  TRACE_EXIT();
  return (ret_val);
} /* hm_peer_fsm */


/**
 *  @brief Adds a location to trees
 *
 *  @param *loc_cb Location Control Block (#HM_LOCATION_CB) which needs to be added
 *    in local Locations tree
 *  @return #HM_OK if successful, #HM_ERR otherwise
 */
int32_t hm_location_add(HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/


  if(hm_global_location_add(loc_cb, HM_STATUS_RUNNING) == HM_ERR)
  {
    TRACE_ERROR(("Error occurred while adding Hardware Location to HM."));
    ret_val = HM_ERR;
  }
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_location_add */


/**
 *  @brief Update the Hardware Location CB
 *
 *  @param *loc_cb Location CB (#HM_LOCATION_CB) of the location which was updated.
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_location_update(HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_NODE_CB *node_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* If the location is down, all its nodes are also marked as down.       */
  /***************************************************************************/
  switch(loc_cb->fsm_state)
  {
  case HM_PEER_FSM_STATE_ACTIVE:
    TRACE_DETAIL(("Peer moved to active state."));
    break;

  case HM_PEER_FSM_STATE_FAILED:
    TRACE_DETAIL(("Peer is no longer active. Mark Nodes as Down"));
    for(node_cb = (HM_NODE_CB *)HM_AVL3_FIRST(loc_cb->node_tree, nodes_tree_by_node_id);
        node_cb != NULL;
        node_cb = (HM_NODE_CB *)HM_AVL3_NEXT(node_cb->index_node, nodes_tree_by_node_id))
    {
      /***************************************************************************/
      /* Mark node as down.                             */
      /***************************************************************************/
      TRACE_INFO(("Mark Node %d as down.", node_cb->index));
      hm_node_fsm(HM_NODE_FSM_TERM, node_cb);
    }
    break;

  default:
    TRACE_WARN(("Unknown state of Node %d", loc_cb->fsm_state));
    TRACE_ASSERT(FALSE);
  }

  if(hm_global_location_update(loc_cb, HM_UPDATE_RUN_STATUS) != HM_OK)
  {
    TRACE_ERROR(("Error occurred while updating Hardware Location."));
    ret_val = HM_ERR;
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();

  return ret_val;
}/* hm_location_update */


/**
 *  @brief Removes the Location from the System
 *
 *  @param *loc_cb Location CB (#HM_LOCATION_CB) that needs to be removed from the local system
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_remove_location(HM_LOCATION_CB *loc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_remove_location */


/**
 *  @brief Keepalive callback on Keepalive Timer Pop
 *
 *  @param *cb Control Block returned by the timer function
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_peer_keepalive_callback(void *cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_LOCATION_CB *loc_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Align offset of node_cb from timer_cb                   */
  /***************************************************************************/
  loc_cb = (HM_LOCATION_CB *)cb;

  TRACE_DETAIL(("Timer Pop for Location %d", loc_cb->index));
  /***************************************************************************/
  /* Call the FSM:                                */
  /* Either timeout or keepalive send sequence will happen.           */
  /***************************************************************************/
  hm_peer_fsm(HM_PEER_FSM_TIMER_POP, loc_cb);

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_peer_keepalive_callback */
