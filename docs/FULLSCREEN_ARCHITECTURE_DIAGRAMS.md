# Fullscreen Mode - Visual Architecture

## Code Flow Diagram

```
retro_run()
    │
    ├─→ InputPoll()
    │
    ├─→ [NEW] Update fullscreen timer
    │   if (fullscreen_mode && fullscreen_hide_timer > 0)
    │       fullscreen_hide_timer--
    │
    ├─→ getControllerState()
    │
    ├─→ setControllerInput()
    │
    ├─→ process_hotspot_input()
    │   │
    │   └─→ [NEW] If fullscreen_mode:
    │       ├─ Check bottom touch zone (80px from bottom)
    │       ├─ Reveal strip if touched
    │       ├─ Reset hide timer
    │       ├─ Process button clicks in strip
    │       └─ Return early (skip keypad processing)
    │
    ├─→ process_utility_button_input()
    │   │
    │   └─→ [NEW] Case 3: Fullscreen button
    │       ├─ Toggle fullscreen_mode
    │       ├─ Show strip initially
    │       └─ Start hide timer
    │
    ├─→ Run() - CPU emulation
    │
    ├─→ render_dual_screen()
    │   │
    │   └─→ [NEW] If fullscreen_mode:
    │       ├─ Scale game to fit canvas
    │       ├─ Center game (maintain aspect)
    │       ├─ If strip_visible:
    │       │   ├─ Draw dark background
    │       │   ├─ Draw 4 buttons
    │       │   └─ Draw borders
    │       └─ Return early
    │       
    │   └─→ Else (dual-screen mode):
    │       ├─ Original rendering
    │       ├─ Game + Keypad + Utility buttons
    │       └─ Continue...
    │
    └─→ Video() - Push to RetroArch
```

## State Machine

```
                    ┌─────────────────────┐
                    │   DUAL SCREEN       │
                    │   (fullscreen_mode) │
                    │   = 0               │
                    │                     │
                    │ Game+Keypad visible │
                    │ All utilities shown │
                    └──────────┬──────────┘
                               │
                    Click "Fullscreen" button
                               │
                               ▼
        ┌──────────────────────────────────────────┐
        │    FULLSCREEN STRIP VISIBLE              │
        │    (fullscreen_mode=1,                   │
        │    fullscreen_strip_visible=1)           │
        │                                          │
        │ Game fills screen                        │
        │ Strip shows at bottom (50px)             │
        │ Timer=300 frames                         │
        └──────────┬───────────────┬───────────────┘
                   │               │
         No touch  │               │ Touch bottom 80px
         5 seconds │               │ (resets timer)
                   │               │
                   ▼               ▼
    ┌──────────────────────────────────────────────┐
    │  FULLSCREEN STRIP HIDDEN                     │
    │  (fullscreen_mode=1,                         │
    │  fullscreen_strip_visible=0)                 │
    │                                              │
    │ Game fills entire screen                    │
    │ Strip hidden (no buttons visible)           │
    │ Touch bottom reveals → back to above state  │
    └──────────┬───────────────────────────────────┘
               │
     Click "Fullscreen" button in strip
               │
               ▼
    (back to DUAL SCREEN)
```

## Touch Zones (Fullscreen Mode)

```
┌────────────────────────────────────────────────┐  Y=0
│                                                │
│                                                │
│  Full Width Touch (anywhere)                   │
│  - If in bottom 80px: show strip, reset timer │
│  - If outside: decrement timer                │
│                                                │
│                                                │
│                ┌──────────────────┐            │  Y=520
│  Game         │ Touch Zone (80px) │            │
│  1074×550     │  y >= 520         │            │
│               └──────────────────┘            │
│                                                │
├────────────────────────────────────────────────┤  Y=550
│ Strip (50px)          (fullscreen_strip_visible)
│ [Menu] [Quit] [Swap] [Fullscreen]             │
└────────────────────────────────────────────────┘  Y=600
```

## Button Layout in Fullscreen Strip

```
Center of each button = button_spacing * (index + 1)
button_spacing = WORKSPACE_WIDTH / (4 + 1) = 1074 / 5 = 214.8px

┌────────────────────────────────────────────────┐
│                                                │
│ [Menu]      [Quit]      [Swap]    [Fullscreen]│
│  214.8      429.6       644.4        858.2    │
│ ±100px      ±100px      ±100px      ±100px   │
│ (200px wide buttons centered)                 │
│                                                │
└────────────────────────────────────────────────┘
Width: 1074px
```

## Game Scaling Algorithm

```
Input:
  - Native game: 352×224 (11:7 aspect ratio)
  - Target canvas: 1074×600 (or 1074×550 with strip)

Calculation:
  scale_x = 1074 / 352 = 3.05
  scale_y = 550 / 224 = 2.46
  scale = min(scale_x, scale_y) = 2.46

Result:
  scaled_width = 352 * 2.46 = 866px
  scaled_height = 224 * 2.46 = 551px
  
Positioning:
  offset_x = (1074 - 866) / 2 = 104px (center horizontally)
  offset_y = 0 (top aligned)

Final:
  Game displayed at position (104, 0) sized 866×551
  Letterboxed by ~104px on each side
```

## Memory Layout (Variables)

```
Stack (retro_run):
  local variables
  fullscreen_hide_timer (counts down)

Static Variables:
  fullscreen_mode         [1 byte] = 0 or 1
  fullscreen_strip_visible [1 byte] = 0 or 1
  fullscreen_hide_timer    [4 bytes] = 0 to 300

Heap (render_dual_screen):
  dual_screen_buffer [1074 * 600 * 4 bytes] = 2,577,600 bytes (2.5 MB)
```

## Toggle Button Coordinates

```
Normal Layout:
Row 1:  [44,471]     [249,471]     [454,471]     [659,471]
        Menu         Quit          Swap          Fullscreen (NEW)
        
Row 2:  [44,526]     [249,526]     [454,526]     (empty)
        Save         Load          Screenshot    

Button size: 200×50px
Spacing: 205px apart horizontally
```

## Frame Timeline (Fullscreen Entry)

```
Frame N:
  User touches "Fullscreen" button in utility area
  
  retro_run():
    process_utility_button_input()
      case 3: fullscreen_mode = 1
              fullscreen_strip_visible = 1
              fullscreen_hide_timer = 300

Frame N+1 to N+4:
  render_dual_screen():
    if (fullscreen_mode):
      - Scale and render game fullscreen
      - Draw strip at bottom
      - Return early
      
  retro_run():
    fullscreen_hide_timer-- (now 299)

Frame N+5 to N+299:
  Strip remains visible, timer counts down
  
Frame N+300:
  fullscreen_hide_timer reaches 0
  fullscreen_strip_visible = 0

Frame N+301 onward:
  render_dual_screen():
    - Game rendered fullscreen
    - No strip shown
    - Only bottom 80px is touch-sensitive
    
  process_hotspot_input():
    - If touch in bottom 80px:
        fullscreen_strip_visible = 1
        fullscreen_hide_timer = 300
        (back to Frame N+1)
```

## Integration Points

```
┌────────────────────────────────────────────────┐
│ libretro.c                                     │
├────────────────────────────────────────────────┤
│                                                │
│ ┌──────────────────────────────────────────┐  │
│ │ retro_run()                              │  │
│ │ - Timer management                       │  │
│ └──────────────────────────────────────────┘  │
│                                                │
│ ┌──────────────────────────────────────────┐  │
│ │ process_utility_button_input()           │  │
│ │ - Fullscreen toggle handling             │  │
│ └──────────────────────────────────────────┘  │
│                                                │
│ ┌──────────────────────────────────────────┐  │
│ │ process_hotspot_input()                  │  │
│ │ - Bottom zone detection                  │  │
│ │ - Strip button handling                  │  │
│ │ - Timer reset logic                      │  │
│ └──────────────────────────────────────────┘  │
│                                                │
│ ┌──────────────────────────────────────────┐  │
│ │ render_dual_screen()                     │  │
│ │ - Fullscreen rendering branch            │  │
│ │ - Game scaling & centering               │  │
│ │ - Strip rendering                        │  │
│ └──────────────────────────────────────────┘  │
│                                                │
└────────────────────────────────────────────────┘
         ▲
         │
         └────── No changes to emulation core
                 (intv.c, cp1610.c, stic.c, psg.c)
                 or controller system
```

---

**All implementations are self-contained in libretro.c with no external dependencies.**
