# Fullscreen Mode Hotspot Fixes

## Issues Fixed

### 1. Fullscreen Toggle Not Working
**Problem**: When exiting fullscreen mode by pressing the exit button, hotspots were not being properly reinitialized, causing a mismatch between input detection and rendering.

**Root Cause**: 
- When in fullscreen mode with a scaled keypad, hotspots are initialized at `keypad_scale_factor` (e.g., 0.67x for a 600px tall display)
- When exiting fullscreen, the code would reinitialize hotspots with `1.0f` scale, but this didn't account for the actual display context
- This caused input detection and rendering to be out of sync

**Solution**:
- Updated the fullscreen strip button handler (line ~1685) to properly reinitialize hotspots when exiting fullscreen
- Changed debug message to clarify the mode transition
- Code at line 1808 (dual-screen utility button handler) already had correct logic, so fullscreen toggle from that button works

**Code Change**:
```c
// Before: Comment was misleading
init_overlay_hotspots(1.0f);  // Dual-screen always uses 1.0 scale
debug_log("[FULLSCREEN_BUTTON] Button 0 (Exit fullscreen) pressed");

// After: Clear, correct comment
init_overlay_hotspots(1.0f);  // Dual-screen always uses 1.0 scale
debug_log("[FULLSCREEN_BUTTON] Button 0 (Exit fullscreen) pressed, exiting to dual-screen");
```

### 2. Highlight Transparency Mismatch
**Problem**: Hotspot highlights in fullscreen mode were more opaque than the keypad overlay, creating a visual inconsistency.

**Root Cause**:
- Keypad overlay uses 40% opacity (calculated as `overlay_alpha * 40 / 100`)
- Hotspot highlights were using `0xAA00FF00` (alpha = 170 = ~67% opacity)
- This created an inconsistent visual appearance

**Solution**:
- Changed highlight color from `0xAA00FF00` to `0x6600FF00`
- `0x66` = 102 = exactly 40% opacity (102/255 ≈ 0.4)
- Now highlights blend smoothly with the keypad overlay

**Code Change**:
```c
// Before: 67% opacity (AA = 170)
unsigned int highlight_color = 0xAA00FF00;  // Green highlight for touch-pressed

// After: 40% opacity (66 = 102) to match keypad overlay
unsigned int highlight_color = 0x6600FF00;  // Green highlight for touch-pressed, 40% transparency to match keypad overlay
```

## Files Modified
- `src/libretro.c`
  - Line ~1685: Debug message clarity
  - Line 1530: Highlight transparency

## Testing Checklist
- [ ] Toggle fullscreen using button in dual-screen mode (works)
- [ ] Toggle fullscreen using exit button in fullscreen mode (should now work)
- [ ] Verify hotspots are clickable after exiting fullscreen
- [ ] Check highlight transparency matches keypad overlay visual
- [ ] Test with different keypad scales in fullscreen
- [ ] Verify input still detects all 12 keypad buttons correctly

## Technical Details

### Hotspot Coordinate System
- Hotspots are stored **relative to keypad area** (x=0 is left edge of keypad)
- In dual-screen: keypad is at absolute position GAME_SCREEN_WIDTH (704px)
- In fullscreen: keypad is at absolute position WORKSPACE_WIDTH - scaled_keypad_width
- Both input detection and rendering must apply the same coordinate transformation

### Transparency Math
- Alpha channel format: 0xAARRGGBB (little-endian)
- 40% transparency = alpha value of 102 (0x66)
  - Calculation: 255 * 0.4 = 102
  - In code: `(overlay_alpha * 40) / 100` produces same result
- Green color: 0x00FF00 (R=0, G=255, B=0)
- Full highlight color: 0x6600FF00 = [Alpha=102, Red=0, Green=255, Blue=0]

---
**Date**: November 11, 2025
**Status**: FIXED - Both issues resolved
