#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#define FIFO 1
#define RR 2
#define SJF 3
#define PSJF 4
#define one_unit ({ volatile unsigned long i; for(i = 0; i < 1000000UL; i++); })
#define CPU1 0
#define CPU2 1
typedef struct {
	pid_t pid;
	char id[33];
	int ready_time;
	int exec_time;
	long start_time;
}PROCESS;
void setcpu(int core , int pid){
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core , &mask);
	sched_setaffinity(pid , sizeof(cpu_set_t) , core);
	return;
}
void wake(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	sched_setscheduler(pid, SCHED_OTHER, &param);
	return;
}
void block(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	sched_setscheduler(pid, SCHED_IDLE, &param);
	return;
}
static int OK = 0;
void ok_to_go(){
	OK = 1;
}
int execute(PROCESS p){
	pid_t pid;
	signal(SIGUSR1 , ok_to_go);
	pid = fork();
	if (pid == 0){
		setcpu(getpid() , CPU2);
		while(OK == 0);
		for (int i = 0; i < p.exec_time; i++){
			one_unit;
		}
		exit(0);
	}
	return pid;
}
int main(int argc, char const *argv[]){
	char policy[6];
	int n;
	scanf("%s", policy);
	scanf("%d", &n);
	PROCESS pcs[n];
	for (int i = 0; i < n; i++){
		pcs[i].pid = -1;
		scanf("%s%d%d", pcs[i].id , &(pcs[i].ready_time) , &(pcs[i].exec_time));
	}
	for (int i = n - 1; i > 0; i--){
		for (int j = 0; j < i; j++){
			if (pcs[j].ready_time > pcs[j + 1].ready_time){
				PROCESS temp = pcs[j];
				pcs[j] = pcs[i];
				pcs[i] = temp;
			}
		}
	}
	int policy_no;
	if (policy[0] == 'F')
		policy_no = FIFO;
	else if (policy[0] == 'R')
		policy_no = RR;
	else if (policy[0] == 'S')
		policy_no = SJF;
	else
		policy_no = PSJF;
	pid_t cur_pid = getpid();
	setcpu(cur_pid , CPU1);
	wake(cur_pid);
	int ntime = 0;
	int running_proc = -1;
	int finish = 0;
	int rr_time = 0;
	int rr_queue[100];
	for (int i = 0; i < n; i++){
		rr_queue[i] = -1;
	}
	int rr_del = 0;
	int rr_add = 0;
	while(1){
		if (running_proc != -1 && pcs[running_proc].exec_time == 0){
			syscall(335 , pcs[running_proc].start_time , pcs[running_proc].pid);
			waitpid(pcs[running_proc].pid , NULL , 0);
			printf("%s %d\n", pcs[running_proc].id , pcs[running_proc].pid);
			fflush(stdout);
			if (policy_no == RR){
				rr_queue[rr_del] = -1;
				rr_del = (rr_del + 1) % 100;
			}
			running_proc = -1;
			finish++;
			if (finish == n) break;
		}
		for (int i = 0; i < n; i++){
			if (pcs[i].ready_time == ntime){
				pcs[i].start_time = syscall(334);
				pcs[i].pid = execute(pcs[i]);
				block(pcs[i].pid);
				if (policy_no == RR){
					rr_queue[rr_add] = i;
					rr_add = (rr_add + 1) % 100;
				}
			}
		}
		// if (pcs[0].pid == 1)
		// 	printf("%d\n", ntime);
		int next_proc = -1;
		switch(policy_no){
			case FIFO:{
				if (running_proc != -1){
					next_proc = running_proc;
					break;
				}
				for (int i = 0; i < n; i++){
					if (pcs[i].pid != -1 && pcs[i].exec_time > 0){
						if (next_proc == -1 || pcs[i].ready_time < pcs[next_proc].ready_time)
							next_proc = i;
					}
				}
				break;
			}
			case RR:{
				// if (pcs[0].pid == 1)
				// 	printf("%d\n", ntime);
				if (running_proc == -1){
					if (rr_queue[rr_del] != -1){
						next_proc = rr_queue[rr_del];
					}
				}
				else if ((ntime - rr_time) % 500 == 0){
					rr_queue[rr_add] = running_proc;
					rr_add = (rr_add + 1) % 100;
					rr_del = (rr_del + 1) % 100;
					next_proc = rr_queue[rr_del];
				}
				else{
					next_proc = running_proc;
				}
				break;
			}
			case SJF: case PSJF:{
				if (running_proc != -1 && policy_no == SJF){
					next_proc = running_proc;
					break;
				}
				for (int i = 0; i < n; i++){
					if (pcs[i].pid != -1 && pcs[i].exec_time > 0){
						if (next_proc == -1 || pcs[i].exec_time < pcs[next_proc].exec_time)
							next_proc = i;
					}
				}
				break;
			}
		}
		if (next_proc != -1 && running_proc != next_proc){
			block(pcs[running_proc].pid);
			kill(pcs[next_proc].pid , SIGUSR1);
			wake(pcs[next_proc].pid);
			running_proc = next_proc;
			rr_time = ntime;
		}
		one_unit;
		if (running_proc != -1)
			(pcs[running_proc].exec_time)--;
		ntime++;
	}
	exit(0);
}
