/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#ifndef MIN
#  define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

typedef enum {
  DPDS=0,
  WADPDS,
  SPSS,
  OURS
} alg_t;

typedef enum {
  RANDOM=0,
  SORTED
} method_t;

typedef struct _scheduling_globals *scheduling_globals_t;
struct _scheduling_globals {
  alg_t alg;
  method_t priority_method;
  double period;

  double budget;
  double deadline;
  double price;       /* VM hourly cost*/
  double uh;          /* upper utilization threshold */
  double ul;          /* lower utilization threshold */
  double vmax;

  /* time before a started VM actually becomes available */
  double provisioning_delay;
  int nVM;            /* Number of VMs that have been initially started */
};

scheduling_globals_t new_scheduling_globals();

char* getAlgorithmName(alg_t);
alg_t getAlgorithmByName(char*);

void handle_resource_dependency(sg_host_t, SD_task_t);

/*****************************************************************************/
/*************       Scheduling algorithms entry points        ***************/
/*****************************************************************************/
void dpds(xbt_dynar_t, scheduling_globals_t);
double compute_budget_consumption();

#endif /* SCHEDULING_H_ */
