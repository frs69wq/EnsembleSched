/*
 * main.c
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include <getopt.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "simdag/simdag.h"
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
  SD_task_t task;
  scheduling_globals_t globals;

  SD_init(&argc, argv);

  /* get rid off some logs that are useless */
  xbt_log_control_set("sd_daxparse.thresh:critical");
  xbt_log_control_set("surf_workstation.thresh:critical");

  globals = new_scheduling_globals();

  daxes = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
  opterr = 0;

  //TODO add command line arguments: utilization thresholds
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
        {0, 0, 0, 0}
    };

    int option_index = 0;
    flag = getopt_long (argc, argv, "abc:",
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
      XBT_INFO("Algorithm: %s",getAlgorithmName(globals->alg));
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
      XBT_INFO("loading %s", daxname);
      current_dax = SD_daxload(daxname);
      xbt_dynar_foreach(current_dax,cursor,task) {
        if (SD_task_get_kind(task)==SD_TASK_COMP_SEQ){
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
    }
  }

  /* Sanity checks about crucial parameters */
  if (globals->budget && globals->deadline){
    XBT_INFO("The constraints are a budget of $%.0f and a deadline of %.0fs",
        globals->budget, globals->deadline);
  }
  if (ceil(globals->budget/((globals->deadline/3600.)*globals->price))>
      SD_workstation_get_number()){
    XBT_ERROR("The platform file doesn't have enough nodes. Stop here");
    exit(1);
  }

  /* Assign priorities to the DAXes composing the ensemble according to the
   * chosen method: RANDOM (default) or SORTED.
   * Then display the result.
   */
  assign_dax_priorities(daxes, globals->priority_method);
  xbt_dynar_foreach(daxes, cursor, current_dax){
     task = get_root(current_dax);
     XBT_INFO("%30s is assigned a priority of %d",
         SD_task_get_dax_name(task),
         SD_task_get_dax_priority(task));
  }

  /* Assign price to workstation/VM (for sake of simplicity) */
  for(cursor=0; cursor<total_nworkstations; cursor++)
    SD_workstation_set_price(workstations[cursor], globals->price);

  switch(globals->alg){
  case DPDS:
    dpds(daxes, globals);
    break;
  default:
    XBT_ERROR("Algorithm not implemented yet.");
    break;
  }

  /* Cleaning step: Free all the allocated data structures */
  xbt_dynar_foreach(daxes, cursor, current_dax){
    xbt_dynar_foreach(current_dax,cursor2,task) {
      SD_task_free_attribute(task);
      free(SD_task_get_data(task));
      SD_task_destroy(task);
    }
    xbt_dynar_free_container(&current_dax);
  }
  xbt_dynar_free(&daxes);
  free(globals);

  for(cursor=0; cursor<total_nworkstations; cursor++)
    SD_workstation_free_attribute(workstations[cursor]);

  SD_exit();

  return 0;
}
