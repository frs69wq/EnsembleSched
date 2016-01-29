/*
 * Copyright (c) Centre de Calcul de l'IN2P3 du CNRS
 * Contributor(s) : Frédéric SUTER (2012-2016)
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "host.h"
#include "scheduling.h"
#include "task.h"
#include "xbt.h"
#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host, EnsembleSched, "Logging specific to hosts");

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/

void sg_host_allocate_attribute(sg_host_t host){
  HostAttribute data;
  data = calloc(1,sizeof(struct _HostAttribute));
  data->total_cost = 0;
  /* Set the hosts to off and idle at the beginning */
  data->on_off = 0;
  data->idle_busy = 0;
  data->booting=NULL;
  sg_host_user_set(host, data);
}

void sg_host_free_attribute(sg_host_t host){
  free(sg_host_user(host));
  sg_host_user_set(host, NULL);
}

void sg_host_set_price(sg_host_t host, double price){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->price = price;
  sg_host_user_set(host, attr);
}

void sg_host_set_provisioning_delay(sg_host_t host, double delay){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->provisioning_delay= delay;
  sg_host_user_set(host, attr);
}


double sg_host_get_available_at(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  return attr->available_at;
}

void sg_host_set_available_at(sg_host_t host, double time){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->available_at=time;
  sg_host_user_set(host, attr);
}

SD_task_t sg_host_get_last_scheduled_task(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  return attr->last_scheduled_task;
}

void sg_host_set_last_scheduled_task(sg_host_t host, SD_task_t task){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->last_scheduled_task=task;
  sg_host_user_set(host, attr);
}

void sg_host_set_to_idle(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->idle_busy=0;
  sg_host_user_set(host, attr);
}

void sg_host_set_to_busy(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->idle_busy=1;
  sg_host_user_set(host, attr);
}

/* compare host names w.r.t. the lexicographic order (Increasing) */
int nameCompareHosts(const void *n1, const void *n2) {
  return strcmp(sg_host_get_name(*((sg_host_t *)n1)), sg_host_get_name(*((sg_host_t *)n2)));
}

/*****************************************************************************/
/*****************************************************************************/
/**************    Functions needed by scheduling algorithms    **************/
/*****************************************************************************/
/*****************************************************************************/

/* Activate a resource, i.e., act as if a VM is started on a host. This amounts to :
 * - setting attributes to 'ON' and 'idle'
 * - Resetting the start time of the host to the current time
 * - bill at least the first hour
 * If a provisioning delay is needed before a VM is actually available for executing task, this function :
 * - creates a task whose name is "Booting " followed by the host name
 * - schedules this task on the host
 * - Ensures that no compute task can be executed before the completion of this booting task.
 */
void sg_host_start(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  char name[1024];

  attr->on_off = 1;
  attr->idle_busy = 0;
  attr->start_time = SD_get_clock();
  attr->total_cost += attr->price;
  if (attr->provisioning_delay){
    sprintf(name,"Booting %s", sg_host_get_name(host));

    attr->booting = SD_task_create_comp_seq(name, NULL, attr->provisioning_delay*sg_host_speed(host));
    SD_task_schedulel(attr->booting, 1, host);
    attr->available_at += attr->provisioning_delay;
    handle_resource_dependency(host, attr->booting);
  }
  XBT_DEBUG("VM started on %s: Total cost is now $%f for this host", sg_host_get_name(host), attr->total_cost);
  sg_host_user_set(host, attr);
}

/* Disable a resource, i.e., act as a VM is terminated on a host. This amounts to:
 * - setting attributes to 'OFF'
 * - Resetting the start time of the host to 0 (just in case)
 * - Do some accounting. The time (in seconds) spent since the last time host/VM was started (state set to ON) is
 *   rounded down (as the first hour is charged on activation) and multiplied by the hour price.
 * If a provisioning delay is needed before a VM is actually available for executing task, this function destroys the
 * booting task created by the sg_host_start function.
*/
void sg_host_terminate(sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  double duration = SD_get_clock() - attr->start_time;

  if (attr->booting)
    SD_task_destroy(attr->booting);
  attr->on_off = 0;
  attr->start_time = 0.0;
  attr->total_cost += ((((int) duration / 3600)) * attr->price);

  XBT_DEBUG("VM stopped on %s: Total cost is now $%f for this host", sg_host_get_name(host), attr->total_cost);
  sg_host_user_set(host, attr);
}

/* Simple function to know whether a host/VM can accept tasks for execution. Has to be ON and idle for that.
 */
int is_on_and_idle(sg_host_t host){
  HostAttribute attr = sg_host_user(host);
  return (attr->on_off && !attr->idle_busy);
}

/* Build an array that contains all the idle hosts/VMs in the platform */
xbt_dynar_t get_idle_VMs(){
  int i;
  const sg_host_t *hosts = sg_host_list();
  int nhosts = sg_host_count();
  xbt_dynar_t idleVMs = xbt_dynar_new(sizeof(sg_host_t), NULL);

  for (i = 0; i < nhosts; i++){
    if (is_on_and_idle(hosts[i]))
      xbt_dynar_push(idleVMs, &(hosts[i]));
  }

  return idleVMs;
}

/* Build an array that contains all the busy hosts/VMs in the platform */
xbt_dynar_t get_running_VMs(){
  int i;
  const sg_host_t *hosts = sg_host_list ();
  int nhosts = sg_host_count ();
  HostAttribute attr;
  xbt_dynar_t runningVMs = xbt_dynar_new(sizeof(sg_host_t), NULL);

  for (i = 0; i < nhosts; i++){
    attr = sg_host_user(hosts[i]);
    if (attr->on_off)
      xbt_dynar_push(runningVMs, &(hosts[i]));
  }

  return runningVMs;
}

/* Build an array that contains all the hosts/VMs that are "approaching their hourly billing cycle" in the platform
 * Remark: In the paper by Malawski et al., no details are provided about when a VM is "approaching" the end of a
 * paid hour. This is hard coded in the source code of cloudworkflowsim: 90s (provisioner interval, a.k.a period) +
 * 1s (optimistic deprovisioning delay)
 */
xbt_dynar_t get_ending_billing_cycle_VMs(double period, double margin){
  int i;
  const sg_host_t *hosts = sg_host_list ();
  int nhosts = sg_host_count ();
  HostAttribute attr;
  xbt_dynar_t endingVMs = xbt_dynar_new(sizeof(sg_host_t), NULL);

  for (i = 0; i < nhosts; i++){
    attr = sg_host_user(hosts[i]);
    /* To determine how far a VM is from the end of a hourly billing cycle, we compute the time spent between the
     * start of the VM and the current, and keep the time spent in the last hour. As times are expressed in seconds,
     * it amounts to computing the modulo to 3600s=1h. Then the current VM is selected if this modulo is greater than
     * 3600-period-margin.
     */
    if (attr->on_off && ((int)(SD_get_clock() - attr->start_time) % 3600) > (3600-period-margin))
      xbt_dynar_push(endingVMs, &(hosts[i]));
  }

  return endingVMs;
}

/* Build the set of hosts/VMs that have to be terminated. This function selects how_many VMs from the source set of
 * candidates.
 * Remark: In the paper by Malawski et al., no details are provided about how the VMs are selected in the source set.
 * Moreover, there is no check w.r.t. the size of the source set.
 * Assumptions:
 * 1) If the source set is too small, display a warning and return a smaller set than expected.
 * 2) Straightforward selection of the VMs in the set. Just pick the how_many first ones without any consideration of
 *    the time remaining until the next hourly billing cycle. Just check if the VM is not currently executing a task
 */
xbt_dynar_t find_active_VMs_to_stop(int how_many, xbt_dynar_t source){
  int i, found;
  long unsigned int source_size = xbt_dynar_length(source);
  xbt_dynar_t to_stop = xbt_dynar_new(sizeof(sg_host_t), NULL);
  sg_host_t v;

  if (how_many > source_size){
    XBT_WARN("Trying to terminate more VMs than what is available (%d > %lu)."
        " Change the number of VMs to terminate to %lu", how_many, source_size, source_size);
    how_many = source_size;
  }

  i = 0;
  found = 0;

  while(found < how_many && i < xbt_dynar_length(source)){
    /* No advanced selection process. Just pick the how_many first idle VMs in the source set. */
    xbt_dynar_get_cpy(source, i, &v);
    HostAttribute attr = sg_host_user(v);
    if (!attr->idle_busy){
      xbt_dynar_push(to_stop, &v);
      found++;
    }
    i++;
  }

  if (found < how_many)
    XBT_WARN("Trying to terminate too many VMs, some are busy... Change the number of VMs to terminate to %d", found);

  return to_stop;
}

/* Return the first inactive host/VM (currently set to OFF) that we find in the platform.
 * Remarks:
 * 1) Straightforward selection, all VMs are assumed to be similar
 * 2) It may happen that no such VM is found. This means that the platform file given as input of the simulator was
 *    too small. The simulation cannot continue while the size of the resource pool represented by the platform file
 *    is not increased.
 */
sg_host_t find_inactive_VM_to_start(){
  int i=0;
  const sg_host_t *hosts = sg_host_list ();
  int nhosts = sg_host_count ();
  HostAttribute attr;
  sg_host_t host = NULL;

  while (i < nhosts){
    attr = sg_host_user(hosts[i]);
    if (!attr->on_off){
      host = hosts[i];
      break;
    }
    i++;
  }

  if (!host){
    xbt_die("Argh. We reached the pool limit. Have to increase the size of the cluster in the platform file.");
  }

  return host;
}

/* Determine the current utilization of VM in the system. This utilization is defined in the paper by Malawski et al.
 * as "the percentage of idle VMs over time".
 * The source code shows that it is the number of busy VMs divided by the total number of active VMs (busy and idle)
 */
double compute_current_VM_utilization(){
  int i=0;
  const sg_host_t *hosts = sg_host_list ();
  int nhosts = sg_host_count ();
  HostAttribute attr;
  int nActiveVMs = 0, nBusyVMs = 0;

  for (i = 0; i < nhosts; i++){
    attr = sg_host_user(hosts[i]);
    if (attr->on_off){
      nActiveVMs++;
      if (!attr->idle_busy)
        nBusyVMs++;
    }
  }
  return (100.*nBusyVMs)/nActiveVMs;
}


/* Randomly select a host in an array. Rely on the rand function provided by math.h
 * Remark: This function actually removes the selected element from the array.
 */
sg_host_t select_random(xbt_dynar_t hosts){
  unsigned long i;
  sg_host_t host=NULL;
  int nhosts = xbt_dynar_length(hosts);

  i = (unsigned long) ((nhosts-1.)*rand()/(RAND_MAX+1.0));
  xbt_dynar_remove_at(hosts, i, &host);
  return host;
}
