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
double get_random_number_between(double x, double y) {
  double r;

  r = x + (y-x)*rand()/(RAND_MAX+1.0);
  return r;
}

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

int is_on_and_idle(SD_workstation_t workstation){
  WorkstationAttribute attr = SD_workstation_get_data(workstation);
  return (attr->on_off && !attr->idle_busy);
}

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

xbt_dynar_t get_ending_billing_cycle_VMs(){
  int i;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  xbt_dynar_t endingVMs = xbt_dynar_new(sizeof(SD_workstation_t), NULL);

  for (i=0; i<nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off &&
        ((int)(SD_get_clock() - attr->start_time) % 3600) > 3000)
      xbt_dynar_push(endingVMs, &(workstations[i]));
  }

  return endingVMs;
}

xbt_dynar_t find_active_VMs_to_stop(int how_many, xbt_dynar_t source){
  int i;
  xbt_dynar_t to_stop = xbt_dynar_new(sizeof(SD_workstation_t), NULL);
  SD_workstation_t v;
  //TODO add an assert to compare length of source and how_many

  for (i=0; i<how_many; i++){
    xbt_dynar_get_cpy(source, i, &v);
    xbt_dynar_push(to_stop, &v);
  }

  return to_stop;
}

SD_workstation_t find_unactive_VM_to_start(){
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

double compute_current_VM_utilization(){
  int i=0;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_number ();
  WorkstationAttribute attr;
  int nActiveVMs=0, nBusyVMs=0;

  for (i=0; i<nworkstations; i++){
    attr = SD_workstation_get_data(workstations[i]);
    if (attr->on_off){
      nActiveVMs++;
      if (attr->idle_busy)
        nBusyVMs++;
    }
  }
  return (100.*nBusyVMs)/nActiveVMs;
}

SD_workstation_t select_random(xbt_dynar_t idleVMs){
  unsigned long i;
  SD_workstation_t v=NULL;
  int nIdleVMs = xbt_dynar_length(idleVMs);
  i = (unsigned long) get_random_number_between(0.0, (double)(nIdleVMs-1.));
  xbt_dynar_remove_at(idleVMs, i, &v);
  return v;
}
