# Button Layout Reorganization - Separated onto Two Rows

**Date**: November 11, 2025 12:00 PM  
**Build**: Windows DLL (204 KB)

## Problem Identified

The Fullscreen button and Swap Screen button were positioned at the same X coordinate after the swap operation:
- **Original layout (same row)**:
  - Button 0 (Fullscreen): x=44
  - Button 2 (Swap): x=454
  
- **After display_swap=true** (buttons move with game):
  - Button 0 (Fullscreen): x=44+370=414
  - Button 2 (Swap): x=454+370=824 (off-screen)
  
The offset caused button 0 to appear at x=414, which was exactly where button 2 started when unswapped (x=454 before, but coordinates shifted). This made clicks on swap activate fullscreen due to coordinate overlap.

## Solution

Reorganized button layout to place both buttons in the **center** of the utility space, but on **separate rows**:

### New Layout
```
Utility Space: 704px wide × 152px tall (Y=448-600)
Center X: (704 - 200) / 2 = 252px

┌────────────────────────────────────────────────────────────────┐
│                                                                │
│                    Button 2: Swap Screen                       │
│                      (X=252, Y=460)                            │
│                      200×50 button                             │
│                                                                │
│                   Button 0: Fullscreen                         │
│                      (X=252, Y=510)                            │
│                      200×50 button                             │
│                                                                │
└────────────────────────────────────────────────────────────────┘
Y=448                                                          Y=600
```

### Key Changes
- **Button 0 (Fullscreen)**: Moved from x=44, y=471 → **x=252, y=510** (bottom center)
- **Button 2 (Swap)**: Moved from x=454, y=471 → **x=252, y=460** (top center)

### Why This Works
1. **No coordinate overlap**: Even with swap offset (+370), buttons remain vertically separated
   - Button 2 stays at y=460 (unaffected by x-offset)
   - Button 0 stays at y=510 (unaffected by x-offset)
   
2. **Visually centered**: Both buttons centered horizontally in utility space regardless of swap state
   
3. **Offset-safe**: The y-axis is completely independent of the `display_swap` flag, eliminating accidental activation

## Testing Checklist

- [ ] Click Swap Screen button (top center) - should only toggle swap
- [ ] Click Fullscreen button (bottom center) - should only toggle fullscreen
- [ ] Click Swap Screen to reverse display
- [ ] Verify both buttons still render in correct positions after swap
- [ ] Click buttons again after swap - no accidental activation
- [ ] Verify yellow highlight appears only on clicked button
- [ ] Test with/without PNG images loaded

## Files Modified
- `src/libretro.c`: Button position definitions (lines 115-128)

## Build Status
✅ **Success** - No compiler warnings or errors
- Windows DLL: 204 KB, 12:00 PM November 11, 2025
