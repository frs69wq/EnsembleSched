/*
 * dax.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#include <stdlib.h>
#include <math.h>
#include "simdag/simdag.h"
#include "xbt.h"
#include "scheduling.h"
#include "dax.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dax, EnsembleSched,
                                "Logging specific to daxes");

SD_task_t get_root(xbt_dynar_t dax){
  SD_task_t task;

  xbt_dynar_get_cpy(dax, 0, &task);
  return task;
}

/* Comparison function to sort DAXes increasingly according to their size.
 * Assumption: this size consider all tasks (computations AND transfers). Could
 * be modified to count only compute tasks.
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
 * Arrange the N elements of ARRAY in random order. Only effective if N is much
 * smaller than RAND_MAX; if this may not be the case, use a better random
 * number generator. */
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
}
