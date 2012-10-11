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
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  free(attr->daxname);
  free(SD_task_get_data(task));
  SD_task_set_data(task, NULL);
}
void SD_task_set_dax_name(SD_task_t task, char *daxname){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->daxname=strdup(daxname);
  SD_task_set_data(task, attr);

}

char* SD_task_get_dax_name(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->daxname;
}

void SD_task_set_dax_priority(SD_task_t task, int priority){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->dax_priority=priority;
  SD_task_set_data(task, attr);
}

int SD_task_get_dax_priority(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->dax_priority;
}

/* Comparison function to sort tasks increasingly according to their priority
 * of the DAX they belong to.
 */
int daxPriorityCompareTasks(const void * t1, const void *t2){
  int priority1, priority2;

  priority1 = SD_task_get_dax_priority(*((SD_task_t *)t1));
  priority2 = SD_task_get_dax_priority(*((SD_task_t *)t2));

  if (priority1 < priority2)
    return -1;
  else if (priority1 == priority2)
    return 0;
  else
    return 1;
}


xbt_dynar_t SD_task_get_ready_children(SD_task_t t){
  xbt_dynar_t ready_children=NULL;
  //TODO
  return ready_children;
}

