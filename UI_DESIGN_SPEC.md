# Route Selection Panel - UI Design

## Panel Layout

```
┌────────────────────────────────────────────────────────────┐
│ SELECT A ROUTE                                          X │
├────────────────────────────────────────────────────────────┤
│                                                            │
│ ┌────────────────────────────────────────────────────────┐│
│ │ Route 1 - Direct                                       ││
│ │ Company: Maersk                                        ││
│ │ Departs: 20/12/2024 06:00 | Arrives: 20/12/2024 18:00 ││
│ │ Duration: 12h 00m | Cost: $350                 [SELECT]││
│ └────────────────────────────────────────────────────────┘│
│                                                            │
│ ┌────────────────────────────────────────────────────────┐│
│ │ Route 2 - 3 Legs                                       ││
│ │                                                        ││
│ │   Leg 1: Karachi -> Dubai                             ││
│ │   Maersk | 06:00 -> 10:00                             ││
│ │                                                        ││
│ │   Leg 2: Dubai -> Jeddah                              ││
│ │   Evergreen | 12:00 -> 18:00                          ││
│ │                                                        ││
│ │   Leg 3: Jeddah -> Istanbul                           ││
│ │   MSC | 20:00 -> 08:00                                ││
│ │                                                        ││
│ │ Total Duration: 26h 00m | Total Cost: $480    [SELECT]││
│ └────────────────────────────────────────────────────────┘│
│                                                            │
│ ┌────────────────────────────────────────────────────────┐│
│ │ Route 3 - 2 Legs                                       ││
│ │                                                        ││
│ │   Leg 1: Karachi -> Mumbai                            ││
│ │   CMA CGM | 06:00 -> 14:00                            ││
│ │                                                        ││
│ │   Leg 2: Mumbai -> Istanbul                           ││
│ │   Hapag-Lloyd | 16:00 -> 10:00                        ││
│ │                                                        ││
│ │ Total Duration: 28h 00m | Total Cost: $420    [SELECT]││
│ └────────────────────────────────────────────────────────┘│
│                                                            │
│              [Book Selected Route]                        │
└────────────────────────────────────────────────────────────┘
```

## Color Scheme

### Route Box (Unselected)
- Background: RGB(30, 35, 45, 200)
- Border: RGB(60, 65, 75) - 1px

### Route Box (Selected)
- Background: RGB(0, 100, 180, 200)
- Border: RGB(0, 200, 255) - 2px

### Panel Background
- Background: RGB(20, 25, 35, 250)
- Border: RGB(0, 180, 255) - 3px
- Overlay: RGB(0, 0, 0, 150) - full screen

### Text Colors
- Title: Accent color (cyan/blue)
- Route Info: White
- Close Button: Red

## Size Specifications

### Panel
- Width: 580px
- Height: 620px
- Position: Centered on screen

### Route Boxes
- Width: 540px (panel width - 40px margins)
- Height: 
  - Direct routes: 85px
  - Multi-leg routes: 95px + (legCount × 35px)
- Spacing between boxes: 10px

### Buttons
- SELECT buttons: 80px × 25px
- Book Selected Route: 200px × 35px
- Close (X) button: 25px × 25px

### Text Sizes
- Panel Title: 16pt, Bold
- Route Info: 10pt
- Close Button: 16pt, Bold

## Route Display Examples

### Direct Route (1 Leg)
```
┌──────────────────────────────────────────────────┐
│ Route 1 - Direct                                 │
│ Company: Maersk                                  │
│ Departs: 20/12/2024 06:00 | Arrives: 18:00      │
│ Duration: 12h 00m | Cost: $350          [SELECT] │
└──────────────────────────────────────────────────┘
Height: 85px
```

### Two-Leg Route
```
┌──────────────────────────────────────────────────┐
│ Route 2 - 2 Legs                                 │
│                                                  │
│   Leg 1: Karachi -> Dubai                        │
│   Maersk | 06:00 -> 10:00                        │
│                                                  │
│   Leg 2: Dubai -> Istanbul                       │
│   MSC | 12:00 -> 08:00                           │
│                                                  │
│ Total Duration: 26h 00m | Total Cost: $480      │
│                                         [SELECT] │
└──────────────────────────────────────────────────┘
Height: 165px (95 + 2×35)
```

### Three-Leg Route
```
┌──────────────────────────────────────────────────┐
│ Route 3 - 3 Legs                                 │
│                                                  │
│   Leg 1: Karachi -> Dubai                        │
│   Maersk | 06:00 -> 10:00                        │
│                                                  │
│   Leg 2: Dubai -> Jeddah                         │
│   Evergreen | 12:00 -> 18:00                     │
│                                                  │
│   Leg 3: Jeddah -> Istanbul                      │
│   MSC | 20:00 -> 08:00                           │
│                                                  │
│ Total Duration: 26h 00m | Total Cost: $520      │
│                                         [SELECT] │
└──────────────────────────────────────────────────┘
Height: 200px (95 + 3×35)
```

## Map Display States

### Before Route Selection
```
Map shows all available routes (up to MAX_ROUTES_TO_DISPLAY)
- Multiple colored lines representing different routes
- All ports highlighted
- Animated route progress indicators
```

### After Route Booking
```
Map shows ONLY the confirmed route
- Single route line (highlighted)
- Only ports in the selected route highlighted
- Single ship animated along the route
- Other route lines are hidden
```

## User Interaction Flow

1. **Initial State**
   - User selects origin port (click on map)
   - User selects destination port (click on map)
   - User enters departure date
   - User clicks "Book Route" button

2. **Panel Appears**
   - Semi-transparent overlay covers map
   - Route selection panel appears centered
   - Up to 3 routes displayed with details
   - All routes are initially unselected

3. **Route Selection**
   - User clicks "SELECT" on desired route
   - Selected route box highlights in blue
   - "Book Selected Route" button activates

4. **Route Booking**
   - User clicks "Book Selected Route"
   - Panel closes
   - **All other route lines disappear from map**
   - Only selected route remains visible
   - Ship spawns and begins simulation

5. **New Search**
   - User clicks "Book Route" again
   - confirmedRouteIndex resets to -1
   - All routes become visible again
   - Process repeats

## Implementation Notes

### Font Sizes
- Character size: 10pt for route info
- Line height: Approximately 12-14px
- 2-line leg info: ~24-28px
- Spacing between legs: ~7-10px

### Button States
- Idle: Default color
- Hover: Lighter color
- Pressed: Darker color
- Disabled: Gray color (for "Book Selected Route" when no selection)

### Panel Positioning
- Horizontal: `MAP_OFFSET_X + (1536 - panelWidth) / 2`
- Vertical: `(1024 - panelHeight) / 2`
- Ensures panel is centered on screen

### Click Detection
- Routes: Check if click is within route box bounds
- Close button: Top-right corner of panel
- Outside panel: Closes panel (cancelled)
- Book button: Only active if route selected
