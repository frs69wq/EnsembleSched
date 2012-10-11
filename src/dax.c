/*
 * dax.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include "simdag/simdag.h"
#include "xbt.h"

SD_task_t get_root(xbt_dynar_t dax){
  SD_task_t task;

  xbt_dynar_get_cpy(dax, 0, &task);
  return task;
}
