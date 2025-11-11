# Fullscreen Mode Implementation Guide

## Overview
This document describes the fullscreen mode feature added to FreeIntvTSOverlay, which allows users to maximize the game display with an auto-hide control strip at the bottom.

## Features

### Fullscreen Toggle
- **New button**: "Fullscreen" button added to utility buttons (index 3, Row 1, Position 4)
- **Location**: Utility area below the game in dual-screen mode
- **Function**: Toggles between dual-screen (game + keypad) and fullscreen (game only) modes

### Fullscreen Display
- Game fills the entire 1074×600 pixel canvas
- Game maintains original 352:224 aspect ratio (11:7)
- Game is automatically scaled to fit the screen while preserving aspect
- Game is centered on the canvas

### Auto-Hide Control Strip
- **Bottom strip**: 50 pixels tall (FULLSCREEN_STRIP_HEIGHT)
- **Auto-hide**: Strip hides after 5 seconds of inactivity (300 frames @ 60fps)
- **Reveal**: Touching the bottom 80 pixels of the screen reveals the strip
- **Timer reset**: Any touch in the bottom zone resets the hide timer

### Control Strip Content
The auto-hide strip displays 4 buttons:
1. **Menu** (Button 0) - Placeholder
2. **Quit** (Button 1) - Placeholder  
3. **Swap Screen** (Button 2) - Toggles between swapped/unswapped positions
4. **Fullscreen** (Button 3) - Exits fullscreen mode back to dual-screen

Buttons are centered and evenly distributed across the strip width (1074px).

## Technical Implementation

### Constants Added
```c
#define FULLSCREEN_STRIP_HEIGHT 50      // Height of bottom auto-hide strip
#define FULLSCREEN_HIDE_DELAY 300       // Frames before auto-hiding (5 seconds @ 60fps)
#define FULLSCREEN_TOUCH_ZONE 80        // Touch area from bottom to reveal strip
```

### Variables Added
```c
static int fullscreen_mode = 0;           // 0 = dual screen, 1 = fullscreen
static int fullscreen_strip_visible = 0;  // 1 = show strip, 0 = hidden
static int fullscreen_hide_timer = 0;     // Countdown timer for auto-hiding
```

### Button Array Update
- `UTILITY_BUTTON_COUNT` increased from 6 to 7
- New button added at position (659, 471) in row 1

### Modified Functions

#### `process_utility_button_input()`
- **Dual-screen mode**: Processes buttons 2 (Swap) and 3 (Fullscreen)
- Fullscreen toggle case:
  - Sets `fullscreen_mode = 1`
  - Initializes `fullscreen_strip_visible = 1` and `fullscreen_hide_timer = FULLSCREEN_HIDE_DELAY`

#### `process_hotspot_input()`
- **Fullscreen mode detection**: Checks for touches in bottom 80px zone (`FULLSCREEN_TOUCH_ZONE`)
- **Strip visibility**: Touches in bottom zone make strip visible and reset timer
- **Button clicks**: When strip is visible, processes clicks on the 4 fullscreen buttons
- **Auto-hide logic**: Touches outside bottom zone count down the timer

#### `render_dual_screen()`
- **Fullscreen branch**: Added at start of function
  - Scales game to fit 1074×600 (or 1074×550 if strip is visible)
  - Maintains 352:224 aspect ratio
  - Centers game horizontally
  - Draws auto-hide strip if visible (dark background + 4 buttons)
  - Returns early to skip dual-screen rendering
- **Dual-screen branch**: Original rendering logic unchanged

#### `retro_run()`
- **Timer decrement**: Auto-hide timer counts down each frame
- **Strip auto-hide**: When timer reaches 0, `fullscreen_strip_visible` is set to 0

## Usage Flow

### Entering Fullscreen
1. User touches the "Fullscreen" button in utility area
2. `fullscreen_mode` set to 1
3. `fullscreen_strip_visible` set to 1 (initially visible)
4. `fullscreen_hide_timer` set to 300 frames
5. Next frame: Game renders fullscreen with strip at bottom

### Using Fullscreen
1. Game displays fullscreen
2. Control strip visible at bottom for 5 seconds
3. User can interact with buttons in strip (Swap, Fullscreen toggle)
4. If user touches anywhere on screen:
   - If in bottom 80px zone: Strip becomes/stays visible, timer resets
   - If outside bottom zone: Timer counts down (eventually hides strip)

### Exiting Fullscreen
1. User touches "Fullscreen" button in the control strip
2. `fullscreen_mode` set to 0
3. `fullscreen_strip_visible` set to 0
4. Next frame: Dual-screen rendering resumes (game + keypad + utility buttons)

## Game Scaling Logic

### Aspect Ratio Calculation
```c
double scale_x = WORKSPACE_WIDTH / (double)GAME_WIDTH;   // 1074 / 352 = 3.05
double scale_y = game_height / (double)GAME_HEIGHT;       // 550 / 224 = 2.46 (with strip)
double scale = (scale_x < scale_y) ? scale_x : scale_y;   // Uses smaller to fit
```

With strip visible:
- `scale_x` = 3.05 (would make width 1074)
- `scale_y` = 2.46 (would make height 550)
- Selected scale = 2.46 (smaller)
- **Final game size**: 859 × 550 (slightly letterboxed on sides)

Without strip:
- `scale_y` = 2.68 (600 / 224)
- Selected scale = 2.46 (still limited by aspect)
- **Final game size**: 859 × 600 (letterboxed on sides)

The game is centered horizontally on the canvas.

## Configuration

### Adjustable Constants
All these can be modified in libretro.c:

- `FULLSCREEN_STRIP_HEIGHT`: Height of bottom control strip (currently 50px)
- `FULLSCREEN_HIDE_DELAY`: Frames before auto-hide (currently 300 = 5 sec @ 60fps)
- `FULLSCREEN_TOUCH_ZONE`: Height of touch zone to reveal strip (currently 80px)

### Changing Hide Behavior
To make strip auto-hide faster, reduce `FULLSCREEN_HIDE_DELAY`:
- 60 frames = 1 second
- 180 frames = 3 seconds
- 300 frames = 5 seconds (current)

To make touch zone larger/smaller, adjust `FULLSCREEN_TOUCH_ZONE`:
- Larger value = easier to reveal strip but more of screen reserved
- Smaller value = more game screen but harder to access strip

## Testing

### Test Cases

1. **Toggle fullscreen**
   - Click "Fullscreen" button → Game should maximize
   - Strip should appear at bottom with 4 buttons

2. **Auto-hide timer**
   - Enter fullscreen
   - Wait 5 seconds without touching screen
   - Strip should disappear

3. **Reveal by touch**
   - In fullscreen with hidden strip
   - Touch bottom 80 pixels of screen
   - Strip should reappear

4. **Button interaction**
   - In fullscreen with visible strip
   - Click "Swap" button → Should swap game/keypad (no effect in fullscreen, but button responds)
   - Click "Fullscreen" button → Should exit fullscreen

5. **Exit fullscreen**
   - In fullscreen mode
   - Click "Fullscreen" button to toggle off
   - Should return to dual-screen layout with keypad visible

6. **Touch zone accuracy**
   - Touch at Y=520 (within 80px zone) → Strip appears
   - Touch at Y=400 (outside 80px zone) → Should start countdown

## Notes

- **Fullscreen button in dual-screen**: When in dual-screen mode, the fullscreen button appears in the utility area (Row 1, Position 4)
- **Button limitations**: In fullscreen mode, buttons 4, 5, 6 (Save, Load, Screenshot) are not shown; only the first 4 buttons are available
- **Aspect ratio**: Game maintains its native 352:224 aspect ratio, so there may be letterboxing on the sides when fullscreen
- **Touch priority**: In fullscreen, touches are primarily processed by the full-screen handler, not the hotspot system

## Future Enhancements

Possible improvements to consider:
1. Configurable hide delay via core options
2. Alternative auto-hide behaviors (always visible, always hidden, etc.)
3. Swipe gestures to hide/show strip
4. Custom button configurations for fullscreen strip
5. Save fullscreen preference between sessions
