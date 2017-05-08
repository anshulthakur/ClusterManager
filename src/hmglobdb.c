/**
 *  @file hmglobdb.c
 *  @brief Hardware Manager Global Database Operational routines
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None
 */
#include <hmincl.h>

/**
 *  @brief Adds a Location to global trees for subscription processing
 *
 *  @param *loc_cb Location CB (#HM_LOCATION_CB) of the location to be added to Global DB
 *  @param status Status of the Location (ACTIVE/INACTIVE)
 *  @return #HM_OK on success, #HM_ERR otherwise.
 */
int32_t hm_global_location_add(HM_LOCATION_CB *loc_cb, uint32_t status)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_GLOBAL_LOCATION_CB *glob_cb = NULL;
  int32_t ret_val = HM_OK;
  int32_t trigger = FALSE;
  HM_SUBSCRIBER_WILDCARD *greedy = NULL;

  HM_SUBSCRIPTION_CB *sub_cb = NULL;
  HM_LIST_BLOCK *list_member = NULL;

  uint32_t *processed = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* First check if we don't have a node with same location ID in tree      */
  /***************************************************************************/
  /* NOTE: Currently, we're keying and sorting it by HW ID only. This is so  */
  /* because the system has a constraint that HW ID will always be unique.   */
  /* The commented code segment would be useful when sifting criteria is not */
  /* on the basis of keys.                           */
  /*
  for(glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIRST(LOCAL.locations_tree,
                            locations_tree_by_db_id);
    glob_cb != NULL;
    glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_NEXT(glob_cb->node,
                            locations_tree_by_db_id))
  {

  }
  */
  glob_cb = (HM_GLOBAL_LOCATION_CB *)HM_AVL3_FIND(LOCAL.locations_tree,
                    &loc_cb->index, locations_tree_by_db_id);
  if(glob_cb != NULL)
  {
    /* THE DESIRED BEHAVIOR IF THIS CONDITION OCCURS IS NOT THOUGHT AS OF YET */
    /* FIXME:                                  */
    /* It is hard to say that such a condition will not arise. This is because*/
    /* we are operating on a cluster, and conflicts may arise.                */
    /* But because, at the given moment, we are in control of the cluster     */
    /* topology (closed system design), we'll handle it as a non-recoverable  */
    /* error.                                  */

    TRACE_WARN(("Found a previous node with the same index."));

    /* Currently ineffective and unhittable. This is just an evolution path.  */
    trigger = TRUE;
    if(glob_cb->status == status)
    {
      TRACE_DETAIL(("States are same. Do not trigger notifications!"));
      trigger = FALSE;
    }
  }
  else
  {
    /***************************************************************************/
    /* Allocate a global location CB                       */
    /***************************************************************************/
    glob_cb = (HM_GLOBAL_LOCATION_CB *)malloc(sizeof(HM_GLOBAL_LOCATION_CB));
    TRACE_ASSERT(glob_cb != NULL);
    if(glob_cb == NULL)
    {
      TRACE_ERROR(("Error allocating resources for Location Entry."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    glob_cb->id = LOCAL.next_loc_tree_id++;
    glob_cb->index =  loc_cb->index;
    HM_AVL3_INIT_NODE(glob_cb->node, glob_cb);
    glob_cb->sub_cb = NULL;
    glob_cb->table_type = HM_TABLE_TYPE_LOCATION;
  }
  glob_cb->loc_cb = loc_cb;
  glob_cb->status = status;

  /***************************************************************************/
  /* Try inserting it into the tree                       */
  /***************************************************************************/
  if(!HM_AVL3_IN_TREE(glob_cb->node))
  {
    TRACE_DETAIL(("Inserting"));
    if(HM_AVL3_INSERT(LOCAL.locations_tree, glob_cb->node, locations_tree_by_db_id) != TRUE)
    {
      TRACE_ERROR(("Error inserting into global trees."));
      ret_val = HM_ERR;
      free(glob_cb);
      glob_cb = NULL;
      goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Create a subscription entry                         */
    /***************************************************************************/
    if((sub_cb = hm_create_subscription_entry(HM_TABLE_TYPE_LOCATION, loc_cb->index,
                             (void *)glob_cb))== NULL)
    {
      TRACE_ERROR(("Error creating subscription."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    glob_cb->sub_cb = sub_cb;
  }
  /***************************************************************************/
  /* Update pointers on Location CB too.                     */
  /***************************************************************************/
  loc_cb->id = glob_cb->id;
  loc_cb->db_ptr = (void *)glob_cb;

  /***************************************************************************/
  /* Find out if there is some greedy (wildcard) subscriber.           */
  /* If present, subscribe to this location implicitly.             */
  /***************************************************************************/
  for(greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
      greedy != NULL;
      greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(greedy->node))
  {
    if((greedy->subs_type == HM_CONFIG_ATTR_SUBS_TYPE_LOCATION) &&
        ((greedy->value == 0)||(greedy->value==loc_cb->index)))
    {
      TRACE_DETAIL(("Found wildcard subscriber."));
      /***************************************************************************/
      /* Allocate Node                               */
      /***************************************************************************/
      list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK) + sizeof(uint32_t));
      if(list_member == NULL)
      {
        TRACE_ERROR(("Error allocating resources for Subscriber list element."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      HM_INIT_LQE(list_member->node, list_member);
      list_member->target = greedy->subscriber.node_cb;
      list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
      processed = (uint32_t *)list_member->opaque;
      *processed = 0;

      if(hm_subscription_insert(sub_cb, list_member) != HM_OK)
      {
        TRACE_ERROR(("Error inserting subscription to its entity"));
        ret_val = HM_ERR;
        hm_free_subscription_cb(sub_cb);
        free(list_member);
        sub_cb = NULL;
        list_member = NULL;
        goto EXIT_LABEL;
      }
    }
  }

  if(trigger == TRUE)
  {
    TRACE_DETAIL(("Trigger notification to existing subscribers."));
    /***************************************************************************/
    /* Send out update to cluster if cluster is enabled.             */
    /***************************************************************************/
    if(hm_global_location_update(loc_cb, HM_UPDATE_RUN_STATUS)==HM_ERR)
    {
      TRACE_ERROR(("Error updating Global Location Tables."));
      ret_val = HM_ERR;
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_global_location_add */


/**
 *  @brief Looks into the Location structure of the Location for updates and perform updates.
 *
 *  @note The Status must have been updated in the passed #HM_LOCATION_CB structure
 * before calling into this method.
 *
 *  @param *loc_cb Location CB of which status has been updated.
 *
 *
 *  @return #HM_OK if successful, #HM_ERR otherwise.
 */
int32_t hm_global_location_update(HM_LOCATION_CB *loc_cb, uint32_t op)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t notify;

  HM_GLOBAL_LOCATION_CB *glob_cb = NULL;
  HM_NOTIFICATION_CB *notify_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(loc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Find Global Location pointer                         */
  /***************************************************************************/
  TRACE_ASSERT(loc_cb->db_ptr != NULL);
  glob_cb = (HM_GLOBAL_LOCATION_CB *)loc_cb->db_ptr;
  glob_cb->status = glob_cb->loc_cb->fsm_state;

  switch (op)
  {
    case HM_UPDATE_RUN_STATUS:
      /***************************************************************************/
      /* There are not going to be any notifications as such.             */
      /* But we need to send out updates on cluster.                 */
      /***************************************************************************/
      switch(glob_cb->status)
      {
      case HM_PEER_FSM_STATE_ACTIVE:
        TRACE_DETAIL(("Location Active."));
        notify = HM_NOTIFICATION_LOCATION_ACTIVE;
        break;
      case HM_PEER_FSM_STATE_FAILED:
        TRACE_DETAIL(("Location Down."));
        notify = HM_NOTIFICATION_LOCATION_INACTIVE;
        break;
      default:
        TRACE_WARN(("Unknown HM Status %d", glob_cb->status));
        TRACE_ASSERT(FALSE);
      }
      break;

    default:
      TRACE_WARN(("Invalid Operation type specified: %d", op));
      TRACE_ASSERT(FALSE);
  }

  /***************************************************************************/
  /* Allocate a Notification CB and Add that notification CB to Notify Queue */
  /***************************************************************************/
  notify_cb =  hm_alloc_notify_cb();
  if(notify_cb == NULL)
  {
    TRACE_ERROR(("Error creating Notification CB"));
    TRACE_ERROR(("Update could not be propagated."));
    ret_val = HM_ERR;
  }
  notify_cb->node_cb.location_cb = glob_cb;
  notify_cb->notification_type = notify;
  notify_cb->id = LOCAL.next_notification_id++;
  /***************************************************************************/
  /* Queue the notification CB                          */
  /***************************************************************************/
  HM_INSERT_BEFORE(LOCAL.notification_queue ,notify_cb->node);

  //FIXME: Move it to a separate thread later
  hm_service_notify_queue();


  /***************************************************************************/
  /* Send Notifications on the cluster too                   */
  /***************************************************************************/
  /*
     TRACE_DETAIL(("Update cluster."));
    hm_cluster_send_update(glob_cb);
  */
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_global_location_update */

/**
 *  @brief Removes the Location CB from global DB
 *
 *  @param *loc_cb Removes the global location CB (#HM_GLOBAL_LOCATION_CB) from DB
 *  @return #HM_OK if sucsessful, else #HM_ERR
 */
int32_t hm_global_location_remove(HM_GLOBAL_LOCATION_CB *loc_cb)
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
}/* hm_global_location_remove */


/**
 *  @brief Adds the node to global aggregate trees and handles the subscriptions processing.
 *
 *  @param *node_cb Node CB pointer (#HM_NODE_CB) that needs to be added into global DB
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_global_node_add(HM_NODE_CB *node_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_GLOBAL_NODE_CB *insert_cb = NULL;
  int32_t ret_val = HM_OK;
  HM_SUBSCRIPTION_CB *sub_cb = NULL, *cross_bind = NULL;
  HM_SUBSCRIBER_WILDCARD *greedy = NULL;

  HM_LIST_BLOCK *list_member = NULL, *cross_bind_member=NULL;
  uint32_t *processed = NULL;

  int32_t subscriber_type;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(node_cb!= NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* First search for a node with same node ID. If not found, grant this CB  */
  /* a new ID, and insert it in the tree.                     */
  /*                                       */
  /* NOTE: We are currently using node index for global keying, because it is*/
  /* guaranteed to be unique in current design.                 */
  /***************************************************************************/
  insert_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
                      &node_cb->index,
                      nodes_tree_by_db_id);
  if(insert_cb != NULL)
  {
    /***************************************************************************/
    /* Same node found.                               */
    /***************************************************************************/
    TRACE_ERROR(("Same node found. This shouldn't happen!"));
    /* THE DESIRED BEHAVIOR IF THIS CONDITION OCCURS IS NOT THOUGHT AS OF YET */
    /* FIXME:                                  */
    /* It is hard to say that such a condition will not arise. This is because*/
    /* we are operating on a cluster, and conflicts may arise.          */
    /* But because, at the given moment, we are in control of the cluster     */
    /* topology (closed system design), we'll handle it as a non-recoverable  */
    /* error.                                  */
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  else
  {
    /***************************************************************************/
    /* Allocate a global location CB                       */
    /***************************************************************************/
    TRACE_DETAIL(("New node %d added", node_cb->index));
    insert_cb = (HM_GLOBAL_NODE_CB *)malloc(sizeof(HM_GLOBAL_NODE_CB));
    TRACE_ASSERT(insert_cb != NULL);
    if(insert_cb == NULL)
    {
      TRACE_ERROR(("Error allocating resources for Location Entry."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }

    insert_cb->id = LOCAL.next_node_tree_id++;
    HM_AVL3_INIT_NODE(insert_cb->node, insert_cb);
    HM_INIT_ROOT(insert_cb->subscriptions);
    insert_cb->index =  node_cb->index;
    insert_cb->group_index = node_cb->group;
    insert_cb->table_type = HM_TABLE_TYPE_NODES;
  }

  TRACE_DETAIL(("Node DB ID: %d", insert_cb->id));
  insert_cb->node_cb = node_cb;
  insert_cb->role = node_cb->role;
  insert_cb->status = node_cb->fsm_state;

  /***************************************************************************/
  /* Try inserting it into the tree                       */
  /***************************************************************************/
  if(!HM_AVL3_IN_TREE(insert_cb->node))
  {
    TRACE_DETAIL(("Inserting"));
    if(HM_AVL3_INSERT(LOCAL.nodes_tree, insert_cb->node, nodes_tree_by_db_id) != TRUE)
    {
      TRACE_ERROR(("Error inserting into global trees."));
      ret_val = HM_ERR;
      free(insert_cb);
      insert_cb = NULL;
      goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Create a subscription entry                         */
    /***************************************************************************/
    if((sub_cb = hm_create_subscription_entry(HM_TABLE_TYPE_NODES, insert_cb->index,
                             (void *)insert_cb))== NULL)
    {
      TRACE_ERROR(("Error creating subscription."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    insert_cb->sub_cb = sub_cb;
  }
  /***************************************************************************/
  /* Update pointers on Location CB too.                     */
  /***************************************************************************/
  node_cb->id = insert_cb->id;
  node_cb->db_ptr = (void *)insert_cb;

  /***************************************************************************/
  /* Find out if there is some greedy (wildcard) subscriber.           */
  /* If present, subscribe to this location implicitly.             */
  /***************************************************************************/
  for(greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
      greedy != NULL;
      greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(greedy->node))
  {
    if((greedy->subs_type == HM_CONFIG_ATTR_SUBS_TYPE_GROUP) &&
        ((greedy->value == 0)||(greedy->value==node_cb->group)))
    {
      TRACE_DETAIL(("Found wildcard subscriber."));
      /***************************************************************************/
      /* Allocate Node                               */
      /***************************************************************************/
      list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK) +sizeof(uint32_t));
      if(list_member == NULL)
      {
        TRACE_ERROR(("Error allocating resources for Subscriber list element."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
      HM_INIT_LQE(list_member->node, list_member);
      list_member->target = greedy->subscriber.node_cb;
      list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
      processed = (uint32_t *)list_member->opaque;
      *processed = 0;
      if(hm_subscription_insert(sub_cb, list_member) != HM_OK)
      {
        TRACE_ERROR(("Error inserting subscription to its entity"));
        ret_val = HM_ERR;
        hm_free_subscription_cb(sub_cb);
        free(list_member);
        sub_cb = NULL;
        list_member = NULL;
        goto EXIT_LABEL;
      }

      /***************************************************************************/
      /* if the wildcard is also a bidirectional subscriber, setup cross-binds   */
      /***************************************************************************/
      if(greedy->cross_bind)
      {
        TRACE_DETAIL(("Setup cross-binding."));
        /***************************************************************************/
        /* Find the type of subscriber                                             */
        /***************************************************************************/
        TRACE_ASSERT(greedy->subscriber.void_cb!=NULL);
        subscriber_type =*(int32_t *)((char *)greedy->subscriber.void_cb
                                                      + (uint32_t)(sizeof(int32_t)));

        /***************************************************************************/
        /* Allocate Node.                                                          */
        /***************************************************************************/
        cross_bind_member = NULL;
        cross_bind_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK)+ sizeof(uint32_t));
        if(cross_bind_member == NULL)
        {
          TRACE_ERROR(("Error allocating resources for Subscriber list element."));
          hm_free_subscription_cb(sub_cb);
          free(list_member);
          sub_cb = NULL;
          list_member = NULL;
          goto EXIT_LABEL;
        }
        HM_INIT_LQE(cross_bind_member->node, cross_bind_member);
        cross_bind_member->opaque = (void *)((char *)cross_bind_member + sizeof(HM_LIST_BLOCK));
        cross_bind_member->target = (void *)insert_cb; //greedy->subscriber.void_cb;

        processed = (uint32_t *)cross_bind_member->opaque;
        *processed = 0;

        switch(subscriber_type)
        {
          case HM_TABLE_TYPE_NODES:
            TRACE_DETAIL(("Global Node Subscriber"));
            cross_bind = greedy->subscriber.node_cb->sub_cb;
            break;
          case HM_TABLE_TYPE_NODES_LOCAL:
            TRACE_DETAIL(("Node Subscriber"));
            cross_bind =
                ((HM_GLOBAL_NODE_CB *)greedy->subscriber.proper_node_cb->db_ptr)->sub_cb;
            break;
          case HM_TABLE_TYPE_PROCESS:
            TRACE_DETAIL(("Global Process Subscriber"));
            cross_bind = greedy->subscriber.process_cb->sub_cb;
            break;

          case HM_TABLE_TYPE_PROCESS_LOCAL:
            TRACE_DETAIL(("Process Subscriber"));
            cross_bind =
                ((HM_GLOBAL_PROCESS_CB *)greedy->subscriber.proper_node_cb->db_ptr)->sub_cb;
            break;

          default:
            TRACE_DETAIL(("Unknown type %d", subscriber_type));
            TRACE_ASSERT(0!=0);
        }

        /***************************************************************************/
        /* Insert into List                                                        */
        /***************************************************************************/
        if((ret_val  = hm_subscription_insert(cross_bind, cross_bind_member)) != HM_OK)
        {
          TRACE_ERROR(("Error inserting subscription to its entity"));
          if(ret_val == HM_DUP)
          {
            /* It isn't a critical error. Could be caused by a duplicate entry */
            TRACE_WARN(("Duplicate Registration"));
            ret_val = HM_OK;
            free(cross_bind_member);
            cross_bind_member = NULL;
          }
          else
          {
            ret_val = HM_ERR;
            free(cross_bind_member);
            cross_bind_member = NULL;
            hm_free_subscription_cb(sub_cb);
            free(list_member);
            sub_cb = NULL;
            list_member = NULL;
            goto EXIT_LABEL;
          }
        }
      }
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_global_node_add */


/**
 *  @brief Updates the node control block and notifies the rest.
 *
 *  @note The State transition must have taken place already, we do not compare
 *  old state with new state in this routine.
 *  @param *node_cb Node CB whose state was updated.
 *  @param op Type of operation for which update is to be sent
 *
 *  @return return_value
 */
int32_t hm_global_node_update(HM_NODE_CB *node_cb, uint32_t op)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t notify = FALSE;
  HM_GLOBAL_NODE_CB *glob_cb = NULL;
  HM_NOTIFICATION_CB *notify_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(node_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  switch(op)
  {
    case HM_UPDATE_RUN_STATUS:
      /***************************************************************************/
      /* Look into the node state in its FSM variable to determine if an update  */
      /* is needed or not.                             */
      /***************************************************************************/
      switch(node_cb->fsm_state)
      {
        case HM_NODE_FSM_STATE_ACTIVE:
          TRACE_DETAIL(("Node moved to active state. Send Notifications."));
          TRACE_ASSERT(node_cb->parent_location_cb->active_nodes>=0);
          notify = HM_NOTIFICATION_NODE_ACTIVE;
          break;

        case HM_NODE_FSM_STATE_FAILING:
          TRACE_DETAIL(("Node is no longer active. Send Notifications."));
          TRACE_ASSERT(node_cb->parent_location_cb->active_nodes>=0);
          notify = HM_NOTIFICATION_NODE_INACTIVE;
          break;

        case HM_NODE_FSM_STATE_FAILED:
          TRACE_DETAIL(("Node is in failed state. Notifications must have been sent if required."));
          break;

        default:
          TRACE_WARN(("Unknown state of Node %d", node_cb->fsm_state));
          TRACE_ASSERT(FALSE);
      }
      break;

      case HM_UPDATE_NODE_ROLE:
        TRACE_DETAIL(("Node Role update."));
        /* Active and None behave alike. */
        TRACE_DETAIL(("Node must be %s",
            (node_cb->role== NODE_ROLE_PASSIVE)?"Passive":"Active/None"));
        notify = (node_cb->role== NODE_ROLE_PASSIVE)?
            HM_NOTIFICATION_NODE_ROLE_PASSIVE:HM_NOTIFICATION_NODE_ROLE_ACTIVE;
        break;

      default:
        TRACE_WARN(("Unknown operation: %d", op));
        TRACE_ASSERT(FALSE);
  }

  if(!notify)
  {
    TRACE_DETAIL(("No updates."));
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* We need to send updates. These work irrespective of previous state. This*/
  /* means that the caller must ensure if a notification must be issued or   */
  /* not. If old and new states are same ('Up', and 'Up' somehow), two same  */
  /* notifications would be sent.                         */
  /***************************************************************************/
  /***************************************************************************/
  /* Find the node in DB.                             */
  /***************************************************************************/
  glob_cb = (HM_GLOBAL_NODE_CB *)HM_AVL3_FIND(LOCAL.nodes_tree,
                      &node_cb->index, nodes_tree_by_db_id);
  if(glob_cb == NULL)
  {
    TRACE_ERROR(("No Global DB Entry found for this node. This shouldn't happen."));
    TRACE_ASSERT(glob_cb != NULL);
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* If subscription was not live, it is now.                   */
  /***************************************************************************/
  /***************************************************************************/
  /* Move this subscription node into the active subscriptions tree.       */
  /* And also notify subscribers.                         */
  /***************************************************************************/
  if(glob_cb->sub_cb->live == FALSE)
  {
    TRACE_DETAIL(("Subscription Now Active."));
    HM_AVL3_DELETE(LOCAL.pending_subscriptions_tree, glob_cb->sub_cb->node);
    if(HM_AVL3_INSERT(LOCAL.active_subscriptions_tree, glob_cb->sub_cb->node,
                            subs_tree_by_db_id)!= TRUE)
    {
      TRACE_ERROR(("Error activating subscription."));
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Mark the subscription as active.                       */
    /***************************************************************************/
    glob_cb->sub_cb->live = TRUE;
  }
  glob_cb->status = node_cb->fsm_state;
  if(op==HM_UPDATE_NODE_ROLE)
  {
    TRACE_DETAIL(("Update Role in Global Tables"));
    glob_cb->role = node_cb->current_role;
  }
  /***************************************************************************/
  /* Allocate a Notification CB and Add that notification CB to Notify Queue */
  /***************************************************************************/
  notify_cb =  hm_alloc_notify_cb();
  if(notify_cb == NULL)
  {
    TRACE_ERROR(("Error creating Notification CB"));
    TRACE_ERROR(("Update could not be propagated."));
    ret_val = HM_ERR;
  }
  notify_cb->node_cb.node_cb = glob_cb;
  notify_cb->notification_type = notify;
  notify_cb->id = LOCAL.next_notification_id++;
  /***************************************************************************/
  /* Queue the notification CB                          */
  /***************************************************************************/
  HM_INSERT_BEFORE(LOCAL.notification_queue ,notify_cb->node);

  //FIXME: Move it to a separate thread later
  hm_service_notify_queue();

  /***************************************************************************/
  /* Send Notifications on the cluster too                   */
  /***************************************************************************/
  if(glob_cb->node_cb->parent_location_cb->index ==
      LOCAL.local_location_cb.index)
  {
    TRACE_DETAIL(("Update cluster."));
    hm_cluster_send_update(glob_cb);
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_global_node_update */


/**
 *  @brief Removes the node from global tables and carries out subscription routines
 *
 *  @param *node_cb #HM_NODE_CB control block of Node that must be removed from the system
 *  @return #HM_OK if successfull, #HM_ERR otherwise
 */
int32_t hm_global_node_remove(HM_NODE_CB *node_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_global_node_remove */


/**
 *  @brief Adds a Process to Global Tables
 *
 *  @param *proc_cb #HM_PROCESS_CB type structure that needs to be added to global DB
 *  @return #HM_OK if successful, #HM_ERR otherwise.
 */
int32_t hm_global_process_add(HM_PROCESS_CB *proc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_GLOBAL_PROCESS_CB *insert_cb = NULL, *temp_cb = NULL;
  int32_t ret_val = HM_OK;
  HM_SUBSCRIPTION_CB *sub_cb = NULL, *cross_bind = NULL;
  HM_SUBSCRIBER_WILDCARD *greedy = NULL;
  HM_LIST_BLOCK *list_member = NULL, *cross_bind_member=NULL;
  uint32_t *processed = NULL;

  int32_t subscriber_type;
  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(proc_cb!= NULL);

  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  /***************************************************************************/
  /* Allocate a global location CB                                           */
  /*                                                                         */
  /* Owing to a composite comapre function, we first allocate a Global Proc  */
  /* CB and fill in the keys for comparison.                                 */
  /***************************************************************************/
  temp_cb = (HM_GLOBAL_PROCESS_CB *)malloc(sizeof(HM_GLOBAL_PROCESS_CB));
  TRACE_ASSERT(temp_cb != NULL);
  if(temp_cb==NULL)
  {
    TRACE_ERROR(("Error allocating resources for Location Entry."));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  temp_cb->pid =  proc_cb->pid;
  temp_cb->node_index = proc_cb->parent_node_cb->index;
  temp_cb->type = proc_cb->type;
  temp_cb->proc_cb = proc_cb;

  /***************************************************************************/
  /* First search for a node with same Process ID. If not found, grant the CB*/
  /* a new ID, and insert it in the tree.                                    */
  /*                                                                         */
  /* We need to insert the node by its proc_type, node_id and PID for unique-*/
  /* -ness constraint. As such db_id is of no use here.                      */
  /***************************************************************************/
  insert_cb = (HM_GLOBAL_PROCESS_CB *)HM_AVL3_FIND(LOCAL.process_tree,
                      temp_cb,
                      global_process_tree_by_id);
  if(insert_cb != NULL)
  {
    /***************************************************************************/
    /* Same node found.                               */
    /***************************************************************************/
    TRACE_ERROR(("Same process found. Validate"));
    if(insert_cb->status == temp_cb->proc_cb->running)
    {
      TRACE_ERROR(("Previous and current Running Statuses are same. Must not happen."));
      TRACE_ERROR(("States: %s", insert_cb->status==TRUE?"Running":"Stopped"));
      free(temp_cb);
      temp_cb = NULL;
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
  }
  else
  {
    TRACE_DETAIL(("New Process Node."));
    insert_cb = temp_cb;
    temp_cb = NULL;

    /***************************************************************************/
    /* Now fill in the rest                             */
    /***************************************************************************/
    insert_cb->id = LOCAL.next_process_tree_id++;

    HM_AVL3_INIT_NODE(insert_cb->node, insert_cb);
    HM_INIT_ROOT(insert_cb->subscriptions);
    insert_cb->table_type = HM_TABLE_TYPE_PROCESS;
  }
  insert_cb->proc_cb = proc_cb;
  insert_cb->status = proc_cb->running;

  TRACE_DETAIL(("Process CB DB ID: %d", insert_cb->id));

  /***************************************************************************/
  /* Try inserting it into the tree                       */
  /***************************************************************************/
  if(!HM_AVL3_IN_TREE(insert_cb->node))
  {
    TRACE_DETAIL(("Inserting %p with node at %p", insert_cb, &insert_cb->node));
    if(HM_AVL3_INSERT(LOCAL.process_tree, insert_cb->node, global_process_tree_by_id) != TRUE)
    {
      TRACE_ERROR(("Error inserting into global trees."));
      ret_val = HM_ERR;
      free(insert_cb);
      insert_cb = NULL;
      goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Create a subscription entry                         */
    /* This process is subscribe-able for PCT_Type.                 */
    /* Since type is not used in Key, many Processes can have same PCT_Type     */
    /***************************************************************************/
    if((sub_cb = hm_create_subscription_entry(HM_TABLE_TYPE_PROCESS, insert_cb->type,
                             (void *)insert_cb))== NULL)
    {
      TRACE_ERROR(("Error creating subscription."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    insert_cb->sub_cb = sub_cb;
  }
  /***************************************************************************/
  /* Update pointers on Location CB too.                     */
  /***************************************************************************/
  proc_cb->id = insert_cb->id;
  proc_cb->db_ptr = (void *)insert_cb;

  /***************************************************************************/
  /* Try only when it is a new insertion.                                    */
  /***************************************************************************/
  if(HM_AVL3_IN_TREE(insert_cb->node))
  {
    /***************************************************************************/
    /* Find out if there is some greedy (wildcard) subscriber.                 */
    /* If present, subscribe to this location implicitly.                      */
    /***************************************************************************/
    for(greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
        greedy != NULL;
        greedy = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(greedy->node))
    {
      if((greedy->subs_type == HM_CONFIG_ATTR_SUBS_TYPE_PROC) &&
          ((greedy->value == 0)||(greedy->value==proc_cb->type)))
      {
        TRACE_DETAIL(("Found wildcard subscriber."));
        /***************************************************************************/
        /* Allocate Node                               */
        /***************************************************************************/
        list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK)+sizeof(uint32_t));
        if(list_member == NULL)
        {
          TRACE_ERROR(("Error allocating resources for Subscriber list element."));
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        HM_INIT_LQE(list_member->node, list_member);
        list_member->target = greedy->subscriber.node_cb;
        list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
        processed = (uint32_t *)list_member->opaque;
        *processed = 0;

        if(hm_subscription_insert(sub_cb, list_member) != HM_OK)
        {
          TRACE_ERROR(("Error inserting subscription to its entity"));
          ret_val = HM_ERR;
          hm_free_subscription_cb(sub_cb);
          free(list_member);
          sub_cb = NULL;
          list_member = NULL;
          goto EXIT_LABEL;
        }

        /***************************************************************************/
        /* if the wildcard is also a bidirectional subscriber, setup cross-binds   */
        /***************************************************************************/
        if(greedy->cross_bind)
        {
          TRACE_DETAIL(("Setup cross-binding."));
          /***************************************************************************/
          /* Find the type of subscriber                                             */
          /***************************************************************************/
          TRACE_ASSERT(greedy->subscriber.void_cb!=NULL);
          subscriber_type =*(int32_t *)((char *)greedy->subscriber.void_cb
                                                        + (uint32_t)(sizeof(int32_t)));

          /***************************************************************************/
          /* Allocate Node.                                                          */
          /***************************************************************************/
          cross_bind_member = NULL;
          cross_bind_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK)+ sizeof(uint32_t));
          if(cross_bind_member == NULL)
          {
            TRACE_ERROR(("Error allocating resources for Subscriber list element."));
            hm_free_subscription_cb(sub_cb);
            free(list_member);
            sub_cb = NULL;
            list_member = NULL;
            goto EXIT_LABEL;
          }
          HM_INIT_LQE(cross_bind_member->node, cross_bind_member);
          cross_bind_member->opaque = (void *)((char *)cross_bind_member + sizeof(HM_LIST_BLOCK));
          cross_bind_member->target = (void *)insert_cb; //greedy->subscriber.void_cb;

          processed = (uint32_t *)cross_bind_member->opaque;
          *processed = 0;

          switch(subscriber_type)
          {
            case HM_TABLE_TYPE_NODES:
              TRACE_DETAIL(("Global Node Subscriber"));
              cross_bind = greedy->subscriber.node_cb->sub_cb;
              break;
            case HM_TABLE_TYPE_NODES_LOCAL:
              TRACE_DETAIL(("Node Subscriber"));
              cross_bind =
                  ((HM_GLOBAL_NODE_CB *)greedy->subscriber.proper_node_cb->db_ptr)->sub_cb;
              break;
            case HM_TABLE_TYPE_PROCESS:
              TRACE_DETAIL(("Global Process Subscriber"));
              cross_bind = greedy->subscriber.process_cb->sub_cb;
              break;

            case HM_TABLE_TYPE_PROCESS_LOCAL:
              TRACE_DETAIL(("Process Subscriber"));
              cross_bind =
                  ((HM_GLOBAL_PROCESS_CB *)greedy->subscriber.proper_node_cb->db_ptr)->sub_cb;
              break;

            default:
              TRACE_DETAIL(("Unknown type %d", subscriber_type));
              TRACE_ASSERT(0!=0);
          }

          /***************************************************************************/
          /* Insert into List                                                        */
          /***************************************************************************/
          if((ret_val  = hm_subscription_insert(cross_bind, cross_bind_member)) != HM_OK)
          {
            TRACE_ERROR(("Error inserting subscription to its entity"));
            if(ret_val == HM_DUP)
            {
              /* It isn't a critical error. Could be caused by a duplicate entry */
              TRACE_WARN(("Duplicate Registration"));
              ret_val = HM_OK;
              free(cross_bind_member);
              cross_bind_member = NULL;
            }
            else
            {
              ret_val = HM_ERR;
              free(cross_bind_member);
              cross_bind_member = NULL;
              hm_free_subscription_cb(sub_cb);
              free(list_member);
              sub_cb = NULL;
              list_member = NULL;
              goto EXIT_LABEL;
            }
          }
        }
      }
    }
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_global_process_add */


/**
 *  @brief Updates the status and does notification triggers of processes
 *
 *  @param *proc_cb Process CB (#HM_PROCESS_CB) whose state has been updated.
 *  @return #HM_OK if successful, #HM_ERR otherwise.
 */
int32_t hm_global_process_update(HM_PROCESS_CB *proc_cb, uint32_t op)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t notify = FALSE;
  HM_GLOBAL_PROCESS_CB *glob_cb = NULL;
  HM_NOTIFICATION_CB *notify_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(proc_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  switch (op)
  {
    case HM_UPDATE_RUN_STATUS:
    /***************************************************************************/
    /* Look into the node state in its FSM variable to determine if an update  */
    /* is needed or not.                             */
    /***************************************************************************/
    if(proc_cb->running)
    {
      TRACE_DETAIL(("Process active. Send Notifications."));
      notify = HM_NOTIFICATION_PROCESS_CREATED;
      TRACE_ASSERT(proc_cb->parent_node_cb->parent_location_cb->active_processes >=0);
    }
    else
    {
      TRACE_DETAIL(("Process is no longer active. Send Notifications."));
      notify = HM_NOTIFICATION_PROCESS_DESTROYED;
      TRACE_ASSERT(proc_cb->parent_node_cb->parent_location_cb->active_processes >=0);
    }
    break;

    default:
      TRACE_WARN(("Unknown operation: %d", op));
      TRACE_ASSERT(FALSE);
  }

  if(!notify)
  {
    TRACE_DETAIL(("No updates."));
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* We need to send updates. These work irrespective of previous state. This*/
  /* means that the caller must ensure if a notification must be issued or   */
  /* not. If old and new states are same ('Up', and 'Up' somehow), two same  */
  /* notifications would be sent.                         */
  /***************************************************************************/
  /***************************************************************************/
  /* Find the process in DB.                           */
  /* The funny problem with not creating a custom key here (that is made from*/
  /* the shuffling of values in proc_cb to make it resemble HM_GLOBAL_PROC_CB*/
  /* and for every update, I don't want to make a temporary variable.       */
  /* Also, since this isn't exactly DB, I have a pointer which MUST NOT be   */
  /* NULL in the absence of any programmatic errors.               */
  /***************************************************************************/
  /*
  glob_cb = (HM_GLOBAL_PROCESS_CB *)HM_AVL3_FIND(LOCAL.process_tree,
                      proc_cb,
                      global_process_tree_by_id);
  if(glob_cb == NULL)
  {
    TRACE_ERROR(("No Global DB Entry found for this node. This shouldn't happen."));
    TRACE_ASSERT(glob_cb != NULL);
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  */
  glob_cb = (HM_GLOBAL_PROCESS_CB *)proc_cb->db_ptr;
  TRACE_ASSERT(glob_cb != NULL);
  if(glob_cb == NULL)
  {
    TRACE_ERROR(("Global Table pointer is NULL. ERROR!"));
    /***************************************************************************/
    /* TERMINAL ERROR! ABORT! ABORT! ABORT!                     */
    /***************************************************************************/
    goto EXIT_LABEL;
  }
  TRACE_DETAIL(("Found CB Type: 0x%x, Node: %d, PID: 0x%x", glob_cb->type,
                                glob_cb->node_index,
                                glob_cb->pid));
  /***************************************************************************/
  /* If subscription was not live, it is now.                   */
  /***************************************************************************/
  /***************************************************************************/
  /* Move this subscription node into the active subscriptions tree.       */
  /* And also notify subscribers.                         */
  /***************************************************************************/
  glob_cb->status = proc_cb->running;
  if(glob_cb->sub_cb->live == FALSE)
  {
    TRACE_DETAIL(("Subscription Now Active."));
    HM_AVL3_DELETE(LOCAL.pending_subscriptions_tree, glob_cb->sub_cb->node);
    if(HM_AVL3_INSERT(LOCAL.active_subscriptions_tree, glob_cb->sub_cb->node,
                            subs_tree_by_db_id)!= TRUE)
    {
      TRACE_ERROR(("Error activating subscription."));
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Mark the subscription as active.                       */
    /***************************************************************************/
    glob_cb->sub_cb->live = TRUE;
  }

  /***************************************************************************/
  /* Allocate a Notification CB and Add that notification CB to Notify Queue */
  /***************************************************************************/
  notify_cb =  hm_alloc_notify_cb();
  if(notify_cb == NULL)
  {
    TRACE_ERROR(("Error creating Notification CB"));
    TRACE_ERROR(("Update could not be propagated."));
    ret_val = HM_ERR;
  }
  notify_cb->node_cb.process_cb = glob_cb;
  notify_cb->notification_type = notify;
  notify_cb->id = LOCAL.next_notification_id++;
  /***************************************************************************/
  /* Queue the notification CB                          */
  /***************************************************************************/
  HM_INSERT_BEFORE(LOCAL.notification_queue ,notify_cb->node);

  //FIXME: Move it to a separate thread later
  hm_service_notify_queue();

  /***************************************************************************/
  /* Send Notifications on the cluster too                   */
  /***************************************************************************/
  if(glob_cb->proc_cb->parent_node_cb->parent_location_cb->index ==
      LOCAL.local_location_cb.index)
  {
    TRACE_DETAIL(("Update cluster."));
    hm_cluster_send_update(glob_cb);
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (ret_val);
}/* hm_global_process_update */


/**
 *  @brief Removes the node from global DBs
 *
 *  @param *proc_cb Process CB (#HM_PROCESS_CB) which needs to be removed from Global DB
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_global_process_remove(HM_PROCESS_CB *proc_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_global_process_remove */


/**
 *  @brief Makes an entry in subscription tree if no previous entry exists
 *
 *  @param subs_type Subscription Type: Whether on Node/Process/Location/Process Group etc.
 *  @return #HM_SUBSCRIPTION_CB pointer if successful @c NULL otherwise
 */
HM_SUBSCRIPTION_CB * hm_create_subscription_entry(uint32_t subs_type, uint32_t value, void *row)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_SUBSCRIPTION_CB *tree_node = NULL;
  HM_SUBSCRIPTION_CB *sub_cb = NULL;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Row may be void. In case the subscriber comes before subscription       */
  /***************************************************************************/

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  TRACE_DETAIL(("Subscription Entry Requested: Type: %d Value: %d",
      subs_type, value));
   /***************************************************************************/
   /* A node added to any aggregate tree can be a subscription. Add a     */
   /* subscription point in the pending subscription tables.            */
   /* It is not currently subscribed to.                      */
   /***************************************************************************/
  sub_cb = hm_alloc_subscription_cb();
  if(sub_cb== NULL)
  {
    TRACE_ERROR(("Error creating subscription point."));
    /***************************************************************************/
    /* Remove from global node tree                         */
    /***************************************************************************/
    goto EXIT_LABEL;
  }
   /***************************************************************************/
   /* We are a node_table type entry                      */
   /***************************************************************************/
   sub_cb->table_type = subs_type;
   sub_cb->value = value;
   sub_cb->row_cb.void_cb = row;

   if(row != NULL)
   {
     sub_cb->row_id = *(int32_t *)row;
     TRACE_DETAIL(("Subscription row ID: %d", sub_cb->row_id));
   }

  /***************************************************************************/
  /* First check the pending list if we have a matching subscription waiting */
  /* This may happen if a subscription was made before its provider could be */
  /* provisioned.                                 */
  /***************************************************************************/
  for(tree_node =
      (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.pending_subscriptions_tree,
                              subs_tree_by_db_id);
    tree_node != NULL;
    tree_node =
      (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(tree_node->node,
                              subs_tree_by_db_id))
  {
    if((tree_node->table_type == sub_cb->table_type) &&
                      (tree_node->value == sub_cb->value))
    {
      TRACE_INFO(("Found a pending subscription."));

#ifdef I_WANT_TO_DEBUG
      if(tree_node->row_cb.void_cb != NULL)
      {
        TRACE_DETAIL(("Previous subscription point has a NON NULL row"));
        TRACE_ASSERT(tree_node->row_id == sub_cb->row_id);
        TRACE_ASSERT(tree_node->row_cb.void_cb == sub_cb->row_cb.void_cb);
      }
#endif
      tree_node->row_cb.void_cb = sub_cb->row_cb.void_cb;
      tree_node->row_id = sub_cb->row_id;

      /***************************************************************************/
      /* Found the node, no need to look any further                 */
      /***************************************************************************/
      hm_free_subscription_cb(sub_cb);
      goto EXIT_LABEL;
    }
  }
  /***************************************************************************/
  /* Previous entry not found. Create a new one.                 */
  /***************************************************************************/
  sub_cb->id = LOCAL.next_pending_tree_id++;
  TRACE_DETAIL(("Subscription ID: %d", sub_cb->id));

  if(HM_AVL3_INSERT(LOCAL.pending_subscriptions_tree, sub_cb->node, subs_tree_by_db_id) != TRUE)
  {
    TRACE_ERROR(("Error creating subscription point."));
    LOCAL.next_pending_tree_id -=1;
    hm_free_subscription_cb(sub_cb);
    goto EXIT_LABEL;
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return sub_cb;
}/* hm_create_subscription_entry */


/**
 *  @brief Updates the subscribers for a particular subscription point
 *
 *  @param *subs_cb Subscription Control Block #HM_SUBSCRIPTION_CB
 *      for which subscribers need to be updated
 *  @return #HM_OK on success, #HM_ERR otherwise.
 */
int32_t hm_update_subscribers(HM_SUBSCRIPTION_CB *subs_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(subs_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_update_subscribers */


/**
 *  @brief Adds a subscription
 *
 *  First traverse the active and pending subscription trees to see if we
 *  already have that kind of subscription point ready. If not, we make one
 *  in the pending tree.
 *
 *  @param subs_type Subscription type
 *  @param value Value of subscription index
 *  @param *cb A Control block of the subscribing entity
 *  @param bidir Whether it is a bidirectional subscription or unidirectional
 *
 *  @return #HM_OK on success, #HM_ERR otherwise
 */
int32_t hm_subscribe(uint32_t subs_type, uint32_t value, void *cb, uint32_t bidir)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_SUBSCRIBER_WILDCARD *subscriber, *looper = NULL;
  HM_SUBSCRIPTION_CB *subscription = NULL;
  HM_SUBSCRIBER global_cb;
  HM_LIST_BLOCK *list_member = NULL;
  int32_t ret_val = HM_OK;

  int32_t exists = FALSE;
  int32_t table_type;
  int32_t keys[2];

  uint32_t *processed = NULL;
  uint32_t subscriber_type;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(cb != NULL);

  TRACE_DETAIL(("Subscription Type: %d. Value= %d", subs_type, value));
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Depending on the subscription type, choose the table             */
  /***************************************************************************/
  switch(subs_type)
  {
  case HM_CONFIG_ATTR_SUBS_TYPE_GROUP:
    table_type = HM_TABLE_TYPE_NODES;
    break;
  case HM_CONFIG_ATTR_SUBS_TYPE_PROC:
    table_type = HM_TABLE_TYPE_PROCESS;
    break;
  case HM_CONFIG_ATTR_SUBS_TYPE_IF:
    table_type = HM_TABLE_TYPE_IF;
    break;
  case HM_CONFIG_ATTR_SUBS_TYPE_LOCATION:
    table_type = HM_TABLE_TYPE_LOCATION;
    break;
  case HM_CONFIG_ATTR_SUBS_TYPE_NODE:
    table_type = HM_TABLE_TYPE_NODES;
    break;
  default:
    TRACE_ERROR(("Unknown subscription group"));
    TRACE_ASSERT(0==1);
  }
  /***************************************************************************/
  /* Group Numbers, Process Types and Interface IDs are wildcards rather than*/
  /* explicit values.                                                        */
  /***************************************************************************/
  if((subs_type == HM_CONFIG_ATTR_SUBS_TYPE_GROUP)
    || (subs_type == HM_CONFIG_ATTR_SUBS_TYPE_PROC)
    || (subs_type == HM_CONFIG_ATTR_SUBS_TYPE_IF)
    ||  value == 0)
  {
    TRACE_DETAIL(("Subscribe to all nodes."));
    /* You can't really be a wildcard which does not have an entity yet */
    TRACE_ASSERT(cb != NULL);
    subscriber = (HM_SUBSCRIBER_WILDCARD *)malloc(sizeof(HM_SUBSCRIBER_WILDCARD));
    if(subscriber==NULL)
    {
      TRACE_ERROR(("Error allocating resources for Greedy subscriber."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    HM_INIT_LQE(subscriber->node, subscriber);
    subscriber->subs_type = subs_type;
    subscriber->value = value;
    subscriber->subscriber.void_cb = cb;
    subscriber->cross_bind = bidir;
    global_cb.void_cb= cb;
    /***************************************************************************/
    /* Determine its table type and accordingly, find its global table entry   */
    /***************************************************************************/
    if(cb != NULL)
    {
      TRACE_DETAIL(("Checking type %d", *(int32_t *)((char *)cb+ (uint32_t)(sizeof(int32_t)))));
      subscriber_type =*(int32_t *)((char *)cb+ (uint32_t)(sizeof(int32_t)));
      switch(subscriber_type)
      {
      case HM_TABLE_TYPE_NODES_LOCAL:
        TRACE_DETAIL(("Local Node Structure. Find its Global Entry."));
        TRACE_DETAIL(("ID in Global Node Table %d", *(int32_t *)(global_cb.void_cb)));
        subscriber->subscriber.void_cb = global_cb.proper_node_cb->db_ptr;
        TRACE_DETAIL(("Subscriber type %d", *(int32_t *)(
        (char *)subscriber->subscriber.void_cb+ (uint32_t)(sizeof(int32_t)))));
        break;
      case HM_TABLE_TYPE_LOCATION_LOCAL:
        TRACE_DETAIL(("Local Location Structure. Find its Global Entry."));
        subscriber->subscriber.void_cb = global_cb.proper_location_cb->db_ptr;
        break;
      case HM_TABLE_TYPE_PROCESS_LOCAL:
        TRACE_DETAIL(("Local Process Structure. Find its Global Entry."));
        subscriber->subscriber.void_cb = global_cb.proper_process_cb->db_ptr;
        break;
      case HM_TABLE_TYPE_PROCESS:
        TRACE_DETAIL(("Remote Process Structure. Find its Global Entry."));
        subscriber->subscriber.void_cb = global_cb.proper_process_cb->db_ptr;
        break;

      default:
        TRACE_WARN(("Unknown type of subscriber"));
        TRACE_ASSERT((FALSE));
      }
    }

    /***************************************************************************/
    /* Insert to wildcard list.                           */
    /***************************************************************************/
    TRACE_DETAIL(("Add to greedy list."));
    /***************************************************************************/
    /* First check if the node is subscribed already, and if it is, then is it */
    /* subscribed to the same table for a wildcard or same value?         */
    /***************************************************************************/
    for(looper = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(LOCAL.table_root_subscribers);
      looper != NULL;
      looper = (HM_SUBSCRIBER_WILDCARD *)HM_NEXT_IN_LIST(looper->node))
    {
      if(looper->subscriber.void_cb == subscriber->subscriber.void_cb)
      {
        TRACE_DETAIL(("Subscription in Wildcard exists. Check exactness of rule"));
        if(looper->subs_type == subscriber->subs_type)
        {
          TRACE_DETAIL(("Subscription Type also matches. Check value!"));
          if((looper->value == 0) || (looper->value == subscriber->value))
          {
            TRACE_WARN(("Subscriber is re-subscribing to same values."));
            /***************************************************************************/
            /* Nothing needs to be done. Everything is already set up.                 */
            /***************************************************************************/
            exists = TRUE;
            free(subscriber);
            subscriber = looper;
            break;
          }
        }
      }
    }
    if(!exists)
    {
      TRACE_DETAIL(("New subscription."));
      HM_INSERT_BEFORE(LOCAL.table_root_subscribers, subscriber->node);
    }
    /***************************************************************************/
    /* Insert it as a subscriber to every existing node right now.             */
    /***************************************************************************/
    for(subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.active_subscriptions_tree,
                            subs_tree_by_db_id );
        subscription != NULL;
        subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
                            subs_tree_by_db_id))
    {
      if(subscription->table_type == table_type)
      {
        if((subscriber->value == 0) || (subscriber->value == subscription->value))
        {
          TRACE_DETAIL(("Found a node. Make subscription."));
          /***************************************************************************/
          /* Do not subscribe to itself                                              */
          /***************************************************************************/
          if(subscription->row_cb.void_cb == subscriber->subscriber.void_cb)
          {
            TRACE_DETAIL(("Do not subscribe to itself!"));
            continue;
          }
          /***************************************************************************/
          /* Allocate Node.                                                          */
          /***************************************************************************/
          list_member = NULL;
          list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK)+ sizeof(uint32_t));
          if(list_member == NULL)
          {
            TRACE_ERROR(("Error allocating resources for Subscriber list element."));
            ret_val = HM_ERR;
            goto EXIT_LABEL;
          }
          HM_INIT_LQE(list_member->node, list_member);
          list_member->target = subscriber->subscriber.void_cb;
          list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
          processed = (uint32_t *)list_member->opaque;
          *processed = 0;
          /***************************************************************************/
          /* Insert into List                                                        */
          /***************************************************************************/
          if((ret_val  = hm_subscription_insert(subscription, list_member)) != HM_OK)
          {
            TRACE_ERROR(("Error inserting subscription to its entity"));
            if(ret_val == HM_DUP)
            {
              /* It isn't a critical error. Could be caused by a duplicate entry */
              TRACE_WARN(("Duplicate Registration"));
              ret_val = HM_OK;
              free(list_member);
              list_member = NULL;
            }
            else
            {
              ret_val = HM_ERR;
              free(list_member);
              list_member = NULL;
              goto EXIT_LABEL;
            }
          }
        }
      }
    }
    for(subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.pending_subscriptions_tree,
                                subs_tree_by_db_id );
            subscription != NULL;
            subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
                                subs_tree_by_db_id))
    {
      TRACE_DETAIL(("Types: Node: %d Current Value: %d",subscription->table_type, subs_type));
      if(subscription->table_type == table_type)
      {
        if((subscriber->value == 0) || (subscriber->value == subscription->value))
        {
          /***************************************************************************/
          /* Do not subscribe to itself                         */
          /***************************************************************************/
          if(subscription->row_cb.void_cb == subscriber->subscriber.void_cb)
          {
            TRACE_DETAIL(("Do not subscribe to itself!"));
            continue;
          }

          TRACE_DETAIL(("Found a node. Make subscription."));
          /***************************************************************************/
          /* Allocate Node.                               */
          /***************************************************************************/
          list_member = NULL;
          list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK)+sizeof(uint32_t));
          if(list_member == NULL)
          {
            TRACE_ERROR(("Error allocating resources for Subscriber list element."));
            ret_val = HM_ERR;
            goto EXIT_LABEL;
          }
          HM_INIT_LQE(list_member->node, list_member);
          list_member->target = subscriber->subscriber.void_cb;
          list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
          processed = (uint32_t *)list_member->opaque;
          *processed = 0;
          /***************************************************************************/
          /* Insert into List                               */
          /***************************************************************************/
          if((ret_val  = hm_subscription_insert(subscription, list_member)) != HM_OK)
          {
            TRACE_ERROR(("Error inserting subscription to its entity"));
            if(ret_val == HM_DUP)
            {
              /* It isn't a critical error. Could be caused by a duplicate entry */
              TRACE_WARN(("Duplicate Registration"));
              ret_val = HM_OK;
              free(list_member);
              list_member = NULL;
            }
            else
            {
              ret_val = HM_ERR;
              free(list_member);
              list_member = NULL;
              goto EXIT_LABEL;
            }
          }
        }
      }
    }
  }
  /***************************************************************************/
  /* Else, it is a singular subscription. Find it, or create one.         */
  /***************************************************************************/
  else
  {
    /***************************************************************************/
    /* Look for a subscription point, first in Active subscription trees     */
    /***************************************************************************/
    keys[0]=table_type;
    keys[1]=value;
    /***************************************************************************/
    /* Allocate Node                               */
    /***************************************************************************/
    list_member = (HM_LIST_BLOCK *) malloc(sizeof(HM_LIST_BLOCK) + sizeof(uint32_t));
    if(list_member == NULL)
    {
      TRACE_ERROR(("Error allocating resources for Subscriber list element."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    HM_INIT_LQE(list_member->node, list_member);
    list_member->target = cb;
    list_member->opaque = (void *)((char *)list_member + sizeof(HM_LIST_BLOCK));
    processed = (uint32_t *)list_member->opaque;
    *processed = 0;

    subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.active_subscriptions_tree,
                                                        subs_tree_by_db_id );
    while(subscription!=NULL)
    {
      if(subscription->table_type == keys[0] && subscription->value==keys[1])
      {
        TRACE_DETAIL(("Found subscription at id %d", subscription->id));
        break;
      }
      subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
                                                          subs_tree_by_db_id );
    }
    if(subscription == NULL)
    {
      TRACE_DETAIL(("Try to find in Pending subscriptions"));

      /* FIXME: Possibly redundant. We always check in pending list in create_entry method */
      subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_FIRST(LOCAL.pending_subscriptions_tree,
                                                          subs_tree_by_db_id );
      while(subscription!=NULL)
      {
        if(subscription->table_type == keys[0] && subscription->value==keys[1])
        {
          TRACE_DETAIL(("Found subscription at id %d", subscription->id));
          break;
        }
        subscription = (HM_SUBSCRIPTION_CB *)HM_AVL3_NEXT(subscription->node,
                                                            subs_tree_by_db_id );
      }
    }
    if(subscription == NULL)
    {
      TRACE_DETAIL(("Subscription not found in active or pending lists. Create one."));
      /***************************************************************************/
      /* Create subscription now.                           */
      /***************************************************************************/
      if((subscription = hm_create_subscription_entry(subs_type, value, NULL))== NULL)
      {
        TRACE_ERROR(("Error creating subscription!"));
        ret_val = HM_ERR;
        free(list_member);
        subscription = NULL;
        list_member = NULL;
        goto EXIT_LABEL;
      }
    }
    /***************************************************************************/
    /* Now, either we've found a previous subscription, or created a new one.  */
    /* Add subscriber to a future node in subscription tree             */
    /***************************************************************************/
    if((ret_val  = hm_subscription_insert(subscription, list_member)) != HM_OK)
    {
      TRACE_ERROR(("Error inserting subscription to its entity"));
      if(ret_val == HM_DUP)
      {
        /* It isn't a critical error. Could be caused by a duplicate entry */
        TRACE_WARN(("Duplicate Registration"));
        ret_val = HM_OK;
        free(list_member);
        list_member = NULL;
      }
      else
      {
        ret_val = HM_ERR;
        hm_free_subscription_cb(subscription);
        free(list_member);
        subscription = NULL;
        list_member = NULL;
        goto EXIT_LABEL;
      }
    }
  }

  /***************************************************************************/
  /* If it is a bidirectional subscription, make the crossbind.              */
  /* It is possible that the remote binding does not exist yet.              */
  /***************************************************************************/
  if(bidir == TRUE)
  {
    TRACE_DETAIL(("Bidirectional Subscription."));
    /***************************************************************************/
    /* Find the type of subscriber                                             */
    /***************************************************************************/
    switch(subscriber_type)
    {
      case HM_TABLE_TYPE_NODES:
        TRACE_DETAIL(("Global Node Subscriber"));
        subs_type = HM_CONFIG_ATTR_SUBS_TYPE_NODE;
        value = ((HM_GLOBAL_NODE_CB *)cb)->index;
        break;
      case HM_TABLE_TYPE_NODES_LOCAL:
        TRACE_DETAIL(("Node Subscriber"));
        subs_type = HM_CONFIG_ATTR_SUBS_TYPE_NODE;
        value = ((HM_NODE_CB *)cb)->index;
        break;
      case HM_TABLE_TYPE_PROCESS:
        TRACE_DETAIL(("Global Process Subscriber"));
        subs_type = HM_CONFIG_ATTR_SUBS_TYPE_PROC;
        value = ((HM_GLOBAL_PROCESS_CB *)cb)->pid;
        break;

      case HM_TABLE_TYPE_PROCESS_LOCAL:
        TRACE_DETAIL(("Process Subscriber"));
        subs_type = HM_CONFIG_ATTR_SUBS_TYPE_PROC;
        value = ((HM_PROCESS_CB *)cb)->pid;
        break;

      default:
        TRACE_DETAIL(("Unknown type %d", subscriber_type));
        TRACE_ASSERT(0!=0);
    }

    if(subscription != NULL)
    {
      if(hm_subscribe(subs_type, value, subscription->row_cb.void_cb, FALSE) != HM_OK)
      {
        TRACE_ERROR(("Error creating cross-binding subscription."));
        ret_val = HM_ERR;
        goto EXIT_LABEL;
      }
    }
    else
    {
      TRACE_DETAIL(("Subscribed entity does not exist yet. Will add when it exists."));
    }
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_subscribe */


/**
 *  @brief Inserts the subscription CB into the list, and triggers Notifications if appropriate.
 *
 *  @param *subs_cb Subscription CB (#HM_SUBSCRIPTION_CB) on which updation of subscription is to be done
 *  @param *node Subscriber Node
 *  @return #HM_OK if successful, #HM_ERR otherwise
 */
int32_t hm_subscription_insert(HM_SUBSCRIPTION_CB *subs_cb, HM_LIST_BLOCK *node)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t exists = FALSE;
  HM_LIST_BLOCK *looper = NULL;

  int32_t notify_type;
  int32_t notify = FALSE;

  HM_NOTIFICATION_CB *notify_cb = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(subs_cb != NULL);
  TRACE_ASSERT(node != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Loop through the subscribers to find if it is already subscribed       */
  /***************************************************************************/
  for(looper = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(subs_cb->subscribers_list);
      looper != NULL;
      looper = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(looper->node))
  {
    if(looper->target == node->target)
    {
      TRACE_WARN(("Duplicate Subscription!"));
      exists = TRUE;
      ret_val = HM_DUP; /* It is not wrong to re-subscribe. Just log it somewhere*/
      break;
    }
  }
  if(!exists)
  {
    HM_INSERT_BEFORE(subs_cb->subscribers_list, node->node);

    /***************************************************************************/
    /* Increment the subscribers count                                         */
    /***************************************************************************/
    subs_cb->num_subscribers++;
    TRACE_DETAIL(("Subscribers increments to %d", subs_cb->num_subscribers));
  }

  /***************************************************************************/
  /* If subscription is active, create a notification response to the subscr-*/
  /* -iber. Other subscribers already know it. This one might need update.   */
  /***************************************************************************/
  if(subs_cb->live)
  {
    switch(GET_TABLE_TYPE(subs_cb->row_cb.void_cb))
    {
    case HM_TABLE_TYPE_NODES:
      if(subs_cb->row_cb.node_cb->status == HM_NODE_FSM_STATE_ACTIVE)
      {
        notify_type = HM_NOTIFICATION_NODE_ACTIVE;
        notify = TRUE;
      }
      break;
    case HM_TABLE_TYPE_PROCESS:
      if(subs_cb->row_cb.process_cb->status == TRUE)
      {
        notify_type = HM_NOTIFICATION_PROCESS_CREATED;
        notify = TRUE;
      }
      break;

    default:
      TRACE_ERROR(("Unsupported Table Type: %d", GET_TABLE_TYPE(subs_cb->row_cb.void_cb)));
      TRACE_ASSERT(FALSE);

    }
    if(notify)
    {
      TRACE_DETAIL(("Send Notification to Subscriber."));

      /***************************************************************************/
      /* Allocate a Notification CB and Add that notification CB to Notify Queue */
      /***************************************************************************/
      notify_cb =  hm_alloc_notify_cb();
      if(notify_cb == NULL)
      {
        TRACE_ERROR(("Error creating Notification CB"));
        TRACE_ERROR(("Update could not be propagated."));
        ret_val = HM_ERR;
      }

      notify_cb->node_cb.void_cb = subs_cb->row_cb.void_cb;
      notify_cb->notification_type = notify_type;
      notify_cb->id = LOCAL.next_notification_id++;
      /***************************************************************************/
      /* Queue the notification CB                          */
      /***************************************************************************/
      HM_INSERT_BEFORE(LOCAL.notification_queue ,notify_cb->node);

      //FIXME: Move it to a separate thread later
      hm_service_notify_queue();
    }
    else
    {
      TRACE_DETAIL(("Not sending notifications"));
    }
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_subscription_insert */


/**
 *  @brief Compare function for Global Process Tree Exact insertion
 *
 *  @param *key1 First Key (of the node under question)
 *  @param *key2 Second Key (Usually from the table being traversed)
 *
 *  @return -1,0 or 1 depending on whether @p key1 is smaller/equal/greater than @p key2
 */
int32_t hm_compare_proc_tree_keys(void *key1, void *key2)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_GLOBAL_PROCESS_CB *proc1 = NULL, *proc2 = NULL;
  int32_t ret_val = -1;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(key1 != NULL);
  TRACE_ASSERT(key2 != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  proc1 = (HM_GLOBAL_PROCESS_CB *)key1;
  proc2 = (HM_GLOBAL_PROCESS_CB *)key2;

  TRACE_DETAIL(("[1:] 0x%x %d 0x%x",proc1->type, proc1->node_index, proc1->pid));
  TRACE_DETAIL(("[2:] 0x%x %d 0x%x",proc2->type, proc2->node_index, proc2->pid));

  if(proc1->type > proc2->type)
  {
    ret_val = 1;
  }
  else if(proc1->type == proc2->type)
  {
    if(proc1->node_index > proc2->node_index)
    {
      ret_val = 1;
    }
    else if(proc1->node_index == proc2->node_index)
    {
      if(proc1->pid > proc2->pid)
      {
        ret_val = 1;
      }
      else if(proc1->pid == proc2->pid)
      {
        ret_val = 0;
      }
    }
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_compare_proc_tree_keys */
