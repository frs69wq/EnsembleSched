/*
 * scheduling.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include <string.h>
#include "simdag/simdag.h"
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
  globals->period = 3600.0;
  globals->price = 1.0;
  globals->budget=0.;
  globals->deadline=0.;

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
