#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <iostream>
#include <cstring> // strcpy aur strcmp ke liye
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
// Har 'Route' ek connection hai ek port se doosre port tak.
// Ismein 'next' pointer hai kyunke yeh Linked List ka hissa hoga.
struct Route {
    int destinationIndex;      // Jis port pe jaa raha hai uska index
    int cost;                  // Voyage Cost
    int durationMinutes;       // Duration calculation ke liye
    char company[50];          // Shipping Company Name
    
    Date voyageDate;           // Date of Voyage
    Time departureTime;        // Departure Time
    Time arrivalTime;          // Arrival Time

    Route* next;               // Aglay route ka pointer (Linked List)
};

// ==========================================
// 3. Port Node (Vertex of the Graph)
// ==========================================
// Har Port main routes ki list (Head pointer) hogi.
struct Port {
    char name[50];             // Port ka naam (e.g., Karachi)
    int dailyCharge;           // PortCharges.txt se aayega
    
    // Graph Connections (Adjacency List)
    Route* headRoute;          // Is port se nikalne walay tamam raston ki list
    
    // Graphics ke liye coordinates (Baad mein fill karenge)
    float x, y;

    // Dijkstra Algorithm ke liye variables
    int minCost;
    int minTime;
    int parentIndex;
    bool visited;
};

// ==========================================
// 4. Custom Linked List Class
// ==========================================
// Yeh class routes ko manage karegi (add karna, traverse karna).
class RouteList {
public:
    Route* head;

    RouteList() {
        head = nullptr;
    }

    // Naya route add karne ka function
    void addRoute(int destIndex, int cost, char* comp, Date date, Time dep, Time arr) {
        Route* newNode = new Route;
        newNode->destinationIndex = destIndex;
        newNode->cost = cost;
        
        // String copy karna kyunke std::string allowed nahi hai
        strcpy(newNode->company, comp);
        
        newNode->voyageDate = date;
        newNode->departureTime = dep;
        newNode->arrivalTime = arr;
        
        // Duration calculate karna (Optional: abhi 0 rakhain, baad mein logic lagayen)
        newNode->durationMinutes = 0; 

        // List ke shuru mein add karna (O(1) time)
        newNode->next = head;
        head = newNode;
    }
    
    // Memory safayi ke liye
    ~RouteList() {
        Route* current = head;
        while (current != nullptr) {
            Route* nextNode = current->next;
            delete current;
            current = nextNode;
        }
    }
};
// Yeh function Shehar ka naam leta hai aur uska Index wapis karta hai
// ==========================================
// 5. Min-Heap (Priority Queue) Implementation
// ==========================================

// Heap ka aik element (Node)
struct HeapNode {
    int portIndex;  // Shehar ka index (v)
    int cost;       // Wahan tak pohnchne ka kharcha (dist)
};

class MinHeap {
private:
    HeapNode* array;    // Array jo heap ka data store karegi
    int capacity;       // Maximum size
    int size;           // Current elements count

    // Helper: Do nodes ko swap karna
    void swapNodes(int a, int b) {
        HeapNode temp = array[a];
        array[a] = array[b];
        array[b] = temp;
    }

    // Helper: Heapify Down (Extract Min ke baad use hota hai)
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

    // Helper: Heapify Up (Insert ke baad use hota hai)
    void heapifyUp(int index) {
        // Parent ka index: (index - 1) / 2
        while (index > 0 && array[(index - 1) / 2].cost > array[index].cost) {
            swapNodes(index, (index - 1) / 2);
            index = (index - 1) / 2;
        }
    }

public:
    // Constructor
    MinHeap(int cap) {
        capacity = cap;
        size = 0;
        array = new HeapNode[cap]; // Dynamic Array
    }

    // Destructor (Memory safayi)
    ~MinHeap() {
        delete[] array;
    }

    // Check if empty
    bool isEmpty() {
        return size == 0;
    }

    // Naya element daalna (Push)
    void push(int pIndex, int c) {
        if (size == capacity) {
            cout << "Heap Overflow! Mazeed space nahi hai." << endl;
            return;
        }

        // 1. Element ko aakhir mein daal do
        array[size].portIndex = pIndex;
        array[size].cost = c;
        
        // 2. Upar le kar jao (Bubble Up) taake sab se chota upar rahe
        heapifyUp(size);
        size++;
    }

    // Sab se chota element nikalna (Pop)
    HeapNode extractMin() {
        if (size <= 0) {
            HeapNode emptyNode = {-1, -1}; // Error case
            return emptyNode;
        }

        if (size == 1) {
            size--;
            return array[0];
        }

        // 1. Root (Minimum) ko save karo
        HeapNode root = array[0];

        // 2. Aakhri element ko utha kar root pe rakh do
        array[0] = array[size - 1];
        size--;

        // 3. Naye root ko sahi jagah pohanchao (Bubble Down)
        heapifyDown(0);

        return root;
    }
};
#endif