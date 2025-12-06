#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <iostream>
#include <cstring> 
using namespace std;

// ==========================================
// 1. Helper Structs (Date & Time)
// ==========================================
struct Date {
    int day;
    int month;
    int year;
};

struct Time {
    int hour;
    int minute;
};

// ==========================================
// 2. Route Node (Edge of the Graph)
// ==========================================
struct Route {
    int destinationIndex;      // Destination Port Index
    int cost;                  // Voyage Cost
    int durationMinutes;       // Duration
    char company[50];          // Shipping Company Name
    
    Date voyageDate;           // Date of Voyage
    Time departureTime;        // Departure Time
    Time arrivalTime;          // Arrival Time

    Route* next;               // Linked List Pointer
};

// ==========================================
// 3. Port Node (Vertex of the Graph)
// ==========================================
struct Port {
    char name[50];             // Port Name
    int dailyCharge;           // Port Charges
    
    Route* headRoute;          // Adjacency List Head
    
    // Graphics Coordinates
    float x, y;

    // Dijkstra Algorithm Variables
    int minCost;
    int minTime;
    int parentIndex;
    bool visited;
    
    // Dock Queue Management
    int dockSlots;             // Number of available docking slots (default: 2)
    int queueCount;            // Number of ships waiting in queue
    int inServiceCount;        // Number of ships currently being serviced
    int estWaitMinutes;        // Estimated wait time in minutes for new arrivals
};

// ==========================================
// 4. Custom Linked List Class
// ==========================================
class RouteList {
public:
    Route* head;

    RouteList() {
        head = nullptr;
    }

    void addRoute(int destIndex, int cost, char* comp, Date date, Time dep, Time arr) {
        Route* newNode = new Route;
        newNode->destinationIndex = destIndex;
        newNode->cost = cost;
        strcpy(newNode->company, comp);
        newNode->voyageDate = date;
        newNode->departureTime = dep;
        newNode->arrivalTime = arr;
        newNode->durationMinutes = 0; 

        newNode->next = head;
        head = newNode;
    }
    
    ~RouteList() {
        Route* current = head;
        while (current != nullptr) {
            Route* nextNode = current->next;
            delete current;
            current = nextNode;
        }
    }
};

// ==========================================
// 5. Min-Heap (Priority Queue)
// ==========================================
struct HeapNode {
    int portIndex;  
    int cost;       
};

class MinHeap {
private:
    HeapNode* array;    
    int capacity;       
    int size;           

    void swapNodes(int a, int b) {
        HeapNode temp = array[a];
        array[a] = array[b];
        array[b] = temp;
    }

    void heapifyDown(int index) {
        int smallest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < size && array[left].cost < array[smallest].cost)
            smallest = left;

        if (right < size && array[right].cost < array[smallest].cost)
            smallest = right;

        if (smallest != index) {
            swapNodes(index, smallest);
            heapifyDown(smallest);
        }
    }

    void heapifyUp(int index) {
        while (index > 0 && array[(index - 1) / 2].cost > array[index].cost) {
            swapNodes(index, (index - 1) / 2);
            index = (index - 1) / 2;
        }
    }

public:
    MinHeap(int cap) {
        capacity = cap;
        size = 0;
        array = new HeapNode[cap]; 
    }

    ~MinHeap() {
        delete[] array;
    }

    bool isEmpty() {
        return size == 0;
    }

    void push(int pIndex, int c) {
        if (size == capacity) return;
        array[size].portIndex = pIndex;
        array[size].cost = c;
        heapifyUp(size);
        size++;
    }

    HeapNode extractMin() {
        if (size <= 0) {
            HeapNode emptyNode = {-1, -1};
            return emptyNode;
        }
        if (size == 1) {
            size--;
            return array[0];
        }
        HeapNode root = array[0];
        array[0] = array[size - 1];
        size--;
        heapifyDown(0);
        return root;
    }
};

// ==========================================
// 6. Stack for Path Tracking (New)
// ==========================================
// Used to reverse the path from End -> Start to Start -> End for display
struct StackNode {
    int data;
    StackNode* next;
};

class IntStack {
    StackNode* top;
public:
    IntStack() { top = nullptr; }
    
    void push(int val) {
        StackNode* newNode = new StackNode;
        newNode->data = val;
        newNode->next = top;
        top = newNode;
    }
    
    int pop() {
        if (!top) return -1;
        int val = top->data;
        StackNode* temp = top;
        top = top->next;
        delete temp;
        return val;
    }
    
    bool isEmpty() { return top == nullptr; }
};

// ==========================================
// 7. Ship State Machine (Simulation)
// ==========================================
enum ShipState {
    TRAVELING,       // Ship is traveling along a route leg
    WAITING_QUEUE,   // Ship is waiting in port queue for a dock slot
    DOCKED,          // Ship is docked and being serviced (layover until next departure)
    COMPLETED,       // Ship has reached final destination
    CANCELED         // Ship journey was canceled
};

// Ship structure for real-time simulation
struct Ship {
    int shipId;                  // Unique identifier for this ship
    int originIndex;             // Starting port index
    int destinationIndex;        // Final destination port index
    
    Route* legs[10];             // Array of route legs (matching Journey structure)
    int legCount;                // Number of legs in journey
    int currentLegIndex;         // Index of current/next leg being traveled
    
    long long departureTimeMin;  // Departure time in absolute minutes (sim time)
    long long arrivalTimeMin;    // Expected arrival time in absolute minutes (sim time)
    long long nextDepartureMin;  // Next leg departure time (for waiting at ports)
    
    int currentPortIndex;        // Current port location (when waiting/docked)
    ShipState state;             // Current state in the state machine
    
    Ship* next;                  // Linked list pointer for ship management
};

// ==========================================
// 8. Scheduled Ship State Machine (All Ships at Ports)
// ==========================================
enum ScheduledShipState {
    SHIP_ARRIVING = 0,   // Moving toward port
    SHIP_WAITING = 1,    // Docked/waiting at port
    SHIP_DEPARTING = 2,  // Leaving port
    SHIP_TRAVELING = 3,  // En route between ports
    SHIP_COMPLETED = 4   // Reached final destination
};

// Scheduled Ship structure for all ships in simulation (including user's ship)
struct ScheduledShip {
    int id;                          // Unique ship ID
    char originPort[32];             // Origin port name
    char destPort[32];               // Destination port name
    char company[32];                // Shipping company
    int departureHour, departureMin; // Departure time
    int arrivalHour, arrivalMin;     // Arrival time
    int departureDay, departureMonth, departureYear; // Departure date
    bool isUserShip;                 // true if booked by user
    
    int originIndex;                 // Origin port index
    int destIndex;                   // Destination port index
    ScheduledShipState state;        // Current state
    float progress;                  // 0.0 to 1.0 for animation
    
    float x, y;                      // Current position for rendering
};

// ==========================================
// 9. Simulation Clock
// ==========================================
const int SIM_SPEED_1X = 1;
const int SIM_SPEED_10X = 10;
const int SIM_SPEED_60X = 60;
const int SIM_SPEED_120X = 120;

// Base date for simulation (20/12/2024 00:00)
const int SIM_BASE_DAY = 20;
const int SIM_BASE_MONTH = 12;
const int SIM_BASE_YEAR = 2024;

// ==========================================
// 10. Queue Management Helper Functions
// ==========================================
const int DEFAULT_SERVICE_TIME_MINUTES = 240; // 4 hours average service time per ship

// Recompute estimated wait time for a port
// Formula: For a new arrival, wait time = queueCount * avgServiceTime if docks are full
// If docks have space, wait is 0. This gives a simple approximation of wait time.
inline void recomputeEstWait(Port& port) {
    // If there's queue and all docks are occupied, new arrivals must wait
    if (port.inServiceCount >= port.dockSlots && port.queueCount > 0) {
        // Wait time is queue size times average service time
        port.estWaitMinutes = port.queueCount * DEFAULT_SERVICE_TIME_MINUTES;
    } else if (port.inServiceCount >= port.dockSlots) {
        // Docks full but no queue - minimal wait for next slot
        port.estWaitMinutes = DEFAULT_SERVICE_TIME_MINUTES;
    } else {
        // Docks available - no wait
        port.estWaitMinutes = 0;
    }
}

// Ship arrives at port and joins queue
inline void shipArrival(Port& port) {
    port.queueCount++;
    recomputeEstWait(port);
}

// Start service when a dock slot becomes available
inline void startService(Port& port) {
    if (port.queueCount > 0 && port.inServiceCount < port.dockSlots) {
        port.queueCount--;
        port.inServiceCount++;
        recomputeEstWait(port);
    }
}

// Finish service and free up a dock slot
// Note: Automatically starts servicing the next ship in queue if available,
// simulating real-world port operations where queued ships immediately dock when slots open
inline void finishService(Port& port) {
    if (port.inServiceCount > 0) {
        port.inServiceCount--;
        // Automatically start servicing next ship in queue if available
        if (port.queueCount > 0) {
            port.queueCount--;
            port.inServiceCount++;
        }
        recomputeEstWait(port);
    }
}

#endif