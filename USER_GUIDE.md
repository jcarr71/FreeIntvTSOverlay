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
- Place your overlay PNG files in the RetroArch `system` directory under the folder `FreeIntvTS_Overlays`:
   - Example: `retroarch/system/FreeIntvTS_Overlays/`
- Ensure the PNG filename matches the ROM filename (excluding extension).

---

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
