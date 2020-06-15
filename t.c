/*********** t.c file of A Multitasking System *********/
#include <stdio.h>
#include "type.h"

PROC proc[NPROC]; // NPROC PROCs
PROC *freeList;   // freeList of PROCs
PROC *sleepList;   // sleepList of PROCs
PROC *readyQueue; // priority queue of READY procs
PROC *running;    // current running proc pointer

#include "queue.c" // include queue.c file
int body();
int kexit(int exitValue);
void tswitch();
int ksleep(int event);
int kawake(int event);
int kwakeup(PROC *p);
int kwait(int *status);
void enter_child(PROC **queue,PROC *p);
void des_free_children(PROC *p);
void showChild(PROC *p);
int *status;
/*******************************************************
kfork() creates a child process; returns child pid.
When scheduled to run, child PROC resumes to body();
********************************************************/
int kfork()
{
  int i;
  PROC *p = dequeue(&freeList);
  if (!p){
    printf("no more proc\n");
    return(-1);
  }

  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1; // ALL PROCs priority=1,except P0
  p->ppid = running->pid;
  p->parent = running;

  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                       -1   -2  -3  -4  -5  -6  -7  -8  -9
   **********************************************************/
  for (i=1; i<10; i++)              // zero out kstack cells
    p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE-1] = (int)body;   // retPC -> body()
  p->ksp = &(p->kstack[SSIZE - 9]); // PROC.ksp -> saved eflag
  enqueue(&readyQueue, p);          // enter p into readyQueue
  enter_child(&running,p);
  return p->pid;
}/*end kfork()*/

int kexit(int exitValue)
{
  
  running->priority = 0;
  //enqueue(&freeList, running);
  //printList("freeList", freeList);
  while(running->child){
  des_free_children(running);
  }
  running->exitCode = exitValue;
  running->status = ZOMBIE;
  kwakeup(running->parent);
  tswitch();
}/*end kexit()*/

int do_kfork()
{
  int child = kfork();
  if (child < 0)
    printf("kfork failed\n");
  else{
    printf("proc %d kforked a child = %d\n", running->pid, child);
    printList("readyQueue", readyQueue);
  }
  return child;
}/*end do_kfork()*/

int do_switch()
{
  tswitch();
}/*end do_switch()*/

int do_exit()
{
  kexit(0);
}/*end do_exit()*/

int body() // process body function
{
  int c;
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printList("sleepList", sleepList);
    printf("***************************************\n");
    printf("proc %d running: Parent=%d\n", running->pid,running->ppid);
    printf("child = %s\n","NULL");
    printf("input a char [f|s|q|c|w] : ");
    c = getchar(); getchar(); // kill the \r key
    switch(c){
      case 'f': do_kfork(); break;
      case 's': do_switch(); break;
      case 'q': do_exit(); break;
      case 'c': showChild(running); break;
      case 'w': kwait(status); break;
    }
  }
}/*end body()*/

// initialize the MT system; create P0 as initial running process
int init()
{
  int i;
  PROC *p;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    p->pid = i;            // PID = 0 to NPROC-1
    p->status = FREE;
    p->priority = 0;
    p->next = p+1;
  }
  proc[NPROC-1].next = 0;
  freeList = &proc[0];     // all PROCs in freeList
  readyQueue = 0;          // readyQueue = empty

  // create P0 as the initial running process
  p = running = dequeue(&freeList); // use proc[0]
  p->status = READY;
  p->ppid = 0; // P0 is its own parent
  printList("freeList", freeList);
  printf("init complete: P0 running\n");
}/*end init()*/

/*************** main() function ***************/
int main()
{
  printf("Welcome to the MT Multitasking System\n");
  init(); // initialize system; create and run P0
  kfork(); // kfork P1 into readyQueue
  while(1){
    printf("P0: switch task\n");
    if (readyQueue)
      tswitch();
  }
}/*end main()*/

/*********** scheduler *************/
int scheduler()
{
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
    enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);
}/*end scheduler()*/

int ksleep(int event)
{
  running->event=event;
  running->status = SLEEP;
  enqueue(&sleepList,running);
  tswitch();
}
int kawake(int event)
{
  PROC **LS = &sleepList;
  PROC *p = *LS;
  while(p)
     {
      PROC *q = p->next;
      printf("Depertar = %d\n", p->pid);
      if(p->event == event){
      	*LS=(*LS)->next;
	p->status=READY;
	enqueue(&readyQueue,p);
      }
      p=q;
     }
}

int kwakeup(PROC *p)
{
  PROC **LS = &sleepList;
  PROC *r = *LS;
  while(r)
     {
      PROC *q = r->next;
      printf("Depertar = %d\n", r->pid);
      if(r->pid == p->pid){
      	*LS=(*LS)->next;
	r->status=READY;
	enqueue(&readyQueue,r);
      }
      r=q;
     }
}

void enter_child(PROC **queue,PROC *p)
{
  PROC *current=*queue;
  if(queue==NULL || p==NULL){
     return;
     }else{
      if((*queue)->child==NULL){
         (*queue)->child=p;
         printf("Primogenito de %d\n", (*queue)->pid);
         return;
         }else{
             current=current->child;
	     while(current->sibling)
               current=current->sibling;
          }
           current->sibling=p;
	   p->sibling=NULL;
	   printf("Soy %d y Mi hermano es: %d\n",p->pid, current->pid);
      }
}

void des_free_children(PROC *p)
{ 
  PROC *tmp,*cur;
  if(p->child==NULL){
   return;
   }
  tmp=p->child;
  if(tmp->status==FREE || tmp->status==READY){
  tmp->parent = p->parent;
  tmp->ppid = p->ppid;
  PROC *tmpr = tmp->sibling;
  enter_child(&(p->parent),tmp);
  printf("Adopcion de %d completada, nuevo papa: %d\n", tmp->pid,p->ppid);
  p->child=tmpr;
  return;
  }

  cur=tmp;
  while(tmp=tmp->sibling){
      if(tmp->status==FREE){
         cur->sibling=tmp->sibling;
         return;
        }
      cur=tmp;
  }
}

void showChild(PROC *p)
{
  PROC *tmp;
  if(p!=NULL){
     if(p->child){
        tmp=p->child;
        printf("[%d %s] -> ",tmp->pid,strStatus[tmp->status]);
        if(tmp=tmp->sibling){
           while(tmp!=NULL){
                if(tmp->sibling==NULL){
                   printf("[%d %s] -> NULL\n",tmp->pid,strStatus[tmp->status]);
                 }else{
                   printf("[%d %s] -> ",tmp->pid,strStatus[tmp->status]);
                 }
                 tmp=tmp->sibling;
                }
            }else{
              printf("NULL\n");
             }
       }
    }
}

int kwait(int *status)
{
  if(running->child==NULL){
     return -1;
     }
  PROC *p = running->child;
  while(1){  
 //printf("ciclo\n");
     while(p){
          PROC *q = p->sibling;
          if(p->status=ZOMBIE){
	     //printf("IF\n");
             int ZPID = p->pid;
             status = &(p->exitCode);
             p->status=FREE;
	     (p->parent)->child = q;  
             enqueue(&freeList,p);
             return p->pid;
            }
       } 
      ksleep(0);
    }
}        
