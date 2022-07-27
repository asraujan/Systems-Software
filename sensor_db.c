/****************************************************************************************************************************
 *
 * FileName:        sensor_db.c
 * Comment:         Database manager
 * Dependencies:    Header (.h) files if applicable, datamanager
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                       		    Date                Version             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Asra Naz Ujan	            		                          0.1                   
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * TODO                         		    Date                Finished
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1) initialise table with
 *  autoincrementing PK
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 ***************************************************************************************************************************/


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"

typedef int (*callback_t)(void *, int, char **, char **);

DBCONN *init_connection(char clear_up_flag){            //perhaps create table
    DBCONN *conn;
    char *err_msg =0;
    int rc = sqlite3_open(TO_STRING(DB_NAME) ,&conn);
    if(rc != SQLITE_OK){
        fprintf(stderr,"Cannot open database:%s\n",sqlite3_errmsg(conn));
        sqlite3_close(conn);
        return NULL;}
    else return conn;

    if (clear_up_flag == 1)
    {
        rc = sqlite3_exec(conn,"DELETE FROM "TO_STRING(TABLE_NAME)";",0,0,&err_msg);
        if(rc != SQLITE_OK){
            fprintf(stderr,"Cannot delete from table:%s\n",sqlite3_errmsg(conn));
            sqlite3_free(err_msg);
            return NULL;}
        sqlite3_free(err_msg);
        return conn;}
    free(err_msg);
    }

void disconnect(DBCONN *conn){
    sqlite3_close(conn);}

int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
    char *sql;
    char * err_msg = 0;
    int x = asprintf(&sql,"INSERT INTO %s Values(%hd, %f, %ld);",TO_STRING(TABLE_NAME),id,value,ts);
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}

    int rc = sqlite3_exec(conn,sql,0,0,&err_msg);
        if(rc != SQLITE_OK){
            fprintf(stderr,"Cannot run query:%s\n",sqlite3_errmsg(conn));
            sqlite3_free(err_msg);
            return -1;}

    free(sql);
    free(err_msg);
    return 0;

}

int insert_sensor_from_file(DBCONN *conn, FILE *sensor_data){
    sensor_id_t sensor_id_store;
    time_t      time_store;
    sensor_value_t value_store;
    while (fread(&sensor_id_store, sizeof(uint16_t), 1, sensor_data) != EOF){
        fread(&time_store,sizeof(time_t),1,sensor_data);
        fread(&value_store,sizeof(double),1,sensor_data);
        int try_insert = insert_sensor(conn, sensor_id_store, value_store, time_store);
        if(try_insert != 0){         
        fprintf(stderr,"Error occured:%s\n",sqlite3_errmsg(conn));
        return -1;}
}
    fclose(sensor_data);
    return 0;
}

int find_sensor_all(DBCONN *conn, callback_t f){
    char * err_msg = 0;
    char * sql;

    int x = asprintf(&sql,"SELECT sensor_value from %s;",TO_STRING(TABLE_NAME));
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}

    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr,"SQL error: %s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f){ //implement callback
    char * err_msg = 0;
    char * sql;
    
    int x = asprintf(&sql,"SELECT sensor_value from %s WHERE sensor_value=%f",TO_STRING(TABLE_NAME),value);
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}
    
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr,"SQL errorL %s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f){ //implement callback
    char * err_msg = 0;
    char * sql;
    
    int x = asprintf(&sql,"SELECT sensor_value from %s WHERE sensor_value>%f",TO_STRING(TABLE_NAME),value);
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr,"SQL errorL %s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}


int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
    char * err_msg = 0;
    char * sql;
    
    int x = asprintf(&sql,"SELECT sensor_value from %s WHERE timestamp=%ld",TO_STRING(TABLE_NAME),ts);
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr,"SQL errorL %s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}



int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
    char * err_msg = 0;
    char * sql;
    int x = asprintf(&sql,"SELECT sensor_value from %s WHERE timestamp>%ld",TO_STRING(TABLE_NAME),ts);
    if(x==-1){   
        fprintf(stderr,"Error in adding parameters:%s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;}
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr,"SQL errorL %s\n",sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

/*
int (*callback_t f)(void * unused, int count, char **data, char **columns){
    unused = 0;
    for(int i=0; int<count;i++){
        printf("%s=%s\n",columns[i],data[i] ? data[i] : "NULL");}
    printf("\n");
    return 0;
} */