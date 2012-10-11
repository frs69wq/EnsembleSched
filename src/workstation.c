/*
 * workstation.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include <stdlib.h>
#include <math.h>
#include "workstation.h"
#include "xbt.h"
#include "simdag/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(workstation, EnsembleSched,
                                "Logging specific to workstations");

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/

void SD_workstation_allocate_attribute(SD_workstation_t workstation){
  void *data;
  data = calloc(1,sizeof(struct _WorkstationAttribute));
  SD_workstation_set_data(workstation, data);
}

void SD_workstation_free_attribute(SD_workstation_t workstation){
  free(SD_workstation_get_data(workstation));
  SD_workstation_set_data(workstation, NULL);
}

double SD_workstation_get_available_at( SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  return attr->available_at;
}

void SD_workstation_set_available_at(SD_workstation_t workstation, double time){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->available_at=time;
  SD_workstation_set_data(workstation, attr);
}

SD_task_t SD_workstation_get_last_scheduled_task( SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  return attr->last_scheduled_task;
}

void SD_workstation_set_last_scheduled_task(SD_workstation_t workstation,
                                            SD_task_t task){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->last_scheduled_task=task;
  SD_workstation_set_data(workstation, attr);
}

void SD_workstation_start(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->on_off = 1;
  attr->start_time = SD_get_clock();
  SD_workstation_set_data(workstation, attr);
}

void SD_workstation_terminate(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->on_off = 0;
  attr->start_time = 0.0;
  SD_workstation_set_data(workstation, attr);
}

void SD_workstation_set_to_idle(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->idle_busy=0;
  SD_workstation_set_data(workstation, attr);
}

void SD_workstation_set_to_busy(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->idle_busy=1;
  SD_workstation_set_data(workstation, attr);
}

/* compare workstation name wrt the lexicographic order (Increasing) */
int nameCompareWorkstations(const void *n1, const void *n2) {
  return strcmp(SD_workstation_get_name(*((SD_workstation_t *)n1)),
                SD_workstation_get_name(*((SD_workstation_t *)n2)));
}

/*****************************************************************************/
/*****************************************************************************/
/**************    Functions needed by scheduling algorithms    **************/
/*****************************************************************************/
/*****************************************************************************/


/* Simple function to know whether a workstation/VM can accept tasks for
 * execution. Has to be ON and idle for that.
 */
int is_on_and_idle(SD_workstation_t workstation){
  WorkstationAttribute attr = SD_workstation_get_data(workstation);
  return (attr->on_off && !attr->idle_busy);
}

/* Build an array that contains all the idle workstations/VMs in the platform */
xbt_dynar_t get_idle_VMs(){
  int i;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  xbt_dynar_t idleVMs = xbt_dynar_new(sizeof(SD_workstation_t), NULL);

  for (i=0; i<nworkstations; i++){
    if (is_on_and_idle(workstations[i]))
      xbt_dynar_push(idleVMs, &(workstations[i]));
  }

  return idleVMs;
}

/* Build an array that contains all the busy workstations/VMs in the platform */
xbt_dynar_t get_running_VMs(){
  int i;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  xbt_dynar_t runningVMs = xbt_dynar_new(sizeof(SD_workstation_t), NULL);

  for (i=0; i<nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off)
      xbt_dynar_push(runningVMs, &(workstations[i]));
  }

  return runningVMs;
}

/* Build an array that contains all the workstations/VMs that are "approaching
 * their hourly billing cycle" in the platform
 * Remark: In the paper by Malawski et al., no details are provided about when
 * a VM is "approaching" the end of a paid hour.
 * Assumption: Consider that VMs are ending a billing cycle when they consumed
 * more than 50 minutes of the current hour. */
xbt_dynar_t get_ending_billing_cycle_VMs(){
  int i;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  xbt_dynar_t endingVMs = xbt_dynar_new(sizeof(SD_workstation_t), NULL);

  for (i=0; i<nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    /* To determine how far a VM is from the end of a hourly billing cycle, we
     * compute the time spent between the start of the VM and the current, and
     * keep the time spent in the last hour. As times are expressed in seconds,
     * it amounts to computing the modulo to 3600s=1h.
     * Then the current VM is selected if this modulo is greater than
     * 3000s=50mn.
     */
    if (attr->on_off &&
        ((int)(SD_get_clock() - attr->start_time) % 3600) > 3000)
      xbt_dynar_push(endingVMs, &(workstations[i]));
  }

  return endingVMs;
}

/* Build the set of workstations/VMs that have to be terminated. This function
 * selects how_many VMs from the source set of candidates.
 * Remark: In the paper by Malawski et al., no details are provided about how
 * the VMs are selected in the source set. Moreover, there is no check w.r.t.
 * the size of the source set.
 * Assumptions:
 * 1) If the source set is too small, display a warning and return a smaller
 *    set than expected.
 * 2) Straightforward selection of the VMs in the set. Just pick the how_many
 *    first ones without any consideration of the time remaining until the next
 *    hourly billing cycle.
 */
xbt_dynar_t find_active_VMs_to_stop(int how_many, xbt_dynar_t source){
  int i;
  long unsigned int source_size = xbt_dynar_length(source);
  xbt_dynar_t to_stop = xbt_dynar_new(sizeof(SD_workstation_t), NULL);
  SD_workstation_t v;

  if (how_many > source_size){
    XBT_WARN("Trying to terminate more VMs than what is available (%d > %lu)."
        " Change the number of VMs to terminate to %lu", how_many,
        source_size, source_size);
    how_many = source_size;
  }
  for (i=0; i<how_many; i++){
    /* No advanced selection process. Just pick the how_many first VMs in the
     * source set.
     */
    xbt_dynar_get_cpy(source, i, &v);
    xbt_dynar_push(to_stop, &v);
  }

  return to_stop;
}

/* Return the first inactive workstation/VM (currently set to OFF) that we
 * find in the platform.
 * Remarks:
 * 1) Straightforward selection, all VMs are assumed to the similar
 * 2) It may happen that no such VM is found. This means that the platform file
 *    given as input of the simulator was too small. The simulation cannot
 *    continue while the size of the resource pool represented by the platform
 *    file is not increased. */
SD_workstation_t find_inactive_VM_to_start(){
  int i=0;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  SD_workstation_t v = NULL;

  while (i<nworkstations){
    attr = SD_workstation_get_data(workstations[i]);
    if (!attr->on_off){
      v = workstations[i];
      break;
    }
    i++;
  }

  if (!v){
    XBT_ERROR("Argh. We reached the pool limit. Have to increase the size of"
        " the cluster in the platform file.");
    exit(1);
  }

  return v;
}

/* Determine the current utilization of VM in the system. This utilization is
 * defined in the paper by Malawski et al. as "the percentage of idle VMs over
 * time".
 * Assumption: we translate that as counting the number of active (ON) VMs,
 * counting the number of idle VMs (ON and idle), and then computing the
 * percentage of idle VMs (100*#idle/#active).
 */
double compute_current_VM_utilization(){
  int i=0;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  int nActiveVMs=0, nIdleVms=0;

  for (i=0; i<nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off){
      nActiveVMs++;
      if (!attr->idle_busy)
        nIdleVms++;
    }
  }
  return (100.*nIdleVms)/nActiveVMs;
}


/* Randomly select a workstation in an array. Rely on the rand function
 * provided by math.h
 * Remark: This function actually removes the selected element from the array.
 */
SD_workstation_t select_random(xbt_dynar_t workstations){
  unsigned long i;
  SD_workstation_t v=NULL;
  int nworkstations = xbt_dynar_length(workstations);

  i = (unsigned long) ((nworkstations-1.)*rand()/(RAND_MAX+1.0));
  xbt_dynar_remove_at(workstations, i, &v);
  return v;
}
