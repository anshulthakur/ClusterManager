/*
 * hmtprt.c
 *
 *  Transport Functions
 *
 *  Created on: 06-May-2015
 *      Author: anshul
 */
/**
 *  @file hmtprt.c
 *  @brief Hardware Manager Transport Routines
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */
#include <hmincl.h>


/**
 *  @brief Opens a Non-Blocking Socket
 *
 *  @param *res a @t< struct addrinfo @t type of address structure containing parameters of connection
 *  @param **saptr @c SOCKADDR type structure to write the @sockaddr structure into
 *  @param *lenp Length of @p saptr structure (to distinguish @sockaddr_in from @sockaddr_in6
 *
 *  @return Descriptor of socket if successful, -1 otherwise
 */
static int32_t hm_open_socket(struct addrinfo *res, SOCKADDR **saptr, socklen_t *lenp)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t sock_fd = -1;
  int32_t val;
  int32_t option_val;

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();
  TRACE_ASSERT(res != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  do{
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd >= 0)
    {
      TRACE_DETAIL(("Got socket."));
      break;
    }
  }while((res = res->ai_next) != NULL);

  if(res == NULL)
  {
    TRACE_PERROR(("Error opening socket!"));
    sock_fd = -1;
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* Socket Opened. Now set its options                          */
  /***************************************************************************/
  TRACE_DETAIL(("Setting the socket to non-blocking mode"));
  val = fcntl(sock_fd, F_GETFL, 0);
  val = fcntl(sock_fd, F_SETFL, val | O_NONBLOCK);

  if (val == -1)
  {
    TRACE_PERROR(("Error setting socket to non-blocking mode"));
    close(sock_fd);
    sock_fd = -1;
    goto EXIT_LABEL;
  } //end if val == -1

  /***************************************************************************/
  /* Allow the descriptors to be reusable                     */
  /***************************************************************************/
    option_val = (int)TRUE;

  /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
    TRACE_DETAIL(("Setting socket descriptor as reusable"));
  val = setsockopt(sock_fd, SOL_SOCKET,  SO_REUSEADDR,
                     (char *)&option_val, sizeof(option_val));
  if (val < 0)
  {
    TRACE_PERROR(("Error settting socket descriptor to reusable"));
    close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Add the descriptor to global descriptor set                 */
  /***************************************************************************/
  if(max_fd < sock_fd)
  {
    TRACE_DETAIL(("Update maximum socket descriptor value to %d", sock_fd));
    max_fd = sock_fd;
  }
  TRACE_DETAIL(("Add FD to set"));
  FD_SET(sock_fd, &hm_tprt_conn_set);

  *saptr = malloc(res->ai_addrlen);
  if(*saptr == NULL)
  {
    TRACE_ERROR(("Error allocating memory for address structure."));
    close(sock_fd);
    sock_fd = -1;
    goto EXIT_LABEL;
  }
  memcpy(*saptr, res->ai_addr, res->ai_addrlen);
  *lenp = res->ai_addrlen;

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return sock_fd;
}/* hm_open_socket */


/**
 *  @brief Accepts the connection into a SOCKET_CB and returns the CB
 *
 *  @param sock_fd Socket Descriptor
 *  @return Socket Control Block (#HM_SOCKET_CB) if successful, @c NULL otherwise
 */
HM_SOCKET_CB * hm_tprt_accept_connection(int32_t sock_fd)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_SOCKET_CB *sock_cb =NULL;
  uint32_t client_len = sizeof(SOCKADDR);
#ifdef I_WANT_TO_DEBUG
  char address[128];
  HM_SOCKADDR_UNION *addr = NULL;
#endif
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(sock_fd > 0);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  sock_cb = hm_alloc_sock_cb();
  if(sock_cb == NULL)
  {
    TRACE_ERROR(("Error allocating resources for incoming connection request"));
    goto EXIT_LABEL;
  }
  TRACE_DETAIL(("Socket: %d", sock_fd));
  sock_cb->sock_fd = accept(sock_fd,
      (SOCKADDR *)&(sock_cb->addr), (socklen_t *)&client_len);
  if (sock_cb->sock_fd < 0)
  {
    if (errno != EWOULDBLOCK)
    {
      TRACE_PERROR(("Failed to accept local connection."));
      hm_free_sock_cb(sock_cb);
      sock_cb = NULL;
      goto EXIT_LABEL;
    }
    TRACE_WARN(("No new connection requests"));
  }
#ifdef I_WANT_TO_DEBUG
  /***************************************************************************/
  /* Accepted Connection. Its parameters are enumerated.             */
  /***************************************************************************/
  addr = (HM_SOCKADDR_UNION *)&sock_cb->addr;
  TRACE_INFO(("New Connection from %s:%d",
      inet_ntop(AF_INET, &addr->in_addr.sin_addr, address, client_len),
      ntohs(addr->in_addr.sin_port)));
#endif
  /***************************************************************************/
  /* Add the descriptor to global descriptor set                 */
  /***************************************************************************/
  TRACE_DETAIL(("Add FD to set"));
  FD_SET(sock_cb->sock_fd, &hm_tprt_conn_set);
  if(max_fd < sock_cb->sock_fd)
  {
    TRACE_DETAIL(("Update maximum socket descriptor value to %d", sock_cb->sock_fd));
    max_fd = sock_cb->sock_fd;
  }
  /***************************************************************************/
  /* We're going to INIT state. Connect has been received, but nothing else  */
  /* has happened. Chances are, it has not been mapped on to a Transport CB  */
  /***************************************************************************/
  sock_cb->conn_state = HM_TPRT_CONN_INIT;

  sock_cb->sock_type = HM_TRANSPORT_SOCK_TYPE_TCP;
  TRACE_DETAIL(("Connection Accepted on Socket %d. Wait for Messages.",
                          sock_cb->sock_fd));
  HM_INSERT_BEFORE(LOCAL.conn_list, sock_cb->node);

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return (sock_cb);
}/* hm_tprt_accept_connection */


/**
 *  @brief Creates a non-blocking socket and returns its descriptor.
 *
 *  @param conn_type Type of connection to be opened
 *  @param *params Structure containing paramters of request for destination.
 *
 *  @return Socket CB (#HM_SOCKET_CB) of the connection opened, else @c NULL
 */
HM_SOCKET_CB * hm_tprt_open_connection(uint32_t conn_type, void * params )
{
  /***************************************************************************/
  /* Descriptor of the socket created. Default value denotes error condition */
  /***************************************************************************/
  int32_t sock_fd = -1;
  int32_t val;
  int32_t option_val = 1;

  socklen_t length;

  HM_SOCKET_CB *sock_cb = NULL;

  HM_INET_ADDRESS *address = params;
  struct ip_mreq mreq;
  char mcast_addr[129];
  int32_t mcast_group = HM_MCAST_BASE_ADDRESS; /* Base value */

  char target[128], service[128];

  int32_t ret_val = HM_OK;

  struct addrinfo hints, *res, *ressave;
  HM_SOCKADDR_UNION *mcast_cast = NULL, *addr = NULL;

  TRACE_ENTRY();

  TRACE_ASSERT(params != NULL);
  /***************************************************************************/
  /* Create a socket control block                       */
  /***************************************************************************/
  sock_cb = hm_alloc_sock_cb();
  if(sock_cb == NULL)
  {
    TRACE_ERROR(("Resource allocation failed for socket control block"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* Depending on the connection type, determine the sockaddr strcuture     */
  switch(conn_type)
  {
  case HM_TRANSPORT_TCP_LISTEN:
    TRACE_INFO(("IPv4 Listen Socket"));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
    break;

  case HM_TRANSPORT_TCP_OUT:
    TRACE_INFO(("IPv4 Outgoing Socket"));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICHOST|AI_NUMERICSERV;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
    break;

  case HM_TRANSPORT_TCP_IN:
    TRACE_INFO(("IPv4 Incoming Socket"));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    break;

  case HM_TRANSPORT_UDP:
    TRACE_INFO(("IPv4 UDP Socket"));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
    break;

  case HM_TRANSPORT_MCAST:
    TRACE_INFO(("IPv4 Multicast Socket"));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
    break;

  case HM_TRANSPORT_SCTP:
    TRACE_INFO(("IPv4 SCTP Socket"));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_SCTP;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN *)params)->sin_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN *)params)->sin_port));
    break;

  case HM_TRANSPORT_TCP_IPv6_LISTEN:
    TRACE_INFO(("IPv6 Listen Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
    break;

  case HM_TRANSPORT_TCP_IPv6_OUT:
    TRACE_INFO(("IPv6 I/O Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
    break;

  case HM_TRANSPORT_TCP_IPv6_IN:
    TRACE_INFO(("IPv6 I/O Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    break;

  case HM_TRANSPORT_UDP_IPv6:
    TRACE_INFO(("IPv6 UDP Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
    break;

  case HM_TRANSPORT_MCAST_IPv6:
    TRACE_INFO(("IPv6 Multicast Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
    break;

  case HM_TRANSPORT_SCTP_IPv6:
    TRACE_INFO(("IPv6 SCTP Socket"));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    inet_ntop(hints.ai_family, &((SOCKADDR_IN6 *)params)->sin6_addr, target, sizeof(target));
    snprintf(service, sizeof(service), "%d", ntohs(((SOCKADDR_IN6 *)params)->sin6_port));
    break;

  default:
    TRACE_ERROR(("Unknown connection Type %d", conn_type));
    TRACE_ASSERT(0==1);
    goto EXIT_LABEL;
  }

  switch(conn_type)
  {
  case HM_TRANSPORT_TCP_IN:
  case HM_TRANSPORT_TCP_IPv6_IN:
    TRACE_INFO(("Accept incoming connection"));
    goto EXIT_LABEL;

  default:
    TRACE_INFO(("One of Outgoing Connections Type %d", conn_type));
  }

  /***************************************************************************/
  /* If unsatisfactory, this code chunk may be replaced by a more elaborate  */
  /* memset(0) and filling of sin_addr structures.               */
  /***************************************************************************/
  TRACE_DETAIL(("AI_FAMILY: %d", hints.ai_family));
  TRACE_DETAIL(("Target: %s:%s", target, service));
  if((conn_type != HM_TRANSPORT_MCAST)&&(conn_type != HM_TRANSPORT_MCAST_IPv6))
  {
    TRACE_DETAIL(("Unicast Address"));
    if((ret_val = getaddrinfo(target, service, &hints, &res)) !=0)
    {
      TRACE_GAI_ERROR(("Error getting information on target %s:%s", target, service),ret_val);
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    ressave = res;
    /***************************************************************************/
    /* Right now, we are only expecting a single address in response.       */
    /* Set appropriate socket options depending on the type of socket.       */
    /***************************************************************************/
    TRACE_DETAIL(("Opening Socket"));
    sock_fd = hm_open_socket(res, (SOCKADDR **)&addr, &length);
  }
  else
  {
    TRACE_DETAIL(("Multicast Address"));
    if((ret_val = getaddrinfo(NULL, service, &hints, &res)) !=0)
    {
      TRACE_GAI_ERROR(("Error getting information on target %s:%s", target, service),ret_val);
      ret_val = HM_ERR;
      goto EXIT_LABEL;
    }
    TRACE_DETAIL(("Opening Socket"));
    sock_fd = hm_open_socket(res, (SOCKADDR **)&addr, &length);
  }
  if(sock_fd == -1)
  {
    TRACE_ERROR(("Error opening socket"));
    ret_val = HM_ERR;
    goto EXIT_LABEL;
  }

  TRACE_DETAIL(("Socket opened: %d", sock_fd));
  TRACE_ASSERT(addr != NULL);
  /***************************************************************************/
  /* Other specific options and processing.                   */
  /***************************************************************************/
  switch (conn_type)
  {
  case HM_TRANSPORT_TCP_LISTEN:
    /***************************************************************************/
    /* Set SOCKOPTS to TCP_NODELAY to disable Nagle's algorithm           */
    /***************************************************************************/
     val =   setsockopt(sock_fd,
                          IPPROTO_TCP,
                          TCP_NODELAY,
                          &option_val,
                          sizeof(option_val));
     if(val == -1)
     {
       TRACE_PERROR(("Error Setting TCP_NODELAY"));
       close(sock_fd);
       sock_fd = -1;
       goto EXIT_LABEL;
     }
    /***************************************************************************/
    /* Bind to address                               */
    /***************************************************************************/
    TRACE_DETAIL(("Binding to address"));
    if(bind(sock_fd, &addr->sock_addr, length) != 0)
    {
      TRACE_PERROR(("Error binding to port"));
      close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
    }

    TRACE_DETAIL(("Start Listen on Port"));
    if(listen(sock_fd, HM_MAX_PENDING_CONNECT_REQ) != 0)
    {
      TRACE_PERROR(("Listen on socket %d failed.", sock_fd));
      close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
    }
    sock_cb->sock_type = HM_TRANSPORT_SOCK_TYPE_TCP;
    break;

  case HM_TRANSPORT_TCP_OUT:
    TRACE_INFO(("Trying to connect"));
    if((val = connect(sock_fd, &addr->sock_addr, length))!=0)
    {
      if(errno == EINPROGRESS)
      {
        TRACE_DETAIL(("Connect will complete asynchronously."));
        FD_SET(sock_fd, &hm_tprt_write_set);
        sock_cb->conn_state = HM_TPRT_CONN_INIT;
      }
      else
      {
        TRACE_PERROR(("Connect failed on socket %d", sock_fd));
        FD_CLR(sock_cb->sock_fd, &hm_tprt_write_set);
        close(sock_fd);
        sock_fd = -1;
        goto EXIT_LABEL;
      }
    }
    else
    {
      TRACE_DETAIL(("Connect succeeded!"));
      /* Directly move connection to active state */
      sock_cb->conn_state = HM_TPRT_CONN_ACTIVE;
    }
    //TODO: Add socket to write_set and poll on write_set too.
    sock_cb->sock_type = HM_TRANSPORT_SOCK_TYPE_TCP;
    FD_SET(sock_fd, &hm_tprt_write_set);
    break;

  case HM_TRANSPORT_UDP:
    /***************************************************************************/
    /* Bind to address                               */
    /***************************************************************************/
    TRACE_DETAIL(("Binding to address"));
    if(bind(sock_fd, &addr->sock_addr, length) != 0)
    {
      TRACE_PERROR(("Error binding to port"));
      close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
    }
    sock_cb->sock_type = HM_TRANSPORT_SOCK_TYPE_UDP;
        break;

  case HM_TRANSPORT_MCAST:
    /***************************************************************************/
    /* Bind to address                               */
    /***************************************************************************/
    if(bind(sock_fd, &addr->sock_addr, length) != 0)
    {
      TRACE_PERROR(("Error binding to port"));
      close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
    }
    sock_cb->sock_type = HM_TRANSPORT_SOCK_TYPE_UDP;

#ifdef I_WANT_TO_DEBUG
    {
    char address_value[128];
    HM_SOCKADDR_UNION addr;
    socklen_t addrlen = sizeof(addr);

    getsockname(sock_fd, &addr.sock_addr,  &addrlen);
    inet_ntop(addr.in_addr.sin_family,
          &addr.in_addr.sin_addr,
            address_value, sizeof(address_value));
    TRACE_INFO(("Address value: %s:%d",address_value,
                  ntohs(addr.in_addr.sin_port)));
    TRACE_INFO(("Family: %d", addr.in_addr.sin_family));
    }
#endif

    snprintf(mcast_addr, sizeof(mcast_addr), "224.0.0.%d", mcast_group+address->mcast_group);
    TRACE_DETAIL(("Multicast Group Address: %s", mcast_addr));
    /***************************************************************************/
    /* Convert to network representation.                     */
    /***************************************************************************/
    inet_pton(res->ai_family, mcast_addr, &mreq.imr_multiaddr.s_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if(setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
    {
      TRACE_PERROR(("Error joining Multicast Group"));
      close(sock_fd);
      sock_fd = -1;
      goto EXIT_LABEL;
    }

    TRACE_INFO(("Joined Multicast Group %d.", address->mcast_group));
    TRACE_INFO(("Update Global Multicast Address Destination"));
    /***************************************************************************/
    /* Also update the Multicast sending address in LOCAL             */
    /* This is a quickfix.                             */
    /* FIXME                                      */
    /***************************************************************************/
    TRACE_ASSERT(LOCAL.mcast_addr != NULL);
    LOCAL.mcast_addr->address.mcast_group = address->mcast_group;
    //IPv4 Specific
    mcast_cast = (HM_SOCKADDR_UNION *)&LOCAL.mcast_addr->address.address;
    inet_pton(res->ai_family, mcast_addr,
        ((SOCKADDR_IN *)&mcast_cast->in_addr.sin_addr));
    mcast_cast->in_addr.sin_port = ((SOCKADDR_IN*)res->ai_addr)->sin_port;
    mcast_cast->in_addr.sin_family = res->ai_family;

#ifdef I_WANT_TO_DEBUG
    {
    char address_value[128];
    inet_ntop(res->ai_family,
        ((SOCKADDR_IN *)&mcast_cast->in_addr.sin_addr),
        address_value, sizeof(address_value));
    TRACE_INFO(("Multicast Address value: %s:%d",address_value,
                  ntohs(mcast_cast->in_addr.sin_port)));
    TRACE_INFO(("Family: %d", res->ai_family));
    }
#endif

    /***************************************************************************/
    /* Do not loopback the packets                         */
    /***************************************************************************/
    /*
    {
      TRACE_DETAIL(("Setting Loopback to off."));
      u_char flag;
      flag = 0;

      if(setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) == -1)
      {
        TRACE_PERROR(("Error setting loopback option on Multicast socket"));
      }
    }
    */
    LOCAL.mcast_addr->sock_cb = sock_cb;
    LOCAL.mcast_addr->location_cb = &LOCAL.local_location_cb;

    break;

  default:
    TRACE_WARN(("Unknown Connection type"));
    break;
  }
  /***************************************************************************/
  /* All went well. Set the sock_fd as that of sock_cb             */
  /***************************************************************************/
  sock_cb->sock_fd = sock_fd;

  memcpy(&sock_cb->addr, &address->address, sizeof(SOCKADDR));
  /***************************************************************************/
  /* Insert in Connection List                         */
  /***************************************************************************/
  HM_INSERT_BEFORE(LOCAL.conn_list, sock_cb->node);

EXIT_LABEL:
  if (ret_val == HM_OK)
  {
    /***************************************************************************/
    /* Free the address structures that were allocated in getaddrinfo in kernel*/
    /* NOTE: We are not saving the struct sockaddr, we will fetch it later      */
    /* using getaddrinfo()                             */
    /***************************************************************************/
    freeaddrinfo(ressave);
    /***************************************************************************/
    /* Add the socket descriptor to the global FD set.               */
    /***************************************************************************/
    //FIXME
  }
  else if(ret_val == HM_ERR)
  {
    hm_free_sock_cb(sock_cb);
    sock_cb = NULL;
  }

  TRACE_EXIT();
  return (sock_cb);
} /* hm_tprt_open_connection */


/**
 *  @brief Send Message on Socket
 *
 *  @param *ip a @t< struct sockaddr @t type of structure to which data must be sent.
 *  @param sock_fd  Socket Descriptor on which to send data
 *  @param sock_type Type of socket to determine sending mechanism
 *  @param *msg_buffer A byte buffer to be sent
 *  @param length   Length of data to be sent from @p msg_buffer
 *
 *  @return Total number of bytes sent
 */
int32_t hm_tprt_send_on_socket(struct sockaddr* ip,int32_t sock_fd,
            uint32_t sock_type,BYTE *msg_buffer, uint32_t length )
{
  /***************************************************************************/
  /* Local variables                               */
  /***************************************************************************/
  int32_t total_bytes_sent=0;
  int32_t bytes_sent=0;
  int32_t success = FALSE;
  BYTE *buf = NULL;
  int32_t os_error;
#ifdef I_WANT_TO_DEBUG
  char temp[129];
#endif

  TRACE_ENTRY();

  TRACE_DETAIL(("Socket type %d", sock_type));
  TRACE_DETAIL(("Attempt to send %d bytes of data on socket %d",
                    length, sock_fd));

  buf = msg_buffer;

  do
  {
    if((sock_type== HM_TRANSPORT_TCP_IN) || (sock_type==HM_TRANSPORT_TCP_OUT))
    {
      TRACE_DETAIL(("data to be sent on TCP connection "));
      bytes_sent = send(sock_fd,
                buf,
                (length - bytes_sent),
                0);

    }

    else if(sock_type==HM_TRANSPORT_UDP)
    {
      TRACE_DETAIL(("Data to be sent on UDP Ucast connection "));
      bytes_sent = sendto(sock_fd,
                                 buf,
                                 (length - bytes_sent),
                                 0,
                                 ip,
                                 sizeof(struct sockaddr));

    }

    else if(sock_type==HM_TRANSPORT_MCAST)
    {
      TRACE_DETAIL(("Data to be sent on UDP Mcast connection "));
#ifdef I_WANT_TO_DEBUG
    {
    char address_value[128];
    HM_SOCKADDR_UNION addr;
    socklen_t addrlen = sizeof(addr);

    getsockname(sock_fd, &addr.sock_addr,  &addrlen);
    inet_ntop(addr.in_addr.sin_family,
          &addr.in_addr.sin_addr,
            address_value, sizeof(address_value));
    TRACE_INFO(("Address value: %s:%d",address_value,
                  ntohs(addr.in_addr.sin_port)));
    TRACE_INFO(("Family: %d", addr.in_addr.sin_family));
    }

    TRACE_DETAIL(("Send to %s:%d",
        inet_ntop(((SOCKADDR_IN *)ip)->sin_family,
          &((SOCKADDR_IN *)ip)->sin_addr,
          temp, sizeof(temp)),
          ntohs(((SOCKADDR_IN *)ip)->sin_port)));
    TRACE_DETAIL(("Family: %d",((SOCKADDR_IN *)ip)->sin_family));
#endif

      bytes_sent = sendto(sock_fd,
                                 buf,
                                 (length - bytes_sent),
                                 0,
                                 ip,
                                 sizeof(struct sockaddr));

        }

    if(bytes_sent == length)
    {
      TRACE_DETAIL(("Message sent in full"));
      buf = NULL;
      success = TRUE;
      break;
    }
    else if (bytes_sent == -1)
    {
      /***********************************************************************/
      /* An error was returned - check for retryable socket error values.    */
      /***********************************************************************/
      os_error = errno;
      TRACE_PERROR(("Send failed on socket %d.",sock_fd));

      if ((os_error == EWOULDBLOCK) ||
                (os_error == ENOMEM) ||
                (os_error == ENOSR))
      {
        /*********************************************************************/
          /* This seems to be a flow control condition - clear the bytes sent  */
          /* value to show that no data was written.                           */
          /*********************************************************************/
          TRACE_WARN(("Resource shortage - try again"));
          bytes_sent = 0;
      }
      else if (os_error == EINTR)
      {
        TRACE_WARN(("Send interrupted - loop round again"));
          bytes_sent = 0;
      }
      else
      {
        /*********************************************************************/
          /* Socket failed so work source needs to be unregistered.            */
          /*********************************************************************/
          TRACE_ERROR(("Socket failed"));
          success = HM_ERR;
          break;
      }
    }
    /***************************************************************************/
    /* Advance current pointer to remaining part of data             */
    /***************************************************************************/
    buf += bytes_sent;
    total_bytes_sent +=bytes_sent;
  }while(total_bytes_sent< length);

  if(total_bytes_sent == length)
  {
    TRACE_DETAIL(("Message sent in full"));
    success = TRUE;
  }

  TRACE_EXIT();
  return (success);
} /* hm_tprt_send_on_socket */


/**
 *  @brief Receives data from socket
 *
 *  @param sock_fd Socket Descriptor on which to receive data
 *  @param sock_type Type of socket to determine method of receiving
 *  @param *msg_buffer Message Buffer into which data must be written
 *  @param length  Length of buffer
 *  @param **src_addr A @c SOCKADDR type of structure where incoming connection
 *      parameters will be written if connection is Datagram based.
 *
 *  @return Total bytes read from the socket
 */
int32_t hm_tprt_recv_on_socket(uint32_t sock_fd , uint32_t sock_type,
              BYTE * msg_buffer, uint32_t length, SOCKADDR **src_addr)
{
  /***************************************************************************/
  /* Local variables                               */
  /***************************************************************************/
  int32_t total_bytes_rcvd=0;
  uint32_t bytes_rcvd=0;
  int32_t os_error;
  int32_t op_complete = FALSE;
  BYTE *buf = msg_buffer;

  SOCKADDR_IN * ip_addr = NULL;
  extern fd_set hm_tprt_conn_set;

  socklen_t len = sizeof(SOCKADDR);

  TRACE_ENTRY();

  TRACE_ASSERT(msg_buffer != NULL);

    /***************************************************************************/
  /* Now try to receive data                           */
  /***************************************************************************/
  if(src_addr != NULL)
  {
    *src_addr = NULL;
  }
  do
  {
    TRACE_DETAIL(("Try to receive %d bytes on Socket %d", (length - bytes_rcvd), sock_fd));

    if(sock_type==HM_TRANSPORT_SOCK_TYPE_TCP)
    {
      TRACE_DETAIL(("TCP Socket"));
      bytes_rcvd = recv(sock_fd,
                buf,
                (length - bytes_rcvd),
                0);
    }

    else if(sock_type==HM_TRANSPORT_SOCK_TYPE_UDP)
    {
      TRACE_DETAIL(("UDP Socket"));
      ip_addr = (SOCKADDR_IN*)malloc(sizeof(SOCKADDR_IN));
      if(ip_addr == NULL)
      {
        TRACE_ERROR(("Error allocating memory for incoming Address"));
        op_complete = TRUE;
          bytes_rcvd = 0;
          break;
      }
      bytes_rcvd = recvfrom(sock_fd,
                                      buf,
                                      (length - bytes_rcvd),
                                       0,
                                       (struct sockaddr *)ip_addr  ,
                                       &len);
       /***************************************************************************/
       /* Do we need to fetch the sender IP information too, from the socket?     */
       /* or will they tell it themselves?                    */
       /* In some upper layer structure?                      */
       /***************************************************************************/
       //FIXME: So far, they're not telling, so we need to get it here.
      {
        char net_addr[128];
        int32_t length = 128;
        inet_ntop(AF_INET, &ip_addr->sin_addr, net_addr, length);
        TRACE_DETAIL(("%s:%d", net_addr, ntohs(ip_addr->sin_port)));

      }
    }
    if(bytes_rcvd == length)
    {
      TRACE_DETAIL(("Message received in full"));
      buf = NULL;
      op_complete = TRUE;
      total_bytes_rcvd +=bytes_rcvd;
      break;
    }
    else if (bytes_rcvd == -1)
    {
      /***********************************************************************/
      /* An error was returned - check for retryable socket error values.    */
      /***********************************************************************/
      TRACE_PERROR(("Recv failed on socket"));
      os_error = errno;
      if ((os_error == EWOULDBLOCK))
      {
        /*********************************************************************/
          /* This seems to be a flow control condition - clear the bytes sent  */
          /* value to show that no data was written.                           */
          /*********************************************************************/
        TRACE_WARN(("Resource shortage - try again"));
          bytes_rcvd = 0;
       }
      else if (os_error == EINTR)
      {
        TRACE_WARN(("Receive interrupted - loop round again"));
          bytes_rcvd = 0;
      }
      else
      {
        /*********************************************************************/
          /* Socket failed so work source needs to be unregistered.            */
          /*********************************************************************/
        TRACE_ERROR(("Socket failed"));
        FD_CLR(sock_fd, &hm_tprt_conn_set);
          op_complete = TRUE;
          bytes_rcvd = 0;
          break;
      }
    }
    else if(bytes_rcvd == 0)
    {
      TRACE_WARN(("The peer has disconnected"));
      op_complete = TRUE;
      FD_CLR(sock_fd, &hm_tprt_conn_set);
      total_bytes_rcvd = HM_ERR;
      break;
    }
    else
    {
      TRACE_INFO(("%d bytes received. Returning.", bytes_rcvd));
      op_complete = TRUE;
      break;
    }
    /***************************************************************************/
    /* Advance current pointer to remaining part of data             */
    /***************************************************************************/
    buf += bytes_rcvd;
    total_bytes_rcvd +=bytes_rcvd;
  }while(total_bytes_rcvd< length);

  if(op_complete == TRUE)
  {
    if(total_bytes_rcvd == length)
    {
      TRACE_DETAIL(("Message received in full"));
      buf = NULL;
    }
    else
    {
      TRACE_WARN(("Some error happened"));
      buf = NULL;
      total_bytes_rcvd = bytes_rcvd;
    }
  }
#ifdef I_WANT_TO_DEBUG
  if(ip_addr != NULL)
  {
    if(src_addr== NULL)
    {
      TRACE_WARN((
      "UDP/Multicast UDP port was specified but address to fill sender addr was not given"
          ));
    }
    TRACE_ASSERT(src_addr!=NULL);
  }
#endif
  if(src_addr != NULL)
  {
    *src_addr = (SOCKADDR *)ip_addr;
  }
  TRACE_EXIT();
  return (total_bytes_rcvd);
} /* hm_tprt_recv_on_socket */


/**
 *  @brief Closes connection on the given transport CB
 *
 *  @param *tprt_cb Transport Control Block whose connection is to be closed (#HM_TRANSPORT_CB)
 *  @return #HM_OK if successful, else #HM_ERR
 */
int32_t hm_tprt_close_connection(HM_TRANSPORT_CB *tprt_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  int32_t ret_val = HM_OK;
  HM_MSG *msg = NULL;

  HM_LIST_BLOCK *block = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(tprt_cb != NULL);

  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/

  /***************************************************************************/
  /* Empty the outgoing buffers queue. Don't try to send them, just drop.     */
  /***************************************************************************/
  TRACE_DETAIL(("Emptying pending queue."));
  for(block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(tprt_cb->pending);
      block != NULL;
    block = (HM_LIST_BLOCK *)HM_NEXT_IN_LIST(tprt_cb->pending))
  {
    msg = (HM_MSG *)block->target;
    /***************************************************************************/
    /* Remove from list.                             */
    /***************************************************************************/
    HM_REMOVE_FROM_LIST(block->node);
    TRACE_DETAIL(("Freeing Message!"));
    hm_free_buffer(msg);
    free(block);
    block = NULL;
  }

  if(tprt_cb->in_buffer != NULL)
  {
    TRACE_DETAIL(("Transport has data in its input buffers. Freeing!"));
    hm_free_buffer((HM_MSG *)tprt_cb->in_buffer);
    tprt_cb->in_buffer = NULL;
  }
  /***************************************************************************/
  /* Transport Types may differ. Default is Socket Based Transport. So, we're*/
  /* currently only implementing the default case.               */
  /***************************************************************************/
  switch(tprt_cb->type)
  {
  default:
    /***************************************************************************/
    /* It is possible that the socket connection was torn down before, or never*/
    /* existed in the first place.                         */
    /***************************************************************************/
    if(tprt_cb->sock_cb != NULL)
    {
      TRACE_DETAIL(("Closing socket connections"));
      hm_close_sock_connection(tprt_cb->sock_cb);
      tprt_cb->sock_cb = NULL;
    }
  }

  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return(ret_val);
}/* hm_tprt_close_connection */


/**
 *  @brief Closes a socket connection and releases its resources
 *
 *  @param *sock_cb Socket Control Block which must be closed (#HM_SOCKET_CB)
 *  @return @c void
 */
void  hm_close_sock_connection(HM_SOCKET_CB *sock_cb)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/

  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(sock_cb != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  if(sock_cb->sock_fd > 0)
  {
    TRACE_INFO(("Closing socket"));
    close(sock_cb->sock_fd);
    sock_cb->sock_fd = -1;
  }
  HM_REMOVE_FROM_LIST(sock_cb->node);
  sock_cb->tprt_cb = NULL;

  hm_free_sock_cb(sock_cb);
  sock_cb = NULL;
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return;
}/* hm_close_sock_connection */
