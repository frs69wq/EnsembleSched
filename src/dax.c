/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#include <stdlib.h>
#include <math.h>
#include "simgrid/simdag.h"
#include "xbt.h"
#include "scheduling.h"
#include "dax.h"
#include "task.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dax, EnsembleSched, "Logging specific to daxes");

SD_task_t get_root(xbt_dynar_t dax){
  SD_task_t task;

  xbt_dynar_get_cpy(dax, 0, &task);
  return task;
}

SD_task_t get_end(xbt_dynar_t dax){
  SD_task_t task;

  xbt_dynar_get_cpy(dax, xbt_dynar_length(dax)-1, &task);
  return task;
}

/* Comparison function to sort DAXes increasingly according to their size.
 * Assumption: this size consider all tasks (computations AND transfers). Could be modified to count only compute tasks.
 */
int sizeCompareDaxes(const void *d1, const void *d2) {
  int size1, size2;

  size1 = xbt_dynar_length(*((xbt_dynar_t *)d1));
  size2 = xbt_dynar_length(*((xbt_dynar_t *)d2));

  if (size1 < size2)
    return -1;
  else if (size1 == size2)
    return 0;
  else
    return 1;
}

/* Implementation of Knuth shuffle found on the web.
 * Arrange the N elements of ARRAY in random order. Only effective if N is much smaller than RAND_MAX; if this may not
 * be the case, use a better random number generator. */
void shuffle(int *array, size_t n){
  if (n > 1){
    size_t i;
    for (i = 0; i < n - 1; i++){
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}


/* Assign priorities to DAXes according to a user specified method. Priorities are unique integer values taken
 * in [0;#daxes].
 *  - RANDOM priorities: the array of possible priorities is shuffled using the Knuth shuffle. Then priorities are
 *    assigned to daxes in order.
 *  - SORTED priorities: Daxes are first sorted by ascending size. Then increasing priorities are assigned according
 *    to this order.
 * Remark: the size of a dax used by the comparison function includes data transfer tasks.
 */
void assign_dax_priorities(xbt_dynar_t daxes, method_t method){
  int *priorities;
  unsigned int i, j;
  int ndaxes = xbt_dynar_length(daxes);
  xbt_dynar_t current_dax;
  SD_task_t task;

  priorities = (int*) calloc (ndaxes, sizeof(int));
  for (i=0;i<ndaxes;i++)
    priorities[i]=i;

  if (method == RANDOM){
    shuffle(priorities, ndaxes);
  } else if (method == SORTED){
     xbt_dynar_sort(daxes,sizeCompareDaxes);
  }

  xbt_dynar_foreach(daxes, i, current_dax){
    xbt_dynar_foreach(current_dax,j, task){
      SD_task_set_dax_priority(task, priorities[i]);
    }
  }
  free(priorities);
}

double compute_score(xbt_dynar_t daxes){
  double total_score = 0;
  double current_score;
  unsigned int i;
  xbt_dynar_t current_dax;
  SD_task_t task;

  xbt_dynar_foreach(daxes, i, current_dax){
    task = get_end(current_dax);
    if (SD_task_get_state(task) == SD_DONE){
      current_score = pow(2.0, -SD_task_get_dax_priority(task));
      XBT_DEBUG("%s has completed its execution. It contributes to the score by %f",
                SD_task_get_dax_name(task), current_score);
      total_score += current_score;
    }
  }
  return total_score;
}
