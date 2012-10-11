/*
 * task.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#include "xbt.h"
#include "task.h"
#include "simdag/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(task, EnsembleSched,
                                "Logging specific to tasks");

void SD_task_allocate_attribute(SD_task_t task){
  void *data;
  data = calloc(1,sizeof(struct _TaskAttribute));
  SD_task_set_data(task, data);
}

void SD_task_free_attribute(SD_task_t task){
  free(SD_task_get_data(task));
  SD_task_set_data(task, NULL);
}

void SD_task_set_dax_priority(SD_task_t task, int priority){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->dax_priority=priority;
  SD_task_set_data(task, attr);
}

int SD_task_set_dax_priority(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->dax_priority;
}



xbt_dynar_t SD_task_get_ready_children(SD_task_t t){
  xbt_dynar_t ready_children=NULL;
  //TODO
  return ready_children;
}

