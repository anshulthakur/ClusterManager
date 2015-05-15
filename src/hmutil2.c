/*
 * hmutil2.c
 *
 *  Created on: 06-May-2015
 *      Author: anshul
 */


#include <hmincl.h>

/***************************************************************************/
/* Timer Related Functions												   */
/***************************************************************************/
#if 0
/**PROC+**********************************************************************/
/* Name:     hm_tprt_make_timer		                                         */
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

int32_t hm_tprt_make_timer(char name[], timer_t *timerID)
{
	static int32_t rc = FALSE;
    struct sigevent         te;

    struct sigaction        sa;
    extern sigset_t mask;
    int32_t  sigNo = SIGRTMIN;


	TRACE_ENTRY();

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = hm_tprt_timer_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        printf("Failed to setup signal handling for %s.\n", name);
        return(-1);
    }

    /* Block timer signal temporarily */

    printf("\nBlocking signal %d\n", sigNo);
    sigemptyset(&mask);
    sigaddset(&mask, sigNo);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		exit(0);
	}

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timerID;
    if(timer_create(CLOCK_REALTIME, &te, timerID)== -1)
    {
    	printf("\nError creating timer.");
    	perror("Cause: ");
    }

    rc = TRUE;
	TRACE_EXIT();
	return (rc);
} /* hm_tprt_make_timer */

/**PROC+**********************************************************************/
/* Name:     hm_tprt_set_timer  	                                         */
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

uint32_t hm_tprt_set_timer(timer_t *timerID, int32_t expireMS, int32_t intervalMS)
{
	uint32_t rc = FALSE;
    struct itimerspec its;

    TRACE_ENTRY();
	/***************************************************************************/
	/* Validate Input														   */
	/***************************************************************************/
	if(timerID == NULL)
	{
		printf("\nInvalid reference to Timer");
		goto EXIT_LABEL;
	}

    /***************************************************************************/
	/* Determine the number of seconds and nano seconds from input time in MS  */
	/***************************************************************************/
    if(intervalMS/1000 == 0)
    {
    	its.it_interval.tv_sec = (time_t )0;
    	its.it_interval.tv_nsec = (long int )(intervalMS * 1000000);

    }
    else if(intervalMS/1000 > 0)
    {
    	its.it_interval.tv_sec = (time_t )(intervalMS/1000);
    	its.it_interval.tv_nsec = (long int )((intervalMS%1000) * 1000000);
    }

    printf("\n Timer Interval Seconds = %lu\nTimer Interval nS = %lu",
    							(unsigned long )its.it_interval.tv_sec,
    							(unsigned long )its.it_interval.tv_nsec);
    if(expireMS/1000 == 0)
    {
    	its.it_value.tv_sec = (time_t )0;
    	its.it_value.tv_nsec =(long int ) (expireMS * 1000000);

    }
    else if(expireMS/1000 > 0)
    {
    	its.it_value.tv_sec = (time_t )(expireMS/1000);
    	its.it_value.tv_nsec = (long int )((expireMS%1000) * 1000000);
    }

    printf("\nTimer Value Seconds = %lu\nTimer Value nS = %lu",
    				(unsigned long )its.it_value.tv_sec,
    				(unsigned long )its.it_value.tv_nsec);

    printf("\nTimer ID %p", timerID);
    fflush(stdout);

    if((timer_settime(timerID, 0, &its, NULL))!=0)
    {
    	printf("\n Error in setting the timer");
    	goto EXIT_LABEL;
    }

    rc = TRUE;

EXIT_LABEL:

	TRACE_EXIT();
	return (rc);
} /* hm_tprt_set_timer */

/**PROC+**********************************************************************/
/* Name:     hm_tprt_timer_delete  	                                         */
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

VOID hm_tprt_timer_delete(timer_t* timer_id)
{
	HM_TPRT_TIMER_CB *timer_cb = NULL;

	TRACE_ENTRY();

	fflush(stdout);

	// delete the timer and then remove it from the timer list
	timer_delete(timer_id);

	// check in keep alive timer list
	 for(timer_cb = (HM_TPRT_TIMER_CB *)(tprt_local.heartbeat_timer_list.next);
                timer_cb != NULL;
                timer_cb = (HM_TPRT_TIMER_CB *)LQE_NEXT_IN_LIST(((LQE *)timer_cb)))
	{
		 printf("\n Found Keepalive timer");
		 if(timer_id == &(timer_cb->timer_val))
			LQE_REMOVE_FROM_LIST(timer_cb->node);
		 FREE(timer_cb);
	}

	// check in the connection timer list as well
	for(timer_cb = (HM_TPRT_TIMER_CB *)(tprt_local.connection_timer_list.next);
                        timer_cb != NULL;
                        timer_cb = (HM_TPRT_TIMER_CB *)LQE_NEXT_IN_LIST(((LQE *)timer_cb)))
	{
		printf("\n Found Connection timer");
		if(timer_id == &(timer_cb->timer_val))
                LQE_REMOVE_FROM_LIST(timer_cb->node);
		FREE(timer_cb);
	}

	TRACE_EXIT();
	return ;
}/* hm_tprt_timer_delete */


/**PROC+**********************************************************************/
/* Name:     hm_tprt_timer_handler  		                                 */
/*                                                                           */
/* Purpose:  Function invoked when any timer of this module expires.         */
/*                                                                           */
/* Returns:   VOID  :									                     */
/*           				                                                 */
/*                                                                           */
/* Params:    IN 	: sig - Signal			                                 */
/*			  IN	: si  - Signal Info containing the parameters of timer	 */
/*			  IN	: uc : additional data						 */
/*            IN/OUT										                 */
/*                                                                           */
/* Operation: Finds the appropriate timer in the list of timers with the     */
/*			 transport layer. If the Timer is of Connection Type, then the   */
/*			 connection must be closed down on this timer's expiry.			 */
/*			 If it is of Keepalive type, Call into the FSM with Keepalive as */
/*           input signal                                                    */
/**PROC-**********************************************************************/

VOID hm_tprt_timer_handler( int sig, siginfo_t *si, void * uc )
{
    timer_t *tidp;

    TRACE_ENTRY();

	TRACE_EXIT();

    return;
} /* hm_tprt_timer_handler */
#endif

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
