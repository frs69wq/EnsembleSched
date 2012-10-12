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

int SD_task_is_ready(SD_task_t task){
  unsigned int i;
  int is_ready = 1;
  xbt_dynar_t parents, grand_parents;
  SD_task_t parent, grand_parent;

  parents = SD_task_get_parents(task);

  if (xbt_dynar_length(parents)) {
    xbt_dynar_foreach(parents, i, parent){
      grand_parents = SD_task_get_parents(parent);
      if (SD_task_get_kind(parent) == SD_TASK_COMM_E2E) {
        xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
        if (SD_task_get_state(grand_parent)<SD_SCHEDULED) {
          is_ready =0;
          xbt_dynar_free_container(&grand_parents);
          break;
        }
      }
      xbt_dynar_free_container(&grand_parents);
    }
  }
  xbt_dynar_free_container(&parents);

  return is_ready;
}

xbt_dynar_t SD_task_get_ready_children(SD_task_t t){
  unsigned int i;
  xbt_dynar_t children=NULL, ready_children;
  xbt_dynar_t output_transfers = SD_task_get_children(t);
  SD_task_t output, child;

  ready_children = xbt_dynar_new(sizeof(SD_task_t), NULL);

  xbt_dynar_foreach(output_transfers, i, output){
    if (SD_task_get_kind(output) == SD_TASK_COMM_E2E) {
      children = SD_task_get_children(output);
      xbt_dynar_get_cpy(children, 0, &child);
      if (xbt_dynar_member(ready_children, &child)){
        XBT_DEBUG("%s already seen, ignore", SD_task_get_name(child));
        continue;
      }
      if (SD_task_get_kind(child) == SD_TASK_COMP_SEQ &&
          (SD_task_get_state(child) == SD_NOT_SCHEDULED ||
           SD_task_get_state(child) == SD_SCHEDULABLE)&&
           SD_task_is_ready(child)){
        xbt_dynar_push(ready_children, &child);
      }
      xbt_dynar_free_container(&children); /* avoid memory leaks */
    } else { /* Control dependency */
      if (xbt_dynar_member(ready_children, &output)){
        XBT_DEBUG("%s already seen, ignore", SD_task_get_name(output));
        continue;
      }
      if (SD_task_get_kind(output) == SD_TASK_COMP_SEQ &&
          (SD_task_get_state(output) == SD_NOT_SCHEDULED ||
           SD_task_get_state(output) == SD_SCHEDULABLE)&&
           SD_task_is_ready(output)){
        xbt_dynar_push(ready_children, &output);
      }
    }
  }

  xbt_dynar_free_container(&output_transfers); /* avoid memory leaks */

  return ready_children;
}

