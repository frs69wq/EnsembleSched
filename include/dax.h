/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#ifndef DAX_H_
#define DAX_H_
#include "scheduling.h"

SD_task_t get_root(xbt_dynar_t dax);
SD_task_t get_end(xbt_dynar_t dax);

void assign_dax_priorities(xbt_dynar_t, method_t);
double compute_score(xbt_dynar_t);

#endif /* DAX_H_ */
