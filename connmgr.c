/****************************************************************************************************************************
 *
 * FileName:        conmgr.c
 * Comment:         Connection manager
 * Dependencies:    Header (.h) files if applicable, tcp connection, sensor node, dplist.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                       		    Date                Version             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Asra Naz Ujan	            		                          0.1
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * TODO                         		    Date                Finished
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1) sensor node: user input
 * validation!
 * 2) fork() and mkfifo() should
 * have a log when they fail
 * 3) check for error responses
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 ***************************************************************************************************************************/

#define _GNU_SOURCE

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <poll.h>
#include <inttypes.h>

#include "connmgr.h"
#include "config.h"
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "lib/tcpsock.c"
#include "lib/dplist.c"

dplist_t *connections = NULL;

struct connection
{
      time_t ts;
      tcpsock_t *socket;
      fds_t socket_descriptor;
};

void *element_copy(void *element);
void element_free(void **element);
int element_compare(void *x, void *y);

void connmgr_listen(int port_number)
{
      tcpsock_t *server, *client;
      connection_t *new_node = calloc(1, sizeof(connection_t));
      FILE *fp_sensor_data = fopen("sensor_data_recv", "wb");
      if (fp_sensor_data == NULL)
            exit(EXIT_FAILURE);
      int fd_temp;

      connections = dpl_create(&element_copy, &element_free, &element_compare); // create connections list

      printf("Initialise server at port: %d and time:%ld\n", port_number, time(NULL));
      if (tcp_passive_open(&server, port_number) != TCP_NO_ERROR) // start server
            exit(EXIT_FAILURE);

      if (tcp_get_sd(server, &fd_temp) != TCP_NO_ERROR)
            exit(EXIT_FAILURE);

      connection_t *listener_node = malloc(sizeof(connection_t));
      listener_node->ts = time(NULL);
      listener_node->socket = server;
      listener_node->socket_descriptor.revents = 0;
      listener_node->socket_descriptor.events = POLLIN;
      listener_node->socket_descriptor.fd = fd_temp;
      time_t server_activity = time(NULL);

      dpl_insert_at_index(connections, (void *)listener_node, -1, false); // add listener at beginning of list
      printf("listener node added\n");
      // free(listener_node);

      static int running = 1; // flag for while loop
      while (running)
      {
            int bytes, result;
            int nr_connections = dpl_size(connections);
            for (int i = 0; i < nr_connections; i++)
            {
                  connection_t *current_conn = dpl_get_element_at_index(connections, i);
                  // printf("last active: %ld, TIMOOUT: %d, now: %ld\n",(current_conn->ts), (TIMEOUT * 1000), time(NULL));
                  poll(&(current_conn->socket_descriptor), nr_connections, 0); // 0 for immediate response

                  if (i == 0 && (current_conn->socket_descriptor.revents & POLLIN))
                  { // add latest connection from listening node
                        server_activity = time(NULL);
                        printf("waiting for new connection\n");
                        if (tcp_wait_for_connection(current_conn->socket, &client) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        printf("new connection received\n");
                        tcp_get_sd(client, &(new_node->socket_descriptor.fd));
                        new_node->socket_descriptor.events = POLLIN | POLLRDHUP;
                        new_node->socket_descriptor.revents = 0;
                        new_node->ts = time(NULL);
                        new_node->socket = client;
                        dpl_insert_at_index(connections, (void *)new_node, nr_connections, false);
                        nr_connections = dpl_size(connections); // recalculate size
                        printf("new connection added\n");
                        // free(new_node);
                        current_conn->ts = time(NULL); // update listener
                  }

                  if (i != 0 && (current_conn->socket_descriptor.revents & POLLIN))
                  {
                        server_activity = time(NULL);
                        sensor_data_t data;
                        // read sensor ID
                        bytes = sizeof(data.id);
                        result = tcp_receive(current_conn->socket, (void *)&data.id, &bytes);
                        fwrite((void *)&(data.id), (size_t)&bytes, 1, fp_sensor_data); // write data to binary file
                        //  read temperature
                        bytes = sizeof(data.value);
                        result = tcp_receive(current_conn->socket, (void *)&data.value, &bytes);
                        fwrite((void *)&(data.value), (size_t)&bytes, 1, fp_sensor_data);
                        //  read timestamp
                        bytes = sizeof(data.ts);
                        result = tcp_receive(current_conn->socket, (void *)&data.ts, &bytes);
                        // write to file
                        fwrite((void *)&(data.ts), (size_t)&bytes, 1, fp_sensor_data);
                        fwrite("\n", sizeof(char), 1, fp_sensor_data); // add new line operator
                        if ((result == TCP_NO_ERROR))
                        {
                              fflush(fp_sensor_data);
                              current_conn->ts = time(NULL); // update timestamp to show activity;
                              printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld, connectiontime=%ld\n", data.id, data.value,
                                     (long int)data.ts, server_activity);
                        }
                  }

                  if (current_conn->socket_descriptor.revents & POLLRDHUP)
                  { // connection terminated
                        server_activity = time(NULL);
                        if (tcp_close(&(current_conn->socket)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        connections = dpl_remove_at_index(connections, i, true);
                        nr_connections = dpl_size(connections); // recalculate
                        printf("connection terminated, connections list:%d\n", nr_connections);
                        break;
                  }
                  if (i > 0 && ((current_conn->ts) + (TIMEOUT)) < time(NULL))
                  { // timout for connection
                        printf("Timeout:%d, timestampt:%ld", TIMEOUT, current_conn->ts);
                        if (tcp_close(&(current_conn->socket)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        connections = dpl_remove_at_index(connections, i, true);
                        nr_connections = dpl_size(connections); // recalculate
                        printf("connection timeout, connections list:%d\n", nr_connections);
                        break;
                  }
            }
            if (((server_activity) + (TIMEOUT)) < time(NULL))
            {
                  if (nr_connections == 1)
                  {
                        printf("server timeout, connections list:%d\n", nr_connections);
                        if (tcp_close(&(server)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        free(new_node);
                        nr_connections = dpl_size(connections);
                        fclose(fp_sensor_data);
                        printf("file closed\n");
                        connmgr_free(connections);
                        running = 0; // terminate while loop, shut down server
                        break;
                  }
            }
      }
}

void connmgr_free()
{
      dpl_free(&connections, true);
}

void *element_copy(void *element)
{ // to be updated
      connection_t *element_tocopy = (connection_t *)element;
      connection_t *element_copy = calloc(1, sizeof(connection_t));

      element_copy->ts = element_tocopy->ts;
      element_copy->socket = element_tocopy->socket;
      element_copy->socket_descriptor.fd = element_tocopy->socket_descriptor.fd;
      element_copy->socket_descriptor.events = element_tocopy->socket_descriptor.events;
      element_copy->socket_descriptor.revents = element_tocopy->socket_descriptor.revents;
      return (void *)element_copy;
}

void element_free(void **element)
{
      connection_t *dummy = *element;
      free(dummy);
}

int element_compare(void *x, void *y)
{
      assert(x != NULL || y != NULL);
      connection_t *comparer = (connection_t *)y;
      connection_t *comparee = (connection_t *)x;
      if (comparer->socket == comparee->socket)
            return 0;
      else
            return -1;
}