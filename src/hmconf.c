/**
 *  @file hmconf.c
 *  @brief Routines to parse and/or write to configuration files that drive the HM
 *
 *  @author Anshul
 *  @date 29-Jul-2015
 *  @bug None so far
 */
#include <hmincl.h>

/**
 *  @brief Gets the attribute type value
 *
 *  @param *value @c xmlChar type of array read from config file
 *  @return int32_t A macro binding of the type of attribute passed in as per supported vocabulary
 */
static int32_t hm_get_attr_type(xmlChar *value)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_ERR;
  uint32_t i;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  if(value == NULL)
  {
    TRACE_WARN(("No attribute passed."));
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  TRACE_DETAIL(("Look for %s", value));
  for(i=0; i< size_of_map; i++)
  {
    if(strncmp((const char *)value, attribute_map[i].attribute, strlen((const char *)value))==0)
    {
      ret_val = attribute_map[i].type;
      TRACE_DETAIL(("Found attribute type %d", attribute_map[i].type));
      break;
    }
  }
EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_get_attr_type */


/**
 *  @brief Gets the type of node from the vocabulary
 *
 *  @param *node @c xmlNode type of character array read from Config file.
 *  @return int32_t Macro binding of the type of XML node from the available vocabulary
 */
static int32_t hm_get_node_type(xmlNode *node)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_ERR;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT((node != NULL));

  TRACE_DETAIL(("%s", node->name));

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if(strstr((const char *)node->name, "hm_instance_info") != NULL)
  {
    ret_val = HM_CONFIG_HM_INSTANCE;
  }
  else if(strstr((const char *)node->name, "config") != NULL)
  {
    ret_val = HM_CONFIG_ROOT;
  }
  else if(strstr((const char *)node->name, "heartbeat") != NULL)
  {
    ret_val = HM_CONFIG_HEARTBEAT;
  }
  else if(strstr((const char *)node->name, "address") != NULL)
  {
    ret_val = HM_CONFIG_ADDRESS;
  }
  else if(strstr((const char *)node->name, "period") != NULL)
  {
    ret_val = HM_CONFIG_PERIOD;
  }
  else if(strstr((const char *)node->name, "threshold") != NULL)
  {
    ret_val = HM_CONFIG_THRESHOLD;
  }
  /* Order of occurance is important while using strstr */
  /* ip occurs in subscr'ip'tions. So, first check for longest word first */
  else if(strstr((const char *)node->name, "subscriptions") != NULL)
  {
    ret_val = HM_CONFIG_SUBSCRIPTION_TREE;
  }
  else if(strstr((const char *)node->name, "subscription") != NULL)
  {
    ret_val = HM_CONFIG_SUBSCRIPTION_INSTANCE;
  }
  else if(strstr((const char *)node->name, "ip") != NULL)
  {
    ret_val = HM_CONFIG_IP;
  }
  else if(strstr((const char *)node->name, "port") != NULL)
  {
    ret_val = HM_CONFIG_PORT;
  }
  else if(strstr((const char *)node->name, "group") != NULL)
  {
    ret_val = HM_CONFIG_GROUP;
  }
  else if(strstr((const char *)node->name, "nodes") != NULL)
  {
    ret_val = HM_CONFIG_NODE_TREE;
  }
  else if(strstr((const char *)node->name, "node") != NULL)
  {
    ret_val = HM_CONFIG_NODE_INSTANCE;
  }
  else if(strstr((const char *)node->name, "index") != NULL)
  {
    ret_val = HM_CONFIG_INDEX;
  }
  else if(strstr((const char *)node->name, "name") != NULL)
  {
    ret_val = HM_CONFIG_NAME;
  }
  else if(strstr((const char *)node->name, "role") != NULL)
  {
    ret_val = HM_CONFIG_ROLE;
  }
  else if(strstr((const char *)node->name, "ha") != NULL)
  {
    ret_val = HM_CONFIG_HA_SPECS;
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_get_node_type */


/**
 *  @brief Performs an in-depth traversal of the tree and writes config
 *
 *  @param *begin_node @c xmlNode type of structure returned by libxml library
 *      on parsing the xml config file.
 *   @param *stack  #HM_STACK type of value keeping track of the current configuration stack.
 *   @param *hm_config #HM_CONFIG_CB type of structure keeping the configuration parsed
 *       from Config file in system defined order.
 *
 *  @return H_OK if successful, HM_ERR otherwise.
 */
static int32_t hm_recurse_tree(xmlNode *begin_node, HM_STACK *stack, HM_CONFIG_CB *hm_config)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  xmlNode *current_node = NULL;

  HM_CONFIG_NODE *config_node = NULL;
  HM_CONFIG_NODE *parent_node = NULL;

  HM_HEARTBEAT_CONFIG *hb_config = NULL;
  HM_CONFIG_ADDRESS_CB *address_cb = NULL;
  HM_CONFIG_NODE_CB *node_config_cb = NULL;
  HM_CONFIG_SUBSCRIPTION_CB *subs_cb = NULL;

  SOCKADDR_IN *sock_addr = NULL;

  HM_STACK *reverse_stack = NULL;
  int32_t ip_scope, ip_type;
  char ip_version;

  int32_t node_type;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(begin_node != NULL);
  TRACE_ASSERT(stack != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  for(current_node = begin_node; current_node; current_node = current_node->next)
  {
    /***************************************************************************/
    /* Check if the node is a blank one. We get plenty of them.           */
    /***************************************************************************/
    if(xmlIsBlankNode(current_node))
    {
      continue;
    }

//    TRACE_INFO(("Node Type: %d", current_node->type));

    if(current_node->type == XML_ELEMENT_NODE)
    {
      node_type = hm_get_node_type(current_node);
      if(node_type == HM_ERR)
      {
        TRACE_ERROR(("Unknown type of node %d", node_type));
        /***************************************************************************/
        /* Not exiting. Ignoring errors                         */
        /***************************************************************************/
        continue;
      }
      //TRACE_DETAIL(("Node Type: %d", node_type));
      config_node = (HM_CONFIG_NODE *)malloc(sizeof(HM_CONFIG_NODE));
      if(config_node == NULL)
      {
        TRACE_ERROR(("Error allocating context node."));
        ret_val = HM_ERR;
        break;
      }

      config_node->type = node_type;
      config_node->self = (void *)current_node;
      TRACE_DETAIL(("Push to context stack if necessary"));
      switch (node_type)
      {
      case HM_CONFIG_ROOT:
      case HM_CONFIG_NODE_TREE:
        HM_STACK_PUSH(stack, config_node);
        break;

      case HM_CONFIG_NODE_INSTANCE:
        /***************************************************************************/
        /* Allocate a node CB now                           */
        /***************************************************************************/
        node_config_cb = (HM_CONFIG_NODE_CB *)malloc(sizeof(HM_CONFIG_NODE_CB));
        if(node_config_cb == NULL)
        {
          TRACE_ERROR(("Error allocating resources for Node information"));
          free(config_node);
          config_node = NULL;
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        node_config_cb->node_cb = hm_alloc_node_cb(TRUE);
        if(node_config_cb == NULL)
        {
          TRACE_ERROR(("Error allocating resources for Node information"));
          free(config_node);
          free(node_config_cb);
          config_node = NULL;
          node_config_cb = NULL;
          ret_val = HM_ERR;
          goto EXIT_LABEL;
        }
        config_node->opaque = (void *)node_config_cb;
        /***************************************************************************/
        /* Add node to list                               */
        /***************************************************************************/
        HM_INIT_LQE(node_config_cb->node, node_config_cb);
        HM_INSERT_BEFORE(hm_config->node_list, node_config_cb->node);

        HM_INIT_ROOT(node_config_cb->subscriptions);

        HM_STACK_PUSH(stack, config_node);
        break;

      case HM_CONFIG_SUBSCRIPTION_TREE:
        parent_node = HM_STACK_POP(stack);
        TRACE_DETAIL(("Parent Type: %d", parent_node->type));
        if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is Node Instance"));
          /***************************************************************************/
          /* Push parent node back on stack                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Push current node to stack. We're now expecting its value node       */
          /* So, set the pointer to its config_cb in LOCAL in the opaque data       */
          /***************************************************************************/
          //TODO
          ret_val = HM_STACK_PUSH(stack, config_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Config node must point to the node of parent node config CB         */
          /***************************************************************************/
          config_node->opaque = parent_node->opaque;
        }
        break;

      case HM_CONFIG_HM_INSTANCE:
        /***************************************************************************/
        /* Set opaque pointer to the config_cb                     */
        /***************************************************************************/
        TRACE_ASSERT(hm_config != NULL);
        config_node->opaque = (void *)hm_config;
        HM_STACK_PUSH(stack, config_node);
        break;

      case HM_CONFIG_ADDRESS:
        /***************************************************************************/
        /* Allocate an address structure                       */
        /***************************************************************************/
        address_cb = (HM_CONFIG_ADDRESS_CB *)malloc(sizeof(HM_CONFIG_ADDRESS_CB));
        if(address_cb == NULL)
        {
          TRACE_ERROR(("Error allocating address structures."));
          free(config_node);
          config_node = NULL;
          goto EXIT_LABEL;
        }
        /***************************************************************************/
        /* Fetch the scope of address first so that we may know the variable to use*/
        /***************************************************************************/
        TRACE_DETAIL(("Address type: %s",xmlGetProp((xmlNode *)config_node->self, (const xmlChar *)"type")));
        if((ip_scope = hm_get_attr_type(xmlGetProp((xmlNode *)config_node->self, (const xmlChar *)"type")))== HM_ERR)
        {
          TRACE_WARN(("Error finding attribute type value. Ignoring!"));
        }
#ifdef I_WANT_TO_DEBUG
        else
        {
          switch (ip_scope)
          {
          case HM_CONFIG_ATTR_ADDR_TYPE_LOCAL:
            TRACE_INFO(("Address scope is local"));
            break;
          case HM_CONFIG_ATTR_ADDR_TYPE_CLUSTER:
            TRACE_INFO(("Address scope is cluster"));
            break;
          default:
            TRACE_ERROR(("Unknown type %d", ret_val));
          }
        }
#endif
        address_cb->scope = ip_scope;

        TRACE_DETAIL(("Comm Scope: %s",xmlGetProp((xmlNode *)config_node->self, (const xmlChar *)"scope")));
        if((ip_scope = hm_get_attr_type(xmlGetProp((xmlNode *)config_node->self, (const xmlChar *)"scope")))== HM_ERR)
        {
          TRACE_WARN(("Error finding attribute type value. Set default!"));
          ip_scope = HM_CONFIG_ATTR_SCOPE_NODE;
        }
#ifdef I_WANT_TO_DEBUG
        else
        {
          switch (ip_scope)
          {
          case HM_CONFIG_ATTR_SCOPE_NODE:
            TRACE_INFO(("Communicate on Nodal Level"));
            break;
          case HM_CONFIG_ATTR_SCOPE_CLUSTER:
            TRACE_INFO(("Communicate on Cluster Level"));
            break;
          default:
            TRACE_ERROR(("Unknown type %d", ret_val));
          }
        }
#endif
        address_cb->comm_scope = ip_scope;
        config_node->opaque = (void *)address_cb;
        HM_STACK_PUSH(stack, config_node);
        break;

      case HM_CONFIG_HEARTBEAT:
        TRACE_INFO(("Scope of Heartbeat: %s", xmlGetProp(current_node, (const xmlChar *)"scope")));
        if((ret_val = hm_get_attr_type(xmlGetProp(current_node, (const xmlChar *)"scope")))== HM_ERR)
        {
          TRACE_WARN(("Error finding attribute type value. Ignoring!"));
        }
        else
        {
          switch (ret_val)
          {
          case HM_CONFIG_ATTR_SCOPE_NODE:
            TRACE_INFO(("Heartbeat scope is local"));
            /***************************************************************************/
            /* Set pointer to appropriate structure                     */
            /***************************************************************************/
            config_node->opaque = &hm_config->instance_info.node;
            hm_config->instance_info.node.scope = HM_CONFIG_ATTR_SCOPE_NODE;

            break;
          case HM_CONFIG_ATTR_SCOPE_CLUSTER:
            TRACE_INFO(("Heartbeat scope is cluster"));
            config_node->opaque = &hm_config->instance_info.cluster;
            hm_config->instance_info.cluster.scope = HM_CONFIG_ATTR_SCOPE_CLUSTER;
            break;

          default:
            TRACE_ERROR(("Unknown type %d", ret_val));
          }
        }
        HM_STACK_PUSH(stack, config_node);
        break;

      case HM_CONFIG_INDEX:
        TRACE_DETAIL(("Index node, determine parent context first"));

        parent_node = HM_STACK_POP(stack);
        TRACE_DETAIL(("Parent Type: %d", parent_node->type));
        if (parent_node->type == HM_CONFIG_HM_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is Hardware Manager Configuration"));
          /***************************************************************************/
          /* Push parent node back on stack                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Push current node to stack. We're now expecting its value node       */
          /* So, set the pointer to its config_cb in LOCAL in the opaque data       */
          /***************************************************************************/
          config_node->opaque = parent_node->opaque;

          ret_val = HM_STACK_PUSH(stack, config_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          //TRACE_INFO(("Hardware Index: %s", current_node->content));
        }
        else if(parent_node->type == HM_CONFIG_NODE_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is of a Monitoring Node"));
          /***************************************************************************/
          /* Push parent node back on stack                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Push current node to stack. We're now expecting its value node       */
          /* So, set the pointer to its config_cb in LOCAL in the opaque data       */
          /***************************************************************************/
          config_node->opaque = parent_node->opaque;

          ret_val = HM_STACK_PUSH(stack, config_node);
          if (ret_val == HM_ERR)
          {
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
        }
        break;

      case HM_CONFIG_PERIOD:

        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT( parent_node->type == HM_CONFIG_HEARTBEAT||
                      parent_node->type == HM_CONFIG_HA_SPECS);
        if (parent_node->type == HM_CONFIG_HEARTBEAT)
        {
          TRACE_DETAIL(("Parent Instance is Hardware Manager Keepalive"));
          hb_config = (HM_HEARTBEAT_CONFIG *)parent_node->opaque;
          if((ret_val = hm_get_attr_type(xmlGetProp(current_node, (const xmlChar *)"resolution")))== HM_ERR)
          {
            TRACE_WARN(("Error finding attribute type value. Ignoring!"));
          }
          else
          {
            switch (ret_val)
            {
            case HM_CONFIG_ATTR_RES_MIL_SEC:
              TRACE_INFO(("Heartbeat resolution is in milliseconds"));
              hb_config->resolution = HM_CONFIG_ATTR_RES_MIL_SEC;
              break;
            case HM_CONFIG_ATTR_RES_SEC:
              TRACE_INFO(("Heartbeat resolution is in seconds"));
              hb_config->resolution = HM_CONFIG_ATTR_RES_MIL_SEC;
              break;
            default:
              TRACE_ERROR(("Unknown type %d", ret_val));
            }
          }
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Set pointer to parent's memory                       */
          /***************************************************************************/
          config_node->opaque = (void *)hb_config;
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          TRACE_INFO(("[Attribute] resolution:  %s",xmlGetProp(current_node, (const xmlChar *)"resolution")));
        }
        else if(parent_node->type == HM_CONFIG_HA_SPECS)
        {
          TRACE_DETAIL(("Hardware Manager Initial Delay"));
          hb_config = (HM_HEARTBEAT_CONFIG *)&hm_config->instance_info.ha_role;
          if((ret_val = hm_get_attr_type(xmlGetProp(current_node,
                                (const xmlChar *)"resolution")))== HM_ERR)
          {
            TRACE_WARN(("Error finding attribute type value. Ignoring!"));
          }
          else
          {
            switch (ret_val)
            {
            case HM_CONFIG_ATTR_RES_MIL_SEC:
              TRACE_INFO(("Heartbeat resolution is in milliseconds"));
              hb_config->resolution = HM_CONFIG_ATTR_RES_MIL_SEC;
              break;
            case HM_CONFIG_ATTR_RES_SEC:
              TRACE_INFO(("Heartbeat resolution is in seconds"));
              hb_config->resolution = HM_CONFIG_ATTR_RES_MIL_SEC;
              break;
            default:
              TRACE_ERROR(("Unknown type %d", ret_val));
            }
          }
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Set pointer to parent's memory                       */
          /***************************************************************************/
          config_node->opaque = (void *)hb_config;
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          TRACE_INFO(("[Attribute] resolution:  %s",xmlGetProp(current_node, (const xmlChar *)"resolution")));
        }

        break;

      case HM_CONFIG_THRESHOLD:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT(parent_node->type == HM_CONFIG_HEARTBEAT);
        if (parent_node->type == HM_CONFIG_HEARTBEAT)
        {
          TRACE_DETAIL(("Parent Instance is Hardware Manager Keepalive"));
          hb_config = (HM_HEARTBEAT_CONFIG *)parent_node->opaque;
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* Set pointer to parent's memory                       */
          /***************************************************************************/
          config_node->opaque = (void *)hb_config;

          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
        }
        break;

      case HM_CONFIG_IP:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT(parent_node->type == HM_CONFIG_ADDRESS);
        if (parent_node->type == HM_CONFIG_ADDRESS)
        {
          TRACE_DETAIL(("Parent Instance is an Address Node"));
          address_cb = (HM_CONFIG_ADDRESS_CB *)parent_node->opaque;
          TRACE_ASSERT(address_cb != NULL);
          ret_val = HM_STACK_PUSH(stack, parent_node);

          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }

          if((ip_version = hm_get_attr_type(xmlGetProp(current_node, (const xmlChar *)"version")))== HM_ERR)
          {
            TRACE_WARN(("Error finding attribute type value. Ignoring!"));
          }
#ifdef I_WANT_TO_DEBUG
          else
          {
            switch (ip_version)
            {
            case HM_CONFIG_ATTR_IP_VERSION_4:
              TRACE_INFO(("IP Version is 4"));

              break;
            case HM_CONFIG_ATTR_IP_VERSION_6:
              TRACE_INFO(("IP Version is 6"));
              break;
            default:
              TRACE_ERROR(("Unknown version %d", ip_version));
            }
          }
#endif

          if((ip_type = hm_get_attr_type(xmlGetProp(current_node, (const xmlChar *)"type")))== HM_ERR)
          {
            TRACE_WARN(("Error finding attribute type value. Ignoring!"));
          }
#ifdef I_WANT_TO_DEBUG
          else
          {
            switch (ip_type)
            {
            case HM_CONFIG_ATTR_IP_TYPE_TCP:
              TRACE_INFO(("IP Type TCP"));
              break;
            case HM_CONFIG_ATTR_IP_TYPE_UDP:
              TRACE_INFO(("IP Type UDP"));
              break;
            case HM_CONFIG_ATTR_IP_TYPE_MCAST:
              TRACE_INFO(("IP Type Multicast"));
              break;

            default:
              TRACE_ERROR(("Unknown type %d", ret_val));
            }
          }
#endif
          ip_scope = address_cb->scope;
          /***************************************************************************/
          /* 4 variables can have 16 combinations.We go down according to most frequ-*/
          /* -ently occuring ones                             */
          /***************************************************************************/
          if(  ip_version==HM_CONFIG_ATTR_IP_VERSION_4 &&
            ip_type==HM_CONFIG_ATTR_IP_TYPE_TCP &&
            ip_scope==HM_CONFIG_ATTR_ADDR_TYPE_LOCAL &&
            address_cb->comm_scope == HM_CONFIG_ATTR_SCOPE_NODE)
          {
            TRACE_DETAIL(("IPv4 TCP Address for Nodes"));
            hm_config->instance_info.node_addr = address_cb;
            address_cb->address.type = HM_TRANSPORT_TCP_LISTEN;
          }
          else if(  ip_version==HM_CONFIG_ATTR_IP_VERSION_4 &&
            ip_type==HM_CONFIG_ATTR_IP_TYPE_TCP &&
            ip_scope==HM_CONFIG_ATTR_ADDR_TYPE_LOCAL &&
            address_cb->comm_scope == HM_CONFIG_ATTR_SCOPE_CLUSTER)
          {
            TRACE_DETAIL(("IPv4 TCP Address for Clusters"));
            hm_config->instance_info.cluster_addr = address_cb;
            address_cb->address.type = HM_TRANSPORT_TCP_LISTEN;
          }
          else if(  ip_version==HM_CONFIG_ATTR_IP_VERSION_4 &&
                ip_type==HM_CONFIG_ATTR_IP_TYPE_UDP &&
                ip_scope==HM_CONFIG_ATTR_ADDR_TYPE_LOCAL &&
                address_cb->comm_scope == HM_CONFIG_ATTR_SCOPE_CLUSTER)
          {
            TRACE_DETAIL(("IPv4 UDP Address for Clusters"));
            hm_config->instance_info.cluster_addr = address_cb;
            address_cb->address.type = HM_TRANSPORT_UDP;
          }
          else if(  ip_version==HM_CONFIG_ATTR_IP_VERSION_4 &&
                ip_type==HM_CONFIG_ATTR_IP_TYPE_MCAST &&
                ip_scope==HM_CONFIG_ATTR_ADDR_TYPE_LOCAL &&
                address_cb->comm_scope == HM_CONFIG_ATTR_SCOPE_CLUSTER)
          {
            TRACE_DETAIL(("IPv4 UDP Address for Multicast on cluster"));
            hm_config->instance_info.mcast = address_cb;
            address_cb->address.type = HM_TRANSPORT_MCAST;
          }
          else if(  ip_version==HM_CONFIG_ATTR_IP_VERSION_4 &&
                ip_type==HM_CONFIG_ATTR_IP_TYPE_TCP &&
                ip_scope==HM_CONFIG_ATTR_ADDR_TYPE_CLUSTER)

          {
            TRACE_DETAIL(("Remote Node information."));
            HM_INIT_LQE(address_cb->node, address_cb);
            address_cb->address.type = HM_TRANSPORT_TCP_OUT;
            address_cb->scope = HM_CONFIG_ATTR_ADDR_TYPE_CLUSTER;
            /***************************************************************************/
            /* Insert into List                               */
            /***************************************************************************/
            HM_INSERT_BEFORE(hm_config->instance_info.addresses, address_cb->node);
          }
          else
          {
            TRACE_DETAIL(("Unknown: IP Type: %d; IP Version: %c; IP Scope: %d; Comm Scope: %d",
                          ip_type, ip_version, ip_scope, address_cb->comm_scope));
            address_cb = NULL;
          }
          parent_node->opaque = (void *)address_cb;
          config_node->opaque = (void *)address_cb;
        }

        break;

      case HM_CONFIG_PORT:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT(parent_node->type == HM_CONFIG_ADDRESS);
        if (parent_node->type == HM_CONFIG_ADDRESS)
        {
          TRACE_DETAIL(("Parent Instance is an Address Node"));
          address_cb = (HM_CONFIG_ADDRESS_CB *)parent_node->opaque;
          TRACE_ASSERT(address_cb != NULL);

          ret_val = HM_STACK_PUSH(stack, parent_node);

          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          config_node->opaque = (void *)address_cb;
        }
        break;

      case HM_CONFIG_GROUP:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT((parent_node->type == HM_CONFIG_ADDRESS)||
                (parent_node->type == HM_CONFIG_NODE_INSTANCE));
        if (parent_node->type == HM_CONFIG_ADDRESS)
        {
          TRACE_DETAIL(("Parent Instance is an Address Node"));
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We'll be writing into the mcast_group next. Hopefully!           */
          /***************************************************************************/
          config_node->opaque = (void *)&hm_config->instance_info;
        }
        else if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is a Node Instance"));
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          config_node->opaque = parent_node->opaque;
        }
        break;

      case HM_CONFIG_NAME:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT((parent_node->type == HM_CONFIG_NODE_INSTANCE));
        if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is a Node Instance"));
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          config_node->opaque = parent_node->opaque;
        }
        break;

      case HM_CONFIG_ROLE:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT((parent_node->type == HM_CONFIG_NODE_INSTANCE));
        if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
        {
          TRACE_DETAIL(("Parent Instance is a Node Instance"));
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          config_node->opaque = parent_node->opaque;
        }
        break;

      case HM_CONFIG_SUBSCRIPTION_INSTANCE:
        parent_node = HM_STACK_POP(stack);
        TRACE_ASSERT((parent_node->type == HM_CONFIG_SUBSCRIPTION_TREE));
        if (parent_node->type == HM_CONFIG_SUBSCRIPTION_TREE)
        {
          TRACE_DETAIL(("Parent Instance is a Subscription Tree"));
          ret_val = HM_STACK_PUSH(stack, parent_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing parent node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          /***************************************************************************/
          /* We expect its value to come next                       */
          /***************************************************************************/
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val == HM_ERR)
          {
            TRACE_ERROR(("Error pushing config node back on stack"));
            free(parent_node);
            free(config_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          node_config_cb = (HM_CONFIG_NODE_CB *)parent_node->opaque;
          subs_cb = (HM_CONFIG_SUBSCRIPTION_CB *)malloc(sizeof(HM_CONFIG_SUBSCRIPTION_CB));
          if(subs_cb == NULL)
          {
            TRACE_ERROR(("Error allocation resources for subscriptions"));
            free(config_node);
            free(parent_node);
            parent_node = NULL;
            config_node = NULL;
            goto EXIT_LABEL;
          }
          HM_INIT_LQE(subs_cb->node, subs_cb);

          if((ret_val = hm_get_attr_type(xmlGetProp(current_node, (const xmlChar *)"type")))== HM_ERR)
          {
            TRACE_WARN(("Error finding attribute type value. Ignoring!"));
          }
          else
          {
            switch (ret_val)
            {
            case HM_CONFIG_ATTR_SUBS_TYPE_GROUP:
              TRACE_INFO(("Subscription type is a group"));
              subs_cb->subs_type = HM_CONFIG_ATTR_SUBS_TYPE_GROUP;
              break;
            case HM_CONFIG_ATTR_SUBS_TYPE_PROC:
              TRACE_INFO(("Subscription type is a process"));
              subs_cb->subs_type = HM_CONFIG_ATTR_SUBS_TYPE_PROC;
              break;
            case HM_CONFIG_ATTR_SUBS_TYPE_IF:
              TRACE_INFO(("Subscription type is an interface"));
              subs_cb->subs_type = HM_CONFIG_ATTR_SUBS_TYPE_IF;
              break;

            default:
              TRACE_ERROR(("Unknown type %d", ret_val));
            }
          }
          /***************************************************************************/
          /* Error or not, insert it into list                     */
          /***************************************************************************/
          HM_INSERT_BEFORE(node_config_cb->subscriptions, subs_cb->node);
          config_node->opaque = (void *)subs_cb;
        }
        else
        {
          TRACE_WARN(("Subscription instance occured without a subscription tree. Ignoring the rest."));
          continue;
        }
        break;

      case HM_CONFIG_HA_SPECS:
        TRACE_DETAIL(("HA Specifications."));

        ret_val = HM_STACK_PUSH(stack, config_node);
        if(ret_val == HM_ERR)
        {
          TRACE_ERROR(("Error pushing config node back on stack"));
          free(config_node);
          config_node = NULL;
          goto EXIT_LABEL;
        }
        config_node->opaque = &hm_config->instance_info.ha_role;
        break;

      default:
        /***************************************************************************/
        /* UNHITTABLE                                 */
        /***************************************************************************/
        TRACE_DETAIL(("UNKNOWN NODE TYPE"));
        TRACE_ASSERT(0==1);

        break;
      }
    }
    else if(current_node->type == XML_TEXT_NODE)
    {
      /***************************************************************************/
      /* The value of an element is contained in the subsequent value node     */
      /* by examining the stack, we can know if we are expecting one         */
      /***************************************************************************/
      config_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
      if(config_node != NULL)
      {
        switch (config_node->type)
        {
        case HM_CONFIG_ROOT:
        case HM_CONFIG_HM_INSTANCE:
        case HM_CONFIG_HEARTBEAT:
        case HM_CONFIG_ADDRESS:
        case HM_CONFIG_NODE_TREE:
        case HM_CONFIG_NODE_INSTANCE:
        case HM_CONFIG_SUBSCRIPTION_TREE:
          TRACE_DETAIL(("Value parsing not needed"));
          ret_val = HM_STACK_PUSH(stack, config_node);
          if(ret_val != HM_OK)
          {
            TRACE_ERROR(("Error pushing parent node back on to stack"));
            ret_val = HM_ERR;
            free(config_node);
            config_node = NULL;
            goto EXIT_LABEL;
          }
          break;
        case HM_CONFIG_INDEX:
          /***************************************************************************/
          /* Determine the context of the index. That would be one step back       */
          /* But it could be many steps back too, so we implement a reverse stack     */
          /* instead.                                   */
          /***************************************************************************/
          reverse_stack = HM_STACK_INIT(10);
          if(reverse_stack == NULL)
          {
            TRACE_ERROR(("Error initializing soft-stack."));
            ret_val = HM_ERR;
          }
          HM_STACK_PUSH(reverse_stack, config_node);
          parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
          if (parent_node->type == HM_CONFIG_HM_INSTANCE)
          {
            TRACE_DETAIL(("Parent Instance is Hardware Manager Configuration"));
            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            hm_config->instance_info.index = atoi((const char *)current_node->content);
            TRACE_INFO(("Hardware Index: %d", hm_config->instance_info.index));
          }
          else if(parent_node->type == HM_CONFIG_NODE_INSTANCE)
          {
            TRACE_DETAIL(("Parent Instance is of a Monitoring Node"));

            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            node_config_cb = (HM_CONFIG_NODE_CB *)config_node->opaque;
            node_config_cb->node_cb->index = atoi((const char *)current_node->content);
            TRACE_INFO(("Node Index: %d", node_config_cb->node_cb->index));
          }
          /***************************************************************************/
          /* Free the reverse stack                           */
          /***************************************************************************/
          TRACE_DETAIL(("Free the reverse stack"));
          for(config_node = (HM_CONFIG_NODE *)HM_STACK_POP(reverse_stack);
            config_node != NULL;
            config_node = (HM_CONFIG_NODE *)HM_STACK_POP(reverse_stack))
          {
            free(config_node);
          }
          HM_STACK_DESTROY(reverse_stack);
          break;

        case HM_CONFIG_PERIOD:
          parent_node = HM_STACK_POP(stack);
          TRACE_ASSERT((parent_node->type == HM_CONFIG_HEARTBEAT)||
                              (parent_node->type == HM_CONFIG_HA_SPECS));
          if (parent_node->type == HM_CONFIG_HEARTBEAT)
          {
            TRACE_DETAIL(("Parent Instance is Hardware Manager Keepalive"));
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if(ret_val == HM_ERR)
            {
              TRACE_ERROR(("Error pushing parent node back on stack"));
              free(parent_node);
              free(config_node);
              parent_node = NULL;
              config_node = NULL;
              goto EXIT_LABEL;
            }

            hb_config = (HM_HEARTBEAT_CONFIG *)config_node->opaque;
            hb_config->timer_val = atoi((const char *)current_node->content);
            TRACE_INFO(("Heartbeat period: %d", hb_config->timer_val));
          }
          else if(parent_node->type == HM_CONFIG_HA_SPECS)
          {
            TRACE_DETAIL(("Parent instance is HA Specification."));
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if(ret_val == HM_ERR)
            {
              TRACE_ERROR(("Error pushing parent node back on stack"));
              free(parent_node);
              free(config_node);
              parent_node = NULL;
              config_node = NULL;
              goto EXIT_LABEL;
            }

            hb_config = (HM_HEARTBEAT_CONFIG *)config_node->opaque;
            hb_config->timer_val = atoi((const char *)current_node->content);
            TRACE_INFO(("Heartbeat period: %d", hb_config->timer_val));
          }
          free(config_node);
          break;

        case HM_CONFIG_THRESHOLD:
          parent_node = HM_STACK_POP(stack);
          TRACE_ASSERT(parent_node->type == HM_CONFIG_HEARTBEAT);
          if (parent_node->type == HM_CONFIG_HEARTBEAT)
          {
            TRACE_DETAIL(("Parent Instance is Hardware Manager Keepalive"));

            ret_val = HM_STACK_PUSH(stack, parent_node);
            if(ret_val == HM_ERR)
            {
              TRACE_ERROR(("Error pushing parent node back on stack"));
              free(parent_node);
              free(config_node);
              parent_node = NULL;
              config_node = NULL;
              goto EXIT_LABEL;
            }
            hb_config = (HM_HEARTBEAT_CONFIG *)config_node->opaque;
            hb_config->threshold = atoi((const char *)current_node->content);
            TRACE_INFO(("Heartbeat threshold: %d", hb_config->threshold));
          }
          free(config_node);
          break;

        case HM_CONFIG_IP:
          parent_node = HM_STACK_POP(stack);
          TRACE_ASSERT(parent_node->type == HM_CONFIG_ADDRESS);
          if (parent_node->type == HM_CONFIG_ADDRESS)
          {
            TRACE_DETAIL(("Parent Instance is an Address Node"));
            address_cb = (HM_CONFIG_ADDRESS_CB *)parent_node->opaque;
            TRACE_ASSERT(address_cb != NULL);
            ret_val = HM_STACK_PUSH(stack, parent_node);

            if(ret_val == HM_ERR)
            {
              TRACE_ERROR(("Error pushing parent node back on stack"));
              free(parent_node);
              free(config_node);
              parent_node = NULL;
              config_node = NULL;
              goto EXIT_LABEL;
            }
          }
          sock_addr = (SOCKADDR_IN *)&address_cb->address.address;
          inet_pton(AF_INET, (const char *)current_node->content, &sock_addr->sin_addr);
#ifdef I_WANT_TO_DEBUG
          {
            char tmp[100];
            TRACE_INFO(("IP: %s", inet_ntop(AF_INET,
                &sock_addr->sin_addr, tmp, sizeof(tmp))));
          }
#endif
          free(config_node);
          break;

        case HM_CONFIG_PORT:
          parent_node = HM_STACK_POP(stack);
          TRACE_ASSERT(parent_node->type == HM_CONFIG_ADDRESS);
          if (parent_node->type == HM_CONFIG_ADDRESS)
          {
            TRACE_DETAIL(("Parent Instance is an Address Node"));
            address_cb = (HM_CONFIG_ADDRESS_CB *)config_node->opaque;
            TRACE_ASSERT(address_cb != NULL);

            ret_val = HM_STACK_PUSH(stack, parent_node);

            if(ret_val == HM_ERR)
            {
              TRACE_ERROR(("Error pushing parent node back on stack"));
              free(parent_node);
              free(config_node);
              parent_node = NULL;
              config_node = NULL;
              goto EXIT_LABEL;
            }

            sock_addr = (SOCKADDR_IN *)&address_cb->address.address;
            sock_addr->sin_port = htons(atoi((const char *)current_node->content));
            address_cb->port = sock_addr->sin_port;
            TRACE_INFO(("Port: %d", ntohs(sock_addr->sin_port)));
          }
          free(config_node);
          break;

        case HM_CONFIG_GROUP:
          parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
          if (parent_node->type == HM_CONFIG_ADDRESS)
          {
            TRACE_DETAIL(("Parent Instance is Address Configuration"));
            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            TRACE_INFO(("Multicast Group: %s", current_node->content));
          }
          else if(parent_node->type == HM_CONFIG_NODE_INSTANCE)
          {
            TRACE_DETAIL(("Parent Instance is of a Node Instance"));

            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            node_config_cb = (HM_CONFIG_NODE_CB *)config_node->opaque;
            node_config_cb->node_cb->group = atoi((const char *)current_node->content);
            TRACE_INFO(("Node Group: %d", node_config_cb->node_cb->group));
          }
          free(config_node);
          break;

        case HM_CONFIG_NAME:
          parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
          if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
          {
            TRACE_DETAIL(("Parent Instance is a Node Instance"));
            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            node_config_cb = (HM_CONFIG_NODE_CB *)config_node->opaque;
            TRACE_ASSERT(node_config_cb != NULL);

            snprintf((char *)node_config_cb->node_cb->name,
                sizeof(node_config_cb->node_cb->name),
                "%s",(const char *)current_node->content);
            TRACE_INFO(("Node Name: %s", node_config_cb->node_cb->name));
          }
          free(config_node);
          break;

        case HM_CONFIG_ROLE:
          parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
          if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
          {
            TRACE_DETAIL(("Parent Instance is a Node Instance"));
            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            node_config_cb = (HM_CONFIG_NODE_CB *)config_node->opaque;
            TRACE_ASSERT(node_config_cb != NULL);

            if(strstr((const char *)current_node->content, "active")== NULL)
            {
              node_config_cb->node_cb->role = NODE_ROLE_PASSIVE;
            }
            else
            {
              node_config_cb->node_cb->role = NODE_ROLE_ACTIVE;
            }
            TRACE_INFO(("Node Role: %d", node_config_cb->node_cb->role));
          }
          free(config_node);
          break;

        case HM_CONFIG_SUBSCRIPTION_INSTANCE:
          parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
          if (parent_node->type == HM_CONFIG_SUBSCRIPTION_TREE)
          {
            TRACE_DETAIL(("Parent Instance is a Subscription Tree"));
            /***************************************************************************/
            /* Push it back                                 */
            /***************************************************************************/
            ret_val = HM_STACK_PUSH(stack, parent_node);
            if (ret_val == HM_ERR)
            {
              ret_val = HM_ERR;
              goto EXIT_LABEL;
            }
            subs_cb = (HM_CONFIG_SUBSCRIPTION_CB *)config_node->opaque;
            subs_cb->value = atoi((const char *)current_node->content);

            TRACE_INFO(("Subscription group: %d", subs_cb->value));
          }
          free(config_node);
          break;
        default:
          break;
        }
      }
      else
      {
        /***************************************************************************/
        /* This isn't necessarily an error.                       */
        /***************************************************************************/
        TRACE_WARN(("Error popping stack"));
      }
    }
    if(current_node->children != NULL)
    {
      ret_val = hm_recurse_tree(current_node->children, stack, hm_config);
      if(ret_val != HM_OK)
      {
        TRACE_ERROR(("Something went wrong."));
      }
    }
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
}/* hm_recurse_tree */


/**
 *  @brief Parses the file into the configuration structure
 *
 *  Currently supported vocabulary:
 *  XML Node Names:
 *    config      : Root of config
 *    hm_instance_info: Config for HM Binary
 *    index      : Index value
 *    heartbeat    : Heartbeat Configuration
 *    period      : Timer Value
 *    threshold    : Maximum number of timeouts
 *    address      : Inet Address structure
 *    ip        : IP Address
 *    port      : Port Value
 *    group      : Multicast Group
 *    nodes      : Container of Nbase Nodes
 *    node      : Single Node in cluster
 *    name      : String Name
 *    role      : Active/Passive
 *    Group
 *
 *  @param *config_cb #HM_CONFIG_CB type of configuration Control Block
 *  @return HM_OK if successful, HM_ERR otherwise
 */
int32_t hm_parse_config(HM_CONFIG_CB *config_cb, char *config_file)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  xmlDoc *doc = NULL;  /* XML File representation */
  xmlNode *root = NULL;

  HM_STACK *config_stack = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(config_cb != NULL);
  TRACE_ASSERT(config_file != NULL);

  TRACE_DETAIL(("Config File name: %s", config_file));
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  /***************************************************************************/
  /* Allocate a configuration stack to keep track of where we are in parsing */
  /***************************************************************************/
  config_stack = HM_STACK_INIT(MAX_STACK_SIZE);
  if(config_stack == NULL)
  {
    TRACE_ERROR(("Error initializing stack"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Read XML File into C                             */
  /***************************************************************************/
  doc = xmlReadFile(config_file, NULL, 0);
  if(doc == NULL)
  {
    TRACE_ERROR(("Error parsing configuration file %s", config_file));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }
  root = xmlDocGetRootElement(doc); /* Must be <config> */
  /***************************************************************************/
  /* Parse HM Instance Specific Configuration                   */
  /***************************************************************************/

  /***************************************************************************/
  /* Start parsing the XML Tree now and build configuration           */
  /***************************************************************************/
  ret_val = hm_recurse_tree(root, config_stack, config_cb);
  if(ret_val != HM_OK)
  {
    TRACE_ERROR(("Error occured while parsing configuration"));
  }

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  if(doc != NULL)
  {
    xmlFreeDoc(doc);
    doc = NULL;
  }
  xmlCleanupParser();
  TRACE_EXIT();
  return ret_val;
}/* hm_parse_config */


/**
 *  @brief Rewrite parts of configuration file that are modifiable
 *
 *  @detail Currently, only the HA roles of the nodes may be re-written.
 * If the config file did not exist before, or has been moved meanwhile,
 * new file will be created instead (minus the meta)
 *
 *  @param char * file_name
 *  @return #HM_OK on success, #HM_ERR on error
 */
int32_t hm_write_config_file(char *file_name)
{
  /* Local variables */
  int32_t ret_val = HM_OK;
  xmlDoc *doc = NULL;  /* XML File representation */
  xmlXPathContext *ctxt = NULL;
  xmlXPathObject *obj = NULL;

  HM_CONFIG_NODE_CB *node = NULL;
  xmlChar value[25];

  char xpath_expr[255];

  /* Sanity checks */
  TRACE_ENTRY();
  TRACE_ASSERT(file_name != NULL);

  /* Main routine */
  /***************************************************************************/
  /* Initialize xml parser                                                   */
  /***************************************************************************/
  xmlInitParser();

  /***************************************************************************/
  /*Check if the file name exists. If it exists, read the file and overwrite */
  /*fields. If it doesn't, create a brand new file by that name from the     */
  /* configuration.                                                          */
  /***************************************************************************/
  if(access(file_name, F_OK) != -1)
  {
    TRACE_INFO(("Writing to %s", file_name));
    /***************************************************************************/
    /* Read XML File into C                                                    */
    /***************************************************************************/
    doc = xmlReadFile(file_name, NULL, 0);
    if(doc == NULL)
    {
      TRACE_ERROR(("Error parsing configuration file %s", file_name));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    /***************************************************************************/
    /* Create an XPath Context                                                 */
    /***************************************************************************/
    ctxt = xmlXPathNewContext(doc);
    if(ctxt == NULL)
    {
      TRACE_ERROR(("Error creating XPath context."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    /* Currently, only active/passive updates are allowed. So, doing here only */
    snprintf(xpath_expr, sizeof(xpath_expr), "/config/nodes/node[group=1]/role");
    obj = xmlXPathEvalExpression((const xmlChar *)xpath_expr, ctxt);
    if(obj == NULL)
    {
      TRACE_WARN(("Error evaluating XPath expression. Nothing will be written"));
      /* it is not a terminal error. At best, nothing will be written here. */
      /* it is possible that the system manager does not run here at all.   */
      ret_val = HM_OK;
      goto EXIT_LABEL;
    }

    /***************************************************************************/
    /* Find the role of Group 1 node                                           */
    /***************************************************************************/
    for(node = (HM_CONFIG_NODE_CB *)HM_NEXT_IN_LIST(LOCAL.config_data->node_list);
        node != NULL;
        node = (HM_CONFIG_NODE_CB *)HM_NEXT_IN_LIST(node->node))
    {
      if(node->node_cb->group == 1)
      {
        TRACE_DETAIL(("Found Node. Write role for value %d", node->node_cb->role));
        snprintf((char *)value, sizeof(value), "%s",
            (node->node_cb->role== NODE_ROLE_ACTIVE? "active":"passive"));
        break;
      }
    }
    /***************************************************************************/
    /* Write value on the node                                                 */
    /***************************************************************************/
    if(hm_config_write_node(obj->nodesetval, value) != HM_OK)
    {
      TRACE_ERROR(("Error updating config file."));
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
  }
  else
  {
    TRACE_WARN(("File %s does not exist. Re-creating configuration", file_name));

  }
  /* Write to the file on disk */
  xmlSaveFileEnc(HM_DEFAULT_CONFIG_FILE_NAME, doc, "UTF-8");

  /* Exit level checks */
EXIT_LABEL:
  if(obj != NULL)
  {
    xmlXPathFreeObject(obj);
    obj = NULL;
  }
  if(ctxt != NULL)
  {
    xmlXPathFreeContext(ctxt);
    ctxt = NULL;
  }
  if(doc != NULL)
  {
    xmlFreeDoc(doc);
    doc = NULL;
  }
  xmlCleanupParser();
  TRACE_EXIT();
  return ret_val;
} /* hm_write_config_file */

/**
 *  @brief Write value to xml node
 *
 *  @param node_set Nodes on which write operation must happen
 *  @return #HM_OK on success, #HM_ERR on error
 */
int32_t hm_config_write_node(xmlNodeSet *nodes, const xmlChar* value)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  int32_t size, i;

  /***************************************************************************/
  /* Sanity Checks                                                           */
  /***************************************************************************/
  TRACE_ENTRY();
  /***************************************************************************/
  /* Main Routine                                                            */
  /***************************************************************************/
  size = (nodes) ? nodes->nodeNr:0;
  for(i= size -1; i>=0;i--)
  {
    TRACE_ASSERT(nodes->nodeTab != NULL);
    xmlNodeSetContent(nodes->nodeTab[i], value);

    if (nodes->nodeTab[i]->type != XML_NAMESPACE_DECL)
    {
      nodes->nodeTab[i] = NULL;
    }
   }
  /***************************************************************************/
  /* Exit Level Checks                                                       */
  /***************************************************************************/
  TRACE_EXIT();
  return ret_val;
} /* hm_config_write_node */
