/*
 * dpds.c
 * SimDAG implementation of the Dynamic Provisioning, Dynamic Scheduling
 * algorihtm from
 * Cost- and Deadline-Constrained Provisioning for Scientific Workflow
 * Ensembles in IaaS Clouds by Maciej Malawski, Gideon Juve, Ewa Deelman and
 * Jarek Nabrzyski published at SC'12.
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */
#include <stdio.h>
#include <math.h>
#include "xbt.h"
#include "simdag/simdag.h"
#include "dax.h"
#include "workstation.h"
#include "task.h"
#include "scheduling.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dpds, EnsembleSched,
                                "Logging specific to the DPDS algorithm");

/* Dynamic provisioning algorithm for DPDS
 * Requires
 *   c: consumed budget
 *   b: total budget
 *   d: deadline
 *   p: price
 *   t: current time
 *   uh: upper utilization threshold
 *   ul: lower utilization threshold
 *   vmax: maximum number of VMs
 *   nVM: number of VMs initially started (added parameter)
 */
void dpds_provision(double c, double b, double d, double p, double t,
                    double uh, double ul, double vmax, int nVM){
  unsigned int i;
  double u;
  xbt_dynar_t VR = get_running_VMs();
  xbt_dynar_t VC = get_ending_billing_cycle_VMs();
  xbt_dynar_t VT = NULL; /* set of VMs to terminate */
  xbt_dynar_t VI = NULL;
  SD_workstation_t v;
  int nT = 0;

  if (((b-c) < (xbt_dynar_length(VC)*p)) || (t>d)){
    nT = xbt_dynar_length(VR) - floor((b-c)/p);
    VT = find_active_VMs_to_stop(nT, VC);
    xbt_dynar_foreach(VT, i, v)
      SD_workstation_terminate(v);
    xbt_dynar_free_container(&VT);
  } else {
    u = compute_current_VM_utilization();
    if ((u > uh) && (xbt_dynar_length(VR) < (vmax*nVM))){
      v = find_inactive_VM_to_start();
      SD_workstation_start(v);
    } else if (u < ul) {
      VI = get_idle_VMs();
      nT = ceil(xbt_dynar_length(VI)/2.);
      VT = find_active_VMs_to_stop(nT, VI);
      xbt_dynar_foreach(VT, i, v)
        SD_workstation_terminate(v);
      xbt_dynar_free_container(&VI);
      xbt_dynar_free_container(&VT);
    }
  }
  xbt_dynar_free_container(&VR);
  xbt_dynar_free_container(&VC);
}

void dpds_schedule(xbt_dynar_t daxes, scheduling_globals_t globals){
  unsigned int i, j;
  int first_call = 1;
  xbt_dynar_t priority_queue = xbt_dynar_new(sizeof (SD_task_t), NULL);
  xbt_dynar_t idleVMs = NULL;
  xbt_dynar_t ready_children = NULL;
  xbt_dynar_t current_dax = NULL, changed = NULL;
  SD_task_t root, t, child;
  SD_workstation_t v;

  idleVMs = get_idle_VMs();

  xbt_dynar_foreach(daxes, i, current_dax){
    root = get_root(current_dax);
    xbt_dynar_push(priority_queue, &root);
  }
  /* Sort the priority queue by increasing value of DAX priority.
   * Tasks that belong to the most important DAX are located toward the end of
   * the dynar. xbt_dynar_pop then return the most important task.
   * Remark: The paper by Malawski et al. does not detail the INSERT function
   * (Algorithm 2, line 5). Then no secondary sort is applied for tasks that
   * belong to the same DAX.
   */
  xbt_dynar_sort(priority_queue, daxPriorityCompareTasks);

  while (first_call || !xbt_dynar_is_empty((changed =
         SD_simulate(globals->deadline-SD_get_clock())))){
    first_call=0;
    if (globals->deadline == SD_get_clock()){
      XBT_INFO("Time's up! Deadline was reached at %.3f", SD_get_clock());
      break;
    }
    XBT_VERB("Simulation stopped at %.3f. %lu tasks changed", SD_get_clock(),
        xbt_dynar_length(changed));
    xbt_dynar_foreach(changed, i, t){
       if (SD_task_get_state(t) == SD_DONE){
         XBT_VERB("%s (%s) has completed", SD_task_get_name(t),
             SD_task_get_dax_name(t));

         /* get the workstation used to compute this task */
         v = (SD_task_get_workstation_list(t))[0];

         /* Set it to idle and add it the list of idle VMs */
         SD_workstation_set_to_idle(v);
         xbt_dynar_push(idleVMs, &v);

         /* add ready children of t to the priority queue */
         ready_children = SD_task_get_ready_children(t);
         XBT_DEBUG("%s has %lu child(ren) to add to the priority queue",
             SD_task_get_name(t),xbt_dynar_length(ready_children));
         xbt_dynar_foreach(ready_children, j, child){
           if (!xbt_dynar_member(priority_queue, &child))
             xbt_dynar_push(priority_queue, &child);
         }
         xbt_dynar_free_container(&ready_children);
       }
    }
    /* Sort again the priority queue as new tasks have been added. */
    xbt_dynar_sort(priority_queue, daxPriorityCompareTasks);
    xbt_dynar_foreach(priority_queue,j,child)
      XBT_DEBUG("%s is in priority queue", SD_task_get_name(child));

    xbt_dynar_free_container(&changed); /* avoid memory leaks */

    while ((!xbt_dynar_is_empty(idleVMs)) &&
           (!xbt_dynar_is_empty(priority_queue))){

      /* This call removes v from the list of idleVMs */
      v = select_random(idleVMs);
      /* Pop the last task from the dynar, i.e. one belonging to the DAX of
       * highest priority.
       */
      xbt_dynar_pop(priority_queue, &t);

      XBT_VERB("Schedule %s (%s) on %s", SD_task_get_name(t),
          SD_task_get_dax_name(t),
          SD_workstation_get_name(v));
      SD_task_schedulel(t, 1, v);
      //handle_resource_dependency(v, t);
      SD_workstation_set_to_busy(v);
     }
  }

  /* Cleaning step once simulation is over */
  xbt_dynar_free_container(&idleVMs);
  xbt_dynar_free_container(&priority_queue);
  xbt_dynar_free_container(&changed);

}

void dpds_init(xbt_dynar_t daxes, scheduling_globals_t globals){
  int i, nVM;
  const SD_workstation_t *workstations = SD_workstation_get_list();

  /* Start by activating nVM VMs
   * Deadline is expressed in seconds, while price is for an hour.
   * Conversion is needed.
   */
  nVM = ceil(globals->budget/((globals->deadline/3600.)*globals->price));
  XBT_INFO("%d VMs are started to begin with", nVM);
  for (i = 0; i < nVM; i++){
    SD_workstation_start(workstations[i]);
  }
  dpds_schedule(daxes, globals);
}
