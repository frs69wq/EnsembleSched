/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#ifndef TASK_H_
#define TASK_H_
#include "simgrid/simdag.h"

typedef struct _TaskAttribute *TaskAttribute;
struct _TaskAttribute {
  char *daxname;
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
void SD_task_set_dax_name(SD_task_t, char*);
char* SD_task_get_dax_name(SD_task_t);
void SD_task_set_dax_priority(SD_task_t, int);
int SD_task_get_dax_priority(SD_task_t);

/*
 * Comparators
 */
int daxPriorityCompareTasks(const void *, const void *);

/* Other functions needed by scheduling algorithms */
xbt_dynar_t SD_task_get_ready_children(SD_task_t t);

#endif /* TASK_H_ */
