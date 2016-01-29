/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#ifndef HOST_H_
#define HOST_H_
#include "simgrid/simdag.h"

typedef struct _HostAttribute *HostAttribute;
struct _HostAttribute {
  /* Earliest time at which a host is ready to execute a task*/
  double available_at;
  SD_task_t last_scheduled_task;
  int on_off;     /* 1 = ON, 0 = OFF */
  int idle_busy;  /* 1 = Busy, 0 = idle */
  double start_time;

  double price;
  double provisioning_delay;
  double total_cost;

  SD_task_t booting;
  //TODO add necessary attributes
};

/*
 * Creator and destructor
 */
void sg_host_allocate_attribute(sg_host_t );
void sg_host_free_attribute(sg_host_t );

/*
 * Accessors
 */
void sg_host_set_price(sg_host_t, double);
void sg_host_set_provisioning_delay(sg_host_t, double);
double sg_host_get_available_at(sg_host_t);
void sg_host_set_available_at(sg_host_t, double);
SD_task_t sg_host_get_last_scheduled_task (sg_host_t);
void sg_host_set_last_scheduled_task (sg_host_t, SD_task_t);
void sg_host_start(sg_host_t);
void sg_host_terminate(sg_host_t);
void sg_host_set_to_idle(sg_host_t);
void sg_host_set_to_busy(sg_host_t);

/*
 * Comparators
 */
int nameCompareHosts(const void *, const void *);

/* Other functions needed by scheduling algorithms */
int is_on_and_idle(sg_host_t);
xbt_dynar_t get_idle_VMs();
xbt_dynar_t get_running_VMs();
xbt_dynar_t get_ending_billing_cycle_VMs(double, double);
xbt_dynar_t find_active_VMs_to_stop(int, xbt_dynar_t);
sg_host_t find_inactive_VM_to_start();
double compute_current_VM_utilization();
sg_host_t select_random(xbt_dynar_t);

#endif /* HOST_H_ */
