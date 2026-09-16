#include "../section8_global-header/global.h"
#include "aliscr.h"
#define MAX_LINES 100
#define LINE_SIZE 64

/* --- VM STATE --- */
static int stack[64];
static int sp = 0;
static int vars[26];
static char script_buffer[MAX_LINES][LINE_SIZE];
static int current_line = 0;

static void push(int v) { if (sp < 64) stack[sp++] = v; }
static int pop() { return (sp > 0) ? stack[--sp] : 0; }


void execute_aliscript_line(char* args) {
    if (!args || *args == '\0') return;
    char* ptr = args;
    while (*ptr) {
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        if (*ptr == '\0') break;
        char token[32]; int t = 0;
        while (*ptr != ' ' && *ptr != '\0' && t < 31) token[t++] = *ptr++;
        token[t] = '\0';

        if ((token[0] >= '0' && token[0] <= '9') || (token[0] == '-' && token[1] >= '0')) push(atoi_custom(token));
        else if (strcmp(token, "add") == 0) push(pop() + pop());
        else if (strcmp(token, "sub") == 0) { int b = pop(); push(pop() - b); }
        else if (strcmp(token, "mul") == 0) push(pop() * pop());
        else if (strncmp(token, "set_", 4) == 0) vars[token[4] - 'a'] = pop();
        else if (strncmp(token, "get_", 4) == 0) push(vars[token[4] - 'a']);
        else if (strcmp(token, "print") == 0) { char b[16]; vga_write(itoa(pop(), b)); vga_write(" "); }
        else if (strcmp(token, "cls") == 0) vga_clear();
        else if (strcmp(token, "plane") == 0) draw_custom_plane();
        else if (strcmp(token, "poke") == 0) { 
            int val = pop();                // Value to write
            int addr = pop();               // Destination address
            unsigned char* ptr = (unsigned char*)addr; 
            *ptr = (unsigned char)val;      // The "Write" Operation
        }
        else if (strcmp(token, "peek") == 0) { 
            int addr = pop();               // Target address
            unsigned char* ptr = (unsigned char*)addr;
            push((int)(*ptr));              // The "Read" Operation
        }
}
}
/* --- THE BLOCKING INPUT ENGINE --- */
// This function polls the hardware ports directly to wait for a key
char wait_for_key() {
    while(1) {
        if (inb(0x64) & 0x01) { // Check if keyboard buffer has data
            unsigned char scancode = inb(0x60);
            char c = kbd_get_char(scancode);
            if (c != 0) return c; // Only return if it's a printable char
        }
    }
}

void cmd_run_script() {
    char input[LINE_SIZE];
    int is_recording = 1;
    current_line = 0;
    sp = 0;

    vga_write("\n--- ScribbleOS 4 Notebook Scripting ---\n");
    vga_write("Enter commands. Type 'doner' to finish.\n");

    while (is_recording) {
        vga_write("Aliscr> ");
        
        int i = 0;
        while (i < LINE_SIZE - 1) {
            char c = wait_for_key(); // Use our polling function

            if (c == '\n') {
                input[i] = '\0';
                vga_putchar('\n');
                break;
            } else if (c == '\b') {
                if (i > 0) {
                    i--;
                    vga_putchar('\b');
                }
            } else {
                input[i++] = c;
                vga_putchar(c);
            }
        }

        if (strcmp(input, "doner") == 0) {
            is_recording = 0;
            vga_write("[!] Executing Notebook Sequence...\n");
            for (int j = 0; j < current_line; j++) {
                execute_aliscript_line(script_buffer[j]);
            }
            vga_write("\n[Script Success]\n");
        } else if (current_line < MAX_LINES) {
            strcpy(script_buffer[current_line++], input);
        }
    }
}
