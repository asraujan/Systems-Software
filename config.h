/**
 * \author Asra Naz Ujan
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define FIFO_NAME 	"FIFO" 
#define MAX         300
#ifndef READERS
#define READERS 2 //default number of reader threads
#endif

#define STORAGEMGR 1
#define DATAMGR 0

#ifndef DEBUGGER
#define DEBUGGER 0
#endif

#include <stdint.h>
#include <pthread.h>
#include <time.h>

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

extern int pfds[2]; //for fifo pipe

#endif /* _CONFIG_H_ */
