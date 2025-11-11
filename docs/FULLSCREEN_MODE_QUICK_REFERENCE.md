# Fullscreen Mode - Quick Reference

## What Was Changed

### 1. Constants (Lines ~82-85)
```c
#define FULLSCREEN_STRIP_HEIGHT 50      // Bottom strip height
#define FULLSCREEN_HIDE_DELAY 300       // Hide after 5 seconds @ 60fps
#define FULLSCREEN_TOUCH_ZONE 80        // Touch area height to reveal
#define UTILITY_BUTTON_COUNT 7          // Increased from 6 (added fullscreen button)
```

### 2. Global Variables (Lines ~137-140)
```c
static int fullscreen_mode = 0;           // Toggle: 0=dual, 1=fullscreen
static int fullscreen_strip_visible = 0;  // Is strip shown?
static int fullscreen_hide_timer = 0;     // Countdown timer
```

### 3. Utility Button Array (Lines ~120-127)
Added 7th button at position (659, 471):
```c
{659, 471, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Fullscreen", 0}  // Button 3
```

### 4. Functions Modified

#### `process_utility_button_input()` - Case 3
Handles fullscreen button toggle in dual-screen mode

#### `process_hotspot_input()`
- Added fullscreen mode detection at start (~70 lines)
- Handles touches in bottom 80px zone to reveal/hide strip
- Processes button clicks in fullscreen strip

#### `render_dual_screen()`
- Added fullscreen rendering branch (85+ lines)
- Scales and centers game to fit screen
- Renders auto-hide strip with 4 buttons

#### `retro_run()`
- Added timer decrement (5-6 lines)

## Behavior Summary

| Mode | Display | Controls | Auto-hide |
|------|---------|----------|-----------|
| **Dual-screen** | Game 704×448 + Keypad 370×600 | Full utility buttons | N/A |
| **Fullscreen** | Game 859×550 (centered) | Bottom strip with 4 buttons | After 5 seconds |
| **Fullscreen (hidden)** | Game full screen | Touch bottom 80px to reveal | N/A |

## Key Sequences

### Entering Fullscreen
1. Touch "Fullscreen" button in utility area
2. Game fills screen, strip appears at bottom
3. Strip auto-hides after 5 seconds

### Revealing Strip While Fullscreen
1. Touch bottom 80 pixels of screen (anywhere)
2. Strip appears, timer resets to 5 seconds
3. Strip hides again if no interaction

### Exiting Fullscreen
1. Touch "Fullscreen" button in the bottom strip
2. Returns to dual-screen immediately

## Dimensions Reference

```
Fullscreen with strip visible:
┌──────────────────────────────────────────┐
│                                          │
│           Game Display 859×550            │ (scaled, centered)
│           (352:224 aspect maintained)    │
│                                          │
├──────────────────────────────────────────┤
│  Menu | Quit | Swap | Fullscreen        │ (50px height)
│  (Touch zone: 80px from bottom)          │
└──────────────────────────────────────────┘
        1074 × 600 total
```

## Testing Checklist

- [ ] Fullscreen button appears in utility area
- [ ] Clicking fullscreen button enters fullscreen mode
- [ ] Game fills screen and maintains aspect ratio
- [ ] Strip appears at bottom with 4 buttons
- [ ] Strip hides after 5 seconds of no interaction
- [ ] Touching bottom 80px reveals strip again
- [ ] Timer resets when strip is revealed
- [ ] Fullscreen button in strip exits fullscreen mode
- [ ] Returns to dual-screen with keypad visible
- [ ] Swap button works in fullscreen mode
- [ ] Dual-screen mode unaffected by changes

## Compilation

```bash
make platform=unix    # Linux
make platform=win     # Windows
make platform=osx     # macOS
```

No additional compilation flags needed.

## Debugging

Add to `libretro.c` for more verbose logging:

```c
// In process_hotspot_input() fullscreen section:
debug_log("[FULLSCREEN] Touch at y=%d, strip_visible=%d, timer=%d", 
          mouse_y, fullscreen_strip_visible, fullscreen_hide_timer);

// In render_dual_screen() fullscreen branch:
debug_log("[FULLSCREEN_RENDER] scale=%.2f, game_size=%dx%d offset_x=%d",
          scale, scaled_width, scaled_height, offset_x);
```

Look for these debug messages in RetroArch logs when testing.

## Known Limitations

- Only 4 buttons shown in fullscreen (Menu, Quit, Swap, Fullscreen)
- Game always maintains aspect ratio (may have letterboxing)
- No gesture support (only touch-based reveal)
- Hide timer cannot be adjusted without recompile
- No persistent fullscreen preference between sessions

## Future Improvements

Consider adding:
- [ ] Core option for auto-hide delay
- [ ] Core option for touch zone height
- [ ] Swipe gestures for reveal/hide
- [ ] Always-on-screen persistent small button for quick exit
- [ ] Memory of fullscreen preference
