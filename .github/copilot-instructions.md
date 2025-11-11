# FreeIntvTSOverlay with Android Touchscreen Support - AI Agent Instructions

## Project Overview

FreeIntvTSOverlay is a libretro emulation core for the Mattel Intellivision with full Android touchscreen support, macOS/Windows mouse support, and Linux pointer support. The codebase is C-based (no external dependencies beyond libretro-common) and runs on 10+ platforms via libretro framework.

**Key Architecture**: Libretro plugin core → RetroArch frontend. Dual-screen rendering displays 352×224 game output (2x scaled) alongside an interactive 370×600 keypad overlay on a 1074×600 canvas.

## Core Components & Architecture

### 1. **Emulation Engine** (`src/intv.c`, `src/cp1610.c`, `src/stic.c`, `src/psg.c`)
- **CP1610**: 16-bit CPU with cycle-accurate instruction execution (800+ opcode handlers in cp1610.c)
- **STIC**: Graphics chip renders directly to `extern unsigned int frame[352*224]` (RGBA, 32-bit pixels)
- **PSG/Intellivoice**: Audio synthesized at 44.1kHz, sampled via `Audio(left, right)` callback
- **Memory**: 16-bit address space (0x10000 locations); BIOS at 0x1000-0x37FF, cartridge/RAM above. **Access via `readMem(adr)` and `writeMem(adr, val)` only**—do not access `Memory[]` directly.

**Serialization Pattern**: Each subsystem (`CP1610Serialize`, `STICSerialize`, `PSGSerialize`, `ivoiceSerialize`) writes state to a struct for save states. Serialization version is 0x4f544702; **increment if any serialized struct changes**.

### 2. **Dual-Screen Rendering & Touchscreen UI** (`src/libretro.c`)

**Display Geometry** (defined at top of libretro.c):
- **Total workspace**: 1074 × 600 pixels
- **Game screen**: 704 × 448 (352×224 scaled 2x), positioned LEFT when normal, RIGHT when `display_swap=true`
- **Keypad overlay**: 370 × 600, positioned RIGHT when normal, LEFT when swapped
- **Utility buttons**: 704 × 100 below game, with 7-layer gradient border (gold retro palette)

**Rendering Flow** (`render_dual_screen()`):
1. Clear workspace to black
2. Render 2x-scaled game frame at game_x_offset
3. Draw keypad background + overlay PNG (if loaded) + controller_base PNG (if loaded)
4. Composite utility buttons with PNG images (if loaded); **only button 2 (Swap Screen) is enabled** (all others skip loading/rendering)
5. Draw utility button highlights (yellow overlay when pressed)
6. Draw hotspot highlights (green overlay for touch-active keypad buttons)

**PNG Loading** (uses stb_image):
- `controller_base.png`: Keypad template layer
- `{rom_name}.png`: Game-specific overlay (fallback to `default.png`)
- `{button_name}.png`: Utility button images (only button 2 loaded currently)
- Located in `RetroArch/system/FreeIntvTS_Overlays/`

### 3. **Touchscreen Input** (Android-specific, pointer input on desktop)
**Data flow**: RetroArch pointer callback → `process_hotspot_input()` and `process_utility_button_input()` → controller memory updates

**Input Sources**:
- `InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X/Y)` - Normalized coordinates (-32767 to 32767)
- `InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)` - Touch active flag

**Coordinate Transformation**:
```c
// From normalized (-32767..32767) to pixels (0..WORKSPACE_WIDTH/HEIGHT)
pixel = (normalized + 32767) * workspace_size / 65534
```

**Hotspot System** (12 keypad buttons):
- `overlay_hotspots[12]` array; each hotspot has `.x, .y, .width, .height, .keypad_code, .label`
- When touch hits hotspot: `hotspot_pressed[i]` set → bitwise OR keypad codes → call `setControllerInput(0, combined_code)`
- Hotspot coordinates adjust based on `display_swap` flag
- Button highlights rendered as green semi-transparent overlay

**Utility Buttons** (below game, 6 total):
- `utility_buttons[6]` array with button positions
- Only button 2 (Swap Screen) is functional; others disabled (`if (i != 2) continue;`)
- On press: `display_swap = !display_swap` (swaps game and keypad positions)
- Can optionally load button PNG images; currently renders as gold placeholder rectangles

### 4. **Controller Mapping** (`src/controller.c`, `src/controller.h`)
- **RetroPad input** (joypad array from RetroArch) → `getControllerState()` → Intellivision bit patterns
- **Keypad codes** (defined in controller.h):
  ```c
  K_1=0x81, K_2=0x41, K_3=0x21, K_4=0x82, K_5=0x42, K_6=0x22
  K_7=0x84, K_8=0x44, K_9=0x24, K_0=0x48, K_C=0x88, K_E=0x28
  ```
  Combine with bitwise OR: `code = K_5 | D_N` for "5 + North direction"
- **Direction codes**: `D_N, D_NE, D_E, D_SE, D_S, D_SW, D_W, D_NW` (16-way disc mapping)
- **Action buttons**: Top/Left/Right buttons via bit masks
- **Controller swap**: `controllerSwap` flag swaps player 1/2 input mappings (some games expect controller on different side)
- **Memory-mapped I/O**: Controller state written to `Memory[0x1FE + player]` (XORed with 0xFF for inverted logic)

## Developer Workflows

### Build Targets
```bash
make platform=unix        # Linux (32/64-bit auto-detect)
make platform=win         # Windows (MinGW cross-compile or native MSVC)
make platform=osx         # macOS
make platform=switch      # Nintendo Switch
make platform=vita        # PS Vita
ndk-build -C jni/         # Android (requires NDK)
make -C jni/              # Alternative Android build
```

**Output**: `freeintvtsoverlay_libretro.{so|dll|dylib}` in repo root

### Testing Locally
1. **Get BIOS**: Place `exec.bin` and `grom.bin` (case-sensitive) in RetroArch `system/` folder
2. **Build**: `make platform=unix` (or platform=win for Windows)
3. **Place core**: Copy .so/.dll to `~/.config/RetroArch/cores/` (or equivalent platform path)
4. **Load ROM**: Open any .intv/.bin/.rom file in RetroArch
5. **Test touchscreen** (Android): Touch keypad area; each press should trigger corresponding keypad button
6. **Test swap**: Click Swap Screen button (yellow rectangle in utility area); game and keypad should exchange positions

### Critical Libretro Callbacks (in `retro_run()`)
- `InputPoll()` - Refresh input state
- `getControllerState()` → `setControllerInput()` - Write controller bits to memory
- `process_hotspot_input()` - Handle touchscreen keypad input (sets hotspot_pressed[] and calls setControllerInput)
- `process_utility_button_input()` - Handle utility button touches
- `Run()` - Execute one emulation frame (calls CP1610 cycle loop)
- `render_dual_screen()` - Composite game + keypad + utility buttons to dual_screen_buffer
- `Video(dual_screen_buffer, 1074, 600, pitch)` - Push to RetroArch for display

## Project-Specific Patterns & Conventions

### Memory System
- **Address space**: 16-bit (0x0000-0xFFFF)
- **Access pattern**: Always use `readMem(adr)` and `writeMem(adr, val)` functions (memory.h)
- **Controller I/O**: `Memory[0x1FE]` = player 0 (right), `Memory[0x1FF]` = player 1 (left)
- **Gotcha**: Controller values **inverted** (XOR 0xFF); button press = 0 bit

**Example**: To set "button 5" for player 0:
```c
setControllerInput(0, K_5);  // Not Memory[0x1FE] = K_5; use setControllerInput()
```

### Keypad Codes
**Always reference constants; never hardcode values**:
```c
#define K_1 0x81  // Keypad 1
#define K_5 0x42  // Keypad 5
#define K_C 0x88  // Keypad * (Clear)
#define K_E 0x28  // Keypad # (Enter)
```
Combine with directional/action codes:
```c
int state = K_5 | D_NE | BTN_TOP;  // "5" + "NE direction" + "top action button"
```

### Touchscreen Coordinate System
- **Origin**: (0, 0) at top-left of 1074×600 canvas
- **Game screen**: x ∈ [0, 704) when normal; x ∈ [370, 1074) when swapped
- **Keypad**: x ∈ [704, 1074) when normal; x ∈ [0, 370) when swapped
- **Utility buttons**: y ∈ [448, 600); x follows game x offset
- **Hotspot check**: When processing input, **always account for display_swap** offset adjustments

### Frame Buffer Format
- **Resolution**: 352 × 224 (native Intellivision)
- **Format**: `unsigned int frame[352*224]` (RGBA, 32-bit per pixel, little-endian: 0xAABBGGRR)
- **Rendering**: 2x bilinear scaling in `render_dual_screen()` (simple duplicate pixels)
- **Color example**: Red = 0xFFFF0000 (A=FF, R=FF, G=00, B=00)

### Serialization (Save States)
- **Version constant**: `0x4f544702` (in libretro.c)
- **Pattern**: Each subsystem writes to a `*Serialized` struct; collect into single state file
- **Gotcha**: If you modify any serialized struct, **increment version number**, or old saves won't load
- **Example**: Adding a new field to `CP1610serialized` requires incrementing version + recompiling old saves

### Rendering Pipeline
```c
render_dual_screen():
  1. Clear dual_screen_buffer to black (0xFF000000)
  2. Copy frame[352*224] → workspace, scaled 2x (y/2, x/2)
  3. Composite overlay PNG + controller_base PNG at keypad area
  4. Draw utility buttons with PNG images (only button 2 enabled)
  5. Highlight pressed buttons/hotspots with semi-transparent overlays
  6. Draw 7-layer gold gradient border around utility area
  
Video() is called with:
  - Buffer: dual_screen_buffer (1074*600 pixels)
  - Width: 1074, Height: 600, Pitch: 1074*4 bytes
```

### Libretro Integration
- **Core options**: Define in `libretro_core_options.h`; accessed via:
  ```c
  struct retro_variable var = {0};
  var.key = "option_key";
  if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) { /* use var.value */ }
  ```
- **Video callback**: `Video(frame_ptr, width, height, pitch)` called at end of `retro_run()`
- **Audio callback**: `Audio(left_sample, right_sample)` for 16-bit stereo PCM at 44.1kHz
- **Environment callback**: `Environ(RETRO_ENVIRONMENT_*, ...)` for core info, options, save states, etc.

## Common Tasks & Code Patterns

### Adding a New Keypad Button to Hotspots
1. Define keypad code constant in `controller.h` (if not already defined)
2. Add hotspot to `init_overlay_hotspots()` in libretro.c:
   ```c
   overlay_hotspots[button_index].x = KEYPAD_OFFSET_X + column * HOTSPOT_SPACING;
   overlay_hotspots[button_index].y = KEYPAD_OFFSET_Y + row * HOTSPOT_SPACING;
   overlay_hotspots[button_index].keypad_code = K_NEWBUTTON;
   overlay_hotspots[button_index].label = "NEWBUTTON";
   ```
3. Update `OVERLAY_HOTSPOT_COUNT` if adding more than 12

### Debugging Touchscreen Input
- Check `debug_log()` output in `process_hotspot_input()` and `process_utility_button_input()`
- Raw pointer coordinates (-32767..32767) are logged before pixel transformation
- Hotspot bounding box checks logged for each touch event
- Enable logging by searching for `debug_log(` calls in libretro.c

### Implementing a New Utility Button
1. Add to `utility_buttons[UTILITY_BUTTON_COUNT]` initialization in libretro.c with position/dimensions
2. Add button image filename to `button_filenames[]` array
3. Update `load_utility_buttons()` to load your button (currently only button 2 is loaded; remove `if (i != 2)` skip)
4. Update `process_utility_button_input()` to handle new button action in switch statement
5. Add to `render_dual_screen()` rendering loop (update disabled button skip logic)

### Modifying Save State Format
1. Add new field to corresponding `*Serialized` struct in source file (e.g., `CP1610serialized`)
2. **Increment serialization version** in libretro.c: `0x4f544702 → 0x4f544703`
3. Update both serialization and unserialization functions
4. Test with both old and new save files

## Critical Files for Common Tasks

| Task | Key Files |
|------|-----------|
| Add/fix controller input | `controller.c`, `controller.h`, `intv.c` |
| Touchscreen hotspots | `libretro.c` (process_hotspot_input, init_overlay_hotspots) |
| Utility buttons | `libretro.c` (process_utility_button_input, utility_buttons[]) |
| Rendering/overlays | `libretro.c` (render_dual_screen) |
| CPU emulation | `cp1610.c` (OpCodes[], instruction handlers) |
| Graphics | `stic.c` (STIC registers, frame buffer) |
| Audio | `psg.c`, `ivoice.c` |
| Save states | `libretro.c` (serialize/unserialize), `cp1610.c`, etc. |
| Memory access | `memory.c`, `readMem()`, `writeMem()` |
| Cartridge loading | `cart.c`, `LoadCart()` |

## Known Gotchas & Limitations

1. **Utility button 2 only**: Only "Swap Screen" button is enabled; other 5 buttons skip loading/rendering via `if (i != 2) continue;` checks
2. **PNG assets required**: Core will load default colors/shapes if PNG files missing, but overlays look better with assets
3. **Controller inversion**: Controller state is XORed with 0xFF (inverted logic); button press = 0 bit
4. **Dual hotspot/button coordinates**: When `display_swap=true`, coordinates translate; always account for offset in input code
5. **Memory access**: Never access `Memory[]` directly; always use `readMem()`/`writeMem()`
6. **ECS not supported**: Entertainment Computer System functionality absent; contributions welcome
7. **Intellivoice**: Added but needs testing with specific games
8. **Serialization version**: 0x4f544702 hardcoded; if you change any serialized struct, increment and handle migration

## Build & Deploy

- **Local testing**: Build with `make platform=unix` (or `platform=win`), copy core to RetroArch `cores/`, load BIOS/ROM
- **Libretro upstream**: Sync changes to https://github.com/libretro/FreeIntv (original) or https://github.com/jcarr71/FreeIntvTSOverlay (fork with touchscreen)
- **Core distribution**: Packaged as `freeintvtsoverlay_libretro.{so|dll|dylib}` + `FreeIntvTSOverlay_libretro.info`
- **Asset distribution**: PNG files in `RetroArch/system/FreeIntvTS_Overlays/` folder

---

**Last Updated**: November 2025 | **Touchscreen Feature**: October 2025 | **Comprehensive Update**: November 2025
