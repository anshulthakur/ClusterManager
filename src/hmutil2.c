/*
 * hmutil2.c
 *
 *  Created on: 06-May-2015
 *      Author: anshul
 */


#include <hmincl.h>

/***************************************************************************/
/* Byte Ordering related functions										   */
/***************************************************************************/
/***************************************************************************/
/* Name:	hm_hton64 													   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int64_t														   */
/* Purpose: Converts from network byte order to host for 64 bit numbers	   */
/***************************************************************************/
int64_t hm_hton64(int64_t num)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
#ifndef BIG_ENDIAN
	int64_t *conv_val = NULL;
	int8_t *ptr = NULL;
	int8_t net_order[8];
	int8_t i;
#endif
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
#ifdef BIG_ENDIAN
	/***************************************************************************/
	/* Do nothing. Machine and Network Byte order are the same				   */
	/***************************************************************************/
	TRACE_EXIT();
	return num;
#else
	/***************************************************************************/
	/* Convert to Network Byte Order										   */
	/* A better way would have been to use masking							   */
	/***************************************************************************/
	ptr = (int8_t *)num;
	for(i=7; i>=0;i--)
	{
		net_order[i] = *(ptr + (7-i));
	}
	conv_val = (int64_t *)net_order;
	TRACE_EXIT();
	return (*conv_val);
#endif

}/* hm_hton64 */

/***************************************************************************/
/* Name:	hm_hton64 													   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int64_t														   */
/* Purpose: Converts from network byte order to host for 64 bit numbers	   */
/***************************************************************************/
int64_t hm_ntoh64(int64_t num)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
#ifndef BIG_ENDIAN
	int64_t *conv_val = NULL;
	int8_t *ptr = NULL;
	int8_t net_order[8];
	int8_t i;
#endif
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
#ifdef BIG_ENDIAN
	/***************************************************************************/
	/* Do nothing. Machine and Network Byte order are the same				   */
	/***************************************************************************/
	TRACE_EXIT();
	return num;
#else
	/***************************************************************************/
	/* Convert from Network Byte Order										   */
	/***************************************************************************/
	ptr = (int8_t *)num;
	for(i=7; i>=0;i--)
	{
		net_order[i] = *(ptr + (7-i));
	}
	conv_val = (int64_t *)net_order;
	TRACE_EXIT();
	return (*conv_val);
#endif
}/* hm_hton64 */


/***************************************************************************/
/* Timer Related Functions												   */
/***************************************************************************/

/***************************************************************************/
/* Name:	hm_timer_set_timer 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Sets the given time in ms on timer			*/
/***************************************************************************/
static int32_t hm_timer_set_timer(timer_t timerID, int32_t period, int32_t repeat)
{
	int32_t rc = HM_OK;
    struct itimerspec its;

    TRACE_ENTRY();
	/***************************************************************************/
	/* Validate Input														   */
	/***************************************************************************/
	TRACE_ASSERT(timerID != NULL);

    /***************************************************************************/
	/* Determine the number of seconds and nano seconds from input time in MS  */
	/***************************************************************************/
    if(period/1000 == 0)
    {
    	its.it_interval.tv_sec = (time_t )0;
    	its.it_interval.tv_nsec = (long int )(period * 1000000);

    }
    else if(period/1000 > 0)
    {
    	its.it_interval.tv_sec = (time_t )(period/1000);
    	its.it_interval.tv_nsec = (long int )((period%1000) * 1000000);
    }

    TRACE_DETAIL(("Timer Interval Sec = %lu",(unsigned long )its.it_interval.tv_sec));
    TRACE_DETAIL(("Timer Interval nS = %lu", (unsigned long )its.it_interval.tv_nsec));
    if(!repeat)
    {
    	TRACE_DETAIL(("Non repeating timer"));
    	period = 0;
    }

	if(period/1000 == 0)
	{
		its.it_value.tv_sec = (time_t )0;
		its.it_value.tv_nsec =(long int ) (period * 1000000);

	}
	else if(period/1000 > 0)
	{
		its.it_value.tv_sec = (time_t )(period/1000);
		its.it_value.tv_nsec = (long int )((period%1000) * 1000000);
	}

    if((timer_settime(timerID, 0, &its, NULL))!=0)
    {
    	TRACE_PERROR(("Error in setting the timer"));
    	rc = HM_ERR;
    	goto EXIT_LABEL;
    }
EXIT_LABEL:

	TRACE_EXIT();
	return (rc);
}/* hm_timer_set_timer */

/**PROC+**********************************************************************/
/* Name:     hm_base_timer_handler  		                                 */
/*                                                                           */
/* Purpose:  Function invoked when any timer of this module expires.         */
/*                                                                           */
/* Returns:   VOID  :									                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	: sig - Signal			                                 */
/*			  IN	: si  - Signal Info containing the parameters of timer	 */
/*			  IN	: uc  - Not used (additional data)						 */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: Finds the appropriate timer in the list of timers with the     */
/*			 transport layer. If the Timer is of Connection Type, then the   */
/*			 connection must be closed down on this timer's expiry.			 */
/*			 If it is of Keepalive type, Call into the FSM with Keepalive as */
/*           input signal                                                    */
/**PROC-**********************************************************************/
VOID hm_base_timer_handler(int32_t sig, siginfo_t *si, void *uc )
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
    timer_t *tidp = NULL;
    HM_TIMER_CB *timer_cb = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	tidp = si->si_value.sival_ptr;

	/* Find the Node in global timer table using  '*tidp' value stored in tidp */
	/* and invoke its callback.												   */
	TRACE_DETAIL(("Timer value: 0x%x", *(uint32_t *)tidp));
	if(global_timer_table.root == NULL)
	{
		TRACE_INFO(("Empty tree!"));
	}
	timer_cb = (HM_TIMER_CB *)HM_AVL3_FIND(global_timer_table,
										tidp, timer_table_by_handle);
	TRACE_ASSERT(timer_cb != NULL);
	if(timer_cb ==NULL)
	{
		TRACE_ERROR(("Timer CB not found! Delete timer!"));
		timer_delete(*tidp);
		goto EXIT_LABEL;
	}
	TRACE_DETAIL(("Found CB. Invoke callback"));
	timer_cb->callback((void *)timer_cb->parent);

	/***************************************************************************/
	/* TODO: Do we need to re-arm the timer or will it keep repeating?		   */
	/***************************************************************************/

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
}/* hm_base_timer_handler */

/**PROC+**********************************************************************/
/* Name:     hm_timer_create		                                         */
/*                                                                           */
/* Purpose:   Creates a timer with the given parameters.                     */
/*                                                                           */
/* Returns:    int rc :											             */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	: name - Name of CB.	                                 */
/*			  IN	: timerID - Unique ID of the structure					 */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/
HM_TIMER_CB * hm_timer_create(uint32_t period,
								uint32_t repeat,
								HM_TIMER_CALLBACK *func,
								void * parent)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_TIMER_CB *timer_cb = NULL;
    struct sigevent         te;
    int32_t  sigNo = SIGRTMIN;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(period >0);
	TRACE_ASSERT((repeat == TRUE)||(repeat == FALSE));
	TRACE_ASSERT(func != NULL);

	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/* Allocated Timer CB	*/
	timer_cb = (HM_TIMER_CB *)malloc(sizeof(HM_TIMER_CB));
	if(timer_cb == NULL)
	{
		TRACE_ERROR(("Error allocating resources for timer creation."));
		goto EXIT_LABEL;
	}

	timer_cb->callback = func;
	HM_AVL3_INIT_NODE(timer_cb->node, timer_cb);
	timer_cb->parent = parent;
	timer_cb->timerID = NULL;
	timer_cb->handle = -1;
	timer_cb->repeat = repeat;
	timer_cb->running = FALSE;
	timer_cb->period = period;

    /* Block timer signal temporarily */

    TRACE_DETAIL(("Blocking signal %d", sigNo));
    sigemptyset(&mask);
    sigaddset(&mask, sigNo);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		TRACE_PERROR(("Error blocking signal while timer creation"));
        free(timer_cb);
        timer_cb = NULL;
        goto EXIT_LABEL;
	}
    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = &timer_cb->timerID;
    if(timer_create(CLOCK_REALTIME, &te, &timer_cb->timerID)== -1)
    {
    	TRACE_PERROR(("Error creating timer."));
        free(timer_cb);
        timer_cb = NULL;
        goto EXIT_LABEL;
    }

    timer_cb->handle = (uint32_t)*(&timer_cb->timerID);
    TRACE_DETAIL(("Timer handle: %x", timer_cb->handle));
    TRACE_DETAIL(("Memory: %x", (uint32_t)timer_cb->timerID));
    /***************************************************************************/
	/* Insert the timer into global table									   */
	/***************************************************************************/
	if(HM_AVL3_INSERT(global_timer_table, timer_cb->node, timer_table_by_handle)!= TRUE)
	{
		TRACE_ERROR(("Error inserting timer into global table."));
		/***************************************************************************/
		/* delete the timer and then remove it from the timer list				   */
		/***************************************************************************/
		timer_delete(timer_cb->timerID);
        free(timer_cb);
        timer_cb = NULL;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (timer_cb);
} /* hm_timer_create */

/**PROC+**********************************************************************/
/* Name:     hm_timer_start                                         		 */
/*                                                                           */
/* Purpose:   Arms the timer.						                         */
/*                                                                           */
/* Returns:   BOOL rc :									                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	:timer_ID – Reference to the timer that must be set.     */
/*			  IN	: expireMS - Timer expires in (milliseconds)			 */
/*            IN	: intervalMS - Repeats after (milliseconds)              */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/
int32_t hm_timer_start(HM_TIMER_CB *timer_cb)
{

	int32_t ret_val = HM_OK;
	TRACE_ENTRY();
	TRACE_ASSERT(timer_cb != NULL);

	if(hm_timer_set_timer(timer_cb->timerID, timer_cb->period, timer_cb->repeat)!= HM_OK)
	{
		TRACE_ERROR(("Error occurred while starting timer"));
		ret_val = HM_ERR;
	}
	timer_cb->running = TRUE;

	TRACE_EXIT();
	return ret_val;
} /* hm_timer_start */


/**PROC+**********************************************************************/
/* Name:     hm_timer_modify                                         		 */
/*                                                                           */
/* Purpose:   Arms the timer with new value if running.                      */
/*                                                                           */
/* Returns:   BOOL rc :									                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	:timer_ID – Reference to the timer that must be set.     */
/*			  IN	: expireMS - Timer expires in (milliseconds)			 */
/*            IN	: intervalMS - Repeats after (milliseconds)              */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: 											                     */
/*                                                                           */
/**PROC-**********************************************************************/
int32_t hm_timer_modify(HM_TIMER_CB *timer_cb, uint32_t period)
{

	int32_t ret_val = HM_OK;
	TRACE_ENTRY();
	TRACE_ASSERT(timer_cb != NULL);
	TRACE_ASSERT(period !=0);

	timer_cb->period = period;
	if(timer_cb->running)
	{
		if(hm_timer_set_timer(timer_cb->timerID, period, timer_cb->repeat)!= HM_OK)
		{
			TRACE_ERROR(("Error occurred while modifying timer"));
			ret_val = HM_ERR;
		}
	}

	TRACE_EXIT();
	return ret_val;
} /* hm_timer_modify */

/***************************************************************************/
/* Name:	hm_timer_stop 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Stops the timer if it is running. Silently exits otherwise	   */
/***************************************************************************/
int32_t hm_timer_stop(HM_TIMER_CB *timer_cb)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(timer_cb != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(timer_cb->running)
	{
		if(hm_timer_set_timer(timer_cb->timerID, 0, FALSE)!= HM_OK)
		{
			TRACE_ERROR(("Error occurred while stopping timer"));
			ret_val = HM_ERR;
		}
	}
	timer_cb->running = FALSE;
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_timer_stop */

/**PROC+**********************************************************************/
/* Name:     hm_timer_delete  	   	                                         */
/*                                                                           */
/* Purpose:   Deletes the timer.					                         */
/*                                                                           */
/* Returns:  VOID 										                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	:timer_ID – Reference to the timer that must be set.     */
/*                                                                           */
/* Operation: Deletes the timer and remove it from the timer list            */
/*                                                                           */
/**PROC-**********************************************************************/
VOID hm_timer_delete(HM_TIMER_CB *timer_cb)
{

	TRACE_ENTRY();

	/***************************************************************************/
	/* delete the timer and then remove it from the timer list				   */
	/***************************************************************************/
	timer_delete(timer_cb->timerID);

	/***************************************************************************/
	/* Remove from the global tree											   */
	/***************************************************************************/
	HM_AVL3_DELETE(global_timer_table, timer_cb->node);

	free(timer_cb);
	timer_cb = NULL;

	TRACE_EXIT();
	return ;
}/* hm_timer_delete */

/***************************************************************************/
/* Stack functions														   */
/***************************************************************************/
/***************************************************************************/
/* Name:	hm_stack_init 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	HM_STACK *													   */
/* Purpose: Initializes a stack with given size							   */
/***************************************************************************/
HM_STACK * hm_stack_init(int32_t size)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_STACK *stack = NULL;
	int32_t i = 0;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(size > 0);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	TRACE_DETAIL(("Initialize stack of size %d", size));
	/***************************************************************************/
	/* Allocate a contiguous memory chunk of size 							   */
	/***************************************************************************/
	stack = (HM_STACK *)malloc(sizeof(HM_STACK)+ ((sizeof(void *))*size));
	if(stack == NULL)
	{
		TRACE_ERROR(("Error allocating stack"));
		stack = NULL;
		goto EXIT_LABEL;
	}
	/***************************************************************************/
	/* Got memory															   */
	/***************************************************************************/
	TRACE_INFO(("Got memory. Set defaults"));
	stack->self = stack;
	stack->size = size;
	stack->top = -1;
	stack->stack = (void *)((char *)(stack)+ (uint32_t)sizeof(HM_STACK));

	TRACE_INFO(("Memory starts at: %p", stack->self));
	TRACE_INFO(("Stack starts at: %p", stack->stack));
	/***************************************************************************/
	/* Set all elements in stack to NULL									   */
	/***************************************************************************/
	for(i = 0 ; i< size; i++)
	{
		stack->stack[i] = NULL;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return stack;
}/* hm_stack_init */

/***************************************************************************/
/* Name:	hm_stack_destroy 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	void														   */
/* Purpose: Deallocate memory for the stack								   */
/***************************************************************************/
void hm_stack_destroy(HM_STACK *stack)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(stack != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	/* Since the chunk was fixed, we need not do anything else other than free */
	free(stack);
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return;
}/* hm_stack_destroy */

/***************************************************************************/
/* Name:	hm_stack_push 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Push the element to the stack								   */
/***************************************************************************/
int32_t hm_stack_push(HM_STACK *stack_ptr, void *value)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_DETAIL(("PUSH"));
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(stack_ptr->top == (stack_ptr->size-1))
	{
		TRACE_DETAIL(("Stack full. Push not allowed"));
		ret_val = HM_ERR;
		goto EXIT_LABEL;
	}
	stack_ptr->top +=1;
	stack_ptr->stack[stack_ptr->top] = value;

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	return ret_val;
}/* hm_stack_push */

/***************************************************************************/
/* Name:	hm_stack_pop 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	void *														   */
/* Purpose: Pops an element from the stack								   */
/***************************************************************************/
void * hm_stack_pop(HM_STACK *stack)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	void * element = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_DETAIL(("POP"));
	TRACE_ASSERT(stack != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(stack->top < 0)
	{
		TRACE_ERROR(("Stack is empty. Pop not allowed"));
		goto EXIT_LABEL;
	}

	element = stack->stack[stack->top];
	TRACE_ASSERT(element != NULL); /* MUST NOT BE NULL */

	stack->top -=1;

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	return element;
}/* hm_stack_pop */

/***************************************************************************/
/* Name:	hm_compare_ulong 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Compares two ulong integers and gives -1,0, +1 on first being  */
/*	smaller, equal or greater than second.								   */
/***************************************************************************/
int32_t hm_compare_ulong(void *key1, void *key2)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = -1;
	uint32_t *aa = (uint32_t *)key1;
	uint32_t *bb = (uint32_t *)key2;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(aa != NULL);
	TRACE_ASSERT(bb != NULL);

	TRACE_DETAIL(("First: 0x%x", *aa));
	TRACE_DETAIL(("Second: 0x%x", *bb));
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(*aa>*bb)
	{
		ret_val = 1;
	}
	else if(*aa==*bb)
	{
		ret_val = 0;
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_compare_ulong */

/***************************************************************************/
/* Name:	hm_compare_2_ulong 											   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Compares two ulong composite key and gives -1,0, +1 on first   */
/*	being smaller, equal or greater than second.						   */
/***************************************************************************/
int32_t hm_compare_2_ulong(void *key1, void *key2)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = -1;
	uint32_t *a1 = (uint32_t *)key1;
	uint32_t *a2 = (uint32_t *)(key1)+1;

	uint32_t *b1 = (uint32_t *)key2;
	uint32_t *b2 = (uint32_t *)(key2)+1;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(key1 != NULL);
	TRACE_ASSERT(key2 != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(*a1>*b1)
	{
		ret_val = 1;
	}
	else if(*a1==*b1)
	{
		if(*a2 > *b2)
		{
			ret_val = 1;
		}
		else if(*a2== *b2)
		{
			ret_val = 0;
		}
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return (ret_val);
}/* hm_compare_ulong */


/***************************************************************************/
/* Name:	hm_aggregate_compare_node_id 								   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t													       */
/* Purpose: Compares the composite key for node: ID and Node ID			   */
/* Node IDs are unique currently, so it wouldn't have mattered if we had   */
/* compared them directly with their Node IDs. But this may not hold in the*/
/* future and we might need more info. Hence this function.			       */
/***************************************************************************/
int32_t hm_aggregate_compare_node_id(void *key1, void *key2)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = -1;
	HM_NODE_CB *node1 = (HM_NODE_CB *)key1;
	HM_NODE_CB *node2 = (HM_NODE_CB *)key2;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(node1->index > node2->index)
	{
		ret_val = 1;
	}
	else if(node1->index == node2->index)
	{
		ret_val = 0;
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_aggregate_compare_node_id */

/***************************************************************************/
/* Name:	hm_aggregate_compare_pid 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	int32_t									*/
/* Purpose: Compares the PID of two Process CBs			*/
/***************************************************************************/
int32_t hm_aggregate_compare_pid(void *key1, void *key2)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = -1;
	HM_PROCESS_CB *node1 = (HM_PROCESS_CB *)key1;
	HM_PROCESS_CB *node2 = (HM_PROCESS_CB *)key2;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(node1->pid > node2->pid)
	{
		ret_val = 1;
	}
	else if(node1->pid == node2->pid)
	{
		ret_val = 0;
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_aggregate_compare_pid */

/***************************************************************************/
/* Name:	hm_aggregate_compare_if_id 									   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Compares interface types								   	   */
/***************************************************************************/
int32_t hm_aggregate_compare_if_id(void *key1, void *key2)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = -1;
	HM_INTERFACE_CB *node1 = (HM_INTERFACE_CB *)key1;
	HM_INTERFACE_CB *node2 = (HM_INTERFACE_CB *)key2;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(node1->if_type > node2->if_type)
	{
		ret_val = 1;
	}
	else if(node1->if_type == node2->if_type)
	{
		ret_val = 0;
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_aggregate_compare_if_id */


/***************************************************************************/
/* Name:	hm_get_buffer 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	HM_MSG *													   */
/* Purpose: Allocates and initializes a buffer to be used for IPS Comm.	   */
/***************************************************************************/
HM_MSG * hm_get_buffer(uint32_t size)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG *msg = NULL;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(size>0);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	msg = (HM_MSG *)malloc(sizeof(HM_MSG) + size);
	if(msg == NULL)
	{
		TRACE_ERROR(("Error allocating buffers for notification."));
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(HM_MSG) + size);
	msg->msg = (char *)msg + sizeof(HM_MSG);

	msg->msg_len = size;

	HM_INIT_LQE(msg->node, msg);
	/***************************************************************************/
	/* Since it is allocated, at least one person is referencing it.		   */
	/***************************************************************************/
	msg->ref_count = 1;

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return msg;
}/* hm_get_buffer */

/***************************************************************************/
/* Name:	hm_grow_buffer 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	HM_MSG *									*/
/* Purpose: Increases the capacity of the buffer to new value			*/
/***************************************************************************/
HM_MSG * hm_grow_buffer(HM_MSG *buf, uint32_t size)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG *msg = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(buf != NULL);
	TRACE_ASSERT(size > buf->msg_len);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	msg = (HM_MSG *)realloc(buf, sizeof(HM_MSG) + size);
	if(msg == NULL)
	{
		TRACE_ERROR(("Error allocating buffers for notification."));
		goto EXIT_LABEL;
	}
	msg->msg = (char *)msg + sizeof(HM_MSG);

	/***************************************************************************/
	/* New size																   */
	/***************************************************************************/
	msg->msg_len = size;

	/***************************************************************************/
	/* Fix pointers in sibling members of list								   */
	/***************************************************************************/
	if(HM_IN_LIST(msg->node))
	{
		TRACE_DETAIL(("Fixing sibling pointers"));
		msg->node.prev->next = msg->node.self;
		msg->node.next->prev = msg->node.self;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return msg;
}/* hm_grow_buffer */


/***************************************************************************/
/* Name:	hm_shrink_buffer 									*/
/* Parameters: Input - 										*/
/*			   Input/Output -								*/
/* Return:	HM_MSG *									*/
/* Purpose: Increases the capacity of the buffer to new value			*/
/***************************************************************************/
HM_MSG * hm_shrink_buffer(HM_MSG *buf, uint32_t size)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	HM_MSG *msg = NULL;

	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();

	TRACE_ASSERT(buf != NULL);
	TRACE_ASSERT(size < buf->msg_len);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	msg = (HM_MSG *)realloc(buf, sizeof(HM_MSG) + size);
	if(msg == NULL)
	{
		TRACE_ERROR(("Error allocating buffers for notification."));
		goto EXIT_LABEL;
	}
	msg->msg = (char *)msg + sizeof(HM_MSG);

	/***************************************************************************/
	/* New size																   */
	/***************************************************************************/
	msg->msg_len = size;

	/***************************************************************************/
	/* Fix pointers in sibling members of list								   */
	/***************************************************************************/
	if(HM_IN_LIST(msg->node))
	{
		TRACE_DETAIL(("Fixing sibling pointers"));
		msg->node.prev->next = msg->node.self;
		msg->node.next->prev = msg->node.self;
	}

EXIT_LABEL:
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return msg;
}/* hm_shrink_buffer */

/***************************************************************************/
/* Name:	hm_free_buffer 												   */
/* Parameters: Input - 													   */
/*			   Input/Output -											   */
/* Return:	int32_t														   */
/* Purpose: Frees the buffer or decrements its reference count if in use   */
/***************************************************************************/
int32_t hm_free_buffer(HM_MSG *msg)
{
	/***************************************************************************/
	/* Variable Declarations												   */
	/***************************************************************************/
	int32_t ret_val = HM_OK;
	/***************************************************************************/
	/* Sanity Checks														   */
	/***************************************************************************/
	TRACE_ENTRY();
	TRACE_ASSERT(msg != NULL);
	/***************************************************************************/
	/* Main Routine															   */
	/***************************************************************************/
	if(--msg->ref_count == 0)
	{
		TRACE_DETAIL(("Freeing Buffer"));
		free(msg);
	}
	/***************************************************************************/
	/* Exit Level Checks													   */
	/***************************************************************************/
	TRACE_EXIT();
	return ret_val;
}/* hm_free_buffer */
