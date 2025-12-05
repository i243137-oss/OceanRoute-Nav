# Interactive Route Selection Feature

## Overview
This document describes the implementation of the interactive route selection feature for the OceanRoute Nav application. This feature allows users to view and select specific routes before booking, replacing the previous "Book Route (All)" behavior that spawned all available routes simultaneously.

## Changes Made

### 1. Global Variables Added (lines 128-130)
```cpp
// Route Selection Panel State
bool showRouteSelectionPanel = false;
int selectedRouteIndex = -1;
```
- `showRouteSelectionPanel`: Controls visibility of the route selection overlay panel
- `selectedRouteIndex`: Tracks which route the user has selected (-1 means no selection)

### 2. Helper Functions Added (lines 786-825)
Four new helper functions to support route information display:

#### `calculateVoyageDuration(Journey& journey)`
- Calculates total voyage duration in minutes for a journey
- Handles day boundary crossings (when arrival time is next day)
- Returns 0 for invalid journeys

#### `formatDuration(int minutes, char* buffer, int bufferSize)`
- Formats duration as human-readable string (e.g., "14h 30m")
- Takes minutes as input and outputs formatted string

#### `getJourneyCompanies(Journey& journey, char* buffer, int bufferSize)`
- Extracts all shipping companies for a multi-leg journey
- Returns comma-separated list of companies (e.g., "Maersk, MSC")
- Handles single and multiple leg journeys

#### `formatDateTime(Date d, Time t, char* buffer, int bufferSize)`
- Formats date and time for display (e.g., "20/12/2024 06:00")
- Consistent format across departure and arrival times

### 3. UI Components Added (lines 1777-1800)
New UI components for the route selection panel:

```cpp
Button btnSelectRoute[10];      // Up to 10 route selection buttons
Button btnBookSelected;          // "Book Selected Route" button
Button btnCancelRoutePanel;      // Close/Cancel button
sf::Text txtRouteInfo[10];       // Route information display
sf::Text txtRoutePanelTitle;     // Panel title "SELECT A ROUTE"
sf::Text txtRoutePanelClose;     // Close button "X"
```

### 4. Modified "Book Route" Button Behavior (lines 2073-2082)
Changed from immediately spawning all ships to showing the selection panel:

**Before:**
- Spawned all found routes immediately
- No user interaction or selection

**After:**
- Opens route selection panel
- Allows user to review and select specific route
- Sets status message to guide user

### 5. Route Selection Panel Click Handlers (lines 1971-2064)
Comprehensive click handling for the route selection panel:

- **Route Selection**: Click SELECT button on any route to highlight it
- **Book Selected Route**: Spawns single ship for selected route and initializes simulation time
- **Cancel/Close**: Close button or clicking outside panel cancels selection
- **Panel Isolation**: Prevents other UI interactions while panel is open

### 6. Route Selection Panel Rendering (lines 2730-2849)
Full-screen overlay panel with route information display:

**Panel Layout:**
- Semi-transparent dark overlay (dims background)
- Centered panel (550x550px) with blue outline
- Title: "SELECT A ROUTE" at top
- Close button (X) in top-right corner
- Up to 5 routes visible simultaneously
- Each route shows:
  - Route number (1, 2, 3, etc.)
  - Direct or number of legs
  - Company name(s)
  - Departure date/time
  - Arrival date/time
  - Duration (hours and minutes)
  - Total cost ($)
  - SELECT button

**Visual Feedback:**
- Selected route highlighted with blue background
- Hover effects on buttons
- Disabled "Book Selected Route" button when no selection (gray color)
- Prompt text when no route selected

### 7. Button State Updates (lines 2216-2223)
Added update calls for route selection panel buttons:
- Updates hover/press states for all route selection buttons
- Only updates when panel is visible
- Includes all SELECT buttons, Book Selected, and Cancel buttons

## User Flow

1. **User selects origin and destination ports** (left/right click on map)
2. **User enters departure date** in the date input field
3. **User clicks "Book Route" button**
4. **Route selection panel appears** showing all available routes
5. **User reviews routes** with detailed information for each
6. **User clicks SELECT on preferred route** (route highlights in blue)
7. **User clicks "Book Selected Route"**
8. **Panel closes, single ship spawns** for the selected route
9. **Simulation begins** with the selected route's departure time

## Key Features

### Interactive Selection
- Visual feedback on hover and selection
- Clear highlighting of selected route
- Disabled state when no selection made

### Comprehensive Route Information
Each route displays:
- Route identification (number and type)
- Shipping company names
- Complete schedule (departure and arrival)
- Voyage duration
- Total cost

### Flexible Cancellation
- Close button (X)
- Click outside panel
- Both methods cancel and close panel

### Single Route Simulation
- Only selected route spawns a ship
- Eliminates confusion from multiple simultaneous ships
- Easier to track specific journey

## Technical Implementation Details

### Memory Management
- Route selection buttons initialized on demand
- Text elements reused for each route display
- No additional heap allocations required

### Panel Positioning
- Centered dynamically based on window size
- Properly positioned over map area
- Responsive to different screen layouts

### State Management
- Panel visibility controlled by boolean flag
- Selection tracked by index
- Clean reset when panel closes

### Integration
- Seamlessly integrates with existing UI
- Uses existing color scheme and styling
- Consistent with preferences panel design
- Works with existing simulation system

## Benefits

1. **User Control**: Users can review all options before committing
2. **Clarity**: Detailed information helps users make informed decisions
3. **Simplicity**: Single ship simulation is easier to follow
4. **Consistency**: Matches modern UI/UX patterns
5. **Flexibility**: Easy to cancel or change selection

## Future Enhancements (Optional)

While not implemented in this version, potential improvements could include:

1. **Scrolling Support**: For more than 5 routes, add scroll functionality
2. **Sorting Options**: Sort by cost, duration, or departure time
3. **Filtering**: Filter by company or number of legs
4. **Route Comparison**: Side-by-side comparison of multiple routes
5. **Favorites**: Save preferred routes for later booking
6. **Search**: Quick search/filter within available routes

## Testing Notes

The implementation compiles successfully with no errors or warnings. The feature should be tested by:

1. Selecting origin and destination ports
2. Clicking "Book Route" button
3. Verifying panel appears with route information
4. Testing route selection (clicking SELECT buttons)
5. Testing "Book Selected Route" button
6. Testing cancel/close functionality
7. Verifying single ship spawns and simulation starts correctly
8. Testing with various numbers of routes (1, 2, 5, 10+)
9. Testing with direct and multi-leg routes
10. Verifying proper date/time initialization

## Compatibility

- Compatible with existing preference system
- Works with current simulation engine
- No breaking changes to existing functionality
- Maintains STL-free design requirement
