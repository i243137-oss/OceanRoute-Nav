# Visual Changes Summary

## Screenshots Would Show (Cannot Generate on Linux)

Since this is a Windows-only GUI application and we're in a Linux environment, actual screenshots cannot be taken. However, here's what the visual changes would look like when tested on Windows:

## 1. Route Selection Panel - Direct Route

### What You Would See:
```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  [Dark semi-transparent overlay covering the entire map]   │
│                                                             │
│    ┌───────────────────────────────────────────────────┐   │
│    │ SELECT A ROUTE                               X    │   │
│    ├───────────────────────────────────────────────────┤   │
│    │                                                   │   │
│    │  ╔═══════════════════════════════════════════╗   │   │
│    │  ║ Route 1 - Direct                          ║   │   │
│    │  ║ Company: Maersk                           ║   │   │
│    │  ║ Departs: 20/12/2024 06:00 | Arrives: 18:00║   │   │
│    │  ║ Duration: 12h 00m | Cost: $350   [SELECT]║   │   │
│    │  ╚═══════════════════════════════════════════╝   │   │
│    │                                                   │   │
│    │              [Book Selected Route]                │   │
│    │                   (disabled)                      │   │
│    └───────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Key Visual Elements:
- **Background**: World map with ports and routes visible but dimmed by overlay
- **Overlay**: Semi-transparent black (opacity ~60%)
- **Panel**: 580×620 pixels, centered on screen
  - Background: Very dark blue-gray (RGB 20, 25, 35, opacity 98%)
  - Border: Bright cyan/blue (RGB 0, 180, 255), 3px thick
- **Title**: "SELECT A ROUTE" in cyan, 16pt, bold
- **Close Button**: Red "X" in top-right corner, 16pt, bold
- **Route Box**: 
  - Background: Dark gray (RGB 30, 35, 45)
  - Border: Light gray (RGB 60, 65, 75), 1px
  - Height: 85px (direct routes)
  - Text: White, 10pt
- **SELECT Button**: 
  - Position: Right side of route box
  - Size: 80×25 pixels
  - Default style (blue when hovered)

## 2. Route Selection Panel - Multi-Leg Route

### What You Would See:
```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  [Dark semi-transparent overlay covering the entire map]   │
│                                                             │
│    ┌───────────────────────────────────────────────────┐   │
│    │ SELECT A ROUTE                               X    │   │
│    ├───────────────────────────────────────────────────┤   │
│    │                                                   │   │
│    │  ╔═══════════════════════════════════════════╗   │   │
│    │  ║ Route 1 - 3 Legs                          ║   │   │
│    │  ║                                            ║   │   │
│    │  ║   Leg 1: Karachi -> Dubai                 ║   │   │
│    │  ║   Maersk | 06:00 -> 10:00                 ║   │   │
│    │  ║                                            ║   │   │
│    │  ║   Leg 2: Dubai -> Jeddah                  ║   │   │
│    │  ║   Evergreen | 12:00 -> 18:00              ║   │   │
│    │  ║                                            ║   │   │
│    │  ║   Leg 3: Jeddah -> Istanbul               ║   │   │
│    │  ║   MSC | 20:00 -> 08:00                    ║   │   │
│    │  ║                                            ║   │   │
│    │  ║ Total Duration: 26h 00m | Cost: $480     ║   │   │
│    │  ║                              [SELECT]     ║   │   │
│    │  ╚═══════════════════════════════════════════╝   │   │
│    │                                                   │   │
│    │              [Book Selected Route]                │   │
│    │                   (disabled)                      │   │
│    └───────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Key Visual Elements:
- **Route Box Height**: Dynamic (200px for 3-leg route)
- **Leg Information**: 
  - Indented by 2 spaces
  - Two lines per leg:
    - Line 1: "Leg X: Origin -> Destination"
    - Line 2: "Company | HH:MM -> HH:MM"
  - Spacing between legs
- **Summary Line**: 
  - Separated by blank line
  - Shows total duration and cost
  - Bold or slightly larger text

## 3. Selected Route (Highlighted)

### What You Would See:
```
    │  ╔═══════════════════════════════════════════╗   │
    │  ║ Route 2 - Direct                ← SELECTED║   │
    │  ║ Company: MSC                              ║   │
    │  ║ Departs: 20/12/2024 08:00 | Arrives: 20:00║  │
    │  ║ Duration: 12h 00m | Cost: $320   [SELECT]║   │
    │  ╚═══════════════════════════════════════════╝   │
```

### Visual Changes When Selected:
- **Background**: Changes to darker blue (RGB 0, 100, 180)
- **Border**: Changes to bright cyan (RGB 0, 200, 255), 2px thick
- **Border Glow**: Subtle glow effect
- **"Book Selected Route" Button**: 
  - Becomes enabled (active blue color)
  - Text clearly visible
  - Clickable

## 4. Map View - Before Booking

### What You Would See:
- Multiple colored route lines on the map:
  - Route 1: Gold/yellow line
  - Route 2: Green line
  - Route 3: Blue line
  - Route 4: Purple line
  - Route 5: Orange line
- All routes visible simultaneously
- Animated "progress" effect cycling through legs
- All relevant ports highlighted

## 5. Map View - After Booking

### What You Would See:
- **ONLY the selected route line is visible**
- All other route lines are **completely hidden**
- Selected route is highlighted (brighter color)
- Only ports in the selected route are highlighted
- Single ship icon at the origin port
- Ship begins animating along the selected route
- Status bar shows "Route booked!"

### Key Difference:
```
BEFORE BOOKING:              AFTER BOOKING:
  Route 1 ═══════╗             [hidden]
  Route 2 ───────╫───╗         [hidden]
  Route 3 ═══════╬═══╫═══╗     Route 3 ═══════════════╗ ← ONLY THIS
  Route 4 ───────╫───╫───╫───╗ [hidden]
  Route 5 ═══════╩═══╩═══╩═══╝ [hidden]
                                
  Ship positions:               Single ship:
  🚢 🚢 🚢 🚢 🚢                 🚢
```

## 6. Multiple Routes Displayed

### What You Would See with 3 Routes:
```
┌───────────────────────────────────────────────────┐
│ SELECT A ROUTE                               X    │
├───────────────────────────────────────────────────┤
│                                                   │
│  ╔═══════════════════════════════════════════╗   │
│  ║ Route 1 - Direct              [SELECT]    ║   │
│  ║ ... (direct route info)                   ║   │
│  ╚═══════════════════════════════════════════╝   │
│                                                   │
│  ╔═══════════════════════════════════════════╗   │
│  ║ Route 2 - 2 Legs              [SELECT]    ║   │
│  ║ ... (leg details)                         ║   │
│  ║ ... (leg details)                         ║   │
│  ║ Total Duration: ... | Cost: ...          ║   │
│  ╚═══════════════════════════════════════════╝   │
│                                                   │
│  ╔═══════════════════════════════════════════╗   │
│  ║ Route 3 - 3 Legs              [SELECT]    ║   │
│  ║ ... (leg details)                         ║   │
│  ║ ... (leg details)                         ║   │
│  ║ ... (leg details)                         ║   │
│  ║ Total Duration: ... | Cost: ...          ║   │
│  ╚═══════════════════════════════════════════╝   │
│                                                   │
│              [Book Selected Route]                │
│                   (disabled)                      │
└───────────────────────────────────────────────────┘
```

### Visual Spacing:
- 10px space between route boxes
- Route boxes stack vertically
- Maximum 3 visible at once (to fit in 620px panel height)
- Scrollable if more than 3 routes (future enhancement)

## Color Palette

### Panel Colors:
- **Overlay**: `RGB(0, 0, 0)` with alpha `150` (semi-transparent black)
- **Panel Background**: `RGB(20, 25, 35)` with alpha `250`
- **Panel Border**: `RGB(0, 180, 255)` - bright cyan

### Route Box Colors:
- **Unselected Background**: `RGB(30, 35, 45)` with alpha `200`
- **Unselected Border**: `RGB(60, 65, 75)` - 1px
- **Selected Background**: `RGB(0, 100, 180)` with alpha `200`
- **Selected Border**: `RGB(0, 200, 255)` - 2px (bright cyan)

### Text Colors:
- **Title**: Cyan (RGB 0, 180, 255) - matches accent color
- **Route Info**: White (RGB 255, 255, 255)
- **Close Button**: Red (RGB 255, 0, 0)
- **Disabled Text**: Gray (muted white)

### Button Colors:
- **Default**: Blue-gray
- **Hover**: Lighter blue
- **Pressed**: Darker blue
- **Disabled**: Gray

## Font Specifications

- **Panel Title**: Arial, 16pt, Bold
- **Route Information**: Arial, 10pt, Regular
- **Leg Details**: Arial, 10pt, Regular
- **Close Button**: Arial, 16pt, Bold
- **Button Text**: Arial, 11pt, Regular

## Animation Effects

### Route Lines on Map:
- **Before Booking**: Pulsing animation cycles through each leg
- **After Booking**: Single route highlighted, no pulsing on other routes
- **Leg Progression**: Active leg is brighter (alpha 255), completed legs dimmer (alpha 150)

### Ship Animation:
- Single ship icon moves along the route
- Smooth interpolation between ports
- Pauses at intermediate ports (multi-leg routes)
- Speed adjustable with simulation controls

## Responsive Behavior

### Route Box Height Adjustment:
```
Direct Route (1 leg):    85px   ┌──────┐
                                │      │
                                │      │
                                └──────┘

2-Leg Route:           165px   ┌──────┐
                                │      │
                                │      │
                                │      │
                                │      │
                                └──────┘

3-Leg Route:           200px   ┌──────┐
                                │      │
                                │      │
                                │      │
                                │      │
                                │      │
                                └──────┘

5-Leg Route:           270px   ┌──────┐
                                │      │
                                │      │
                                │      │
                                │      │
                                │      │
                                │      │
                                │      │
                                └──────┘
```

## User Interaction Visual Feedback

1. **Hover over SELECT button**: Button highlights
2. **Click SELECT button**: Route box background changes to blue
3. **Selected route**: Border becomes thick and bright cyan
4. **Book Selected Route enabled**: Button color changes from gray to active blue
5. **Click Book**: Panel fades out, map shows only selected route

## Testing Visual Checklist

When testing on Windows, verify:
- [ ] Panel appears centered on screen
- [ ] Panel has correct dimensions (580×620px)
- [ ] Dark overlay covers map but map is still visible underneath
- [ ] Title "SELECT A ROUTE" is cyan and bold
- [ ] Close button (X) is red and in top-right corner
- [ ] Route boxes have correct heights based on leg count
- [ ] Text is white and readable on dark background
- [ ] Leg details are properly indented and formatted
- [ ] Arrow symbol (→) displays correctly between port names
- [ ] Selected route highlights in blue
- [ ] "Book Selected Route" button state changes correctly
- [ ] After booking, only selected route visible on map
- [ ] No other route lines visible after booking
- [ ] Ship appears and animates on selected route

---

**Note**: These visual descriptions represent the expected appearance based on the code implementation. Actual screenshots must be taken on a Windows system where the application can run.
