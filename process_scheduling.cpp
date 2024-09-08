#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <iomanip>

class Process {
public:
    int id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
    int priority;

    Process(int id, int arrival, int burst, int priority = 0)
        : id(id), arrivalTime(arrival), burstTime(burst), remainingTime(burst),
          completionTime(0), turnaroundTime(0), waitingTime(0), responseTime(-1), priority(priority) {}
};

class Scheduler {
protected:
    std::vector<Process> processes;
    double avgWaitingTime;
    double avgTurnaroundTime;
    double avgResponseTime;
    double throughput;

public:
    virtual void addProcess(const Process& p) {
        processes.push_back(p);
    }

    virtual void schedule() = 0;
    
    virtual void printResults() {
        std::cout << "Process\tArrival\tBurst\tResponse\tCompletion\tTurnaround\tWaiting\n";
        for (const auto& p : processes) {
            std::cout << p.id << "\t" << p.arrivalTime << "\t" << p.burstTime << "\t"
                      << p.responseTime << "\t\t" << p.completionTime << "\t\t" 
                      << p.turnaroundTime << "\t\t" << p.waitingTime << "\n";
        }
        std::cout << "Average Waiting Time: " << avgWaitingTime << std::endl;
        std::cout << "Average Turnaround Time: " << avgTurnaroundTime << std::endl;
        std::cout << "Average Response Time: " << avgResponseTime << std::endl;
        std::cout << "Throughput: " << throughput << " processes per unit time" << std::endl;
    }

    void calculateMetrics() {
        int totalWaitingTime = 0;
        int totalTurnaroundTime = 0;
        int totalResponseTime = 0;
        int maxCompletionTime = 0;

        for (const auto& p : processes) {
            totalWaitingTime += p.waitingTime;
            totalTurnaroundTime += p.turnaroundTime;
            totalResponseTime += p.responseTime;
            maxCompletionTime = std::max(maxCompletionTime, p.completionTime);
        }

        avgWaitingTime = static_cast<double>(totalWaitingTime) / processes.size();
        avgTurnaroundTime = static_cast<double>(totalTurnaroundTime) / processes.size();
        avgResponseTime = static_cast<double>(totalResponseTime) / processes.size();
        throughput = static_cast<double>(processes.size()) / maxCompletionTime;
    }
};

class FCFSScheduler : public Scheduler {
public:
    void schedule() override {
        std::sort(processes.begin(), processes.end(), 
                  [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

        int currentTime = 0;
        for (auto& p : processes) {
            if (currentTime < p.arrivalTime) {
                currentTime = p.arrivalTime;
            }
            p.responseTime = currentTime - p.arrivalTime;
            p.completionTime = currentTime + p.burstTime;
            p.turnaroundTime = p.completionTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;
            currentTime = p.completionTime;
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "FCFS Scheduling Results:\n";
        Scheduler::printResults();
    }
};

class SJFScheduler : public Scheduler {
public:
    void schedule() override {
        std::sort(processes.begin(), processes.end(), 
                  [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

        int currentTime = 0;
        size_t completed = 0;
        auto cmp = [](const Process* a, const Process* b) { return a->burstTime > b->burstTime; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> pq(cmp);

        size_t i = 0;
        while (completed < processes.size()) {
            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                pq.push(&processes[i]);
            }

            if (pq.empty()) {
                currentTime = processes[i].arrivalTime;
                continue;
            }

            Process* p = pq.top();
            pq.pop();

            if (p->responseTime == -1) {
                p->responseTime = currentTime - p->arrivalTime;
            }

            p->completionTime = currentTime + p->burstTime;
            p->turnaroundTime = p->completionTime - p->arrivalTime;
            p->waitingTime = p->turnaroundTime - p->burstTime;
            currentTime = p->completionTime;

            completed++;
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "SJF Scheduling Results:\n";
        Scheduler::printResults();
    }
};

class SRTFScheduler : public Scheduler {
public:
    void schedule() override {
        auto cmp = [](const Process* a, const Process* b) { return a->remainingTime > b->remainingTime; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> pq(cmp);

        std::sort(processes.begin(), processes.end(), 
                  [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

        int currentTime = 0;
        size_t completed = 0;
        size_t i = 0;

        while (completed < processes.size()) {
            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                pq.push(&processes[i]);
            }

            if (pq.empty()) {
                currentTime = processes[i].arrivalTime;
                continue;
            }

            Process* p = pq.top();
            pq.pop();

            if (p->responseTime == -1) {
                p->responseTime = currentTime - p->arrivalTime;
            } 

            int executionTime = (i < processes.size()) ? 
                std::min(p->remainingTime, processes[i].arrivalTime - currentTime) : 
                p->remainingTime;

            p->remainingTime -= executionTime;
            currentTime += executionTime;

            if (p->remainingTime == 0) {
                p->completionTime = currentTime;
                p->turnaroundTime = p->completionTime - p->arrivalTime;
                p->waitingTime = p->turnaroundTime - p->burstTime;
                completed++;
            } else {
                pq.push(p);
            }
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "SRTF Scheduling Results:\n";
        Scheduler::printResults();
    }
};

class RoundRobinScheduler : public Scheduler {
private:
    int timeQuantum;

public:
    RoundRobinScheduler(int quantum) : timeQuantum(quantum) {}

    void schedule() override {
        std::queue<Process*> readyQueue;
        int currentTime = 0;
        size_t completed = 0;
        size_t i = 0;

        while (completed < processes.size()) {
            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                readyQueue.push(&processes[i]);
            }

            if (readyQueue.empty()) {
                currentTime = processes[i].arrivalTime;
                continue;
            }

            Process* p = readyQueue.front();
            readyQueue.pop();

            if (p->responseTime == -1) {
                p->responseTime = currentTime - p->arrivalTime;
            }

            int executionTime = std::min(timeQuantum, p->remainingTime);
            p->remainingTime -= executionTime;
            currentTime += executionTime;

            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                readyQueue.push(&processes[i]);
            }

            if (p->remainingTime > 0) {
                readyQueue.push(p);
            } else {
                p->completionTime = currentTime;
                p->turnaroundTime = p->completionTime - p->arrivalTime;
                p->waitingTime = p->turnaroundTime - p->burstTime;
                completed++;
            }
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "Round Robin (Time Quantum: " << timeQuantum << ") Scheduling Results:\n";
        Scheduler::printResults();
    }
};

class PriorityScheduler : public Scheduler {
public:  
    void schedule() override {
        auto cmp = [](const Process* a, const Process* b) { return a->priority < b->priority; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> pq(cmp);

        std::sort(processes.begin(), processes.end(), 
                  [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

        int currentTime = 0;
        size_t completed = 0;
        size_t i = 0;

        while (completed < processes.size()) {
            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                pq.push(&processes[i]);
            }

            if (pq.empty()) {
                currentTime = processes[i].arrivalTime;
                continue;
            }

            Process* p = pq.top();
            pq.pop();

            if (p->responseTime == -1) {
                p->responseTime = currentTime - p->arrivalTime;
            }

            p->completionTime = currentTime + p->burstTime;
            p->turnaroundTime = p->completionTime - p->arrivalTime;
            p->waitingTime = p->turnaroundTime - p->burstTime;
            currentTime = p->completionTime;

            completed++;
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "Priority Scheduling Results:\n";
        Scheduler::printResults();
    }
};


class PreemptivePriorityScheduler : public Scheduler {
public:
    void schedule() override {
        auto cmp = [](const Process* a, const Process* b) { return a->priority > b->priority; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> pq(cmp);

        std::sort(processes.begin(), processes.end(), 
                  [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

        int currentTime = 0;
        size_t completed = 0;
        size_t i = 0;

        while (completed < processes.size()) {
            for (; i < processes.size() && processes[i].arrivalTime <= currentTime; ++i) {
                pq.push(&processes[i]);
            }

            if (pq.empty()) {
                currentTime = processes[i].arrivalTime;
                continue;
            }

            Process* p = pq.top();
            pq.pop();

            if (p->responseTime == -1) {
                p->responseTime = currentTime - p->arrivalTime;
            }

            int executionTime = (i < processes.size()) ? 
                std::min(p->remainingTime, processes[i].arrivalTime - currentTime) : 
                p->remainingTime;

            p->remainingTime -= executionTime;
            currentTime += executionTime;

            if (p->remainingTime == 0) {
                p->completionTime = currentTime;
                p->turnaroundTime = p->completionTime - p->arrivalTime;
                p->waitingTime = p->turnaroundTime - p->burstTime;
                completed++;
            } else {
                pq.push(p);
            }
        }
        calculateMetrics();
    }

    void printResults() override {
        std::cout << "Preemptive Priority Scheduling Results:\n";
        Scheduler::printResults();
    }
};

int main() {
    std::vector<Process> processes = {
        {1, 0, 10, 3},
        {2, 1, 5, 1},
        {3, 3, 8, 2},
        {4, 5, 2, 4},
        {5, 6, 4, 5}
    };

    std::vector<Scheduler*> schedulers = {
        new FCFSScheduler(),
        new SJFScheduler(),
        new SRTFScheduler(),
        new RoundRobinScheduler(2),
        new PriorityScheduler()
    };

    for (auto scheduler : schedulers) {
        for (const auto& p : processes) {
            scheduler->addProcess(p);
        }
        scheduler->schedule();
        scheduler->printResults();
        std::cout << std::string(50, '-') << std::endl;
    }

    // Clean up
    for (auto scheduler : schedulers) {
        delete scheduler;
    }

    return 0;
}

// FCFS: O(n log n)

// Remains the same, as sorting is still needed.


// SJF: O(n log n)

// Uses a priority queue to efficiently select the shortest job.


// SRTF: O(n log n)

// Uses a priority queue to efficiently select the process with the shortest remaining time.


// Round Robin: O(n * total_burst_time)

// Remains similar, as the nature of RR doesn't allow for significant optimization without changing the algorithm.
 

// Priority Scheduling: O(n log n)

// Uses a priority queue to efficiently select the highest priority process.