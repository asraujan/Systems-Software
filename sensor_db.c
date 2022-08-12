/**
 * \author Asra Naz Ujan
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"

typedef int (*callback_t)(void *, int, char **, char **);

DBCONN *init_connection(char clear_up_flag, sbuffer_t *sbuffer)
{ // perhaps create table
    DBCONN *conn;
    char *err_msg = 0;
    int rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database:%s\n", sqlite3_errmsg(conn));
        sqlite3_close(conn);
        char *logtxt;
        asprintf(&logtxt, "Unable to connect to SQL server\n");
        sbuffer_write_fifo(sbuffer, pfds, logtxt);
        return NULL;
    }

    if (clear_up_flag == 1)
    {
        rc = sqlite3_exec(conn, "DROP TABLE IF EXISTS " TO_STRING(TABLE_NAME) ";", 0, 0, &err_msg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Cannot delete table:%s\n", sqlite3_errmsg(conn));
            sqlite3_free(err_msg);
            return NULL;
        }
        sqlite3_free(err_msg);
    }

    rc = sqlite3_exec(conn, "CREATE TABLE IF NOT EXISTS " TO_STRING(TABLE_NAME) "(id INTEGER PRIMARY KEY ASC AUTOINCREMENT, sensor_id INTEGER, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);", 0, 0, &err_msg);
    if (rc == SQLITE_OK)
    {
        char *logtxt;
        asprintf(&logtxt, "New table "TO_STRING(TABLE_NAME)" created\n");
        sbuffer_write_fifo(sbuffer, pfds, logtxt);
    }
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot create table:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return NULL;
    }

    char *logtxt;
    asprintf(&logtxt, "Connection to SQL server established\n");
    sbuffer_write_fifo(sbuffer, pfds, logtxt);
    return conn;
    free(err_msg);
}

void disconnect(DBCONN *conn, sbuffer_t * sbuffer)
{
    char *logtxt;
    asprintf(&logtxt, "Connection to SQL server lost.\n");
    sbuffer_write_fifo(sbuffer, pfds, logtxt);
    sqlite3_close(conn);
}

int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    char *sql;
    char *err_msg = 0;
    int x = asprintf(&sql, "INSERT INTO %s Values(%hd, %f, %ld);", TO_STRING(TABLE_NAME), id, value, ts);
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }

    int rc = sqlite3_exec(conn, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot run query:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }

    free(sql);
    free(err_msg);
    return 0;
}

int insert_sensor_from_file(DBCONN *conn, sbuffer_t **sbuffer)
{
    sensor_data_t data;
    int result = sbuffer_read(*sbuffer, &data, STORAGEMGR);
    while (result == SBUFFER_SUCCESS)
    {
        insert_sensor(conn, data.id, data.value, data.ts);
        return 0;
    }
    if (result == SBUFFER_FAILURE || result == SBUFFER_NO_DATA)
    {
        return -1;
    }
    return 0;
}

int find_sensor_all(DBCONN *conn, callback_t f)
{
    char *err_msg = 0;
    char *sql;

    int x = asprintf(&sql, "SELECT sensor_value from %s;", TO_STRING(TABLE_NAME));
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }

    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f)
{ // implement callback
    char *err_msg = 0;
    char *sql;

    int x = asprintf(&sql, "SELECT sensor_value from %s WHERE sensor_value=%f", TO_STRING(TABLE_NAME), value);
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }

    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL errorL %s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f)
{ // implement callback
    char *err_msg = 0;
    char *sql;

    int x = asprintf(&sql, "SELECT sensor_value from %s WHERE sensor_value>%f", TO_STRING(TABLE_NAME), value);
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL errorL %s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f)
{
    char *err_msg = 0;
    char *sql;

    int x = asprintf(&sql, "SELECT sensor_value from %s WHERE timestamp=%ld", TO_STRING(TABLE_NAME), ts);
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL errorL %s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}

int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f)
{
    char *err_msg = 0;
    char *sql;
    int x = asprintf(&sql, "SELECT sensor_value from %s WHERE timestamp>%ld", TO_STRING(TABLE_NAME), ts);
    if (x == -1)
    {
        fprintf(stderr, "Error in adding parameters:%s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL errorL %s\n", sqlite3_errmsg(conn));
        sqlite3_free(err_msg);
        return -1;
    }
    free(sql);
    free(err_msg);
    return 0;
}
