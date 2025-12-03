#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cmath> // Needed for sin()
#include <SFML/Graphics.hpp> 
#include "DataStructures.h" 

using namespace std;

// ==========================================
// 0. Custom Helper Functions (No STL)
// ==========================================

// Helper to convert int to char array
void intToString(int val, char* buffer) {
    sprintf(buffer, "%d", val);
}

// Helper to append char to string
void appendChar(char* str, char c, int maxSize) {
    int len = strlen(str);
    if (len < maxSize - 1) {
        str[len] = c;
        str[len + 1] = '\0';
    }
}

// Helper to remove last char
void popBack(char* str) {
    int len = strlen(str);
    if (len > 0) {
        str[len - 1] = '\0';
    }
}

// ==========================================
// 1. Global Data & Constants
// ==========================================
// Dynamic Array Pointer
Port* ports = nullptr; 
int totalPorts = 0;    

// UI Constants
const float SIDEBAR_WIDTH = 350.0f;
const float MAP_OFFSET_X = 350.0f; 

// App State
int selectedStart = -1;
int selectedEnd = -1;
int hoveredPortIndex = -1; 

// Text buffers instead of strings
char inputStartName[50] = ""; 
char inputEndName[50] = "";   
char inputDateString[20] = "01/01/2024";
char statusMessage[100] = "Ready to navigate.";
char pathDetails[100] = ""; 

bool isTypingStart = false; 
bool isTypingEnd = false;
bool isTypingDate = false;

// Colors
sf::Color COL_BG_DARK(30, 30, 35);
sf::Color COL_ACCENT(0, 180, 255); 
sf::Color COL_BTN_HOVER(0, 200, 255);
sf::Color COL_BTN_IDLE(50, 50, 60);
sf::Color COL_TEXT_WHITE(240, 240, 240);
sf::Color COL_INPUT_BG(255, 255, 255);
sf::Color COL_INPUT_FOCUS(200, 230, 255);

// ==========================================
// 2. UI Components (Buttons & Boxes)
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
        label.setCharacterSize(18);
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
        displayText.setCharacterSize(18);
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

// Path Storage (Replaces std::vector)
struct PathSegment {
    sf::Vector2f start;
    sf::Vector2f end;
    PathSegment* next; // Linked list for segments
};

// Custom List for Path Segments
struct PathList {
    PathSegment* head;
    
    PathList() : head(nullptr) {}
    
    void add(sf::Vector2f s, sf::Vector2f e) {
        PathSegment* newNode = new PathSegment;
        newNode->start = s;
        newNode->end = e;
        newNode->next = head;
        head = newNode;
    }
    
    void clear() {
        PathSegment* current = head;
        while (current != nullptr) {
            PathSegment* temp = current;
            current = current->next;
            delete temp;
        }
        head = nullptr;
    }
};

PathList bestPathLines;
bool pathFound = false;

// Helpers for All Routes Mode
struct PathData { int stops[50]; int count; };
PathData allPaths[100];
int allPathsCount = 0;
bool showAllPaths = false;
bool* visitedDFS = nullptr; 
int tempDFS[50];
long long dfsSteps = 0;

// ==========================================
// 3. Data Loading & Mapping
// ==========================================

int getPortIndex(const char* name) {
    for(int i=0; i<totalPorts; i++) if(strcmp(ports[i].name, name)==0) return i;
    return -1;
}

// Manual Line Counting
int countFileLines(const char* filename) {
    ifstream f(filename);
    if(!f.is_open()) return 0;
    int count = 0;
    char buffer[100];
    while(f.getline(buffer, 100)) {
        if(strlen(buffer) > 0) count++;
    }
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
}

long long getMinutes(Date d, Time t) {
    return ((long long)d.year*525600) + ((long long)d.month*43200) + ((long long)d.day*1440) + (t.hour*60) + t.minute;
}

// Manual String Parsing for Date
Date parseDate(char* s) {
    Date d = {1, 1, 2024};
    int i = 0;
    // Parse Day
    int val = 0;
    while(s[i] != '/' && s[i] != '\0') {
        val = val * 10 + (s[i] - '0');
        i++;
    }
    if (val > 0) d.day = val;
    if (s[i] == '\0') return d;
    i++; // Skip /

    // Parse Month
    val = 0;
    while(s[i] != '/' && s[i] != '\0') {
        val = val * 10 + (s[i] - '0');
        i++;
    }
    if (val > 0) d.month = val;
    if (s[i] == '\0') return d;
    i++; // Skip /

    // Parse Year
    val = 0;
    while(s[i] != '\0') {
        val = val * 10 + (s[i] - '0');
        i++;
    }
    if (val > 0) d.year = val;

    return d;
}

void loadData() {
    totalPorts = countFileLines("ports.txt");
    if(totalPorts == 0) { cout << "Error: ports.txt empty or missing!" << endl; return; }

    ports = new Port[totalPorts];
    visitedDFS = new bool[totalPorts]; 

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
        char o[50], d[50], co[50], sk; int c; Date dt; Time dep, arr;
        while(fr >> o >> d >> dt.day >> sk >> dt.month >> sk >> dt.year >> dep.hour >> sk >> dep.minute >> arr.hour >> sk >> arr.minute >> c >> co) {
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
// 4. Algorithms Logic
// ==========================================

void resetAlgorithms() {
    for(int i=0; i<totalPorts; i++) {
        ports[i].minCost = 99999999;
        ports[i].minTime = 2000000000;
        ports[i].parentIndex = -1;
        ports[i].visited = false;
        visitedDFS[i] = false;
    }
    bestPathLines.clear();
    pathFound = false;
    allPathsCount = 0;
    dfsSteps = 0;
    showAllPaths = false;
}

void runCheapest(int start, int end) {
    resetAlgorithms();
    if(start == -1 || end == -1) { strcpy(statusMessage, "Error: Select Start & End Points"); return; }

    MinHeap pq(totalPorts * totalPorts);
    ports[start].minCost = 0;
    pq.push(start, 0);

    while(!pq.isEmpty()) {
        int u = pq.extractMin().portIndex;
        if(ports[u].visited) continue;
        ports[u].visited = true;
        if(u == end) break;

        Route* e = ports[u].headRoute;
        while(e) {
            int v = e->destinationIndex;
            if(!ports[v].visited && ports[u].minCost + e->cost < ports[v].minCost) {
                ports[v].minCost = ports[u].minCost + e->cost;
                ports[v].parentIndex = u;
                pq.push(v, ports[v].minCost);
            }
            e = e->next;
        }
    }

    if(ports[end].minCost != 99999999) {
        strcpy(statusMessage, "Cheapest Route Calculated!");
        char costStr[50];
        intToString(ports[end].minCost, costStr);
        strcpy(pathDetails, "Total Cost: $");
        strcat(pathDetails, costStr);
         if (ports[end].minCost != 99999999) {
        statusMessage = "Route Found! Total: $" + to_string(ports[end].minCost);
        
        // Reconstruct Path for text display
        IntStack pathStack;
        int curr = end;
        while (curr != -1) {
            pathStack.push(curr);
            curr = ports[curr].parentIndex;
        }

        pathDetails = "VOYAGE PLAN:\n";
        while (!pathStack.isEmpty()) {
            int pIdx = pathStack.pop();
            pathDetails += "-> ";
            pathDetails += ports[pIdx].name;
            pathDetails += "\n";
        }

    } else {
        statusMessage = "No route possible.";
        pathDetails = "";
    }
        
        pathFound = true;
        int curr = end;
        while(curr != start && curr != -1) {
            int p = ports[curr].parentIndex;
            if(p != -1) {
                // FIXED: Store segments as Source -> Dest for correct animation flow
                bestPathLines.add(sf::Vector2f(ports[p].x, ports[p].y), sf::Vector2f(ports[curr].x, ports[curr].y));
            }
            curr = p;
        }
    } else {
        strcpy(statusMessage, "No path exists.");
    }
}

void runFastest(int start, int end, Date date) {
    resetAlgorithms();
    if(start == -1 || end == -1) { strcpy(statusMessage, "Error: Select Start & End Points"); return; }

    MinHeap pq(totalPorts * totalPorts);
    Time t = {0,0};
    long long startMin = getMinutes(date, t);
    ports[start].minTime = (int)startMin;
    pq.push(start, ports[start].minTime);

    while(!pq.isEmpty()) {
        int u = pq.extractMin().portIndex;
        int currT = ports[u].minTime;
        
        if(ports[u].visited) continue;
        ports[u].visited = true;
        if(u == end) break;

        Route* e = ports[u].headRoute;
        while(e) {
            int v = e->destinationIndex;
            long long dep = getMinutes(e->voyageDate, e->departureTime);
            long long arr = getMinutes(e->voyageDate, e->arrivalTime);
            if(e->arrivalTime.hour < e->departureTime.hour) arr += 1440; 

            if(currT <= dep) { 
                if(!ports[v].visited && arr < ports[v].minTime) {
                    ports[v].minTime = (int)arr;
                    ports[v].parentIndex = u;
                    pq.push(v, (int)arr);
                }
            }
            e = e->next;
        }
    }

    if(ports[end].minTime != 2000000000) {
        strcpy(statusMessage, "Fastest Route Calculated!");
        long long mins = ports[end].minTime - startMin;
        char hoursStr[50];
        intToString(mins/60, hoursStr);
        strcpy(pathDetails, "Duration: ");
        strcat(pathDetails, hoursStr);
        strcat(pathDetails, " Hours");
        
        pathFound = true;
        int curr = end;
        while(curr != start && curr != -1) {
            int p = ports[curr].parentIndex;
            if(p != -1) {
                // FIXED: Store segments as Source -> Dest for correct animation flow
                bestPathLines.add(sf::Vector2f(ports[p].x, ports[p].y), sf::Vector2f(ports[curr].x, ports[curr].y));
            }
            curr = p;
        }
    } else {
        strcpy(statusMessage, "No valid route for this date.");
        strcpy(pathDetails, "Check schedule.");
    }
}

void dfs(int u, int end, int d, int cost) {
    if(dfsSteps++ > 100000 || allPathsCount >= 50 || d >= 20) return;
    visitedDFS[u] = true;
    tempDFS[d] = u;

    if(u == end) {
        allPaths[allPathsCount].count = d + 1;
        for(int i=0; i<=d; i++) allPaths[allPathsCount].stops[i] = tempDFS[i];
        allPathsCount++;
    } else {
        Route* e = ports[u].headRoute;
        while(e) {
            if(!visitedDFS[e->destinationIndex]) dfs(e->destinationIndex, end, d+1, cost);
            e = e->next;
        }
    }
    visitedDFS[u] = false;
}

void runAllRoutes(int start, int end) {
    resetAlgorithms();
    if(start == -1 || end == -1) { strcpy(statusMessage, "Error: Select Start & End Points"); return; }
    
    strcpy(statusMessage, "Searching connections...");
    dfs(start, end, 0, 0);
    
    if(allPathsCount > 0) {
        char countStr[20];
        intToString(allPathsCount, countStr);
        strcpy(statusMessage, "Found ");
        strcat(statusMessage, countStr);
        strcat(statusMessage, " routes.");
        strcpy(pathDetails, "Shown in Blue.");
        showAllPaths = true;
    } else {
        strcpy(statusMessage, "No routes found.");
    }
}

// ==========================================
// 5. Visualization System
// ==========================================

void runGraphics() {
    sf::RenderWindow window(sf::VideoMode(1536 + (int)SIDEBAR_WIDTH, 1024), "OceanRoute Nav System", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Clock animClock; // For path animation

    sf::Texture tMap, tPin;
    if(!tMap.loadFromFile("map.png")) cout << "Map Error" << endl;
    bool hasPin = tPin.loadFromFile("pin.png");
    if(hasPin) tPin.setSmooth(true);
    
    sf::Sprite sMap(tMap);
    sMap.setScale(1536.0f/tMap.getSize().x, 1024.0f/tMap.getSize().y);
    sMap.setPosition(MAP_OFFSET_X, 0);

    sf::Font font;
    if(!font.loadFromFile("arial.ttf")) cout << "Font Error" << endl;

    // --- GUI SETUP ---
    // Sidebar on the LEFT (0 to 350)
    sf::RectangleShape sidebar(sf::Vector2f(SIDEBAR_WIDTH, 1024.0f));
    sidebar.setFillColor(COL_BG_DARK); 
    sidebar.setPosition(0, 0); 
    sidebar.setOutlineColor(sf::Color(100, 100, 100));
    sidebar.setOutlineThickness(2);

    float uiPadding = 20.0f;
    
    sf::Text txtTitle("NAVIGATION CONTROL", font, 22); 
    txtTitle.setPosition(uiPadding, 30); txtTitle.setFillColor(sf::Color(0, 255, 255)); 

    sf::Text txtStart("Departure: None", font, 18); txtStart.setPosition(uiPadding, 80);
    sf::Text txtEnd("Arrival:     None", font, 18); txtEnd.setPosition(uiPadding, 120);

    sf::Text txtDateLabel("Voyage Date:", font, 18); txtDateLabel.setPosition(uiPadding, 170);
    
    InputBox dateInput;
    dateInput.init(uiPadding, 200, 300, 35, font);

    Button btnCheap, btnFast, btnAll, btnClear;
    btnCheap.init(uiPadding, 260, 300, 45, "Cheapest ($)", font);
    btnFast.init(uiPadding, 320, 300, 45, "Fastest (Time)", font);
    btnAll.init(uiPadding, 380, 300, 45, "Show All Routes", font);
    btnClear.init(uiPadding, 460, 300, 45, "Reset / Clear", font);

    sf::Text txtMsg(statusMessage, font, 16); 
    txtMsg.setPosition(uiPadding, 540); 
    txtMsg.setFillColor(sf::Color::Yellow);
    
    sf::Text txtRes(pathDetails, font, 20); 
    txtRes.setPosition(uiPadding, 580); 
    txtRes.setFillColor(sf::Color::Green);

    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) window.close();

            if(event.type == sf::Event::TextEntered) {
                if(isTypingDate) {
                    if(event.text.unicode == 8) popBack(inputDateString);
                    else if(event.text.unicode >= 32 && event.text.unicode < 128) appendChar(inputDateString, (char)event.text.unicode, 20);
                }
            }

            if(event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i pos = sf::Mouse::getPosition(window);
                
                if(dateInput.isClicked(pos)) isTypingDate = true;
                else isTypingDate = false;

                if(pos.x > SIDEBAR_WIDTH) { 
                    for(int i=0; i<totalPorts; i++) {
                        if(abs(pos.x - ports[i].x) < 20 && abs(pos.y - ports[i].y) < 20) {
                            if(event.mouseButton.button == sf::Mouse::Left) {
                                selectedStart = i; 
                                strcpy(inputStartName, ports[i].name);
                                char buffer[60] = "Departure: ";
                                strcat(buffer, inputStartName);
                                txtStart.setString(buffer);
                            } else if(event.mouseButton.button == sf::Mouse::Right) {
                                selectedEnd = i; 
                                strcpy(inputEndName, ports[i].name);
                                char buffer[60] = "Arrival:     ";
                                strcat(buffer, inputEndName);
                                txtEnd.setString(buffer);
                            }
                        }
                    }
                }

                if(btnCheap.isClicked(pos)) runCheapest(selectedStart, selectedEnd);
                if(btnFast.isClicked(pos)) runFastest(selectedStart, selectedEnd, parseDate(inputDateString));
                if(btnAll.isClicked(pos)) runAllRoutes(selectedStart, selectedEnd);
                if(btnClear.isClicked(pos)) {
                    resetAlgorithms(); selectedStart = -1; selectedEnd = -1;
                    inputStartName[0] = '\0'; inputEndName[0] = '\0'; 
                    txtStart.setString("Departure: None"); txtEnd.setString("Arrival:     None");
                    strcpy(statusMessage, "Ready."); strcpy(pathDetails, "");
                }
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        hoveredPortIndex = -1;
        if(mPos.x > SIDEBAR_WIDTH) {
            for(int i=0; i<totalPorts; i++) {
                if(abs(mPos.x - ports[i].x) < 15 && abs(mPos.y - ports[i].y) < 15) {
                    hoveredPortIndex = i;
                    break;
                }
            }
        }

        btnCheap.update(mPos); btnFast.update(mPos); btnAll.update(mPos); btnClear.update(mPos);
        dateInput.update(inputDateString, isTypingDate);
        txtMsg.setString(statusMessage); txtRes.setString(pathDetails);

        window.clear(sf::Color(30, 30, 30));
        window.draw(sMap);

        float animTime = animClock.getElapsedTime().asSeconds();
        
        // --- DRAW ALL PATHS ---
        if(showAllPaths) {
            for(int i=0; i<allPathsCount; i++) {
                for(int j=0; j<allPaths[i].count-1; j++) {
                    int u = allPaths[i].stops[j], v = allPaths[i].stops[j+1];
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(ports[u].x, ports[u].y), sf::Color(0, 255, 255, 50)),
                        sf::Vertex(sf::Vector2f(ports[v].x, ports[v].y), sf::Color(0, 255, 255, 50))
                    };
                    window.draw(line, 2, sf::Lines);
                }
            }
        }

        // --- DRAW BEST PATH (Improved UI) ---
        if(pathFound) {
            // Pulsating Green Effect
            uint8_t alpha = 150 + (uint8_t)(105 * sin(animTime * 5.0f));
            sf::Color pathColor(0, 255, 0, alpha); // Green with dynamic alpha

            PathSegment* seg = bestPathLines.head;
            while (seg != nullptr) {
                // 1. Draw Thick Line
                for(float w=-3; w<=3; w+=1.0f) {
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(seg->start.x+w, seg->start.y+w), pathColor),
                        sf::Vertex(sf::Vector2f(seg->end.x+w, seg->end.y+w), pathColor)
                    };
                    window.draw(line, 2, sf::Lines);
                }

                // 2. Draw Moving Particle (Ship Flow)
                float t = fmod(animTime * 1.5f, 1.0f); // 0.0 to 1.0 progress
                sf::Vector2f dotPos = seg->start + (seg->end - seg->start) * t;
                
                sf::CircleShape dot(4);
                dot.setOrigin(4, 4);
                dot.setPosition(dotPos);
                dot.setFillColor(sf::Color::White); // White dot moving on green path
                window.draw(dot);

                seg = seg->next;
            }
        }

        // --- DRAW PINS ---
        for(int i=0; i<totalPorts; i++) {
            sf::Color col = sf::Color::White;
            float scale = 40.0f / tPin.getSize().y;
            
            if(i == selectedStart) { col = sf::Color::Green; scale *= 1.3f; }
            else if(i == selectedEnd) { col = sf::Color::Red; scale *= 1.3f; }
            if(i == hoveredPortIndex) scale *= 1.2f;
            
            if(hasPin) {
                sf::Sprite p(tPin); p.setColor(col); p.setScale(scale, scale);
                p.setOrigin(tPin.getSize().x/2.0f, (float)tPin.getSize().y);
                p.setPosition(ports[i].x, ports[i].y);
                window.draw(p);
            } else {
                sf::CircleShape c(6); c.setFillColor(i==selectedStart ? sf::Color::Green : sf::Color::Red);
                c.setPosition(ports[i].x-6, ports[i].y-6); window.draw(c);
            }

            if(i == hoveredPortIndex) {
                sf::Text t(ports[i].name, font, 16);
                t.setPosition(ports[i].x+10, ports[i].y-30);
                t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(2);
                window.draw(t);
            }
        }

        window.draw(sidebar);
        window.draw(txtTitle); window.draw(txtStart); window.draw(txtEnd);
        window.draw(txtDateLabel); 
        dateInput.draw(window);
        window.draw(txtMsg); window.draw(txtRes);
        btnCheap.draw(window); btnFast.draw(window); btnAll.draw(window); btnClear.draw(window);

        window.display();
    }
}

int main() {
    loadData();
    initCoordinates();
    runGraphics();
    
    if(ports) delete[] ports;
    if(visitedDFS) delete[] visitedDFS;
    bestPathLines.clear();
    return 0;
}