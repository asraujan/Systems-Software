/****************************************************************************************************************************
 *
 * FileName:        dplist.c
 * Comment:         My Double Linked List Implementation
 * Dependencies:    Header (.h) files if applicable, see below.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                       		    Date                Version             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Asra Naz Ujan	            		                          0.1                   
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * TODO                         		    Date                Finished
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1) 
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 ***************************************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include <assert.h>
#include "datamgr.h"
#include "lib/dplist.h"
#include "lib/dplist.c"

struct element{
sensor_id_t    sensor_id;
uint16_t       room_id;
double         running_avg;
sensor_ts_t    timestamp;
};

dplist_t *list = NULL;

void* element_copy(void* element);
void element_free(void** element);
int element_compare(void* x,void* y);

void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data){

list = dpl_create(&element_copy, &element_free, &element_compare);               //initilise list
element_t *list_item;
list_item = NULL;
fp_sensor_map = fopen("room_sensor.map","r");                                 //read map file
fp_sensor_data = fopen("sensor_data","r");                                    //read binary data file
if(fp_sensor_data == NULL || fp_sensor_map ==((void *)0)) 
//{fprintf(stderr, ERROR_HANDLER((fp_sensor_data == NULL || fp_sensor_map ==NULL), "File Error"));}

while (fscanf(fp_sensor_map,"%hd %hd", &(list_item->room_id), &(list_item->sensor_id))>0){
  int i=0;
  element_t *list_item= malloc(sizeof(element_t)); //create pointer to data 
  dplist_node_t *unique_node = malloc(sizeof(dplist_node_t)); 
  unique_node->element = list_item;
  dpl_insert_at_index(list,unique_node,i,true);                                   //insert room data to list
  i++;}

int sensor_id_store;
while (fread(&sensor_id_store, sizeof(uint16_t), 1, fp_sensor_data) != EOF){
  list_item -> sensor_id = sensor_id_store;
  dplist_node_t * data_node = dpl_get_reference_of_element(list,list_item);

  long int time_store;
  while (fread(&time_store,sizeof(time_t),1,fp_sensor_data) != EOF){
  list_item ->timestamp =time_store;}
  
  sensor_value_t temperature[RUN_AVG_LENGTH];
  int count = 0;
  double sum=0; 
  while(fread(temperature,sizeof(double),1,fp_sensor_data) != EOF){
  sum += temperature[count];
  if (count >= RUN_AVG_LENGTH){
  list_item -> running_avg = (sum)/RUN_AVG_LENGTH;}
  else {list_item -> running_avg = 0;}}
  count++;
  data_node ->element = list_item;
}
fclose(fp_sensor_data);
fclose(fp_sensor_map);
}

void datamgr_free(){
dpl_free(&list,true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_number){
  assert(list !=NULL);
  dplist_node_t *dummy;
  element_t * temp_element;
  uint16_t result=0;
for (dummy = list -> head, temp_element = dummy ->element; dummy -> next != NULL; dummy = dummy ->next)
{
  if(temp_element -> sensor_id == sensor_number) return result = temp_element -> room_id;
 // ERROR_HANDLER((dummy -> sensor_id != sensor_number && dummy -> next == NULL),"Sensor ID Invalid");
}return result;

}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_number){
assert(list !=NULL);
  dplist_node_t *dummy;
  element_t * temp_element;
  double result=0;
for (dummy = list -> head, temp_element = dummy ->element; dummy -> next != NULL;dummy = dummy ->next)
{
  if(temp_element -> sensor_id == sensor_number) {
    result = temp_element -> running_avg;
    return result;
    }
//  ERROR_HANDLER((dummy -> sensor_id != sensor_number && dummy -> next == NULL),"Sensor ID Invalid");
}return result;
}


time_t datamgr_get_last_modified(sensor_id_t sensor_number){
  assert(list !=NULL);
  dplist_node_t *dummy;
  element_t * temp_element;
  time_t result=0;
for (dummy = list -> head, temp_element = dummy ->element; dummy -> next != NULL;dummy = dummy ->next)
{
  if(temp_element -> sensor_id == sensor_number) {
    result = temp_element -> timestamp;
    return result;
    }
// ERROR_HANDLER((dummy -> sensor_id != sensor_number && dummy -> next == NULL),"Sensor ID Invalid");
}return result;} 

int datamgr_get_total_sensors() {return dpl_size(list);}


void* element_copy(void* element){
  element_t *element_tocopy = (element_t *) element;
  element_t *element_copy = malloc(sizeof(element_t));

  element_copy-> sensor_id = element_tocopy -> sensor_id;
  element_copy-> room_id = element_tocopy-> room_id;
  element_copy -> running_avg = element_tocopy -> running_avg;
  element_copy -> timestamp = element_tocopy -> timestamp;
  return (void*)element_copy;
  }

void element_free(void** element){
  element_t * dummy = *element;
  free(dummy);
}

int element_compare(void* x,void* y){
  assert(x != NULL || y != NULL);
  element_t * comparer = (element_t *)y;
  element_t * comparee = (element_t *) x;
  if(comparer -> sensor_id == comparee -> sensor_id) return 0;
  else return -1;
}