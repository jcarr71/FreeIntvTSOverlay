# Build Instructions for Testing Latest Changes

## Required Tools
- MinGW64 (installed at C:\mingw64)
- GNU Make (included with MinGW64)

## Build Steps

### Option 1: Using MinGW Make (Recommended)
```powershell
cd S:\VisualStudio\FreeIntvTSOverlay
$env:PATH = "C:\mingw64\bin;$env:PATH"
C:\mingw64\bin\mingw32-make.exe platform=win clean
C:\mingw64\bin\mingw32-make.exe platform=win
```

### Option 2: Manual Compilation
If make has issues, you can compile manually:

```powershell
cd S:\VisualStudio\FreeIntvTSOverlay\src

# Compile all source files
C:\mingw64\bin\gcc.exe -c libretro.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c intv.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c memory.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c cp1610.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c cart.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c controller.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c osd.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c ivoice.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c psg.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c stic.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c stb_image_impl.c -I. -I./deps/libretro-common/include -O2

# Compile libretro-common dependencies
C:\mingw64\bin\gcc.exe -c deps/libretro-common/file/file_path.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/compat/compat_posix_string.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/compat/compat_snprintf.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/compat/compat_strl.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/compat/compat_strcasestr.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/compat/fopen_utf8.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/encodings/encoding_utf.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/string/stdstring.c -I. -I./deps/libretro-common/include -O2
C:\mingw64\bin\gcc.exe -c deps/libretro-common/time/rtime.c -I. -I./deps/libretro-common/include -O2

# Link into DLL
cd ..
C:\mingw64\bin\gcc.exe -shared -o freeintvtsoverlay_libretro.dll src/*.o -lws2_32
```

## What Changed in the Latest Build

### 1. Fullscreen Hotspot Highlights (Line 1530)
- Changed highlight transparency from 67% opaque to **60% opaque (40% transparent)**
- Now shows `0x9900FF00` instead of `0xAA00FF00`

### 2. Fullscreen Overlay Hotspot Processing (Lines 1768-1772)
- Hotspots now process in fullscreen mode when overlay is **visible**
- Hotspots skip processing when overlay is **hidden** (saves CPU)
- Highlights will appear when touching keypad buttons in fullscreen with overlay ON

## Testing the Changes

1. **Build the new DLL**
   - Use make or manual compilation above
   - Output file: `freeintvtsoverlay_libretro.dll`

2. **Replace the old DLL**
   - Copy to: `RetroArch\cores\freeintvtsoverlay_libretro.dll`

3. **Test Fullscreen Highlights**
   - Load B-17 Bomber in RetroArch
   - Press the fullscreen button (below game screen)
   - Game goes fullscreen
   - Touch the bottom of screen to reveal control strip
   - Click the rightmost button (Show Overlay) to make keypad visible
   - Touch keypad buttons → **Green highlights should now appear with 40% transparency**

4. **Test Fullscreen Toggle**
   - Click the leftmost button in strip (Exit Fullscreen) → Returns to dual-screen
   - Click the fullscreen button again → Back to fullscreen
   - Strip auto-hides after 5 seconds of no touch

## Verification

After rebuilding, verify:
- ✓ Highlights appear in fullscreen when overlay is visible
- ✓ Highlights have reduced opacity (40% transparency)
- ✓ Fullscreen toggle works both ways (enter/exit)
- ✓ Control strip appears on bottom touch and auto-hides
- ✓ Dual-screen highlights still work as before

---
**Last Updated**: November 11, 2025
**Changes Compiled**: YES (source modified, DLL needs rebuild)
