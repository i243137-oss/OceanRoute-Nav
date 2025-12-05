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

// Debug flag to enable demo queue data at Singapore
#define ENABLE_DEMO_QUEUE_DATA 1

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

// Custom min function - project avoids STL per design requirement
int minInt(int a, int b) {
    return (a < b) ? a : b;
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
const int MAX_ROUTES_TO_DISPLAY = 5;
const int MAX_ROUTES_FOR_GLOW = 3;
const float GLOW_OFFSET = 2.0f;
const int MAX_QUEUE_SHIPS_DISPLAY = 3; // Maximum number of animated ships to show in queue visualization
const int DEFAULT_DOCK_SLOTS = 1;      // Default number of docking slots per port (reduced to surface contention)
const int MAX_PORTS = 100;             // Maximum number of ports supported
const float SIM_SPEED_TO_MULTIPLIER = 60.0f; // Conversion factor: simSpeed (minutes/second) to speed multiplier (1x, 2x, etc.)

// Demo queue data constants (used when ENABLE_DEMO_QUEUE_DATA is enabled)
const int DEMO_QUEUE_COUNT = 2;        // Number of ships waiting in Singapore demo queue
const int DEMO_SERVICE_COUNT = 2;      // Number of ships being serviced in Singapore demo

// UI constants
const int QUEUE_LABEL_BUFFER_SIZE = 50; // Buffer size for queue label text
const int SPEED_LABEL_BUFFER_SIZE = 30; // Buffer size for speed label text

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

// ==========================================
// Simulation State
// ==========================================
Ship* activeShipsHead = nullptr;  // Linked list of active ships
int nextShipId = 1;                // Unique ID counter for ships
long long simTimeMinutes = 0;      // Current simulation time in absolute minutes since epoch
bool simPaused = false;            // Simulation auto-starts on launch (requirement)
int simSpeed = SIM_SPEED_1X;       // Simulation speed in minutes per real-time second (60 = 1x speed = 1 hour/second)
sf::Clock simClock;                // Clock for tracking real time
float simAccumulator = 0.0f;       // Accumulator for fractional minutes
sf::Clock globalAnimClock;         // Global animation clock for logging

// ==========================================
// Ship Simulation State
// ==========================================
struct ShipSimulation {
    bool isRunning;
    int currentLegIndex;
    float legProgress;      // 0.0 to 1.0 within current leg
    float simulationSpeed;  // 1.0 = normal, 2.0 = 2x speed
    int selectedJourneyIndex;
    
    // Ship position
    float shipX, shipY;
    
    // Current status
    enum Status { AT_PORT, DEPARTING, SAILING, ARRIVING } status;
    char currentPortName[50];
    char nextPortName[50];
    
    // Timing
    float timeAtPort;       // Time spent at current port
    float departureDelay;   // 2 seconds at each port
};

ShipSimulation shipSim = {false, 0, 0.0f, 1.0f, 0, 0, 0, ShipSimulation::AT_PORT, "", "", 0, 2.0f};

// ==========================================
// Port Queue System
// ==========================================
struct PortQueue {
    int shipCount;
    float ships[10]; // Arrival times of ships in queue
};

PortQueue* portQueues = nullptr; // One for each port (dynamically allocated)

// ==========================================
// Other Ships System
// ==========================================
struct OtherShip {
    bool active;
    int portIndex;
    char company[20];
    int shipNumber;
    enum State { ARRIVING, IN_QUEUE, DEPARTING, GONE } state;
    float stateTimer;
    float queuePosition; // Angle in the queue circle
};

OtherShip otherShips[20];
int otherShipCount = 0;

// ==========================================
// Ship Logs System
// ==========================================
struct ShipLog {
    char message[200];
    sf::Color color;
    float timestamp;
};

// Log storage (circular buffer)
ShipLog shipLogs[20];
int logCount = 0;
int logStartIndex = 0;

// Time simulation
struct TimeSimulation {
    int day;    // 1-31 (NEVER 0)
    int month;  // 1-12 (NEVER 0)
    int year;   // e.g., 2024
    int hour;   // 0-23
    int minute; // 0-59
    bool isPaused;
    
    // Constructor with valid defaults
    TimeSimulation() {
        day = SIM_BASE_DAY;      // Start at base date (20/12/2024)
        month = SIM_BASE_MONTH;
        year = SIM_BASE_YEAR;
        hour = 0;
        minute = 0;
        isPaused = false; // Auto-start simulation on launch
    }
};

TimeSimulation timeSim;

// Enhanced Color Palette for better visual hierarchy
sf::Color COL_BG_DARK(30, 30, 35);
sf::Color COL_ACCENT(0, 180, 255); 
sf::Color COL_BTN_HOVER(0, 200, 255);
sf::Color COL_BTN_ACTIVE(0, 220, 255);
sf::Color COL_BTN_IDLE(50, 50, 60);
sf::Color COL_TEXT_WHITE(240, 240, 240);
sf::Color COL_TEXT_MUTED(180, 180, 180);
sf::Color COL_INPUT_BG(255, 255, 255);
sf::Color COL_INPUT_FOCUS(200, 230, 255);
sf::Color COL_INPUT_PLACEHOLDER(150, 150, 150);
sf::Color COL_SEPARATOR(70, 70, 75);
sf::Color COL_SECTION_HEADER(0, 180, 255);
sf::Color COL_STATUS_SUCCESS(100, 255, 100);
sf::Color COL_STATUS_WARNING(255, 200, 100);

// Route colors for multi-route visualization
// Note: Array size matches MAX_ROUTES_TO_DISPLAY (5 colors for 5 max routes)
const sf::Color ROUTE_COLORS[5] = {
    sf::Color(0, 255, 100, 200),    // Green
    sf::Color(255, 200, 0, 200),    // Gold
    sf::Color(0, 200, 255, 200),    // Cyan
    sf::Color(255, 100, 200, 200),  // Pink
    sf::Color(200, 100, 255, 200)   // Purple
};
const int ROUTE_COLORS_COUNT = sizeof(ROUTE_COLORS) / sizeof(ROUTE_COLORS[0]);

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

// ==========================================
// Ship Logs & Simulation Functions
// ==========================================
void addShipLog(const char* message, bool isOurShip) {
    int index = (logStartIndex + logCount) % 20;
    if (logCount < 20) logCount++;
    else logStartIndex = (logStartIndex + 1) % 20;
    
    strcpy(shipLogs[index].message, message);
    shipLogs[index].color = isOurShip ? sf::Color::Green : sf::Color(255, 165, 0); // Green for our ship, Orange for others
    shipLogs[index].timestamp = globalAnimClock.getElapsedTime().asSeconds();
}

// Days in each month (index 1-12, index 0 unused)
int getDaysInMonth(int month, int year) {
    // Month is 1-12, NOT 0-11
    int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    //            ^-- Index 0 unused
    
    // Leap year check for February
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        return 29;
    }
    
    // Safety check: return 31 for invalid months to prevent crash
    // This should never happen in normal operation due to safety checks in caller
    if (month < 1 || month > 12) return 31;
    return days[month];
}

// Update time display based on simulation progress
void updateSimulatedTime(float deltaTime) {
    if (!timeSim.isPaused) {
        // Advance simulation time based on simSpeed
        // simSpeed is in minutes per real-time second (e.g., 60 = 1x speed = 1 hour per real second)
        static float fractionalMinutes = 0.0f;
        fractionalMinutes += deltaTime * simSpeed;
        
        int wholeMinutes = (int)fractionalMinutes;
        fractionalMinutes -= wholeMinutes;
        
        timeSim.minute += wholeMinutes;
        
        // Handle minute overflow -> hours
        while (timeSim.minute >= 60) {
            timeSim.minute -= 60;
            timeSim.hour++;
        }
        
        // Handle hour overflow -> days
        while (timeSim.hour >= 24) {
            timeSim.hour -= 24;
            timeSim.day++;
        }
        
        // Handle day overflow -> months
        while (timeSim.day > getDaysInMonth(timeSim.month, timeSim.year)) {
            timeSim.day = 1;  // Start of next month
            timeSim.month++;
            
            // Handle month overflow -> years
            // CRITICAL: Month resets to 1, NOT 0!
            if (timeSim.month > 12) {
                timeSim.month = 1;  // January = 1
                timeSim.year++;
            }
        }
        
        // SAFETY: Ensure month and day are never 0
        if (timeSim.month < 1) timeSim.month = 1;
        if (timeSim.day < 1) timeSim.day = 1;
    }
}

// Calculate angle between two points for ship rotation
float getAngle(float x1, float y1, float x2, float y2) {
    return atan2(y2 - y1, x2 - x1) * 180.0f / M_PI + 90.0f;
}

// Spawn random other ships
void spawnOtherShip(int portIndex) {
    if (otherShipCount >= 20) return;
    if (portIndex < 0 || portIndex >= totalPorts) return; // Bounds check
    
    const char* companies[] = {"MSC", "Maersk", "CMA-CGM", "Evergreen", "ONE"};
    const int numCompanies = sizeof(companies) / sizeof(companies[0]);
    
    OtherShip& ship = otherShips[otherShipCount++];
    ship.active = true;
    ship.portIndex = portIndex;
    strcpy(ship.company, companies[rand() % numCompanies]);
    ship.shipNumber = rand() % 1000;
    ship.state = OtherShip::ARRIVING;
    ship.stateTimer = 0;
    ship.queuePosition = portQueues[portIndex].shipCount * 45.0f;
    
    // Add to port queue (with bounds check for ships array)
    if (portQueues[portIndex].shipCount < 10) {
        portQueues[portIndex].shipCount++;
    }
    
    // Log arrival
    char msg[100];
    snprintf(msg, sizeof(msg), "Ship %s-%d arriving at %s", ship.company, ship.shipNumber, ports[portIndex].name);
    addShipLog(msg, false);
}

// Update other ships
void updateOtherShips(float deltaTime) {
    for (int i = 0; i < otherShipCount; i++) {
        OtherShip& ship = otherShips[i];
        if (!ship.active) continue;
        
        ship.stateTimer += deltaTime;
        
        switch (ship.state) {
            case OtherShip::ARRIVING:
                if (ship.stateTimer > 1.0f) {
                    ship.state = OtherShip::IN_QUEUE;
                    ship.stateTimer = 0;
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Ship %s-%d docked at %s", ship.company, ship.shipNumber, ports[ship.portIndex].name);
                    addShipLog(msg, false);
                }
                break;
                
            case OtherShip::IN_QUEUE:
                if (ship.stateTimer > 3.0f + (rand() % 3)) { // 3-5 seconds in queue
                    ship.state = OtherShip::DEPARTING;
                    ship.stateTimer = 0;
                    // Bounds check before decrementing
                    if (ship.portIndex >= 0 && ship.portIndex < totalPorts && portQueues[ship.portIndex].shipCount > 0) {
                        portQueues[ship.portIndex].shipCount--;
                    }
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Ship %s-%d departing from %s", ship.company, ship.shipNumber, ports[ship.portIndex].name);
                    addShipLog(msg, false);
                }
                break;
                
            case OtherShip::DEPARTING:
                if (ship.stateTimer > 1.0f) {
                    ship.state = OtherShip::GONE;
                    ship.active = false;
                }
                break;
        }
    }
}

// Draw ship icon with wake effect
void drawShip(sf::RenderWindow& window, float x, float y, float angle, sf::Color color) {
    // Ship triangle shape
    sf::ConvexShape ship;
    ship.setPointCount(3);
    ship.setPoint(0, sf::Vector2f(0, -12));   // Front
    ship.setPoint(1, sf::Vector2f(-8, 10));   // Back left
    ship.setPoint(2, sf::Vector2f(8, 10));    // Back right
    
    ship.setFillColor(color);
    ship.setOutlineColor(sf::Color::White);
    ship.setOutlineThickness(2);
    ship.setPosition(x, y);
    ship.setRotation(angle);
    
    window.draw(ship);
    
    // Draw wake effect behind ship
    sf::CircleShape wake(4);
    wake.setFillColor(sf::Color(255, 255, 255, 100));
    wake.setPosition(x - 15 * cos(angle * M_PI / 180.0f), 
                     y - 15 * sin(angle * M_PI / 180.0f));
    window.draw(wake);
}

// Draw port queue visualization (dashed line with ship markers)
void drawPortQueue(sf::RenderWindow& window, float portX, float portY, int queueSize) {
    if (queueSize <= 0) return;
    
    // Draw dashed circular queue around port
    float radius = 30.0f;
    for (int i = 0; i < queueSize && i < 8; i++) {
        float angle = (i * 45.0f) * 3.14159f / 180.0f; // 45 degrees apart
        float dashX = portX + radius * cos(angle);
        float dashY = portY + radius * sin(angle);
        
        // Each dash is a small rectangle representing a ship
        sf::RectangleShape dash(sf::Vector2f(8, 3));
        dash.setFillColor(sf::Color(255, 255, 255, 150));
        dash.setOrigin(4, 1.5f);
        dash.setPosition(dashX, dashY);
        dash.setRotation(angle * 180.0f / 3.14159f + 90);
        window.draw(dash);
    }
}

// Draw other ships in queue as dashes
void drawOtherShipsInQueue(sf::RenderWindow& window) {
    for (int i = 0; i < otherShipCount; i++) {
        OtherShip& ship = otherShips[i];
        if (!ship.active || ship.state == OtherShip::GONE) continue;
        
        // Bounds check for portIndex
        if (ship.portIndex < 0 || ship.portIndex >= totalPorts) continue;
        
        float portX = ports[ship.portIndex].x;
        float portY = ports[ship.portIndex].y;
        float radius = 35.0f;
        float angle = ship.queuePosition * 3.14159f / 180.0f;
        
        float dashX = portX + radius * cos(angle);
        float dashY = portY + radius * sin(angle);
        
        // Draw dash (small rectangle) representing ship in queue
        sf::RectangleShape dash(sf::Vector2f(10, 4));
        
        // Color based on state
        if (ship.state == OtherShip::ARRIVING) {
            dash.setFillColor(sf::Color(255, 200, 0, 200)); // Yellow - arriving
        } else if (ship.state == OtherShip::IN_QUEUE) {
            dash.setFillColor(sf::Color(255, 255, 255, 180)); // White - in queue
        } else if (ship.state == OtherShip::DEPARTING) {
            dash.setFillColor(sf::Color(100, 255, 100, 200)); // Green - departing
        }
        
        dash.setOrigin(5, 2);
        dash.setPosition(dashX, dashY);
        dash.setRotation(angle * 180.0f / 3.14159f + 90);
        window.draw(dash);
    }
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
    bool isPressed = false;

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

    void update(sf::Vector2i mousePos, bool mousePressed = false) {
        bool contains = shape.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y);
        
        if (contains && mousePressed) {
            // Active/pressed state
            shape.setFillColor(COL_BTN_ACTIVE);
            shape.setOutlineColor(COL_ACCENT);
            shape.setOutlineThickness(2);
            isHovered = true;
            isPressed = true;
        } else if (contains) {
            // Hover state
            shape.setFillColor(COL_BTN_HOVER);
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(1);
            isHovered = true;
            isPressed = false;
        } else {
            // Idle state
            shape.setFillColor(COL_BTN_IDLE);
            shape.setOutlineThickness(0);
            isHovered = false;
            isPressed = false;
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
    sf::Text placeholderText;
    bool isFocused = false;
    bool hasContent = false;  // Cache to avoid repeated string length checks
    char placeholder[50];

    void init(float x, float y, float w, float h, sf::Font& font, const char* placeholderStr = "") {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(w, h));
        shape.setFillColor(COL_INPUT_BG);
        shape.setOutlineColor(COL_ACCENT);
        displayText.setFont(font);
        displayText.setCharacterSize(14);
        displayText.setFillColor(sf::Color::Black);
        displayText.setPosition(x + 5, y + 5);
        
        // Setup placeholder with bounds checking
        if (placeholderStr != nullptr) {
            strncpy(placeholder, placeholderStr, sizeof(placeholder) - 1);
            placeholder[sizeof(placeholder) - 1] = '\0';
        } else {
            placeholder[0] = '\0';
        }
        placeholderText.setFont(font);
        placeholderText.setCharacterSize(14);
        placeholderText.setFillColor(COL_INPUT_PLACEHOLDER);
        placeholderText.setString(placeholder);
        placeholderText.setPosition(x + 5, y + 5);
    }

    void update(const char* content, bool focus) {
        displayText.setString(content);
        isFocused = focus;
        hasContent = (strlen(content) > 0);  // Cache content check
        if (isFocused) {
            shape.setOutlineThickness(2);
            shape.setFillColor(COL_INPUT_FOCUS);
        } else {
            shape.setOutlineThickness(1);
            shape.setOutlineColor(COL_SEPARATOR);
            shape.setFillColor(COL_INPUT_BG);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        // Show placeholder if input is empty and not focused
        if (!hasContent && !isFocused && placeholder[0] != '\0') {
            window.draw(placeholderText);
        } else {
            window.draw(displayText);
        }
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
    // Calculate absolute minutes from a consistent base date: 01/01/2024 00:00
    // This ensures proper time ordering across different dates
    
    long long totalMinutes = 0;
    
    // Add years (from 2024 base)
    int yearDiff = d.year - 2024;
    totalMinutes += (long long)yearDiff * 365 * 1440; // 365 days * 1440 minutes/day
    
    // Add months (using actual days per month for accuracy)
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // Adjust February for leap year
    if (d.year % 4 == 0 && (d.year % 100 != 0 || d.year % 400 == 0)) {
        daysInMonth[2] = 29;
    }
    
    for (int m = 1; m < d.month; m++) {
        totalMinutes += daysInMonth[m] * 1440;
    }
    
    // Add days (subtract 1 because day 1 is the first day of the month)
    totalMinutes += (long long)(d.day - 1) * 1440;
    
    // Add hours and minutes
    totalMinutes += t.hour * 60;
    totalMinutes += t.minute;
    
    return totalMinutes;
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
    
    // Allocate port queues dynamically based on total ports
    portQueues = new PortQueue[totalPorts];
    
    // Initialize port queues
    for (int i = 0; i < totalPorts; i++) {
        portQueues[i].shipCount = 0;
        for (int j = 0; j < 10; j++) {
            portQueues[i].ships[j] = 0.0f;
        }
    }
    
    // Initialize other ships
    for (int i = 0; i < 20; i++) {
        otherShips[i].active = false;
    }
    otherShipCount = 0;

    ifstream fp("ports.txt");
    if(fp.is_open()) {
        char n[50]; int c;
        int i = 0;
        while(fp >> n >> c && i < totalPorts) {
            strcpy(ports[i].name, n);
            ports[i].dailyCharge = c;
            ports[i].headRoute = nullptr;
            
            // Initialize dock queue management fields with defaults
            ports[i].dockSlots = DEFAULT_DOCK_SLOTS;  // Default docking slots per port
            ports[i].queueCount = 0;                   // No ships waiting initially
            ports[i].inServiceCount = 0;               // No ships being serviced initially
            ports[i].estWaitMinutes = 0;               // No wait time initially
            
            #if ENABLE_DEMO_QUEUE_DATA
            // Seed Singapore with demo queue data for visualization
            if (strcmp(ports[i].name, "Singapore") == 0) {
                ports[i].queueCount = DEMO_QUEUE_COUNT;       // Ships waiting in queue
                ports[i].inServiceCount = DEMO_SERVICE_COUNT; // Ships currently being serviced
                recomputeEstWait(ports[i]);                   // Compute estimated wait time
            }
            #endif
            
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
// Simulation Functions
// ==========================================
void addShipToActiveList(Ship* ship) {
    ship->next = activeShipsHead;
    activeShipsHead = ship;
}

void removeShipFromActiveList(int shipId) {
    Ship* prev = nullptr;
    Ship* curr = activeShipsHead;
    while (curr != nullptr) {
        if (curr->shipId == shipId) {
            if (prev == nullptr) {
                activeShipsHead = curr->next;
            } else {
                prev->next = curr->next;
            }
            delete curr;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void spawnShip(Journey& journey) {
    if (journey.legCount == 0) return;
    
    Ship* ship = new Ship;
    ship->shipId = nextShipId++;
    ship->originIndex = selectedStart;
    ship->destinationIndex = selectedEnd;
    
    // Copy journey legs
    ship->legCount = journey.legCount;
    for (int i = 0; i < journey.legCount; i++) {
        ship->legs[i] = journey.legs[i];
    }
    
    // Initialize ship state
    ship->currentLegIndex = 0;
    
    // Set departure and arrival times for first leg using ROUTE'S actual voyage date
    Route* firstLeg = ship->legs[0];
    ship->departureTimeMin = getMinutes(firstLeg->voyageDate, firstLeg->departureTime);
    ship->arrivalTimeMin = getMinutes(firstLeg->voyageDate, firstLeg->arrivalTime);
    
    // Handle day boundary crossing (arrival next day)
    if (firstLeg->arrivalTime.hour < firstLeg->departureTime.hour) {
        ship->arrivalTimeMin += 1440; // Add 24 hours
    }
    
    ship->currentPortIndex = ship->originIndex;
    ship->nextDepartureMin = ship->departureTimeMin;
    
    // Ship starts waiting at origin port until departure time
    // Join port queue - ship arrives at origin
    shipArrival(ports[ship->originIndex]);
    ship->state = WAITING_QUEUE;
    
    // Log ship booking/spawn with detailed timing information
    char logMsg[200];
    snprintf(logMsg, sizeof(logMsg), "[BOOK] Ship #%d booked: %s -> %s (%d legs, departs: %02d/%02d/%04d %02d:%02d)", 
             ship->shipId, ports[ship->originIndex].name, ports[ship->destinationIndex].name, 
             ship->legCount, firstLeg->voyageDate.day, firstLeg->voyageDate.month, 
             firstLeg->voyageDate.year, firstLeg->departureTime.hour, firstLeg->departureTime.minute);
    addShipLog(logMsg, true);
    
    #if DEBUG_ROUTE_EVALUATION
    printf("DEBUG SPAWN: Ship #%d spawned with departure=%lld, arrival=%lld, nextDeparture=%lld\n", 
           ship->shipId, ship->departureTimeMin, ship->arrivalTimeMin, ship->nextDepartureMin);
    #endif
    
    addShipToActiveList(ship);
}

void formatSimDateTime(long long absoluteMinutes, char* buffer, int bufferSize) {
    // Convert absolute minutes (from getMinutes()) back to date/time
    // Reverse the calculation in getMinutes(): year*525600 + month*43200 + day*1440 + hour*60 + minute
    // Note: Uses simplified calendar from existing codebase (30 days/month, 12*30=360 days/year)
    // The 525600 constant is 365 days but months are 30 days, creating a slight inconsistency
    // This matches the existing getMinutes() implementation used throughout the project
    
    long long remaining = absoluteMinutes;
    
    // Extract year (525600 minutes per year = 365 days * 1440 minutes/day)
    int year = (int)(remaining / 525600);
    remaining %= 525600;
    
    // Extract month (43200 minutes per month = 30 days * 1440 minutes/day)
    int month = (int)(remaining / 43200);
    remaining %= 43200;
    
    // Extract day (1440 minutes per day)
    int day = (int)(remaining / 1440);
    remaining %= 1440;
    
    // Extract hour and minute
    int hour = (int)(remaining / 60);
    int minute = (int)(remaining % 60);
    
    snprintf(buffer, bufferSize, "%02d/%02d/%04d %02d:%02d", day, month, year, hour, minute);
}

void processSimulationTick() {
    if (simPaused) return;
    
    Ship* curr = activeShipsHead;
    Ship* prev = nullptr;
    
    while (curr != nullptr) {
        Ship* next = curr->next;
        bool removeShip = false;
        
        switch (curr->state) {
            case TRAVELING:
                // Check if ship has arrived
                if (simTimeMinutes >= curr->arrivalTimeMin) {
                    // Ship has arrived at destination of current leg
                    int destPortIdx = curr->legs[curr->currentLegIndex]->destinationIndex;
                    curr->currentPortIndex = destPortIdx;
                    
                    // Log arrival at port
                    char arrivalMsg[200];
                    snprintf(arrivalMsg, sizeof(arrivalMsg), "[ARRIVE] Ship #%d arrived at %s", 
                             curr->shipId, ports[destPortIdx].name);
                    addShipLog(arrivalMsg, true);
                    
                    // Check if this is the final destination
                    if (curr->currentLegIndex >= curr->legCount - 1) {
                        // Reached final destination
                        char completeMsg[200];
                        snprintf(completeMsg, sizeof(completeMsg), "[COMPLETE] Ship #%d completed journey at %s", 
                                 curr->shipId, ports[destPortIdx].name);
                        addShipLog(completeMsg, true);
                        curr->state = COMPLETED;
                        removeShip = true;
                    } else {
                        // More legs to go - join port queue and prepare for next leg
                        curr->currentLegIndex++;
                        Route* nextLeg = curr->legs[curr->currentLegIndex];
                        curr->nextDepartureMin = getMinutes(nextLeg->voyageDate, nextLeg->departureTime);
                        
                        // Ship arrives at port and joins queue
                        shipArrival(ports[destPortIdx]);
                        curr->state = WAITING_QUEUE;
                        
                        // Log queue join
                        char queueMsg[200];
                        snprintf(queueMsg, sizeof(queueMsg), "[QUEUE] Ship #%d waiting at %s (Q:%d, Wait:%dmin)", 
                                 curr->shipId, ports[destPortIdx].name, 
                                 ports[destPortIdx].queueCount, ports[destPortIdx].estWaitMinutes);
                        addShipLog(queueMsg, true);
                    }
                }
                break;
                
            case WAITING_QUEUE:
                // Ships in queue must wait for two conditions:
                // 1. Scheduled departure time has arrived
                // 2. A dock slot is available
                
                // First, try to get a dock slot if available (transition to DOCKED)
                // Note: startService maintains FIFO ordering via queueCount mechanism
                // Only one ship per port can transition per tick, ensuring fair processing
                if (ports[curr->currentPortIndex].inServiceCount < ports[curr->currentPortIndex].dockSlots) {
                    // Dock slot available - start service
                    startService(ports[curr->currentPortIndex]);
                    curr->state = DOCKED;
                    
                    // Log docking
                    char dockMsg[200];
                    snprintf(dockMsg, sizeof(dockMsg), "[DOCK] Ship #%d docked at %s (waiting for departure time)", 
                             curr->shipId, ports[curr->currentPortIndex].name);
                    addShipLog(dockMsg, true);
                }
                // Ship stays in WAITING_QUEUE until dock opens
                break;
                
            case DOCKED:
                // Ship is docked and being serviced
                // Check if scheduled departure time has arrived
                
                // DEBUG: Log time comparison for debugging departure issues
                #if DEBUG_ROUTE_EVALUATION
                printf("DEBUG DOCKED: Ship #%d at %s: simTime=%lld, nextDeparture=%lld, diff=%lld min\n",
                       curr->shipId, ports[curr->currentPortIndex].name,
                       simTimeMinutes, curr->nextDepartureMin,
                       curr->nextDepartureMin - simTimeMinutes);
                #endif
                
                if (simTimeMinutes >= curr->nextDepartureMin) {
                    // Departure time reached - finish service and depart
                    finishService(ports[curr->currentPortIndex]);
                    
                    // Set up next leg travel
                    Route* departingLeg = curr->legs[curr->currentLegIndex];
                    curr->departureTimeMin = getMinutes(departingLeg->voyageDate, departingLeg->departureTime);
                    curr->arrivalTimeMin = getMinutes(departingLeg->voyageDate, departingLeg->arrivalTime);
                    
                    // Handle day boundary
                    if (departingLeg->arrivalTime.hour < departingLeg->departureTime.hour) {
                        curr->arrivalTimeMin += 1440;
                    }
                    
                    // Log departure
                    char departMsg[200];
                    snprintf(departMsg, sizeof(departMsg), "[DEPART] Ship #%d departed from %s to %s", 
                             curr->shipId, ports[curr->currentPortIndex].name, 
                             ports[departingLeg->destinationIndex].name);
                    addShipLog(departMsg, true);
                    
                    curr->state = TRAVELING;
                }
                // Ship stays DOCKED until departure time
                break;
                
            case COMPLETED:
            case CANCELED:
                removeShip = true;
                break;
        }
        
        if (removeShip) {
            if (prev == nullptr) {
                activeShipsHead = next;
                delete curr;
            } else {
                prev->next = next;
                delete curr;
            }
            curr = next;
        } else {
            prev = curr;
            curr = next;
        }
    }
}

void updateSimulationClock(float deltaTime) {
    if (simPaused) return;
    
    // deltaTime is in seconds, simSpeed is in minutes per real-time second
    // At 60x speed (1x multiplier): 1 real second = 60 sim minutes = 1 sim hour
    // simSpeed directly represents simulation minutes per real-time second
    simAccumulator += deltaTime * simSpeed;
    
    // Process whole minutes
    while (simAccumulator >= 1.0f) {
        simTimeMinutes++;
        simAccumulator -= 1.0f;
        processSimulationTick();
    }
}

void bookRouteAndSpawnShip(int routeIndex) {
    if (routeIndex < 0 || routeIndex >= foundJourneysCount) return;
    spawnShip(foundJourneys[routeIndex]);
}

// ==========================================
// Ship Simulation Update Function
// ==========================================
void updateShipSimulation(float deltaTime) {
    if (!shipSim.isRunning || foundJourneysCount == 0) return;
    
    Journey& journey = foundJourneys[shipSim.selectedJourneyIndex];
    
    if (shipSim.status == ShipSimulation::AT_PORT) {
        shipSim.timeAtPort += deltaTime;
        
        // Generate random other ship events at this port (time-based, approximately 1 per second on average)
        static float eventAccumulator = 0.0f;
        eventAccumulator += deltaTime;
        if (eventAccumulator >= 1.0f && (rand() % 100 < 50)) { // 50% chance per second
            eventAccumulator = 0.0f;
            char msg[200];
            const char* companies[] = {"MSC", "Maersk", "CMA-CGM", "Evergreen"};
            snprintf(msg, sizeof(msg), "Ship %s-%d arrived at %s", 
                    companies[rand() % 4], rand() % 1000, shipSim.currentPortName);
            addShipLog(msg, false);
        }
        
        if (shipSim.timeAtPort >= shipSim.departureDelay) {
            shipSim.status = ShipSimulation::DEPARTING;
            shipSim.timeAtPort = 0;
            
            if (shipSim.currentLegIndex < journey.legCount) {
                Route* leg = journey.legs[shipSim.currentLegIndex];
                strcpy(shipSim.nextPortName, ports[leg->destinationIndex].name);
                
                char msg[200];
                snprintf(msg, sizeof(msg), "[>>] Departing from %s to %s", 
                        shipSim.currentPortName, shipSim.nextPortName);
                addShipLog(msg, true);
            }
        }
    }
    else if (shipSim.status == ShipSimulation::SAILING || shipSim.status == ShipSimulation::DEPARTING) {
        shipSim.legProgress += deltaTime * 0.1f * shipSim.simulationSpeed;
        
        if (shipSim.legProgress >= 1.0f) {
            // Arrived at next port
            shipSim.legProgress = 0;
            shipSim.currentLegIndex++;
            shipSim.status = ShipSimulation::AT_PORT;
            strcpy(shipSim.currentPortName, shipSim.nextPortName);
            
            char msg[200];
            snprintf(msg, sizeof(msg), "[OK] Arrived at %s", shipSim.currentPortName);
            addShipLog(msg, true);
            
            if (shipSim.currentLegIndex >= journey.legCount) {
                addShipLog("[!!] Journey Complete!", true);
                shipSim.isRunning = false;
            }
        } else {
            shipSim.status = ShipSimulation::SAILING;
        }
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
    
    // FIXED SIDEBAR LAYOUT - No overlapping
    // === Section 1: Port Selection (y: 10-65) ===
    float section1Y = 10;
    
    // === Section 2: Date Input (y: 70-115) ===
    float section2Y = 70;
    
    // === Section 3: Search Buttons (y: 120-235) ===
    float section3Y = 120;
    
    // === Section 4: Status + Reset (y: 240-310) ===
    float section4Y = 240;
    
    // === Section 5: Preferences Button (y: 305-335) ===
    float section5Y = 305;
    
    // === Section 6: Simulation Controls (y: 340-380) ===
    float section6Y = 345;
    
    // === Section 7: Ship Logs (y: 395-1000) ===
    float section7Y = 395;
    
    InputBox dateInput; 
    dateInput.init(20, 88, 310, 25, font, "DD/MM/YYYY");
    
    // Preferences input boxes (positioned over map area)
    InputBox companyInput; 
    companyInput.init(MAP_OFFSET_X + 395, 283, 290, 28, font, "e.g., Maersk, MSC");
    
    InputBox avoidPortInput; 
    avoidPortInput.init(MAP_OFFSET_X + 395, 343, 290, 28, font, "e.g., Dubai, Mumbai");
    
    Button btnSearch, btnDijkstra, btnBook, btnClear;
    btnSearch.init(20, 138, 310, 28, "Find Routes (Date)", font);
    btnDijkstra.init(20, 170, 310, 28, "Find Cheapest Route", font);
    btnBook.init(20, 202, 310, 28, "Book Route (All)", font);
    btnClear.init(250, 238, 80, 20, "Reset", font);
    
    Button btnPreferences, btnApplyPrefs;
    btnPreferences.init(20, 305, 310, 28, "Preferences", font);
    btnApplyPrefs.init(MAP_OFFSET_X + 445, 385, 190, 30, "Apply Filters", font);
    
    // Simulation control buttons
    Button btnPlayPause, btnSpeedCycle;
    btnPlayPause.init(20, 363, 148, 24, "Play/Pause", font);
    btnSpeedCycle.init(173, 363, 157, 24, "Speed: 1.0x", font);

    // Section Headers and Labels with improved visual hierarchy
    sf::Text txtSectionPortSelect("PORT SELECTION", font, 10); 
    txtSectionPortSelect.setPosition(20, 10);
    txtSectionPortSelect.setFillColor(COL_SECTION_HEADER);
    txtSectionPortSelect.setStyle(sf::Text::Bold);
    
    sf::Text txtStart("From: None", font, 14); 
    txtStart.setPosition(20, 28);
    txtStart.setFillColor(COL_TEXT_WHITE);
    
    sf::Text txtEnd("To:   None", font, 14); 
    txtEnd.setPosition(20, 46);
    txtEnd.setFillColor(COL_TEXT_WHITE);
    
    // Date Section
    sf::Text txtSectionDate("DEPARTURE DATE", font, 10);
    txtSectionDate.setPosition(20, 70);
    txtSectionDate.setFillColor(COL_SECTION_HEADER);
    txtSectionDate.setStyle(sf::Text::Bold);
    
    // Actions Section
    sf::Text txtSectionActions("SEARCH OPTIONS", font, 10);
    txtSectionActions.setPosition(20, 120);
    txtSectionActions.setFillColor(COL_SECTION_HEADER);
    txtSectionActions.setStyle(sf::Text::Bold);
    
    // Status Section
    sf::Text txtSectionStatus("STATUS", font, 10);
    txtSectionStatus.setPosition(20, 240);
    txtSectionStatus.setFillColor(COL_SECTION_HEADER);
    txtSectionStatus.setStyle(sf::Text::Bold);
    
    sf::Text txtStatus(statusMessage, font, 13); 
    txtStatus.setPosition(20, 258); 
    txtStatus.setFillColor(COL_STATUS_SUCCESS);
    
    sf::Text txtDetails(pathDetails, font, 11); 
    txtDetails.setPosition(20, 278); 
    txtDetails.setFillColor(sf::Color::Cyan);
    
    // Preferences Section overlay elements (positioned over map area)
    sf::Text txtPrefTitle("PREFERENCES", font, 14); 
    txtPrefTitle.setPosition(MAP_OFFSET_X + 420, 230); 
    txtPrefTitle.setFillColor(sf::Color::Cyan);
    txtPrefTitle.setStyle(sf::Text::Bold);
    
    sf::Text txtCompanyLabel("Preferred Company (comma-sep):", font, 11); 
    txtCompanyLabel.setPosition(MAP_OFFSET_X + 395, 265); 
    txtCompanyLabel.setFillColor(COL_TEXT_WHITE);
    
    sf::Text txtAvoidLabel("Avoid Ports (comma-sep):", font, 11); 
    txtAvoidLabel.setPosition(MAP_OFFSET_X + 395, 325); 
    txtAvoidLabel.setFillColor(COL_TEXT_WHITE);
    
    sf::Text txtCloseBtn("X", font, 16);
    txtCloseBtn.setPosition(MAP_OFFSET_X + 670, 205);
    txtCloseBtn.setFillColor(sf::Color::Red);
    txtCloseBtn.setStyle(sf::Text::Bold);
    
    // Simulation Section
    sf::Text txtSimSection("SIMULATION", font, 10);
    txtSimSection.setPosition(20, 345);
    txtSimSection.setFillColor(COL_SECTION_HEADER);
    txtSimSection.setStyle(sf::Text::Bold);
    
    // Ship Logs Section
    sf::Text txtLogsTitle("SHIP LOGS", font, 10);
    txtLogsTitle.setPosition(20, 395);
    txtLogsTitle.setFillColor(COL_SECTION_HEADER);
    txtLogsTitle.setStyle(sf::Text::Bold);
    
    // Separator lines for visual grouping
    sf::RectangleShape separator1(sf::Vector2f(300, 1));
    separator1.setPosition(20, 60);
    separator1.setFillColor(COL_SEPARATOR);
    
    sf::RectangleShape separator2(sf::Vector2f(300, 1));
    separator2.setPosition(20, 115);
    separator2.setFillColor(COL_SEPARATOR);
    
    sf::RectangleShape separator3(sf::Vector2f(300, 1));
    separator3.setPosition(20, 235);
    separator3.setFillColor(COL_SEPARATOR);
    
    sf::RectangleShape separator4(sf::Vector2f(300, 1));
    separator4.setPosition(20, 300);
    separator4.setFillColor(COL_SEPARATOR);
    
    sf::RectangleShape separator5(sf::Vector2f(300, 1));
    separator5.setPosition(20, 340);
    separator5.setFillColor(COL_SEPARATOR);
    
    sf::RectangleShape separator6(sf::Vector2f(300, 1));
    separator6.setPosition(20, 390);
    separator6.setFillColor(COL_SEPARATOR);
    
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
                
                // If preferences panel is open, handle clicks specially
                if (showPreferencesPanel) {
                    // Check if clicking close button (X)
                    if (pos.x >= MAP_OFFSET_X + 665 && pos.x <= MAP_OFFSET_X + 695 && 
                        pos.y >= 200 && pos.y <= 225) {
                        showPreferencesPanel = false;
                    }
                    // Check if clicking Apply button
                    else if (btnApplyPrefs.isClicked(pos)) {
                        // Apply preferences logic...
                        userPrefs.preferredCompanyCount = 0;
                        userPrefs.avoidedPortCount = 0;
                        
                        if (strlen(tempCompany) > 0 || strlen(tempAvoidPort) > 0) {
                            userPrefs.usePreferences = true;
                            
                            // Parse comma-separated companies
                            if (strlen(tempCompany) > 0) {
                                char tempCopy[50];
                                strncpy(tempCopy, tempCompany, 49);
                                tempCopy[49] = '\0';
                                char* token = strtok(tempCopy, ",");
                                while (token != nullptr && userPrefs.preferredCompanyCount < 5) {
                                    // Trim leading spaces
                                    while (*token == ' ') token++;
                                    // Trim trailing spaces
                                    int len = strlen(token);
                                    if (len > 0) {
                                        char* end = token + len - 1;
                                        while (end > token && *end == ' ') { *end = '\0'; end--; }
                                    }
                                    
                                    if (strlen(token) > 0) {
                                        strcpy(userPrefs.preferredCompanies[userPrefs.preferredCompanyCount], token);
                                        userPrefs.preferredCompanyCount++;
                                    }
                                    token = strtok(nullptr, ",");
                                }
                            }
                            
                            // Parse comma-separated avoided ports
                            if (strlen(tempAvoidPort) > 0) {
                                char tempCopy[50];
                                strncpy(tempCopy, tempAvoidPort, 49);
                                tempCopy[49] = '\0';
                                char* token = strtok(tempCopy, ",");
                                while (token != nullptr && userPrefs.avoidedPortCount < 10) {
                                    // Trim leading spaces
                                    while (*token == ' ') token++;
                                    // Trim trailing spaces
                                    int len = strlen(token);
                                    if (len > 0) {
                                        char* end = token + len - 1;
                                        while (end > token && *end == ' ') { *end = '\0'; end--; }
                                    }
                                    
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
                    // Check if clicking input boxes
                    else if (companyInput.isClicked(pos)) {
                        focusCompany = true;
                        focusAvoidPort = false;
                    }
                    else if (avoidPortInput.isClicked(pos)) {
                        focusAvoidPort = true;
                        focusCompany = false;
                    }
                    // Click outside panel closes it
                    else if (pos.x < MAP_OFFSET_X + 375 || pos.x > MAP_OFFSET_X + 705 || 
                             pos.y < 195 || pos.y > 425) {
                        showPreferencesPanel = false;
                    }
                    
                    // Don't process other clicks when panel is open
                    continue;
                }
                
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
                if(btnBook.isClicked(pos)) {
                    findAllAvailableRoutes(selectedStart, selectedEnd, parseDate(inputDateString));
                    
                    if (foundJourneysCount > 0) {
                        // Find earliest departure time across ALL journeys
                        long long earliestDeparture = LLONG_MAX;
                        for (int i = 0; i < foundJourneysCount; i++) {
                            if (foundJourneys[i].legCount > 0) {
                                Route* firstLeg = foundJourneys[i].legs[0];
                                long long depTime = getMinutes(firstLeg->voyageDate, firstLeg->departureTime);
                                if (depTime < earliestDeparture) {
                                    earliestDeparture = depTime;
                                }
                            }
                        }
                        
                        // Spawn ships for all found routes
                        for (int i = 0; i < foundJourneysCount; i++) {
                            spawnShip(foundJourneys[i]);
                        }
                        
                        // Set simulation time to the earliest departure time
                        // This ensures all ships can depart when their scheduled time arrives
                        simTimeMinutes = earliestDeparture;
                        
                        // Update timeSim display to match simTimeMinutes
                        // Convert back from absolute minutes to Date/Time for display
                        // Note: This is a simplified reverse calculation for display only
                        // The actual simulation uses simTimeMinutes for accuracy
                        long long remaining = simTimeMinutes;
                        
                        // Extract year
                        int yearsPassed = (int)(remaining / (365 * 1440));
                        timeSim.year = 2024 + yearsPassed;
                        remaining -= (long long)yearsPassed * 365 * 1440;
                        
                        // Extract month (approximate using actual days per month)
                        int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                        if (timeSim.year % 4 == 0 && (timeSim.year % 100 != 0 || timeSim.year % 400 == 0)) {
                            daysInMonth[2] = 29;
                        }
                        
                        timeSim.month = 1;
                        long long daysRemaining = remaining / 1440;
                        while (timeSim.month <= 12 && daysRemaining >= daysInMonth[timeSim.month]) {
                            daysRemaining -= daysInMonth[timeSim.month];
                            timeSim.month++;
                        }
                        if (timeSim.month > 12) timeSim.month = 12;
                        
                        // Extract day (add 1 because day 1 is the first day)
                        timeSim.day = (int)daysRemaining + 1;
                        remaining -= daysRemaining * 1440;
                        
                        // Extract hour and minute
                        timeSim.hour = (int)(remaining / 60);
                        timeSim.minute = (int)(remaining % 60);
                        
                        // Safety checks
                        if (timeSim.month < 1) timeSim.month = 1;
                        if (timeSim.day < 1) timeSim.day = 1;
                        
                        timeSim.isPaused = false;
                        
                        char buff[100];
                        snprintf(buff, sizeof(buff), "Spawned %d ships!", foundJourneysCount);
                        strcpy(statusMessage, buff);
                        // Unpause simulation to start ship movement
                        simPaused = false;
                    }
                }
                
                // Simulation control buttons
                if(btnPlayPause.isClicked(pos)) {
                    simPaused = !simPaused;
                    timeSim.isPaused = simPaused;
                }
                
                if(btnSpeedCycle.isClicked(pos)) {
                    // Cycle through speed options: 0.5x -> 1x -> 2x -> 5x -> 10x -> 0.5x
                    // Speed limits: 0.5x to 10x maximum (as per requirements)
                    // Use ranges to avoid issues with floating point equality
                    if (shipSim.simulationSpeed < 0.75f) {  // 0.5x or less
                        shipSim.simulationSpeed = 1.0f;
                    } else if (shipSim.simulationSpeed < 1.5f) {  // 1.0x
                        shipSim.simulationSpeed = 2.0f;
                    } else if (shipSim.simulationSpeed < 3.5f) {  // 2.0x
                        shipSim.simulationSpeed = 5.0f;
                    } else if (shipSim.simulationSpeed < 7.5f) {  // 5.0x
                        shipSim.simulationSpeed = 10.0f;
                    } else {  // 10.0x or more
                        shipSim.simulationSpeed = 0.5f;
                    }
                    
                    // Synchronize with simSpeed (used by ship state machine)
                    // simSpeed is in minutes per real-time second
                    // Convert from speed multiplier to minutes per second: 1x = 60 min/sec
                    simSpeed = (int)(shipSim.simulationSpeed * SIM_SPEED_TO_MULTIPLIER);
                    
                    // Update button label with proper formatting
                    char speedLabel[SPEED_LABEL_BUFFER_SIZE];
                    snprintf(speedLabel, sizeof(speedLabel), "Speed: %.1fx", shipSim.simulationSpeed);
                    btnSpeedCycle.label.setString(speedLabel);
                    
                    // Re-center the label
                    sf::FloatRect textRect = btnSpeedCycle.label.getLocalBounds();
                    btnSpeedCycle.label.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
                    btnSpeedCycle.label.setPosition(173 + 157/2.0f, 363 + 24/2.0f);
                }
                
                if(btnClear.isClicked(pos)) {
                    selectedStart = -1; selectedEnd = -1; showJourneys = false;
                    bookingMode = 0;
                    showingDijkstra = false;
                    
                    // Reset ship simulation
                    shipSim.isRunning = false;
                    shipSim.currentLegIndex = 0;
                    shipSim.legProgress = 0;
                    logCount = 0;
                    logStartIndex = 0;
                    
                    userPrefs.usePreferences = false;
                    userPrefs.preferredCompanyCount = 0;
                    userPrefs.avoidedPortCount = 0;
                    tempCompany[0] = '\0';
                    tempAvoidPort[0] = '\0';
                    
                    // Clear all active ships
                    while (activeShipsHead != nullptr) {
                        Ship* temp = activeShipsHead;
                        activeShipsHead = activeShipsHead->next;
                        
                        // Remove ship from port queue if it's waiting or docked
                        if ((temp->state == WAITING_QUEUE || temp->state == DOCKED) && 
                            temp->currentPortIndex >= 0 && temp->currentPortIndex < totalPorts) {
                            // If docked, finish service to free dock slot
                            if (temp->state == DOCKED) {
                                finishService(ports[temp->currentPortIndex]);
                            }
                            // If waiting in queue, remove from queue
                            else if (temp->state == WAITING_QUEUE && ports[temp->currentPortIndex].queueCount > 0) {
                                ports[temp->currentPortIndex].queueCount--;
                                recomputeEstWait(ports[temp->currentPortIndex]);
                            }
                        }
                        
                        delete temp;
                    }
                    
                    // Reset simulation state to base date/time
                    Date baseDate = {SIM_BASE_DAY, SIM_BASE_MONTH, SIM_BASE_YEAR};
                    Time baseTime = {0, 0};
                    simTimeMinutes = getMinutes(baseDate, baseTime);
                    simPaused = false;  // Keep auto-start behavior
                    
                    // Reset TimeSimulation to base date
                    timeSim.day = SIM_BASE_DAY;
                    timeSim.month = SIM_BASE_MONTH;
                    timeSim.year = SIM_BASE_YEAR;
                    timeSim.hour = 0;
                    timeSim.minute = 0;
                    timeSim.isPaused = false;  // Keep auto-start behavior
                    
                    strcpy(statusMessage, "Ready."); strcpy(pathDetails, ""); strcpy(inputDateString, "20/12/2024");
                    txtStart.setString("From: None"); txtEnd.setString("To:   None");
                }
                
                if(btnPreferences.isClicked(pos)) {
                    showPreferencesPanel = !showPreferencesPanel;
                }
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        bool mousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        
        btnSearch.update(mPos, mousePressed); 
        btnDijkstra.update(mPos, mousePressed); 
        btnBook.update(mPos, mousePressed); 
        btnClear.update(mPos, mousePressed);
        btnPreferences.update(mPos, mousePressed); 
        btnApplyPrefs.update(mPos, mousePressed);
        btnPlayPause.update(mPos, mousePressed);
        btnSpeedCycle.update(mPos, mousePressed);
        dateInput.update(inputDateString, isTypingDate);
        companyInput.update(tempCompany, focusCompany);
        avoidPortInput.update(tempAvoidPort, focusAvoidPort);
        txtStatus.setString(statusMessage); txtDetails.setString(pathDetails);
        
        // Update simulation clock
        float deltaTime = simClock.restart().asSeconds();
        updateSimulationClock(deltaTime);
        
        // Update ship simulation
        updateShipSimulation(deltaTime);
        updateSimulatedTime(deltaTime);
        updateOtherShips(deltaTime);
        
        // Count active ships (for old system)
        int activeShipCount = 0;
        Ship* s = activeShipsHead;
        while (s != nullptr) {
            activeShipCount++;
            s = s->next;
        }
        
        if (showingDijkstra && explorationProgress < 1.0f) {
            explorationProgress += 0.02f;
            if (explorationProgress > 1.0f) explorationProgress = 1.0f;
        }

        window.clear(sf::Color(30, 30, 30));
        window.draw(sMap);

        bool hoverFound = false;
        float time = globalAnimClock.getElapsedTime().asSeconds();

        bool* portInRoute = new bool[totalPorts];
        for(int i=0; i<totalPorts; i++) portInRoute[i] = false;

        if (showJourneys) {
            // Limit the number of routes displayed to prevent clutter and crashes
            int maxRoutesToShow = minInt(foundJourneysCount, MAX_ROUTES_TO_DISPLAY);
            
            for(int i=0; i<maxRoutesToShow; i++) {
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

            for(int i=0; i<maxRoutesToShow; i++) {
                Journey& j = foundJourneys[i];
                
                float totalCycleDuration = 2.0f + (j.legCount * 0.2f);
                float cycleProgress = fmod(time, totalCycleDuration) / totalCycleDuration;
                float legProgressInCycle = cycleProgress * j.legCount;
                int activeLeg = (int)legProgressInCycle;
                if (activeLeg >= j.legCount) activeLeg = j.legCount - 1;
                float legProgress = legProgressInCycle - (float)activeLeg;
                if (legProgress > 1.0f) legProgress = 1.0f;
                
                float offset = (i - maxRoutesToShow/2.0f) * 8.0f;

                sf::Color pathColor;
                if (j.isDijkstra) {
                    pathColor = sf::Color(0, 150, 255, 220);
                } else if (bookingMode == 3) {
                    pathColor = ROUTE_COLORS[i % ROUTE_COLORS_COUNT];
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
                        
                        // Draw glow effect only for single route or filtered routes with few results
                        if (userPrefs.usePreferences && maxRoutesToShow <= MAX_ROUTES_FOR_GLOW) {
                            sf::Color glowColor = sf::Color(255, 200, 0, 60); // Lower alpha for less intensity
                            
                            // Draw just 2 glow lines (above and below) instead of 6
                            sf::Vertex glowLine1[] = {
                                sf::Vertex(sf::Vector2f(p1.x, p1.y - GLOW_OFFSET), glowColor),
                                sf::Vertex(sf::Vector2f(p2.x, p2.y - GLOW_OFFSET), glowColor)
                            };
                            sf::Vertex glowLine2[] = {
                                sf::Vertex(sf::Vector2f(p1.x, p1.y + GLOW_OFFSET), glowColor),
                                sf::Vertex(sf::Vector2f(p2.x, p2.y + GLOW_OFFSET), glowColor)
                            };
                            window.draw(glowLine1, 2, sf::Lines);
                            window.draw(glowLine2, 2, sf::Lines);
                        }

                        sf::Vertex line[] = {
                            sf::Vertex(p1, legColor),
                            sf::Vertex(p2, legColor)
                        };
                        window.draw(line, 2, sf::Lines);

                        // Draw ship simulation if active (ONLY our ship - removed white particle)
                        if (shipSim.isRunning && shipSim.selectedJourneyIndex == 0 && k == shipSim.currentLegIndex 
                            && (shipSim.status == ShipSimulation::SAILING || shipSim.status == ShipSimulation::DEPARTING)) {
                            sf::Vector2f shipPos = p1 + (p2 - p1) * shipSim.legProgress;
                            float shipAngle = getAngle(p1.x, p1.y, p2.x, p2.y);
                            
                            // Draw our ship as a colored triangle (green/cyan)
                            drawShip(window, shipPos.x, shipPos.y, shipAngle, sf::Color(0, 255, 150));
                            
                            // Update ship position for reference
                            shipSim.shipX = shipPos.x;
                            shipSim.shipY = shipPos.y;
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
        
        // Draw active ships in transit
        Ship* ship = activeShipsHead;
        while (ship != nullptr) {
            if (ship->state == TRAVELING) {
                Route* currentLeg = ship->legs[ship->currentLegIndex];
                int originIdx = (ship->currentLegIndex == 0) ? ship->originIndex : ship->legs[ship->currentLegIndex - 1]->destinationIndex;
                int destIdx = currentLeg->destinationIndex;
                
                // Calculate progress along current leg
                long long totalTravelTime = ship->arrivalTimeMin - ship->departureTimeMin;
                long long elapsedTime = simTimeMinutes - ship->departureTimeMin;
                float progress = 0.0f;
                
                if (totalTravelTime > 0 && elapsedTime >= 0) {
                    progress = (float)elapsedTime / (float)totalTravelTime;
                    if (progress > 1.0f) progress = 1.0f;
                    if (progress < 0.0f) progress = 0.0f;
                }
                
                // Interpolate position
                float shipX = ports[originIdx].x + (ports[destIdx].x - ports[originIdx].x) * progress;
                float shipY = ports[originIdx].y + (ports[destIdx].y - ports[originIdx].y) * progress;
                
                // Draw ship as a moving circle
                sf::CircleShape shipShape(5.0f);
                shipShape.setFillColor(sf::Color(255, 150, 0, 230));
                shipShape.setOutlineThickness(2.0f);
                shipShape.setOutlineColor(sf::Color(255, 255, 255, 200));
                shipShape.setOrigin(5.0f, 5.0f);
                shipShape.setPosition(shipX, shipY);
                window.draw(shipShape);
            } else if (ship->state == WAITING_QUEUE || ship->state == DOCKED) {
                // Draw ship waiting at port (near the port icon)
                // Ships in queue appear slightly farther out than docked ships
                float baseRadius = (ship->state == DOCKED) ? 20.0f : 30.0f;
                float angle = (float)ship->shipId * 0.5f; // Different angle for each ship
                float shipX = ports[ship->currentPortIndex].x + cos(angle) * baseRadius;
                float shipY = ports[ship->currentPortIndex].y + sin(angle) * baseRadius;
                
                sf::CircleShape shipShape(4.0f);
                // Color coding: Blue for waiting in queue, Cyan for docked
                sf::Color shipColor = (ship->state == DOCKED) ? sf::Color(0, 255, 255, 230) : sf::Color(100, 200, 255, 230);
                shipShape.setFillColor(shipColor);
                shipShape.setOutlineThickness(1.5f);
                shipShape.setOutlineColor(sf::Color(255, 255, 255, 200));
                shipShape.setOrigin(4.0f, 4.0f);
                shipShape.setPosition(shipX, shipY);
                window.draw(shipShape);
            }
            ship = ship->next;
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
            
            // Draw queue visualization if port has waiting ships
            if (ports[i].queueCount > 0) {
                // Draw dashed line queue indicator - length proportional to queue size
                float queueLineLength = 30.0f + (ports[i].queueCount * 10.0f);
                float dashLength = 5.0f;
                float gapLength = 3.0f;
                float startX = ports[i].x - queueLineLength;
                float startY = ports[i].y - 15.0f; // Above the port pin
                
                // Draw dashed line approaching the port
                int numDashes = (int)(queueLineLength / (dashLength + gapLength));
                for (int d = 0; d < numDashes; d++) {
                    float dashX = startX + d * (dashLength + gapLength);
                    sf::Vertex dash[] = {
                        sf::Vertex(sf::Vector2f(dashX, startY), sf::Color(200, 200, 0, 180)),
                        sf::Vertex(sf::Vector2f(dashX + dashLength, startY), sf::Color(200, 200, 0, 180))
                    };
                    window.draw(dash, 2, sf::Lines);
                }
                
                // Animate small circles (ships) moving toward the port
                int shipsToShow = (ports[i].queueCount > MAX_QUEUE_SHIPS_DISPLAY) ? MAX_QUEUE_SHIPS_DISPLAY : ports[i].queueCount;
                for (int s = 0; s < shipsToShow; s++) {
                    // Each ship animates with a different phase offset
                    float animPhase = fmod(time * 0.5f + s * 0.3f, 1.0f);
                    float shipX = startX + animPhase * queueLineLength;
                    float shipY = startY;
                    
                    sf::CircleShape ship(3.0f);
                    ship.setFillColor(sf::Color(255, 200, 50, 220));
                    ship.setOutlineThickness(1.0f);
                    ship.setOutlineColor(sf::Color(180, 150, 0, 220));
                    ship.setOrigin(3.0f, 3.0f);
                    ship.setPosition(shipX, shipY);
                    window.draw(ship);
                }
                
                // Draw text label showing queue size and estimated wait
                char queueLabel[QUEUE_LABEL_BUFFER_SIZE];
                // Display time in hours if >= 1 hour, otherwise show minutes
                if (ports[i].estWaitMinutes >= 60) {
                    snprintf(queueLabel, sizeof(queueLabel), "Q:%d | %dh", 
                            ports[i].queueCount, ports[i].estWaitMinutes / 60);
                } else {
                    snprintf(queueLabel, sizeof(queueLabel), "Q:%d | %dm", 
                            ports[i].queueCount, ports[i].estWaitMinutes);
                }
                
                sf::Text queueText(queueLabel, font, 9);
                queueText.setFillColor(sf::Color(255, 255, 100));
                queueText.setOutlineColor(sf::Color::Black);
                queueText.setOutlineThickness(1.0f);
                queueText.setPosition(ports[i].x - 35, ports[i].y - 30);
                window.draw(queueText);
            }
            
            // Draw additional port queue visualization (dashed circles)
            drawPortQueue(window, ports[i].x, ports[i].y, portQueues[i].shipCount);
            
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
        
        // Draw other ships in queue
        drawOtherShipsInQueue(window);
        
        // Draw time simulation display on TOP of the map (not in sidebar)
        // Always show the clock (not just when simulation is running)
        {
            // Position at top center of map area
            float mapCenterX = MAP_OFFSET_X + (1536 / 2.0f);
            float timeY = 20;
            
            // Background box for time display
            sf::RectangleShape timeBox(sf::Vector2f(250, 60));
            timeBox.setFillColor(sf::Color(0, 0, 0, 200));
            timeBox.setOutlineColor(sf::Color::Cyan);
            timeBox.setOutlineThickness(2);
            timeBox.setPosition(mapCenterX - 125, timeY);
            window.draw(timeBox);
            
            // Format and display simulation time
            char timeStr[50];
            // Display date starting from user's entered date
            // Format: DD/MM/YYYY HH:MM
            snprintf(timeStr, sizeof(timeStr), "%02d/%02d/%04d %02d:%02d", 
                    timeSim.day,    // 01-31 (never 00)
                    timeSim.month,  // 01-12 (never 00)
                    timeSim.year,   // 2024, 2025, etc.
                    timeSim.hour,   // 00-23
                    timeSim.minute  // 00-59
            );
            sf::Text timeText(timeStr, font, 16);
            timeText.setFillColor(sf::Color::White);
            timeText.setStyle(sf::Text::Bold);
            
            // Center the text in the box
            sf::FloatRect textBounds = timeText.getLocalBounds();
            timeText.setOrigin(textBounds.width / 2, textBounds.height / 2);
            timeText.setPosition(mapCenterX, timeY + 18);
            window.draw(timeText);
            
            // Pause/speed indicator below time
            char statusStr[40];
            float speedMultiplier = simSpeed / SIM_SPEED_TO_MULTIPLIER;  // Convert from minutes/second to multiplier (1x, 2x, etc.)
            if (simPaused) {
                snprintf(statusStr, sizeof(statusStr), "PAUSED | Speed: %.1fx", speedMultiplier);
            } else {
                snprintf(statusStr, sizeof(statusStr), "RUNNING | Speed: %.1fx", speedMultiplier);
            }
            sf::Text statusText(statusStr, font, 12);
            statusText.setFillColor(simPaused ? sf::Color(255, 150, 150) : sf::Color(150, 255, 150));
            sf::FloatRect statusBounds = statusText.getLocalBounds();
            statusText.setOrigin(statusBounds.width / 2, 0);
            statusText.setPosition(mapCenterX, timeY + 40);
            window.draw(statusText);
        }

        // Draw sidebar and all UI elements with proper layering
        window.draw(sidebar);
        
        // Draw separators for visual grouping
        window.draw(separator1);
        window.draw(separator2);
        window.draw(separator3);
        window.draw(separator4);
        window.draw(separator5);
        window.draw(separator6);
        
        // Section 1: Port Selection
        window.draw(txtSectionPortSelect);
        window.draw(txtStart); 
        window.draw(txtEnd);
        
        // Section 2: Date Input
        window.draw(txtSectionDate);
        dateInput.draw(window);
        
        // Section 3: Action Buttons
        window.draw(txtSectionActions);
        btnSearch.draw(window); 
        btnDijkstra.draw(window); 
        btnBook.draw(window); 
        btnClear.draw(window);
        
        // Section 4: Status
        window.draw(txtSectionStatus);
        window.draw(txtStatus); 
        window.draw(txtDetails);
        
        // Section 5: Preferences button
        btnPreferences.draw(window);
        
        // Section 6: Simulation Controls
        window.draw(txtSimSection);
        btnPlayPause.draw(window);
        btnSpeedCycle.draw(window);
        
        // Section 7: Ship Logs Panel
        sf::RectangleShape logsPanel(sf::Vector2f(310, 600));
        logsPanel.setPosition(20, 413);
        logsPanel.setFillColor(sf::Color(15, 18, 25, 240));
        logsPanel.setOutlineColor(sf::Color(50, 55, 65));
        logsPanel.setOutlineThickness(1);
        window.draw(logsPanel);
        
        window.draw(txtLogsTitle);
        
        // Draw each log entry (most recent first)
        int yOffset = 418;
        for (int i = 0; i < logCount && i < 30; i++) {
            int idx = (logStartIndex + logCount - 1 - i) % 20; // Most recent first
            sf::Text logText(shipLogs[idx].message, font, 9);
            logText.setPosition(25, yOffset);
            logText.setFillColor(shipLogs[idx].color);
            window.draw(logText);
            yOffset += 15;
            if (yOffset > 990) break; // Don't draw beyond visible area
        }
        
        // Preferences panel - draw as an OVERLAY on top of map (centered)
        if (showPreferencesPanel) {
            // Semi-transparent dark overlay for the entire screen to focus on preferences
            sf::RectangleShape screenOverlay(sf::Vector2f(1536 + MAP_OFFSET_X, 1024));
            screenOverlay.setFillColor(sf::Color(0, 0, 0, 150));
            screenOverlay.setPosition(0, 0);
            window.draw(screenOverlay);
            
            // Preferences panel background
            sf::RectangleShape prefOverlay(sf::Vector2f(330, 230));
            prefOverlay.setFillColor(sf::Color(20, 25, 35, 250));
            prefOverlay.setOutlineColor(sf::Color(0, 180, 255));
            prefOverlay.setOutlineThickness(3);
            prefOverlay.setPosition(MAP_OFFSET_X + 380, 195);
            window.draw(prefOverlay);
            
            // Preferences title
            window.draw(txtPrefTitle);
            
            // Close button (X) in top right corner of panel
            window.draw(txtCloseBtn);
            
            // Company filter
            window.draw(txtCompanyLabel);
            companyInput.draw(window);
            
            // Avoid ports filter
            window.draw(txtAvoidLabel);
            avoidPortInput.draw(window);
            
            // Apply button
            btnApplyPrefs.draw(window);
        }
        
        window.display();
    }
}

int main() {
    // Seed random number generator for ship events
    srand((unsigned int)time(NULL));
    
    loadData();
    initCoordinates();
    
    // Initialize simulation time to base date
    Date baseDate = {SIM_BASE_DAY, SIM_BASE_MONTH, SIM_BASE_YEAR};
    Time baseTime = {0, 0};
    simTimeMinutes = getMinutes(baseDate, baseTime);
    
    runGraphics();
    
    // Clean up active ships
    while (activeShipsHead != nullptr) {
        Ship* temp = activeShipsHead;
        activeShipsHead = activeShipsHead->next;
        delete temp;
    }
    
    if(ports) delete[] ports;
    if(visited) delete[] visited;
    if(exploredPorts) delete[] exploredPorts;
    if(finalPathPorts) delete[] finalPathPorts;
    if(portQueues) delete[] portQueues;
    return 0;
}