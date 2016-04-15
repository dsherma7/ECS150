extern "c"{     //Allow for backwards compabitlity with name mangling

//void MyCallback(void * cd, int result)
//    Use this to handle different function call backs
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

}
