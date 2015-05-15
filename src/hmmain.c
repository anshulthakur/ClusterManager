/*
 * hmmain.c
 *
 *  Created on: 07-May-2015
 *      Author: anshul
 */


#include <hmincl.h>

HM_GLOBAL_DATA global;

/***************************************************************************/
/* Name:	main 														   */
/* Parameters: Input - 	Stdargs											   */
/*			   Input/Output -											   */
/* Return:	int															   */
/* Purpose: Main routine												   */
/***************************************************************************/
int32_t main(int32_t argc, char **argv)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	extern char *optarg;
	extern int32_t optind;

	int32_t cmd_opt;
	int32_t ret_val = HM_OK;

	char *config_file = "config.xml";
	HM_CONFIG_CB *config_cb = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize Logging													   */
	/***************************************************************************/

	/***************************************************************************/
	/* Allocate Configuration Control Block									   */
	/* This will load default options everywhere							   */
	/***************************************************************************/
	config_cb = hm_alloc_config_cb();
	TRACE_ASSERT(config_cb != NULL);
	if(config_cb == NULL)
	{
		TRACE_ERROR(("Error allocating Configuration Control Block. System will quit."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Parse command line arguments to find the name of configuration file 	   */
	/***************************************************************************/
	while ((cmd_opt=getopt(argc, argv, "c:h")) !=-1)
	{
		switch(cmd_opt)
		{
		case 'c':
			config_file = optarg;
			break;

		case 'h':
		default:
			printf("\nUsage:\n <cmd> [-c <config file name>]\n");
			break;
		}
	}
	/***************************************************************************/
	/* If file is present, read the XML Tree into the configuration	CB		   */
	/* NOTE: access() is POSIX standard, but it CAN cause race conditions.	   */
	/* replace if necessary.												   */
	/***************************************************************************/
	if(access(config_file, F_OK) != -1)
	{
		TRACE_INFO(("Using %s for Configuration", config_file));
		ret_val = hm_parse_config(config_cb, config_file);
	}
	else
	{
		TRACE_WARN(("File %s does not exist. Use default configuration", config_file));
		/***************************************************************************/
		/* We need not parse anything now, since we already loaded up defaults 	   */
		/***************************************************************************/
	}
	/***************************************************************************/
	/* By now, we will be aware of all things we need to start the HM		   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize HM Local structure										   */
	/***************************************************************************/
	ret_val = hm_init_local(config_cb);
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Local Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Initialize Transport Layer											   */
	/***************************************************************************/
	ret_val = hm_init_transport();
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Transport Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Start Hardware Location Layer										   */
	/***************************************************************************/
	ret_val = hm_init_location_layer();
	if(ret_val != HM_OK)
	{
		TRACE_ERROR(("Hardware Manager Location Layer Initialization Failed."));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}


EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* main */


/***************************************************************************/
/* Name:	hm_init_local 									*/
/* Parameters: Input - 	config_cb: Configuration structure read									*/
/*																		   */
/* Return:	int32_t														   */
/* Purpose: Initializes local data strcuture which houses all Global Data  */
/***************************************************************************/
int32_t hm_init_local(HM_CONFIG_CB *config_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize Trees														   */
	/***************************************************************************/
	/* Aggregate Nodes Tree	*/
	/* Aggregate Process Tree */
	/* Aggregate PID Tree	*/
	/* Aggregate Interfaces Tree */
	/* Active Joins Tree */
	/* Broken/Pending Joins Tree */
	/* Notifications Queue */

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_local */

/***************************************************************************/
/* Name:	hm_init_transport 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Initializes Transport Layer									   */
/* 1. Set SIGIGN for dead sockets										   */
/***************************************************************************/
int32_t hm_init_transport()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_transport */

/***************************************************************************/
/* Name:	hm_init_location_layer 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Initialize Hardware Location Layer			*/
/***************************************************************************/
int32_t hm_init_location_layer()
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/

	/***************************************************************************/
	/* Initialize Local Location CB											   */
	/***************************************************************************/
	/* Initialize FD_SET */

	/***************************************************************************/
	/* Initialize Transport CB for Listen									   */
	/***************************************************************************/
	/*
    * Initialize Socket Connection CB:
        * Set Connection Type
        * Open Socket
        * Set Transport Location CB Pointer
        * Set Node CB to NULL
        * Add Sock to FD_SET
    * Start listen on TCP port
	*/

	/***************************************************************************/
	/* Initialize Transport CB for Unicast									   */
	/***************************************************************************/
	/*
	 * Initialize TCB for Unicast Port
        * Initialize Socket Connection CB:
            * Open UDP Unicast port
    */

	/***************************************************************************/
	/* Initialize Cluster Specific Function									   */
	/***************************************************************************/
	/*
    * If Cluster enabled:
        * Initialize TCB for Multicast Port
            * Open UDP Multicast port
            * Join Multicast Group
     */

	//hm_init_location_cb(HM_LOCATION_CB &LOCAL.local_location_cb)

	/***************************************************************************/
	/* For each node in Configuration, initialize Node CB and Fill it up	   */
	/***************************************************************************/

	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_init_location_layer */

/***************************************************************************/
/* Name:	hm_get_node_type 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	static int32_t												   */
/* Purpose: Gets the type of node from the vocabulary					   */
/***************************************************************************/
static int32_t hm_get_node_type(xmlNode *node)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_ERR;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT((node != NULL));

	TRACE_DETAIL(("%s", node->name));

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(strstr(node->name, "hm_instance_info") != NULL)
	{
		ret_val = HM_CONFIG_HM_INSTANCE;
	}
	else if(strstr(node->name, "config") != NULL)
	{
		ret_val = HM_CONFIG_ROOT;
	}
	else if(strstr(node->name, "heartbeat") != NULL)
	{
		ret_val = HM_CONFIG_HEARTBEAT;
	}
	else if(strstr(node->name, "address") != NULL)
	{
		ret_val = HM_CONFIG_ADDRESS;
	}
	else if(strstr(node->name, "period") != NULL)
	{
		ret_val = HM_CONFIG_PERIOD;
	}
	else if(strstr(node->name, "threshold") != NULL)
	{
		ret_val = HM_CONFIG_THRESHOLD;
	}
	/* Order of occurance is important while using strstr */
	/* ip occurs in subscr'ip'tions. So, first check for longest word first */
	else if(strstr(node->name, "subscriptions") != NULL)
	{
		ret_val = HM_CONFIG_SUBSCRIPTION_TREE;
	}
	else if(strstr(node->name, "subscription") != NULL)
	{
		ret_val = HM_CONFIG_SUBSCRIPTION_INSTANCE;
	}
	else if(strstr(node->name, "ip") != NULL)
	{
		ret_val = HM_CONFIG_IP;
	}
	else if(strstr(node->name, "port") != NULL)
	{
		ret_val = HM_CONFIG_PORT;
	}
	else if(strstr(node->name, "group") != NULL)
	{
		ret_val = HM_CONFIG_GROUP;
	}
	else if(strstr(node->name, "nodes") != NULL)
	{
		ret_val = HM_CONFIG_NODE_TREE;
	}
	else if(strstr(node->name, "node") != NULL)
	{
		ret_val = HM_CONFIG_NODE_INSTANCE;
	}
	else if(strstr(node->name, "index") != NULL)
	{
		ret_val = HM_CONFIG_INDEX;
	}
	else if(strstr(node->name, "name") != NULL)
	{
		ret_val = HM_CONFIG_NAME;
	}
	else if(strstr(node->name, "role") != NULL)
	{
		ret_val = HM_CONFIG_ROLE;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_get_node_type */

/***************************************************************************/
/* Name:	hm_recurse_tree 											   */
/* Parameters: Input - 		node: xmlNode								   */
/*			   Input -		stack									   	   */
/* Return:	static void													   */
/* Purpose: Performs an in-depth traversal of the tree and writes config   */
/***************************************************************************/
static int32_t hm_recurse_tree(xmlNode *begin_node, HM_STACK *stack, HM_CONFIG_CB *hm_config)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	xmlNode *current_node = NULL;
	xmlAttr *attributes = NULL;

	HM_CONFIG_NODE *config_node = NULL;
	HM_CONFIG_NODE *parent_node = NULL;

	HM_STACK *reverse_stack = NULL;

	int32_t node_type;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(begin_node != NULL);
	TRACE_ASSERT(stack != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	for(current_node = begin_node; current_node; current_node = current_node->next)
	{
		/***************************************************************************/
		/* Check if the node is a blank one. We get plenty of them.				   */
		/***************************************************************************/
		if(xmlIsBlankNode(current_node))
		{
			continue;
		}

//		TRACE_INFO(("Node Type: %d", current_node->type));

		if(current_node->type == XML_ELEMENT_NODE)
		{
			node_type = hm_get_node_type(current_node);
			if(node_type == HM_ERR)
			{
				TRACE_ERROR(("Unknown type of node %d", node_type));
				/***************************************************************************/
				/* Not exiting. Ignoring errors											   */
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
			TRACE_DETAIL(("Push to stack if necessary"));
			switch (node_type)
			{
			case HM_CONFIG_ROOT:
			case HM_CONFIG_HM_INSTANCE:
			case HM_CONFIG_NODE_TREE:
			case HM_CONFIG_NODE_INSTANCE:

			case HM_CONFIG_SUBSCRIPTION_TREE:
				HM_STACK_PUSH(stack, config_node);
				break;

			case HM_CONFIG_ADDRESS:
				TRACE_INFO(("Address Type: %s", xmlGetProp(current_node, "type")));
				HM_STACK_PUSH(stack, config_node);
				break;

			case HM_CONFIG_HEARTBEAT:
				TRACE_INFO(("Scope of Heartbeat: %s", xmlGetProp(current_node, "scope")));
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
					/* Push parent node back on stack										   */
					/***************************************************************************/
					ret_val = HM_STACK_PUSH(stack, parent_node);
					if (ret_val == HM_ERR)
					{
						free(config_node);
						config_node = NULL;
						goto EXIT_LABEL;
					}
					/***************************************************************************/
					/* Push current node to stack. We're now expecting its value node		   */
					/* So, set the pointer to its config_cb in LOCAL in the opaque data		   */
					/***************************************************************************/
					//TODO
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
					/* Push parent node back on stack										   */
					/***************************************************************************/
					ret_val = HM_STACK_PUSH(stack, parent_node);
					if (ret_val == HM_ERR)
					{
						free(config_node);
						config_node = NULL;
						goto EXIT_LABEL;
					}
					/***************************************************************************/
					/* Push current node to stack. We're now expecting its value node		   */
					/* So, set the pointer to its config_cb in LOCAL in the opaque data		   */
					/***************************************************************************/
					//TODO
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
					/***************************************************************************/
					/* We expect its value to come next										   */
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
					TRACE_INFO(("[Attribute] resolution:  %s",xmlGetProp(current_node, "resolution")));
				}

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
					/***************************************************************************/
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
				TRACE_INFO(("[Attribute] Version: %s", xmlGetProp(current_node, "version")));
				TRACE_INFO(("[Attribute] Type: %s", xmlGetProp(current_node, "type")));
				break;

			case HM_CONFIG_PORT:
				parent_node = HM_STACK_POP(stack);
				TRACE_ASSERT(parent_node->type == HM_CONFIG_ADDRESS);
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
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
					/* We expect its value to come next										   */
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
				TRACE_INFO(("[Attribute] type: %s", xmlGetProp(current_node, "type")));
				break;
			default:
				/***************************************************************************/
				/* UNHITTABLE															   */
				/***************************************************************************/
				TRACE_DETAIL(("UNKNOWN NODE TYPE"));
				TRACE_ASSERT(0==1);

				break;
			}
		}
		else if(current_node->type == XML_TEXT_NODE)
		{
			/***************************************************************************/
			/* The value of an element is contained in the subsequent value node	   */
			/* by examining the stack, we can know if we are expecting one			   */
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
					/* Determine the context of the index. That would be one step back		   */
					/* But it could be many steps back too, so we implement a reverse stack	   */
					/* instead.																   */
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
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Hardware Index: %s", current_node->content));
					}
					else if(parent_node->type == HM_CONFIG_NODE_INSTANCE)
					{
						TRACE_DETAIL(("Parent Instance is of a Monitoring Node"));

						/***************************************************************************/
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Node Index: %s", current_node->content));
					}
					/***************************************************************************/
					/* Free the reverse stack												   */
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
					TRACE_INFO(("Heartbeat: %s", current_node->content));
					free(config_node);
					break;

				case HM_CONFIG_THRESHOLD:
					TRACE_INFO(("Threshold: %s", current_node->content));
					free(config_node);
					break;

				case HM_CONFIG_IP:
					TRACE_INFO(("IP: %s", current_node->content));
					free(config_node);
					break;

				case HM_CONFIG_PORT:
					TRACE_INFO(("Port: %s", current_node->content));
					free(config_node);
					break;

				case HM_CONFIG_GROUP:
					parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
					if (parent_node->type == HM_CONFIG_ADDRESS)
					{
						TRACE_DETAIL(("Parent Instance is Address Configuration"));
						/***************************************************************************/
						/* Push it back															   */
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
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Node Group: %s", current_node->content));
					}
					free(config_node);
					break;

				case HM_CONFIG_NAME:
					parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
					if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
					{
						TRACE_DETAIL(("Parent Instance is a Node Instance"));
						/***************************************************************************/
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Node Name: %s", current_node->content));
					}
					free(config_node);
					break;

				case HM_CONFIG_ROLE:
					parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
					if (parent_node->type == HM_CONFIG_NODE_INSTANCE)
					{
						TRACE_DETAIL(("Parent Instance is a Node Instance"));
						/***************************************************************************/
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Node Role: %s", current_node->content));
					}
					free(config_node);
					break;

				case HM_CONFIG_SUBSCRIPTION_INSTANCE:
					parent_node = (HM_CONFIG_NODE *)HM_STACK_POP(stack);
					if (parent_node->type == HM_CONFIG_SUBSCRIPTION_TREE)
					{
						TRACE_DETAIL(("Parent Instance is a Subscription Tree"));
						/***************************************************************************/
						/* Push it back															   */
						/***************************************************************************/
						ret_val = HM_STACK_PUSH(stack, parent_node);
						if (ret_val == HM_ERR)
						{
							ret_val = HM_ERR;
							goto EXIT_LABEL;
						}
						TRACE_INFO(("Subscription group: %s", current_node->content));
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
				/* This isn't necessarily an error.										   */
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
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_recurse_tree */

/***************************************************************************/
/* Name:	hm_parse_config 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Parses the file into the configuration structure			   */
/*																		   */
/*	Currently supported vocabulary:										   */
/*  XML Node Names:														   */
/*		config			: Root of config								   */
/*		hm_instance_info: Config for HM Binary							   */
/*		index			: Index value									   */
/*		heartbeat		: Heartbeat Configuration						   */
/*		period			: Timer Value			 						   */
/*		threshold		: Maximum number of timeouts					   */
/*		address			: Inet Address structure						   */
/*		ip				: IP Address									   */
/*		port			: Port Value									   */
/*		group			: Multicast Group								   */
/*		nodes			: Container of Nbase Nodes						   */
/*		node			: Single Node in cluster						   */
/*		name			: String Name									   */
/*		role			: Active/Passive								   */
/*		Group															   */
/***************************************************************************/
int32_t hm_parse_config(HM_CONFIG_CB *config_cb, char *config_file)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	xmlDoc *doc = NULL;	/* XML File representation */
	xmlNode *root = NULL;

	HM_STACK *config_stack = NULL;
	HM_CONFIG_NODE *config_node = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(config_cb != NULL);
	TRACE_ASSERT(config_file != NULL);

	TRACE_DETAIL(("Config File name: %s", config_file));
	/***************************************************************************/
	/* Main Routine															   */
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
	/* Read XML File into C													   */
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
	/* Parse HM Instance Specific Configuration								   */
	/***************************************************************************/
#if 0
	/* Just testing my stack :P It works fine */
	int a=10;
	int b=100;
	int c=134;
	HM_STACK_PUSH(config_stack, &a);
	HM_STACK_PUSH(config_stack, &b);
	HM_STACK_PUSH(config_stack, &c);
	int i;
	for(i=0; i<3;i++)
	{
		TRACE_INFO(("%d", *(int *)HM_STACK_POP(config_stack)));
	}
#endif

	/***************************************************************************/
	/* Start parsing the XML Tree now and build configuration				   */
	/***************************************************************************/
	hm_recurse_tree(root, config_stack, config_cb);

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_parse_config */
