/**
 *  @file hmnodemgmt.c
 *  @brief Node Management Layer routines
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */
#include <hmincl.h>


/********************************************************************************/
/* FSM Description:                                                             */
/*                                                                              */
/* FSM States:                                                                  */
/*----------------------------------------------------------------------------- */
/*| S.No.  |  State Name               |    Remark                             |*/
/*----------------------------------------------------------------------------- */
/*|  0     |  HM_NODE_FSM_STATE_NULL   |  No connection exists.                |*/
/*|  1     |  HM_NODE_FSM_STATE_WAITING|  Node has been added.                 |*/
/*|        |                           |  Waiting for INIT request             |*/
/*|  2     |  HM_NODE_FSM_STATE_ACTIVE |  Received INIT request.               |*/
/*|        |                           |  This is (should be) the first        |*/
/*|        |                           |  message that is received.            |*/
/*|        |                           |  The Node is now an active member     |*/
/*|  3     |  HM_NODE_FSM_STATE_FAILING|   The Node is failing. Update info.   |*/
/*|  4     |  HM_NODE_FSM_STATE_FAILED |  Node is down. Cleanup is underway    |*/
/*------------------------------------------------------------------------------*/
/* State Table:                                                                 */
/*-------------------------------------------------------                       */
/*|State    |NULL[0]|WAIT[1]|ACTV[2]|FAIL[3]|CLSE[4]|                           */
/*|Input    |       |       |       |       |       |                           */
/*|          Next State via Path      |    |                                    */
/* ------------------------------------------------------                       */
/*|Create    |1    A |--- ERR|--- ERR|--- ERR|--- ERR|                          */
/*|INIT      |--- ERR|2    B |--- ERR|--- ERR|--- ERR|                          */
/*|Data      |--- ERR|--- ERR|---  C |--- ERR|--- ERR|                          */
/*|Terminate |--- ERR|--- ERR| 3   D |---  I |--- ---|                          */
/*|Close     |--- ERR|--- ERR|--- ERR|--- ERR| 1  ---|                          */
/*|TimerPop  |--- ERR| 3   E | 2   F |--- ERR|--- ERR|                          */
/*|TimeOut   |--- ERR|--- ERR| 3   G |--- ERR|--- ERR|                          */
/*|Failed    |--- ERR|--- ERR|--- ERR| 4   J |--- ERR|                          */
/*|Active    |--- ERR|--- ERR|---  I |--- ERR|--- ERR|                          */
/*-------------------------------------------------------                       */
/*                                                                              */
/*Path Descriptions:                                                            */
/*                                                                              */
/*A.  Node Created :                                                            */
/*  A local Node has been created in the system. Start the wait timers. Assert  */
/* that the node belongs to the local location only. Remote Nodes do not need an*/
/* FSM.                                                                         */
/*B.  Received INIT :                                                           */
/*  Assert that an appropriate Transport CB has been assigned to the node.      */
/*  Reset the Timer for Timeout.                                                */
/*  Send a response to the INIT message.                                        */
/*  Send a keepalive message.                                                   */
/*  Set timer for Keepalive messages.                                           */
/*  Update Global Tables for Nodes.                                             */
/*C.  Data Arrived:                                                             */
/*  Some data arrived on the connection. Read this data into a buffer and       */
/*  process it.                                                                 */
/*  The Node may receive subscription/unsubscribe requests for PCT_TYPE         */
/*  or IF_ID, or GROUPS, or NODEs. Further, it may receive HA Specific requests.*/
/*D.  Closing Connection:                                                       */
/*  The connection was either terminated by the other party or a request for    */
/*  disconnection was made by a local sub-module. Both of these cases result    */
/*  in disconnection and the socket descriptor is closed.                       */
/*  Free all resources associated in other layers and update global tables.     */
/*E.  Timeout:                                                                  */
/*  The node did not send an INIT request within the window period. Mark the    */
/*  node as down, and send updates, if necessary. A local node failing to start */
/* is a relevant event.                                                         */
/*F.  Timer Popped:                                                             */
/*  The node timer popped. Check if the missed Keepalive coun has not exceeded  */
/* the threshold value. If not, send a keepalive and increment the missed count.*/
/* The incoming Keepalive may reset this counter each time.                     */
/*G.   Keepalive Timeout:                                                       */
/*  Keepalive Timer Popped and threshold was exceeded.                          */
/*  The connection must be shut down and remote party declared as dead.         */
/*H.  Closed:                                                                   */
/* Connection was closed and resources freed.                                   */
/*I.  Failed:                                                                   */
/*  Node Down updates were propagated. Now release resources.                   */
/*  Node Active Signal                                                          */
/* Active signal (internally generated) to trigger global update. This signal   */
/* was grafted to compensate for the lack of foresight during FSM design.       */
/* While FSM Design, the path taken(the routine) must take into account the     */
/* state of machine AT THAT time, and not the one it is transitioning into.     */
/* J. Failing complete:                                                         */
/* Notifications have been sent. Release pending resources.                     */
/*                                                                              */
/********************************************************************************/

HM_TPRT_FSM_ENTRY hm_node_fsm_table[HM_NODE_FSM_NUM_SIGNALS][HM_NODE_FSM_NUM_STATES] =
{
    //HM_NDOE_FSM_CREATE SIGNAL Received
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_WAITING  ,  ACT_A  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  }
  },

  //HM_NDOE_FSM_INIT
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_ACTIVE  ,  ACT_B  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_ACTIVE  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  },

  //HM_NDOE_FSM_DATA
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_ACTIVE  ,  ACT_C  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  },

  //HM_NDOE_FSM_TERM
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* FIXME: Arises when Node Up was not received from remote, but then remote HM Goes down. */
/* HM_NODE_FSM_STATE_WAITING */   /*{  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },*/
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_FAILING  ,  ACT_D  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_FAILING  ,  ACT_D  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  ACT_I  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  ACT_NO  }
  },

  //HM_NDOE_FSM_CLOSE
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_ACTIVE  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_WAITING  ,  ACT_NO  }
  },

  //HM_NDOE_FSM_TIMER_POP
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_FAILING  ,  ACT_E  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_ACTIVE  ,  ACT_F  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  },

  //HM_NDOE_FSM_TIMEOUT
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_FAILING  ,  ACT_G  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  },
  //HM_NODE_FSM_FAILED
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILED  ,  ACT_J  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  },
  //HM_NODE_FSM_ACTIVE
  {                  //Next State        Path
/* HM_NODE_FSM_STATE_NULL  */    {  HM_NODE_FSM_STATE_NULL    ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_WAITING */   {  HM_NODE_FSM_STATE_WAITING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_ACTIVE  */    {  HM_NODE_FSM_STATE_ACTIVE  ,  ACT_I  },
/* HM_NODE_FSM_STATE_FAILING*/    {  HM_NODE_FSM_STATE_FAILING  ,  FSM_ERR  },
/* HM_NODE_FSM_STATE_FAILED */    {  HM_NODE_FSM_STATE_FAILED  ,  FSM_ERR  }
  }
};


/**
 *  @brief FSM Routines for a local node on this HM.
 *
 * FSM Description:
 *
 *FSM States:
 *-----------------------------------------------------------------------------
 *| S.No.  |  State Name        |    Remark               |
 *-----------------------------------------------------------------------------
 *|  0  |  HM_NODE_FSM_STATE_NULL  |  No connection exists.           |
 *|  1  |  HM_NODE_FSM_STATE_WAITING|  Node has been added.           |
 *|    |              |  Waiting for INIT request      |
 *|  2  |  HM_NODE_FSM_STATE_ACTIVE|  Received INIT request.         |
 *|    |              |  This is (should be) the first      |
 *|    |              |  message that is received.       |
 *|    |              |  The Node is now an active member  |
 *|  3  |  HM_NODE_FSM_STATE_FAILING|   The Node is failing. Update info.  |
 *|  4  |  HM_NODE_FSM_STATE_FAILED|  Node is down. Cleanup is underway   |
 *------------------------------------------------------------------------------
 *State Table:
 *-------------------------------------------------------
 *|State    |NULL[0]|WAIT[1]|ACTV[2]|FAIL[3]|CLSE[4]|
 *|Input    |    |    |    |    |    |
 *|          Next State via Path      |    |
 * ------------------------------------------------------
 *|Create    |1    A  |--- ERR|--- ERR|--- ERR|--- ERR|
 *|INIT      |--- ERR|2    B |--- ERR|--- ERR|--- ERR|
 *|Data      |--- ERR|--- ERR|---  C |--- ERR|--- ERR|
 *|Terminate  |--- ERR|--- ERR| 3   D |---  I |--- ---|
 *|Close     |--- ERR|--- ERR|--- ERR|--- ERR| 1  ---|
 *|TimerPop   |--- ERR| 3   E | 2   F |--- ERR|--- ERR|
 *|TimeOut     |--- ERR|--- ERR| 3   G |--- ERR|--- ERR|
 *|Failed    |--- ERR|--- ERR|--- ERR| 4   J |--- ERR|
 *|Active    |--- ERR|--- ERR|---  I |--- ERR|--- ERR|
 *-------------------------------------------------------
 *
 *Path Descriptions:
 *
 *A.  Node Created :
 *  A local Node has been created in the system. Start the wait timers. Assert
 * that the node belongs to the local location only. Remote Nodes do not need an
 * FSM.
 *B.  Received INIT :
 *  Assert that an appropriate Transport CB has been assigned to the node.
 *  Reset the Timer for Timeout.
 *  Send a response to the INIT message.
 *  Send a keepalive message.
 *  Set timer for Keepalive messages.
 *  Update Global Tables for Nodes.
 *C.  Data Arrived:
 *  Some data arrived on the connection. Read this data into a buffer and
 *  process it.
 *  The Node may receive subscription/unsubscribe requests for PCT_TYPE
 *   or IF_ID, or GROUPS, or NODEs. Further, it may receive HA Specific requests.
 *D.  Closing Connection:
 *  The connection was either terminated by the other party or a request for
 *  disconnection was made by a local sub-module. Both of these cases result
 *  in disconnection and the socket descriptor is closed.
 *  Free all resources associated in other layers and update global tables.
 *E.  Timeout:
 *  The node did not send an INIT request within the window period. Mark the
 *  node as down, and send updates, if necessary. A local node failing to start
 * is a relevant event.
 *F.  Timer Popped:
 *  The node timer popped. Check if the missed Keepalive coun has not exceeded
 * the threshold value. If not, send a keepalive and increment the missed count.
 * The incoming Keepalive may reset this counter each time.
 *G.   Keepalive Timeout:
 *  Keepalive Timer Popped and threshold was exceeded.
 *  The connection must be shut down and remote party declared as dead.
 *H.  Closed:
 * Connection was closed and resources freed.
 *I.  Failed:
 *  Node Down updates were propagated. Now release resources.
 *  Node Active Signal
 * Active signal (internally generated) to trigger global update. This signal
 * was grafted to compensate for the lack of foresight during FSM design.
 * While FSM Design, the path taken(the routine) must take into account the
 * state of machine AT THAT time, and not the one it is transitioning into.
 * J. Failing complete:
 * Notifications have been sent. Release pending resources.
 *
 *
 *  @param input_signal Incoming Signal into the FSM
 *  @param *node_cb Node Control Block (#HM_NODE_CB) for which this instance of FSM is running.
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_node_fsm(uint32_t input_signal, HM_NODE_CB * node_cb)
{
  uint32_t next_input = input_signal;
  uint32_t action;
  int32_t ret_val = HM_OK;;

  HM_PROCESS_CB *proc_cb = NULL;

  TRACE_ENTRY();

  TRACE_ASSERT(node_cb != NULL);

  /***************************************************************************/
  /* Run the FSM now.                               */
  /***************************************************************************/
  while(next_input != HM_NODE_FSM_NULL)
  {
    TRACE_DETAIL(("Signal : %d", next_input));
    input_signal = next_input;
    action = hm_node_fsm_table[next_input][node_cb->fsm_state].path;
    next_input = HM_NODE_FSM_NULL; /* Set unless specifically changed later. */
    /***************************************************************************/
    /* Loop until the inputs are processed properly.               */
    /***************************************************************************/
    switch(action)
    {
    case ACT_A:
      TRACE_DETAIL(("Act A"));
      TRACE_ASSERT(node_cb->parent_location_cb->index == LOCAL.local_location_cb.index);
      TRACE_INFO(("Local Node added. Initialize timer processing."));
      /***************************************************************************/
      /* Alter its timers to desired values.                     */
      /***************************************************************************/
      HM_TIMER_MODIFY(node_cb->timer_cb, LOCAL.node_keepalive_period);
      node_cb->keepalive_period = LOCAL.node_keepalive_period;
      /***************************************************************************/
      /* Arm the timer to receive a INIT request.                  */
      /***************************************************************************/
      HM_TIMER_START(node_cb->timer_cb);
      break;

    case ACT_B:
      TRACE_DETAIL(("Act B"));
      TRACE_ASSERT(node_cb->transport_cb != NULL);
      /***************************************************************************/
      /* Stop the timeout timer.                           */
      /***************************************************************************/
      HM_TIMER_STOP(node_cb->timer_cb);
      TRACE_DETAIL(("Sending INIT Response"));

      if(hm_node_send_init_rsp(node_cb) != HM_OK)
      {
        TRACE_ERROR(("Error sending INIT Response!"));
        break;
      }
      TRACE_DETAIL(("Sending Keepalive Message"));
      /***************************************************************************/
      /* Start the Keepalive timer again. The value MIGHT need modification.     */
      /***************************************************************************/
      TRACE_DETAIL(("Arm the keepalive timer."));
      node_cb->keepalive_missed++;
      HM_TIMER_START(node_cb->timer_cb);

      /***************************************************************************/
      /* Increment Parent Location's active node count                           */
      /***************************************************************************/
      node_cb->parent_location_cb->active_nodes++;
      TRACE_INFO(("Parent location %d's active nodes now %d",
          node_cb->parent_location_cb->index,
          node_cb->parent_location_cb->active_nodes));
      next_input = HM_NODE_FSM_ACTIVE;
      break;

    case ACT_C:
      TRACE_DETAIL(("Act C"));
      TRACE_DETAIL(("Receive Data for node from transport."));
      //TODO: Call into Transport Data FSM.

      //Parse data and do the rest of processing.
      break;

    case ACT_D:
      TRACE_DETAIL(("Act D"));
      TRACE_DETAIL(("Releasing resources for the node."));
      //TODO
      //All processes on this node are now down. Mark them down (causes notifications)
      for(proc_cb = (HM_PROCESS_CB *)HM_AVL3_FIRST(node_cb->process_tree,
                        node_process_tree_by_proc_type_and_pid);
          proc_cb != NULL;
          proc_cb = (HM_PROCESS_CB *)HM_AVL3_NEXT(proc_cb->node,
                      node_process_tree_by_proc_type_and_pid ))
      {
        proc_cb->running = FALSE;
        hm_process_update(proc_cb);
      }
      /***************************************************************************/
      /* Decrement the number of active nodes on Location, since we're going down*/
      /***************************************************************************/
      node_cb->parent_location_cb->active_nodes--;
      TRACE_INFO(("Parent location %d's active nodes now %d",
          node_cb->parent_location_cb->index,
          node_cb->parent_location_cb->active_nodes));
      next_input = HM_NODE_FSM_TERM;
      break;

    case ACT_E:
      TRACE_DETAIL(("Act E"));
      TRACE_DETAIL(("INIT Message was not received within window period."));
      /* Stop timer. No more Pop-ups! */
      HM_TIMER_STOP(node_cb->timer_cb);

      next_input = HM_NODE_FSM_TERM;
      break;

    case ACT_F:
      TRACE_DETAIL(("Act F"));
      TRACE_DETAIL(("Keepalive Timer Popped. Transmit next or Mark node as dead."));
      break;

    case ACT_G:
      TRACE_DETAIL(("Act G"));
      TRACE_DETAIL(("Node Timed Out. Cleanup resources."));
      break;

    case ACT_H:
      TRACE_DETAIL(("Act H"));
      TRACE_DETAIL(("Resources cleaned up. Free memories!"));
      break;

    case ACT_I:
      TRACE_DETAIL(("Act I"));
      TRACE_DETAIL(("Node status changed. Notify subscribers."));
      hm_global_node_update(node_cb, HM_UPDATE_RUN_STATUS);
      if(node_cb->fsm_state == HM_NODE_FSM_STATE_FAILING)
      {
        next_input = HM_NODE_FSM_FAILED;
      }
      break;

    case ACT_J:
      TRACE_DETAIL(("Act J"));
      TRACE_DETAIL(("Node has failed. Release resources."));
      if(node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
      {
        hm_tprt_close_connection(node_cb->transport_cb);
      }
      else
      {
        TRACE_DETAIL(("Do not remove shared resources for remote node."));
      }
      next_input = HM_NODE_FSM_CLOSE;
      break;

    case ACT_NO:
      /***************************************************************************/
      /* Do nothing. Just advance the state to next.                 */
      /***************************************************************************/
      TRACE_DETAIL(("Act NO"));
      break;

    default:
      TRACE_WARN(("Invalid FSM Branch Hit. Error!"));
      TRACE_WARN(("Current State: %d: Signal Received: %d", node_cb->fsm_state,input_signal));
      TRACE_ASSERT(FALSE);
      break;
    }//end switch

    /***************************************************************************/
    /* Update the state of the connection                     */
    /***************************************************************************/
    if(node_cb != NULL)
    {
      TRACE_DETAIL(("Current State is: %d", node_cb->fsm_state));
      if(node_cb->fsm_state != hm_node_fsm_table[input_signal][node_cb->fsm_state].next_state)
      {
        node_cb->fsm_state = hm_node_fsm_table[input_signal][node_cb->fsm_state].next_state;
        /***************************************************************************/
        /* Update global tables.                           */
        /* NOTE:                                   */
        /* The updation of global tables is disparate from node function. This is  */
        /* just to keep the codes separate and node code as independent from global*/
        /* tables code.                                 */
        /***************************************************************************/
        //hm_global_node_update(node_cb);
        TRACE_DETAIL(("New State is: %d", node_cb->fsm_state));
      }
    }
    else
    {
      TRACE_DETAIL(("Connection was closed and cleaned up."));
    }
  }//end while
  TRACE_EXIT();
  return (ret_val);
} /* hm_node_fsm */


/**
 *  @brief Adds a node to its parent Location and does the subscription triggers
 *
 *  @param *node_cb Node Control Block (#H_NODE_CB) to be added to the
 *  @param *location_cb Location CB (#HM_LOCATION_CB) to which Node CB must be added to.
 *  Location Control Block (#HM_LOCATION_CB)
 *  @return return_value
 */
int32_t hm_node_add(HM_NODE_CB *node_cb, HM_LOCATION_CB *location_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_NODE_CB *insert_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(node_cb != NULL);
  TRACE_ASSERT(location_cb !=NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  node_cb->parent_location_cb = location_cb;
  /***************************************************************************/
  /* The generic stuff                             */
  /***************************************************************************/
  /***************************************************************************/
  /* Add to Location CBs Tree                           */
  /***************************************************************************/
  TRACE_DETAIL(("Add Node with ID %d to Location %d", node_cb->index, location_cb->index));

  insert_cb = (HM_NODE_CB *)HM_AVL3_INSERT_OR_FIND(location_cb->node_tree,
                    node_cb->index_node,
                    nodes_tree_by_node_id);
  if(insert_cb != NULL)
  {
     TRACE_WARN(("Found an existing entry with same parameters. Overwriting fields"));
     /***************************************************************************/
     /* Validate that the two are one and the same thing. Maybe it had gone down*/
     /* and is coming back up again?                      */
     /***************************************************************************/
     TRACE_ASSERT(insert_cb->index==node_cb->index);
     /* FSM State field determines if the node is active or not */
     /* For Local Nodes, this field is governed by an FSM.      */
     /* For Remote field, it is directly modified        */
     /* In any case, a new ADD request must only arrive if it   */
     /* went down once.                      */
     if(insert_cb->fsm_state == HM_NODE_FSM_STATE_ACTIVE)
     {
       TRACE_ERROR(("Existing node already in active state."));
       ret_val = HM_ERR;
       goto EXIT_LABEL;
     }
     TRACE_DETAIL(("Initial transport CB (%p) written with (%p)",
            insert_cb->transport_cb, node_cb->transport_cb));
     insert_cb->transport_cb = node_cb->transport_cb;
     hm_free_node_cb(node_cb);
     node_cb = insert_cb;
  }
  else
  {
    TRACE_DETAIL(("New node %d added.", node_cb->index));
    insert_cb = node_cb;
    /***************************************************************************/
    /* Increment the number of nodes on this location.               */
    /***************************************************************************/
    location_cb->total_nodes++;
  }

   if(node_cb->parent_location_cb->index == LOCAL.local_location_cb.index)
   {
    /***************************************************************************/
    /* Allocate a transport CB to this node and associate the sock_cb with that*/
    /* transport CB.                               */
    /***************************************************************************/
    node_cb->transport_cb = hm_alloc_transport_cb(HM_TRANSPORT_TCP_IN);
    if(node_cb->transport_cb == NULL)
    {
      TRACE_ERROR(("Error creating a transport CB for the connection."));
      //Now this is a non-recoverable problem. TODO
      goto EXIT_LABEL;
    }
    node_cb->transport_cb->node_cb = node_cb;
    node_cb->transport_cb->location_cb =
                    node_cb->parent_location_cb;
    //TODO: Move it to FSM later.: Go into WAIT State
     if((ret_val = hm_node_fsm(HM_NODE_FSM_CREATE, node_cb)) != HM_OK)
     {
       TRACE_ERROR(("Error while transitioning the state machine."));
       //FIXME: Is this is terminal error?
       //Currently ignoring this error!
     }
   }
   else
   {
     TRACE_INFO(("Remote Node information added."));
     /***************************************************************************/
     /*Associate the transport of parent Location with this node also.      */
     /***************************************************************************/
     node_cb->transport_cb = node_cb->parent_location_cb->peer_listen_cb;
     TRACE_ASSERT(node_cb->transport_cb->sock_cb!=NULL);

     TRACE_DETAIL(("Node associated with socket %d",
         node_cb->transport_cb->sock_cb->sock_fd));
     /* Transport doesn't know this association */
     TRACE_ASSERT(node_cb->transport_cb->node_cb == NULL);
   }

   /***************************************************************************/
   /* Add node to global tables for subscriptions and stuff.            */
   /***************************************************************************/
   if(hm_global_node_add(node_cb) != HM_OK)
   {
     TRACE_ERROR(("Error updating node statistics in system"));
     ret_val = HM_ERR;
     if(hm_node_remove(node_cb)!= HM_OK)
     {
       TRACE_ERROR(("Error removing node from system."));
       ret_val = HM_ERR;
       goto EXIT_LABEL;
     }
   }

   /***************************************************************************/
   /* If node is not local, and its status is active, the subscription must go*/
   /* live                                  */
   /***************************************************************************/
   if(node_cb->parent_location_cb->index != LOCAL.local_location_cb.index)
   {
     if(node_cb->fsm_state == HM_NODE_FSM_STATE_ACTIVE)
     {
       node_cb->parent_location_cb->active_nodes++;
       if(hm_global_node_update(node_cb, HM_UPDATE_RUN_STATUS)!= HM_OK)
       {
         TRACE_ERROR(("Error propagating node update"));
         ret_val = HM_ERR;
         goto EXIT_LABEL;
       }
     }
   }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_node_add */


/**
 *  @brief Removes the node from the system
 *
 *  @param *node_cb Pointer to the Node CB (#HM_NODE_CB) that needs to be removed
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_node_remove(HM_NODE_CB *node_cb  )
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(node_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  if(node_cb->id > 0)
  {
    TRACE_DETAIL(("Remove node from global tables too."));
    hm_global_node_remove(node_cb);
  }
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_node_remove */


/**
 *  @brief Keepalive Callback for Keepalive Timer Pop
 *
 *  @param *cb Pointer to the Node CB (#HM_NODE_CB passed as @c void)
 *  @return #HM_OK if successful, #HM_ERR otherwise,
 */
int32_t hm_node_keepalive_callback(void *cb)
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

  TRACE_ASSERT(cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Align offset of node_cb from timer_cb                   */
  /***************************************************************************/
  node_cb = (HM_NODE_CB *)cb;

  TRACE_DETAIL(("Timer Pop for node %d on Location %d",
              node_cb->index, node_cb->parent_location_cb->index));
  /***************************************************************************/
  /* Call the FSM:                                */
  /* Either timeout or keepalive send sequence will happen.           */
  /***************************************************************************/
  hm_node_fsm(HM_NODE_FSM_TIMER_POP, node_cb);

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_node_keepalive_callback */
