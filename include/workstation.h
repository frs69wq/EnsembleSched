/*
 * workstation.h
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#ifndef WORKSTATION_H_
#define WORKSTATION_H_
#include "simdag/simdag.h"

typedef struct _WorkstationAttribute *WorkstationAttribute;
struct _WorkstationAttribute {
  /* Earliest time at wich a workstation is ready to execute a task*/
  double available_at;
  SD_task_t last_scheduled_task;
  int on_off;     /* 1 = ON, 0 = OFF */
  int idle_busy;  /* 1 = Busy, 0 = idle */
  double start_time;

  double total_cost;
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
void SD_workstation_bill(SD_workstation_t, double, double);
int is_on_and_idle(SD_workstation_t);
xbt_dynar_t get_idle_VMs();
xbt_dynar_t get_running_VMs();
xbt_dynar_t get_ending_billing_cycle_VMs();
xbt_dynar_t find_active_VMs_to_stop(int, xbt_dynar_t);
SD_workstation_t find_inactive_VM_to_start();
double compute_current_VM_utilization();
SD_workstation_t select_random(xbt_dynar_t);

#endif /* WORKSTATION_H_ */
