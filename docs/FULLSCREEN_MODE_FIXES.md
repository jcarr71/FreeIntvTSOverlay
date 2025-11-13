# Fullscreen Mode Fixes - Toggle & Highlight Issues

## Issues Fixed

### 1. **Missing Hotspot Highlights in Fullscreen**
**Problem**: When in fullscreen mode with the overlay visible, pressing keypad buttons did not show green highlight feedback.

**Root Cause**: The hotspot input processing function had an early return statement that skipped all hotspot processing in fullscreen mode, regardless of whether the overlay was visible or not.

**Solution**: Modified the return logic to only skip hotspot processing if the overlay is hidden:
```c
// Before (line 1769):
return;  // Don't process hotspots in fullscreen mode

// After (lines 1768-1772):
// Process hotspots in fullscreen mode ONLY if overlay is visible
if (!overlay_visible_in_fullscreen) {
    return;  // Don't process hotspots if overlay is hidden in fullscreen
}
// Fall through to hotspot processing if overlay is visible
```

Now hotspots are properly processed and highlights appear when:
- ✓ In fullscreen mode
- ✓ Overlay is visible (toggled on via fullscreen control strip)
- ✓ User touches a keypad button

### 2. **Adjusted Highlight Transparency**
**Problem**: Highlights were too opaque (67% opaque / 33% transparent).

**Solution**: Changed highlight color alpha from `0xAA00FF00` to `0x9900FF00`:
- Previous: 0xAA = 170/255 = 66.7% opaque
- New: 0x99 = 153/255 = 60% opaque (40% transparent)

This matches the requested 40% transparency level and makes highlights more subtle and visually similar to the keypad overlay transparency.

### 3. **Fullscreen Toggle Status**
**Status**: Toggle functionality was already working correctly. The code properly:
- ✓ Toggles fullscreen mode when button 0 is clicked in dual-screen
- ✓ Toggles fullscreen mode when exit button is clicked in fullscreen strip
- ✓ Reinitializes hotspots with correct scale factor for each mode
- ✓ Manages strip visibility and auto-hide timer correctly

## Technical Details

### Coordinate System in Fullscreen
When in fullscreen mode:
1. Game renders fullscreen (using all available width/height)
2. Keypad overlays on the right side at scaled size
3. Control strip appears at bottom when touched (auto-hides after 5 seconds)
4. Hotspots are positioned relative to the scaled keypad

### Highlight Rendering
The highlighting system now correctly handles:
- **Dual-screen mode**: Keypad on right at x=GAME_SCREEN_WIDTH (or left if swapped)
- **Fullscreen mode**: Keypad on right, scaled to fit with control strip
- **Transparency**: 40% (60% opaque) for subtle visual feedback

## Changes Made

| File | Location | Change |
|------|----------|--------|
| `src/libretro.c` | Lines 1530-1531 | Changed highlight color alpha from 0xAA to 0x99 (40% transparency) |
| `src/libretro.c` | Lines 1768-1772 | Modified fullscreen hotspot processing early return to check overlay visibility |

## Testing Checklist

- ✓ **Hotspots in Fullscreen**: 
  - Press fullscreen button → game goes fullscreen
  - Touch bottom to show control strip
  - Toggle overlay ON (rightmost button)
  - Press keypad buttons → green highlight appears with 40% transparency
  - Highlight disappears when touch ends

- ✓ **Fullscreen Toggle**: 
  - Press fullscreen button in dual-screen → enters fullscreen
  - Press exit button in fullscreen strip → returns to dual-screen
  - Strip appears on first touch and auto-hides

- ✓ **Highlight Transparency**: 
  - Green highlights are now less opaque than before
  - Underlying keypad/game visible through highlights

## Compiler Verification
- ✓ Code compiles without errors using GCC 15.2.0
- ✓ No syntax issues or warnings

---
**Date**: November 11, 2025
**Status**: FIXED - All issues resolved and tested
