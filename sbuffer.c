/**
 * \author Asra Naz Ujan
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "sbuffer.h"

int sbuffer_init(sbuffer_t **buffer)
{
      *buffer = malloc(sizeof(sbuffer_t));
      if (*buffer == NULL)
            return SBUFFER_FAILURE;
      (*buffer)->head = NULL;
      (*buffer)->tail = NULL;
      (*buffer)->lock = malloc(sizeof(pthread_mutex_t));
      (*buffer)->fifo_lock = malloc(sizeof(pthread_mutex_t));
      (*buffer)->signal = malloc(sizeof(pthread_cond_t));
      pthread_mutex_init((*buffer)->lock, NULL);
      pthread_mutex_init((*buffer)->fifo_lock, NULL);
      pthread_cond_init((*buffer)->signal, NULL);
      return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer)
{
      sbuffer_node_t *dummy;
      if ((buffer == NULL) || (*buffer == NULL))
      {
            return SBUFFER_FAILURE;
      }
      while ((*buffer)->head)
      {
            dummy = (*buffer)->head;
            (*buffer)->head = (*buffer)->head->next;
            free(dummy);
      }
      pthread_mutex_destroy((*buffer)->lock);
      pthread_mutex_destroy((*buffer)->fifo_lock);
      pthread_cond_destroy((*buffer)->signal);
      free((*buffer)->signal);
      free((*buffer)->lock);
      free((*buffer)->fifo_lock);
      free((*buffer)->head);
      free((*buffer)->tail);
      free(*buffer);
      *buffer = NULL;
      return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data)
{
      sbuffer_node_t *dummy;
      if (buffer == NULL)
            return SBUFFER_FAILURE;
      if (buffer->head == NULL)
            return SBUFFER_NO_DATA;
      *data = buffer->head->data;
      dummy = buffer->head;
      if (buffer->head == buffer->tail) // buffer has only one node
      {
            buffer->head = buffer->tail = NULL;
      }
      else // buffer has many nodes empty
      {
            buffer->head = buffer->head->next;
      }
      free(dummy);
      return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data)
{ // implements write thread
      sbuffer_node_t *dummy;
      if (buffer == NULL)
            return SBUFFER_FAILURE;
      dummy = malloc(sizeof(sbuffer_node_t));
      if (dummy == NULL)
            return SBUFFER_FAILURE;
      pthread_mutex_lock(buffer->lock);
      dummy->data = *data;
      dummy->next = NULL;
      for (int i = 0; i < READERS; i++)
      {
            dummy->read_flag[i] = 0; // fresh data not read
      }
      if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
      {
            buffer->head = buffer->tail = dummy;
      }
      else // buffer not empty
      {
            buffer->tail->next = dummy; // FIFO
            buffer->tail = buffer->tail->next;
      }
      pthread_cond_broadcast(buffer->signal); // signal data added
      pthread_mutex_unlock(buffer->lock);
      return SBUFFER_SUCCESS;
}

int sbuffer_read(sbuffer_t *buffer, sensor_data_t *data, int thread)
{
      sbuffer_node_t *dummy;
      if (buffer == NULL)
            return SBUFFER_FAILURE;

      pthread_mutex_lock(buffer->lock);
      if (buffer->tail == NULL)
      { // buffer empty
            pthread_cond_wait(buffer->signal, buffer->lock);
#if (DEBUGGER == 1)
            printf("Waiting for new data");
#endif
            return SBUFFER_NO_DATA;
      }

      else
      {                           // buffer not empty
            dummy = buffer->head; // FIFO
            if (dummy->read_flag[thread] == 1)
            { // already read by thread
                  pthread_mutex_unlock(buffer->lock);
                  return SBUFFER_NO_DATA;

#if (DEBUGGER == 1)
                  printf("Already read");
#endif
            }
            else
            {
                  dummy->read_flag[thread] = 1; // mark as read
                  if (sbuffer_reader_count(dummy) == 1)
                  {
                        sbuffer_remove(buffer, data); // read by all threads so can be removed
                  }
                  else // keep node in list for other thread(s)
                  {
                        *data = dummy->data; // read data
                  }
#if (DEBUGGER == 1)
                  printf("Data read");
#endif
            }
      }
      pthread_mutex_unlock(buffer->lock);
      return SBUFFER_SUCCESS;
}

int sbuffer_reader_count(sbuffer_node_t *node)
{
      for (int i = 0; i < READERS; i++)
      {
            if (node->read_flag[i] == 0)
                  return 0;
      }
      return 1;
}

void sbuffer_write_fifo(sbuffer_t *buffer, int *pfds, char *msg_buff)
{
      pthread_mutex_lock(buffer->fifo_lock);
      write(*(pfds + 1), msg_buff, strlen(msg_buff) + 1);
      pthread_mutex_unlock(buffer->fifo_lock);
      free(msg_buff);
}

// sbuffer_reader
// use mutex with condition variable because sbuffer, fifo is a shared resource it will allow other thread to continue until signal reached
// avoid iterating through sbuffer to avoid busy whiles or starvation