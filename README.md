# ⚓ OceanRoute Nav

### Maritime Navigation Optimizer & Logistics Visualization System

![OceanRoute Nav](./demo.gif)

> **Optimizing maritime logistics through intelligent pathfinding and real-time visualization.**

**OceanRoute Nav** is a high-performance C++ application designed to map, optimize, and visualize complex cargo ship routes between international ports. Built from scratch without standard library containers, it demonstrates advanced data structure implementation and graphical simulation using SFML.

---

## 🌊 Project Overview

This system assists logistics managers and captains in selecting optimal sea routes by balancing **travel time**, **fuel cost**, and **weather conditions**. It handles the complexities of port docking schedules, layovers, and cargo transfers dynamically.

### 🚀 Key Features

| Feature | Data Structure Implemented | Description |
| :--- | :--- | :--- |
| **Global Route Mapping** | **Graph (Adjacency List)** | Ports are vertices, sea routes are weighted edges (cost, duration, company). |
| **Optimal Pathfinding** | **Min-Priority Queue (Heap)** | Custom Dijkstra & A* algorithms to find the Shortest (Time) or Cheapest (Cost) paths. |
| **Voyage Booking** | **Linked List** | Dynamic multi-leg journeys (e.g., Karachi → Dubai → Athens). Easy insertion/deletion of stops. |
| **Port Traffic Control** | **Queue (FIFO)** | Manages docking ships. First-come-first-serve handling for layovers and cargo delays. |
| **Custom Filtering** | **Subgraph Generation** | Filter views by Shipping Company (e.g., Maersk) or Weather conditions. |

---

## 🛠️ Tech Stack

*   **Language:** C++ (Strictly no STL Containers: `std::vector`, `std::map` etc. prohibited)
*   **Graphics Engine:** SFML (Simple and Fast Multimedia Library)
*   **Build System:** CMake

---

## 🎮 Interactive Visualization

OceanRoute Nav isn't just a calculator; it's a visual tool:
*   **Real-time Exploration:** Watch the pathfinding algorithms (Dijkstra/A*) light up nodes as they search.
*   **Dynamic Queues:** See ships lineup at busy ports with visual indicators for wait times.
*   **Route Animation:** Highlighted paths show the journey sequence with directional animations.
*   **Interactive Map:** Hover over routes for fuel cost, duration, and company details.

---

## ⚙️ Setup & Build

### Prerequisites
*   C++ Compiler (supporting C++11 or higher)
*   CMake
*   SFML library (installed and linked)

### Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/i243137-oss/OceanRoute-Nav.git
    cd OceanRoute-Nav
    ```

2.  **Build using CMake:**
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```

3.  **Run the Application:**
    *   Ensure `Routes.txt`, `PortCharges.txt`, and the `assets/` (fonts/images) are in the executable's directory or correctly referenced.
    ```bash
    ./OceanRouteNav
    ```

---

## 📂 Data Configuration

The system relies on two primary data files located in the root directory:

### `Routes.txt`
Defines the connections between ports. Format:
```text
Origin_Port Destination_Port Date Departure_Time Arrival_Time Cost($) Company_Name
```
*Note: The system automatically calculates layovers based on Arrival vs. next Departure times.*

### `PortCharges.txt`
Defines the daily docking fees. Used to calculate total cost when layovers exceed 12 hours.

---

## 👥 Team & Contribution

**Project Type:** Academic Group Project (Fall 2025)
**Constraints:**
*   Custom implementation of all Data Structures (Lists, Trees, Graphs).
*   Modular design separating logic, data, and visualization.

*Contributions are welcome! Please open a PR with a screenshot of your changes.*

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.