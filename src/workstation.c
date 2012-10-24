/*
 * workstation.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "workstation.h"
#include "scheduling.h"
#include "task.h"
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
  WorkstationAttribute data;
  data = calloc(1,sizeof(struct _WorkstationAttribute));
  data->total_cost = 0;
  /* Set the workstations to off and idle at the beginning */
  data->on_off = 0;
  data->idle_busy = 0;
  data->booting=NULL;
  SD_workstation_set_data(workstation, data);
}

void SD_workstation_free_attribute(SD_workstation_t workstation){
  free(SD_workstation_get_data(workstation));
  SD_workstation_set_data(workstation, NULL);
}

void SD_workstation_set_price(SD_workstation_t workstation, double price){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->price = price;
  SD_workstation_set_data(workstation, attr);
}

void SD_workstation_set_provisioning_delay(SD_workstation_t workstation,
    double delay){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->provisioning_delay= delay;
  SD_workstation_set_data(workstation, attr);
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

/* Activate a resource, i.e., act as if a VM is started on a workstation. This
 * amounts to :
 * - setting attributes to 'ON' and 'idle'
 * - Resetting the start time of the workstation to the current time
 * - bill at least the first hour
 * If a provisioning delay is needed before a VM is actually available for
 * executing task, this function :
 * - creates a task whose name is "Booting " followed by the workstation name
 * - schedules this task on the workstation
 * - Ensures that no compute task can be executed before the completion of this
 *   booting task.
 */
void SD_workstation_start(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  char name[1024];

  attr->on_off = 1;
  attr->idle_busy = 0;
  attr->start_time = SD_get_clock();
  attr->total_cost += attr->price;
  if (attr->provisioning_delay){
    sprintf(name,"Booting %s", SD_workstation_get_name(workstation));

    attr->booting = SD_task_create_comp_seq(name, NULL,
        attr->provisioning_delay*SD_workstation_get_power(workstation));
    SD_task_schedulel(attr->booting, 1, workstation);
    attr->available_at += attr->provisioning_delay;
    handle_resource_dependency(workstation, attr->booting);
  }
  XBT_DEBUG("VM started on %s: Total cost is now $%f for this workstation",
      SD_workstation_get_name(workstation),
      attr->total_cost);
  SD_workstation_set_data(workstation, attr);
}

/* Disable a resource, i.e., act as a VM is terminated on a workstation. This
 * amounts to:
 * - setting attributes to 'OFF'
 * - Resetting the start time of the workstation to 0 (just in case)
 * - Do some accounting. The time (in seconds) spent since the last time
 *   workstation/VM was started (state set to ON) is rounded down (as the first
 *   hour is charged on activation) and multiplied by the hour price.
 * If a provisioning delay is needed before a VM is actually available for
 * executing task, this function destroys the booting task created by the
 * SD_workstation_start function.
*/
void SD_workstation_terminate(SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  double duration = SD_get_clock() - attr->start_time;

  if (attr->booting)
    SD_task_destroy(attr->booting);
  attr->on_off = 0;
  attr->start_time = 0.0;
  attr->total_cost += ((((int) duration / 3600)) * attr->price);

  XBT_DEBUG("VM stopped on %s: Total cost is now $%f for this workstation",
      SD_workstation_get_name(workstation),
      attr->total_cost);
  SD_workstation_set_data(workstation, attr);
}

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

  for (i = 0; i < nworkstations; i++){
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

  for (i = 0; i < nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off)
      xbt_dynar_push(runningVMs, &(workstations[i]));
  }

  return runningVMs;
}

/* Build an array that contains all the workstations/VMs that are "approaching
 * their hourly billing cycle" in the platform
 * Remark: In the paper by Malawski et al., no details are provided about when
 * a VM is "approaching" the end of a paid hour. This is hard coded in the
 * source code of cloudworkflowsim: 90s (provisioner interval, a.k.a period) +
 * 1s (optimistic deprovisioning delay)
 */
xbt_dynar_t get_ending_billing_cycle_VMs(double period, double margin){
  int i;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  xbt_dynar_t endingVMs = xbt_dynar_new(sizeof(SD_workstation_t), NULL);

  for (i = 0; i < nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    /* To determine how far a VM is from the end of a hourly billing cycle, we
     * compute the time spent between the start of the VM and the current, and
     * keep the time spent in the last hour. As times are expressed in seconds,
     * it amounts to computing the modulo to 3600s=1h.
     * Then the current VM is selected if this modulo is greater than
     * 3600-period-margin.
     */
    if (attr->on_off && ((int)(SD_get_clock() - attr->start_time) % 3600) >
                         (3600-period-margin))
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
 *    hourly billing cycle. Just check if the VM is not currently executing a
 *    task
 */
xbt_dynar_t find_active_VMs_to_stop(int how_many, xbt_dynar_t source){
  int i, found;
  long unsigned int source_size = xbt_dynar_length(source);
  xbt_dynar_t to_stop = xbt_dynar_new(sizeof(SD_workstation_t), NULL);
  SD_workstation_t v;

  if (how_many > source_size){
    XBT_WARN("Trying to terminate more VMs than what is available (%d > %lu)."
        " Change the number of VMs to terminate to %lu", how_many,
        source_size, source_size);
    how_many = source_size;
  }

  i = 0;
  found = 0;

  while(found < how_many && i < xbt_dynar_length(source)){
    /* No advanced selection process. Just pick the how_many first idle VMs
     * in the source set.
     */
    xbt_dynar_get_cpy(source, i, &v);
    WorkstationAttribute attr = SD_workstation_get_data(v);
    if (!attr->idle_busy){
      xbt_dynar_push(to_stop, &v);
      found++;
    }
    i++;
  }

  if (found < how_many)
    XBT_WARN("Trying to terminate too many VMs, some are busy.."
        " Change the number of VMs to terminate to %d", found);

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

  while (i < nworkstations){
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
 * The source code shows that it is the number of busy VMs divided by the total
 * number of active VMs (busy and idle)
 */
double compute_current_VM_utilization(){
  int i=0;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  int nActiveVMs = 0, nBusyVMs = 0;

  for (i = 0; i < nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off){
      nActiveVMs++;
      if (!attr->idle_busy)
        nBusyVMs++;
    }
  }
  return (100.*nBusyVMs)/nActiveVMs;
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
