#include "scheduling_simulator.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "string.h"
#include "stdbool.h"
#include "ucontext.h"
#include "unistd.h"
#include "sys/time.h"


struct my_task{
	int pid;
	char task_name[128];
	int priority;
	int Quantum;
	int status;
	int time;
	int preselect;
	int remaining;
	int suspend_time;
	ucontext_t context;	
	//ucontext_t finish;
	char stack[1024*128];
	//char finish_stack[1024*128];
};

static struct my_task highQ[16];
static int highQ_ptr = -1;
static struct my_task lowQ[16];
static int lowQ_ptr = -1;
static struct my_task* waitingQ[16];
static int waitQ_ptr = -1;
static struct my_task initQ[32];
static int initQ_ptr = -1;
static struct my_task terminateQ[32];
static int terminateQ_ptr = -1;
static int stop_flag = 0;
static struct my_task * current_text;
static ucontext_t shell;
static ucontext_t simulation;
static ucontext_t finish;
static ucontext_t high_context;
static ucontext_t low_context;
static ucontext_t test;
static ucontext_t test2;
char finish_stack[1024*128];
char test_stack[1024*128];
char test_stack2[1024*128];
char shell_stack[1024*128];
char simulation_stack[1024*128];


bool streq(char a[],char b[]){
	if(strlen(a) != strlen(b)) return false;
	else{
		for(int i=0;i<strlen(a);i++){
			if(a[i] != b[i]) return false;
		}
		return true;
	}
}

struct my_task * find_task(int pid){
	if(initQ[0].status == 0) return NULL;
	for(int i=0;i<=highQ_ptr;i++){
		if(pid == highQ[i].pid) return &highQ[i];
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(pid == lowQ[i].pid) return &lowQ[i];
	}
	return NULL;
}

void task_remove(struct my_task queue[],int index,int* ptr){
	//printf("%d %d",index,*ptr);
	if(index == *ptr) (*ptr)--;
	else{
		for(int i=index;i<*ptr;i++){
			queue[i] = queue[i+1];
		}
		(*ptr)--;
	}
}


void hw_suspend(int msec_10)
{
	current_text->suspend_time = msec_10;
	current_text->status = 3;
	return;
}

void hw_wakeup_pid(int pid)
{
	struct my_task* task = find_task(pid);
	if(task->status == 3){
		task->status = 1;
		task->suspend_time = 0;
	}
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	int count = 0;
	for(int i=0;i<=highQ_ptr;i++){
		if(highQ[i].status == 3 && streq(highQ[i].task_name,task_name)){
			count++;
			highQ[i].status = 1;
			highQ[i].suspend_time = 0;
		}
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(lowQ[i].status == 3 && streq(lowQ[i].task_name,task_name)){
			count++;
			lowQ[i].status = 1;
			lowQ[i].suspend_time = 0;
		}
	}
    return count;
}

int hw_task_create(char *task_name)
{
	static int mypid = 0;
	//printf("pid : %d\n",mypid);
	if(!streq(task_name,"task1") && !streq(task_name,"task2") && !streq(task_name,"task3") && !streq(task_name,"task4") && !streq(task_name,"task5") && !streq(task_name,"task6")) return -1;
	else {
		mypid++;
	}
    return mypid; // the pid of created task name
}


void switch_main(int p){
	printf("\n");
	stop_flag = 1;
	for(int i=0;i<=highQ_ptr;i++){
		if(highQ[i].status == 2){
			swapcontext(&highQ[i].context,&shell);
		}
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(lowQ[i].status == 2){
			swapcontext(&lowQ[i].context,&shell);
		}
	}
}

void finish_task(){
	//printf("goto finish\n");
	current_text->status = 4;
	setcontext(&simulation);
}


void counting_time(){
	if(stop_flag){
		//signal(SIGALRM, counting_time);
		//alarm(1);
		return;
	} 
	else{
		int status = counting_waiting_time();
		//if (current_text->status == 2)current_text->status = 1;
		current_text->preselect = 1;
		swapcontext(&current_text->context,&simulation);
	}
}

bool check_tabu(int arr[],int size,int target){
	for(int i=0;i<=size;i++){
		if(arr[i] == target) return false;
	}
	return true;
}

struct my_task *select_next(struct my_task queue[],int size){
	int running_index = -1;
	int next_index = running_index;
	int finsih_flag = 1;
	int tabu[32];
	int tabu_ptr = -1;
	for(int i=0;i<=size;i++){
		if(queue[i].status != 1){
			tabu_ptr++;
			tabu[tabu_ptr] = i;
		} 
		if(queue[i].preselect == 1){
			if(queue[i].status == 2) queue[i].status = 1;
			running_index = i;
			queue[i].preselect = 0;
		} 
		if(queue[i].status != 4 && queue[i].status != 3) finsih_flag = 0;
	}
	next_index = running_index;
	if(running_index == -1 && finsih_flag == 1) return NULL;
	if(running_index > -1){
		queue[running_index].remaining -= 10;
		if(queue[running_index].remaining == 10) return current_text;
		else if(queue[running_index].remaining == 0){
			if(queue[running_index].Quantum == 1){
				queue[running_index].remaining = 20;
			}
			else{
				queue[running_index].remaining = 10;
			}
		}
		else {
			printf("error when check remaining, %d\n",queue[running_index].remaining);
		}
	}
	while(1){
		next_index = next_index+1;
		if(next_index > size) next_index = 0;
		if(queue[next_index].status == 1 || queue[next_index].status == 2){
			break;
		}
	}
	queue[next_index].status = 2;
	return &queue[next_index];
}

void add_waiting_time(struct my_task queue[],int size){
	for(int i=0;i<=size;i++){
		if(queue[i].status == 1)queue[i].time += 1;
	}
}

int counting_waiting_time(){
	int flag = 1;
	for(int i=0;i<=highQ_ptr;i++){
		if(highQ[i].status == 3){
			flag = 0;
			if(highQ[i].suspend_time == 0) highQ[i].status = 1;
			if(highQ[i].suspend_time > 0) highQ[i].suspend_time -= 1;
		}
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(lowQ[i].status == 3){
			flag = 0;
			if(lowQ[i].suspend_time == 0) lowQ[i].status = 1;
			if(lowQ[i].suspend_time > 0) lowQ[i].suspend_time -= 1;
		}
	}
	if(flag == 0) return -1;
	return 0;
}

bool not_all_end(){
	int flag = 0;
	for(int i=0;i<=highQ_ptr;i++){
		if(highQ[i].status != 4) flag = 1;
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(lowQ[i].status != 4) flag = 1;
	}
	if(flag == 0) return false;
	else return true;
}

struct my_task *select_remain(){
	for(int i=0;i<=highQ_ptr;i++){
		if(highQ[i].status == 1) return &highQ[i];
		if(highQ[i].status == 3){
			while(highQ[i].suspend_time){
				highQ[i].suspend_time -= 1;
				sleep(1);
			}
			highQ[i].status = 4;
			return &highQ[i];
		} 
	}
	for(int i=0;i<=lowQ_ptr;i++){
		if(lowQ[i].status == 1) return &lowQ[i];
		if(lowQ[i].status == 3){
			while(lowQ [i].suspend_time){
				lowQ[i].suspend_time -= 1;
				sleep(1);
			}
			lowQ[i].status = 4;
			return &lowQ[i];
		} 
	}
}

void simu_mode(){
	printf("start simulating\n");
	signal(SIGTSTP,switch_main);
	initation_simulation();
	current_text = &highQ[0];
	current_text->status = 2;
	int status;
	int i=0;
	while(1){
		i++;
		stop_flag = 0;
		add_waiting_time(highQ,highQ_ptr);
		add_waiting_time(lowQ,lowQ_ptr);
		signal(SIGALRM, counting_time);
		alarm(1);
		swapcontext(&simulation,&current_text->context);
		stop_flag = 0;
		current_text = select_next(highQ,highQ_ptr);
		if(current_text == NULL) break;
	}
	current_text = &lowQ[0];
	current_text->status = 2;
	while(1){
		stop_flag = 0;
		add_waiting_time(highQ,highQ_ptr);
		add_waiting_time(lowQ,lowQ_ptr);
		signal(SIGALRM, counting_time);
		alarm(1);
		swapcontext(&simulation,&current_text->context);
		stop_flag = 0;
		current_text = select_next(lowQ,lowQ_ptr);
		if(current_text == NULL) break;
	}
	stop_flag = 1;
	while(not_all_end()){
		current_text = select_remain();
		if(current_text != NULL){
			swapcontext(&simulation,&current_text->context);
		}
	}
}

void struct_strcpy(struct my_task* task,char *str){
	for(int i=0;i<strlen(str)+1;i++){
		task->task_name[i] = str[i];
	}
}

struct my_task  mytask_create(char * task_name, int pid,int priority,int Quantum){
	struct my_task task;
	struct_strcpy(&task,task_name);
	task.pid = pid;
	task.priority = priority;
	task.Quantum = Quantum;
	task.status = 0;
	task.time = 0;
	task.preselect = 0;
	task.suspend_time = 0;
	if(Quantum == 1) task.remaining = 20;
	else task.remaining = 10;
	return task;
}

struct my_task pop(struct my_task queue[],int ptr){
	struct my_task result;
	if(ptr == -1){
		result.pid = -1;
		return result;
	}
	else{
		result = queue[0];
		for(int i=1;i<ptr;i++) queue[i-1] = queue[i];
		return result;
	}
}

void my_task_push(struct my_task queue[],struct my_task task,int* ptr){
	(*ptr)++;
	queue[*ptr].pid = task.pid;
	struct_strcpy(&(queue[*ptr]),task.task_name);
	queue[*ptr].priority = task.priority;
	queue[*ptr].Quantum = task.Quantum;
	queue[*ptr].remaining = task.remaining;
	queue[*ptr].time = task.time;
	queue[*ptr].suspend_time = task.suspend_time;
}


void push_in_Q(struct my_task task){
	my_task_push(initQ,task,&initQ_ptr);
}

void add_handler(char command[],char task_name[],char option1[],char priority[],char option2[],char Quantum[]){
	int pid,_priority,_Quantum;
	struct my_task current_task; 
	if(streq(option1,"-t")){
		if(streq(priority,"L")){
			_Quantum = 1;
		}
		else if(streq(priority,"S")){
			_Quantum = 0;
		}
	}
	if(streq(option2,"-p")){
		if(streq(Quantum,"H")){
			_priority = 1;
		}
		else if(streq(Quantum,"L")){
			_priority = 0;
		}
	}
	pid = hw_task_create(task_name);
	current_task = mytask_create(task_name, pid,_priority, _Quantum);
	if(pid != -1) push_in_Q(current_task);
}

void remove_handler(int mypid){
	for(int i=0;i<=initQ_ptr;i++){
		if(initQ[i].pid == mypid) task_remove(initQ,i,&initQ_ptr);
	}
}

void foo(){
	printf("work\n");
}

void foo2(){
	printf("work2\n");
}

void initation_simulation(){
	for(int i=0;i<=initQ_ptr;i++){
		initQ[i].status = 1;
		if(initQ[i].priority == 1){
			highQ_ptr++;
			highQ[highQ_ptr] = initQ[i];
			getcontext(&highQ[highQ_ptr].context); 
			highQ[highQ_ptr].context.uc_stack.ss_sp = highQ[highQ_ptr].stack;
			highQ[highQ_ptr].context.uc_stack.ss_size = sizeof(highQ[highQ_ptr].stack);
			highQ[highQ_ptr].context.uc_stack.ss_flags = 0;
			highQ[highQ_ptr].context.uc_link = &finish;
			if(streq(highQ[highQ_ptr].task_name,"task1")){
				makecontext(&(highQ[highQ_ptr].context),task1,0);
			}
			else if(streq(highQ[highQ_ptr].task_name,"task2")){
				makecontext(&(highQ[highQ_ptr].context),task2,0);
			}
			else if(streq(highQ[highQ_ptr].task_name,"task3")){
				makecontext(&highQ[highQ_ptr].context,task3,0);
			}
			else if(streq(highQ[highQ_ptr].task_name,"task4")){
				makecontext(&highQ[highQ_ptr].context,task4,0);
			}
			else if(streq(highQ[highQ_ptr].task_name,"task5")){
				makecontext(&highQ[highQ_ptr].context,task5,0);
			}
			else if(streq(highQ[highQ_ptr].task_name,"task6")){
				makecontext(&highQ[highQ_ptr].context,task6,0);
			}
			else{
				printf("error when init simulation\n");
			}
		}
		else{
			lowQ_ptr++;
			lowQ[lowQ_ptr] = initQ[i];
			getcontext(&lowQ[lowQ_ptr].context); 
			lowQ[lowQ_ptr].context.uc_stack.ss_sp = lowQ[lowQ_ptr].stack;
			lowQ[lowQ_ptr].context.uc_stack.ss_size = sizeof(lowQ[lowQ_ptr].stack);
			lowQ[lowQ_ptr].context.uc_stack.ss_flags = 0;
			lowQ[lowQ_ptr].context.uc_link = &finish;
			if(streq(lowQ[lowQ_ptr].task_name,"task1")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task1,0);
			}
			else if(streq(lowQ[lowQ_ptr].task_name,"task2")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task2,0);
			}
			else if(streq(lowQ[lowQ_ptr].task_name,"task3")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task3,0);
			}
			else if(streq(lowQ[lowQ_ptr].task_name,"task4")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task4,0);
			}
			else if(streq(lowQ[lowQ_ptr].task_name,"task5")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task5,0);
			}
			else if(streq(lowQ[lowQ_ptr].task_name,"task6")){
				makecontext(&lowQ[lowQ_ptr].context,(void (*)(void))task6,0);
			}
		}
	}
}

int main()
{
	char command[32];
	char task_name[32];
	char option1[32];
	char priority[32];
	char option2[32];
	char Quantum[32];
	int pid;
	char stack[1024*128];
	struct my_task * temp_task;
	getcontext(&simulation); 
    simulation.uc_stack.ss_sp = simulation_stack;
    simulation.uc_stack.ss_size = sizeof(simulation_stack);
	simulation.uc_stack.ss_flags = 0;
	simulation.uc_link = &shell;
	makecontext(&simulation,(void (*)(void))simu_mode,0);
	getcontext(&finish); 
	finish.uc_stack.ss_sp = finish_stack;
	finish.uc_stack.ss_size = sizeof(finish_stack);
	finish.uc_stack.ss_flags = 0;
	finish.uc_link = NULL;
	makecontext(&finish,(void (*)(void))finish_task,0);
	while(1){
		scanf("%s",command);
		if(streq(command,"add")){
			scanf("%s",task_name);
			scanf("%s",option1);
			scanf("%s",priority);
			scanf("%s",option2);
			scanf("%s",Quantum);
			add_handler(command,task_name,option1,priority,option2,Quantum);
		}
		else if(streq(command,"remove")){
			scanf("%d",&pid);
			remove_handler(pid);
		}
		else if(streq(command,"ps")){
			for(int i=0;i<=initQ_ptr;i++){
				printf("%d %s ",initQ[i].pid,initQ[i].task_name);
				temp_task = find_task(initQ[i].pid);
				if(temp_task == NULL) temp_task = &initQ[i];
				if(temp_task->status == 0) printf("TASK_INIT ");
				else if(temp_task->status == 1) printf("TASK_READY ");
				else if(temp_task->status == 2) printf("TASK_RUNNING ");
				else if(temp_task->status == 3) printf("TASK_WAITING ");
				else if(temp_task->status == 4) printf("TASK_TERMINATE ");
				printf("%d ",temp_task->time);
				if(temp_task->priority == 1) printf("H ");
				else if(temp_task->priority == 0) printf("L ");
				if(temp_task->Quantum == 1) printf("L");
				else if(temp_task->Quantum == 0) printf("S");
				printf("\n");
			}
		}
		else if(streq(command,"start")){
			swapcontext(&shell,&simulation);

			printf("end simulation\n");
			stop_flag = 1;
		}
	}
	return 0;
}