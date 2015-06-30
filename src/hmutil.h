/*
 * hmutil.h
 *
 *  Created on: 06-May-2015
 *      Author: anshul
 */

#ifndef SRC_HMUTIL_H_
#define SRC_HMUTIL_H_

/***************************************************************************/
/* Useful Utility Defines												   */
/***************************************************************************/
#define VOID void
#define TRUE 				(1==1)
#define FALSE 				(0==1)

#define MIN(X, Y)   (((X)<(Y)) ? (X) : (Y))
#define MAX(X, Y)   (((X)>(Y)) ? (X) : (Y))

#define HM_OK 				((int16_t)0)
#define HM_ERR 				((int16_t)-1)

#define GET_TABLE_TYPE(row_cb)  (*(uint32_t *)((char *)row_cb + sizeof(uint32_t)))

/***************************************************************************/
/* To address to Memory Addresses										   */
/***************************************************************************/
#define BYTE 		uint8_t

typedef struct sockaddr_in				SOCKADDR_IN;
typedef struct sockaddr					SOCKADDR;
typedef struct sockaddr_in6				SOCKADDR_IN6;
typedef struct in_addr					IN_ADDR;
typedef struct in6_addr					IN_ADDR6;
/*****************************************************************************/
/* AVL3 functions.                                                           */
/*****************************************************************************/
void *avl3_insert_or_find(HM_AVL3_TREE *,
						 	 HM_AVL3_NODE *,
							 const HM_AVL3_TREE_INFO *);

void avl3_delete(HM_AVL3_TREE *, HM_AVL3_NODE *);

void *avl3_find(HM_AVL3_TREE *,
				   void *,
				   const HM_AVL3_TREE_INFO *);

void *avl3_find_or_find_next(HM_AVL3_TREE *,
								void *,
								uint32_t,
								const HM_AVL3_TREE_INFO *);

void *avl3_first(HM_AVL3_TREE *,
					const HM_AVL3_TREE_INFO *);

void *avl3_last(HM_AVL3_TREE *,
				   const HM_AVL3_TREE_INFO *);

void *avl3_next(HM_AVL3_NODE *,
				   const HM_AVL3_TREE_INFO *);

void *avl3_prev(HM_AVL3_NODE *,
				   const HM_AVL3_TREE_INFO *);

void avl3_verify_tree(HM_AVL3_TREE *,
				 	 const HM_AVL3_TREE_INFO *);

HM_AVL3_GEN_NODE * hm_avl3_gen_init(void *, void *);
/*****************************************************************************/
/* AVL3 access macros.                                                       */
/*****************************************************************************/
#define HM_AVL3_INIT_TREE(TREE, TREE_INFO)                                    \
  (TREE).first = NULL;                                                        \
  (TREE).last = NULL;                                                         \
  (TREE).root = NULL;

#define HM_AVL3_INIT_NODE(NODE, SELF)    (NODE).parent = NULL;                \
                                         (NODE).left = NULL;                  \
                                         (NODE).right = NULL;                 \
                                         (NODE).left_height = -1;                \
                                         (NODE).right_height = -1;				\
										 (NODE).self = SELF

#define HM_AVL3_INSERT(TREE, NODE, TREE_INFO)                                    \
    (NULL == avl3_insert_or_find(&(TREE), &(NODE), &(TREE_INFO) ))

#define HM_AVL3_INSERT_OR_FIND(TREE, NODE, TREE_INFO)                            \
              avl3_insert_or_find(&(TREE), &(NODE), &(TREE_INFO) )

#define HM_AVL3_DELETE(TREE, NODE)      avl3_delete(&(TREE), &(NODE))

#define HM_AVL3_FIND(TREE, KEY, TREE_INFO)                                       \
                          avl3_find(&(TREE), (KEY), &(TREE_INFO) )

#define HM_AVL3_NEXT(NODE, TREE_INFO)                                            \
                                 avl3_next(&(NODE), &(TREE_INFO) )

#define HM_AVL3_PREV(NODE, TREE_INFO)                                            \
                                 avl3_prev(&(NODE), &(TREE_INFO) )

#define HM_AVL3_FIRST(TREE, TREE_INFO)                                           \
                                avl3_first(&(TREE), &(TREE_INFO) )

#define HM_AVL3_LAST(TREE, TREE_INFO)                                            \
                                 avl3_last(&(TREE), &(TREE_INFO) )

#define HM_AVL3_IN_TREE(NODE) (((NODE).left_height != -1) && ((NODE).right_height != -1))

#define HM_AVL3_FIND_NEXT(TREE, KEY, TREE_INFO)                                  \
		avl3_find_or_find_next(&(TREE), (KEY), TRUE, &(TREE_INFO) )

#define HM_AVL3_FIND_OR_FIND_NEXT(TREE, KEY, TREE_INFO)                          \
		avl3_find_or_find_next(&(TREE), (KEY), FALSE, &(TREE_INFO) )

#define HM_AVL3_MOVE_TREE(TARGET_TREE, SRC_TREE, TREE_INFO)                      \
                       TRACE_ASSERT((TARGET_TREE).first == NULL);          \
                       TRACE_ASSERT((TARGET_TREE).last == NULL);           \
                       TRACE_ASSERT((TARGET_TREE).root == NULL);           \
                       (TARGET_TREE).first = (SRC_TREE).first;                \
                       (TARGET_TREE).last = (SRC_TREE).last;                  \
                       (TARGET_TREE).root = (SRC_TREE).root;                  \
                       HM_AVL3_INIT_TREE((SRC_TREE), (TREE_INFO));

#define HM_AVL3_GEN_INIT(KEY, PARENT)	hm_avl3_gen_init(KEY, PARENT)

/*****************************************************************************/
/* List Management                                                           */
/*****************************************************************************/
#define HM_INIT_ROOT(R)        (R).self = NULL;               \
                                (R).next = &(R);               \
                                (R).prev = &(R)

#define HM_INIT_LQE(E, S)      (E).self = (S);                \
                                (E).next = NULL;               \
                                (E).prev = NULL

#define HM_LIST_ROOT(R)        ((R).self == NULL)
#define HM_EMPTY_LIST(R)       ((R).next == &(R))
#define HM_IN_LIST(E)          ((E).next != NULL)
#define HM_NEXT_IN_LIST(E)     (VOID *)((E).next->self)
#define HM_PREV_IN_LIST(E)     (VOID *)((E).prev->self)

#define HM_INSERT_AFTER(P, E)  TRACE_ASSERT((E).next == NULL);         \
								TRACE_ASSERT((E).prev == NULL);         \
								TRACE_ASSERT((P).prev != NULL);         \
								TRACE_ASSERT((P).next!= NULL);         \
                                (E).next = (P).next;                       \
                                (E).prev = &(P);                           \
                                (E).next->prev = &(E);                     \
                                (E).prev->next = &(E)

#define HM_INSERT_BEFORE(N, E) TRACE_ASSERT((E).next== NULL);         \
								TRACE_ASSERT((E).prev== NULL);         \
								TRACE_ASSERT((N).prev != NULL);         \
								TRACE_ASSERT((N).next != NULL);         \
                                (E).prev = (N).prev;                       \
                                (E).next = &(N);                           \
                                (E).next->prev = &(E);                     \
                                (E).prev->next = &(E)

#define HM_INSERT_LIST_AFTER(P, R)                                        \
								TRACE_ASSERT(HM_LIST_ROOT(R) == TRUE);         \
                                if (!HM_EMPTY_LIST(R))                    \
                                {                                          \
                                	TRACE_ASSERT((P).prev != NULL);       \
                                	TRACE_ASSERT((P).next != NULL);       \
                                  (P).next->prev = (R).prev;               \
                                  (R).prev->next = (P).next;               \
                                  (P).next = (R).next;                     \
                                  (R).next->prev = &(P);                   \
                                }                                          \
                                HM_INIT_ROOT(R)

#define HM_INSERT_LIST_BEFORE(N, R)                                       \
								TRACE_ASSERT(HM_LIST_ROOT(R) == TRUE);         \
                                if (!HM_EMPTY_LIST(R))                    \
                                {                                          \
                                	TRACE_ASSERT((N).prev != NULL);       \
                                	TRACE_ASSERT((N).next != NULL);       \
                                  (N).prev->next = (R).next;               \
                                  (R).next->prev = (N).prev;               \
                                  (N).prev = (R).prev;                     \
                                  (R).prev->next = &(N);                   \
                                }                                          \
                                HM_INIT_ROOT(R)

#define HM_REMOVE_FROM_LIST(E) TRACE_ASSERT((E).prev != NULL);         \
								TRACE_ASSERT((E).next != NULL);         \
                                (E).next->prev = (E).prev;                 \
                                (E).prev->next = (E).next;                 \
                                (E).next = NULL;                           \
                                (E).prev = NULL


/*****************************************************************************/
/* Find the offset of specified field within specified structure             */
/*****************************************************************************/
#define HM_OFFSETOF(STRUCT, FIELD)                                           \
            (uint32_t)((BYTE *)(&((STRUCT *)0)->FIELD) - (BYTE *)0)

/***************************************************************************/
/* Stack operations														   */
/***************************************************************************/
HM_STACK * hm_stack_init(int32_t );
void hm_stack_destroy(HM_STACK *);
int32_t hm_stack_push(HM_STACK *, void *);
void * hm_stack_pop(HM_STACK *);

#define HM_STACK_INIT(stack)				hm_stack_init(stack)
#define HM_STACK_DESTROY(stack)				hm_stack_destroy(stack)
#define HM_STACK_PUSH(stack, element)										\
				hm_stack_push(stack, element)

#define HM_STACK_POP(stack)													\
				hm_stack_pop(stack)

#define HM_PUSH_ALLOWED(stack)												\
	((stack->top < (stack->size -1)) ? TRUE: FALSE)

#define HM_POP_ALLOWED(stack)												\
	((stack->top > -1)? TRUE: FALSE)


/***************************************************************************/
/* Timer functions														   */
/***************************************************************************/
HM_TIMER_CB * hm_timer_create(uint32_t, uint32_t , HM_TIMER_CALLBACK *,void * );
int32_t hm_timer_start(HM_TIMER_CB *);
int32_t hm_timer_modify(HM_TIMER_CB *, uint32_t );
int32_t hm_timer_stop(HM_TIMER_CB *);
VOID hm_timer_delete(HM_TIMER_CB *);


#define HM_TIMER_CREATE(PERIOD, REPEATING, CALLBACK, PARENT) 				\
		hm_timer_create(PERIOD, REPEATING, CALLBACK, PARENT)
#define HM_TIMER_START(CB) hm_timer_start(CB)
#define HM_TIMER_MODIFY(CB, NEW_PERIOD) hm_timer_modify(CB, NEW_PERIOD);
#define HM_TIMER_STOP(CB) hm_timer_stop(CB)
#define HM_TIMER_DELETE(CB) hm_timer_delete(CB)


/***************************************************************************/
/* Byte Buffer Operations												   */
/***************************************************************************/
#ifdef BIG_ENDIAN
#define HM_PUT_LONG(into, from)                             \
             (into)[0] = (uint8_t)((from) >> 24);           \
             (into)[1] = (uint8_t)(((from) >> 16) & 0xFF);  \
             (into)[2] = (uint8_t)(((from) >> 8)  & 0xFF);  \
             (into)[3] = (uint8_t)((from) & 0xFF)

#define HM_GET_LONG(into, from)                                         \
  (into) = (uint32_t)(((uint32_t)((uint8_t *)(from))[0] << 24) |      \
                    ((uint32_t)((uint8_t *)(from))[1] << 16) |      \
                    ((uint32_t)((uint8_t *)(from))[2] << 8)  |      \
                    ((uint32_t)((uint8_t *)(from))[3]))
#else
//FIXME
#define HM_PUT_LONG(into, from)                             \
             (into)[3] = (uint8_t)((from) >> 24);           \
             (into)[2] = (uint8_t)(((from) >> 16) & 0xFF);  \
             (into)[1] = (uint8_t)(((from) >> 8)  & 0xFF);  \
             (into)[0] = (uint8_t)((from) & 0xFF)

#define HM_GET_LONG(into, from)                                         \
  (into) = (uint32_t)(((uint32_t)((uint8_t *)(from))[0] << 24) |      \
                    ((uint32_t)((uint8_t *)(from))[1] << 16) |      \
                    ((uint32_t)((uint8_t *)(from))[2] << 8)  |      \
                    ((uint32_t)((uint8_t *)(from))[3]))

#endif


#endif /* SRC_HMUTIL_H_ */
