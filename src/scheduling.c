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

void handle_resource_dependency(SD_workstation_t workstation, SD_task_t task){
  if (SD_workstation_get_last_scheduled_task(workstation) &&
      !SD_task_dependency_exists(
          SD_workstation_get_last_scheduled_task(workstation), task))
    SD_task_dependency_add("resource", NULL,
                           SD_workstation_get_last_scheduled_task(workstation),
                                                                  task);
    SD_workstation_set_last_scheduled_task(workstation, task);
}
