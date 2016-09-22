/*
 * main.cpp
 *
 *  Created on: Nov 12, 2014
 *      Author: trevor
 */

//#include "Scheduler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#define LINESIZE 32
#define DEBUG false

typedef struct{

    int burstc;
    int* burstv;
}bursts;

typedef struct{
    int waitc;
    int* waitv;
}waits;

typedef struct
{
    bursts* bursts;
    waits* waits;
	int pid;
	//int timeLeft;
	int startTime;
	int ready_time;
	int burstnum;
	int waitnum;
	int cpu_time;
	int wait_time;
	//int sleep_until;
	int first_time;
	int seen;
	int examined;
	int finish;
} process;

typedef struct node
{
	process *process;
	struct node *next;
} node;

typedef struct
{
	node *front;
	node *rear;
	node *temp;
	node *front1;
	int count;
	//queue() : front(NULL), rear(NULL), temp(NULL), front1(NULL){}
} queue;




process* frontElementOfQueue(queue* queue);
void enq(queue* queue, process* data);
void deq(queue* queue);
bool empty(queue* queue);
void displayQueue(queue* queue);
queue createQueue();
int queueSize(queue* queue);
void loadNewProcesses(queue* toQueue, queue* fromQueue);
void loadBusyProcesses(queue* toQueue, queue* fromQueue);
void removeProcess(queue* queue, process* process);
void removeCurrentProcess(queue* queue);
void readinProcesses(queue* processes, char* data);
process* nextShortestProcess(queue* queue);
void new_process(queue* toQueue,char* line, int pid);
char** string_split(char* a_str, const char a_delim);
void analyze_all(queue* queue);

int currentCPUTime = 0;
int processOnCPUTime = 0;
process* currentProcess = NULL;

/*
Usage: sched <input file> <scheduling algorithm> <scheduling parameters>

Supported schedulers:

1) FCFS – Parameters: Quantum, Cost of half C.S.

2) SJNFP – Parameters: Cost of half C.S.
*/
int main(int argc, char** argv){
	//#ifdef DEBUG printf("debugging\n");
	if(DEBUG) printf("START\n");
	if(DEBUG) printf("argc: %d\n", argc);
	if (argc == 1){
		printf("Usage: sched <input file> <scheduling algorithm> <scheduling parameters>\n\nSupported schedulers:\n\n1) FCFS – Parameters: none\n\n2) RR - Parameters: Quatum, Cost of Quantum\n\n3) SJN – Parameters: Cost of Quantum. (doesn't quite work)\n");
		printf("END\n");
		fflush(stdout);
		exit(1);
	}
	if (DEBUG) printf("Setting up\n");
	char * scheduler = argv[2];
	queue processes = createQueue();
	queue readyQueue = createQueue();
	queue busyQueue = createQueue();
	queue doneQueue = createQueue();
	int quantum = atoi((const char*)argv[3]);
	//int csCost = atoi((const char*)argv[4]) -1;
	readinProcesses(&processes, argv[1]);  //load list of processes
	if (DEBUG) printf("Setup complete\n");
	printf("type: %s\n", argv[2]);
	while(!empty(&processes) || !empty(&readyQueue) || !empty(&busyQueue) || currentProcess != NULL){  //while we have a job to run or a job yet to run
		if(strcmp (scheduler, "FCFS") == 0 || strcmp (scheduler, "RR") == 0 || strcmp (scheduler, "SJN") == 0){
			if(DEBUG) printf("moving processes\n");
			loadNewProcesses(&readyQueue, &processes); //move any processes into readyQueue if they're ready
			loadBusyProcesses(&readyQueue, &busyQueue); //move any processes into readyQueue if they're done being busy
			if(DEBUG) printf("done moving processes\n");
			if(DEBUG) {printf("process");displayQueue(&processes);}
			if(DEBUG) {printf("ready");displayQueue(&readyQueue);}
			if(DEBUG) {printf("busy");displayQueue(&busyQueue);}
			if(DEBUG) {printf("done");displayQueue(&doneQueue);}
			if(DEBUG && currentProcess != NULL) printf("CPU: %d\n", currentProcess->pid);
			else if(DEBUG) printf("CPU: empty\n");
			if (DEBUG) printf("----------CPU Cycle %d Start----------\n", currentCPUTime);
			if (currentProcess == NULL){ // no process on CPU, try to load one
				if(DEBUG) printf("No Process, Loading Next @ time: %d\n", currentCPUTime);
				if(strcmp (scheduler, "SJN") == 0){
					currentProcess = nextShortestProcess(&readyQueue); // load the next shortest item onto CPU
					removeProcess(&readyQueue, currentProcess);
				}
				else{
					currentProcess = frontElementOfQueue(&readyQueue); //load the next item onto CPU
					deq(&readyQueue);
				}
				if (currentProcess != NULL){
					if(!currentProcess->seen){
						currentProcess->first_time = currentCPUTime;
						currentProcess->seen = true;
					}
					printf("Process %d Loaded onto CPU @ time: %d\n",currentProcess->pid, currentCPUTime);
				}
				processOnCPUTime = 0; //reset CPU time
			}
			else{ // process on CPU, do stuff
				if(currentProcess->bursts->burstv[currentProcess->burstnum] > 0){ //Process with time left
					/*if(currentProcess->bursts->burstv[currentProcess->burstnum] == processOnCPUTime){ // burst is done
						printf("Process %d Burst Complete, removing @ time: %d\n", currentProcess->pid, currentCPUTime);
						currentProcess->burstnum++;
						removeCurrentProcess(&busyQueue); //put it in the busy queue
						currentProcess = NULL;
					}
					else*/{ // process still has burst time
						if((strcmp (scheduler, "RR" ) == 0 || strcmp (scheduler, "SJN") == 0 )&& processOnCPUTime > quantum && !empty(&readyQueue)){ //quantum is up
							printf("Process %d Quantum Exceeded, removing @ time: %d\n", currentProcess->pid, currentCPUTime);
							removeCurrentProcess(&readyQueue); //remove process from CPU as quantum is up
							currentProcess = NULL;
						}
						else{
							if(DEBUG) printf("Process %d Used CPU\n", currentProcess->pid);
							currentProcess->bursts->burstv[currentProcess->burstnum]--;  //deduct time from process
							processOnCPUTime++; //add time to processes current run
							currentProcess->cpu_time++;
						}
					}
				}
				else{ //burst is done
					if( currentProcess->burstnum == currentProcess->bursts->burstc){ //if this was the last burst
						printf("Process %d is Done, moving to done queue @ time: %d\n", currentProcess->pid, currentCPUTime);
						removeCurrentProcess(&doneQueue);  //remove process from CPU
						currentProcess->finish = currentCPUTime;
						currentProcess = NULL;
					}
					else{ //not the last burst
						printf("Process %d Burst Complete, removing @ time: %d\n", currentProcess->pid, currentCPUTime);
						removeCurrentProcess(&busyQueue); //remove process from CPU
						currentProcess->burstnum++; //increase the burst number
						currentProcess = NULL;
					}
				}
			}
		}
		else{
			break;
		}
		if (DEBUG) printf("----------CPU Cycle %d End----------\n", currentCPUTime);
		currentCPUTime++;
	}
	analyze_all(&doneQueue);
	printf("END\n");
	return 0;
}
/* loads processes into waitQueue if they have arrived */
void loadNewProcesses(queue* toQueue, queue* fromQueue){
	if (DEBUG) printf("loading new processes\n");
	node* nodeToCheck = fromQueue->front;
	int i;
	for( i = queueSize(fromQueue); i > 0; i--){
		if(nodeToCheck->process->startTime == currentCPUTime){
			enq(toQueue,nodeToCheck->process);
			if(DEBUG)printf("process %d moved to ready queue\n",nodeToCheck->process->pid);
			removeProcess(fromQueue,nodeToCheck->process);
		}
		nodeToCheck = nodeToCheck->next;
	}
}
void loadBusyProcesses(queue* toQueue, queue* fromQueue){
	if (DEBUG) printf("loading busy processes\n");
	node* nodeToCheck = fromQueue->front;
	int i;
	//for( i = queueSize(fromQueue); i > 0; i--){
	while(nodeToCheck != NULL){
		if(nodeToCheck != NULL){
			if(nodeToCheck->process->waits->waitv[nodeToCheck->process->waitnum] == 0 /*nodeToCheck->process->wait_time*/){
				enq(toQueue,nodeToCheck->process);
				nodeToCheck->process->waitnum++;
				if(DEBUG)printf("process %d moved from busy queue to ready queue\n", nodeToCheck->process->pid);
				removeProcess(fromQueue,nodeToCheck->process);
			}
			else{
				nodeToCheck->process->waits->waitv[nodeToCheck->process->waitnum]--;
			}
			nodeToCheck = nodeToCheck->next;
		}
		else
			if(DEBUG) printf("For some reason there were fewer items in the queue then expected\n");

	}
}

/* puts the current process in doneQueue */
void removeCurrentProcess(queue* queue){
	if (DEBUG) printf("removing done process from CPU\n");
	enq(queue, currentProcess);
}

/* loads the list of processes from input */
void readinProcesses(queue* processes, char* input){
	if (DEBUG) printf("reading in processes\n");
	char* buffer;
    FILE *fp= fopen(input, "rb");
	if ( fp != NULL )
	{
		fseek(fp, 0L, SEEK_END);
		long s = ftell(fp);
		rewind(fp);
		buffer = malloc(s);
		if ( buffer != NULL )
		{
			fread(buffer, s, 1, fp);
			// we can now close the file
			fclose(fp); fp = NULL;

			// do something, e.g.
			//fwrite(buffer, s, 1, stdout);
		    char** line = string_split(buffer, '\n');
		    int j = 0;
		    while (line[j] != NULL)
		    {
		        new_process(processes, line[j], j);
		        j++;
		    }
			free(buffer);
		}
		if (fp != NULL) fclose(fp);
	}
    /*char* arr_lines;
	char* line;
    char buf_line[LINESIZE];
    int processCount = 0;
    //char* data = input[processCount];

    if ((fp = fopen(input, "r")) == NULL)
    {
        printf("File Not Found: %c\n", *strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(DEBUG) printf("Counting processes\n");
    while (fgets(buf_line, LINESIZE, fp))
    {
       if (!(strlen(buf_line) == LINESIZE - 1 && buf_line[LINESIZE - 2] != '\n'))
       {
           processCount++;
       }
    }
    if(DEBUG) printf("There are %d processes in the file\n", processCount);
    arr_lines = malloc(processCount * LINESIZE * sizeof(char*));
    rewind(fp);
    while (!fp->eof()){
      fp.getline(getdata,sizeof(infile));
      // if i cout here it looks fine
      //cout << getdata << endl;
    }
    //processCount = 0;
    /*
    line = arr_lines;
    while (fgets(line, LINESIZE, fp))
    {
        if (!(strlen(line) == LINESIZE - 1 && line[LINESIZE - 2] != '\n'))
        {
            line +=  LINESIZE;
        }
    }
    */
    //fclose(fp);
    //free(line);
	/*
	process* newProcess = (process *)malloc(1*sizeof(process));;
	newProcess->pid = 1;
	newProcess->startTime = 2;
	newProcess->timeLeft = 10;
	newProcess->bursts = (bursts *)malloc(sizeof(bursts));
	newProcess->burstnum = 0;
	newProcess->bursts->burstc = 1;
	newProcess->bursts->burstv = (int *)malloc(newProcess->bursts->burstc*sizeof(int));
	newProcess->bursts->burstv[0] = 10;
	newProcess->waits = (waits *)malloc(sizeof(waits));
	newProcess->waitnum = 0;
	newProcess->waits->waitc = 0;
	newProcess->waits->waitv = (int *)malloc(newProcess->waits->waitc*sizeof(int));
	enq(processes, newProcess);
	process* newProcess2 = (process *)malloc(1*sizeof(process));;
	newProcess2->pid = 2;
	newProcess2->startTime = 5;
	newProcess2->timeLeft = 5;
	newProcess2->bursts = (bursts *)malloc(sizeof(bursts));
	newProcess2->burstnum = 0;
	newProcess2->bursts->burstc = 1;
	newProcess2->bursts->burstv = (int *)malloc(newProcess->bursts->burstc*sizeof(int));
	newProcess2->bursts->burstv[0] = 5;
	newProcess2->waits = (waits *)malloc(sizeof(waits));
	newProcess2->waitnum = 0;
	newProcess2->waits->waitc = 0;
	newProcess2->waits->waitv = (int *)malloc(newProcess->waits->waitc*sizeof(int));
	enq(processes, newProcess2);
	*/
}

void get_waits(waits* ws, char** tokens, int waitc){
	if (DEBUG) printf("getting wait times for process\n");
	int* waitv = malloc(sizeof(int) * waitc);
	char** nums = tokens + 3;
	int i = 0;

	while (i < waitc){
		waitv[i] = atoi((const char*)*nums);
		i++;
		nums = nums + 2;
	}

	ws->waitc = waitc;
	ws->waitv = waitv;
}

void get_bursts(bursts* bs, char** tokens, int burstc){
	if (DEBUG) printf("getting burst times for process\n");
	int* burstv = malloc(sizeof(int) * burstc);
	char** nums = tokens + 2;
	int i = 0;

	while (i < burstc){
		burstv[i] = atoi((const char*)*nums);
		i++;
		nums = nums + 2;
	}

	bs->burstc = burstc;
    bs->burstv = burstv;
}

/* Creates a new process */
void new_process(queue* toQueue,char* line, int pid){
	if (DEBUG) printf("Creating new Process, PID: %d\n", pid);
	process* newProcess= (process *)malloc(1*sizeof(process));;
    char* tokens[LINESIZE];
    int i = 0;

    tokens[i++] = strtok(line, " \n");
    while((tokens[i++] = strtok(NULL, " \n"))!= NULL);

    int start_time = atoi((const char*)tokens[0]);
    int burstc = atoi((const char*)tokens[1]);
    newProcess->bursts = malloc(sizeof(bursts*));
    newProcess->waits = malloc(sizeof(waits*));

    newProcess->pid = pid;
    newProcess->startTime = start_time;
    newProcess->ready_time = 0;
    newProcess->burstnum = 0;
    newProcess->cpu_time = 0;
    newProcess->wait_time = 0;
    //newProcess->sleep_until = 0;
    newProcess->first_time = 0;
    newProcess->seen = 0;
    newProcess->examined = 0;
    newProcess->finish = 0;

    get_bursts(newProcess->bursts, tokens, burstc);
    get_waits(newProcess->waits, tokens, burstc - 1);

    enq(toQueue, newProcess);
}

/* Create an empty queue */
queue createQueue()
{
	if (DEBUG) printf("Creating New Queue\n");
	queue queue;
	queue.count = 0;
	queue.rear = NULL;
	queue.front = NULL;
	queue.front1 = NULL;
	queue.temp = NULL;
	if (DEBUG) printf("New Queue Created\n");
	return queue;
}

/* Returns queue size */
int queueSize(queue* queue)
{
	if (DEBUG) printf("Queue size : %d\n", queue->count);
	return queue->count;
}

/* Enqueue item in the queue */
void enq(queue* queue, process* data)
{
	if (DEBUG) printf("Enqueueing process %d\n", data->pid);
	if (queue->rear == NULL)
	{
		queue->rear = (node *)malloc(1*sizeof(node));
		queue->rear->next = NULL;
		queue->rear->process = data;
		queue->front = queue->rear;
	}
	else
	{
		queue->temp=(node *)malloc(1*sizeof(node));
		queue->rear->next = queue->temp;
		queue->temp->process = data;
		queue->temp->next = NULL;
		queue->rear = queue->temp;
	}
	queue->count++;
}

/* Display the queue elements */
void displayQueue(queue* queue)
{
	printf("Queue: ");
	queue->front1 = queue->front;

	if ((queue->front1 == NULL) /*&& (queue->rear == NULL)*/)
	{
		printf("empty\n");
		return;
	}
	if (queue->rear == NULL){
		//TODO
	}
	while (queue->front1 != NULL/*queue->rear*/)
	{
		printf("%d ", queue->front1->process->pid);
		queue->front1 = queue->front1->next;
	}
	//if (queue->front1 == queue->rear)
		//printf("%d", queue->front1->process->pid);
	printf("\n");
}

/* Dequeue first item in the queue */
void deq(queue* queue)
{
	if (DEBUG) printf("Dequeing First Process\n");
	queue->front1 = queue->front;

	if (queue->front1 == NULL)
	{
		if(DEBUG)printf("Error: Trying to displayQueue elements from empty queue\n");
		return;
	}
	else{
		if (queue->front1->next != NULL)
		{
			queue->front1 = queue->front1->next;
			if(DEBUG)printf("Removed Process %d from queue\n", queue->front->process->pid);
			//free(queue->front);
			queue->front = queue->front1;
		}
		else
		{
			if(DEBUG)printf("Removed Process %d from queue\n", queue->front->process->pid);
			//free(queue->front);
			queue->front = NULL;
			queue->rear = NULL;
		}
		queue->count--;
	}
}

void removeProcess(queue* queue, process* process){
	if (DEBUG) printf("Removing process from queue\n");
	int i = 0;
	node* previousNode = NULL;
	node* nodeToCheck = queue->front;
	//for(i = 0; i < queue->count; i++){
	while(nodeToCheck != NULL){
		if (nodeToCheck->process->pid == process->pid){
			if (DEBUG) printf("Process %d found, Removing\n", nodeToCheck->process->pid);
			if(previousNode != NULL){ // I have a previous
				previousNode->next = nodeToCheck->next;
			}
			else{ // I don't have a previous
				queue->front = nodeToCheck->next;
			}
			if(nodeToCheck == queue->rear){
				queue->rear = previousNode;
			}
			else{
				queue->rear = nodeToCheck->next;
			}
			queue->count--;
		}
		previousNode = nodeToCheck;
		nodeToCheck = nodeToCheck->next;
	}
}

/* Returns the front element of queue */
process* frontElementOfQueue(queue* queue)
{
	if (DEBUG) printf("getting front element of queue\n");
	if ((queue->front != NULL) && (queue->rear != NULL))
		return(queue->front->process);
	else
		return NULL;
}

process* nextShortestProcess(queue* queue){
	if (DEBUG) printf("getting next shortest process in queue\n");
	if ((queue->front != NULL) && (queue->rear != NULL)){
		int i = 0;
		node* nodeToCheck = queue->front->next;
		process* shortestProcess = queue->front->process;
		//for(i = 0; i < queue->count-1; i++){
		while(nodeToCheck != NULL){
			if (nodeToCheck->process->bursts->burstv[nodeToCheck->process->burstnum] < shortestProcess->bursts->burstv[shortestProcess->burstnum]){
				shortestProcess = nodeToCheck->process;
			}
			nodeToCheck = nodeToCheck->next;
		}
		if(DEBUG) printf("Next Shortest is process %d\n", shortestProcess->pid);
		return shortestProcess;
	}
	else
		if (DEBUG) printf("ERROR: No Process in Queue\n");
		return NULL;
}

/* Return if queue is empty or not */
bool empty(queue* queue)
{
	if (DEBUG) printf("Checking if Queue is Empty\n");
	if (queue != NULL){
		if ((queue->front == NULL) && (queue->rear == NULL)){
			if (DEBUG) printf("Queue empty\n");
			return true;
		}
		else{
			if (DEBUG) printf("Queue not empty\n");
			return false;
		}
	}
	else{
		if (DEBUG) printf("Queue not initialized\n");
		return false;
	}
}


char** string_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 2);
        *(result + idx) = 0;
    }

    return result;
}
#define PID 0
#define TAT 1
#define IO 2
#define WT 3
#define RT 4
#define METRICS 5

void printStatistics(queue* queue){


}

void analyze_proc(process* process, int* analysis)
{
    int tat = process->finish - process->startTime;
    int rt = process->first_time - process->startTime;
    int wt = tat - process->cpu_time;
    int io = process->wait_time;

    analysis[PID] = process->pid;
    analysis[TAT] = tat;
    analysis[RT] = rt;
    analysis[WT] = wt;
    analysis[IO] = io;
}

void analyze_all(queue* queue)
{
    int* analysis = malloc(sizeof(int) * METRICS);
    int i = 0;
    int max_tat = 0;
    int max_wait = 0;
    int max_rt = 0;
    double cpu_util = 0;
    node* currentNode = queue->front;

    while (currentNode != NULL)
    {
    analyze_proc(currentNode->process, analysis);
	if (analysis[TAT] > max_tat) max_tat = analysis[TAT];
	if (analysis[RT] > max_rt) max_rt = analysis[RT];
	if (analysis[WT] > max_wait) max_wait = analysis[WT];
	cpu_util += (double)currentNode->process->cpu_time / (double)(currentNode->process->finish - currentNode->process->first_time);
	currentNode = currentNode->next;
    }
    cpu_util = 100 * (cpu_util / queue->count);
    printf("MAX RT: %d\n",max_rt);
    printf("MAX TAT: %d\n",max_tat);
    printf("MAX WAIT: %d\n",max_wait);
    printf("CPU UTIL: %f%c\n",cpu_util, 0x25);

    free(analysis);
}

void show_analysis(int* analysis, int timer)
{
    printf("Process %d terminated at t = %d.\n" ,analysis[PID], timer);
    printf("TAT: %d\nRT: %d\nWT : %d\nIO : %d\n", analysis[TAT], analysis[RT], analysis[WT], analysis[IO]);
}
