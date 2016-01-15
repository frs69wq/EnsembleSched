/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */
#include <getopt.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "simgrid/simdag.h"
#include "xbt.h"
#include "dax.h"
#include "task.h"
#include "workstation.h"
#include "scheduling.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(EnsembleSched,
    "Logging specific to EnsembleSched");

int main(int argc, char **argv) {
  unsigned int flag, cursor, cursor2;
  char *platform_file = NULL, *daxname = NULL, *priority=NULL;
  int total_nworkstations = 0;
  const SD_workstation_t *workstations = NULL;
  xbt_dynar_t daxes = NULL, current_dax = NULL;
  int completed_daxes = 0;
  SD_task_t task;
  scheduling_globals_t globals;
  WorkstationAttribute attr;
  double total_cost = 0.0, score = 0.0;

  SD_init(&argc, argv);

  /* get rid off some logs that are useless */
  xbt_log_control_set("sd_daxparse.thresh:critical");
  xbt_log_control_set("surf_workstation.thresh:critical");
  xbt_log_control_set("root.fmt:[%9.3r]%e[%13c/%7p]%e%m%n");

  globals = new_scheduling_globals();

  daxes = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
  opterr = 0;

  while (1){
    static struct option long_options[] = {
        {"alg", 1, 0, 'a'},
        {"platform", 1, 0, 'b'},
        {"dax", 1, 0, 'c'},
        {"priority", 1, 0, 'd'},
        {"deadline", 1, 0, 'e'},
        {"budget", 1, 0, 'f'},
        {"price", 1, 0, 'g'},
        {"period", 1, 0, 'h'},
        {"uh", 1, 0, 'i'},
        {"ul", 1, 0, 'j'},
        {"provisioning_delay", 1, 0, 'k'},
        {"silent", 0, 0, 'y'},
        {"dump", 1, 0, 'z'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    flag = getopt_long (argc, argv, "",
                        long_options, &option_index);

    /* Detect the end of the options. */
    if (flag == -1)
      break;

    switch (flag) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
        printf (" with arg %s", optarg);
        printf ("\n");
      break;
    case 'a': /* Algorithm name */
      /* DPDS, WA-DPDS, SPSS, Ours*/
      globals->alg = getAlgorithmByName(optarg);
      break;
    case 'b':
      platform_file = optarg;
      SD_create_environment(platform_file);
      total_nworkstations = SD_workstation_get_number();
      workstations = SD_workstation_get_list();

      /* Sort the hosts by name for sake of simplicity */
      qsort((void *)workstations,total_nworkstations, sizeof(SD_workstation_t),
          nameCompareWorkstations);

      for(cursor=0; cursor<total_nworkstations; cursor++){
        SD_workstation_allocate_attribute(workstations[cursor]);
      }
      break;
    case 'c':
      /* List of DAGs to schedule concurrently (just file names here) */
      daxname = optarg;
      XBT_DEBUG("Loading %s", daxname);
      current_dax = SD_daxload(daxname);
      xbt_dynar_foreach(current_dax,cursor,task) {
        if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ){
          SD_task_watch(task, SD_DONE);
        }
        SD_task_allocate_attribute(task);
        SD_task_set_dax_name(task, daxname);
      }
      xbt_dynar_push(daxes,&current_dax);
      break;
    case 'd':
      priority = optarg;
      if (!strcmp(priority,"random"))
        globals->priority_method = RANDOM;
      else if (!strcmp(priority, "sorted"))
        globals->priority_method = SORTED;
      else {
        XBT_ERROR("Unknown priority setting method.");
        exit(1);
      }
      break;
    case 'e':
      globals->deadline = atof(optarg);
      break;
    case 'f':
      globals->budget = atof(optarg);
      break;
    case 'g':
      globals->price = atof(optarg);
      break;
    case 'h':
      globals->period = atof(optarg);
      break;
    case 'i':
      globals->uh = atof(optarg);
      break;
    case 'j':
      globals->ul = atof(optarg);
      break;
    case 'k':
      globals->provisioning_delay = atof(optarg);
      break;
    case 'y':
      xbt_log_control_set("root.thresh:critical");
      break;
    case 'z':
      break;
    }
  }
  /* Display some information about the current run */
  XBT_INFO("Algorithm: %s",getAlgorithmName(globals->alg));
  XBT_INFO("  Priority method: %s",
      globals->priority_method ? "SORTED" : "RANDOM");
  XBT_INFO("  Dynamic provisioning period: %.0fs", globals->period);
  XBT_INFO("  Lower utilization threshold: %.2f%%", globals->ul);
  XBT_INFO("  Upper utilization threshold: %.2f%%", globals->uh);

  XBT_INFO("Platform: %s (%d potential VMs)", platform_file,
      SD_workstation_get_number());
  XBT_INFO("  VM hourly cost: $%f", globals->price);
  XBT_INFO("  VM provisioning delay: %.0fs", globals->provisioning_delay);
  if (ceil(globals->budget/((globals->deadline/3600.)*globals->price))>
      SD_workstation_get_number()){
    XBT_ERROR("The platform file doesn't have enough nodes. Stop here");
    exit(1);
  }
  /* Assign price and provisioning delay to workstation/VM (for the sake of
   * simplicity) */
  for(cursor=0; cursor<total_nworkstations; cursor++){
    SD_workstation_set_price(workstations[cursor], globals->price);
    SD_workstation_set_provisioning_delay(workstations[cursor],
        globals->provisioning_delay);
  }

  XBT_INFO("Ensemble: %lu DAXes", xbt_dynar_length(daxes));
  /* Assign priorities to the DAXes composing the ensemble according to the
   * chosen method: RANDOM (default) or SORTED.
   * Then display the result.
   */
  assign_dax_priorities(daxes, globals->priority_method);
  xbt_dynar_foreach(daxes, cursor, current_dax){
     task = get_root(current_dax);
     XBT_INFO("  %s", SD_task_get_dax_name(task));
     XBT_INFO("    Priority: %d", SD_task_get_dax_priority(task));
  }

  XBT_INFO("Scheduling constraints:");
  /* Sanity checks about crucial parameters */
  if (globals->budget && globals->deadline){
    XBT_INFO("  Budget: $%.0f", globals->budget);
    XBT_INFO("  Deadline: %.0fs", globals->deadline);
  } else {
    XBT_ERROR("  A budget and a deadline have to be provided. Stop here");
    exit(1);
  }
  printf("\n");
  switch(globals->alg){
  case DPDS:
    dpds(daxes, globals);
    break;
  default:
    XBT_ERROR("Algorithm not implemented yet.");
    break;
  }
  printf("\n");

  /* Post-processing of simulation */
  /* Determine how many DAXes are complete */
  xbt_dynar_foreach(daxes, cursor, current_dax){
    task = get_end(current_dax);
    if (SD_task_get_state(task) == SD_DONE){
      completed_daxes++;
    }
  }

  /* Terminate all VMs and do the final billing*/
  for (cursor = 0; cursor < total_nworkstations; cursor++){
    attr = SD_workstation_get_data(workstations[cursor]);
    if (attr->on_off)
      SD_workstation_terminate(workstations[cursor]);
    total_cost += attr->total_cost;
  }

  /* Compute the score of the schedule */
  score = compute_score(daxes);

  /* Display main information about the schedule */
  XBT_INFO("Makespan: %.3f seconds.", SD_get_clock());
  XBT_INFO("Success rate: %d/%lu", completed_daxes, xbt_dynar_length(daxes));
  XBT_INFO("Total cost: $%.2f", total_cost);
  XBT_INFO("Score: %f", score);
  XBT_INFO("Cost/Budget: %f", total_cost / globals->budget);
  XBT_INFO("Makespan/Deadline: %f", SD_get_clock() / globals->deadline);

  /* Cleaning step: Free all the allocated data structures */
  xbt_dynar_foreach(daxes, cursor, current_dax){
    xbt_dynar_foreach(current_dax, cursor2, task) {
      SD_task_free_attribute(task);
      free(SD_task_get_data(task));
      SD_task_destroy(task);
    }
    xbt_dynar_free_container(&current_dax);
  }
  xbt_dynar_free(&daxes);
  free(globals);

  for(cursor = 0; cursor < total_nworkstations; cursor++)
    SD_workstation_free_attribute(workstations[cursor]);

  SD_exit();

  return 0;
}
