/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */
#include <string.h>
#include "simgrid/simdag.h"
#include "workstation.h"
#include "scheduling.h"
#include "xbt.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(scheduling, EnsembleSched,
                                "Logging specific to scheduling");

scheduling_globals_t new_scheduling_globals(){
  scheduling_globals_t globals =
      (scheduling_globals_t) calloc (1, sizeof(struct _scheduling_globals));

  /* set some default values */
  globals->priority_method = RANDOM;
  globals->period = 90.0; /* value found in the source code of
                             cloudworkflowsim */
  globals->price = 1.0;
  globals->budget = 0.;
  globals->deadline = 0.;
  globals->uh = 90; /* value found in the source code of cloudworkflowsim */
  globals->ul = 70; /* value found in the source code of cloudworkflowsim */
  globals->vmax = 1.0;
  globals->provisioning_delay = 0.;

  return globals;
}

char* getAlgorithmName(alg_t a){
  switch (a){
    case DPDS: return "DPDS";
    case WADPDS: return "WA-DPDS";
    case SPSS: return "SPSS";
    case OURS: return "OURS";
    default: XBT_ERROR("Unknown algorithm");
       exit(1);
  }
}

alg_t getAlgorithmByName(char* name) {
  if (!strcmp(name,"DPDS"))
     return DPDS;
  else if (!strcmp(name,"WA-DPDS"))
     return WADPDS;
  else if (!strcmp(name,"SPSS"))
     return SPSS;
  else if (!strcmp(name,"OURS"))
     return OURS;
  else {
     XBT_ERROR("Unknown algorithm");
     exit (1);
  }
}


/* When some independent tasks are scheduled on the same resource, the SimGrid
 * kernel start them in parallel as soon as possible even though the scheduler
 * assumed a sequential execution. This function addresses this issue by
 * enforcing that sequential execution wanted by the scheduler. A resource
 * dependency is added to that extent.
 */
void handle_resource_dependency(SD_workstation_t workstation, SD_task_t task){
  /* Get the last task executed on this workstation */
  SD_task_t source = SD_workstation_get_last_scheduled_task(workstation);

  /* If such a task exists, is still in the system (scheduled or running) and is
   * not already a predecessor of the current task, create a resource dependency
   */
  if (source && (SD_task_get_state(source)!= SD_DONE) &&
      !SD_task_dependency_exists(source, task))
    SD_task_dependency_add("resource", NULL, source, task);

  /* update the information on what is the last task executed on this
   * workstation */
  SD_workstation_set_last_scheduled_task(workstation, task);
}


/* Determine how much money has already been spent. Each workstation/VM has an
 * attribute that sums the cost (#hours*price) for each period in which the VM
 * is on.
 */
double compute_budget_consumption(){
  double consumed_budget = 0.0;
  int i=0;
  WorkstationAttribute attr;
  const SD_workstation_t *workstations = SD_workstation_get_list ();
  int nworkstations = SD_workstation_get_count ();

  for(i=0;i<nworkstations;i++){
    attr = SD_workstation_get_data(workstations[i]);
    consumed_budget += attr->total_cost;
    if (attr->on_off){
      XBT_DEBUG("%s : Account for %d consumed hours",
          SD_workstation_get_name(workstations[i]),
          (int)(SD_get_clock()-attr->start_time)/3600);
      consumed_budget += (((int)(SD_get_clock()-attr->start_time)/3600))*attr->price;
    }
  }

  return consumed_budget;
}
