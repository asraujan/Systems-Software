/**
 * \author Asra Naz Ujan
 */

#ifndef _TCPSOCK_H_
#define _TCPSOCK_H

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "config.h"
#include <poll.h>
#include "sbuffer.h"

#ifndef TIMEOUT
#error TIMEOUT not set
#endif

typedef struct connection connection_t;
typedef struct pollfd    fds_t;

/* This method holds the core functionality of your connmgr. It starts listening on the given port and when a sensor node 
 connects it writes the data to a sensor_data_recv file. This file must have the same format as the sensor_data file in 
 assignment 6 and 7. 
 number of sensors connecting changes over time.
 sensor node opens a TCP connection to conmgr which remains open during the entire lifetime of the sensor node.
  Data in packet format is sent over TCP connection.
 sensor node started with (sensor_id) (sleep time between measurements in seconds) (IP address) (port number)
 conmgr can handle multiple TCP connections
 only one process which listens to multiple sockets and listens for new connection requests
 default blocking mode causes problem when one socket blocked ande receiving data on another
 ^ solved by multiplexing io with select() or poll()
 if no activity for TIMEOUT then socket closed and removed from connections.
 if connections empty, wait for TIMEOUT, terminate conmgr
 define TIMEOUT at compilation
 connection manager starts up a command line argument to define the port number for incoming connections. */
void connmgr_listen(int port_number, sbuffer_t ** sbuffer);

//This method should be called to clean up the connmgr, and to free all used memory. After this no new connections will be accepted
void connmgr_free();


void* element_copy(void* element);
void element_free(void** element);
int element_compare(void* x,void* y);

#endif //__TCPSOCK_H__
    