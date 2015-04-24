High Availability Design Details
================================

1. HM Start
    * Parse config into memory data structures
        
        ```
            <!--HM's own information -->
            <meta>
                <service_groups>
                    <service_group>
                        <index>0</index>
                        <name>All</name>                 
                   </service_group>            
                   <service_group>
                        <index>1</index>
                        <name>Main System Manager</name>                 
                   </service_group>
                   <service_group>
                        <index>2</index>
                        <name>Control Plane</name>                 
                   </service_group>
                   <service_group>
                        <index>3</index>
                        <name>Data Plane Controller Card Entity</name>                 
                   </service_group>
                   <service_group>
                        <index>4</index>
                        <name>Data Plane Line Card Entity</name>                 
                   </service_group>                                                
                </service_groups>
            </meta>
            
            <hm_instance_info>
                <index>1</index>
                <heartbeat scope="node">
                    <period resolution="ms">5000</period>
                    <threshold>3</threshold>
                </heartbeat>
                <heartbeat scope="cluster">
                    <period resolution="ms">10000</period>
                    <threshold>1</threshold>
                </heartbeat>
                <address type="local">
                    <ip version="4" type="tcp">127.0.0.1</ip>
                    <port>4999</port>
                </address>
                <address type="local">
                    <ip version="4" type="udp">127.0.0.1</ip>
                    <port>5496</port>
                </address>
                <address type="local">
                    <ip version="4" type="mcast">127.0.0.1</ip>
                    <port>4936</port>
                    <group>3</group>
                </address>
                
                <address type="remote">
                    <ip version="4" type="tcp">192.168.0.3</ip>
                    <port>4999</port>
                </address>
            </hm_instance_info>  
              
            <!-- Nodes Capability information based on which HM creates and invokes the CLI command and also communicates with other HMs too -->
            
            <nodes>
                <node>
                    <index>1</index>
                    <name>nbase-stub</name>
                    <role>active</role>
                    <group>1</group>
                    <subscriptions>
                        <subscription type="group">0</subscription>                
                    </subscriptions>
                </node>
                <node>
                    <index>2</index>
                    <name>cDotCPSrvr</name>
                    <role>active</role>
                    <group>2</group>
                    <subscriptions>
                        <subscription type="group">1</subscription>                
                        <subscription type="group">3</subscription>
                    </subscriptions>
                </node>
                <node>
                    <index>3</index>
                    <name>cDotDPSrvr</name>
                    <role>active</role>
                    <group>3</group>
                    <subscriptions>
                        <subscription type="group">1</subscription>                
                        <subscription type="group">2</subscription>
                        <subscription type="group">5</subscription>
                    </subscriptions>
                </node>
                <node>
                    <index>4</index>
                    <name>cDotFMSrvr</name>
                    <role>active</role>
                    <group>4</group>
                    <subscriptions>
                        <subscription type="group">1</subscription>                
                    </subscriptions>
                </node>
            </nodes> 
        ```
        
    *  Data Structures may look like:
    
        ```
        typedef struct hm_lqe 
        {
            void *self;
            struct hm_lqe *next;
            struct hm_lqe *prev;
        } HM_LQE;
        
        typedef struct hm_inet_address 
        {
            uint8_t type;
            struct sockaddr_storage address;            
        } HM_INET_ADDRESS;
        
        typedef struct hm_location_cb
        {
            uint8_t index;
            
            HM_INET_ADDRESS tcp_addr; /* TCP Address */
            uint16_t tcp_port;
            
            HM_INET_ADDRESS udp_addr; /* UDP Address */
            uint16_t udp_port;
                        
            HM_INET_ADDRESS mcast_addr; /* Multicast Address */
            uint16_t mcast_port;
            uint16_t mcast_group;
            
            HM_LQE node; /* It can be a node in HM Peer List */            
            HM_LQE *peer_list; /* List of other known peers */
            
            HM_AVLL_TREE node_tree; /* Nodes on this location */
            
        } HM_LOCATION_CB;
        
        typedef struct hm_node_cb
        {
            uint16_t index;
            uint16_t group;
            uint8_t role;
            
            HM_TRANSPORT_CB *transport_cb;
            HM_LOCATION_CB *parent_location_cb;
            
            HM_LQE *subscribers;
            HM_LQE *subscriptions;
            
            HM_AVLL_TREE process_tree;
            HM_AVLL_TREE interface_tree;
            
        } HM_NODE_CB;
        ```
               