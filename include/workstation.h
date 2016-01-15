/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */

#ifndef WORKSTATION_H_
#define WORKSTATION_H_
#include "simgrid/simdag.h"

typedef struct _WorkstationAttribute *WorkstationAttribute;
struct _WorkstationAttribute {
  /* Earliest time at wich a workstation is ready to execute a task*/
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
void SD_workstation_allocate_attribute(SD_workstation_t );
void SD_workstation_free_attribute(SD_workstation_t );

/*
 * Accessors
 */
void SD_workstation_set_price(SD_workstation_t, double);
void SD_workstation_set_provisioning_delay(SD_workstation_t, double);
double SD_workstation_get_available_at(SD_workstation_t);
void SD_workstation_set_available_at(SD_workstation_t, double);
SD_task_t SD_workstation_get_last_scheduled_task (SD_workstation_t);
void SD_workstation_set_last_scheduled_task (SD_workstation_t, SD_task_t);
void SD_workstation_start(SD_workstation_t);
void SD_workstation_terminate(SD_workstation_t);
void SD_workstation_set_to_idle(SD_workstation_t);
void SD_workstation_set_to_busy(SD_workstation_t);

/*
 * Comparators
 */
int nameCompareWorkstations(const void *, const void *);

/* Other functions needed by scheduling algorithms */
int is_on_and_idle(SD_workstation_t);
xbt_dynar_t get_idle_VMs();
xbt_dynar_t get_running_VMs();
xbt_dynar_t get_ending_billing_cycle_VMs(double, double);
xbt_dynar_t find_active_VMs_to_stop(int, xbt_dynar_t);
SD_workstation_t find_inactive_VM_to_start();
double compute_current_VM_utilization();
SD_workstation_t select_random(xbt_dynar_t);

#endif /* WORKSTATION_H_ */
