#include <string>

using namespace std;

struct Data
{
    string operation;
    int parameter;
};

struct Process
{
    int pid; // process sequence number
    string type;
    int firstLine;
    int lastLine;
    int currentLine;
    int startTime;
    int completionTime;
    int TTY_Delays;
    int deadLine; // deadline for real-time processes
};

struct Device
{
    bool isIdle;
    int currentProcess;
    int completionTime;
};

struct Event
{
    string name;
    int time;
    int pid;
};

class CompareEvent
{
public:
    bool operator()(Event& event1, Event& event2)
    {
        if (event1.time > event2.time)
            return true;
        
        return false;
    }
};