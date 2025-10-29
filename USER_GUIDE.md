# FreeIntvTSOverlay User Guide

## Overview
FreeIntvTSOverlay is a libretro core for emulating the Mattel Intellivision, featuring enhanced touchscreen support and a dual-screen overlay UI. This guide explains how to set up and use the core in RetroArch, including instructions for overlay PNG files.

---

## Requirements
- RetroArch (latest recommended)
- FreeIntvTSOverlay core (`freeintvtsoverlay_libretro.so`, `.dll`, or `.dylib`)
- Intellivision BIOS files: `exec.bin`, `grom.bin` (place in RetroArch `system` folder)
- Game ROMs (Intellivision format)
- Overlay PNG files (for custom keypad/utility UI)

---

## Installation
1. **Install RetroArch**
   - Download from https://www.retroarch.com/
2. **Install FreeIntvTSOverlay Core**
   - Place the core file in RetroArch's `cores` directory
   - Load the core via RetroArch's "Load Core" menu
3. **Add BIOS Files**
   - Place `exec.bin` and `grom.bin` in the `system` folder inside your RetroArch directory
4. **Add Game ROMs**
   - Place Intellivision ROM files in your preferred ROMs directory

---

## Using Overlay PNG Files
Overlay PNG files provide custom graphics for the keypad and utility buttons.


### Where to Place Overlay PNG Files
   - Example: `retroarch/system/FreeIntvTS_Overlays/`


## Download Core Assets

You will need to download two separate ZIP files:

1. **Controller Templates & Buttons:**
   - [FreeTS_Overlay Core Assets.zip](https://github.com/jcarr71/FreeIntvTSOverlay/releases/latest/download/FreeTS_Overlay_Core_Assets.zip)
   - This contains the controller template, default keypad, and utility button PNGs.

2. **Overlay PNGs:**
   - [Overlays.zip](https://github.com/jcarr71/FreeIntvTSOverlay/releases/latest/download/Overlays.zip)
   - This contains the actual overlays for individual games.

**Installation Instructions:**
1. Download both ZIP files from the links above.
2. Unzip the contents of each ZIP file.
3. Copy all files and folders from both ZIPs into your `retroarch\system` directory.
   - The overlays should end up in `retroarch\system\FreeIntvTS_Overlays`.
4. Restart RetroArch if it is running.

Your overlays and assets will now be available for use with the FreeIntvTSOverlay core.

## Touchscreen UI

## Touchscreen & Mouse Support
- On Android, touch the keypad area to send input to the emulator.
- On Windows and Linux, you can use the mouse to click on the keypad overlay and utility buttons. Mouse clicks are mapped to touch events, allowing full use of the overlay UI features.
- Utility button (Swap Screen) is available below the game screen.
- The "Swap Screen" button toggles the position of the game and keypad overlays.

---

## Troubleshooting
- If overlays do not appear, verify PNG file location and naming.
- Ensure BIOS files are present in the `system` folder.
- For touchscreen issues, check RetroArch input settings and core options.

---

## Additional Resources
- [RetroArch Documentation](https://docs.libretro.com/)
- [FreeIntvTSOverlay GitHub](https://github.com/jcarr71/FreeIntvTSOverlay)

---

**Last Updated:** October 2025
