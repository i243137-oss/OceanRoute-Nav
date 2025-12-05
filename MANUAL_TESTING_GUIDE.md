# Manual Testing Guide for Interactive Route Selection Feature

## Prerequisites
- Windows operating system (Windows 10/11)
- OceanRouteNav.exe built and in the build directory
- All required DLL files present in build directory:
  - sfml-graphics-2.dll
  - sfml-window-2.dll
  - sfml-system-2.dll
  - openal32.dll
- Required data files:
  - ports.txt
  - Routes.txt
  - arial.ttf
  - map.png

## Test Scenario 1: Direct Route Selection

### Setup
1. Launch OceanRouteNav.exe
2. Click on Karachi port on the map (origin)
3. Click on Dubai port on the map (destination)
4. Enter departure date: 20/12/2024

### Steps
1. Click "Book Route" button
2. Observe route selection panel appears

### Expected Results
- ✅ Panel appears centered on screen (580×620 pixels)
- ✅ Semi-transparent dark overlay covers the map
- ✅ Panel shows "SELECT A ROUTE" title at top
- ✅ Close button (X) visible in top-right corner
- ✅ At least one direct route is displayed
- ✅ Direct route shows:
  ```
  Route 1 - Direct
  Company: [Company Name]
  Departs: [Date Time] | Arrives: [Date Time]
  Duration: [Hours]h [Minutes]m | Cost: $[Amount]
  ```
- ✅ "SELECT" button visible on the right side of route box
- ✅ "Book Selected Route" button at bottom (disabled/grayed out)

### Actions
1. Click "SELECT" button on the direct route
2. Observe route box highlights in blue
3. Observe "Book Selected Route" button becomes active
4. Click "Book Selected Route" button

### Expected Results After Booking
- ✅ Panel closes
- ✅ Map shows ONLY the selected route line
- ✅ No other route lines visible on map
- ✅ Single ship appears at origin port
- ✅ Ship begins moving along the selected route
- ✅ Status message shows "Route booked!"
- ✅ Simulation time starts

## Test Scenario 2: Multi-Leg Route Selection

### Setup
1. Launch OceanRouteNav.exe (or continue from previous test)
2. If continuing, click "Book Route" to start fresh search
3. Click on Karachi port on the map (origin)
4. Click on Istanbul port on the map (destination)
5. Enter departure date: 20/12/2024

### Steps
1. Click "Book Route" button
2. Observe route selection panel appears
3. Look for multi-leg routes (routes with 2 or more legs)

### Expected Results
- ✅ Panel shows multiple route options (up to 3 visible)
- ✅ At least one multi-leg route displays with format:
  ```
  Route X - [N] Legs
  
    Leg 1: [Origin] -> [Destination]
    [Company] | [HH:MM] -> [HH:MM]
    
    Leg 2: [Origin] -> [Destination]
    [Company] | [HH:MM] -> [HH:MM]
    
    Leg 3: [Origin] -> [Destination]
    [Company] | [HH:MM] -> [HH:MM]
  
  Total Duration: [Hours]h [Minutes]m | Total Cost: $[Amount]
  ```
- ✅ Each leg shows:
  - Leg number (Leg 1, Leg 2, etc.)
  - Origin port → Destination port
  - Shipping company name
  - Departure time → Arrival time
- ✅ Route box height adjusts based on number of legs
- ✅ Total duration and cost shown at bottom

### Actions
1. Click "SELECT" button on a multi-leg route
2. Observe route box highlights in blue
3. Verify leg details are still readable when highlighted
4. Click "Book Selected Route" button

### Expected Results After Booking
- ✅ Panel closes
- ✅ Map shows ONLY the selected multi-leg route
- ✅ All route legs are visible as connected lines
- ✅ No other route lines visible on map
- ✅ Single ship appears at origin port
- ✅ Ship travels through each leg sequentially
- ✅ Ship stops at intermediate ports as specified
- ✅ Status message shows "Route booked!"

## Test Scenario 3: Route Comparison

### Setup
1. Launch OceanRouteNav.exe
2. Select ports that have both direct and multi-leg routes available
   - Example: Karachi → Athens or similar

### Steps
1. Click "Book Route" button
2. Panel shows multiple routes (both direct and multi-leg)

### Expected Results
- ✅ Direct routes show compact format (single company, no leg breakdown)
- ✅ Multi-leg routes show detailed leg information
- ✅ Each route has "SELECT" button on the right
- ✅ Can visually compare:
  - Total duration
  - Total cost
  - Number of legs
  - Companies involved
  - Departure and arrival times
- ✅ Different route box heights (direct=85px, multi-leg varies)

### Actions
1. Compare information between routes
2. Select a route (any)
3. Verify only selected route highlights
4. Click "Book Selected Route"

### Expected Results
- ✅ Only selected route becomes active
- ✅ Other routes are not spawned
- ✅ Map only shows selected route line

## Test Scenario 4: Panel Interactions

### Test 4A: Close Panel with X Button
1. Open route selection panel
2. Click the X button in top-right corner
3. **Expected**: Panel closes, no route booked, map returns to normal

### Test 4B: Close Panel by Clicking Outside
1. Open route selection panel
2. Click on the map area outside the panel
3. **Expected**: Panel closes, no route booked, status shows "Booking cancelled"

### Test 4C: Book Without Selection
1. Open route selection panel
2. Try to click "Book Selected Route" without selecting a route
3. **Expected**: Button is disabled (grayed), click does nothing

### Test 4D: Select Multiple Routes (Sequential)
1. Open route selection panel
2. Click "SELECT" on Route 1
3. Verify Route 1 highlights
4. Click "SELECT" on Route 2
5. **Expected**: Route 2 highlights, Route 1 unhighlights (only one selected at a time)

## Test Scenario 5: New Search After Booking

### Setup
1. Book any route (complete Test Scenario 1 or 2)
2. Observe only selected route is visible on map

### Steps
1. Click "Book Route" button again (to search for new routes)
2. Select same or different origin/destination
3. Enter new date
4. Click "Book Route" button

### Expected Results
- ✅ Route selection panel appears with new routes
- ✅ Previous confirmed route is no longer locked
- ✅ All available routes for new search are visible in panel
- ✅ Can select and book a different route
- ✅ After booking new route, only new route is visible on map

## Test Scenario 6: Edge Cases

### Test 6A: No Routes Available
1. Select origin and destination with no available routes
2. Click "Book Route"
3. **Expected**: Panel does not open, status shows "No routes found"

### Test 6B: Only One Route Available
1. Select ports with only one route
2. Click "Book Route"
3. **Expected**: Panel shows single route, can be selected and booked normally

### Test 6C: Maximum Legs (5 Legs)
1. Find route with 5 or more legs
2. Click "Book Route"
3. **Expected**: Panel shows up to 5 legs (MAX_LEGS_TO_DISPLAY), no overflow

### Test 6D: Long Port Names
1. Select routes with long port/city names
2. Click "Book Route"
3. **Expected**: Text displays without truncation or buffer overflow, stays within route box

### Test 6E: Long Company Names
1. Select routes with long company names
2. Click "Book Route"
3. **Expected**: Company names display fully, no truncation

## Visual Verification Checklist

### Panel Appearance
- [ ] Panel is centered on screen
- [ ] Panel has blue border (RGB 0, 180, 255)
- [ ] Panel has dark background (RGB 20, 25, 35)
- [ ] Screen overlay is semi-transparent dark
- [ ] Close button (X) is red and visible
- [ ] Title "SELECT A ROUTE" is cyan/blue and bold

### Route Boxes
- [ ] Unselected boxes have dark background with thin border
- [ ] Selected box has blue background with thicker blue border
- [ ] Text is white and readable on all backgrounds
- [ ] Boxes are properly aligned and spaced (10px between)
- [ ] SELECT buttons are properly positioned on the right

### Text Formatting
- [ ] Dates formatted as DD/MM/YYYY HH:MM
- [ ] Duration formatted as Xh XXm
- [ ] Cost formatted as $XXX
- [ ] Port names use → arrow symbol
- [ ] Times formatted as HH:MM -> HH:MM
- [ ] Font size is 10pt and readable
- [ ] No text overflow or clipping

### Map Rendering
- [ ] Only confirmed route shows after booking
- [ ] Route line is visible and highlighted
- [ ] Intermediate ports on multi-leg routes are marked
- [ ] No ghost lines from unselected routes
- [ ] Ship animation follows the route correctly

## Performance Verification

### Timing Tests
- [ ] Panel opens immediately (< 100ms)
- [ ] Panel closes immediately when X or Cancel clicked
- [ ] Route selection highlights instantly
- [ ] No lag when switching between route selections
- [ ] No lag when booking route

### Memory Tests
- [ ] No memory leaks after booking multiple routes
- [ ] No crashes when opening/closing panel repeatedly
- [ ] No crashes with maximum legs displayed

## Regression Tests

### Verify Existing Features Still Work
- [ ] Port selection on map still functions
- [ ] Date input still accepts valid dates
- [ ] Search and Dijkstra buttons still work as before
- [ ] Preferences panel still functions
- [ ] Simulation controls (play/pause/speed) still work
- [ ] Ship animation still works correctly
- [ ] Port queue visualization still works
- [ ] Logging system still outputs correctly

## Bug Report Template

If any test fails, document using this template:

```
Test Case: [Scenario Number and Name]
Step: [Which step failed]
Expected: [What should have happened]
Actual: [What actually happened]
Screenshot: [Attach if visual issue]
Console Output: [Any error messages]
Reproducible: [Yes/No, and how many times]
```

## Success Criteria

All tests must pass with ✅ for the feature to be considered complete:
- [ ] All 6 test scenarios pass
- [ ] All visual verification items pass
- [ ] All performance checks pass
- [ ] No regression in existing features
- [ ] No crashes or errors during testing

## Notes
- This is a Windows-only application
- Testing should be done on Windows 10 or Windows 11
- If running on Linux via Wine, results may vary (not officially supported)
