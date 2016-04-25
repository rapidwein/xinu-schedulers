#include <conf.h>
#include <kernel.h>
#include <sched.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lab1.h>
int highestGoodness(int queueClass){
	int max_goodness = 0,process_goodness,i;
	struct pentry *proc;
	for(i = 0; i < NPROC; i++){
		proc = &proctab[i];
		if(proc->pstate == PRREADY && proc->epoch == 1 && proc->queue == queueClass){
			process_goodness = goodness(i);
			if(process_goodness > max_goodness)
				max_goodness = process_goodness;
		}
	}
	return max_goodness;
}
