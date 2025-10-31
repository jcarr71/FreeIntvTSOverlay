# FreeIntv with Android Touchscreen Support
FreeIntv is a libretro emulation core for the Mattel Intellivision designed to be compatible with joypads from the SNES era forward even if they originally required a number pad.

**Current Version**: Enhanced with full touchscreen support for Android devices (October 24, 2025)

## Authors & Contributors

FreeIntv was created by David Richardson.
The PSG and STIC emulation was made closer to hardware and optimized by Oscar Toledo G. (nanochess), who also added save states.

The Intellivoice code has been contributed by Joe Zbiciak (author of jzintv), and adapted by Oscar Toledo G. (nanochess)

**Touchscreen Enhancement**: Android touchscreen / Windows mouse / Linux mouse keypad and utility button implementation (October 2025) by Jason Carr

<div style="margin: 18px 0;"><img width="1291" height="727" alt="Screenshot 2025-10-29 135437" src="https://github.com/user-attachments/assets/92d2cafc-44f9-4bfa-aad7-9adfa14d2e0b" style="border:2px solid #FFD700; border-radius:6px;" /></div>
<div style="margin: 18px 0;"><img width="1372" height="770" alt="Screenshot 2025-10-29 135007" src="https://github.com/user-attachments/assets/0aeeada9-423b-406d-9b4f-0081e81c312f" style="border:2px solid #FFD700; border-radius:6px;" /></div>
<div style="margin: 18px 0;"><img width="1365" height="765" alt="Screenshot 2025-10-29 135115" src="https://github.com/user-attachments/assets/725acb8c-2d6b-4170-9893-45f9d2b3f447" style="border:2px solid #FFD700; border-radius:6px;" /></div>

## License
The FreeIntv core is licensed under GPLv2+. More information at https://github.com/libretro/FreeIntv/blob/master/LICENSE

## BIOS
FreeIntv requires two Intellivision BIOS files to be placed in the libretro 'system' folder:

| Function | Filename* | MD5 Hash |
| --- | --- | --- | 
| Executive ROM | `exec.bin`  | `62e761035cb657903761800f4437b8af` |
| Graphics ROM | `grom.bin` | `0cd5946c6473e42e8e4c2137785e427f` |

* BIOS filenames are case-sensitive

## Entertainment Computer System
FreeIntv does not currently support Entertainment Computer System (ECS) functionality. Contributions to the code are welcome!

## Controller overlays

Mattel Intellivision games were often meant to be played with game-specific cards overlaid on the numeric keypad. These overlays convey information which can be very useful in gameplay. Images of a limited selection of Intellivision titles are available at: http://www.intellivisionlives.com/bluesky/games/instructions.shtml

### Overlay PNG Files (Touchscreen)
To use custom keypad and utility overlays with the touchscreen UI, download the required asset files:

1. **Compiled Core Library:**
  - [FreeIntvTSOverlay_libretro.zip](Assets/FreeIntvTSOverlay_libretro.zip)
  - Pre-compiled core binaries for all platforms (Windows DLL, Linux SO, etc.)

2. **Controller Templates & Buttons:**
  - [FreeIntvTS_Overlay Core Assets.zip](Assets/FreeIntvTS_Overlay%20Core%20Assets.zip)
  - Contains the controller template, default keypad, and utility button PNGs.

3. **Overlay PNGs:**
  - [Overlays.zip](Assets/Overlays.zip)
  - Contains the actual overlays for individual games.

**Installation Instructions:**

**Step 1: Download the ZIP files**
- Download all 3 ZIP files from the links above.

**Step 2: Extract and Organize Files**
- Extract `FreeIntvTSOverlay_libretro.zip` → Contains core binaries and `.info` file
- Extract `FreeIntvTS_Overlay Core Assets.zip` → Contains controller template and default keypad
- Extract `Overlays.zip` → Contains game-specific overlays

**Step 3: Install Core Binary**
- Place the core file for your platform into: `RetroArch/cores/`
  - Windows: `FreeIntvTSOverlay_libretro.dll`
  - Linux: `FreeIntvTSOverlay_libretro.so`
  - macOS: `FreeIntvTSOverlay_libretro.dylib`
- ⚠️ **Important:** If the core filename includes a platform identifier in brackets like `[Windows]FreeIntvTSOverlay_libretro.dll`, remove the brackets and platform name so it matches the info file: `FreeIntvTSOverlay_libretro.dll`

**Step 4: Install Info File**
- Copy `FreeIntvTSOverlay_libretro.info` to: `RetroArch/info/`
- ⚠️ **Important:** The info filename must match the core filename exactly (excluding file extension)

**Step 5: Install Overlays**
- Copy all files from both extracted archives to: `RetroArch/system/FreeIntvTS_Overlays/`
  - Game overlays should be named to match your ROM files (e.g., `Astrosmash.png`)

**Step 6: Refresh RetroArch**
- Delete RetroArch's info cache (usually in `RetroArch/system/` or config folder)
- Restart RetroArch to load the new core  

Recommended naming: ensure the PNG filename matches the ROM filename (excluding extension).
### Overlay PNG Files (Touchscreen & Mouse)
To use custom keypad and utility overlays with the touchscreen UI, place your PNG files in the following directory:

  retroarch/system/FreeIntvTS_Overlays/

Recommended naming: ensure the PNG filename matches the ROM filename (excluding extension).

#### Touch and Mouse Input
- On Android, touch the keypad and utility buttons directly.
- On Windows and Linux, you can use the mouse to click on the keypad overlay and utility buttons. Mouse clicks are mapped to touch events, enabling full overlay UI functionality.

## Controls

### Traditional Gamepad Controls
* **Controller Swap** - Some Intellivision games expect the left controller to be player one, others expect the right controller. This isn't a problem if you have two controllers (and don't mind juggling them) but users with only one controller or using a portable setup would be effectively locked out of some games. Controller Swap swaps the two controller interfaces so that the player does not have to physically swap controllers.

### Android Touchscreen Controls *(New)*
* **Touchscreen Keypad** - Touch any of the 12 buttons (1-9, *, 0, #) on the right side of the screen to input commands directly
* **Utility Buttons** - Quick access buttons below the game screen:
  - **Swap Screen** - Toggle game/keypad positions (fully implemented)
* **Visual Feedback** - Button presses show green highlighting on keypad buttons, yellow on utility buttons

### RetroPad Mapping

| RetroPad | FreeIntv Function |
| --- | --- |
| D-Pad| 8-way movement |
| Left Analog Stick | 16-way disc |
| Right Analog Stick | 8-way keypad |
| L3 | Keypad 0 |
| R3 | Keypad 5 |
| L2 | Keypad Clear |
| R2 | Keypad Enter |
| A | Left Action Button |
| B | Right Action Button |
| Y | Top Action Button |
| X | Use the Last Selected Intellivision Keypad Button. In Astrosmash, for example, you can leave "3" selected to enable instant access to hyperspace. |
| L/R | Activate the Mini-Keypad |
| Start | Pause Game |
| Select | Controller Swap |
