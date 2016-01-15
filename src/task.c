/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#include "xbt.h"
#include "task.h"
#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(task, EnsembleSched,
                                "Logging specific to tasks");

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/

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

/*****************************************************************************/
/*****************************************************************************/
/**************    Functions needed by scheduling algorithms    **************/
/*****************************************************************************/
/*****************************************************************************/

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

/* Determine if a task is ready. The condition to meet is that all its
 * compute predecessors have to be in one of the followind state:
 *  - SD_RUNNABLE
 *  - SD_RUNNING
 *  - SD_DONE
 */
int SD_task_is_ready(SD_task_t task){
  unsigned int i;
  int is_ready = 1;
  xbt_dynar_t parents, grand_parents;
  SD_task_t parent, grand_parent;

  parents = SD_task_get_parents(task);

  if (xbt_dynar_length(parents)) {
    xbt_dynar_foreach(parents, i, parent){
      if (SD_task_get_kind(parent) == SD_TASK_COMM_E2E) {
        /* Data dependency case: a compute task is preceded by a data transfer.
         * Its parent (in a scheduling sense) is then the grand parent
         */
        grand_parents = SD_task_get_parents(parent);
        xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
        if (SD_task_get_state(grand_parent)<SD_SCHEDULED) {
          is_ready =0;
          xbt_dynar_free_container(&grand_parents); /* avoid memory leaks */
          break;
        } else {
          xbt_dynar_free_container(&grand_parents); /* avoid memory leaks */
        }
      } else {
        /* Control dependency case: a compute task predecessor is another
         * compute task. */
        if (SD_task_get_state(parent)<SD_SCHEDULED) {
         is_ready =0;
         break;
        }
      }
    }
  }
  xbt_dynar_free_container(&parents); /* avoid memory leaks */

  return is_ready;
}

/* Build the set of the compute successors of a task that are ready (i.e., with
 * all parents already scheduled). Both data and control dependencies are
 * checked. As more than one transfer may exist between two compute tasks, it is
 * mandatory to check whether the successor is not already in the set.
 */
xbt_dynar_t SD_task_get_ready_children(SD_task_t t){
  unsigned int i;
  xbt_dynar_t children=NULL, ready_children;
  xbt_dynar_t output_transfers = SD_task_get_children(t);
  SD_task_t output, child;

  ready_children = xbt_dynar_new(sizeof(SD_task_t), NULL);

  xbt_dynar_foreach(output_transfers, i, output){
    if (SD_task_get_kind(output) == SD_TASK_COMM_E2E) {
      /* Data dependency case: a compute task is followed by a data transfer.
       * Its child (in a scheduling sense) is then the grand child
       */
      children = SD_task_get_children(output);
      xbt_dynar_get_cpy(children, 0, &child);

      /* check if this child is already in the set */
      if (xbt_dynar_member(ready_children, &child)){
        XBT_DEBUG("%s already seen, ignore", SD_task_get_name(child));
        xbt_dynar_free_container(&children); /* avoid memory leaks */
        continue;
      }

      if (SD_task_get_kind(child) == SD_TASK_COMP_SEQ &&
          (SD_task_get_state(child) == SD_NOT_SCHEDULED ||
           SD_task_get_state(child) == SD_SCHEDULABLE)&&
           SD_task_is_ready(child)){
        xbt_dynar_push(ready_children, &child);
      }
      xbt_dynar_free_container(&children); /* avoid memory leaks */
    } else {
      /* Control dependency case: a compute task successor is another
       * compute task. */
      /* check if this child is already in the set */
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

