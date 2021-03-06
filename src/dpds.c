/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * dpds.c
 * SimDAG implementation of the Dynamic Provisioning, Dynamic Scheduling
 * Algorithm from
 * Cost- and Deadline-Constrained Provisioning for Scientific Workflow
 * Ensembles in IaaS Clouds by Maciej Malawski, Gideon Juve, Ewa Deelman and
 * Jarek Nabrzyski published at SC'12.
 */
#include <stdio.h>
#include <math.h>
#include "xbt.h"
#include "simgrid/simdag.h"
#include "dax.h"
#include "host.h"
#include "task.h"
#include "scheduling.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dpds, EnsembleSched, "Logging specific to the DPDS algorithm");

/* Dynamic provisioning algorithm for DPDS
 * Requires
 *   c: consumed budget
 *   t: current time
 *   nVM: number of VMs initially started (added parameter)
 *   budget (in scheduling_globals_t)
 *   deadline (in scheduling_globals_t)
 *   price (in scheduling_globals_t)
 *   uh: upper utilization threshold (in scheduling_globals_t)
 *   ul: lower utilization threshold (in scheduling_globals_t)
 *   vmax: maximum number of VMs (in scheduling_globals_t)
 */
void dpds_provision(double c, double t, scheduling_globals_t globals){
  unsigned int i;
  double u;
  xbt_dynar_t VR = get_running_VMs();
  xbt_dynar_t VC = get_ending_billing_cycle_VMs(globals->period, 1.);
  xbt_dynar_t VT = NULL; /* set of VMs to terminate */
  xbt_dynar_t VI = NULL;
  sg_host_t v;
  int nT = 0;

  if (((globals->budget-c) < (xbt_dynar_length(VC)*globals->price)) || (t > globals->deadline)){
    nT = xbt_dynar_length(VR) - floor((globals->budget-c)/globals->price);

    XBT_VERB("$%f remain and %zu VMs are close to their billing cycle. Have to stop %d VMs",
             globals->budget-c, xbt_dynar_length(VC), nT);

    VT = find_active_VMs_to_stop(nT, VC);
    xbt_dynar_foreach(VT, i, v){
      XBT_VERB("Terminate %s", sg_host_get_name(v));
      sg_host_terminate(v);
    }
    xbt_dynar_free_container(&VT);
  } else {
    u = compute_current_VM_utilization();

    if ((u > globals->uh) && (xbt_dynar_length(VR) < (globals->vmax*globals->nVM))){
      /* WARNING: a VM can be start while the budget is already spent! An extra test should be added */
      XBT_VERB("%.2f is above upper threshold and some VMs have been stopped before (%zu < %f). Start a new VM ...",
          u, xbt_dynar_length(VR), (globals->vmax*globals->nVM));

      v = find_inactive_VM_to_start();
      sg_host_start(v);
    } else if (u < globals->ul) {
      VI = get_idle_VMs();
      nT = ceil(xbt_dynar_length(VI)/2.);
      XBT_VERB("%.2f is under lower threshold. Have to stop %d VMs", u, nT);
      VT = find_active_VMs_to_stop(nT, VI);
      xbt_dynar_foreach(VT, i, v){
        XBT_VERB("Terminate %s", sg_host_get_name(v));
        sg_host_terminate(v);
      }
      xbt_dynar_free_container(&VI);
      xbt_dynar_free_container(&VT);
    }
  }
  xbt_dynar_free_container(&VR);
  xbt_dynar_free_container(&VC);
}


/* (adapted) Implementation of Algorithm 2 on page 3 of the paper by Malawski et al. Use the global scheduling data
 * structure for convenience.
 */
void dpds_schedule(xbt_dynar_t daxes, scheduling_globals_t globals){
  unsigned int i, j;
  int first_call = 1, step = 1;
  int completed_daxes = 0;
  double consumed_budget;
  xbt_dynar_t priority_queue;
  xbt_dynar_t idleVMs = NULL;
  xbt_dynar_t ready_children = NULL;
  xbt_dynar_t current_dax = NULL, changed = NULL;
  SD_task_t root, t, child;
  sg_host_t v;

  /* Initialization step: lines 2 to 6 */
  priority_queue = xbt_dynar_new(sizeof (SD_task_t), NULL);
  idleVMs = get_idle_VMs();

  xbt_dynar_foreach(daxes, i, current_dax){
    root = get_root(current_dax);
    xbt_dynar_push(priority_queue, &root);
  }

  /* Sort the priority queue by increasing value of DAX priority.
   * Tasks that belong to the most important DAX are located toward the end ofthe dynar. xbt_dynar_pop then return the
   * most important task.
   * Remark: The paper by Malawski et al. does not detail the INSERT function(Algorithm 2, line 5). Then no secondary
   * sort is applied for tasks that belong to the same DAX.
   */
  xbt_dynar_sort(priority_queue, daxPriorityCompareTasks);

  do{
    /* Main scheduling loop: lines 7 to 16 */
    /* Remark: order is changed w.r.t to the article (lines 13-15 then lines 8-12) but not the behavior as there is a
     * specific first call.
     */

    /* This do-while external loop is to ensure that we call the main scheduling loop again even though no event
     * occurred during a complete provisioning period.
     */
    while (first_call || ((completed_daxes < xbt_dynar_length(daxes)) &&
            ((step*globals->period - SD_get_clock())<0.00001 ||
            !xbt_dynar_is_empty((changed = SD_simulate(MIN(step*globals->period,globals->deadline) - SD_get_clock())))
            ))){
      /* Apart of the first specific call, the simulation is suspended when
       *  - All the DAXes are done (no more work to be done, why continue?)
       *  - a watch point is reached, meaning a compute task has finished
       *  - a provisioning period has ended
       *  - the deadline is met
       */
      /* Handling specific stopping conditions */
      first_call=0;
      if (((step*globals->period)-SD_get_clock())<0.00001){
        XBT_DEBUG("End of a period of %.0f seconds. Start a new one", globals->period);
        step++;

        /* Compute current budget consumption */
        consumed_budget = compute_budget_consumption();
        XBT_VERB("$%f have already been spent", consumed_budget);

        /* Call dpds_provision*/
        XBT_DEBUG("Dynamic Provisioning at time %f", SD_get_clock());
        dpds_provision(consumed_budget, SD_get_clock(), globals);
        /* It may have change the set of idle VMs, recompute it */
        xbt_dynar_free_container(&idleVMs); /*avoid memory leaks */
        idleVMs = get_idle_VMs();
        continue;
      }
      if (globals->deadline <= SD_get_clock()){
        XBT_INFO("Time's up! Deadline was reached at %.3f", SD_get_clock());
        break;
      }

      /* Typical loop body*/
      /* Action on completion of a task (lines 13 to 15) */

      xbt_dynar_foreach(changed, i, t){
        /* If VM have a provisioning delay, a task whose name starts by "Booting" has been created. No action taken
         * upon completion of such a task apart from displaying some verbose output.
         */
        if (!strncmp(SD_task_get_name(t), "Booting", 7)){
          XBT_VERB("%s is done", SD_task_get_name(t));
          continue;
        }

        if (SD_task_get_kind(t) == SD_TASK_COMP_SEQ && SD_task_get_state(t) == SD_DONE){
          XBT_VERB("%s (from %s) has completed", SD_task_get_name(t), SD_task_get_dax_name(t));

          /* get the host used to compute this task */
          v = (SD_task_get_workstation_list(t))[0];

          /* Set it to idle and add it the list of idle VMs */
          sg_host_set_to_idle(v);
          xbt_dynar_push(idleVMs, &v);

          /* Detection of the completion of a workflow */
          if (!strcmp(SD_task_get_name(t), "end")){
            XBT_INFO("%s: Complete!", SD_task_get_dax_name(t));
            completed_daxes++;
          }
          /* add ready children of t to the priority queue */
          ready_children = SD_task_get_ready_children(t);
          xbt_dynar_foreach(ready_children, j, child){
            /* Ensure that a task is not added more than once to the queue. May occur as soon as a task as more than
             * one parent. */
            if (!xbt_dynar_member(priority_queue, &child))
              xbt_dynar_push(priority_queue, &child);
          }
          xbt_dynar_free_container(&ready_children); /* avoid memory leaks */
        }
      }

      /* Sort again the priority queue as new tasks have been added. */
      xbt_dynar_sort(priority_queue, daxPriorityCompareTasks);
      /* Display the current contents of the priority queue as debug information*/
      xbt_dynar_foreach(priority_queue,j,child)
        XBT_DEBUG("%s is in priority queue", SD_task_get_name(child));

      /* Task scheduling part (lines 8 to 12) */
      while ((!xbt_dynar_is_empty(idleVMs)) && (!xbt_dynar_is_empty(priority_queue))){

        /* Remove a random VM from the list of idleVMs and set it as busy */
        v = select_random(idleVMs);
        sg_host_set_to_busy(v);

        /* Pop the last task from the queue, i.e. one belonging to the DAX of highest priority. */
        xbt_dynar_pop(priority_queue, &t);

        XBT_VERB("Schedule %s (%s) on %s", SD_task_get_name(t), SD_task_get_dax_name(t), sg_host_get_name(v));

        SD_task_schedulel(t, 1, v);
        handle_resource_dependency(v, t);
      }
    }
  } while ((globals->deadline - SD_get_clock() > 0.00001) && (completed_daxes < xbt_dynar_length(daxes)));

  /* We may have reached the deadline while some tasks are still running. Let's call SD_simulate one last time to let
   * them finish.
   */
  if (globals->deadline - SD_get_clock() < 0.00001){
    XBT_INFO("Deadline was reached!");
    changed = SD_simulate(-1);
    xbt_dynar_foreach(changed, i, t){
      if (SD_task_get_kind(t) == SD_TASK_COMP_SEQ && SD_task_get_state(t) == SD_DONE){
        XBT_VERB("%s (from %s) has completed after the deadline", SD_task_get_name(t), SD_task_get_dax_name(t));
      }
    }
  }

  /* Cleaning step once simulation is over */
  xbt_dynar_free_container(&idleVMs);
  xbt_dynar_free_container(&priority_queue);
}

void dpds(xbt_dynar_t daxes, scheduling_globals_t globals){
  int i;
  const sg_host_t *hosts = sg_host_list();

  /* Start by activating nVM VMs
   * Deadline is expressed in seconds, while price is for an hour.
   * Conversion is needed.
   */
  globals->nVM = ceil(globals->budget / (MAX(1, (globals->deadline / 3600.)) * globals->price));
  XBT_VERB("%d VMs are initially started", globals->nVM);
  for (i = 0; i < globals->nVM; i++){
    sg_host_start(hosts[i]);
  }
  dpds_schedule(daxes, globals);
}
