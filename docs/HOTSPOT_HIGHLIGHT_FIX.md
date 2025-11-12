# Hotspot Highlight Fix - Button 0 Not Visible

## Problem
The green highlight indicator was not appearing over button 0 (and potentially other keypad buttons) when pressed on the touchscreen.

## Root Cause
The hotspot highlighting code in `render_dual_screen()` was using incorrect coordinate calculations. Specifically:

### The Issue:
1. **Hotspot coordinates storage**: Hotspots are stored with coordinates **relative to the keypad area** (starting at 0)
2. **Hotspot input handling**: The input detection code correctly **adds the keypad offset** (`GAME_SCREEN_WIDTH` in dual-screen mode) to convert to absolute workspace coordinates
3. **Hotspot highlighting**: The rendering code was **NOT adding the keypad offset**, causing highlights to be drawn in the wrong location (off-screen or on top of the game screen instead of the keypad)

### Code Comparison:
**Input Handling (CORRECT):**
```c
// In process_hotspot_input()
int keypad_x_workspace = display_swap ? 0 : GAME_SCREEN_WIDTH;
hotspot_x = keypad_x_workspace + h->x;  // ✓ Adds offset
```

**Highlighting (BEFORE FIX - WRONG):**
```c
// In render_dual_screen() - OLD CODE
int hotspot_x_adjust = display_swap ? (-GAME_SCREEN_WIDTH) : 0;
for (int x = h->x + hotspot_x_adjust; x < h->x + h->width + hotspot_x_adjust; ++x)
// ✗ Using negative offset logic instead of positive offset
```

## Solution
Updated the hotspot highlighting code to match the coordinate transformation logic used in input handling:

### Highlighting (AFTER FIX - CORRECT):**
```c
// In render_dual_screen() - NEW CODE
if (fullscreen_mode) {
    // Fullscreen mode: hotspots relative to scaled keypad
    int scaled_keypad_w = (int)(KEYPAD_WIDTH * keypad_scale_factor);
    int keypad_x_workspace = WORKSPACE_WIDTH - scaled_keypad_w;
    abs_hotspot_x = keypad_x_workspace + h->x;  // ✓ Adds offset
    abs_hotspot_y = h->y;
} else {
    // Dual-screen mode: hotspots relative to keypad area
    int keypad_x_workspace = display_swap ? 0 : GAME_SCREEN_WIDTH;
    abs_hotspot_x = keypad_x_workspace + h->x;  // ✓ Adds offset
    abs_hotspot_y = h->y;
}
```

## Changes Made
- **File**: `src/libretro.c`
- **Function**: `render_dual_screen()`
- **Lines**: ~1520-1560 (hotspot highlighting section)
- **Change Type**: Logic correction for coordinate transformation

## Testing
The fix ensures that:
1. ✓ Button 0 highlight appears correctly when pressed
2. ✓ All 12 keypad buttons show green highlight when touched
3. ✓ Highlights appear on the correct screen area (keypad, not game)
4. ✓ Works in both dual-screen and fullscreen modes
5. ✓ Works with display_swap enabled/disabled
6. ✓ Works with scaled keypads in fullscreen mode

## Impact
- **Bug severity**: Medium (visual feedback missing)
- **Affected code paths**: Touchscreen input visual feedback
- **No breaking changes**: Only affects rendering, not input handling
- **Compiler check**: Code passes GCC compilation without errors

---
**Date**: November 11, 2025
**Status**: FIXED - Code change applied and verified
