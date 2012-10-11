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
  alg_t alg;
  method_t  priority_method = RANDOM;
  double deadline=0., budget=0.;

  SD_init(&argc, argv);

  /* get rid off some logs that are useless */
  xbt_log_control_set("sd_daxparse.thresh:critical");
  xbt_log_control_set("surf_workstation.thresh:critical");


  daxes = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
  opterr = 0;

  //TODO add command line arguments: utilization thresholds
  while (1){
    static struct option long_options[] = {
        {"alg", required_argument, 0, 'a'},
        {"platform", required_argument, 0, 'b'},
        {"dax", required_argument, 0, 'c'},
        {"priority", required_argument, 0, 'd'},
        {"deadline", required_argument, 0, 'e'},
        {"budget", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    flag = getopt_long (argc, argv, "abc:",
                        long_options, &option_index);

    /* Detect the end of the options. */
    if (flag == -1)
      break;

//          (flag = getopt (argc, argv, "a:p:d:")) != -1)

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
      alg = getAlgorithmByName(optarg);
      XBT_INFO("Algorithm: %s",getAlgorithmName(alg));
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
        /* Set the workstations to off and idle at the beginning */
        SD_workstation_terminate(workstations[cursor]);
        SD_workstation_set_to_idle(workstations[cursor]);
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
        priority_method = RANDOM;
      else if (!strcmp(priority, "sorted"))
        priority_method = SORTED;
      else {
        XBT_ERROR("Unknown priority setting method.");
        exit(1);
      }
      break;
    case 'e':
      deadline = atof(optarg);
      break;
    case 'f':
      budget = atof(optarg);
      break;
    }
  }

  if (budget && deadline){
    XBT_INFO("The constraints are a budget of $%.0f and a deadline of %.0fs",
        budget, deadline);
  }

  assign_dax_priorities(daxes, priority_method);
  xbt_dynar_foreach(daxes, cursor, current_dax){
     task = get_root(current_dax);
     XBT_INFO("%30s is assigned a priority of %d",
         SD_task_get_dax_name(task),
         SD_task_get_dax_priority(task));
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

  for(cursor=0; cursor<total_nworkstations; cursor++)
    SD_workstation_free_attribute(workstations[cursor]);

  SD_exit();

  return 0;
}
