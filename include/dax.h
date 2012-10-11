/*
 * dax.h
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#ifndef DAX_H_
#define DAX_H_
#include "scheduling.h"

SD_task_t get_root(xbt_dynar_t dax);

void assign_dax_priorities(xbt_dynar_t, method_t);

#endif /* DAX_H_ */
