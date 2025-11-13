# Quick Deploy - Button Layout Fix

## Updated DLL Ready

**File**: `FreeIntvTSOverlay_libretro.dll`  
**Size**: 201 KB  
**Built**: 11/11/2025 11:48:27 AM  
**Status**: ✅ Ready to deploy  

## Deploy Steps

### 1. Copy DLL to RetroArch
```powershell
Copy-Item `
  "s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll" `
  "$env:APPDATA\RetroArch\cores\"
```

### 2. Restart RetroArch
Close and reopen RetroArch to load the new DLL.

### 3. Test
Load a game ROM and test the button positions:
- **Fullscreen button**: LEFT side of utility area (x=44)
- **Swap button**: CENTER of utility area (x=454)

## Button Layout

```
Utility Area (704 pixels wide):
┌──────────────────────────────────────┐
│ [FS]     [Swap]                      │  Y=471
│ x=44     x=454                       │
└──────────────────────────────────────┘

FS = Fullscreen button (now at correct position)
```

## What Was Fixed

Changed the button input detection to recognize buttons at their new positions:
- Button 0 (Fullscreen) at x=44 ← NOW DETECTED
- Button 2 (Swap) at x=454 ← STILL WORKING

---

**Ready to test!**
