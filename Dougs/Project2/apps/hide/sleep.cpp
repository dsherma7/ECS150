#include "VirtualMachine.h"
#include <iostream>
using namespace std;

void VMMain(int argc, char *argv[]){
    VMPrint("Going to sleep for 10 ticks\n");
    cout << "VMPrint(Going to sleep for 10 ticks\n);" << endl;
    VMThreadSleep(10);
    cout << " VMThreadSleep(10);" << endl;
    VMPrint("Awake\nGoodbye\n");
    cout << " VMPrint(Awake\nGoodbye\n);" << endl;
}

