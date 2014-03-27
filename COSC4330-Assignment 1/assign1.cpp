#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iomanip>

#include "struct.h"

const int NUMBER_OF_DEVICES = 5;

using namespace std;

int checkCPU(Device *);
void freeDevice(Device *, int);
void createEvent(priority_queue<Event, vector<Event>, CompareEvent> *, string, int, int);
void computeOPERATION(string, priority_queue<Event, vector<Event>, CompareEvent> *, vector<Data> &, queue<Process> &,
                      vector<Process> &, queue<Process> &, queue<Process> &, int, Device *, Event, int &, int &);
void computeRUN(priority_queue<Event, vector<Event>, CompareEvent> *, vector<Data> &, vector<Process> &, queue<Process> &, queue<Process> &, int, Device *, int, int &);
void computeDISK(priority_queue<Event, vector<Event>, CompareEvent> *, vector<Data> &, vector<Process> &, queue<Process> &, int, Device *, int, int &, int &);
void computeTTY(priority_queue<Event, vector<Event>, CompareEvent> *, vector<Data> &, vector<Process> &, int, Event, int &);
void computeEND(vector<Process> &, Event, int &);

void printSummary(int, float, int, int, float, int, float, float);

int main()
{
    ifstream arq(getenv("MYARQ"));
    cin.rdbuf(arq.rdbuf());
    
    string line;
    istringstream iss;
    
    vector<Data> dataTable;
    vector<Process> processTable;
    
    priority_queue<Event, vector<Event>, CompareEvent> eventList;
    
    queue<Process> interactiveQueue;
    queue<Process> real_timeQueue;
    queue<Process> diskQueue;
    
    Device device[NUMBER_OF_DEVICES];
    
    for (int i = 0; i < NUMBER_OF_DEVICES; i++)
        device[i].isIdle = true;
    
    int processCounter = 0;
    int currentLine = 0;
    
    int totalRUN = 0;
    int totalDISK = 0;
    int diskCount = 0;
    int diskTime = 0;
    int totalRT = 0;        // total Real Time Process
    int totalRT_Miss = 0;   // total Real Time Process miss their deadline
    int totalIN = 0;        // total Interactive Process
    
    Data tempData;
    Process tempProcess;
    
    // ----- INPUT PHASE -----
    while (getline(cin, line))
    {
        string operation;
        int parameter;
        
        iss.clear();    // clear iss first
        iss.str(line);  // load line into iss
        iss >> operation >> parameter;  // use iss as cin
        
        tempData.operation = operation;
        tempData.parameter = parameter;
        
        dataTable.push_back(tempData);
        
        if (operation == "INTERACTIVE" || operation == "REAL-TIME")
        {
            tempProcess.pid = processCounter;
            tempProcess.type = operation;
            tempProcess.startTime = parameter;
            tempProcess.firstLine = currentLine;
            tempProcess.currentLine = currentLine;
            
            printf("%s process %d starts at t = %dms \n", operation.c_str(), processCounter, parameter);
            
            processTable.push_back(tempProcess);
            
            createEvent(&eventList, "NEW PROCESS", processCounter, parameter);
        }

        if (operation == "END")
        {
            processTable[processCounter].lastLine = currentLine;
            processCounter++;
        }
        
        if (operation == "DISK")
        {
            diskCount++;
            totalDISK += parameter;
        }
        
        if (operation == "RUN")
            totalRUN += parameter;
        
        currentLine++;  
    }
    
    // ----- PROCESSING PHASE -----
    int clock = 0; // current time
    currentLine = 0;
    
    // while event list is not empty, process next event in the list
    while (!eventList.empty())
    {
        Event tempEvent = eventList.top();
        eventList.pop();
        
        clock = tempEvent.time;
        int computingLine = processTable[tempEvent.pid].currentLine + 1;

        if (tempEvent.name == "NEW PROCESS")
        {
            if (dataTable[computingLine].operation == "DEADLINE")
            {
                computingLine++;
                processTable[tempEvent.pid].deadLine = dataTable[computingLine - 1].parameter;
            }
            
            computeOPERATION(dataTable[computingLine].operation, &eventList, dataTable, diskQueue,
                             processTable, interactiveQueue, real_timeQueue, computingLine, device, tempEvent, clock, diskTime);
        }
        
        else if (tempEvent.name == "RELEASE CORE")
        {
            freeDevice(device, tempEvent.pid);
            
            if (real_timeQueue.empty())
            {
                if (!interactiveQueue.empty())
                {
                    computingLine = interactiveQueue.front().currentLine + 1;
                    
                    computeRUN(&eventList, dataTable, processTable, interactiveQueue, real_timeQueue, computingLine, device, interactiveQueue.front().pid, clock);
                    
                    interactiveQueue.pop();
                }
            }
            else // real-time queue is not empty
            {
                computingLine = real_timeQueue.front().currentLine + 1;
                
                computeRUN(&eventList, dataTable, processTable, interactiveQueue, real_timeQueue, computingLine, device, real_timeQueue.front().pid, clock);
                
                real_timeQueue.pop();
            }

            computeOPERATION(dataTable[computingLine].operation, &eventList, dataTable, diskQueue,
                             processTable, interactiveQueue, real_timeQueue, computingLine, device, tempEvent, clock, diskTime);
        }
        
        else if (tempEvent.name == "RELEASE DISK")
        {
            freeDevice(device, tempEvent.pid);
            
            diskTime += clock;
            
            if (!diskQueue.empty())
            {
                int line = diskQueue.front().currentLine + 1;
                
                computeDISK(&eventList, dataTable, processTable, diskQueue, line, device, diskQueue.front().pid, clock, diskTime);
                
                diskQueue.pop();
                
                diskTime += clock;
            }
            
            computeOPERATION(dataTable[computingLine].operation, &eventList, dataTable, diskQueue,
                             processTable, interactiveQueue, real_timeQueue, computingLine, device, tempEvent, clock, diskTime);
        }
        
        else if (tempEvent.name == "WAITING TTY")
            computeOPERATION(dataTable[computingLine].operation, &eventList, dataTable, diskQueue,
                             processTable, interactiveQueue, real_timeQueue, computingLine, device, tempEvent, clock, diskTime);
        
        
    }
    
    // print simulation results
    
    for (int i = 0; i < processTable.size(); i++)
    {
        if (processTable[i].type == "REAL-TIME")
        {
            totalRT++;
            if (processTable[i].completionTime > processTable[i].deadLine)
                totalRT_Miss++;
        }
        else
            totalIN++;
    }
    
    float CPU_Utilization = (float) (totalRUN) / (float) (clock);
    float DISK_Utilization = (float) (totalDISK) / (float) (clock);
    float totalRT_Miss_Percent = (float) (totalRT_Miss * 100) / (float) (totalRT);
    float averageDiskTime = (float) (diskTime) / (float) (diskCount);
    
    printSummary(totalRT, totalRT_Miss_Percent, totalIN, diskCount, averageDiskTime, clock, CPU_Utilization, DISK_Utilization);
}

// function to check if any core is available and return that core number
int checkCPU(Device *core)
{
    for (int i = 0; i < 4; i++)
        if (core[i].isIdle)
            return i;
    
    return -1; // return -1 if all cores are busy
}

// function to free device which is currently holding process pid
void freeDevice(Device *device, int pid)
{
    for (int i = 0; i < NUMBER_OF_DEVICES; i++)
        if (device[i].currentProcess == pid)
        {
            device[i].isIdle = true;
            device[i].currentProcess = -1;
            device[i].completionTime = -1;
        }
}

void createEvent(priority_queue<Event, vector<Event>, CompareEvent> *eventList, string eventName, int pid, int time)
{
    Event newEvent;
    newEvent.name = eventName;
    newEvent.time = time;
    newEvent.pid = pid;
    
    eventList->push(newEvent);
}

void computeOPERATION(string operation, priority_queue<Event, vector<Event>, CompareEvent> *eventList, vector<Data> &dataTable, queue<Process> &diskQueue,
                      vector<Process> &processTable, queue<Process> &interactiveQueue, queue<Process> &real_timeQueue, int computingLine, Device *device, Event tempEvent, int &clock, int &diskTime)
{
    if (operation == "RUN")
        computeRUN(eventList, dataTable, processTable, interactiveQueue, real_timeQueue, computingLine, device, tempEvent.pid, clock);
    else if (operation == "DISK")
        computeDISK(eventList, dataTable, processTable, diskQueue, computingLine, device, tempEvent.pid, clock, diskTime);
    else if (operation == "TTY")
        computeTTY(eventList, dataTable, processTable, computingLine, tempEvent, clock);
    else if (operation == "END")
        computeEND(processTable, tempEvent, clock);
    
    
}

void computeRUN(priority_queue<Event, vector<Event>, CompareEvent> *eventList, vector<Data> &dataTable,
                vector<Process> &processTable, queue<Process> &interactiveQueue, queue<Process> &real_timeQueue, int computingLine, Device *device, int pid, int &clock)
{
    int CPU_Request_Time = dataTable[computingLine].parameter;
    
    int freeCore = checkCPU(device);
    
    // if a core is free
    if (freeCore != -1)
    {
        // mark core busy until clock + CPU_Request_Time
        device[freeCore].isIdle = false;
        device[freeCore].currentProcess = pid;
        device[freeCore].completionTime = clock + CPU_Request_Time;
        
        createEvent(eventList, "RELEASE CORE", pid, clock + CPU_Request_Time);
        
        processTable[pid].currentLine = computingLine;
    }
    else
    {
        if (processTable[pid].type == "INTERACTIVE")
            interactiveQueue.push(processTable[pid]);
        else
            real_timeQueue.push(processTable[pid]);
    }
}

void computeDISK(priority_queue<Event, vector<Event>, CompareEvent> *eventList, vector<Data> &dataTable,
                 vector<Process> &processTable, queue<Process> &diskQueue, int computingLine, Device *device, int pid, int &clock, int &diskTime)
{
    int DISK_Request_Time = dataTable[computingLine].parameter;
    diskTime -= clock;
    
    if (device[4].isIdle)
    {
        // mark disk busy until clock + DISK_Request_Time
        device[4].isIdle = false;
        device[4].currentProcess = pid;
        device[4].completionTime = clock + DISK_Request_Time;
        
        createEvent(eventList, "RELEASE DISK", pid, clock + DISK_Request_Time);
        
        processTable[pid].currentLine = computingLine;
    }
    else
        diskQueue.push(processTable[pid]);
}

void computeTTY(priority_queue<Event, vector<Event>, CompareEvent> *eventList, vector<Data> &dataTable,
                vector<Process> &processTable, int computingLine, Event tempEvent, int &clock)
{
    int TTY_Request_Time = dataTable[computingLine].parameter;
    processTable[tempEvent.pid].TTY_Delays = TTY_Request_Time;
    
    createEvent(eventList, "WAITING TTY", tempEvent.pid, clock + TTY_Request_Time);
    
    processTable[tempEvent.pid].currentLine = computingLine;
}

void computeEND(vector<Process> &processTable, Event tempEvent, int &clock)
{
    processTable[tempEvent.pid].completionTime = clock;
    printf("%s process %d terminates at t = %dms \n", processTable[tempEvent.pid].type.c_str(), tempEvent.pid, clock);
}

void printSummary(int totalRT, float totalRT_Miss_Percent, int totalIN, int diskCount, float averageDiskTime, int clock, float CPU_Utilization, float DISK_Utilization)
{
    cout << "\n==========SUMMARY==========\n"
         << "\nNumber of RT processes that completed: " << totalRT
         << "\nPercentage of RT processes that missed their deadline: " << setprecision(5) << totalRT_Miss_Percent << "%"
         << "\nNumber of interactive process that completed: " << totalIN
         << "\nTotal number of disk accesses " << diskCount
         << "\nAverage disk access time " << setprecision(5) << averageDiskTime
         << "\nTotal elapsed time: " << clock << "ms"
         << "\nCPU utilization: " << setprecision(3) << CPU_Utilization
         << "\nDisk utilization: " << setprecision(3) << DISK_Utilization << endl;
}