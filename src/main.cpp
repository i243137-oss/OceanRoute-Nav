#include <iostream>
#include <fstream>
#include <cstring>
#include <SFML/Graphics.hpp> 
#include "DataStructures.h" 

using namespace std;

// ==========================================
// 1. Global Variables
// ==========================================
const int MAX_PORTS = 50;
Port ports[MAX_PORTS];
int totalPorts = 0;

// Selection Variables (For Clicking & Typing)
int selectedStart = -1;
int selectedEnd = -1;
string inputStartName = ""; // User jo type karega
string inputEndName = "";   // User jo type karega
bool isTypingStart = false; // Kya user abhi Start box mein likh raha hai?
bool isTypingEnd = false;   // Kya user abhi End box mein likh raha hai?

// Result String (To show on screen)
string resultTextString = "Select Ports or Type Names to Find Route";

// ==========================================
// 2. Helper Functions
// ==========================================

int getPortIndex(const char* portName) {
    for (int i = 0; i < totalPorts; i++) {
        // Case insensitive compare (simple version)
        if (strcasecmp(ports[i].name, portName) == 0) {
            return i;
        }
    }
    return -1;
}

void setPortPos(const char* name, float x, float y) {
    int idx = getPortIndex(name);
    if (idx != -1) {
        ports[idx].x = x;
        ports[idx].y = y;
    }
}

// === NEW COORDINATES FOR 1536x1024 MAP ===
void initPortCoordinates() {
    // Asia / Middle East
    setPortPos("Karachi", 1015, 460);
    setPortPos("Dubai", 960, 450);
    setPortPos("AbuDhabi", 950, 455);
    setPortPos("Jeddah", 900, 470);
    setPortPos("Doha", 970, 450);
    setPortPos("Mumbai", 1040, 500);
    setPortPos("Colombo", 1060, 560);
    setPortPos("Chittagong", 1120, 470);
    
    // East Asia
    setPortPos("HongKong", 1220, 460);
    setPortPos("Shanghai", 1250, 420);
    setPortPos("Tokyo", 1330, 390);
    setPortPos("Osaka", 1310, 400);
    setPortPos("Busan", 1280, 400);
    setPortPos("Manila", 1250, 520);
    setPortPos("Singapore", 1180, 590);
    setPortPos("Jakarta", 1190, 640);

    // Europe
    setPortPos("London", 735, 290);
    setPortPos("Hamburg", 770, 280);
    setPortPos("Rotterdam", 760, 290);
    setPortPos("Antwerp", 765, 295);
    setPortPos("Marseille", 765, 335);
    setPortPos("Genoa", 780, 330);
    setPortPos("Lisbon", 690, 350);
    setPortPos("Oslo", 780, 240);
    setPortPos("Stockholm", 810, 240);
    setPortPos("Helsinki", 840, 230);
    setPortPos("Copenhagen", 785, 265);
    setPortPos("Athens", 830, 360);
    setPortPos("Istanbul", 850, 350);
    setPortPos("Dublin", 710, 285);

    // Americas
    setPortPos("NewYork", 430, 360);
    setPortPos("Montreal", 420, 330);
    setPortPos("Vancouver", 250, 300);
    setPortPos("LosAngeles", 270, 390);
    
    // Africa
    setPortPos("Alexandria", 860, 380);
    setPortPos("CapeTown", 810, 810);
    setPortPos("Durban", 860, 780);
    setPortPos("PortLouis", 970, 750); 

    // Australia
    setPortPos("Sydney", 1420, 800);
    setPortPos("Melbourne", 1390, 820);
}

long long convertToMinutes(Date d, Time t) {
    long long total = 0;
    total += (long long)d.year * 525600;
    total += (long long)d.month * 43200;
    total += (long long)d.day * 1440;
    total += t.hour * 60;
    total += t.minute;
    return total;
}

// ==========================================
// 3. File Loading Functions
// ==========================================

void loadPorts(const char* filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: " << filename << " open nahi ho saki!" << endl;
        return;
    }
    char tempName[50];
    int tempCharge;
    while (file >> tempName >> tempCharge) {
        if (totalPorts >= MAX_PORTS) break;
        strcpy(ports[totalPorts].name, tempName);
        ports[totalPorts].dailyCharge = tempCharge;
        ports[totalPorts].headRoute = nullptr; 
        totalPorts++;
    }
    file.close();
    cout << "Success: " << totalPorts << " Ports Loaded." << endl;
}

void loadRoutes(const char* filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: " << filename << " open nahi ho saki!" << endl;
        return;
    }
    char originName[50], destName[50], company[50];
    char skip; 
    int cost;
    Date d; Time dep, arr;
    int routesLoaded = 0;

    while (file >> originName >> destName 
                >> d.day >> skip >> d.month >> skip >> d.year 
                >> dep.hour >> skip >> dep.minute 
                >> arr.hour >> skip >> arr.minute 
                >> cost >> company) {
        
        int u = getPortIndex(originName);
        int v = getPortIndex(destName);

        if (u != -1 && v != -1) {
            Route* newRoute = new Route;
            newRoute->destinationIndex = v;
            newRoute->cost = cost;
            strcpy(newRoute->company, company);
            newRoute->voyageDate = d;
            newRoute->departureTime = dep;
            newRoute->arrivalTime = arr;
            newRoute->durationMinutes = 0; 
            newRoute->next = ports[u].headRoute;
            ports[u].headRoute = newRoute;
            routesLoaded++;
        }
    }
    file.close();
    cout << "Success: " << routesLoaded << " Routes Loaded." << endl;
}

// ==========================================
// 4. Algorithm Functions (Modified for GUI)
// ==========================================

void runDijkstra(int start, int end) {
    if (start == -1 || end == -1) {
        resultTextString = "Please select both Start and Destination ports!";
        return;
    }

    MinHeap pq(1000);
    for (int i = 0; i < totalPorts; i++) {
        ports[i].minCost = 99999999;
        ports[i].parentIndex = -1;
        ports[i].visited = false;
    }

    ports[start].minCost = 0;
    pq.push(start, 0);

    resultTextString = "Calculating...";

    while (!pq.isEmpty()) {
        HeapNode current = pq.extractMin();
        int u = current.portIndex;

        if (ports[u].visited) continue;
        ports[u].visited = true;

        if (u == end) break; 

        Route* edge = ports[u].headRoute;
        while (edge != nullptr) {
            int v = edge->destinationIndex;
            int voyageCost = edge->cost;

            if (!ports[v].visited && (ports[u].minCost + voyageCost < ports[v].minCost)) {
                ports[v].minCost = ports[u].minCost + voyageCost;
                ports[v].parentIndex = u;
                pq.push(v, ports[v].minCost);
            }
            edge = edge->next;
        }
    }

    if (ports[end].minCost != 99999999) {
        resultTextString = "Cheapest Path Found!\nTotal Cost: $" + to_string(ports[end].minCost);
        // Build path string for display? Can be added later.
    } else {
        resultTextString = "No valid path found between selected ports.";
    }
}

// ==========================================
// 5. Visualization
// ==========================================

void visualizeSystem() {
    sf::RenderWindow window(sf::VideoMode(1536, 1024), "OceanRoute Nav - Visualization");

    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("map.png")) {
        cout << "Error: Map image (map.png) not found!" << endl;
    }
    sf::Sprite mapSprite(mapTexture);
    
    float scaleX = 1536.0f / mapTexture.getSize().x;
    float scaleY = 1024.0f / mapTexture.getSize().y;
    mapSprite.setScale(scaleX, scaleY);

    sf::Texture pinTexture;
    bool usePin = false;
    if (pinTexture.loadFromFile("pin.png")) {
        usePin = true;
        pinTexture.setSmooth(true);
    }

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) { 
        cout << "Warning: Font not found." << endl;
    }

    // === GUI ELEMENTS ===
    // Background Box for Menu (Bottom Left)
    sf::RectangleShape menuBox(sf::Vector2f(400, 250));
    menuBox.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent Black
    menuBox.setOutlineColor(sf::Color::White);
    menuBox.setOutlineThickness(2);
    menuBox.setPosition(20, 750); // Bottom Left position

    // Labels
    sf::Text titleLabel("Route Finder Menu", font, 24);
    titleLabel.setPosition(35, 760);
    titleLabel.setFillColor(sf::Color::Yellow);

    sf::Text startLabel("Departure:", font, 18);
    startLabel.setPosition(35, 800);
    
    sf::Text endLabel("Arrival:", font, 18);
    endLabel.setPosition(35, 840);

    // Input Boxes (Visual Only)
    sf::RectangleShape startInputBox(sf::Vector2f(200, 30));
    startInputBox.setPosition(140, 800);
    startInputBox.setFillColor(sf::Color::White);

    sf::RectangleShape endInputBox(sf::Vector2f(200, 30));
    endInputBox.setPosition(140, 840);
    endInputBox.setFillColor(sf::Color::White);

    // Input Texts (Dynamic)
    sf::Text startInputText("", font, 18);
    startInputText.setFillColor(sf::Color::Black);
    startInputText.setPosition(145, 805);

    sf::Text endInputText("", font, 18);
    endInputText.setFillColor(sf::Color::Black);
    endInputText.setPosition(145, 845);

    // Find Button
    sf::RectangleShape findButton(sf::Vector2f(150, 40));
    findButton.setPosition(140, 890);
    findButton.setFillColor(sf::Color(0, 200, 0)); // Green

    sf::Text findButtonText("FIND SHIP", font, 20);
    findButtonText.setFillColor(sf::Color::Black);
    findButtonText.setPosition(165, 898);

    // Result Text Area
    sf::Text resultDisplay(resultTextString, font, 16);
    resultDisplay.setFillColor(sf::Color::White);
    resultDisplay.setPosition(35, 940);


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // === MOUSE CLICK HANDLING ===
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = {event.mouseButton.x, event.mouseButton.y};
                
                // 1. Check Input Boxes Click
                if (startInputBox.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y)) {
                    isTypingStart = true; isTypingEnd = false;
                    startInputBox.setOutlineColor(sf::Color::Blue); startInputBox.setOutlineThickness(2);
                    endInputBox.setOutlineThickness(0);
                }
                else if (endInputBox.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y)) {
                    isTypingStart = false; isTypingEnd = true;
                    endInputBox.setOutlineColor(sf::Color::Blue); endInputBox.setOutlineThickness(2);
                    startInputBox.setOutlineThickness(0);
                }
                // 2. Check Find Button Click
                else if (findButton.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y)) {
                    // Try to find indices from typed names
                    int sIdx = getPortIndex(inputStartName.c_str());
                    int eIdx = getPortIndex(inputEndName.c_str());
                    
                    if (sIdx != -1) selectedStart = sIdx;
                    if (eIdx != -1) selectedEnd = eIdx;

                    if (selectedStart != -1 && selectedEnd != -1) {
                        runDijkstra(selectedStart, selectedEnd);
                    } else {
                        resultTextString = "Invalid Port Names! Please check spelling.";
                    }
                }
                // 3. Check Map Pins Click
                else {
                    isTypingStart = false; isTypingEnd = false; // Unfocus inputs
                    startInputBox.setOutlineThickness(0); endInputBox.setOutlineThickness(0);

                    for (int i = 0; i < totalPorts; i++) {
                        float px = ports[i].x;
                        float py = ports[i].y;
                        if (mousePos.x >= px - 15 && mousePos.x <= px + 15 &&
                            mousePos.y >= py - 15 && mousePos.y <= py + 15) {
                            
                            if (event.mouseButton.button == sf::Mouse::Left) {
                                selectedStart = i;
                                inputStartName = ports[i].name; // Auto-fill text
                            } 
                            else if (event.mouseButton.button == sf::Mouse::Right) {
                                selectedEnd = i;
                                inputEndName = ports[i].name; // Auto-fill text
                            }
                            break;
                        }
                    }
                }
            }

            // === KEYBOARD TYPING HANDLING ===
            if (event.type == sf::Event::TextEntered) {
                if (isTypingStart || isTypingEnd) {
                    sf::Uint32 unicode = event.text.unicode;
                    string* targetString = isTypingStart ? &inputStartName : &inputEndName;

                    if (unicode == 8) { // Backspace
                        if (!targetString->empty()) targetString->pop_back();
                    }
                    else if (unicode < 128) { // Standard ASCII
                        *targetString += static_cast<char>(unicode);
                    }
                }
            }
        }

        window.clear();
        window.draw(mapSprite);

        // Update Dynamic Texts
        startInputText.setString(inputStartName);
        endInputText.setString(inputEndName);
        resultDisplay.setString(resultTextString);

        // Draw GUI
        window.draw(menuBox);
        window.draw(titleLabel);
        window.draw(startLabel); window.draw(endLabel);
        window.draw(startInputBox); window.draw(endInputBox);
        window.draw(startInputText); window.draw(endInputText);
        window.draw(findButton); window.draw(findButtonText);
        window.draw(resultDisplay);

        // A. Draw Computed Path
        if (selectedStart != -1 && selectedEnd != -1 && ports[selectedEnd].minCost != 99999999) {
            int curr = selectedEnd;
            while (curr != selectedStart && curr != -1) {
                int parent = ports[curr].parentIndex;
                if (parent != -1) {
                    for(float offset = -1.5f; offset <= 1.5f; offset += 0.5f) {
                        sf::Vertex thickLine[] = {
                            sf::Vertex(sf::Vector2f(ports[curr].x + offset, ports[curr].y + offset), sf::Color(0, 255, 0, 255)),
                            sf::Vertex(sf::Vector2f(ports[parent].x + offset, ports[parent].y + offset), sf::Color(0, 255, 0, 255))
                        };
                        window.draw(thickLine, 2, sf::Lines);
                    }
                }
                curr = parent;
            }
        }

        // B. Draw Hover Routes & Pins (Same as before)
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        int hoveredPortIndex = -1;

        for (int i = 0; i < totalPorts; i++) {
            float px = ports[i].x;
            float py = ports[i].y;
            if (mousePos.x >= px - 15 && mousePos.x <= px + 15 &&
                mousePos.y >= py - 15 && mousePos.y <= py + 15) {
                hoveredPortIndex = i;
            }
        }

        if (hoveredPortIndex != -1) {
            Route* current = ports[hoveredPortIndex].headRoute;
            while (current != nullptr) {
                int destIndex = current->destinationIndex;
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(ports[hoveredPortIndex].x, ports[hoveredPortIndex].y), sf::Color(255, 255, 0, 150)),
                    sf::Vertex(sf::Vector2f(ports[destIndex].x, ports[destIndex].y), sf::Color(255, 255, 255, 50))
                };
                window.draw(line, 2, sf::Lines);
                current = current->next;
            }
        }

        for (int i = 0; i < totalPorts; i++) {
            sf::Color pinColor = sf::Color::White; 
            float scale = 40.0f / pinTexture.getSize().y;

            if (i == selectedStart) { pinColor = sf::Color::Green; scale *= 1.2f; }
            else if (i == selectedEnd) { pinColor = sf::Color::Red; scale *= 1.2f; }
            if (i == hoveredPortIndex) scale *= 1.2f;

            if (usePin) {
                sf::Sprite pin(pinTexture);
                pin.setColor(pinColor);
                pin.setScale(scale, scale);
                pin.setOrigin((float)pinTexture.getSize().x / 2.0f, (float)pinTexture.getSize().y);
                pin.setPosition(ports[i].x, ports[i].y);
                window.draw(pin);
            } else {
                sf::CircleShape dot(5);
                dot.setFillColor((i==selectedStart) ? sf::Color::Green : (i==selectedEnd ? sf::Color::Red : sf::Color::Red));
                dot.setPosition(ports[i].x-5, ports[i].y-5);
                window.draw(dot);
            }

            if (i == hoveredPortIndex || i == selectedStart || i == selectedEnd) {
                sf::Text text(ports[i].name, font, 16);
                text.setFillColor(sf::Color::White);
                text.setPosition(ports[i].x + 10, ports[i].y - 30);
                text.setOutlineColor(sf::Color::Black);
                text.setOutlineThickness(2);
                window.draw(text);
            }
        }
        window.display();
    }
}

int main() {
    loadPorts("ports.txt"); 
    loadRoutes("Routes.txt");
    initPortCoordinates();

    cout << "Launching Interactive System..." << endl;
    visualizeSystem();

    return 0;
}