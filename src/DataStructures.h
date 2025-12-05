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
// 7. Queue Management Helper Functions
// ==========================================
const int DEFAULT_SERVICE_TIME_MINUTES = 240; // 4 hours average service time per ship

// Recompute estimated wait time for a port
// Formula: ((queueCount + inServiceCount - dockSlots) * avgServiceMinutes), clamped at >= 0
inline void recomputeEstWait(Port& port) {
    int occupancy = port.queueCount + port.inServiceCount - port.dockSlots;
    if (occupancy < 0) occupancy = 0;
    port.estWaitMinutes = occupancy * DEFAULT_SERVICE_TIME_MINUTES;
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