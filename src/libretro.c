/*
	This file is part of FreeIntv.

	FreeIntv is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeIntv is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with FreeIntv; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "libretro.h"
#include "libretro_core_options.h"
#include <file/file_path.h>
#include <retro_miscellaneous.h>

#include "intv.h"
#include "cp1610.h"
#include "memory.h"
#include "stic.h"
#include "psg.h"
#include "ivoice.h"
#include "controller.h"
#include "osd.h"

// Include stb_image header (implementation in stb_image_impl.c)
#include "stb_image.h"

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224

// ========================================
// HORIZONTAL LAYOUT DISPLAY CONFIGURATION
// ========================================
// Game Screen: Left side (704×448, 2x scaled from 352×224)
// Utility Buttons: Below game (704×100)
// Keypad: Right side (370×600)
// Total Workspace: 1074 × 600 pixels (keypad full height)

#define WORKSPACE_WIDTH 1074    // Game (704) + Keypad (370)
#define WORKSPACE_HEIGHT 600    // Keypad full height (600px)
#define GAME_SCREEN_WIDTH 704   // 352 * 2x
#define GAME_SCREEN_HEIGHT 448  // 224 * 2x
#define UTILITY_AREA_WIDTH 704  // Same as game width
#define UTILITY_AREA_HEIGHT 100 // Space for 6 buttons in 2 rows
#define KEYPAD_WIDTH 370        // Keypad overlay width
#define KEYPAD_HEIGHT 600       // Keypad overlay height
#define UTILITY_BUTTON_WIDTH 60
#define UTILITY_BUTTON_HEIGHT 50

// Keypad hotspot configuration
#define OVERLAY_HOTSPOT_COUNT 12
#define OVERLAY_HOTSPOT_SIZE 70

// RetroArch utility button codes
#define RETROARCH_MENU 1000
#define RETROARCH_PAUSE 1001
#define RETROARCH_REWIND 1002
#define RETROARCH_SAVE 1003
#define RETROARCH_LOAD 1004
#define RETROARCH_SWAP_OVERLAY 1005
#define RETROARCH_QUIT 1006
#define RETROARCH_RESET 1007
#define RETROARCH_SCREENSHOT 1008
#define RETROARCH_TOGGLE_DISPLAY 1009

#define UTILITY_BUTTON_COUNT 6
#define MENU_BUTTON_WIDTH 200
#define MENU_BUTTON_HEIGHT 50

// NOTE: Keypad codes are defined in controller.c - DO NOT redefine here!
// Using correct codes from controller.c:
// extern from controller.c: K_1=0x81, K_2=0x41, K_3=0x21, K_4=0x82, K_5=0x42, K_6=0x22
// K_7=0x84, K_8=0x44, K_9=0x24, K_0=0x48, K_C=0x88, K_E=0x28
// These are declared in controller.c and used by setControllerInput()

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* label;
    int command;  // RetroArch command code
} utility_button_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int id;
    int keypad_code;
} overlay_hotspot_t;

overlay_hotspot_t overlay_hotspots[OVERLAY_HOTSPOT_COUNT];

// Utility buttons positioned in 704×152 utility workspace (below game screen)
// Layout: 2 rows × 3 columns, menu_button.png placeholders (200×50 each)
// Available space: X=0-704 (704px = same as game width), Y=448-600 (152px height)
// Evenly distributed with 5px gaps: 3*200 + 2*5 = 610px, margins = (704-610)/2 = 47px
// Vertically: 2*50 + 1*5 = 105px, margins = (152-105)/2 = 23.5px
static utility_button_t utility_buttons[UTILITY_BUTTON_COUNT] = {
    // Row 1 (Y=471) - 3 buttons with 5px horizontal gaps
    {44, 471, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Menu", RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO},           // Menu button - command placeholder
    {249, 471, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Quit", 0},                                              // Quit button - command placeholder
    {454, 471, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Swap", 0},                                              // Swap button - command placeholder
    // Row 2 (Y=526) - 3 buttons with 5px horizontal gaps
    {44, 526, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Save", 0},                                               // Save button - command placeholder
    {249, 526, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Load", 0},                                               // Load button - command placeholder
    {454, 526, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Screenshot", 0}                                          // Screenshot button - command placeholder
};

// Display system variables
static int dual_screen_enabled = 1;
static void* dual_screen_buffer = NULL;
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;
static int display_swap = 0;  // 0 = game left/keypad right, 1 = game right/keypad left

// Hotspot input tracking
static int hotspot_pressed[OVERLAY_HOTSPOT_COUNT] = {0};  // Track which hotspots are currently pressed
static int hotspot_hold_frames[OVERLAY_HOTSPOT_COUNT] = {0};  // Hold button presses for multiple frames
#define BUTTON_HOLD_FRAMES 3  // Minimum frames to keep button active after touch ends

// Utility button input tracking
static int utility_button_pressed[UTILITY_BUTTON_COUNT] = {0};  // Track which buttons are currently pressed

// PNG overlay system
static char current_rom_path[512] = {0};
static char system_dir[512] = {0};
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;

// Debug logging to file
static FILE* debug_log_file = NULL;
static void debug_log(const char* format, ...) {
    if (!debug_log_file) {
        // Try multiple paths
        const char* paths[] = {
            "/storage/emulated/0/Download/freeintv_debug.log",
            "/data/local/tmp/freeintv_debug.log",
            "/sdcard/freeintv_debug.log",
            "/storage/3861-3938/freeintv_debug.log"
        };
        for (int i = 0; i < 4; i++) {
            debug_log_file = fopen(paths[i], "a");
            if (debug_log_file) {
                fprintf(debug_log_file, "[LOG STARTED] Path: %s\n", paths[i]);
                fflush(debug_log_file);
                break;
            }
        }
    }
    if (debug_log_file) {
        va_list args;
        va_start(args, format);
        vfprintf(debug_log_file, format, args);
        fprintf(debug_log_file, "\n");
        fflush(debug_log_file);
        va_end(args);
    }
}
static int overlay_width = 370;
static int overlay_height = 600;

// Controller base
static unsigned int* controller_base = NULL;
static int controller_base_loaded = 0;
static int controller_base_width = 446;
static int controller_base_height = 620;

// Individual button images for utility buttons (6 separate PNGs)
// Top row: button_ra_menu.png, button_quit.png, button_swapscreen.png
// Bottom row: button_save.png, button_load.png, button_screenshot.png
static struct {
    unsigned int* buffer;
    int loaded;
    int width;
    int height;
} utility_button_images[UTILITY_BUTTON_COUNT] = {
    {NULL, 0, 0, 0},  // Button 0: button_ra_menu.png
    {NULL, 0, 0, 0},  // Button 1: button_quit.png
    {NULL, 0, 0, 0},  // Button 2: button_swapscreen.png
    {NULL, 0, 0, 0},  // Button 3: button_save.png
    {NULL, 0, 0, 0},  // Button 4: button_load.png
    {NULL, 0, 0, 0}   // Button 5: button_screenshot.png
};

static const char* button_filenames[UTILITY_BUTTON_COUNT] = {
    "button_ra_menu.png",
    "button_quit.png",
    "button_swapscreen.png",
    "button_save.png",
    "button_load.png",
    "button_screenshot.png"
};

// Initialize overlay hotspots for keypad (positioned on RIGHT side)
static void init_overlay_hotspots(void)
{
    printf("[INIT] Initializing overlay hotspots (horizontal layout)...\n");
    fflush(stdout);
    
    // Layout: 4 rows x 3 columns, positioned on RIGHT side of workspace
    int hotspot_w = OVERLAY_HOTSPOT_SIZE;
    int hotspot_h = OVERLAY_HOTSPOT_SIZE;
    int gap_x = 28;
    int gap_y = 29;
    int rows = 4;
    int cols = 3;
    
    // Position keypad on right side: start at GAME_SCREEN_WIDTH
    int keypad_x_offset = GAME_SCREEN_WIDTH;
    int keypad_y_offset = 0;  // Align to top of keypad region
    
    // IMPORTANT: Controller base is 446px wide, centered in 370px keypad space
    // This creates a left/right margin of (370 - 446) / 2 = -38px (extends beyond)
    // Hotspots must account for this centering offset
    int ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;  // = -38
    
    // Center hotspots within the ACTUAL controller base (446px), not the keypad space
    int hotspots_width = 3 * hotspot_w + 2 * gap_x;  // 266
    int hotspots_x_in_base = (controller_base_width - hotspots_width) / 2;  // center in 446px
    int top_margin = 183;  // From DS version: hotspots start 183px from top of workspace
    
    int start_x = keypad_x_offset + ctrl_base_x_offset + hotspots_x_in_base;
    int start_y = keypad_y_offset + top_margin;
    
    int keypad_map[12] = { K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_C, K_0, K_E };
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = row * cols + col;
            overlay_hotspots[idx].x = start_x + col * (hotspot_w + gap_x);
            overlay_hotspots[idx].y = start_y + row * (hotspot_h + gap_y);
            overlay_hotspots[idx].width = hotspot_w;
            overlay_hotspots[idx].height = hotspot_h;
            overlay_hotspots[idx].id = idx + 1;
            overlay_hotspots[idx].keypad_code = keypad_map[idx];
            printf("[INIT] Hotspot %d: pos=(%d,%d), size=%dx%d, keypad_code=0x%02X\n",
                   idx, overlay_hotspots[idx].x, overlay_hotspots[idx].y,
                   overlay_hotspots[idx].width, overlay_hotspots[idx].height,
                   overlay_hotspots[idx].keypad_code);
        }
    }
    printf("[INIT] Hotspot initialization complete!\n");
    fflush(stdout);
}

// Helper function to build system overlay path (handles both Windows \\ and Android / paths)
static void build_system_overlay_path(char* out_path, size_t out_size, const char* filename)
{
    if (!out_path || !system_dir[0] || !filename) {
        printf("[DEBUG] build_system_overlay_path: out_path=%p, system_dir_empty=%d, filename=%p\n", 
               out_path, !system_dir[0], filename);
        return;
    }
    
    // Detect path separator based on system_dir content
    char sep = strchr(system_dir, '/') ? '/' : '\\';
    
    // Check if system_dir already has trailing separator
    size_t dir_len = strlen(system_dir);
    char last_char = system_dir[dir_len - 1];
    int has_trailing_sep = (last_char == '/' || last_char == '\\');
    
    if (has_trailing_sep) {
        snprintf(out_path, out_size, "%sfreeintvds-overlays%c%s", system_dir, sep, filename);
    } else {
        snprintf(out_path, out_size, "%s%cfreeintvds-overlays%c%s", system_dir, sep, sep, filename);
    }
    
    printf("[DEBUG] Built overlay path: %s\n", out_path);
    printf("[DEBUG]   sep='%c' | trailing_sep=%d | filename=%s\n", sep, has_trailing_sep, filename);
    
    // Try to open file to verify it exists
    FILE* test = fopen(out_path, "rb");
    if (test) {
        fclose(test);
        printf("[DEBUG]   File status: EXISTS ✓\n");
    } else {
        printf("[DEBUG]   File status: NOT FOUND (errno=%d)\n", errno);
    }
}

// Forward declaration for build_overlay_path (ROM-specific overlay)
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size);

// Load controller base PNG
static void load_controller_base(void)
{
    if (controller_base_loaded || !system_dir[0]) {
        return;
    }
    
    char base_path[512];
    build_system_overlay_path(base_path, sizeof(base_path), "controller_base.png");
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(base_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        build_system_overlay_path(base_path, sizeof(base_path), "default.png");
        img_data = stbi_load(base_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[CONTROLLER] Loaded controller base: %dx%d\n", width, height);
        controller_base_width = width;
        controller_base_height = height;
        
        if (!controller_base) {
            controller_base = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        }
        
        if (controller_base) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    controller_base[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            controller_base_loaded = 1;
            stbi_image_free(img_data);
            printf("[CONTROLLER] Controller base loaded successfully\n");
        }
    }
}

// Load individual button images for utility buttons
static void load_utility_buttons(void)
{
    if (!system_dir[0]) {
        return;
    }
    
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        // DISABLED: Only load swap screen button (button 2)
        // All other utility buttons are disabled
        if (i != 2) {
            // Skip loading for disabled buttons
            continue;
        }
        
        if (utility_button_images[i].loaded) {
            continue;  // Already loaded
        }
        
        char btn_path[512];
        build_system_overlay_path(btn_path, sizeof(btn_path), button_filenames[i]);
        
        int width, height, channels;
        unsigned char* img_data = stbi_load(btn_path, &width, &height, &channels, 4);
        
        if (img_data) {
            printf("[UTILITY_BUTTON] Loaded button %d (%s): %dx%d\n", i, button_filenames[i], width, height);
            utility_button_images[i].width = width;
            utility_button_images[i].height = height;
            
            if (!utility_button_images[i].buffer) {
                utility_button_images[i].buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
            }
            
            if (utility_button_images[i].buffer) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        unsigned char* pixel = img_data + (y * width + x) * 4;
                        unsigned int alpha = pixel[3];
                        unsigned int r = pixel[0];
                        unsigned int g = pixel[1];
                        unsigned int b = pixel[2];
                        utility_button_images[i].buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                    }
                }
                utility_button_images[i].loaded = 1;
                stbi_image_free(img_data);
                printf("[UTILITY_BUTTON] Button %d loaded successfully\n", i);
            }
        } else {
            printf("[UTILITY_BUTTON] Failed to load %s from %s\n", button_filenames[i], btn_path);
        }
    }
}

// Cleanup utility button images
static void cleanup_utility_buttons(void)
{
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        if (utility_button_images[i].buffer) {
            free(utility_button_images[i].buffer);
            utility_button_images[i].buffer = NULL;
        }
        utility_button_images[i].loaded = 0;
        utility_button_images[i].width = 0;
        utility_button_images[i].height = 0;
    }
}

// Build overlay path from ROM name
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0 || system_dir[0] == '\0') {
        overlay_path[0] = '\0';
        return;
    }
    
    const char* filename = rom_path;
    const char* p = rom_path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    const char* ext = filename;
    const char* q = filename;
    while (*q) {
        if (*q == '.') {
            ext = q;
        }
        q++;
    }
    size_t name_len = ext - filename;
    
    // Detect path separator based on system_dir content
    char sep = strchr(system_dir, '/') ? '/' : '\\';
    snprintf(overlay_path, overlay_path_size, 
             "%s%cfreeintvds-overlays%c%.*s.png",
             system_dir, sep, sep, (int)name_len, filename);
    printf("[DEBUG] ROM overlay path (sep=%c): %s\n", sep, overlay_path);
}

// Load overlay for ROM
static void load_overlay_for_rom(const char* rom_path)
{
    if (!rom_path || !dual_screen_enabled) return;
    
    char overlay_path[1024];
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    
    overlay_loaded = 0;
    
    if (overlay_buffer) {
        free(overlay_buffer);
        overlay_buffer = NULL;
    }
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        char jpg_path[1024];
        strncpy(jpg_path, overlay_path, sizeof(jpg_path) - 1);
        char* ext = strrchr(jpg_path, '.');
        if (ext) {
            strcpy(ext, ".jpg");
            img_data = stbi_load(jpg_path, &width, &height, &channels, 4);
        }
    }
    
    if (!img_data && system_dir[0] != '\0') {
        char default_path[1024];
        build_system_overlay_path(default_path, sizeof(default_path), "default.png");
        img_data = stbi_load(default_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[OVERLAY] Loaded overlay: %dx%d\n", width, height);
        overlay_width = width;
        overlay_height = height;
        overlay_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    overlay_buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            init_overlay_hotspots();
        }
        stbi_image_free(img_data);
    } else {
        overlay_width = 370;
        overlay_height = 600;
        overlay_buffer = (unsigned int*)malloc(overlay_width * overlay_height * sizeof(unsigned int));
        if (overlay_buffer) {
            for (int y = 0; y < overlay_height; y++) {
                for (int x = 0; x < overlay_width; x++) {
                    if (y < overlay_height / 2 && x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF0000FF;
                    else if (y < overlay_height / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF00FF00;
                    else if (x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFFFF0000;
                    else
                        overlay_buffer[y * overlay_width + x] = 0xFFFFFFFF;
                }
            }
        }
    }
    
    overlay_loaded = 1;
    strncpy(current_rom_path, rom_path, sizeof(current_rom_path) - 1);
}


// Render display with game screen LEFT and keypad RIGHT
static void render_dual_screen(void)
{
    if (!dual_screen_enabled) return;
    
    if (!dual_screen_buffer) {
        dual_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
    }
    if (!dual_screen_buffer) return;
    
    unsigned int* dual_buffer = (unsigned int*)dual_screen_buffer;
    extern unsigned int frame[352 * 224];
    
    // Clear entire workspace with black
    for (int i = 0; i < WORKSPACE_WIDTH * WORKSPACE_HEIGHT; i++) {
        dual_buffer[i] = 0xFF000000;
    }
    
    // Determine screen positions based on display_swap setting
    int game_x_offset = display_swap ? KEYPAD_WIDTH : 0;
    int keypad_x_offset = display_swap ? 0 : GAME_SCREEN_WIDTH;
    
    // === UTILITY SECTION BACKGROUND (drawn first, behind buttons) ===
    // Add a more visible background to the utility area
    int util_bg_x1 = game_x_offset;
    int util_bg_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    int util_bg_y1 = 448;
    int util_bg_y2 = 600;
    
    // More visible dark background color - dark blue with better contrast than near-black
    unsigned int util_bg_color = 0xFF1a2a3a;  // Dark blue-gray with visible contrast to black
    
    for (int y = util_bg_y1; y < util_bg_y2; y++) {
        if (y >= WORKSPACE_HEIGHT) break;
        for (int x = util_bg_x1; x < util_bg_x2; x++) {
            if (x < WORKSPACE_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = util_bg_color;
            }
        }
    }
    
    // === GAME SCREEN ===
    for (int y = 0; y < GAME_SCREEN_HEIGHT; ++y) {
        int src_y = y / 2;
        for (int x = 0; x < GAME_SCREEN_WIDTH; ++x) {
            int src_x = x / 2;
            int workspace_x = game_x_offset + x;
            
            if (workspace_x >= WORKSPACE_WIDTH) continue;
            
            if (src_y < GAME_HEIGHT && src_x < GAME_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = frame[src_y * GAME_WIDTH + src_x];
            } else {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000;
            }
        }
    }
    
    // === KEYPAD ===
    // Background for keypad area
    unsigned int bg_color = 0xFF1a1a1a;
    for (int y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = keypad_x_offset + x;
            if (workspace_x < WORKSPACE_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = bg_color;
            }
        }
    }
    
    // Layer overlay and controller base
    int ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;
    int overlay_x_offset = (KEYPAD_WIDTH - overlay_width) / 2;
    
    for (int y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = keypad_x_offset + x;
            int workspace_y = y;
            
            if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
            
            unsigned int pixel = bg_color;
            
            // Layer game overlay (back)
            if (overlay_buffer && y < overlay_height) {
                int overlay_x = x - overlay_x_offset;
                if (overlay_x >= 0 && overlay_x < overlay_width) {
                    unsigned int overlay_pixel = overlay_buffer[y * overlay_width + overlay_x];
                    if ((overlay_pixel >> 24) & 0xFF) {
                        pixel = overlay_pixel;
                    }
                }
            }
            
            // Layer controller base (front)
            if (controller_base_loaded && controller_base && y < controller_base_height) {
                int ctrl_x = x - ctrl_base_x_offset;
                if (ctrl_x >= 0 && ctrl_x < controller_base_width) {
                    unsigned int base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    if ((base_pixel >> 24) & 0xFF) {
                        pixel = base_pixel;
                    }
                }
            }
            
            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = pixel;
        }
    }
    
    // === UTILITY BUTTONS (BELOW game screen, move with game when swapped) ===
    // Draw utility button PNG images
    int buttons_loaded = 0;
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        if (utility_button_images[i].loaded) {
            buttons_loaded++;
        }
    }
    
    if (buttons_loaded > 0) {
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            // DISABLED: Only render swap screen button (button 2)
            // All other utility buttons are disabled and not rendered
            if (i != 2) {
                continue;
            }
            
            if (!utility_button_images[i].loaded || !utility_button_images[i].buffer) {
                continue;
            }
            
            utility_button_t* btn = &utility_buttons[i];
            int img_width = utility_button_images[i].width;
            int img_height = utility_button_images[i].height;
            
            // Apply game_x_offset to utility button position (buttons move with game)
            int button_x_offset = game_x_offset;
            
            // Blit individual button texture to workspace
            for (int img_y = 0; img_y < img_height; img_y++) {
                for (int img_x = 0; img_x < img_width; img_x++) {
                    int workspace_x = button_x_offset + btn->x + img_x;
                    int workspace_y = btn->y + img_y;
                    
                    if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                    if (workspace_x < 0) continue;
                    
                    unsigned int button_pixel = utility_button_images[i].buffer[img_y * img_width + img_x];
                    unsigned int alpha = (button_pixel >> 24) & 0xFF;
                    
                    if (alpha > 0) {
                        // Blend with alpha
                        if (alpha == 255) {
                            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                        } else {
                            // Alpha blend
                            unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                            unsigned int inv_alpha = 255 - alpha;
                            
                            unsigned int r = ((button_pixel >> 16) & 0xFF);
                            unsigned int g = ((button_pixel >> 8) & 0xFF);
                            unsigned int b = (button_pixel & 0xFF);
                            
                            unsigned int existing_r = ((existing >> 16) & 0xFF);
                            unsigned int existing_g = ((existing >> 8) & 0xFF);
                            unsigned int existing_b = (existing & 0xFF);
                            
                            unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
        }
        
        // === UTILITY BUTTON HIGHLIGHTING WHEN PRESSED ===
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            // DISABLED: Only highlight swap screen button (button 2)
            // All other utility buttons are disabled
            if (i != 2) {
                continue;
            }
            
            if (utility_button_pressed[i]) {
                utility_button_t* btn = &utility_buttons[i];
                unsigned int highlight_color = 0x88FFFF00;  // Yellow semi-transparent highlight
                
                // Apply game_x_offset to highlight position (buttons move with game)
                int button_x_offset = game_x_offset;
                
                for (int y = btn->y; y < btn->y + btn->height; ++y) {
                    if (y >= WORKSPACE_HEIGHT) continue;
                    for (int x = button_x_offset + btn->x; x < button_x_offset + btn->x + btn->width; ++x) {
                        if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                        
                        unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                        unsigned int alpha = (highlight_color >> 24) & 0xFF;
                        unsigned int inv_alpha = 255 - alpha;
                        
                        unsigned int r = ((highlight_color >> 16) & 0xFF);
                        unsigned int g = ((highlight_color >> 8) & 0xFF);
                        unsigned int b = (highlight_color & 0xFF);
                        
                        unsigned int existing_r = ((existing >> 16) & 0xFF);
                        unsigned int existing_g = ((existing >> 8) & 0xFF);
                        unsigned int existing_b = (existing & 0xFF);
                        
                        unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                        unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                        unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                        
                        dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                    }
                }
            }
        }
    } else {
        // Fallback: Draw gold rectangles if utility buttons not loaded
        unsigned int utility_color = 0xFFFFD700;
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            utility_button_t* btn = &utility_buttons[i];
            for (int y = btn->y; y < btn->y + btn->height; ++y) {
                if (y >= WORKSPACE_HEIGHT) break;
                for (int x = btn->x; x < btn->x + btn->width; ++x) {
                    if (x >= WORKSPACE_WIDTH) break;
                    dual_buffer[y * WORKSPACE_WIDTH + x] = utility_color;
                }
            }
        }
    }
    
    // === UTILITY SECTION BORDER - 7 LAYER GRADIENT WITH 45° CORNERS (gold retro palette) ===
    // Colors from outside to inside: #605117, #927b18, #c7a814, #ffd700, #c7a814, #927b18, #605117
    int util_border_x1 = game_x_offset;
    int util_border_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    int util_border_y1 = 448;
    int util_border_y2 = 600;
    
    // 7-layer color palette (ARGB format with full opacity)
    unsigned int border_colors[7] = {
        0xFF605117,  // Layer 0 (outermost): Dark gold/brown
        0xFF927b18,  // Layer 1: Medium-dark gold
        0xFFc7a814,  // Layer 2: Medium gold
        0xFFffd700,  // Layer 3 (center): Bright gold
        0xFFc7a814,  // Layer 4: Medium gold (mirror)
        0xFF927b18,  // Layer 5: Medium-dark gold (mirror)
        0xFF605117   // Layer 6 (innermost): Dark gold/brown (mirror)
    };
    
    // Draw each layer from outside to inside
    for (int layer = 0; layer < 7; layer++) {
        int offset = layer;
        unsigned int color = border_colors[layer];
        int corner_cut = offset;  // Amount to cut corners at 45° angle
        
        // Top border line
        for (int y = util_border_y1 + offset; y < util_border_y1 + offset + 1; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom border line
        for (int y = util_border_y2 - offset - 1; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Left border line
        for (int y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + offset; x < util_border_x1 + offset + 1; x++) {
                if (x >= 0 && x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Right border line
        for (int y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x2 - offset - 1; x < util_border_x2 - offset; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Top-left 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Top-right 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom-left 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom-right 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
    }
    
    // === HOTSPOT HIGHLIGHTING - Show which buttons are pressed by touch ===
    // Highlight all pressed hotspots (from touch input detection)
    // When display_swap is true, hotspots translate from right side to left side
    int hotspot_x_adjust = display_swap ? (-GAME_SCREEN_WIDTH) : 0;
    
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
        if (hotspot_pressed[i]) {
            overlay_hotspot_t *h = &overlay_hotspots[i];
            unsigned int highlight_color = 0xAA00FF00;  // Green highlight for touch-pressed
            
            for (int y = h->y; y < h->y + h->height; ++y) {
                if (y >= WORKSPACE_HEIGHT) continue;
                for (int x = h->x + hotspot_x_adjust; x < h->x + h->width + hotspot_x_adjust; ++x) {
                    if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                    
                    unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                    unsigned int alpha = (highlight_color >> 24) & 0xFF;
                    unsigned int inv_alpha = 255 - alpha;
                    
                    unsigned int r = ((highlight_color >> 16) & 0xFF);
                    unsigned int g = ((highlight_color >> 8) & 0xFF);
                    unsigned int b = (highlight_color & 0xFF);
                    
                    unsigned int existing_r = ((existing >> 16) & 0xFF);
                    unsigned int existing_g = ((existing >> 8) & 0xFF);
                    unsigned int existing_b = (existing & 0xFF);
                    
                    unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                    unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                    unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                    
                    dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                }
            }
        }
    }
}

char *SystemPath;

retro_environment_t Environ;
retro_video_refresh_t Video;
retro_audio_sample_t Audio;
retro_audio_sample_batch_t AudioBatch;
retro_input_poll_t InputPoll;
retro_input_state_t InputState;

void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
void retro_set_audio_sample(retro_audio_sample_t fn) { Audio = fn; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t fn) { AudioBatch = fn; }
void retro_set_input_poll(retro_input_poll_t fn) { InputPoll = fn; }
void retro_set_input_state(retro_input_state_t fn) { InputState = fn; }

// Forward declarations
void quit(int state);

// ========================================
// HOTSPOT INPUT HANDLING
// ========================================

// Process utility button touchscreen input and trigger RetroArch commands
static void process_utility_button_input(void)
{
    static int call_count = 0;
    call_count++;
    if (call_count % 100 == 0) {
        debug_log("[UTILITY_INPUT] Function called %d times", call_count);
    }
    
    // Get pointer/touchscreen input (RETRO_DEVICE_POINTER for touchscreen, RETRO_DEVICE_MOUSE for mouse)
    int16_t ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    int mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates (-32767 to 32767) to pixel coordinates (0 to WORKSPACE_WIDTH/HEIGHT)
    int mouse_x = 0;
    int mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
    }
    
    static int last_mouse_x = -1;
    static int last_mouse_y = -1;
    static int coord_logged = 0;
    
    // Log coordinates on change or button press
    if (mouse_button && (!coord_logged || mouse_x != last_mouse_x || mouse_y != last_mouse_y)) {
        debug_log("[UTILITY] TOUCH DETECTED! Raw: x_norm=%d y_norm=%d -> Transformed: x=%d y=%d button=%d", 
                  ptr_x_normalized, ptr_y_normalized, mouse_x, mouse_y, mouse_button);
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            utility_button_t* btn = &utility_buttons[i];
            int is_over = (mouse_x >= btn->x && mouse_x < btn->x + btn->width &&
                          mouse_y >= btn->y && mouse_y < btn->y + btn->height);
            debug_log("  Btn%d [x=%d-%d y=%d-%d]: %s", i, btn->x, btn->x+btn->width, btn->y, btn->y+btn->height, is_over?"HIT":"miss");
        }
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        coord_logged = 1;
    }
    if (!mouse_button) coord_logged = 0;
    
    // Track pressed buttons
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++)
    {
        // DISABLED: Only process swap screen button (button 2)
        // All other utility buttons are disabled
        if (i != 2) {
            // Clear pressed state for disabled buttons
            utility_button_pressed[i] = 0;
            continue;
        }
        
        utility_button_t* btn = &utility_buttons[i];
        
        // When display_swap is true, utility buttons move with game (apply same offset as rendering)
        // Use the same offset calculation as the rendering: display_swap ? KEYPAD_WIDTH : 0
        int button_x = (display_swap ? KEYPAD_WIDTH : 0) + btn->x;
        
        // Check if mouse is over this button
        int is_over = (mouse_x >= button_x && mouse_x < button_x + btn->width &&
                       mouse_y >= btn->y && mouse_y < btn->y + btn->height);
        
        if (is_over && mouse_button)
        {
            // Button was pressed/held over this button
            if (!utility_button_pressed[i])
            {
                // Button press detected - trigger command
                utility_button_pressed[i] = 1;
                
                // Execute button-specific action
                switch(i)
                {
                    case 2:  // Swap screen button
                        debug_log("[BUTTON] Swap screen button pressed at x=%d y=%d", mouse_x, mouse_y);
                        display_swap = !display_swap;
                        break;
                }
            }
        }
        else
        {
            // Button released or mouse moved away
            if (utility_button_pressed[i])
            {
                utility_button_pressed[i] = 0;
                printf("[BUTTON] Button %d released\n", i);
            }
        }
    }
}

// Process hotspot input and update controller state directly
static void process_hotspot_input(void)
{
    static int call_count = 0;
    call_count++;
    
    // Get pointer/touchscreen input (RETRO_DEVICE_POINTER for touchscreen on Android)
    // Pointer returns coordinates in -32767 to 32767 range (normalized, not pixel coords)
    int16_t ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    int mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates (-32767 to 32767) to pixel coordinates (0 to WORKSPACE_WIDTH/HEIGHT)
    // Formula: pixel = (normalized + 32767) / 65534 * workspace_size
    // This maps -32767 -> 0, 0 -> 50% of screen, 32767 -> 100%
    int mouse_x = 0;
    int mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        // Transform coordinates
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
        
        // Log HOTSPOT activity
        debug_log("[HOTSPOT_INPUT] Call#%d: mouse_x=%d, mouse_y=%d, button=%d, ptr_x_norm=%d, ptr_y_norm=%d",
                  call_count, mouse_x, mouse_y, mouse_button, ptr_x_normalized, ptr_y_normalized);
    }
    
    // Track pressed hotspots
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        overlay_hotspot_t* h = &overlay_hotspots[i];
        
        // When display_swap is true, keypad moves to LEFT (0) and game moves to RIGHT (370)
        // Hotspots are defined with keypad on RIGHT (x starts at 704), so translate them
        // In normal mode: hotspot at original x position
        // In swapped mode: subtract GAME_SCREEN_WIDTH (704) to move to LEFT side
        int hotspot_x = h->x;
        if (display_swap) {
            // Translate hotspot from RIGHT side to LEFT side
            // Original x is ~750-883 (right side), new x should be ~46-179 (left side, same relative position)
            hotspot_x = h->x - GAME_SCREEN_WIDTH;
        }
        
        // Check if mouse is over this hotspot
        int is_over = (mouse_x >= hotspot_x && mouse_x < hotspot_x + h->width &&
                       mouse_y >= h->y && mouse_y < h->y + h->height);
        
        if (is_over && mouse_button)
        {
            // Button was pressed/held over this hotspot
            if (!hotspot_pressed[i])
            {
                // Button press detected - send keypad code
                hotspot_pressed[i] = 1;
                hotspot_hold_frames[i] = BUTTON_HOLD_FRAMES;  // Start hold counter
                debug_log("[HOTSPOT_PRESS] Button %d (idx=%d) pressed at (%d,%d) code=0x%02X",
                          i, i, mouse_x, mouse_y, h->keypad_code);
            }
        }
        else
        {
            // Button released or mouse moved away
            if (hotspot_pressed[i])
            {
                hotspot_pressed[i] = 0;
                debug_log("[HOTSPOT_RELEASE] Button %d released", i);
            }
        }
    }
    
    // Build controller input from pressed hotspots (including held buttons from previous frames)
    int hotspot_input = 0;
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        // Keep button active if currently pressed OR if hold counter is active
        if (hotspot_pressed[i] || hotspot_hold_frames[i] > 0)
        {
            hotspot_input |= overlay_hotspots[i].keypad_code;
            if (hotspot_hold_frames[i] > 0) {
                hotspot_hold_frames[i]--;
            }
            debug_log("[HOTSPOT_COMBINE] Button %d: code=0x%02X, combined=0x%02X, hold_frames=%d",
                      i, overlay_hotspots[i].keypad_code, hotspot_input, hotspot_hold_frames[i]);
        }
    }
    
    // Send hotspot input directly to controller 0 (player 1)
    if (hotspot_input)
    {
        debug_log("[HOTSPOT_SEND] hotspot_input=0x%02X -> setControllerInput(0, 0x%02X)",
                  hotspot_input, hotspot_input);
        setControllerInput(0, hotspot_input);
    }
}

struct retro_game_geometry Geometry;

static bool libretro_supports_option_categories = false;

int joypad0[20]; // joypad 0 state
int joypad1[20]; // joypad 1 state
int joypre0[20]; // joypad 0 previous state
int joypre1[20]; // joypad 1 previous state

bool paused = false;

bool keyboardChange = false;
bool keyboardDown = false;
int  keyboardState = 0;

// at 44.1khz, read 735 samples (44100/60) 
// at 48khz, read 800 samples (48000/60)
// e.g. audioInc = 3733.5 / 735
int audioSamples = AUDIO_FREQUENCY / 60;

double audioBufferPos = 0.0;
double audioInc;

double ivoiceBufferPos = 0.0;
double ivoiceInc;

unsigned int frameWidth = MaxWidth;
unsigned int frameHeight = MaxHeight;
unsigned int frameSize =  MaxWidth * MaxHeight; //78848

void quit(int state)
{
	cleanup_utility_buttons();
	Reset();
	MemoryInit();
}

static void Keyboard(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers)
{
	/* Keyboard Input */
	keyboardDown = down;
	keyboardChange = true; 
	switch (character)
	{
		case 48: keyboardState = keypadStates[10]; break; // 0
		case 49: keyboardState = keypadStates[0]; break; // 1
		case 50: keyboardState = keypadStates[1]; break; // 2
		case 51: keyboardState = keypadStates[2]; break; // 3
		case 52: keyboardState = keypadStates[3]; break; // 4
		case 53: keyboardState = keypadStates[4]; break; // 5
		case 54: keyboardState = keypadStates[5]; break; // 6
		case 55: keyboardState = keypadStates[6]; break; // 7
		case 56: keyboardState = keypadStates[7]; break; // 8
		case 57: keyboardState = keypadStates[8]; break; // 9
		case 91: keyboardState = keypadStates[9]; break; // C [
		case 93: keyboardState = keypadStates[11]; break; // E ]
		default: 
			keyboardChange = false;
			keyboardDown = false;
	}
}

static void check_variables(bool first_run)
{
	struct retro_variable var = {0};

	if (first_run)
	{
		var.key   = "default_p1_controller";
		var.value = NULL;

		// by default input 0 maps to Right Controller (0x1FE)
		// and input 1 maps to Left Controller (0x1FF)
		controllerSwap = 0;

		if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "left") == 0)
				controllerSwap = 1;
		}
	}
}

void retro_set_environment(retro_environment_t fn)
{
	Environ = fn;

	// Set core options
	libretro_supports_option_categories = false;
	libretro_set_core_options(Environ, &libretro_supports_option_categories);
}

void retro_init(void)
{
	char execPath[PATH_MAX_LENGTH];
	char gromPath[PATH_MAX_LENGTH];
	struct retro_keyboard_callback kb = { Keyboard };

	// controller descriptors
	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 0 },
	};

	// init buffers, structs
	memset(frame, 0, frameSize);
	OSD_setDisplay(frame, MaxWidth, MaxHeight);

	Environ(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	// reset console
	Init();
	Reset();

	// get paths
	Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath);

	// load exec
	fill_pathname_join(execPath, SystemPath, "exec.bin", PATH_MAX_LENGTH);
	loadExec(execPath);

	// load grom
	fill_pathname_join(gromPath, SystemPath, "grom.bin", PATH_MAX_LENGTH);
	loadGrom(gromPath);

	// Setup keyboard input
	Environ(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb);
}

bool retro_load_game(const struct retro_game_info *info)
{
	check_variables(true);
	LoadGame(info->path);
	
	// Capture system directory and load overlays
	if (SystemPath && SystemPath[0]) {
		strncpy(system_dir, SystemPath, sizeof(system_dir) - 1);
		system_dir[sizeof(system_dir) - 1] = '\0';
		printf("[GAME] ==================== SYSTEM DIR DEBUG ====================\n");
		printf("[GAME] Raw SystemPath from RetroArch: %s\n", SystemPath);
		printf("[GAME] Copied to system_dir: %s\n", system_dir);
		printf("[GAME] system_dir length: %zu\n", strlen(system_dir));
		printf("[GAME] Last char: '%c' (code %d)\n", system_dir[strlen(system_dir)-1], system_dir[strlen(system_dir)-1]);
		printf("[GAME] ============================================================\n");
		
		// Load controller base, utility buttons, and ROM-specific overlay
		load_controller_base();
		load_utility_buttons();
		load_overlay_for_rom(info->path);
		init_overlay_hotspots();
	} else {
		printf("[GAME] ERROR: SystemPath is NULL or empty!\n");
	}
	
	return true;
}

void retro_unload_game(void)
{
	quit(0);
}

void retro_run(void)
{
	int c, i, j, k, l;
	int showKeypad0 = false;
	int showKeypad1 = false;

	bool options_updated  = false;
	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
		check_variables(false);

	InputPoll();
	
	// DEBUG: Check pointer input at the start of retro_run and write to file
	static int debug_frame_count = 0;
	if (debug_frame_count < 300) {
		int px = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
		int py = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
		int pp = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
		
		FILE *f = fopen("/storage/emulated/0/Download/freeintv_pointer_debug.txt", "a");
		if (f) {
			fprintf(f, "Frame %d: POINTER x=%d y=%d pressed=%d\n", debug_frame_count, px, py, pp);
			fflush(f);
			fclose(f);
		}
		debug_frame_count++;
	}

	for(i = 0; i < 20; i++) // Copy previous state
	{
		joypre0[i] = joypad0[i];
		joypre1[i] = joypad1[i];
	}

	/* JoyPad 0 */
	joypad0[0] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad0[1] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad0[2] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad0[3] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad0[4] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad0[5] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad0[6] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad0[7] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad0[8] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad0[9] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad0[10] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad0[11] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad0[12] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad0[13] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad0[14] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[15] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[18] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad0[19] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	/* JoyPad 1 */
	joypad1[0] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad1[1] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad1[2] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad1[3] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad1[4] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad1[5] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad1[6] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad1[7] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad1[8] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad1[9] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad1[10] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad1[11] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad1[12] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad1[13] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad1[14] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[15] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[18] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad1[19] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	// Pause
	if((joypad0[8]==1 && joypre0[8]==0) || (joypad1[8]==1 && joypre1[8]==0))
	{
		paused = !paused;
		if(paused)
		{
			OSD_drawPaused();
			OSD_drawTextCenterBG(21, "HELP - PRESS A");
		}
	}

	if(paused)
	{
		// help menu //
		if(joypad0[4]==1 || joypad1[4]==1)
		{
			OSD_drawTextBG(3,  4, "                                      ");
			OSD_drawTextBG(3,  5, "               - HELP -               ");
			OSD_drawTextBG(3,  6, "                                      ");
			OSD_drawTextBG(3,  7, " A      - RIGHT ACTION BUTTON         ");
			OSD_drawTextBG(3,  8, " B      - LEFT ACTION BUTTON          ");
			OSD_drawTextBG(3,  9, " Y      - TOP ACTION BUTTON           ");
			OSD_drawTextBG(3, 10, " X      - LAST SELECTED KEYPAD BUTTON ");
			OSD_drawTextBG(3, 11, " L/R    - SHOW KEYPAD                 ");
			OSD_drawTextBG(3, 12, " LT/RT  - KEYPAD CLEAR/ENTER          ");
			OSD_drawTextBG(3, 13, "                                      ");
			OSD_drawTextBG(3, 14, " START  - PAUSE GAME                  ");
			OSD_drawTextBG(3, 15, " SELECT - SWAP LEFT/RIGHT CONTROLLERS ");
			OSD_drawTextBG(3, 16, "                                      ");
			OSD_drawTextBG(3, 17, " FREEINTV 1.2          LICENSE GPL V2+");
			OSD_drawTextBG(3, 18, "                                      ");
		}
	}
	else
	{
		// Process hotspot input directly - each hotspot assigned to its relative keypad button
		process_hotspot_input();
		
		// Process utility button input - map to RetroArch commands
		process_utility_button_input();
		
		// Keep regular controller input for compatibility with non-overlay gameplay
		// If no hotspot is pressed, fall back to standard controller input
		int any_hotspot_pressed = 0;
		for (int h = 0; h < OVERLAY_HOTSPOT_COUNT; h++)
		{
			if (hotspot_pressed[h])
			{
				any_hotspot_pressed = 1;
				break;
			}
		}
		
		// If no hotspots pressed, handle regular controller input
		if (!any_hotspot_pressed)
		{
			setControllerInput(0, getControllerState(joypad0, 0));
		}

		// Player 2 controller input (unchanged - no hotspot overlay for player 2)
		if(joypad1[10] | joypad1[11]) // left shoulder down
		{
			showKeypad1 = true;
			setControllerInput(1, getKeypadState(1, joypad1, joypre1));
		}
		else
		{
			showKeypad1 = false;
			setControllerInput(1, getControllerState(joypad1, 1));
		}

		if(keyboardDown || keyboardChange)
		{
			setControllerInput(0, keyboardState);
			keyboardChange = false;
		}

		// grab frame
		Run();

		// draw overlays
		if(showKeypad0) { drawMiniKeypad(0, frame); }
		if(showKeypad1) { drawMiniKeypad(1, frame); }

		// sample audio from buffer
		audioInc = 3733.5 / audioSamples;
		ivoiceInc = 1.0;

		j = 0;
		for(i=0; i<audioSamples; i++)
		{
			// Sound interpolator:
			//   The PSG module generates audio at 224010 hz (3733.5 samples per frame)
			//   Very high frequencies like 0x0001 would generate chirps on output
			//   (For example, Lock&Chase) so this code interpolates audio, making
			//   these silent as in real hardware.
			audioBufferPos += audioInc;
			k = audioBufferPos;
			l = k - j;

			c = 0;
			while (j < k)
				c += PSGBuffer[j++];
			c = c / l;
			// Finally it adds the Intellivoice output (properly generated at the
			// same frequency as output)
			c = (c + ivoiceBuffer[(int) ivoiceBufferPos]) / 2;

			Audio(c, c); // Audio(left, right)

			ivoiceBufferPos += ivoiceInc;

			if (ivoiceBufferPos >= ivoiceBufferSize)
				ivoiceBufferPos = 0.0;

			audioBufferPos = audioBufferPos * (audioBufferPos<(PSGBufferSize-1));
		}
		audioBufferPos = 0.0;
		PSGFrame();
		ivoiceBufferPos = 0.0;
		ivoice_frame();
	}

	// Swap Left/Right Controller
	if(joypad0[9]==1 || joypad1[9]==1)
	{
		if ((joypad0[9]==1 && joypre0[9]==0) || (joypad1[9]==1 && joypre1[9]==0))
		{
			controllerSwap = controllerSwap ^ 1;
		}
		if(controllerSwap==1)
		{
			OSD_drawLeftRight();
		}
		else
		{
			OSD_drawRightLeft();
		}
	}

	if (intv_halt)
		OSD_drawTextBG(3, 5, "INTELLIVISION HALTED");
	
	// Render dual-screen display (game + keypad)
	render_dual_screen();
	
	// Send frame to libretro
	if (dual_screen_enabled && dual_screen_buffer) {
		Video(dual_screen_buffer, WORKSPACE_WIDTH, WORKSPACE_HEIGHT, sizeof(unsigned int) * WORKSPACE_WIDTH);
	} else {
		Video(frame, frameWidth, frameHeight, sizeof(unsigned int) * frameWidth);
	}

}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "FreeIntv";
	info->library_version = "1.2";
	info->valid_extensions = "int|bin|rom";
	info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;

	memset(info, 0, sizeof(*info));
	
	// Report dimensions based on dual-screen mode
	if (dual_screen_enabled) {
		info->geometry.base_width   = WORKSPACE_WIDTH;
		info->geometry.base_height  = WORKSPACE_HEIGHT;
		info->geometry.max_width    = WORKSPACE_WIDTH;
		info->geometry.max_height   = WORKSPACE_HEIGHT;
		info->geometry.aspect_ratio = ((float)WORKSPACE_WIDTH) / ((float)WORKSPACE_HEIGHT);
	} else {
		info->geometry.base_width   = MaxWidth;
		info->geometry.base_height  = MaxHeight;
		info->geometry.max_width    = MaxWidth;
		info->geometry.max_height   = MaxHeight;
		info->geometry.aspect_ratio = ((float)MaxWidth) / ((float)MaxHeight);
	}

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = AUDIO_FREQUENCY;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void)
{
	libretro_supports_option_categories = false;
	quit(0);
}

void retro_reset(void)
{
	// Reset (from intv.c) //
	Reset();
}

RETRO_API void *retro_get_memory_data(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return Memory;
	}
	return 0;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return 0x10000;
	}
	return 0;
}

#define SERIALIZED_VERSION 0x4f544702

struct serialized {
	int version;
	struct CP1610serialized CP1610;
	struct STICserialized STIC;
	struct PSGserialized PSG;
	struct ivoiceSerialized ivoice;
	unsigned int Memory[0x10000];   // Should be equal to Memory.c
	// Extra variables from intv.c
	int SR1;
	int intv_halt;
};

size_t retro_serialize_size(void)
{
	return sizeof(struct serialized);
}

bool retro_serialize(void *data, size_t size)
{
	struct serialized *all;

	all = (struct serialized *) data;
	all->version = SERIALIZED_VERSION;
	CP1610Serialize(&all->CP1610);
	STICSerialize(&all->STIC);
	PSGSerialize(&all->PSG);
	ivoiceSerialize(&all->ivoice);
	memcpy(all->Memory, Memory, sizeof(Memory));
	all->SR1 = SR1;
	all->intv_halt = intv_halt;
	return true;
}

bool retro_unserialize(const void *data, size_t size)
{
	const struct serialized *all;

	all = (const struct serialized *) data;
	if (all->version != SERIALIZED_VERSION)
		return false;
	CP1610Unserialize(&all->CP1610);
	STICUnserialize(&all->STIC);
	PSGUnserialize(&all->PSG);
	ivoiceUnserialize(&all->ivoice);
	memcpy(Memory, all->Memory, sizeof(Memory));
	SR1 = all->SR1;
	intv_halt = all->intv_halt;
	return true;
}

/* Stubs */
unsigned int retro_api_version(void) { return RETRO_API_VERSION; }
void retro_cheat_reset(void) {  }
void retro_cheat_set(unsigned index, bool enabled, const char *code) {  }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {  }
