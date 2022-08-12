/**
 * \author Asra Naz Ujan
 */


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

dplist_t *connections = NULL;

struct connection
{
      time_t ts;
      tcpsock_t *socket;
      fds_t socket_descriptor;
      sensor_id_t sensor_id;
};

void *connection_copy(void *element);
void connection_free(void **element);
int connection_compare(void *x, void *y);

void connmgr_listen(int port_number, sbuffer_t ** sbuffer)
{
      tcpsock_t *server, *client;

      FILE *fp_sensor_data = fopen("sensor_data_recv", "wb");
      if (fp_sensor_data == NULL)
            exit(EXIT_FAILURE);
      int fd_temp;

      connections = dpl_create(&connection_copy, &connection_free, &connection_compare); // create connections list
      #if(DEBUGGER == 1)
      printf("Initialise server at port: %d and time:%ld\n", port_number, time(NULL));
      #endif
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

      dpl_insert_at_index(connections, (void *)listener_node, -1, true); // add listener at beginning of list
      #if(DEBUGGER == 1)
      printf("listener node added\n");
      #endif
      free(listener_node);

      static int running = 1; // flag for while loop
      while (running)
      {
            int bytes, result;
            int nr_connections = dpl_size(connections);
            for (int i = 0; i < nr_connections; i++)
            {
                  connection_t *current_conn = dpl_get_element_at_index(connections, i);
                  int rc = poll(&(current_conn->socket_descriptor), nr_connections, 0); // 0 for immediate response
                  if (rc < 0)
                  {
                        continue;
                  }
                  if (i == 0 && (current_conn->socket_descriptor.revents & POLLIN))
                  { // add latest connection from listening node
                        server_activity = time(NULL);
                        connection_t *new_node = calloc(1,sizeof(connection_t)+sizeof(fds_t));
                        fds_t fd_temp;
                        #if(DEBUGGER == 1)
                        printf("waiting for new connection\n");
                        #endif
                        if (tcp_wait_for_connection(current_conn->socket, &client) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        #if(DEBUGGER == 1)
                        printf("new connection received\n");
                        #endif
                        tcp_get_sd(client, &(fd_temp.fd));
                        fd_temp.events = POLLIN | POLLRDHUP;
                        fd_temp.revents = 0;
                        new_node->socket_descriptor = fd_temp;
                        new_node->ts = time(NULL);
                        new_node->socket = client;
                        new_node->sensor_id = 0;
                        dpl_insert_at_index(connections, (void *)new_node, nr_connections, true);
                        nr_connections = dpl_size(connections); // recalculate size
                        #if(DEBUGGER == 1)
                        printf("new connection added\n");
                        #endif
                        free(new_node);
                        current_conn->ts = time(NULL); // update listener
                  }

                  if (i != 0 && (current_conn->socket_descriptor.revents & POLLIN))
                  {
                        server_activity = time(NULL);
                        sensor_data_t data;
                        // write data to binary file
                        // read sensor ID
                        bytes = sizeof(data.id);
                        result = tcp_receive(current_conn->socket, (void *)&data.id, &bytes);
                        //  read temperature
                        bytes = sizeof(data.value);
                        result = tcp_receive(current_conn->socket, (void *)&data.value, &bytes);
                        //  read timestamp
                        bytes = sizeof(data.ts);
                        result = tcp_receive(current_conn->socket, (void *)&data.ts, &bytes);
                        // write to file
                        if ((result == TCP_NO_ERROR))
                        {
                              fflush(fp_sensor_data);
                              current_conn->ts = time(NULL); // update timestamp to show activity;
                              sbuffer_insert(*sbuffer, &data);
                              #if(DEBUGGER == 1)
                              printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld, connectiontime=%ld\n", data.id, data.value,
                                     (long int)data.ts, server_activity);
                              #endif
                        }
                        if(current_conn->sensor_id == 0){
                        current_conn->sensor_id = data.id;
                        char * logtxt;
                        asprintf(&logtxt, "The sensor node with %d has opened a new connection",current_conn->sensor_id);
                        sbuffer_write_fifo(*sbuffer,pfds,logtxt);
                        }
                  }

                  if (current_conn->socket_descriptor.revents & POLLRDHUP)
                  { // connection terminated
                        sensor_data_t data;
                        bytes = sizeof(data.id);
                        result = tcp_receive(current_conn->socket, (void *)&data.id, &bytes);
                        server_activity = time(NULL);
                        if (tcp_close(&(current_conn->socket)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        connections = dpl_remove_at_index(connections, i, true);
                        nr_connections = dpl_size(connections); // recalculate
                        #if(DEBUGGER == 1)
                        printf("connection terminated, connections list:%d\n", nr_connections);
                        #endif
                        char * logtxt;
                        asprintf(&logtxt, "The sensor node with %d has closed the connection",current_conn->sensor_id);
                        sbuffer_write_fifo(*sbuffer,pfds,logtxt);
                        break;
                  }
                  if ((i > 0 && ((current_conn->ts) + (TIMEOUT)) < time(NULL)) || (rc<0))
                  { // timout for connection
                        #if(DEBUGGER == 1)
                        printf("Timeout:%d, timestampt:%ld", TIMEOUT, current_conn->ts);
                        #endif
                        if (tcp_close(&(current_conn->socket)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);
                        connections = dpl_remove_at_index(connections, i, true);
                        nr_connections = dpl_size(connections); // recalculate
                        #if(DEBUGGER == 1)
                        printf("connection timeout, connections list:%d\n", nr_connections);
                        #endif
                        break;
                  }


            }
            if (((server_activity) + (TIMEOUT)) < time(NULL))
            {
                  if (nr_connections == 1)
                  {
                        #if(DEBUGGER == 1)
                        printf("server timeout, connections list:%d\n", nr_connections);
                        #endif
                        if (tcp_close(&(server)) != TCP_NO_ERROR)
                              exit(EXIT_FAILURE);

                        nr_connections = dpl_size(connections);
                        fclose(fp_sensor_data);
                        #if(DEBUGGER == 1)
                        printf("file closed\n");
                        #endif
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

void *connection_copy(void *element)
{ // to be updated
      connection_t *element_tocopy = (connection_t *)element;
      connection_t *element_copy = calloc(1, sizeof(connection_t) + sizeof(fds_t));

      element_copy->ts = element_tocopy->ts;
      element_copy->socket = element_tocopy->socket;
      element_copy->socket_descriptor.fd = element_tocopy->socket_descriptor.fd;
      element_copy->socket_descriptor.events = element_tocopy->socket_descriptor.events;
      element_copy->socket_descriptor.revents = element_tocopy->socket_descriptor.revents;
      return (void *)element_copy;
}

void connection_free(void **element)
{
      connection_t *dummy = *element;
      free(dummy);
}

int connection_compare(void *x, void *y)
{
      assert(x != NULL || y != NULL);
      connection_t *comparer = (connection_t *)y;
      connection_t *comparee = (connection_t *)x;
      if (comparer->socket == comparee->socket)
            return 0;
      else
            return -1;
}