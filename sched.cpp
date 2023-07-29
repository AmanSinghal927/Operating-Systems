#include <iostream>
#include "getopt.h"
#include <fstream>
#include <string.h> // What is the difference between #include <string> and #include <string.h>
#include <cctype>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <list>
using namespace std;

int max_prio=4;
int ofs = 0; // Stands for offset 
enum State {CREATED, READY, RUNNING, BLOCKED}; 
const char* state_names[] = {"CREATED", "READY", "RUNNG", "BLOCK"};
enum Trans {TRANS_TO_READY, TRANS_TO_PREEMPT, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_DONE};
vector <int> randvals;
int time_cpubusy = 0;
int time_iobusy = 0;
int quantum = 10000;
string scheduler_type;
bool verbose;

int myrandom(int burst) {
    if (ofs == randvals.size()){ofs = 1;}
    else{ofs = ofs + 1;}
    return 1 + (randvals[ofs]%burst);
    }

// Process class to create process objects 
class Process{
    public:
        int pid;
        State state; 
        int AT; 
        int TC;
        int CB; 
        int IO; 
        int dp; 
        int sp; 
        int FT = 0; //Finishing time 
        int TT = 0; //Turnaround time
        int IOT = 0; // I/O time
        int CW = 0; // CPU waiting time -> How much time in ready state
        int ST; // Time when the process went into state
        int ITC; // Initial total completion 
        int RCPU = 0; // Remaining CPU Burst for that process

        Process(int pid, int AT, int TC, int CB, int IO){
            this->pid = pid; 
            this-> AT = AT; 
            this-> TC = TC; 
            this-> CB = CB;
            this-> IO = IO;
            this-> state = CREATED;
            this->ITC = TC;
            }
};
vector<Process> processQueue; 

// Event class
class Event{
    public:
        Process *p;
        int timeStamp;
        State prevState;
        Trans transition;
        // Returns nothing; just creates the process 
        Event(Process *p, int timeStamp, Trans transition){
            this->p = p;
            this->timeStamp = timeStamp;
            this->transition = transition;
        }

};

class DES_layer{
    public:
        list <Event> eventQueue; 
        void put_event(Event e){
            list <Event>::iterator e_iter = eventQueue.begin();
            if (eventQueue.empty()){
                eventQueue.insert(e_iter, e);
            }

            else{
                // Dereferencing the iterator object with ->
                while (e.timeStamp>=e_iter->timeStamp){
                    advance(e_iter, 1);
                }
                eventQueue.insert(e_iter, e); 
            }
        }

        Event* get_event(){
            if (!(eventQueue.empty())){
                return (&eventQueue.front());
            }
            else{
                return NULL;
            }
        }

        void rm_event(){
            eventQueue.pop_front();
        }
        
        int get_next_event_time(){
            if (eventQueue.empty()){
                return -1;
            }
            else{
                return (eventQueue.front().timeStamp);
            }
        }

        int get_next_event_time_process(Process *pro){
            for(auto i = eventQueue.begin() ; i!= eventQueue.end(); i++){
                Event& e = *i;
                if (e.p->pid == pro->pid){
                    return (e.timeStamp);}
                }
            return (10000);
            }

        void rm_proc_event(Process *pro){
            for(auto i = eventQueue.begin() ; i!= eventQueue.end(); i++){
                Event& e = *i;
                if (e.p->pid == pro->pid){
                    eventQueue.erase(i);
                    break;}
                }
            }
};

// Sheduler has to work on a process level -> Think about FCFS and LCFS
class Scheduler{
    public:
    virtual void add_process(Process *p) = 0; // Keyword virtual enables polymorphism 
    virtual Process* get_next_process() = 0;
    virtual bool does_preempt() = 0;
};

Scheduler *scheduler;
list <Process*> readyQueue;

class FCFS : public Scheduler{
    public:
    void add_process(Process *p){
        readyQueue.push_back(p);
    }

    Process* get_next_process(){
        if (readyQueue.empty()){
            return NULL;
        }
        Process* p;
        p = readyQueue.front();
        readyQueue.pop_front();
        return (p);
    }

    bool does_preempt(){
        return (false);
    }

};

class LCFS : public Scheduler{
    public:
    void add_process(Process *p){
        readyQueue.push_back(p);
    }

    Process* get_next_process(){
        if (readyQueue.empty()){
            return NULL;
        }
        Process* p;
        p = readyQueue.back();
        readyQueue.pop_back();
        return (p);
    }

    bool does_preempt(){
        return (false);
    }

};

class SRTF : public Scheduler {
    public:
    void add_process(Process *p) {
        readyQueue.push_back(p);
    }

Process* get_next_process() {
    if (readyQueue.empty()) {
        return NULL;
    }

    Process* p;
    p = readyQueue.front();
    list<Process*>::iterator shortest_process_iter = readyQueue.begin();
    list<Process*>::iterator r_iter;

    for (r_iter = readyQueue.begin(); r_iter != readyQueue.end(); ++r_iter) {
        if ((*r_iter)->TC < p->TC) {
            p = *r_iter;
            shortest_process_iter = r_iter;
        } 
    }

    readyQueue.erase(shortest_process_iter);
    return p;
}

    bool does_preempt() {
        return (false);
    }
};

class RR : public Scheduler{
    public:
    bool does_preempt(){
        return (false);
    }
    void add_process(Process *p){
        readyQueue.push_back(p);
    }

    Process* get_next_process(){
        if (readyQueue.empty()){
            return NULL;
        }
        Process* p;
        p = readyQueue.back();
        readyQueue.pop_back();
        return (p);
    }

};

class PRIO : public Scheduler {
    // Create list of lists and their ptrs
public:
    vector<list<Process*>> q1;
    vector<list<Process*>> q2;
    vector<list<Process*>>* activeQ;
    vector<list<Process*>>* expiredQ;
    vector<list<Process*>>* temp;

    int c_active = 0;
    int c_expired = 0;

    // Constructor
    PRIO() {
        // Initialization
        for (int i = 0; i < max_prio; i++) {
            q1.push_back(list<Process*>()); // Calling the default constructor
        }

        for (int i = 0; i < max_prio; i++) {
            q2.push_back(list<Process*>());
        }
        activeQ = &q1;
        expiredQ = &q2;
    }

    void add_process(Process* p) {
        int sp = p->sp;
        int dp = p->dp;
        // if the dp is less than 0 then add to expired queue with sp - 1
        if (dp < 0) {
            p->dp = sp - 1;
            dp = p->dp;
            // add to expiredQ
            auto it = expiredQ->begin();
            advance(it, dp);
            it->push_back(p);
            c_expired = c_expired + 1;
        }
        else {
            auto it = activeQ->begin();
            advance(it, dp);
            it->push_back(p);
            c_active = c_active + 1;
        }
    }
    Process* get_next_process() {
        if (c_active == 0) {
            // conduct a swap
            temp = expiredQ;
            expiredQ = activeQ;
            activeQ = temp;
            swap(c_active, c_expired);
        }
        // Now get a process from the active queue
        for (int i = max_prio - 1; i >= 0; i--) {
            auto it = activeQ->begin();
            advance(it, i);
            if (!it->empty()) {
                c_active = c_active - 1;
                Process* p;
                p = it->front();
                it->pop_front();
                return (p);
            }
        }
        return NULL; // Return nullptr if no process is found
    }
    bool does_preempt(){
        return (true);
    }
};

void Simulation(DES_layer *des){
    int io_start = 0;
    int io_end = 0;
    Event* e; // Get the first ever event
    bool CALL_SCHEDULER = false;
    Process *CURRENT_RUNNING_PROCESS = NULL;
    int prev_cpu = 0;
    while ((e = des->get_event())!=NULL){
        Process *proc = e->p;
        Trans transition = e->transition;
        int CURRENT_TIME = e->timeStamp;
        int timeInPrevState = CURRENT_TIME - proc->ST; // When did a process enter the previous state?
        State prev_state = e->prevState;
        des->rm_event();
        int cpu = 0;
        // Resetting the dynamic priority 
        switch (transition)
        {
        case TRANS_TO_READY:{
            if (prev_state == CREATED){
                if (verbose){
                    printf("%d %d %d: %s -> READY\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state]);}
            }
            else{
                if (prev_state == BLOCKED){
                    proc->dp = proc->sp - 1; // Resetting the priority 
                    if (verbose){
                        printf("%d %d %d: %s -> READY\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state]);}
                    proc->ST = CURRENT_TIME;
                }
            }
            
            if ((scheduler_type == "PREPRIO") && (CURRENT_RUNNING_PROCESS!=NULL)){
                if ((proc->dp > CURRENT_RUNNING_PROCESS->dp) && (CURRENT_TIME < des->get_next_event_time_process(CURRENT_RUNNING_PROCESS))){
                    des->rm_proc_event(CURRENT_RUNNING_PROCESS);
                    Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME, TRANS_TO_PREEMPT);
                    e_new.prevState = RUNNING;
                    des->put_event(e_new);
                    if (CURRENT_TIME>CURRENT_RUNNING_PROCESS->ST){
                        time_cpubusy = time_cpubusy - (prev_cpu - (CURRENT_TIME - CURRENT_RUNNING_PROCESS->ST)); // Of the initially thought utilization, the CPU didn't run for this much 
                        // printf("CURRENT_RUNNING_PROCESS->RCPU %d prev_cpu %d CURRENT_TIME %d CURRENT_RUNNING_PROCESS->ST %d \n", CURRENT_RUNNING_PROCESS->RCPU, prev_cpu, CURRENT_TIME, CURRENT_RUNNING_PROCESS->ST);
                        CURRENT_RUNNING_PROCESS->RCPU = prev_cpu - (CURRENT_TIME - CURRENT_RUNNING_PROCESS->ST) + CURRENT_RUNNING_PROCESS->RCPU;
                        CURRENT_RUNNING_PROCESS->RCPU = max(CURRENT_RUNNING_PROCESS->RCPU, 0);
                        CURRENT_RUNNING_PROCESS->TC = CURRENT_RUNNING_PROCESS->TC + prev_cpu - (CURRENT_TIME - CURRENT_RUNNING_PROCESS->ST);
                    }
                    else if (CURRENT_TIME < CURRENT_RUNNING_PROCESS->ST){
                        // printf("(II) CURRENT_RUNNING_PROCESS->RCPU %d prev_cpu %d CURRENT_TIME %d CURRENT_RUNNING_PROCESS->ST %d \n", CURRENT_RUNNING_PROCESS->RCPU, prev_cpu, CURRENT_TIME, CURRENT_RUNNING_PROCESS->ST);
                        CURRENT_RUNNING_PROCESS->RCPU = max((CURRENT_RUNNING_PROCESS->ST - CURRENT_TIME) + CURRENT_RUNNING_PROCESS->RCPU, 0);
                        CURRENT_RUNNING_PROCESS->TC = CURRENT_RUNNING_PROCESS->RCPU;
                        time_cpubusy = time_cpubusy - CURRENT_RUNNING_PROCESS->TC;
                    }
                }
            }

            scheduler->add_process(proc);
            CALL_SCHEDULER = true;
            break;
            }

        case TRANS_TO_RUN:
        {
            // Process is running here 
            CURRENT_RUNNING_PROCESS = proc;
            int prio = proc->dp;
            int p_cpuburst = proc->CB; 

        // des will give us that the next event is supposed to be 
            if (proc->RCPU>0){
                cpu = proc->RCPU;
                proc->RCPU = max(proc->RCPU-quantum, 0);
            }

            if (cpu == 0){
                cpu = myrandom(p_cpuburst);
                } 
            prev_cpu = min(cpu, quantum);
        // incorporating time quantum
            if (cpu > quantum) {
                proc->RCPU = cpu - quantum;
                cpu = quantum;
                transition = TRANS_TO_PREEMPT; // Change the transition to preemption
            }
            else{
                transition = TRANS_TO_BLOCK;
            }


            if ((p_cpuburst>=cpu) & (cpu<proc->TC)){
                // create an event to block
                if (verbose){
                    printf("%d %d %d: %s -> RUNNG cb=%d rem=%d prio=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], cpu, proc->TC, prio);}
                proc->TC = proc->TC - cpu;
                time_cpubusy = time_cpubusy + cpu;
                Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME + cpu, transition); 
                cpu = 0;
                e_new.prevState = RUNNING;
                des->put_event(e_new);
                CURRENT_TIME = CURRENT_TIME + cpu;
                // printf("1. CURRENT_TIME %d\n", CURRENT_TIME);
                proc->ST = CURRENT_TIME;

            }
            else{
                if ((p_cpuburst>=cpu) & (cpu>=proc->TC)){
                    // Create a done event 
                    if (verbose){
                        printf("%d %d %d: %s -> RUNNG cb=%d rem=%d prio=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], proc->TC, proc->TC, prio);}
                    time_cpubusy = time_cpubusy + proc->TC;
                    cpu = cpu - proc->TC;
                    Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME + proc->TC, TRANS_TO_DONE); 
                    des->put_event(e_new);
                    CURRENT_TIME = CURRENT_TIME + proc->TC; 
                    proc->FT = CURRENT_TIME;
                    proc->ST = CURRENT_TIME;
                    // printf("2. CURRENT_TIME %d\n", CURRENT_TIME);

                    // CURRENT_RUNNING_PROCESS = NULL;

                }
                else{
                    if ((p_cpuburst<cpu) & (cpu<proc->TC)){
                        // remaining cpu burst is non zero
                        cpu = cpu - p_cpuburst;
                        // send event to block
                        if (verbose){
                            printf("%d %d %d: %s -> RUNNG cb=%d rem=%d prio=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], p_cpuburst, proc->TC, prio);}
                        proc->TC = proc->TC - cpu;
                        time_cpubusy = time_cpubusy + cpu;
                        Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME + p_cpuburst, transition); 
                        e_new.prevState = RUNNING;
                        des->put_event(e_new);
                        CURRENT_TIME = CURRENT_TIME + p_cpuburst;
                        proc->ST = CURRENT_TIME;
                        // printf("3. CURRENT_TIME %d\n", CURRENT_TIME);

                    }
                    else{
                        // send event to done 
                        if (verbose){
                            printf("%d %d %d: %s -> RUNNG cb=%d rem=%d prio=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], proc->TC, proc->TC, prio);}
                        cpu = cpu - proc->TC;
                        time_cpubusy = time_cpubusy + proc->TC;
                        CURRENT_TIME = CURRENT_TIME + proc->TC;
                        proc->FT = CURRENT_TIME;
                        Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME, TRANS_TO_DONE);
                        des->put_event(e_new);
                        proc->ST = CURRENT_TIME;
                        // printf("4. CURRENT_TIME %d\n", CURRENT_TIME);

                    }
                }
            }
            proc->CW = proc->CW + timeInPrevState;
            CALL_SCHEDULER = true;
            break;
        }

        case TRANS_TO_PREEMPT: {
            if (verbose){
                printf("%d %d %d: %s -> READY cb=%d rem=%d prio=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], cpu, proc->TC, proc->dp);}
            proc->ST = CURRENT_TIME;
            proc->dp = proc->dp - 1;
            scheduler->add_process(proc);
            CALL_SCHEDULER = true;
            CURRENT_RUNNING_PROCESS = NULL;
            break;
        }

        case TRANS_TO_DONE:{
            if (verbose){
                printf ("%d %d %d: Done\n", CURRENT_TIME, proc->pid, proc->TC);}
            CURRENT_RUNNING_PROCESS = NULL;
            CALL_SCHEDULER = true;
            break;
        }


        case TRANS_TO_BLOCK:{
            // If process is in blocked state then 
            // CURRENT_RUNNING_PROCESS = proc;
            int prio = proc->dp;
            int p_io = proc->IO; 
            int io = myrandom(p_io);
            if (verbose){
                printf("%d %d %d: %s -> BLOCK ib=%d rem=%d\n", CURRENT_TIME, proc->pid, timeInPrevState, state_names[prev_state], io, proc->TC);}

            if (io_end < CURRENT_TIME + io) { 
                time_iobusy = time_iobusy + io_end - io_start;
                if (CURRENT_TIME > io_end){
                    io_start = CURRENT_TIME;}
                else{
                    io_start = io_end;}
                io_end = CURRENT_TIME + io;
            }

            Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME + io, TRANS_TO_READY); 
            proc->IOT = proc->IOT + io;
            e_new.prevState = BLOCKED;
            des->put_event(e_new);
            CALL_SCHEDULER = true;
            proc->ST = CURRENT_TIME;
            CURRENT_RUNNING_PROCESS = NULL;
            break;
            
        }
            

        
        default:
            break;
        }
    if (CALL_SCHEDULER){
        // 
        if (des->get_next_event_time() == CURRENT_TIME){ // For instance output2, events initialize at same timestep
            continue; 
        }
        CALL_SCHEDULER = false;
        // Get the right event from the sceduler to generate the run event for that process
        if (CURRENT_RUNNING_PROCESS == NULL){
            CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
            if (CURRENT_RUNNING_PROCESS == NULL){
                continue;
            }
            Event e_new(CURRENT_RUNNING_PROCESS, CURRENT_TIME, TRANS_TO_RUN); 
            e_new.prevState = READY;
            des->put_event(e_new);
            CURRENT_RUNNING_PROCESS = NULL;
        }
    }
    }
    time_iobusy = time_iobusy + io_end - io_start;
}



int main(int argc, char *argv[]){ // Number of inputs and the inputs argv[0] is the filename
    int opt;
    const char* schedspec;
    string ipath;
    string rpath;
    bool verbose = false;
    bool t_flag = false;
    bool e_flag = false;
    bool p_flag = false;
    bool i_flag = false;
    char shed;
    // verbose = 1; 
    // rpath = "C:/Users/J C SINGLA/OneDrive/Desktop/OS/lab2/lab2_assign/rfile";
    // ipath = "C:/Users/J C SINGLA/OneDrive/Desktop/OS/lab2/lab2_assign/input1";
    // max_prio = 4;
    // scheduler_type = "PREPRIO";
    // quantum = 4;

    // Take all the inputs using getopt
    while ((opt = getopt(argc, argv, "vtes:")) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
                break;
            case 't':
                t_flag = true;
                break;
            case 'e':
                e_flag = true;
                break;
            case 'p':
                p_flag = true;
                break;
            case 'i':
                i_flag = true;
                break;
            case 's':
                schedspec = optarg;
                break;
            default:
                return 1;
        }
    }

    // Also handle the case when randfile and inputfile are not given
    if (optind + 1 < argc){ // index of the first non-options array
        ipath = argv[optind];
        rpath = argv[optind + 1];
        optind = 1;
    }
    else{
        return 1;
    }

    // Resolving the -s argument 
    sscanf(schedspec, "%c%d:%d", &shed, &quantum, &max_prio);

    switch(shed){
        case 'F':
            scheduler_type = "FCFS";
            break;
        case 'L':
            scheduler_type = "LCFS";
            break;
        case 'S':
            scheduler_type = "SRTF";
            break;
        case 'R':
            scheduler_type = "RR";
            break;
        case 'P':
            scheduler_type = "PRIO";
            break;
        case 'E':
            scheduler_type = "PREPRIO";
            break;
        default:
            break;
    }

    // // Read the input file
    ifstream rfile;
    rfile.open(rpath);
    string line;
    // Read rfile for generating priorities
    while (getline(rfile, line)) {
        randvals.push_back(atoi(line.c_str()));
    }
    rfile.close();


    // Store the read processes in a process block
    ifstream ifile;
    string s;
	int pid = 0;
    ifile.open(ipath);
    int AT;
    int TC; 
    int CB; 
    int IO;
	while (ifile>>s){
        AT = atoi(s.c_str());
        ifile>>s;
        TC = atoi(s.c_str());
        ifile>>s;
        CB = atoi(s.c_str());
        ifile>>s;
        IO = atoi(s.c_str());
        Process p(pid,AT,TC,CB,IO);
        p.sp = myrandom(max_prio);
        p.dp = p.sp - 1;
		pid++;
        processQueue.push_back(p);
	}
	ifile.close();

    // Initializing the DES_layer with prevstate CREATED
    DES_layer des;
    int timestamp;
    for (int i = 0; i<processQueue.size(); i++){
        timestamp = processQueue[i].AT;
        Event e1(&processQueue[i], timestamp, TRANS_TO_READY); 
        e1.prevState = CREATED;
        e1.p->ST = timestamp; // At what timestamp does this event start
        des.put_event(e1);
    }


    if (scheduler_type == "FCFS") {
        scheduler = new FCFS();
    } else if (scheduler_type == "LCFS") {
        scheduler = new LCFS();
    } else if (scheduler_type == "SRTF") {
        scheduler = new SRTF();
    } else if (scheduler_type == "RR") {
        scheduler = new FCFS();
    } else if (scheduler_type == "PRIO") {
        scheduler = new PRIO();
    } else if (scheduler_type == "PREPRIO") {
        scheduler = new PRIO();
    } else {
        std::cout << "Invalid scheduler type!" << std::endl;}


    // scheduler->add_process(&processQueue[0]);
    // scheduler->add_process(&processQueue[1]);
    // Process* p = scheduler->get_next_process();
    // p = scheduler->get_next_process();
    // printf("%d", p->pid);

    
    Simulation(&des);  
    printf ("%s",scheduler_type.c_str()); 
    // Print time quantum if RR, PRIO, PREPRIO
    if ((scheduler_type == "RR") || (scheduler_type == "PRIO") || (scheduler_type == "PREPRIO")){
        printf(" %d\n", quantum);
    }
    else{
        printf("\n");
    }

    // Print overall calculations 
    int finishtime = 0;
    int num_processes = 0;
    int total_TT = 0;
    int total_CW = 0;
    for (int i = 0;i<processQueue.size();i++){
        Process final_p = processQueue[i];
        num_processes = num_processes + 1;
        int pid = final_p.pid;
        int AT = final_p.AT;
        int ITC = final_p.ITC;
        int CB = final_p.CB;
        int IO = final_p.IO;
        int prio = final_p.sp;
        int FT = final_p.FT;
        int TT = FT - AT;
        int CW = final_p.CW;
        total_TT = total_TT + TT;
        total_CW = total_CW + CW;
        int IT = final_p.IOT;
        if (FT>=finishtime){
            finishtime = FT;}
            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", pid, AT, ITC, CB, IO, prio, FT, TT, IT, CW);
    }
    processQueue.clear();
    randvals.clear();
    readyQueue.clear();

    // Calculate SUM variables
    double cpu_util = 100.0 * (time_cpubusy / (double) finishtime);
    double io_util = 100.0 * (time_iobusy / (double) finishtime);
    double throughput = 100.0 * (num_processes / (double) finishtime);
    double avg_TT = total_TT / (double) num_processes;
    double avg_CW = total_CW / (double) num_processes;
    printf("SUM: %d %.2lf %.2lf %.2lf  %.2lf %.3lf\n", finishtime, cpu_util, io_util, avg_TT, avg_CW, throughput); 

    return 0;
}