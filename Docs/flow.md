Flow
====

## Hardware Manager Initialization

* Start HM
* Load init file
* Initialize Core

    * Initialize Logging
    * Initialize Aggregate Nodes Tree
    * Initialize Aggregate Process Tree
    * Initialize Aggregate PID Tree
    * Initialize Aggregate Interface Tree
    * Initialize Active Joins Tree
    * Initialize Broken/Pending Joins Tree
    * Initialize Mutex on Notifications Pending Flag
    * Initialize Notifications Queue  
      
* Initialize Transport layer

    * Set signal handlers for dead sockets
    
* Initialize Hardware Location Layer

    * Initialize Local Location CB    
        * Initialize Transport Location CB for Listen Port        
            * Initialize Socket Connection CB:
                * Set Connection Type
                * Open Socket
                * Set Transport Location CB Pointer
                * Set Node CB to NULL
            * Start listen on TCP port
                
        * Initialize TCB for Unicast Port
            * Initialize Socket Connection CB:        
                * Open UDP Unicast port
        * If Cluster enabled:        
            * Initialize TCB for Multicast Port            
                * Open UDP Multicast port
                * Join Multicast Group
                
    * Do generic hardware location processing    
        * Initialize Nodes Tree
        
    * Parse Node info from init config and for each node:    
        * **Initialize Node Layer**        
            * Initialize Node CB            
                * Set Parent Hardware Location CB Pointer
                * Set Location Index
                * Set Location Group
                * Set Location Name
                * Set role state
                * Initialize TCB to NULL
                * If Hardware Location Index is Local Location Index:                
                    * Initialize node FSM                     
                        * Initialize Node Startup State
                        * Initialize Tick Count to 0
                        * Initialize Connection Keepalive Timer [First time to check for incoming connection. On pop, it must check for TCB being NULL. If Connect request arrives before, it must stop and reset this timer for Keepalive purposes.]
                                        
                * Initialize Local Process Tree
                * Initialize Local Interface Tree
                * Initialize Subscribers List Root [For whom we want notifications]
                * Initialize Subscriptions List Root [Who will be notified about us]
                * Add to Hardware Location CB Tree
                * Add to Aggregate Nodes Tree
                
        * Set up dependencies [both forward and reverse]
    * **Start Peer Discovery Thread**
        * Send Hello on Multicast
        * Start Request Timeout Timers
        * Set state to _WAITING_
    * **Start Notification Dispatcher Thread**
        * Get lock on Notifications Pending Flag
        * Go to Conditioned Sleep on the flag value       

## Node Startup

_Assuming that NBASE creates the Process satisfactorily, I discus only the HM Stub-HM specific function only_

* Open Non-blocking Socket towards HM TCP Port
* Send connect
* Register with NBASE as worksource and a callback function.

## HM `connect()` request arrival

* Transport Layer Calls accepts connection request.
    * Initialize new Socket Control Block
    

## Node connect callback

**On `connect()` callback**        
    *  __MUST be a write event: Signalling Connect OK__
    *  Send *INIT* message

## HM receives INIT

* Transport Layer Fetches Connection CB
* Calls into Hardware Location CB to handle incoming message and connection type.
* HM receives Packet Header to determine Message Type:
    If Connection Type is TCP and message is one of:
    
    * NODE_INIT
    * NODE_CLOSE
    * NODE_KEEPALIVE
    * NODE_UPDATE
    * NODE_SUBSCRIBE
    * NODE_UNSUBSCRIBE
    * PROCESS_CREATE
    * PROCESS_READ
    * PROCESS_UPDATE
    * PROCESS_DELETE
    * PROCESS_SUBSCRIBE
    * PROCESS_UNSUBSCRIBE
    * INTERFACE_SUBSCRIBE
    * INTERFACE_UNSUBSCRIBE    
    
    Pass to higher layer for processing with Node CB set to NULL.
    (We don't have an association yet.)
    
    * Node Management Layer receives rest of message:
        * Determines Location Index from message
        * Determines Group Index from message
        * Finds Node CB in Nodes tree
        * Cancels its Timeout timer
        * Update Node CB pointer in Transport Connection CB.
        * Kicks in Connection FSM
            * Send INIT response: Sends Minimum Keepalive Interval
            * Set state to `waiting`
            * Reset Timeout Timer

## Node receives INIT response

* Callback is invoked.
* Receive complete message and determine type
    * NODE_INIT message response:
        * If return code is OK
        * Send Keepalive message
        * Set timer for sending keepalives based on received value
        * Set connection as ACTIVE
        * Send all Queued messages (If any)  [*HM Stub MUST follow a queuing model on HM-HM Stub interface send*]         

## HM receives KEEPALIVE message

* Transport Layer Fetches Connection CB
* ...
*   Pass to higher layer for processing with Node CB set to proper value.
    (We WILL have an association now.)
    
    * Node Management Layer receives rest of message:
        * Kick Connection FSM                 
            * Reset Timeout Timer
            * Advance state to Active
            * Update location status in Node table
            * Queue notification CB in Notifications queue.
                * Set Notification Type: NODE_ACTIVE
                * Set Notification Value: Node CB pointer (Notify thread will read from subscribers queue. This queue must be lock protected to prevent case of unsubscribe event while fanning out notifications.) 
            * Wake up notifier thread
                * Get lock on section [It actually doesn't need a lock. No one actually needs a lock if we are implementing circular queues. But it does need a timed wait on a variable.]
                * Pop notification from queue
                * Get buffer to send data on socket
                * For each subscriber in node_cb subscriber queue, send message. [Might need some mechanism to allow failed send on few subscribers and retry later. State resides in Notification CB. Maybe implement a delayed queue there.]
                * Reset Flag.
                * Add Timestamp to Node CB [The one that was put in Node Update Message to Peer].
                * Add Timestamp to Hardware Location CB [Timestamp in Node CB must never be newer than that in Hadware Location CB]
                * Queue Update on Peer Nodes Queue
                * Update Aggregate Tables.
                * Go to sleep again.                          
    
            * Send KEEPALIVE message.
            * Set Keepalive timer.

## Node receives KEEPALIVE message

* Callback is invoked
* Receive complete message and determine type
    * NODE_KEEPALIVE message:
        * OK. Do nothing for now.

## New Process is created on Node

* Watcher (ILT) detects new PID
* Prepare PROCESS_CREATE message:
    * Type: PCT_CODE
    * Name: Friendly name registered (if ANY)
    * PID: pid_value
    * Num Interfaces supported: value
    * Interfaces: Linked list of interface ID values
* Add to HM Queue
* Process Queue
    * Send message

## PROCESS_CREATE message received on HM

* Transport Layer Fetches Connection CB
* ...
*   Pass to higher layer for processing with Node CB set to proper value.
    (We WILL have an association now.)         
    
    * Node Management Layer receives rest of message:
        * PROCESS_CREATE message:
            * Pass to Process Management Layer
                * Initialize Process_CB
                * Fill in proper values
                * Add to Processes tree in Node CB
                * Add Notification CB to Notifier Queue
                * Wake Notifier Thread
                    * Get notification type: PROCESS_CREATE
                    * Get notification parameters: PROCESS_CB pointer
                    * Resolve dependency on PCT_TYPE from Aggregate Tables and update pointers here.
                    * Send Notification to all subscribers [If any, for that PCT_TYPE]
                    * Add timestamps and send to Peer Queue also.
                    * Reset Notification Flag
                    * Add Process CB to aggregate processes table
                    * Goto Sleep
                * Pass to interface Management Layer
                    * Initialize Interface CB
                    * Fill in Interface values
                    * Add Interface CB to interfaces queue
                    * Add notification CB to Notifier Queue
                        * Set Notification Type: INTERFACE_ADDED
                    * Wake Notifier Thread
                        * Get Notification Type: INTERFACE_ADDED
                        * Get notification parameters INTERFACE_CB pointer
                        * Resolve dependencies on INTERFACE_TYPE from Aggregate Interfaces table and update pointers here too (in Subscribers and Subscriptions list)
                        * Notify subscribers [If any, for that IF_ID]
                        * Reset Notification Flag.
                        * Goto sleep.

## PROCESS registers for PCT_TYPE

* Sends HM\_SUBSCRIBE\_PROCESS
    * PCT_TYPE
    * Location Index [0 means any]
    * Location Group [0 means any]

* HM Stub receives:
    * Creates HM\_SUBSCRIBE\_PROCESS
    * Send to HM

## HM Receives HM\_SUBSCRIBE\_PROCESS message

* Transport Layer Fetches Connection CB
* ...
*   Pass to higher layer for processing with Node CB set to proper value.
    (We WILL have an association now.)         
    
    * Node Management Layer receives rest of message:
        * PROCESS_SUBSCRIBE message
        * Sets up subscriptions in Aggregate Tables: 
            * If PCT_NODE not present, allocate and add to tree.
            * If PID Tree has entry, setup Notification CB
                * Type: NOTIFY_PROCESS_AVAILABLE
                * PCT_TYPE: value
                * PID: value
                * Location Index: value
                * IP: Value
                * Port: Value
                * Source PID: Subscriber's PID
                
        * Send OK Reply
        * If Notification CB is setup
            * Add Notification CB to Notification Queue.
            * Wake up Notification Thread
                * Process notification Queue

## Node receives HM\_SUBSCRIBE\_PROCESS response

* If OK, do nothing.

## Node receives HM\_PROCESS\_NOTIFICATION

* Read Message from Socket
* Create HM\_PROCESS\_NOTIFICATION message (IPS)
* If Location not in Remote Location Tree, add to tree
    * Initialize Connection to location
* Send to PID of subscriber on HM\_TO\_USER Queue ID.

## Node goes down:
    
* Keepalive Times out
* Check if Missed Beats > Kickout Threshold
    * Yes. Set Node as down.
    * Setup Notification CB with NODE_INACTIVE
    * Wake up Notifier Thread
        * Parse subscribers and subscriptions
        * Send Notification to each
    
## HM Receives HELLO Response from Peer

* Transport Layer calls in Hardware Location CB callback
* Hardware Location CB Receives message header:
    
    * PEER_HELLO
    * Initialize Location CB
        * Parse complete message
        * Update Location CB
            * Index
        * Nodes CB Tree initialize
* Expect DB Updates

## HM Receives DB UPDATE
    
* Hardware Location CB Receives message header:
    
    * PEER_DB_UPDATE
    * Find Location CB
        * Parse complete message
            * Put Timestamp in Location CB Last Update [Read from Message]
            * Parse Node Information
                * Number of Nodes [For Pre-allocation of Node CBs?]
                * Node Indices
                * Node groups
                * Node Listen IPs
                * Timestamp
            * Add to Node Tree
                * Process subscriptions and notifications
                * If active-backup candidates found, add backup to active and active to backup relationships.
            * Parse Process Information
                * PCT Type
                * PID
                * Interfaces
                * Role
                * Timestamp
            * Add to Processes Tree
                * Process subscriptions and notifications

## Peer Goes Down

* Kickout timer determines the Peer as down
* Parse Node and Process Dependency lists
    * If backup available, send message to backup about primary failover \[Event Handler in the location may take decisions accordingly.\]\[With priority\]                                      
    * Send Notifications to All
    * Mark Nodes and Processes on that Node as down.                     
                
                                                        
                
                           