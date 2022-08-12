/**
 * \author Asra Naz Ujan
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include <assert.h>
#include "datamgr.h"
#include "lib/dplist.c"

struct element
{
    sensor_id_t sensor_id;
    uint16_t room_id;
    sensor_value_t temperatures[RUN_AVG_LENGTH];
    double running_avg;
    sensor_ts_t timestamp;
};

dplist_t *list = NULL;

void *element_copy(void *element);
void element_free(void **element);
int element_compare(void *x, void *y);

void datamgr_parse_sensor_files(FILE *fp_sensor_map, sbuffer_t **sbuffer)
{
    list = dpl_create(&element_copy, &element_free, &element_compare); // initilise list
    fp_sensor_map = fopen("room_sensor.map", "r");                     // read map file

    element_t *unique_element = malloc(sizeof(element_t)); // create pointer to data

    while (fscanf(fp_sensor_map, "%hd %hd", &(unique_element->room_id), &(unique_element->sensor_id)) > 0)
    {
        int i = 0;
        dplist_node_t *unique_node = calloc(1,sizeof(dplist_node_t));
        unique_node->element = unique_element;
        dpl_insert_at_index(list, unique_node, i, false); // insert room data to list
        i++;
    }

    sensor_data_t data;
    int result = sbuffer_read(*sbuffer, &data, DATAMGR);

    while (result == SBUFFER_SUCCESS)
    {
        element_t *list_item = malloc(sizeof(element_t));
        list_item->sensor_id = data.id;
        int index = dpl_get_index_of_element(list, list_item);
        if (index == -1)
        {
            char *logtxt;
            asprintf(&logtxt, "Recieved sensor data with invalid sensor node ID %d", data.id);
            sbuffer_write_fifo(*sbuffer, pfds, logtxt);
        }

        dplist_node_t *data_node = dpl_get_reference_of_element(list, list_item);
        list_item->timestamp = data.ts;

        sensor_value_t temperaturebuffer[RUN_AVG_LENGTH];
        {
            for (int i = 0; i < RUN_AVG_LENGTH; i++)
            {
                temperaturebuffer[i] = list_item->temperatures[i];
            }
            memmove(&temperaturebuffer[0], &temperaturebuffer[1], (RUN_AVG_LENGTH - 1) * sizeof(sensor_value_t)); // shifts array 1 to the left
            temperaturebuffer[RUN_AVG_LENGTH - 1] = data.value;
            // adds new value to end of array

            for (int i = 0; i < RUN_AVG_LENGTH; i++)
            {
                list_item->temperatures[i] = temperaturebuffer[i];
            }
            double sum = 0;
            if (list_item->temperatures[0] != 0) // check for full array
            {
                for (int i = 0; i < RUN_AVG_LENGTH; i++)
                {
                    sum += list_item->temperatures[i];
                }
                list_item->running_avg = (sum) / RUN_AVG_LENGTH;
            }
            else
            {
                list_item->running_avg = 0;
            }
            data_node->element = list_item;

            if (list_item->running_avg > SET_MAX_TEMP)
            {
                char *logtxt;
                asprintf(&logtxt, "The sensor with value %d reports it's too hot: %f.\n", list_item->sensor_id, list_item->running_avg);
                sbuffer_write_fifo(*sbuffer, pfds, logtxt);
            }
            if (list_item->running_avg > SET_MIN_TEMP)
            {
                char *logtxt;
                asprintf(&logtxt, "The sensor with value %d reports it's too cold: %f.\n", list_item->sensor_id, list_item->running_avg);
                sbuffer_write_fifo(*sbuffer, pfds, logtxt);
            }
        }
        fclose(fp_sensor_map);
    }
}

void datamgr_free()
{
    dpl_free(&list, true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_number)
{
    assert(list != NULL);
    dplist_node_t *dummy;
    element_t *temp_element;
    uint16_t result = 0;
    for (dummy = list->head, temp_element = dummy->element; dummy->next != NULL; dummy = dummy->next)
    {
        if (temp_element->sensor_id == sensor_number)
            return result = temp_element->room_id;
        // ERROR_HANDLER((dummy -> sensor_id != sensor_number && dummy -> next == NULL),"Sensor ID Invalid");
    }
    return result;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_number)
{
    assert(list != NULL);
    dplist_node_t *dummy;
    element_t *temp_element;
    double result = 0;
    for (dummy = list->head, temp_element = dummy->element; dummy->next != NULL; dummy = dummy->next)
    {
        if (temp_element->sensor_id == sensor_number)
        {
            result = temp_element->running_avg;
            return result;
        }
    }
    return result;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_number)
{
    assert(list != NULL);
    dplist_node_t *dummy;
    element_t *temp_element;
    time_t result = 0;
    for (dummy = list->head, temp_element = dummy->element; dummy->next != NULL; dummy = dummy->next)
    {
        if (temp_element->sensor_id == sensor_number)
        {
            result = temp_element->timestamp;
            return result;
        }
    }
    return result;
}

int datamgr_get_total_sensors() { return dpl_size(list); }

void *element_copy(void *element)
{
    element_t *element_tocopy = (element_t *)element;
    element_t *element_copy = malloc(sizeof(element_t));

    element_copy->sensor_id = element_tocopy->sensor_id;
    element_copy->room_id = element_tocopy->room_id;
    element_copy->running_avg = element_tocopy->running_avg;
    element_copy->timestamp = element_tocopy->timestamp;
    return (void *)element_copy;
}

void element_free(void **element)
{
    element_t *dummy = *element;
    free(dummy);
}

int element_compare(void *x, void *y)
{
    assert(x != NULL || y != NULL);
    element_t *comparer = (element_t *)y;
    element_t *comparee = (element_t *)x;
    if (comparer->sensor_id == comparee->sensor_id)
        return 0;
    else
        return -1;
}