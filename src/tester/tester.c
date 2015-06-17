/*
 * tester.c
 *
 *  Created on: 10-Jun-2015
 *      Author: anshul
 */


#include <hmincl.h>

int32_t location_index = 0;
int32_t sock_fd = -1; /* Socket Descriptor */

HM_NODE_INIT_MSG init_msg;
HM_REGISTER_MSG reg_msg;
HM_KEEPALIVE_MSG keepalive_msg;
HM_PROCESS_UPDATE_MSG proc_msg;
HM_REGISTER_TLV_CB tlv;
HM_UNREGISTER_MSG unreg_msg;
HM_NOTIFICATION_MSG notify_msg;
HM_HA_STATUS_UPDATE_MSG hm_msg;


/* http://stackoverflow.com/questions/9571738/picking-random-number-between-two-points-in-c */
static unsigned int random_num(unsigned int min, unsigned int max)
{
    int32_t r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

/***************************************************************************/
/* MAIN FUNCTION														   */
/***************************************************************************/
int32_t main(int32_t argc, char **argv)
{
	int32_t ret_val = HM_OK;
	SOCKADDR_IN addr;

	int32_t cmd_opt;
	/* Randomly select a group for subscription */
	int32_t subs_group = random_num(0, 4);
	int32_t node[2] = {random_num(1,4), random_num(1,4)};

	int32_t pct_type = 0x75010001;
	int32_t pid = 0x00000034;

	extern char *optarg;

	while((cmd_opt = getopt(argc, argv, "l:")) != -1)
	{
		switch(cmd_opt)
		{
		case 'l':
			TRACE_INFO(("Location Index: %s", optarg));
			location_index = atoi(optarg);
			break;
		default:
			printf("\nUsage: %s -l <location_number>", argv[0]);
			break;
		}
	}

	if(location_index == 0)
	{
		TRACE_ERROR(("Did not find Location Index"));
		goto EXIT_LABEL;
	}

	pid = pid | (location_index << 31);
	TRACE_INFO(("PID Assigned: 0x%x", pid));

	/***************************************************************************/
	/* Setup address														   */
	/***************************************************************************/
	memset(&addr, 0, sizeof(SOCKADDR));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(4999);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	/***************************************************************************/
	/* Open Socket															   */
	/***************************************************************************/
	sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock_fd < 0)
	{
		TRACE_PERROR(("Error opening socket"));
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Send connect request													   */
	/***************************************************************************/
	ret_val = connect(sock_fd, (SOCKADDR *)&addr, (socklen_t)sizeof(SOCKADDR));
	if(ret_val < 0)
	{
		TRACE_PERROR(("Error connecting to HM."));
		goto EXIT_LABEL;
	}

	/***************************************************************************/
	/* Connected! Go ahead, send an init									   */
	/***************************************************************************/
	init_msg.hdr.msg_id = 1;
	init_msg.hdr.msg_len = sizeof(init_msg);
	init_msg.hdr.msg_type = HM_MSG_TYPE_INIT;
	init_msg.hdr.request = TRUE;
	init_msg.hdr.response_ok = FALSE;

	init_msg.hardware_num = 0;
	init_msg.index = location_index;
	init_msg.keepalive_period = 1000; /* ms */
	init_msg.location_status = TRUE;
	init_msg.service_group_index = location_index;

	TRACE_INFO(("Sending INIT message!"));
	ret_val = send(sock_fd, (char *)&init_msg, sizeof(init_msg), 0);
	if(ret_val != sizeof(init_msg))
	{
		TRACE_PERROR(("Error sending complete message on socket!"));
		goto EXIT_LABEL;
	}
	TRACE_INFO(("INIT Message sent."));

	ret_val = recv(sock_fd, (char *)&init_msg, sizeof(init_msg), 0);
	if(ret_val != sizeof(init_msg))
	{
		TRACE_WARN(("Partial Message Received!"));
		goto EXIT_LABEL;
	}
	TRACE_INFO(("Message response received"));
	if(init_msg.hdr.response_ok == TRUE)
	{
		TRACE_INFO(("Hardware Index is %d", init_msg.hardware_num));
	}

	//TRACE_INFO(("Send Keepalive"));
	//TODO: LATER

	//Send Process UP Notification
	proc_msg.hdr.msg_id = 1;
	proc_msg.hdr.msg_len = sizeof(proc_msg);
	proc_msg.hdr.msg_type = HM_MSG_TYPE_PROCESS_CREATE;
	proc_msg.hdr.request = TRUE;
	proc_msg.hdr.response_ok = FALSE;

	proc_msg.if_offset = 0;
	snprintf(proc_msg.name, sizeof(proc_msg.name), "TEST");
	proc_msg.pid = pid;
	proc_msg.proc_type = pct_type;

	TRACE_INFO(("Sending PROCESS_CREATED message!"));
	ret_val = send(sock_fd, (char *)&proc_msg, sizeof(proc_msg), 0);
	if(ret_val != sizeof(proc_msg))
	{
		TRACE_PERROR(("Error sending complete message on socket!"));
		goto EXIT_LABEL;
	}
	TRACE_INFO(("PROCESS_CREATED Message sent."));

	ret_val = recv(sock_fd, (char *)&proc_msg, sizeof(proc_msg), 0);
	if(ret_val != sizeof(proc_msg))
	{
		TRACE_WARN(("Partial Message Received!"));
	}
	TRACE_INFO(("Message response received"));

	if(proc_msg.hdr.response_ok == TRUE)
	{
		TRACE_INFO(("Process Create Notification OK"));
	}

	//Send REGISTER for Group
//	TRACE_INFO(("Sending Register for Group %d", subs_group));
	reg_msg.hdr.msg_id = 1;
	reg_msg.hdr.msg_len = sizeof(reg_msg);
	reg_msg.hdr.msg_type = HM_MSG_TYPE_REGISTER;
	reg_msg.hdr.request = TRUE;
	reg_msg.hdr.response_ok = FALSE;

	reg_msg.num_register = subs_group;
	//Receive REGISTER Response

	//Send Unregister

	//Receive Unregister Response

	//Send REGISTER for Nodes
//	TRACE_INFO(("Sending Register for Nodes %d, %d", node[0], node[1]));

	//Receive REGISTER Response

	//Send Unregister

	//Receive Unregister Response



	while(1)
	{
		continue;
	}

EXIT_LABEL:
	if (sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}
	return ret_val;
}
