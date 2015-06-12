/*
 * tester.c
 *
 *  Created on: 10-Jun-2015
 *      Author: anshul
 */


#include <hmincl.h>

int location_index = 0;
int sock_fd = -1; /* Socket Descriptor */

HM_NODE_INIT_MSG init_msg;

int main(int argc, char **argv)
{
	int ret_val = HM_OK;
	SOCKADDR_IN addr;

	int cmd_opt;
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

	TRACE_INFO(("Sending message!"));
	ret_val = send(sock_fd, (char *)&init_msg, sizeof(init_msg), 0);
	if(ret_val != sizeof(init_msg))
	{
		TRACE_PERROR(("Error sending complete message on socket!"));
		goto EXIT_LABEL;
	}
	TRACE_INFO(("Message sent."));

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
