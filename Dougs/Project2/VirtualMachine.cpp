#include "VirtualMachine.h"
#include "Machine.h"
#include <errno.h>
#include <string.h>
#include <vector>
#include <stdlib.h>


//DELETE************
#include <iostream>
using namespace std;
//******************

extern "C" {    //Allow for backwards compabitlity with name mangling
volatile int tickCount, ret_flag, totalTicks, num_TCB, curr_TCB, tckInterval;
TMachineSignalState SigState;

class TCB{
private:
	TVMThreadState state;
	TVMThreadPriority pri;
	TVMThreadID id;
	TVMThreadEntry entry;
	TVMMemorySize stacksize;
	uint8_t * stack;
	SMachineContext context;

public:
	TCB(){ }
	TCB (TVMThreadState st, TVMThreadPriority pr){
		state = st;
		pri = pr;
		id = -1;
		entry = NULL;
		stacksize = 0;
	}
	TCB (TVMThreadState st, TVMThreadPriority pr, TVMThreadID tid, TVMThreadEntry fnc, TVMMemorySize memsize ){
		state = st;
		pri = pr;
		id = tid;
		entry = fnc;
	    stack = (uint8_t*)malloc(memsize*sizeof(uint8_t));
		stacksize = memsize;
	}
	~TCB (){
		free(stack);
	}

	void set_state (TVMThreadState st){
		state = st;
	}
	void set_priority (TVMThreadPriority pr){
		pri = pr;
	}
	void set_ID (TVMThreadID tid){
		id = tid;
	}
	void set_entry (TVMThreadEntry fnc){
		entry = fnc;
	}
	bool waiting (){
		return (VM_THREAD_STATE_WAITING == state);
	} 
	bool dead (){
		return (VM_THREAD_STATE_DEAD 	== state);
	}
	TVMThreadID get_ID (){
		return id;
	}
	void get_state (TVMThreadStateRef ref){
		*ref = state;
	}
	TVMThreadPriority get_priority (){
		return pri;
	}
	TVMThreadEntry get_entry (){
		return entry;
	}
	void * get_stack (){
		return (void*)stack;
	}
	TVMMemorySize get_size (){
		return stacksize;
	}
	SMachineContextRef get_context (){
		return &context;
	}

};std::vector<TCB*> tcbs;


//void MyCallback(void * cd, int result)
//    Use this to handle dif ferent function call backs
//(int?) MachineFileWrite(int fd, void * buffer, int len, void * callback, void * calldata)

//Order to Start
//Machine Initialize Starts the Machine 
//VMStart()
//Load Module (App) (ptr VMLoadModule(module.so))
//Call ptr

//Get Hello working
//Need to get VMFileWrite done first

//Then Impliment Sleep
//Requires Timer and MachineRequestAlarm
//while(sleepcount)
//volatile int sleepcount

//Note: Don't Make Vector<TCB> use Vector<TCB*>




/*            Prototypes                     */
TVMMainEntry VMLoadModule(const char *module);


/*                                            */

void MyCallback(void * cd, int result){
	totalTicks++;
	if (--tickCount <= 0){((TCB*)cd)->set_state(VM_THREAD_STATE_READY);}
	ret_flag = result;
}

void MyFileCallback(void * cd, int result){
	totalTicks++;
	if  (result < 0){
		std::string err = strerror(errno);
		MachineFileWrite(STDOUT_FILENO, (void*)err.c_str(), 
			err.length(), (TMachineFileCallback)MyFileCallback, (void*)VM_FILE_ERR);
	}
	
	ret_flag = result;
	((TCB*)cd)->set_state(VM_THREAD_STATE_READY);
}

TVMStatus VMStart(int tickms, int argc, char *argv[]){
	MachineInitialize();
	MachineEnableSignals();
	tcbs.push_back( new TCB(VM_THREAD_STATE_READY, VM_THREAD_PRIORITY_NORMAL));
	MachineRequestAlarm((useconds_t)(tickms*500),(TMachineAlarmCallback)MyCallback,((void*)tcbs[0]));
	ret_flag = -1;	totalTicks = 0;   tckInterval = tickms;
	num_TCB  =  0;	curr_TCB   = 0;

	TVMMainEntry fnc = VMLoadModule(argv[0]);
	if (NULL == fnc){
		MachineTerminate();
		return VM_STATUS_FAILURE;
	}
	fnc(argc, argv);
	

//****************DELETE*******************************************************
	TVMTickRef x  = new TVMTick();
	if (VMTickCount(x) == VM_STATUS_ERROR_INVALID_PARAMETER){cout << "ERR" << endl;}
	cout << "Total Ticks: " << *x << endl;
	delete x;
//*******************************************************************************

	MachineTerminate();
	return VM_STATUS_SUCCESS;
}

TVMStatus VMTickMS(int *tickmsref){
	if (NULL == tickmsref){return VM_STATUS_ERROR_INVALID_PARAMETER;}
	*tickmsref = tckInterval;
    return VM_STATUS_SUCCESS;	
} 


TVMStatus VMTickCount(TVMTickRef tickref){
	if (NULL == tickref){return VM_STATUS_ERROR_INVALID_PARAMETER;}
	*tickref = totalTicks;
	return VM_STATUS_SUCCESS;
}


TVMStatus VMThreadCreate( TVMThreadEntry entry, void *param, TVMMemorySize memsize,
					      TVMThreadPriority prio, TVMThreadIDRef tid){

	if (NULL == entry || NULL == tid){
		return VM_STATUS_ERROR_INVALID_PARAMETER;
	}

	++num_TCB; 
	TCB * new_tcb = new TCB(VM_THREAD_STATE_DEAD, prio, ((TVMThreadID)tcbs.size()+1), entry, memsize);
	tcbs.push_back(new_tcb);

	*tid = num_TCB;
	return VM_STATUS_SUCCESS;
}
//TVMStatus VMThreadDelete(TVMThreadID thread);
TVMStatus VMThreadActivate(TVMThreadID thread){
	if ((int)thread > (int)tcbs.size()){
		return VM_STATUS_ERROR_INVALID_ID;
	}else
	if (!(tcbs[(int)thread]->dead())){
		return VM_STATUS_ERROR_INVALID_STATE;
	}
	tcbs[(int)thread]->set_state(VM_THREAD_STATE_READY);

	
	MachineContextCreate( tcbs[thread] ->get_context(), tcbs[thread] -> get_entry(), tcbs[thread],
						  tcbs[thread] ->  get_stack(), tcbs[thread] ->  get_size() );

	return VM_STATUS_SUCCESS;
}
//TVMStatus VMThreadTerminate(TVMThreadID thread);
//TVMStatus VMThreadID(TVMThreadIDRef threadref);
TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
	if (NULL == stateref){return VM_STATUS_ERROR_INVALID_PARAMETER;}
	if ((int)thread > (int)tcbs.size()){return VM_STATUS_ERROR_INVALID_ID;}
	tcbs[thread]->get_state(stateref);

	return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadSleep(TVMTick tick){
	tickCount = (int)tick;
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
    while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }
	return VM_STATUS_SUCCESS;
}

//TVMStatus VMMutexCreate(TVMMutexIDRef mutexref);
//TVMStatus VMMutexDelete(TVMMutexID mutex);
//TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref);
//TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout);     
//TVMStatus VMMutexRelease(TVMMutexID mutex);

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
	if (NULL == filename || NULL == filedescriptor){
		return VM_STATUS_ERROR_INVALID_PARAMETER;
	}

	MachineSuspendSignals(&SigState);
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
	MachineFileOpen(filename,flags,mode,MyFileCallback,((void*)tcbs[curr_TCB]));
	MachineEnableSignals();
    while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }
	*filedescriptor = ret_flag;

    if (ret_flag < 0){return VM_STATUS_FAILURE;}
    ret_flag = -1;
	return VM_STATUS_SUCCESS;
}
TVMStatus VMFileClose(int filedescriptor){
	MachineSuspendSignals(&SigState);
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
	MachineFileClose(filedescriptor, MyFileCallback, ((void*)tcbs[curr_TCB]));
	MachineEnableSignals();
    while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }

    if (ret_flag < 0){return VM_STATUS_FAILURE;}
    ret_flag = -1;
	return VM_STATUS_SUCCESS;

}      
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
	if (NULL == data || NULL == length){
		return VM_STATUS_ERROR_INVALID_PARAMETER;
	}

	MachineSuspendSignals(&SigState);
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
    MachineFileRead(filedescriptor, data, *length, MyFileCallback, ((void*)tcbs[curr_TCB]));
	MachineEnableSignals();
     while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }
    *length = ret_flag;

    if (ret_flag < 0){return VM_STATUS_FAILURE;}
    ret_flag = -1;
	return VM_STATUS_SUCCESS;
}

TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
	if (NULL == data || NULL == length){
		return VM_STATUS_ERROR_INVALID_PARAMETER;
	}

	MachineSuspendSignals(&SigState);
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
	MachineFileWrite(filedescriptor, data, *length, MyFileCallback, ((void*)tcbs[curr_TCB]));
	MachineEnableSignals();
     while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }
  	*length = ret_flag;

    if (ret_flag < 0){return VM_STATUS_FAILURE;}
    ret_flag = -1;
	return VM_STATUS_SUCCESS;
}

TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
	MachineSuspendSignals(&SigState);
    tcbs[curr_TCB]->set_state(VM_THREAD_STATE_WAITING);
	MachineFileSeek(filedescriptor, offset, whence, MyFileCallback, ((void*)tcbs[curr_TCB]));
	MachineEnableSignals();
    while( tcbs[curr_TCB]->waiting() ){cout << "\0"; }
	*newoffset = ret_flag;

    if (ret_flag < 0){return VM_STATUS_FAILURE;}
    ret_flag = -1;
	return VM_STATUS_SUCCESS;	
}



}
