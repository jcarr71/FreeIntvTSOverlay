# Fullscreen Overlay Toggle Button Implementation

**Date**: November 11, 2025 12:12 PM  
**Build**: Windows DLL (206 KB)

## Feature Added

A new button has been added to the fullscreen control strip that toggles the visibility of the controller overlay on the game screen with transparency.

## Implementation Details

### New Global Variable
- `overlay_visible_in_fullscreen`: Tracks whether overlay is shown in fullscreen mode (1 = visible, 0 = hidden)

### Fullscreen Strip Layout (Now 2 Buttons)
The fullscreen strip now displays 2 buttons, evenly spaced at 1/3 and 2/3 of the workspace width:

```
Fullscreen Strip (1074px wide):
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│    Button 0: Exit Fullscreen           Button 1: Show/Hide Overlay         │
│    (Position: 1/3 width)               (Position: 2/3 width)               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Button Click Detection
- **Button 0 (Exit)**: Toggles `fullscreen_mode` flag to exit fullscreen
- **Button 1 (Overlay Toggle)**: Toggles `overlay_visible_in_fullscreen` flag
  - Uses index 7 for tracking pressed state (separate from utility buttons 0-6)
  - Shows/hides the controller overlay with transparency on the game

### Overlay Rendering (New)
When `overlay_visible_in_fullscreen` is enabled:

1. **Scaling**: Overlay is scaled to match the scaled game display
2. **Transparency**: Applied at 60% opacity for visibility through the overlay
3. **Alpha Blending**: Properly blends overlay pixels with game pixels using alpha compositing
4. **Full Coverage**: Overlay covers the entire game screen area in fullscreen mode

**Rendering Process**:
- Scales both game and overlay to fit fullscreen display
- Loops through overlay pixels with proper coordinate transformation
- Applies 60% opacity (0x99 alpha) to overlay for game visibility
- Uses per-pixel alpha blending to composite overlay on top of game

### Code Changes
1. Added `overlay_visible_in_fullscreen` static variable (line 149)
2. Updated fullscreen strip button rendering (lines 673-767)
3. Added overlay rendering in fullscreen mode (lines 611-658)
4. Updated fullscreen strip button click detection (lines 1238-1285)

## User Experience

**In Fullscreen Mode**:
1. User presses Swap Screen to enter fullscreen
2. Fullscreen strip appears at bottom with 2 buttons
3. User can:
   - Click left button (Exit) to exit fullscreen back to dual-screen
   - Click right button (Overlay) to show/hide the transparent controller layout
4. When overlay is visible, the keypad layout shows over the game at 60% transparency
5. User can still interact with controller hotspots while overlay is visible

**Advantages**:
- ✅ No overlay blocking visible game screen (60% transparency)
- ✅ Players can see exact button positions relative to game content
- ✅ Easy toggle between bare fullscreen and labeled fullscreen
- ✅ Only appears in fullscreen mode (doesn't clutter dual-screen view)
- ✅ Hotspots remain functional with overlay visible

## Testing Checklist

- [ ] Enter fullscreen mode (press Fullscreen button in dual-screen)
- [ ] Verify 2 buttons appear in fullscreen strip
- [ ] Click left button (Exit) - should return to dual-screen mode
- [ ] Click right button (Overlay) - should show controller overlay
- [ ] Verify overlay appears with 60% transparency (game visible underneath)
- [ ] Click Overlay button again - should hide overlay
- [ ] Toggle overlay multiple times - should work smoothly
- [ ] Verify overlay covers entire game display area
- [ ] Test with and without PNG images loaded
- [ ] Verify no accidental button activations
- [ ] Verify strip auto-hides when not touching for 5 seconds

## Files Modified
- `src/libretro.c`: 
  - Line 149: Added `overlay_visible_in_fullscreen` variable
  - Lines 611-658: Added overlay rendering in fullscreen
  - Lines 673-767: Updated fullscreen button rendering for 2 buttons
  - Lines 1238-1285: Updated fullscreen button click detection

## Build Status
✅ **Success** - No compiler errors or warnings
- Windows DLL: 206 KB, 12:12 PM November 11, 2025
