/**
 * \author Asra Naz Ujan
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include "sensor_db.h"
#include "datamgr.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "config.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"

void run_child();
void *connmgr();
void *datamgr();
void *storagemgr();

int port;
FILE *fp_sensor_map;
pid_t parent_pid, child_pid;
sbuffer_t *sbuffer;
pthread_t connmgr_thread;
pthread_t datamgr_thread;
pthread_t storagemgr_thread;
int pfds[2];

int main(int argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        printf("Port number for server not specified\n");
        return -1;
    }
    port = atoi(argv[1]);

    pipe(pfds);
    parent_pid = getpid();
    child_pid = fork();
#if (DEBUGGER == 1)
    printf("Parent process (pid = %d) is started ...\n", parent_pid);
#endif

    if (child_pid == 0)
    {
        run_child();
    }

    else
    { // parent's code
#if (DEBUGGER == 1)
        printf("Parent process (pid = %d) has created child process (pid = %d)...\n", parent_pid, child_pid);
#endif
        close(pfds[1]); // not needed by parent

        sbuffer_init(&sbuffer);

        pthread_create(&connmgr_thread, NULL, &connmgr, NULL);
        pthread_create(&datamgr_thread, NULL, &datamgr, NULL);
        pthread_create(&storagemgr_thread, NULL, &storagemgr, NULL);

        pthread_join(connmgr_thread, NULL);
        pthread_join(storagemgr_thread, NULL);
        pthread_join(datamgr_thread, NULL);

        sbuffer_free(&sbuffer);
    }

    // initialise fifo and child process
}

void run_child()
{

    child_pid = getpid();
    close(pfds[0]); // not needed by child
#if (DEBUGGER == 1)
    printf("Greetings from child process (pid = %d) of parent (pid = %d) ...\n", child_pid, parent_pid);
#endif

    FILE *log = fopen("gateway.log", "w");
    char *str_result;
    char recv_buf[MAX];
    int sequence = 0;

    /* Create the FIFO if it does not exist */
    mkfifo(FIFO_NAME, 0666);
    FILE *fifo = fopen(FIFO_NAME, "r");
#if (DEBUGGER == 1)
    printf("syncing with writer ok\n");
#endif

    do
    {
        str_result = fgets(recv_buf, MAX, fifo);
        if (str_result != NULL)
        {
#if (DEBUGGER == 1)
            printf("Message received: %s", recv_buf);
#endif
            fprintf(log, "%d %ld %s\n", sequence, time(NULL), recv_buf);
        }
    } while (str_result != NULL);

    fclose(log);
    fclose(fifo);
    close(pfds[1]); // indicate end of writing
#if (DEBUGGER == 1)
    printf("Child process (pid = %d) of parent (pid = %d) is terminating ...\n", child_pid, parent_pid);
#endif
    exit(EXIT_SUCCESS); // this terminates the child process
}

void *connmgr()
{
    connmgr_listen(port, &sbuffer);
    pthread_exit(NULL);
}

void *datamgr()
{
    fp_sensor_map = fopen("room_sensor.map", "r");
    datamgr_parse_sensor_files(fp_sensor_map, &sbuffer);
    datamgr_free();
    fclose(fp_sensor_map);
    pthread_exit(NULL);
}

void *storagemgr()
{
    int tries = 0;
    DBCONN *database = NULL;
    while (tries < 3 && (database == NULL))
    {
        database = init_connection(1, sbuffer);
        tries++;
    }

    if (tries == 3 && (database == NULL))
    {
        pthread_cancel(connmgr_thread);
        pthread_cancel(datamgr_thread);
    }

    if (database != NULL)
    {
        insert_sensor_from_file(database, &sbuffer);
    }
    disconnect(database, sbuffer);
    pthread_exit(NULL);
}