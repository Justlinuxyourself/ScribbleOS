#include "../section8_global-header/global.h"
/* Copyright (c) 2026 Ali  
All rights reserved.
*/
#include "tui_video.h"
#include "../section1_cpu/io.h"
int current_selection = 0; // 0-5 for App grid, 6 for Volume Slider
int is_dragging = 0;       // Keeps focus locked to slider values
int slider_val = 5;        // Defaults midrange volume state

void launch_app_stub(int app_id) {
    // Clear screen before launching an app so it looks clean
    vga_clear(); 
    
    switch(app_id) {
        case 0: // Neofetch Info
            cmd_neofetch(0);
            break;
        case 1: // To-Do List
            todo_show();
            break;
        case 2: // Calculator
            vga_write("Use 'calc' in the terminal for now!\n");
            break;
        case 3: // Quran Ayah
            cmd_ayah();
            break;
        case 4: // Plane Art
            draw_custom_plane();
            break;
        case 5: // Lock Screen
            shell_lock();
            break;
        default:
            break;
    }
    
    // Wait for a quick keypress before jumping straight back to the TUI dashboard loop
    vga_write("\n[Press any key to return to Dashboard]");
    wait_for_key(); 
}

void update_hardware_speaker() {
    // Map slider values (0 to 9) to actual frequencies (Hz)
    // 0 is mute, 1 is low bass, 9 is high pitch
    uint32_t frequencies[] = {
        0,    // 0: Mute / Off
        150,  // 1: Very Low
        300,  // 2
        450,  // 3
        600,  // 4: Midtone
        750,  // 5
        900,  // 6
        1050, // 7
        1200, // 8
        1350  // 9: High Pitch
    };

    uint32_t freq = frequencies[slider_val];

    if (freq == 0) {
        nosound(); // Turn off the speaker if slider is 0
    } else {
        play_sound(freq); // Play the corresponding pitch
    }
}

static void render_dashboard() {
    tui_clear(ATTR_DEFAULT);

    // 1. Top Title bar banner
    for (int i = 0; i < VGA_WIDTH; i++) {
        tui_print_at(i, 0, " ", ATTR_HEADER);
    }
    tui_print_at(26, 0, "ScribbleOS 4.0 - Custom TUI Notebook", ATTR_HEADER);

    // 2. Generate 2x3 grid matrix for Applications (IDs 0 to 5)
    const char* app_names[] = {
        "Neofetch Info", "To-Do List", "Calculator",
        "Quran Ayah",    "Plane Art",  "Lock Screen"
    };

    int box_w = 22, box_h = 4;
    int start_x = 5, start_y = 3;

    for (int i = 0; i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = start_x + (col * (box_w + 3));
        int y = start_y + (row * (box_h + 2));

        uint8_t attr = (current_selection == i) ? ATTR_SELECTED : ATTR_DEFAULT;
        tui_draw_box(x, y, box_w, box_h, app_names[i], attr);
        
        if (current_selection == i) {
            tui_print_at(x + 2, y + 2, "-> Press ENTER", ATTR_ACCENT);
        } else {
            tui_print_at(x + 4, y + 2, "Launch App", ATTR_DEFAULT);
        }
    }

    // 3. Audio Controller Slider Block (ID 6)
    int slider_y = 17;
    uint8_t slider_attr = (current_selection == 6) ? ATTR_SELECTED : ATTR_DEFAULT;
    
    tui_draw_box(5, slider_y, 70, 5, "System Hardware Volume Level Control", slider_attr);
    
    // Draw visual graphical trackbar scale
    tui_print_at(10, slider_y + 2, "Volume Bar: [", slider_attr);
    for (int v = 0; v < 10; v++) {
        if (v == slider_val) {
            tui_print_at(23 + v, slider_y + 2, "O", ATTR_ACCENT); // Indicator slider node
        } else {
            tui_print_at(23 + v, slider_y + 2, "-", slider_attr);
        }
    }
    tui_print_at(33, slider_y + 2, "]", slider_attr);

    // Instruction dynamic tips line
    if (current_selection == 6) {
        if (is_dragging) {
            tui_print_at(40, slider_y + 2, "<- -> Adjust | ENTER to Save", ATTR_ACCENT);
        } else {
            tui_print_at(40, slider_y + 2, "Press ENTER/SPACE to adjust tuning", ATTR_DEFAULT);
        }
    } else {
        tui_print_at(10, 23, "Use Arrow Keys to Navigate Grid Selection System", ATTR_DEFAULT);
    }
}

void cmd_start_gui() {
    int running = 1;
    
    // Keep internal screen clean on startup
    tui_clear(ATTR_DEFAULT);

    while (running) {
        render_dashboard();
        vga_draw_status_bar(); // Keeps time ticking right at the bottom edge

        // Wait for hardware register scancode availability
        while (!(inb(0x64) & 0x01)) {
            // Spin loop hook keeping status engine operational
            vga_draw_status_bar();
        }

        unsigned char scancode = inb(0x60);

        // Global escape key check to exit shell straight back to command line terminal
        if (scancode == 0x01) { 
            running = 0;
            break;
        }

        // Split Controller logic handling based on Slider state focus locks
        if (is_dragging) {
            if (scancode == 0x4B && slider_val > 0) { // Left arrow
                slider_val--;
                 update_hardware_speaker();
            }
            else if (scancode == 0x4D && slider_val < 9) { // Right arrow
                slider_val++;
                 update_hardware_speaker();
            }
            else if (scancode == 0x1C || scancode == 0x39) { // Enter or Space bar
                is_dragging = 0; // Release lock focus back to navigation grid system
            }
        } 
        else {
            // Regular Navigation Track grid modes mapping layout matrix
            if (scancode == 0x4D) { // Right Arrow
                if (current_selection < 6) current_selection++;
            }
            else if (scancode == 0x4B) { // Left Arrow
                if (current_selection > 0) current_selection--;
            }
            else if (scancode == 0x50) { // Down Arrow
                if (current_selection <= 2) current_selection += 3;
                else if (current_selection <= 5) current_selection = 6;
            }
            else if (scancode == 0x48) { // Up Arrow
                if (current_selection == 6) current_selection = 3;
                else if (current_selection >= 3) current_selection -= 3;
            }
            else if (scancode == 0x1C || scancode == 0x39) { // Enter or Space bar input
                if (current_selection >= 0 && current_selection <= 5) {
                    // Instantly execute target launcher routine
                    launch_app_stub(current_selection);
                } 
                else if (current_selection == 6) {
                    is_dragging = 1; // Capture interactive focus lock onto the scale element
                }
            }
        }
    }

    tui_clear(0x1E); 
}
