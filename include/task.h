/*
 * task.h
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#ifndef TASK_H_
#define TASK_H_
#include "simdag/simdag.h"

typedef struct _TaskAttribute *TaskAttribute;
struct _TaskAttribute {
  int dax_priority;
  //TODO add necessary attributes
};

/*
 * Creator and destructor
 */
void SD_task_allocate_attribute(SD_task_t);
void SD_task_free_attribute(SD_task_t);

/*
 * Accessors
 */
void SD_task_set_dax_priority(SD_task_t, int);
int SD_task_set_dax_priority(SD_task_t);

/* Other functions needed by scheduling algorithms */
xbt_dynar_t SD_task_get_ready_children(SD_task_t t);

#endif /* TASK_H_ */
