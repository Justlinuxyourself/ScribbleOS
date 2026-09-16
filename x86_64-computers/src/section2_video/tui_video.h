/* Copyright (c) 2026 Ali  
All rights reserved.
*/
#ifndef TUI_VIDEO_H
#define TUI_VIDEO_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM_ADDR 0xB8000

// ScribbleOS Notebook Design Colors
#define ATTR_DEFAULT 0x1E      // Yellow text on Blue background
#define ATTR_SELECTED 0x70     // Inverted: Black text on Light Gray
#define ATTR_HEADER 0x1F       // White text on Blue background
#define ATTR_ACCENT 0x1B       // Cyan text on Blue background

void tui_clear(uint8_t attribute);
void tui_print_at(int x, int y, const char* str, uint8_t attribute);
void tui_draw_box(int x, int y, int width, int height, const char* title, uint8_t attribute);
void tui_set_cursor_hidden(int hidden);

#endif
