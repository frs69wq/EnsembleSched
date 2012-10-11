/*
 * scheduling.h
 *
 *  Created on: 10 oct. 2012
 *      Author: suter
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

typedef enum {
DPDS=0,
WADPDS,
SPSS,
OURS
} alg_t;

char* getAlgorithmName(alg_t);
alg_t getAlgorithmByName(char*);

void handle_resource_dependency(SD_workstation_t, SD_task_t);
#endif /* SCHEDULING_H_ */
