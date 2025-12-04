#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cmath> // For math functions
#include <SFML/Graphics.hpp> 
#include "DataStructures.h" 

using namespace std;

// ==========================================
// 0. Custom Helper Functions (No STL)
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

// Math helper for Hover Detection
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

// App State
int selectedStart = -1;
int selectedEnd = -1;
int hoveredPortIndex = -1; 

char inputStartName[50] = ""; 
char inputEndName[50] = "";   
char inputDateString[20] = "20/12/2024"; 
char statusMessage[100] = "Ready to navigate.";
char pathDetails[100] = ""; 

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
// 2. Journey Data Structure
// ==========================================

struct Journey {
    Route* legs[10]; // Max 10 stops
    int legCount;
    int totalCost;
    int totalTimeMin;
    bool isDirect; // Flag to prioritize display
};

Journey foundJourneys[50];
int foundJourneysCount = 0;
bool showJourneys = false;

// Helpers for DFS - Dynamically allocated later
bool* visited = nullptr; 

// ==========================================
// 3. UI Components
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

// ==========================================
// 4. Data Loading & Mapping
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
}

long long getMinutes(Date d, Time t) {
    return ((long long)d.year*525600) + ((long long)d.month*43200) + ((long long)d.day*1440) + (t.hour*60) + t.minute;
}

Date parseDate(char* s) {
    Date d = {1, 1, 2024};
    int i = 0;
    int val = 0;
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
    if(totalPorts == 0) { cout << "Error: ports.txt empty or missing!" << endl; return; }

    ports = new Port[totalPorts];
    visited = new bool[totalPorts]; 

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
// 5. Time-Aware Search Algorithm
// ==========================================

void findScheduledRoutes(int u, int target, int depth, long long currentArrivalTime, int currentCost, Route* pathSoFar[]) {
    if (depth >= 5 || foundJourneysCount >= 10) return;

    if (u == target) {
        foundJourneys[foundJourneysCount].legCount = depth;
        foundJourneys[foundJourneysCount].totalCost = currentCost;
        foundJourneys[foundJourneysCount].isDirect = (depth == 1);
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
        if (!visited[v]) {
            long long departureTime = getMinutes(r->voyageDate, r->departureTime);
            
            // Check for valid connection (Must depart at or after arrival)
            if (departureTime >= currentArrivalTime) {
                pathSoFar[depth] = r;
                long long arrivalTime = getMinutes(r->voyageDate, r->arrivalTime);
                if (r->arrivalTime.hour < r->departureTime.hour) arrivalTime += 1440; 

                findScheduledRoutes(v, target, depth + 1, arrivalTime, currentCost + r->cost, pathSoFar);
            } 
        }
        r = r->next;
    }
    
    visited[u] = false; 
}

void runSearch(int start, int end, Date userDate) {
    if (start == -1 || end == -1) { strcpy(statusMessage, "Select Start & End first!"); return; }
    
    foundJourneysCount = 0;
    for(int i=0; i<totalPorts; i++) visited[i] = false;
    
    strcpy(statusMessage, "Searching scheduled routes...");
    showJourneys = false;

    Route* tempPath[10];
    Time startTime = {0, 0};
    long long userStartTime = getMinutes(userDate, startTime);
    
    findScheduledRoutes(start, end, 0, userStartTime, 0, tempPath);

    if (foundJourneysCount > 0) {
        // --- NEW: Check if found route is on the SAME date ---
        Route* firstLeg = foundJourneys[0].legs[0];
        
        bool exactDateMatch = (firstLeg->voyageDate.day == userDate.day && 
                               firstLeg->voyageDate.month == userDate.month &&
                               firstLeg->voyageDate.year == userDate.year);

        char dateBuff[20];
        sprintf(dateBuff, "%d/%d/%d", firstLeg->voyageDate.day, firstLeg->voyageDate.month, firstLeg->voyageDate.year);

        if (exactDateMatch) {
            strcpy(statusMessage, "Route Found on Selected Date!");
            strcpy(pathDetails, "Departing: "); strcat(pathDetails, dateBuff);
        } else {
            strcpy(statusMessage, "No ship on selected date.");
            strcpy(pathDetails, "Next available: "); strcat(pathDetails, dateBuff);
        }
        showJourneys = true;
    } else {
        // --- LOGIC FOR NEXT AVAILABLE SHIP ---
        long long minDeparture = -1;
        Route* bestNext = nullptr;
        
        Route* r = ports[start].headRoute;
        while(r != nullptr) {
            long long dep = getMinutes(r->voyageDate, r->departureTime);
            if (dep > userStartTime) {
                if (minDeparture == -1 || dep < minDeparture) {
                    minDeparture = dep;
                    bestNext = r;
                }
            }
            r = r->next;
        }

        if (bestNext != nullptr) {
            strcpy(statusMessage, "No complete route found.");
            strcpy(pathDetails, "Earliest ship: ");
            char dateBuff[20];
            sprintf(dateBuff, "%d/%d/%d", bestNext->voyageDate.day, bestNext->voyageDate.month, bestNext->voyageDate.year);
            strcat(pathDetails, dateBuff);
            strcat(pathDetails, " (");
            strcat(pathDetails, bestNext->company);
            strcat(pathDetails, ")");
        } else {
            strcpy(statusMessage, "No routes found.");
            strcpy(pathDetails, "Try different ports.");
        }
    }
}

// ==========================================
// 6. Visualization
// ==========================================

void runGraphics() {
    sf::RenderWindow window(sf::VideoMode(1536 + (int)SIDEBAR_WIDTH, 1024), "OceanRoute Nav System", sf::Style::Close);
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
    
    Button btnSearch, btnClear;
    btnSearch.init(20, 260, 300, 45, "Find Routes (Date)", font);
    btnClear.init(20, 320, 300, 45, "Reset", font);

    sf::Text txtStart("From: None", font, 18); txtStart.setPosition(20, 80);
    sf::Text txtEnd("To:   None", font, 18); txtEnd.setPosition(20, 120);
    sf::Text txtStatus(statusMessage, font, 16); txtStatus.setPosition(20, 400); txtStatus.setFillColor(sf::Color::Yellow);
    sf::Text txtDetails(pathDetails, font, 18); txtDetails.setPosition(20, 440); txtDetails.setFillColor(sf::Color::Green);

    sf::RectangleShape tooltipBox(sf::Vector2f(280, 120));
    tooltipBox.setFillColor(sf::Color(0, 0, 0, 220));
    tooltipBox.setOutlineThickness(1); tooltipBox.setOutlineColor(sf::Color::White);
    sf::Text tooltipText("", font, 14); tooltipText.setFillColor(sf::Color::White);

    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) window.close();

            if(event.type == sf::Event::TextEntered && isTypingDate) {
                if(event.text.unicode == 8) popBack(inputDateString);
                else if(event.text.unicode < 128) appendChar(inputDateString, (char)event.text.unicode, 20);
            }

            if(event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i pos = sf::Mouse::getPosition(window);
                
                if(dateInput.isClicked(pos)) isTypingDate = true; else isTypingDate = false;

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
                if(btnClear.isClicked(pos)) {
                    selectedStart = -1; selectedEnd = -1; showJourneys = false;
                    strcpy(statusMessage, "Ready."); strcpy(pathDetails, "");
                    txtStart.setString("From: None"); txtEnd.setString("To:   None");
                }
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        btnSearch.update(mPos); btnClear.update(mPos);
        dateInput.update(inputDateString, isTypingDate);
        txtStatus.setString(statusMessage); txtDetails.setString(pathDetails);

        window.clear(sf::Color(30, 30, 30));
        window.draw(sMap);

        bool hoverFound = false;
        if (showJourneys) {
            for(int i=0; i<foundJourneysCount; i++) {
                Journey& j = foundJourneys[i];
                float offset = (i - foundJourneysCount/2.0f) * 4.0f; // Increase separation for clarity

                // Determine color: Gold for Direct, Green for Connecting
                sf::Color pathColor = j.isDirect ? sf::Color(255, 215, 0, 220) : sf::Color(0, 255, 0, 180);

                for(int k=0; k<j.legCount; k++) {
                    Route* r = j.legs[k];
                    
                    // Logic to find source of this leg
                    int u = (k == 0) ? selectedStart : -1;
                    // If not first leg, we need to find which port this route originates from
                    // Since 'r' only knows destination, we infer from previous leg's destination
                    if (k > 0) u = j.legs[k-1]->destinationIndex;

                    int v = r->destinationIndex;

                    if (u != -1) {
                        sf::Vector2f p1(ports[u].x + offset, ports[u].y + offset);
                        sf::Vector2f p2(ports[v].x + offset, ports[v].y + offset);

                        sf::Vertex line[] = {
                            sf::Vertex(p1, pathColor),
                            sf::Vertex(p2, pathColor)
                        };
                        window.draw(line, 2, sf::Lines);

                        float dist = getDistanceToLine(sf::Vector2f(mPos.x, mPos.y), p1, p2);
                        if (dist < 6.0f && !hoverFound) {
                            hoverFound = true;
                            
                            char info[300] = "";
                            if(j.isDirect) strcat(info, "[DIRECT ROUTE]\n");
                            else strcat(info, "[CONNECTING ROUTE]\n");
                            
                            strcat(info, "Co: "); strcat(info, r->company); strcat(info, "\n");
                            char buff[20];
                            strcat(info, "Cost: $"); intToString(r->cost, buff); strcat(info, buff); strcat(info, "\n");
                            
                            strcat(info, "Dep: "); intToString(r->departureTime.hour, buff); strcat(info, buff); strcat(info, ":");
                            if(r->departureTime.minute < 10) strcat(info, "0");
                            intToString(r->departureTime.minute, buff); strcat(info, buff);
                            
                            strcat(info, "\nArr: "); intToString(r->arrivalTime.hour, buff); strcat(info, buff); strcat(info, ":");
                            if(r->arrivalTime.minute < 10) strcat(info, "0");
                            intToString(r->arrivalTime.minute, buff); strcat(info, buff);
                            
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
            if(hasPin) {
                sf::Sprite p(tPin); 
                p.setPosition(ports[i].x, ports[i].y);
                float s = 40.0f/tPin.getSize().y;
                if (i==selectedStart) { p.setColor(sf::Color::Green); s*=1.3f; }
                else if (i==selectedEnd) { p.setColor(sf::Color::Red); s*=1.3f; }
                else p.setColor(sf::Color::White);
                
                p.setScale(s, s);
                p.setOrigin(tPin.getSize().x/2.0f, tPin.getSize().y);
                window.draw(p);
            }
        }

        window.draw(sidebar);
        window.draw(txtStart); window.draw(txtEnd);
        dateInput.draw(window);
        btnSearch.draw(window); btnClear.draw(window);
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
    return 0;
}