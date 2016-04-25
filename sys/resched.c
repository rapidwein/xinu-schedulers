/* resched.c  -  resched */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>
#include <math.h>
unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
int findQueue();
static int queueType = 3;
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	int schedClass = getSchedClass();
	int nextEpoch = 0;
	/* no switch needed if current process priority higher than next*/
	if(schedClass == XINUSCHED){
		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
		   (lastkey(rdytail)<optr->pprio)) {
			return(OK);
		}
		
		/* force context switch */

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[ (currpid = getlast(rdytail)) ];
		nptr->pstate = PRCURR;		/* mark it currently running	*/
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif
		
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		
		/* The OLD process returns here when resumed. */
		return OK;
	}
	else if(schedClass == LINUXSCHED){
		if(linuxSched(LINUXSCHED) == OK)
			return OK;
	}
	else if(schedClass == MULTIQSCHED){
		int i;
		if(queueType == 3){
			queueType = findQueue();
			for(i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev){
				if(proctab[i].queue == queueType && proctab[i].quantum > 0)
					if(multiSched(queueType) == OK)
						return OK;
			}
		queueType = 3;
		}
		else if(queueType == NORMALQUEUE){
			if(multiSched(NORMALQUEUE) == OK)
				return OK;
		}
		else if(queueType == REALQUEUE){
			if(multiSched(REALQUEUE) == OK)
				return OK;
		}
	}
	return -1;
}
int findQueue(){
	int result;
  result = rand() % 100;
  if(result < 70)	
	return REALQUEUE;
  else
	return NORMALQUEUE;
}
int linuxSched(int schedClass){
	register struct pentry *optr,*nptr;
	int max_goodness;
	int i,pre_left = preempt,nextProc = 0,nextEpoch=0;
                optr = &proctab[currpid];
                optr->quantum -= (QUANTUM - preempt);
                if(optr->quantum < 0)
                        optr->quantum = 0;
                max_goodness = highestGoodness(NORMALQUEUE);
                if((optr->pstate != PRCURR || optr->quantum == 0) && max_goodness == 0){
                        nextEpoch = 1;
                        for(i = q[rdyhead].qnext; i != rdytail; i=q[i].qnext){
                                if(proctab[i].queue == NORMALQUEUE)
                                        proctab[i].epoch = 1;
                        }
                }
                for(i = q[rdytail].qprev; i != rdyhead; i=q[i].qprev){
                        if(proctab[i].epoch == 1 && proctab[i].queue == NORMALQUEUE){
                                if(goodness(i) >= max_goodness){
                                        nextProc = i;
                                }
                        }
                }
                if(optr->pstate == PRCURR && (goodness(currpid) > max_goodness) && (optr->quantum > 0) && nextEpoch == 0){
			//kprintf("\npid %d goodness %d max %d quantum %d epoch %d ",currpid,goodness(currpid),max_goodness,optr->quantum,nextEpoch);
                        if(optr->quantum < QUANTUM)
                                preempt = optr->quantum;
                        else
                                preempt = QUANTUM;
                        return(OK);
                }
		else if(nextEpoch == 0 && ((optr->pstate != PRCURR) || (optr->quantum == 0) || (goodness(currpid) <= max_goodness))){
                        if(optr->pstate == PRCURR){
                                optr -> pstate = PRREADY;
                                insert(currpid, rdyhead, goodness(currpid));
				//kprintf("\n pid %d goodness %lu switch with ",currpid,goodness(currpid));
                        }
                        currpid = nextProc;
                        nptr = &proctab[currpid];
                        nptr->pstate = PRCURR;
                        dequeue(currpid);
                        if(nptr->quantum < QUANTUM)
                                preempt = nptr->quantum;
                        else
                                preempt = QUANTUM;
                        //kprintf("process %d goodness %lu\n",currpid,goodness(currpid));
                        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                        return(OK);
                }
                else if(nextEpoch == 1){
			nextEpoch = 0;
			if(proctab[currpid].pstate == PRCURR){
				proctab[currpid].pstate = PRREADY;
                        	insert(currpid, rdyhead, goodness(currpid));
			}
                        for(i = NPROC - 1; i >= 0; i--){
                                if( proctab[i].pstate != PRFREE && proctab[i].queue == NORMALQUEUE){
					if(proctab[i].newprio != 0)
						proctab[i].pprio = proctab[i].newprio;
					proctab[i].epoch = 1;
                                        proctab[i].quantum = (int)floor(proctab[i].quantum/2) + proctab[i].pprio;
					if(proctab[i].pstate == PRREADY){
						dequeue(i);
						insert(i, rdyhead, goodness(i));
						//kprintf("\n pid %d goodness %lu ENDEND",i,goodness(i));
					}
                                }
				else if(proctab[i].pstate != PRFREE && proctab[i].queue == REALQUEUE && schedClass == MULTIQSCHED){
					proctab[i].epoch = 1;
					proctab[i].quantum = REALQUANTUM;
					if(proctab[i].pstate == PRREADY){
						dequeue(i);
						insert(i, rdyhead, goodness(i));
					}
				}
                        }
			if(schedClass == LINUXSCHED){
				max_goodness = highestGoodness(NORMALQUEUE);
				for(i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev){
					if(proctab[i].epoch == 1 && proctab[i].queue == NORMALQUEUE && proctab[i].pstate == PRREADY){
						if(goodness(i) >= max_goodness){
							currpid = i;
							nptr = &proctab[i];
							nptr->pstate = PRCURR;
							dequeue(currpid);
							break;
						}
					}
				}
				
				//kprintf("\n Running process %d with goodness %d (ended for loop at %d)",currpid,goodness(currpid),i);
				if(nptr->quantum < QUANTUM)
					preempt = optr->quantum;
				else
					preempt = QUANTUM;
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return(OK);
			}
			else if(schedClass == MULTIQSCHED){
				queueType = findQueue();
				if(multiSched(queueType) == OK)
					return OK;
			}
                }
		return -1;
}
int multiSched(int queueClass){
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	int i,nextProc = 0;
	if(queueClass == NORMALQUEUE){
		if(proctab[currpid].queue == REALQUEUE){
			optr = &proctab[currpid];
			optr->quantum -= (REALQUANTUM - preempt);
			if(optr->quantum < 0)
				optr->quantum = 0;
			if(optr->pstate == PRCURR){
				optr->pstate = PRREADY;
				insert(currpid, rdyhead, goodness(currpid));
			}
			for(i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev){
				if(proctab[i].queue == NORMALQUEUE && proctab[i].quantum > 0){
					currpid = i;
					nptr = &proctab[currpid];
					nptr->pstate = PRCURR;
					dequeue(currpid);
					if(proctab[currpid].quantum < QUANTUM)
						preempt = proctab[currpid].quantum;
					else
						preempt = QUANTUM;
					ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                			return OK;
				}
				
			}
		}
		else{
			if(linuxSched(MULTIQSCHED) == OK)
				return OK;
		}
	}
	else if(queueClass == REALQUEUE){
		int i,nextProc = 0;
		if(proctab[currpid].queue == NORMALQUEUE){
	
			optr = &proctab[currpid];
			optr->quantum -= (QUANTUM - preempt);
			if(optr->quantum < 0)
				optr->quantum = 0;
                        if(optr->pstate == PRCURR){
                                optr->pstate = PRREADY;
                                insert(currpid, rdyhead, goodness(currpid));
                        }
			for(i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev){
				if(proctab[i].queue == REALQUEUE && proctab[i].quantum > 0){
					currpid = i;
                                        nptr = &proctab[currpid];
                                        nptr->pstate = PRCURR;
                                        dequeue(currpid);
					if(nptr->quantum < REALQUANTUM)
	                                        preempt = nptr->quantum;
					else
						preempt = REALQUANTUM;
                                        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                                        return OK;
				}
			}
		}
		optr = &proctab[currpid];
		optr->quantum -= (REALQUANTUM - preempt);
		if(optr->quantum < 0)
			optr->quantum = 0;
		if(optr->pstate == PRCURR && optr->quantum < REALQUANTUM){
			optr->pstate = PRREADY;	
			insert(currpid, rdyhead, goodness(currpid));
			return OK;
		}
		else if(optr->pstate != PRCURR || optr->quantum == 0){
			if(optr->pstate == PRCURR){
				optr->pstate = PRREADY;
				insert(currpid, rdyhead, goodness(currpid));
			}
			for(i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev){
				if(proctab[i].queue == REALQUEUE && proctab[i].quantum > 0){
					nextProc = i;
					currpid = i;
					nptr = &proctab[currpid];
                                        nptr->pstate = PRCURR;
                                        dequeue(currpid);
					if(nptr->quantum < REALQUANTUM)
	                                        preempt = nptr->quantum;
					else
						preempt = REALQUANTUM;
                                        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                                        return OK;
				}
			}
			if(nextProc == 0){
				for(i = NPROC - 1; i >= 0; i--){
					if(proctab[i].pstate != PRFREE && proctab[i].queue == REALQUEUE){
						proctab[i].epoch = 1;
						proctab[i].quantum = REALQUANTUM;
					}
					else if(proctab[i].pstate != PRFREE && proctab[i].queue == NORMALQUEUE){
						proctab[i].epoch = 1;
						proctab[i].quantum = (int)floor(proctab[i].quantum/2) + proctab[i].pprio;
					}
					if(proctab[i].pstate == PRREADY){
						dequeue(i);
						insert(i, rdyhead, goodness(i));
					}
				}
				queueType = findQueue();
				if(multiSched(queueType) == OK)
					return OK;
			}
		}
	}
	return -1;
}
