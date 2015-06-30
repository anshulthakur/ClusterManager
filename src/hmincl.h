/*
 * hmincl.h
 *
 *	Single point of includes for Hardware Manager
 *  Created on: 29-Apr-2015
 *      Author: anshul
 */

#ifndef SRC_HMINCL_H_
#define SRC_HMINCL_H_
//GENERIC INCLUDES
/***************************************************************************/
/* defines 'size_t', 'FILE', 'NULL', 'stderr', 'stdin', 'stdout' other 	   */
/* FileOps.																   */
/* Functions like printf(), sprintf(), perror etc.						   */
/* For Input and Output methods basically								   */
/***************************************************************************/
#include <stdio.h>
/***************************************************************************/
/* General Functions													   */
/* Defines NULL, size_t, atoi, atol,strtol, strtoul,calloc, free, malloc,  */
/* exit, system,														   */
/***************************************************************************/
#include <stdlib.h>
/***************************************************************************/
/* Testing and Character Mapping 'isdigit()', 'islower()' etc		       */
/***************************************************************************/
#include <ctype.h>
/***************************************************************************/
/* Assert functions to verify assumptions								   */
/***************************************************************************/
#include <assert.h>
/***************************************************************************/
/* errno variable														   */
/***************************************************************************/
#include <errno.h>
/***************************************************************************/
/* Signal Declarations and Handling										   */
/***************************************************************************/
#include <signal.h>
/***************************************************************************/
/* String Handling														   */
/***************************************************************************/
#include <string.h>
/***************************************************************************/
/* Time variable types: clock_t, time_t, struct tm						   */
/* Methods: asctime(), clock(), ctime(), difftime(), mktime(), time()	   */
/***************************************************************************/
#include <time.h>
/***************************************************************************/
/* Threading and Mutex Functions										   */
/***************************************************************************/
#include <pthread.h>
/***************************************************************************/
/* Semaphores															   */
/***************************************************************************/
#include <semaphore.h>

/***************************************************************************/
/* Parsing CLI Options													   */
/***************************************************************************/
#include <unistd.h>

/***************************************************************************/
/* Socket Layer Includes												   */
/***************************************************************************/
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <fcntl.h>       /* for nonblocking */
#include <netinet/tcp.h>

#include <sys/select.h>
#include <sys/types.h>   /* basic system data types */
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>    /* timeval{} for select() */

#include <netdb.h>		/* AI_PASSIVE and other Macros for getaddrinfo() */

/***************************************************************************/
/* Parsing XML Tree														   */
/***************************************************************************/
#include <libxml/parser.h>
#include <libxml/tree.h>

/***************************************************************************/
/* Module includes														   */
/* A rough guideline to includes:										   */
/* First include what is external (shared), like interfaces so that they   */
/* inclusive. We don't want interface headers to be dependent on internal  */
/* definitions.	They must be fully all-inclusive						   */
/*																		   */
/* Custom Datatypes for ease of typing. [Optional]						   */
/*																		   */
/* Now, Macros be declared which may or may not use the Structure defs.	   */
/* These are Constants replaced at compile time. 						   */
/*																		   */
/* Then Structure definitions, to declare all composite structures from    */
/* standard types.														   */
/*																		   */
/* Now, the global variables (extern type) which the module will use.	   */
/*																		   */
/* Function Declarations												   */
/*																		   */
/* Internal APIs, if any.												   */
/***************************************************************************/
#include <hmnodeif.h>
#include <hmpeerif.h>
#include <hmdef.h>
#include <hmstrc.h>
#include <hmutil.h>
#include <hmfunc.h>
#include <hmglob.h>
#include <hmlog.h>

#endif /* SRC_HMINCL_H_ */
