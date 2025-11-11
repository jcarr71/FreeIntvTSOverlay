# Deployment Instructions - FreeIntvTSOverlay with Fullscreen

## Quick Deploy (Windows)

### Step 1: Locate the Compiled DLL
```
Source: s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll
Size: 205 KB
Built: 11/11/2025
```

### Step 2: Find Your RetroArch Cores Folder
```
Windows Path: C:\Users\[YOUR_USERNAME]\AppData\Roaming\RetroArch\cores\
```

**To Find It Quickly**:
1. Press `Win + R`
2. Type: `%APPDATA%\RetroArch\cores\`
3. Press Enter

### Step 3: Copy the DLL
```
Copy From:  s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll
Copy To:    %APPDATA%\RetroArch\cores\FreeIntvTSOverlay_libretro.dll
```

**Quick Method**:
```powershell
# In PowerShell:
Copy-Item `
  s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll `
  "$env:APPDATA\RetroArch\cores\"
```

### Step 4: Ensure BIOS Files Exist
Required files (case-sensitive):
```
Location: C:\Users\[YOUR_USERNAME]\AppData\Roaming\RetroArch\system\

Files:
- exec.bin      (Intellivision ROM cartridge BIOS)
- grom.bin      (Intellivision graphics ROM)
```

**Both files must exist or games won't load.**

### Step 5: Test in RetroArch
1. Start RetroArch
2. Go to **Main Menu** → **Load Core**
3. Select **FreeIntvTSOverlay** from the list
4. Load a game ROM (e.g., `.intv`, `.bin`, `.rom` file)
5. Game should start

### Step 6: Test Fullscreen Button
In the game:
1. Look at the right side of the screen (utility area)
2. Find the **yellow rectangle** or **Button 3** with image
3. Click it to toggle fullscreen mode
4. Game should scale to fill screen with control strip at bottom
5. Click again to return to normal dual-screen view

---

## Testing Fullscreen Features

### Feature 1: Fullscreen Toggle
- **Action**: Click Button 3 (Fullscreen button) in utility area
- **Expected**: Game scales to fill entire screen, control strip (50px) appears at bottom
- **Return**: Click Button 3 again or press related hotkey

### Feature 2: Auto-Hide Control Strip
- **Action**: In fullscreen mode, wait 5 seconds without input
- **Expected**: Control strip slides down out of view
- **Result**: Game occupies full screen

### Feature 3: Strip Reveal by Touch Zone
- **Action**: In fullscreen mode with hidden strip, touch/click bottom 80 pixels of screen
- **Expected**: Control strip slides back up immediately
- **Auto-Hide**: Disappears again after 5 seconds of no input

### Feature 4: Swap Screen Button
- **Action**: Click Button 2 (Swap button) in control strip or utility area
- **Expected**: Game and keypad positions swap (game moves left/right, keypad opposite)
- **Note**: Works in both fullscreen and normal modes

---

## Optional: Custom Button Images

### Preparing PNG Images

**Dimensions**: 200 × 50 pixels (same as existing buttons)  
**Format**: PNG with alpha channel (transparency)  
**Color Space**: RGBA (recommended)

### Placing Custom Images

Folder: `C:\Users\[YOUR_USERNAME]\AppData\Roaming\RetroArch\system\FreeIntvTS_Overlays\`

**Create this folder if it doesn't exist.**

### Supported Filenames

```
button_full_screen_toggle.png      → Button 3 (Fullscreen Toggle)
button_swapscreen.png               → Button 2 (Swap Screen)
{game_name}.png                     → Game-specific overlay
                                     (e.g., "Robotron_2084.png")
default.png                         → Fallback overlay
controller_base.png                 → Keypad background
```

### How They're Used

| Filename | Used For | Priority |
|----------|----------|----------|
| button_full_screen_toggle.png | Fullscreen button image | 1st (if exists) |
| button_swapscreen.png | Swap button image | 1st (if exists) |
| {game_name}.png | Game-specific keypad overlay | 1st (if exists) |
| default.png | Generic overlay for all games | 2nd (fallback) |
| controller_base.png | Keypad background layer | Always |

### Example Setup

```
RetroArch/system/FreeIntvTS_Overlays/
├── controller_base.png
├── button_full_screen_toggle.png
├── button_swapscreen.png
├── default.png
├── Robotron_2084.png              (For this specific game)
├── Defender.png                   (For this specific game)
└── ... more game-specific overlays
```

---

## Troubleshooting Deployment

### Issue: Core doesn't appear in RetroArch
**Solution**:
1. Verify file location: `%APPDATA%\RetroArch\cores\FreeIntvTSOverlay_libretro.dll`
2. Restart RetroArch
3. Go to **Main Menu** → **Online Updater** → **Update Cores**
4. Scroll to find FreeIntvTSOverlay

### Issue: Game ROM won't load
**Solution**:
1. Verify BIOS files exist:
   - `%APPDATA%\RetroArch\system\exec.bin`
   - `%APPDATA%\RetroArch\system\grom.bin`
2. Check file names are exact (case-sensitive on some systems)
3. Verify ROM file format (.intv, .bin, or .rom)
4. Check RetroArch logs for error messages

### Issue: Fullscreen button doesn't appear
**Solution**:
1. Verify game is running
2. Look in utility area (right side of screen, under game)
3. Should be gold rectangle or custom PNG if loaded
4. If missing, check RetroArch settings for overlay display

### Issue: Fullscreen button doesn't work
**Solution**:
1. Verify you're clicking the correct button (Button 3, rightmost)
2. Check that button is highlighted (should show yellow on press)
3. Try with mouse if using touchscreen (might be input issue)
4. Restart RetroArch and try again

### Issue: Auto-hide not working
**Solution**:
1. In fullscreen mode, wait 5 seconds without moving mouse/touching
2. Strip should slide out of view automatically
3. Touch/click bottom 80 pixels of screen to reveal
4. If not working, check RetroArch input settings

---

## Verification Checklist

After deployment, verify the following:

- [ ] DLL copied to `%APPDATA%\RetroArch\cores\`
- [ ] BIOS files exist in `%APPDATA%\RetroArch\system\`
- [ ] RetroArch recognizes FreeIntvTSOverlay core
- [ ] Game ROM loads without crashing
- [ ] Game renders correctly (352×224 scaled 2x)
- [ ] Fullscreen button visible in utility area
- [ ] Clicking fullscreen button works
- [ ] Auto-hide strip appears and hides
- [ ] Touch zone (bottom 80px) reveals strip
- [ ] Swap button (Button 2) works
- [ ] Keypad buttons respond to input

---

## File Locations Summary

| Component | Location | Size |
|-----------|----------|------|
| **Compiled DLL** | `s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll` | 205 KB |
| **Deployment** | `%APPDATA%\RetroArch\cores\FreeIntvTSOverlay_libretro.dll` | 205 KB |
| **BIOS Files** | `%APPDATA%\RetroArch\system\exec.bin` and `grom.bin` | ~64 KB each |
| **Overlays** | `%APPDATA%\RetroArch\system\FreeIntvTS_Overlays\*.png` | Variable |
| **System Folder** | `%APPDATA%\RetroArch\system\` | Contains BIOS & overlays |

---

## Support Files

Detailed documentation available in the project folder:

- **BUILD_COMPLETE_NEXT_STEPS.md** - Build & deployment overview
- **BUILD_TEST_REPORT.md** - Detailed build test results
- **FULLSCREEN_MODE_IMPLEMENTATION.md** - Technical details
- **FULLSCREEN_MODE_VERIFICATION_CHECKLIST.md** - Testing guide
- **UPSTREAM_MERGE_COMPLETED.md** - Upstream integration info

---

## Quick Commands (PowerShell)

### Copy DLL to RetroArch
```powershell
Copy-Item `
  "s:\VisualStudio\FreeIntvTSOverlay\FreeIntvTSOverlay_libretro.dll" `
  "$env:APPDATA\RetroArch\cores\"
```

### Verify DLL Deployment
```powershell
Test-Path "$env:APPDATA\RetroArch\cores\FreeIntvTSOverlay_libretro.dll"
# Should output: True
```

### Verify BIOS Files
```powershell
Test-Path "$env:APPDATA\RetroArch\system\exec.bin"
Test-Path "$env:APPDATA\RetroArch\system\grom.bin"
# Both should output: True
```

### Create Overlays Folder
```powershell
New-Item -ItemType Directory `
  -Force `
  -Path "$env:APPDATA\RetroArch\system\FreeIntvTS_Overlays"
```

---

**Deployment Date**: November 11, 2025  
**Core Version**: FreeIntvTSOverlay with Fullscreen Mode  
**Status**: Ready for Testing  

---

## Next: Start Testing!

1. Copy DLL to RetroArch/cores
2. Verify BIOS files exist
3. Start RetroArch
4. Load game ROM
5. Click fullscreen button to test

**Your FreeIntvTSOverlay emulator with fullscreen mode is ready to use!**
