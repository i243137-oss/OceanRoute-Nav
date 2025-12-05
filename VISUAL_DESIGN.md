# Route Selection Panel - Visual Design Specification

## Panel Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Semi-Transparent Dark Overlay                    │
│                                                                     │
│    ┌─────────────────────────────────────────────────────────┐    │
│    │  SELECT A ROUTE                                      X  │    │
│    │─────────────────────────────────────────────────────────│    │
│    │                                                         │    │
│    │  ┌───────────────────────────────────────────────────┐ │    │
│    │  │ Route 1 - Direct                                  │ │    │
│    │  │ Company: Maersk                                   │ │    │
│    │  │ Departs: 20/12/2024 06:00 | Arrives: 20/12/2024  │ │    │
│    │  │ Duration: 12h 00m | Cost: $350                    │ │    │
│    │  │                                        [SELECT]    │ │    │
│    │  └───────────────────────────────────────────────────┘ │    │
│    │                                                         │    │
│    │  ┌───────────────────────────────────────────────────┐ │    │
│    │  │ Route 2 - 2 Legs                                  │ │    │
│    │  │ Company: MSC, Evergreen                           │ │    │
│    │  │ Departs: 20/12/2024 08:00 | Arrives: 21/12/2024  │ │    │
│    │  │ Duration: 18h 00m | Cost: $280                    │ │    │
│    │  │                                        [SELECT]    │ │    │
│    │  └───────────────────────────────────────────────────┘ │    │
│    │                                                         │    │
│    │  ┌───────────────────────────────────────────────────┐ │    │
│    │  │ Route 3 - 3 Legs                                  │ │    │
│    │  │ Company: Maersk, CMA CGM, Hapag-Lloyd             │ │    │
│    │  │ Departs: 20/12/2024 10:00 | Arrives: 22/12/2024  │ │    │
│    │  │ Duration: 30h 00m | Cost: $250                    │ │    │
│    │  │                                        [SELECT]    │ │    │
│    │  └───────────────────────────────────────────────────┘ │    │
│    │                                                         │    │
│    │                Select a route to book                   │    │
│    │               [  Book Selected Route  ]                 │    │
│    └─────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## Color Scheme

### Panel Colors
- **Background**: RGB(20, 25, 35, 250) - Dark blue-gray with high opacity
- **Outline**: RGB(0, 180, 255) - Bright cyan (3px thickness)
- **Overlay**: RGB(0, 0, 0, 150) - Semi-transparent black

### Route Box Colors (Unselected)
- **Background**: RGB(30, 35, 45, 200) - Slightly lighter dark blue-gray
- **Outline**: RGB(60, 65, 75) - Muted gray (1px thickness)

### Route Box Colors (Selected)
- **Background**: RGB(0, 100, 180, 200) - Blue with transparency
- **Outline**: RGB(0, 200, 255) - Bright cyan (2px thickness)

### Text Colors
- **Title**: RGB(0, 180, 255) - Cyan, Bold, 16pt
- **Route Info**: RGB(240, 240, 240) - White, 10pt
- **Prompt Text**: RGB(180, 180, 180) - Muted gray, 11pt
- **Close Button**: RGB(255, 0, 0) - Red, Bold, 16pt

### Button Colors
- **SELECT Button (Idle)**: RGB(50, 50, 60)
- **SELECT Button (Hover)**: RGB(0, 200, 255)
- **Book Button (Active)**: Same as SELECT
- **Book Button (Disabled)**: RGB(80, 80, 80) - `COL_BTN_DISABLED`

## Panel Dimensions

### Overall Panel
- **Width**: 550px
- **Height**: 550px
- **Position**: Centered on map area (horizontally and vertically)

### Route Box
- **Width**: Panel width - 40px = 510px
- **Height**: 85px per route
- **Spacing**: 10px between route boxes
- **Padding**: 10px internal padding for text

### Buttons
- **SELECT Button**: 80px × 25px
- **Book Selected Button**: 200px × 35px
- **Close Button (X)**: 25px × 25px (clickable area)

### Text Layout
- **Title**: 20px from top, 20px from left
- **Close Button**: 30px from right, 15px from top
- **Route Text**: 30px from left edge of route box, 8px from top of box
- **SELECT Button**: 100px from right edge of route box, 30px from bottom
- **Book Button**: Centered horizontally, 50px from bottom of panel
- **Prompt Text**: Centered horizontally, 80px from bottom of panel

## Font Specifications

### Font Family
- **Primary**: Arial (loaded from arial.ttf)
- **Fallback**: System default sans-serif

### Font Sizes
- **Panel Title**: 16pt, Bold
- **Route Information**: 10pt, Regular
- **Button Labels**: 11pt, Regular (default Button class size)
- **Prompt Text**: 11pt, Regular
- **Close Button**: 16pt, Bold

## Visual States

### Route Box States

#### Idle (Not Selected, Not Hovered)
```
Background: Dark gray-blue
Outline: 1px muted gray
Text: White
```

#### Selected (User clicked SELECT)
```
Background: Medium blue
Outline: 2px bright cyan
Text: White (remains same)
```

#### Hovered (Mouse over SELECT button)
```
Route box: Same as current state
Button: Cyan background (hover effect)
Cursor: Pointer
```

### Button States

#### Book Selected Route - Active
```
Background: Dark gray
Outline: Default
Text: White
Enabled: Yes
```

#### Book Selected Route - Disabled
```
Background: Dark gray (COL_BTN_DISABLED)
Outline: Default
Text: White
Enabled: No
Cursor: Default (not clickable)
```

#### SELECT Button - Idle
```
Background: Dark gray
Text: White
Border: Default button styling
```

#### SELECT Button - Hover
```
Background: Bright cyan
Text: White
Border: Highlighted
```

## Interaction Zones

### Clickable Areas
1. **SELECT Buttons**: 80×25px each, positioned at right side of route boxes
2. **Book Selected Route**: 200×35px, centered at bottom
3. **Close Button (X)**: 25×25px, top-right corner
4. **Outside Panel**: Entire area outside panel boundaries closes panel

### Non-Clickable Areas
- Route information text (for reading only)
- Panel background (unless clicking to close)
- Overlay background (clicking here closes panel)

## Animation and Transitions

### Panel Appearance
- **Type**: Instant (no fade-in animation)
- **Effect**: Full opacity immediately
- **Overlay**: Applied instantly

### Route Selection
- **Type**: Instant color change
- **Effect**: Immediate highlight when clicked
- **Previous Selection**: Instantly unhighlights

### Button Hover
- **Type**: Instant (follows SFML button class behavior)
- **Effect**: Color changes immediately on mouse enter/exit

## Accessibility Considerations

### Visual Clarity
- High contrast between text and background
- Clear visual distinction between selected/unselected routes
- Large enough text for readability (10pt minimum)
- Distinct button states (enabled/disabled)

### Interaction Feedback
- Hover effects on all clickable elements
- Clear visual selection state
- Disabled state prevents accidental clicks
- Multiple ways to close panel (X, outside click)

### Information Hierarchy
- Title at top (largest text)
- Route information organized consistently
- Action buttons separated from information
- Prompt text guides user when needed

## Responsive Behavior

### Window Size
- Panel size is fixed (550×550px)
- Position calculated dynamically to center on map
- Works with window size: 1536 + SIDEBAR_WIDTH (350) × 1024

### Content Overflow
- Maximum 5 routes displayed simultaneously
- If more than 5 routes exist, only first 5 shown
- No scroll bars (limitation noted in documentation)
- Route boxes stack vertically with fixed spacing

## Z-Index Ordering (Front to Back)

1. **Route Selection Panel** (frontmost)
   - Panel background and outline
   - Title and close button
   - Route boxes and information
   - Buttons (SELECT, Book Selected)
   - Prompt text

2. **Screen Overlay**
   - Semi-transparent dark overlay
   - Covers entire screen behind panel

3. **Map and Sidebar** (background)
   - Map area (frozen/non-interactive while panel open)
   - Sidebar (frozen/non-interactive while panel open)
   - Ports and routes (visible but not clickable)

## Implementation Details

### SFML Components Used
- `sf::RectangleShape` - Panel backgrounds, overlays, route boxes
- `sf::Text` - All text elements (title, route info, buttons)
- `Button` class - Custom button implementation for all buttons
- `sf::Font` - Arial font for all text rendering

### Drawing Order (in render loop)
1. Draw screen overlay
2. Draw panel background with outline
3. Draw panel title
4. Draw close button text
5. For each route (loop):
   - Draw route box background
   - Draw route information text
   - Draw SELECT button
6. Draw prompt text (if no selection)
7. Draw Book Selected Route button

### State Management
- Panel visibility: `showRouteSelectionPanel` boolean
- Current selection: `selectedRouteIndex` integer (-1 = none)
- Button states: Managed by Button class update() calls
- Route data: Read from existing `foundJourneys[]` array

## Example Route Display Formats

### Single Company, Direct Route
```
Route 1 - Direct
Company: Maersk
Departs: 20/12/2024 06:00 | Arrives: 20/12/2024 18:00
Duration: 12h 00m | Cost: $350
```

### Multiple Companies, Multi-Leg Route
```
Route 2 - 3 Legs
Company: MSC, Evergreen, CMA CGM
Departs: 20/12/2024 08:00 | Arrives: 22/12/2024 02:00
Duration: 42h 00m | Cost: $280
```

### Very Long Company Names (Truncated)
```
Route 3 - 4 Legs
Company: Maersk, Mediterranean Shipping Company, Evergreen, CMA...
Departs: 20/12/2024 10:00 | Arrives: 23/12/2024 14:30
Duration: 76h 30m | Cost: $225
```

## Cross-Platform Considerations

### Font Rendering
- Arial.ttf must be present in build directory
- If font fails to load, fallback to SFML default
- Font smoothing depends on SFML settings

### Color Rendering
- All colors specified in RGB with alpha
- Alpha blending requires graphics card support
- Colors tested on Windows platform

### Mouse Interaction
- Standard SFML mouse event handling
- Left click only (no right-click on panel)
- Hover effects require mouse position tracking

## Testing Scenarios for Visual Design

1. **Panel Positioning**: Verify panel is centered on various screen sizes
2. **Color Contrast**: Check text readability on all background colors
3. **Button States**: Test all button hover and disabled states
4. **Selection Visual**: Confirm selected route is clearly distinguished
5. **Overflow Handling**: Test with 1, 3, 5, and 10+ routes
6. **Long Text**: Test with very long company names
7. **Font Rendering**: Verify text is crisp and readable at all sizes
8. **Alpha Blending**: Check overlay transparency shows map underneath
9. **Z-Order**: Confirm panel appears on top of all other UI elements
10. **Close Interaction**: Test both close button and outside click

## Known Visual Limitations

1. **No Animations**: Panel appears/disappears instantly (no transitions)
2. **Fixed Layout**: Panel doesn't adapt to content (always 550×550px)
3. **No Scrolling**: Only 5 routes visible at maximum
4. **Text Truncation**: Very long text may be cut off without ellipsis
5. **No Tooltips**: Route boxes don't show additional info on hover
6. **Static Font Size**: Font size doesn't scale with window size

## Design Rationale

### Why This Layout?
- **Centered Panel**: Focuses user attention on route selection
- **Dark Overlay**: Reduces distraction from background elements
- **Vertical Stack**: Easy to scan and compare routes
- **Large Buttons**: Easy to click, clear action items
- **Consistent Spacing**: Professional, organized appearance

### Why These Colors?
- **Cyan/Blue Theme**: Matches existing UI color scheme (water/ocean theme)
- **High Contrast**: Ensures text readability
- **Subtle Selection**: Blue highlight is clear but not overwhelming
- **Disabled Gray**: Universal convention for non-interactive elements

### Why These Dimensions?
- **550×550px Panel**: Large enough for 5 routes with comfortable spacing
- **85px Route Height**: Fits 5 lines of text with padding
- **10px Spacing**: Provides clear visual separation between routes
- **200px Book Button**: Large enough for text, visually prominent

This visual design ensures a professional, user-friendly interface that integrates seamlessly with the existing OceanRoute Nav application while providing clear, actionable information for route selection.
