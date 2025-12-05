#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <climits>
#include <SFML/Graphics.hpp> 
#include "DataStructures.h" 

using namespace std;

// Debug flag for route evaluation logging
#define DEBUG_ROUTE_EVALUATION 1

// Cross-platform case-insensitive string comparison
#ifdef _WIN32
    #define strcasecmp _stricmp
#endif

// ==========================================
// 0. Preference System Structures
// ==========================================
struct UserPreferences {
    char preferredCompanies[5][50];
    int preferredCompanyCount;
    
    char avoidedPorts[10][50];
    int avoidedPortCount;
    
    int maxVoyageTimeMinutes;
    bool usePreferences;
    
    UserPreferences() {
        preferredCompanyCount = 0;
        avoidedPortCount = 0;
        maxVoyageTimeMinutes = 10080; // 7 days default
        usePreferences = false;
    }
};

UserPreferences userPrefs;

// ==========================================
// 0. Custom Helper Functions
// ==========================================
void intToString(int val, char* buffer) {
    sprintf(buffer, "%d", val);
}

void appendChar(char* str, char c, int maxSize) {
    int len = strlen(str);
    if (len < maxSize - 1) {
        str[len] = c;
        str[len + 1] = '\0';
    }
}

void popBack(char* str) {
    int len = strlen(str);
    if (len > 0) str[len - 1] = '\0';
}

float getDistanceToLine(sf::Vector2f point, sf::Vector2f start, sf::Vector2f end) {
    float A = point.x - start.x;
    float B = point.y - start.y;
    float C = end.x - start.x;
    float D = end.y - start.y;
    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    float param = -1;
    if (len_sq != 0) param = dot / len_sq;
    float xx, yy;
    if (param < 0) {
        xx = start.x; yy = start.y;
    } else if (param > 1) {
        xx = end.x; yy = end.y;
    } else {
        xx = start.x + param * C;
        yy = start.y + param * D;
    }
    float dx = point.x - xx;
    float dy = point.y - yy;
    return sqrt(dx * dx + dy * dy);
}


// ==========================================
// 1. Global Data & Constants
// ==========================================
Port* ports = nullptr; 
int totalPorts = 0;    
const float SIDEBAR_WIDTH = 350.0f;
const float MAP_OFFSET_X = 350.0f; 

int selectedStart = -1;
int selectedEnd = -1;
int bookingMode = 0;
char inputDateString[20] = "20/12/2024"; 
char statusMessage[100] = "Ready to navigate.";
char pathDetails[100] = ""; 
bool isTypingDate = false;
bool showPreferencesPanel = false;

// Algorithm visualization tracking
bool* exploredPorts = nullptr;
bool* finalPathPorts = nullptr;
float explorationProgress = 0.0f;
bool showingDijkstra = false;

sf::Color COL_BG_DARK(30, 30, 35);
sf::Color COL_ACCENT(0, 180, 255); 
sf::Color COL_BTN_HOVER(0, 200, 255);
sf::Color COL_BTN_IDLE(50, 50, 60);
sf::Color COL_TEXT_WHITE(240, 240, 240);
sf::Color COL_INPUT_BG(255, 255, 255);
sf::Color COL_INPUT_FOCUS(200, 230, 255);

bool isPortAvoided(int portIndex) {
    if (!userPrefs.usePreferences) return false;
    for (int i = 0; i < userPrefs.avoidedPortCount; i++) {
        if (strcasecmp(userPrefs.avoidedPorts[i], ports[portIndex].name) == 0) {
            return true;
        }
    }
    return false;
}

bool isCompanyPreferred(const char* company) {
    if (!userPrefs.usePreferences || userPrefs.preferredCompanyCount == 0) return true;
    for (int i = 0; i < userPrefs.preferredCompanyCount; i++) {
        if (strcasecmp(userPrefs.preferredCompanies[i], company) == 0) {
            return true;
        }
    }
    return false;
}

bool meetsTimeLimit(long long departureTime, long long arrivalTime) {
    if (!userPrefs.usePreferences) return true;
    long long voyageTime = arrivalTime - departureTime;
    if (voyageTime < 0) voyageTime += 1440; // Handle day boundary
    return voyageTime <= userPrefs.maxVoyageTimeMinutes;
}

bool portHasPreferredCompanyRoute(int portIndex) {
    if (!userPrefs.usePreferences || userPrefs.preferredCompanyCount == 0) return false;
    Route* r = ports[portIndex].headRoute;
    while (r != nullptr) {
        if (isCompanyPreferred(r->company)) {
            return true;
        }
        r = r->next;
    }
    return false;
}

struct Journey {
    Route* legs[10];
    int legCount;
    int totalCost;
    int totalTimeMin;
    bool isDirect;
    bool isDijkstra;
    int routeId;
    long long departureTime;
    long long arrivalTime;
};

Journey foundJourneys[50];
int foundJourneysCount = 0;
bool showJourneys = false;
bool* visited = nullptr; 

// ==========================================
// UI Components
// ==========================================
struct Button {
    sf::RectangleShape shape;
    sf::Text label;
    bool isHovered = false;

    void init(float x, float y, float w, float h, const char* text, sf::Font& font) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(w, h));
        shape.setFillColor(COL_BTN_IDLE);
        shape.setOutlineThickness(0);
        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(14);
        label.setFillColor(COL_TEXT_WHITE);
        sf::FloatRect textRect = label.getLocalBounds();
        label.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        label.setPosition(x + w/2.0f, y + h/2.0f);
    }

    void update(sf::Vector2i mousePos) {
        if (shape.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y)) {
            shape.setFillColor(COL_BTN_HOVER);
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(1);
            isHovered = true;
        } else {
            shape.setFillColor(COL_BTN_IDLE);
            shape.setOutlineThickness(0);
            isHovered = false;
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(label);
    }
    
    bool isClicked(sf::Vector2i mousePos) {
        return shape.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y);
    }
};

struct InputBox {
    sf::RectangleShape shape;
    sf::Text displayText;
    bool isFocused = false;

    void init(float x, float y, float w, float h, sf::Font& font) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(w, h));
        shape.setFillColor(COL_INPUT_BG);
        shape.setOutlineColor(COL_ACCENT);
        displayText.setFont(font);
        displayText.setCharacterSize(14);
        displayText.setFillColor(sf::Color::Black);
        displayText.setPosition(x + 5, y + 5);
    }

    void update(const char* content, bool focus) {
        displayText.setString(content);
        isFocused = focus;
        if (isFocused) {
            shape.setOutlineThickness(2);
            shape.setFillColor(COL_INPUT_FOCUS);
        } else {
            shape.setOutlineThickness(0);
            shape.setFillColor(COL_INPUT_BG);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(displayText);
    }
    
    bool isClicked(sf::Vector2i mousePos) {
        return shape.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y);
    }
};

// ==========================================
// Data Loading
// ==========================================
int getPortIndex(const char* name) {
    for(int i=0; i<totalPorts; i++) if(strcmp(ports[i].name, name)==0) return i;
    return -1;
}

int countFileLines(const char* filename) {
    ifstream f(filename);
    if(!f.is_open()) return 0;
    int count = 0;
    char buffer[100];
    while(f.getline(buffer, 100)) if(strlen(buffer) > 0) count++;
    return count;
}

void setPos(const char* name, float x, float y) {
    int idx = getPortIndex(name);
    if(idx != -1) { 
        ports[idx].x = x + MAP_OFFSET_X; 
        ports[idx].y = y; 
    }
}

void initCoordinates() {
    setPos("Karachi", 1015, 460); setPos("Dubai", 960, 450);
    setPos("AbuDhabi", 950, 455); setPos("Jeddah", 900, 470);
    setPos("Doha", 970, 450); setPos("Mumbai", 1040, 500);
    setPos("Colombo", 1060, 560); setPos("Chittagong", 1120, 470);
    setPos("HongKong", 1220, 460); setPos("Shanghai", 1250, 420);
    setPos("Tokyo", 1330, 390); setPos("Osaka", 1310, 400);
    setPos("Busan", 1280, 400); setPos("Manila", 1250, 520);
    setPos("Singapore", 1180, 590); setPos("Jakarta", 1190, 640);
    setPos("London", 735, 290); setPos("Hamburg", 770, 280);
    setPos("Rotterdam", 760, 290); setPos("Antwerp", 765, 295);
    setPos("Marseille", 765, 335); setPos("Genoa", 780, 330);
    setPos("Lisbon", 690, 350); setPos("Oslo", 780, 240);
    setPos("Stockholm", 810, 240); setPos("Helsinki", 840, 230);
    setPos("Copenhagen", 785, 265); setPos("Athens", 830, 360);
    setPos("Istanbul", 850, 350); setPos("Dublin", 710, 285);
    setPos("NewYork", 430, 360); setPos("Montreal", 420, 330);
    setPos("Vancouver", 250, 300); setPos("LosAngeles", 270, 390);
    setPos("Alexandria", 860, 380); setPos("CapeTown", 810, 810);
    setPos("Durban", 860, 780); setPos("PortLouis", 970, 750); 
    setPos("Sydney", 1420, 800); setPos("Melbourne", 1390, 820);
    setPos("Bangkok", 1150, 620);
    setPos("Chennai", 1090, 580);
    setPos("Suez", 880, 380);
}

long long getMinutes(Date d, Time t) {
    return ((long long)d.year*525600) + ((long long)d.month*43200) + ((long long)d.day*1440) + (t.hour*60) + t.minute;
}

Date parseDate(char* s) {
    Date d = {1, 1, 2024};
    int i = 0, val = 0;
    while(s[i] != '/' && s[i] != '\0') { val = val * 10 + (s[i] - '0'); i++; }
    if (val > 0) d.day = val; if (s[i] == '\0') return d; i++;
    val = 0;
    while(s[i] != '/' && s[i] != '\0') { val = val * 10 + (s[i] - '0'); i++; }
    if (val > 0) d.month = val; if (s[i] == '\0') return d; i++;
    val = 0;
    while(s[i] != '\0') { val = val * 10 + (s[i] - '0'); i++; }
    if (val > 0) d.year = val;
    return d;
}

void loadData() {
    totalPorts = countFileLines("ports.txt");
    if(totalPorts == 0) { cout << "Error: ports.txt empty!" << endl; return; }
    ports = new Port[totalPorts];
    visited = new bool[totalPorts];
    exploredPorts = new bool[totalPorts];
    finalPathPorts = new bool[totalPorts];

    ifstream fp("ports.txt");
    if(fp.is_open()) {
        char n[50]; int c;
        int i = 0;
        while(fp >> n >> c && i < totalPorts) {
            strcpy(ports[i].name, n);
            ports[i].dailyCharge = c;
            ports[i].headRoute = nullptr;
            i++;
        }
    }
    
    ifstream fr("Routes.txt");
    if(fr.is_open()) {
        char o[50], d[50], co[50];
        char dateStr[20], depStr[10], arrStr[10];
        int c;
        
        while(fr >> o >> d >> dateStr >> depStr >> arrStr >> c >> co) {
            Date dt;
            sscanf(dateStr, "%d/%d/%d", &dt.day, &dt.month, &dt.year);
            
            Time dep, arr;
            sscanf(depStr, "%d/%d", &dep.hour, &dep.minute);
            sscanf(arrStr, "%d/%d", &arr.hour, &arr.minute);
            
            int u = getPortIndex(o), v = getPortIndex(d);
            if(u!=-1 && v!=-1) {
                Route* r = new Route;
                r->destinationIndex = v; r->cost = c; strcpy(r->company, co);
                r->voyageDate = dt; r->departureTime = dep; r->arrivalTime = arr;
                r->next = ports[u].headRoute; ports[u].headRoute = r;
            }
        }
    }
}

// ==========================================
// Forward Declarations
// ==========================================
void findScheduledRoutes(int u, int target, int depth, long long currentArrivalTime, int currentCost, Route* pathSoFar[]);

// ==========================================
// Dijkstra's Algorithm with Preferences
// ==========================================
struct HeapNodeTime {
    int portIndex;
    long long cost;
};

class MinHeapTime {
private:
    HeapNodeTime* array;
    int capacity;
    int size;

    void swapNodes(int a, int b) {
        HeapNodeTime temp = array[a];
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
    MinHeapTime(int cap) : capacity(cap), size(0), array(new HeapNodeTime[cap]) {}
    ~MinHeapTime() { delete[] array; }
    bool isEmpty() { return size == 0; }

    void push(int pIndex, long long c) {
        if (size == capacity) return;
        array[size].portIndex = pIndex;
        array[size].cost = c;
        heapifyUp(size);
        size++;
    }

    HeapNodeTime extractMin() {
        if (size <= 0) return {-1, -1};
        if (size == 1) { size--; return array[0]; }
        HeapNodeTime root = array[0];
        array[0] = array[size - 1];
        size--;
        heapifyDown(0);
        return root;
    }
};

void initDijkstraData() {
    for (int i = 0; i < totalPorts; i++) {
        ports[i].minCost = 2147483647;
        ports[i].minTime = 2147483647;
        ports[i].parentIndex = -1;
        ports[i].visited = false;
    }
    
    for (int i = 0; i < totalPorts; i++) {
        exploredPorts[i] = false;
        finalPathPorts[i] = false;
    }
}

void dijkstra_shortest_cost_scheduled(int source, int destination, Date userDate) {
    initDijkstraData();
    ports[source].minCost = 0;
    exploredPorts[source] = true;
    MinHeapTime pq(totalPorts);
    pq.push(source, 0);
    
    Time startTime = {0, 0};
    long long userStartTime = getMinutes(userDate, startTime);
    
    // CRITICAL FIX: Separate array to track arrival times at each port
    long long* arrivalTimes = new long long[totalPorts];
    for (int i = 0; i < totalPorts; i++) {
        arrivalTimes[i] = (i == source) ? userStartTime : LLONG_MAX;
    }
    
    while (!pq.isEmpty()) {
        HeapNodeTime current = pq.extractMin();
        int u = current.portIndex;
        
        if (ports[u].visited) continue;
        ports[u].visited = true;
        if (u == destination) break;
        
        // Skip if port is avoided
        if (isPortAvoided(u) && u != source && u != destination) continue;
        
        Route* r = ports[u].headRoute;
        while (r != nullptr) {
            int v = r->destinationIndex;
            
            #if DEBUG_ROUTE_EVALUATION
            printf("DEBUG: Evaluating route %s -> %s, Company: %s\n", 
                   ports[u].name, ports[v].name, r->company);
            
            bool companyOk = isCompanyPreferred(r->company);
            printf("DEBUG: Company preferred: %s\n", companyOk ? "YES" : "NO");
            #else
            bool companyOk = isCompanyPreferred(r->company);
            #endif
            
            // Apply preference filters
            if (!isPortAvoided(v) && companyOk) {
                long long departureTime = getMinutes(r->voyageDate, r->departureTime);
                
                // FIX: Use arrivalTimes[u] + 120 minute layover instead of ports[u].minCost
                long long minDepartureTime = (u == source) ? userStartTime : arrivalTimes[u] + 120;
                
                long long routeArrivalTime = getMinutes(r->voyageDate, r->arrivalTime);
                if (r->arrivalTime.hour < r->departureTime.hour) 
                    routeArrivalTime += 1440;
                
                if (departureTime >= minDepartureTime && meetsTimeLimit(departureTime, routeArrivalTime)) {
                    int edgeCost = r->cost + ports[v].dailyCharge;
                    long long newCost = ports[u].minCost + edgeCost;
                    
                    if (!ports[v].visited && newCost < ports[v].minCost) {
                        ports[v].minCost = newCost;
                        ports[v].parentIndex = u;
                        arrivalTimes[v] = routeArrivalTime; // FIX: Store actual arrival time
                        exploredPorts[v] = true;
                        pq.push(v, newCost);
                        #if DEBUG_ROUTE_EVALUATION
                        printf("DEBUG: Route accepted! Cost: %lld\n", newCost);
                        #endif
                    }
                } else {
                    #if DEBUG_ROUTE_EVALUATION
                    printf("DEBUG: Route rejected (timing constraints)\n");
                    #endif
                }
            } else {
                #if DEBUG_ROUTE_EVALUATION
                printf("DEBUG: Route rejected (port avoided or company not preferred)\n");
                #endif
            }
            r = r->next;
        }
    }
    
    delete[] arrivalTimes; // Clean up
}

void reconstructDijkstraPath(int source, int destination, Journey& result) {
    result.legCount = 0;
    result.totalCost = 0;
    result.isDirect = false;
    result.isDijkstra = true;
    result.departureTime = 0;
    result.arrivalTime = 0;
    
    IntStack pathStack;
    int current = destination;
    int pathLength = 0;
    
    while (current != -1 && pathLength < 10) {
        pathStack.push(current);
        finalPathPorts[current] = true;
        current = ports[current].parentIndex;
        pathLength++;
    }
    
    int prevPort = pathStack.pop();
    
    while (!pathStack.isEmpty()) {
        int nextPort = pathStack.pop();
        Route* foundRoute = nullptr;
        Route* r = ports[prevPort].headRoute;
        while (r != nullptr) {
            if (r->destinationIndex == nextPort && isCompanyPreferred(r->company)) {
                foundRoute = r;
                break;
            }
            r = r->next;
        }
        
        if (foundRoute != nullptr && result.legCount < 10) {
            result.legs[result.legCount] = foundRoute;
            result.totalCost += foundRoute->cost;
            
            if (result.legCount == 0) {
                result.departureTime = getMinutes(foundRoute->voyageDate, foundRoute->departureTime);
            }
            
            long long arrTime = getMinutes(foundRoute->voyageDate, foundRoute->arrivalTime);
            if (foundRoute->arrivalTime.hour < foundRoute->departureTime.hour) 
                arrTime += 1440;
            result.arrivalTime = arrTime;
            result.legCount++;
        }
        prevPort = nextPort;
    }
    
    result.isDirect = (result.legCount == 1);
}

void runDijkstraSearch(int start, int end, Date userDate) {
    if (start == -1 || end == -1) { 
        strcpy(statusMessage, "Select Start & End!"); 
        return; 
    }
    
    foundJourneysCount = 0;
    strcpy(statusMessage, "Computing optimal route...");
    showJourneys = false;
    showingDijkstra = true;
    explorationProgress = 0.0f;
    
    dijkstra_shortest_cost_scheduled(start, end, userDate);
    
    if (ports[end].minCost == 2147483647) {
        strcpy(statusMessage, "No route meets preferences.");
        strcpy(pathDetails, "Adjust filters & retry.");
        showingDijkstra = false;
        return;
    }
    
    Journey optimalRoute = {};
    optimalRoute.routeId = 0;
    reconstructDijkstraPath(start, end, optimalRoute);
    
    if (optimalRoute.legCount > 0) {
        foundJourneys[foundJourneysCount] = optimalRoute;
        foundJourneysCount++;
        
        strcpy(statusMessage, "Optimal route found!");
        char buff[50];
        strcpy(pathDetails, "Cost: $");
        intToString(ports[end].minCost, buff);
        strcat(pathDetails, buff);
        strcat(pathDetails, " | Legs: ");
        intToString(optimalRoute.legCount, buff);
        strcat(pathDetails, buff);
        
        if (userPrefs.usePreferences) {
            strcat(pathDetails, " [Filtered]");
        }
        
        bookingMode = 2;
        showJourneys = true;
        explorationProgress = 1.0f;
    }
}

// ==========================================
// DFS Search with Preferences
// ==========================================
void findScheduledRoutes(int u, int target, int depth, long long currentArrivalTime, int currentCost, Route* pathSoFar[]) {
    if (depth >= 10 || foundJourneysCount >= 50) return;
    
    if (isPortAvoided(u) && u != target && depth > 0) return;

    if (u == target) {
        foundJourneys[foundJourneysCount].legCount = depth;
        foundJourneys[foundJourneysCount].totalCost = currentCost;
        foundJourneys[foundJourneysCount].isDirect = (depth == 1);
        foundJourneys[foundJourneysCount].isDijkstra = false;
        foundJourneys[foundJourneysCount].routeId = foundJourneysCount;
        for(int k=0; k<depth; k++) {
            foundJourneys[foundJourneysCount].legs[k] = pathSoFar[k];
        }
        foundJourneysCount++;
        return;
    }

    visited[u] = true;
    Route* r = ports[u].headRoute;
    
    while (r != nullptr) {
        int v = r->destinationIndex;
        #if DEBUG_ROUTE_EVALUATION
        printf("DEBUG DFS: Checking %s -> %s, Company: %s\n", 
               ports[u].name, ports[v].name, r->company);
        #endif
        
        if (!visited[v] && !isPortAvoided(v) && isCompanyPreferred(r->company)) {
            #if DEBUG_ROUTE_EVALUATION
            printf("DEBUG DFS: Route matches preferences\n");
            #endif
            long long departureTime = getMinutes(r->voyageDate, r->departureTime);
            long long requiredDepartureTime = currentArrivalTime + 120;
            long long arrivalTime = getMinutes(r->voyageDate, r->arrivalTime);
            
            if (arrivalTime < departureTime) arrivalTime += 1440;
            
            if (departureTime >= requiredDepartureTime && meetsTimeLimit(departureTime, arrivalTime)) {
                pathSoFar[depth] = r;
                findScheduledRoutes(v, target, depth + 1, arrivalTime, currentCost + r->cost, pathSoFar);
            }
        } else {
            #if DEBUG_ROUTE_EVALUATION
            printf("DEBUG DFS: Route rejected - visited:%d, avoided:%d, companyOk:%d\n",
                   visited[v], isPortAvoided(v), isCompanyPreferred(r->company));
            #endif
        }
        r = r->next;
    }
    
    visited[u] = false; 
}

void runSearch(int start, int end, Date userDate) {
    if (start == -1 || end == -1) { strcpy(statusMessage, "Select Start & End!"); return; }
    
    foundJourneysCount = 0;
    for(int i=0; i<totalPorts; i++) visited[i] = false;
    strcpy(statusMessage, "Searching...");
    showJourneys = false;
    showingDijkstra = false;

    Route* tempPath[10];
    Time startTime = {0, 0};
    long long userStartTime = getMinutes(userDate, startTime);
    
    findScheduledRoutes(start, end, 0, userStartTime, 0, tempPath);

    if (foundJourneysCount > 0) {
        Route* firstLeg = foundJourneys[0].legs[0];
        bool exactDateMatch = (firstLeg->voyageDate.day == userDate.day && 
                               firstLeg->voyageDate.month == userDate.month &&
                               firstLeg->voyageDate.year == userDate.year);
        char dateBuff[20];
        sprintf(dateBuff, "%d/%d/%d", firstLeg->voyageDate.day, firstLeg->voyageDate.month, firstLeg->voyageDate.year);
        if (exactDateMatch) {
            strcpy(statusMessage, "Route Found!");
            strcpy(pathDetails, "Departing: "); strcat(pathDetails, dateBuff);
        } else {
            strcpy(statusMessage, "No ship on date.");
            strcpy(pathDetails, "Next: "); strcat(pathDetails, dateBuff);
        }
        bookingMode = 1;
        showJourneys = true;
    } else {
        strcpy(statusMessage, "No routes found.");
        strcpy(pathDetails, "");
    }
}

void findAllAvailableRoutes(int start, int end, Date userDate) {
    if (start == -1 || end == -1) { 
        strcpy(statusMessage, "Select Start & End!"); 
        return; 
    }
    
    foundJourneysCount = 0;
    for(int i=0; i<totalPorts; i++) visited[i] = false;
    strcpy(statusMessage, "Finding routes...");
    showJourneys = false;
    showingDijkstra = false;

    Route* tempPath[10];
    Time startTime = {0, 0};
    long long userStartTime = getMinutes(userDate, startTime);
    
    findScheduledRoutes(start, end, 0, userStartTime, 0, tempPath);

    if (foundJourneysCount > 0) {
        for (int i = 0; i < foundJourneysCount; i++) {
            if (foundJourneys[i].legCount > 0) {
                Route* firstRoute = foundJourneys[i].legs[0];
                foundJourneys[i].departureTime = getMinutes(firstRoute->voyageDate, firstRoute->departureTime);
                
                Route* lastRoute = foundJourneys[i].legs[foundJourneys[i].legCount - 1];
                foundJourneys[i].arrivalTime = getMinutes(lastRoute->voyageDate, lastRoute->arrivalTime);
                if (lastRoute->arrivalTime.hour < lastRoute->departureTime.hour)
                    foundJourneys[i].arrivalTime += 1440;
                
                foundJourneys[i].routeId = i;
                foundJourneys[i].isDijkstra = false;
            }
        }
        
        char buff[50];
        strcpy(statusMessage, "Routes found!");
        strcpy(pathDetails, "Total: ");
        intToString(foundJourneysCount, buff);
        strcat(pathDetails, buff);
        
        bookingMode = 3;
        showJourneys = true;
    } else {
        strcpy(statusMessage, "No routes.");
        strcpy(pathDetails, "");
    }
}

// ==========================================
// Graphics & Animation
// ==========================================
void runGraphics() {
    sf::RenderWindow window(sf::VideoMode(1536 + (int)SIDEBAR_WIDTH, 1024), "OceanRoute Nav", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Texture tMap, tPin;
    if(!tMap.loadFromFile("map.png")) cout << "Map Error" << endl;
    bool hasPin = tPin.loadFromFile("pin.png");
    if(hasPin) tPin.setSmooth(true);
    
    sf::Sprite sMap(tMap);
    sMap.setScale(1536.0f/tMap.getSize().x, 1024.0f/tMap.getSize().y);
    sMap.setPosition(MAP_OFFSET_X, 0);

    sf::Font font;
    if(!font.loadFromFile("arial.ttf")) cout << "Font Error" << endl;

    sf::RectangleShape sidebar(sf::Vector2f(SIDEBAR_WIDTH, 1024.0f));
    sidebar.setFillColor(COL_BG_DARK); 
    
    InputBox dateInput; dateInput.init(20, 200, 300, 35, font);
    InputBox companyInput; companyInput.init(20, 520, 300, 28, font);
    InputBox avoidPortInput; avoidPortInput.init(20, 570, 300, 28, font);
    
    Button btnSearch, btnDijkstra, btnBook, btnClear;
    btnSearch.init(20, 250, 300, 40, "Find Routes (Date)", font);
    btnDijkstra.init(20, 295, 300, 40, "Find Cheapest Route", font);
    btnBook.init(20, 340, 300, 40, "Book Route (All)", font);
    btnClear.init(20, 385, 300, 40, "Reset", font);
    
    Button btnPreferences, btnApplyPrefs;
    btnPreferences.init(20, 430, 300, 30, "Preferences", font);
    btnApplyPrefs.init(20, 620, 300, 30, "Apply Filters", font);

    sf::Text txtStart("From: None", font, 16); txtStart.setPosition(20, 80);
    sf::Text txtEnd("To:   None", font, 16); txtEnd.setPosition(20, 120);
    sf::Text txtStatus(statusMessage, font, 14); txtStatus.setPosition(20, 440); txtStatus.setFillColor(sf::Color::Yellow);
    sf::Text txtDetails(pathDetails, font, 12); txtDetails.setPosition(20, 470); txtDetails.setFillColor(sf::Color::Cyan);
    sf::Text txtPrefTitle("Filter Preferences", font, 12); txtPrefTitle.setPosition(20, 495); txtPrefTitle.setFillColor(sf::Color::Cyan);
    sf::Text txtCompanyLabel("Company (comma-separated):", font, 10); txtCompanyLabel.setPosition(20, 505); txtCompanyLabel.setFillColor(sf::Color::White);
    sf::Text txtAvoidLabel("Avoid Ports (comma-separated):", font, 10); txtAvoidLabel.setPosition(20, 555); txtAvoidLabel.setFillColor(sf::Color::White);
    
    sf::RectangleShape tooltipBox(sf::Vector2f(320, 140));
    tooltipBox.setFillColor(sf::Color(0, 0, 0, 220));
    tooltipBox.setOutlineThickness(1); tooltipBox.setOutlineColor(sf::Color::White);
    sf::Text tooltipText("", font, 12); tooltipText.setFillColor(sf::Color::White);

    sf::Clock animClock;
    
    char tempCompany[50] = "";
    char tempAvoidPort[50] = "";
    bool focusCompany = false;
    bool focusAvoidPort = false;

    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) window.close();

            if(event.type == sf::Event::TextEntered && isTypingDate) {
                if(event.text.unicode == 8) popBack(inputDateString);
                else if(event.text.unicode < 128) appendChar(inputDateString, (char)event.text.unicode, 20);
            }
            
            if(event.type == sf::Event::TextEntered && focusCompany) {
                if(event.text.unicode == 8) popBack(tempCompany);
                else if(event.text.unicode < 128) appendChar(tempCompany, (char)event.text.unicode, 50);
            }
            
            if(event.type == sf::Event::TextEntered && focusAvoidPort) {
                if(event.text.unicode == 8) popBack(tempAvoidPort);
                else if(event.text.unicode < 128) appendChar(tempAvoidPort, (char)event.text.unicode, 50);
            }

            if(event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i pos = sf::Mouse::getPosition(window);
                
                if(dateInput.isClicked(pos)) isTypingDate = true; else isTypingDate = false;
                if(companyInput.isClicked(pos)) focusCompany = true; else focusCompany = false;
                if(avoidPortInput.isClicked(pos)) focusAvoidPort = true; else focusAvoidPort = false;

                if(pos.x > SIDEBAR_WIDTH) { 
                    for(int i=0; i<totalPorts; i++) {
                        if(abs(pos.x - ports[i].x) < 20 && abs(pos.y - ports[i].y) < 20) {
                            if(event.mouseButton.button == sf::Mouse::Left) {
                                selectedStart = i; 
                                char buff[60] = "From: "; strcat(buff, ports[i].name); txtStart.setString(buff);
                            } else if(event.mouseButton.button == sf::Mouse::Right) {
                                selectedEnd = i;
                                char buff[60] = "To:   "; strcat(buff, ports[i].name); txtEnd.setString(buff);
                            }
                        }
                    }
                }

                if(btnSearch.isClicked(pos)) runSearch(selectedStart, selectedEnd, parseDate(inputDateString));
                if(btnDijkstra.isClicked(pos)) runDijkstraSearch(selectedStart, selectedEnd, parseDate(inputDateString));
                if(btnBook.isClicked(pos)) findAllAvailableRoutes(selectedStart, selectedEnd, parseDate(inputDateString));
                if(btnClear.isClicked(pos)) {
                    selectedStart = -1; selectedEnd = -1; showJourneys = false;
                    bookingMode = 0;
                    showingDijkstra = false;
                    
                    userPrefs.usePreferences = false;
                    userPrefs.preferredCompanyCount = 0;
                    userPrefs.avoidedPortCount = 0;
                    tempCompany[0] = '\0';
                    tempAvoidPort[0] = '\0';
                    
                    strcpy(statusMessage, "Ready."); strcpy(pathDetails, ""); strcpy(inputDateString, "20/12/2024");
                    txtStart.setString("From: None"); txtEnd.setString("To:   None");
                }
                
                if(btnPreferences.isClicked(pos)) {
                    showPreferencesPanel = !showPreferencesPanel;
                }
                
                if(btnApplyPrefs.isClicked(pos)) {
                    userPrefs.preferredCompanyCount = 0;
                    userPrefs.avoidedPortCount = 0;
                    
                    if (strlen(tempCompany) > 0 || strlen(tempAvoidPort) > 0) {
                        userPrefs.usePreferences = true;
                        
                        // Parse comma-separated companies
                        if (strlen(tempCompany) > 0) {
                            char tempCopy[200];
                            strcpy(tempCopy, tempCompany);
                            char* token = strtok(tempCopy, ",");
                            while (token != nullptr && userPrefs.preferredCompanyCount < 5) {
                                // Trim leading/trailing spaces
                                while (*token == ' ') token++;
                                char* end = token + strlen(token) - 1;
                                while (end > token && *end == ' ') { *end = '\0'; end--; }
                                
                                if (strlen(token) > 0) {
                                    strcpy(userPrefs.preferredCompanies[userPrefs.preferredCompanyCount], token);
                                    userPrefs.preferredCompanyCount++;
                                }
                                token = strtok(nullptr, ",");
                            }
                        }
                        
                        // Parse comma-separated avoided ports
                        if (strlen(tempAvoidPort) > 0) {
                            char tempCopy[200];
                            strcpy(tempCopy, tempAvoidPort);
                            char* token = strtok(tempCopy, ",");
                            while (token != nullptr && userPrefs.avoidedPortCount < 10) {
                                // Trim leading/trailing spaces
                                while (*token == ' ') token++;
                                char* end = token + strlen(token) - 1;
                                while (end > token && *end == ' ') { *end = '\0'; end--; }
                                
                                if (strlen(token) > 0) {
                                    strcpy(userPrefs.avoidedPorts[userPrefs.avoidedPortCount], token);
                                    userPrefs.avoidedPortCount++;
                                }
                                token = strtok(nullptr, ",");
                            }
                        }
                        
                        // Debug output
                        printf("DEBUG: Preferences Applied\n");
                        printf("DEBUG: usePreferences = %s\n", userPrefs.usePreferences ? "true" : "false");
                        printf("DEBUG: avoidedPortCount = %d\n", userPrefs.avoidedPortCount);
                        for (int i = 0; i < userPrefs.avoidedPortCount; i++) {
                            printf("DEBUG: avoidedPorts[%d] = '%s'\n", i, userPrefs.avoidedPorts[i]);
                        }
                        printf("DEBUG: preferredCompanyCount = %d\n", userPrefs.preferredCompanyCount);
                        for (int i = 0; i < userPrefs.preferredCompanyCount; i++) {
                            printf("DEBUG: preferredCompanies[%d] = '%s'\n", i, userPrefs.preferredCompanies[i]);
                        }
                        
                        strcpy(statusMessage, "Filters applied!");
                    } else {
                        userPrefs.usePreferences = false;
                        strcpy(statusMessage, "Filters cleared!");
                    }
                    
                    showPreferencesPanel = false;
                }
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        btnSearch.update(mPos); btnDijkstra.update(mPos); btnBook.update(mPos); btnClear.update(mPos);
        btnPreferences.update(mPos); btnApplyPrefs.update(mPos);
        dateInput.update(inputDateString, isTypingDate);
        companyInput.update(tempCompany, focusCompany);
        avoidPortInput.update(tempAvoidPort, focusAvoidPort);
        txtStatus.setString(statusMessage); txtDetails.setString(pathDetails);
        
        if (showingDijkstra && explorationProgress < 1.0f) {
            explorationProgress += 0.02f;
            if (explorationProgress > 1.0f) explorationProgress = 1.0f;
        }

        window.clear(sf::Color(30, 30, 30));
        window.draw(sMap);

        bool hoverFound = false;
        float time = animClock.getElapsedTime().asSeconds();

        bool* portInRoute = new bool[totalPorts];
        for(int i=0; i<totalPorts; i++) portInRoute[i] = false;

        if (showJourneys) {
            for(int i=0; i<foundJourneysCount; i++) {
                Journey& j = foundJourneys[i];
                if (selectedStart != -1) portInRoute[selectedStart] = true;
                if (selectedEnd != -1) portInRoute[selectedEnd] = true;
                
                for(int k=0; k<j.legCount; k++) {
                    Route* r = j.legs[k];
                    int u = (k == 0) ? selectedStart : j.legs[k-1]->destinationIndex;
                    int v = r->destinationIndex;
                    if (u != -1) portInRoute[u] = true;
                    portInRoute[v] = true;
                }
            }

            for(int i=0; i<foundJourneysCount; i++) {
                Journey& j = foundJourneys[i];
                
                float totalCycleDuration = 2.0f + (j.legCount * 0.2f);
                float cycleProgress = fmod(time, totalCycleDuration) / totalCycleDuration;
                float legProgressInCycle = cycleProgress * j.legCount;
                int activeLeg = (int)legProgressInCycle;
                if (activeLeg >= j.legCount) activeLeg = j.legCount - 1;
                float legProgress = legProgressInCycle - (float)activeLeg;
                if (legProgress > 1.0f) legProgress = 1.0f;
                
                float offset = (i - foundJourneysCount/2.0f) * 5.0f;

                sf::Color pathColor;
                if (j.isDijkstra) {
                    pathColor = sf::Color(0, 150, 255, 220);
                } else if (bookingMode == 3) {
                    pathColor = sf::Color(255, 0, 200, 200);
                } else {
                    pathColor = j.isDirect ? sf::Color(255, 215, 0, 220) : sf::Color(0, 255, 0, 180);
                }
                
                // Add pulsing animation for filtered routes
                if (userPrefs.usePreferences && showJourneys) {
                    float pulseAlpha = (sin(time * 4.0f) + 1.0f) / 2.0f; // 0 to 1
                    int alpha = 150 + (int)(pulseAlpha * 105); // 150 to 255
                    pathColor.a = alpha;
                }

                for(int k=0; k<j.legCount; k++) {
                    Route* r = j.legs[k];
                    int u = (k == 0) ? selectedStart : -1;
                    if (k > 0) u = j.legs[k-1]->destinationIndex;
                    int v = r->destinationIndex;

                    if (u != -1) {
                        sf::Vector2f p1(ports[u].x + offset, ports[u].y + offset);
                        sf::Vector2f p2(ports[v].x + offset, ports[v].y + offset);

                        sf::Color legColor = pathColor;
                        if (k == activeLeg) {
                            legColor.a = 255;
                        } else if (k < activeLeg) {
                            legColor.a = 150;
                        } else {
                            legColor.a = 50;
                        }
                        
                        // Draw glow effect for filtered routes
                        if (userPrefs.usePreferences) {
                            sf::Color glowColor = sf::Color(255, 200, 0, 100); // Golden glow for filtered routes
                            
                            // Draw glow (thicker line effect using multiple offset lines)
                            for (int offset_val = -2; offset_val <= 2; offset_val++) {
                                sf::Vertex glowLine[] = {
                                    sf::Vertex(sf::Vector2f(p1.x + offset_val, p1.y), glowColor),
                                    sf::Vertex(sf::Vector2f(p2.x + offset_val, p2.y), glowColor)
                                };
                                window.draw(glowLine, 2, sf::Lines);
                            }
                        }

                        sf::Vertex line[] = {
                            sf::Vertex(p1, legColor),
                            sf::Vertex(p2, legColor)
                        };
                        window.draw(line, 2, sf::Lines);

                        if (k == activeLeg && legProgress >= 0.0f && legProgress <= 1.0f) {
                            sf::Vector2f particlePos = p1 + (p2 - p1) * legProgress;
                            sf::CircleShape particle(5);
                            particle.setFillColor(sf::Color::White);
                            particle.setOutlineThickness(1.0f);
                            particle.setOutlineColor(sf::Color::Yellow);
                            particle.setOrigin(5, 5);
                            particle.setPosition(particlePos);
                            window.draw(particle);
                        }

                        float dist = getDistanceToLine(sf::Vector2f(mPos.x, mPos.y), p1, p2);
                        if (dist < 8.0f && !hoverFound) {
                            hoverFound = true;
                            char info[400] = "";
                            strcat(info, "Leg "); intToString(k+1, (char*)&info[strlen(info)]); strcat(info, "/"); 
                            intToString(j.legCount, (char*)&info[strlen(info)]);
                            strcat(info, " | Co: "); strcat(info, r->company);
                            
                            tooltipText.setString(info);
                            tooltipBox.setPosition(mPos.x + 15, mPos.y + 15);
                            tooltipText.setPosition(mPos.x + 20, mPos.y + 20);
                            
                            window.draw(tooltipBox);
                            window.draw(tooltipText);
                        }
                    }
                }
            }
        }

        for(int i=0; i<totalPorts; i++) {
            bool isHovering = false;
            
            if (abs(mPos.x - ports[i].x) < 20 && abs(mPos.y - ports[i].y) < 20) {
                isHovering = true;
            }
            
            if(hasPin) {
                sf::Sprite p(tPin); 
                p.setPosition(ports[i].x, ports[i].y);
                float s = 40.0f/tPin.getSize().y;
                
                if (i==selectedStart) { p.setColor(sf::Color::Green); s*=1.3f; }
                else if (i==selectedEnd) { p.setColor(sf::Color::Red); s*=1.3f; }
                else if (isPortAvoided(i)) { p.setColor(sf::Color(200, 0, 0)); s*=1.1f; }
                else if (portHasPreferredCompanyRoute(i) && userPrefs.usePreferences) {
                    p.setColor(sf::Color(255, 215, 0)); // Gold color for ports with preferred company
                    s*=1.15f;
                }
                else if (finalPathPorts[i] && showingDijkstra) { 
                    p.setColor(sf::Color(0, 255, 100));
                    s*=1.25f; 
                }
                else if (exploredPorts[i] && showingDijkstra) { 
                    p.setColor(sf::Color(100, 150, 255));
                    float pulse = sin(time * 3.0f) * 0.1f + 0.9f;
                    s *= pulse;
                }
                else if (isHovering) { p.setColor(sf::Color::Yellow); s*=1.2f; }
                else p.setColor(sf::Color::White);
                
                p.setScale(s, s);
                p.setOrigin(tPin.getSize().x/2.0f, tPin.getSize().y);
                window.draw(p);
            }
            
            if (showJourneys && portInRoute[i]) {
                sf::Text portLabel(ports[i].name, font, 11);
                portLabel.setFillColor(sf::Color::White);
                portLabel.setOutlineColor(sf::Color::Black);
                portLabel.setOutlineThickness(1.5f);
                
                sf::FloatRect bounds = portLabel.getLocalBounds();
                portLabel.setOrigin(bounds.width / 2.0f, 0);
                portLabel.setPosition(ports[i].x, ports[i].y + 8);
                
                window.draw(portLabel);
            }
            
            if (isHovering) {
                sf::Text hoverLabel(ports[i].name, font, 13);
                hoverLabel.setFillColor(sf::Color::Yellow);
                hoverLabel.setOutlineColor(sf::Color::Black);
                hoverLabel.setOutlineThickness(2.0f);
                hoverLabel.setStyle(sf::Text::Bold);
                
                sf::FloatRect hoverBounds = hoverLabel.getLocalBounds();
                hoverLabel.setOrigin(hoverBounds.width / 2.0f, 0);
                hoverLabel.setPosition(ports[i].x, ports[i].y + 8);
                
                window.draw(hoverLabel);
            }
        }

        delete[] portInRoute;

        window.draw(sidebar);
        window.draw(txtStart); window.draw(txtEnd);
        dateInput.draw(window);
        btnSearch.draw(window); btnDijkstra.draw(window); btnBook.draw(window); btnClear.draw(window);
        btnPreferences.draw(window);
        
        if (showPreferencesPanel) {
            window.draw(txtPrefTitle);
            window.draw(txtCompanyLabel);
            companyInput.draw(window);
            window.draw(txtAvoidLabel);
            avoidPortInput.draw(window);
            btnApplyPrefs.draw(window);
        }
        
        window.draw(txtStatus); window.draw(txtDetails);
        window.display();
    }
}

int main() {
    loadData();
    initCoordinates();
    runGraphics();
    
    if(ports) delete[] ports;
    if(visited) delete[] visited;
    if(exploredPorts) delete[] exploredPorts;
    if(finalPathPorts) delete[] finalPathPorts;
    return 0;
}