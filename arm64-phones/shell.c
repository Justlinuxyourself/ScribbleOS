/* 
Copyright (c) 2026 Ali  
All rights reserved.
*/

#include <stdint.h>

// Color configurations targeting a high-contrast theme (White/Yellow on Dark Blue)
#define COLOR_WHITE   0xFFFFFFFF  // Full White
#define COLOR_YELLOW  0xFFFFFF00  // Red + Green = Yellow
#define COLOR_RED     0xFFFF0000  // Pure Red
#define COLOR_GREEN   0xFF00FF00  // Pure Green
#define COLOR_BG      0xFF000033  // Very dark, solid navy blue background (or change Alpha if you want it transparent)

// External declarations from your phone port environment
extern void fb_puts(const char* str, uint32_t fg, uint32_t bg);
extern void fb_clear(uint32_t color);

// Local function declarations
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
int strlen(const char* s);
void strcpy(char* dest, const char* src);
void reverse(char* str, int length);
char* itoa(int value, char* str);
int atoi_custom(char* str);

/* --- Built-in Command Definitions --- */

void cmd_help() {
    fb_puts("\nScribbleOS Phone Commands:\n", COLOR_YELLOW, COLOR_BG);
    fb_puts("help      - List all available commands\n", COLOR_WHITE, COLOR_BG);
    fb_puts("cls       - Clear the screen\n", COLOR_WHITE, COLOR_BG);
    fb_puts("echo      - Repeat text back to screen\n", COLOR_WHITE, COLOR_BG);
    fb_puts("neofetch  - Display dynamic mobile architecture metadata\n", COLOR_WHITE, COLOR_BG);
    fb_puts("about_dev - About the Developer\n", COLOR_WHITE, COLOR_BG);
    fb_puts("twins     - Show the Legends list\n", COLOR_WHITE, COLOR_BG);
    fb_puts("lullaby   - Display synchronization note status\n", COLOR_WHITE, COLOR_BG);
    fb_puts("plane     - Display system custom aviation art\n", COLOR_WHITE, COLOR_BG);
}

void cmd_cls() {
    fb_clear(COLOR_BG);
}

void cmd_echo(char* args) {
    if (args && *args != '\0') {
        fb_puts(args, COLOR_WHITE, COLOR_BG);
        fb_puts("\n", COLOR_WHITE, COLOR_BG);
    }
}

void cmd_neofetch() {
    fb_puts("   ______      ScribbleOS 4.0 (Phone Edition)\n", COLOR_YELLOW, COLOR_BG);
    fb_puts("  / ____/      ---------------------------\n", COLOR_YELLOW, COLOR_BG);
    fb_puts(" / /  __       ARCH: AArch64 (ARMv8-A)\n", COLOR_WHITE, COLOR_BG);
    fb_puts("/ /__/ /       TARGET: Samsung A12\n", COLOR_WHITE, COLOR_BG);
    fb_puts("\\____ /        MODE: 64-bit Kernel Execution Context\n", COLOR_WHITE, COLOR_BG);
}

void cmd_about_dev() {
    fb_puts("Hi my name is ali, i am 12 years old, and i like anything that has engines :3\n", COLOR_WHITE, COLOR_BG);
}

void draw_custom_plane() {
    fb_puts("            __\\/__\n", COLOR_WHITE, COLOR_BG);
    fb_puts("           `==/\\==` \n", COLOR_WHITE, COLOR_BG);
    fb_puts(" ____________/__\\____________\n", COLOR_WHITE, COLOR_BG);
    fb_puts("/____________________________\\\n", COLOR_WHITE, COLOR_BG);
    fb_puts("  __||__||__/.--.\\__||__||__\n", COLOR_WHITE, COLOR_BG);
    fb_puts(" /__|___|___( >< )___|___|__\\\n", COLOR_WHITE, COLOR_BG);
    fb_puts("           _/`--`\\_\n", COLOR_WHITE, COLOR_BG);
    fb_puts("          (/------\\)\n", COLOR_WHITE, COLOR_BG);
}

void twins() {
    fb_puts("\n--- THE TWINS & LEGENDS ---\n", COLOR_YELLOW, COLOR_BG);
    
    const char* names[] = {
        "APHRODITE", "SAKI (BEST SUNDAY GOONER)", "QOQO", "KEI", "LWAH", 
        "O1", "O2", "RAYA THE KARAOKE QUEEN", "SEL", "ISHI", "ZAZA", 
        "VANILLA & MAX", "SUSTUBE", "ADNAN", "ZIKE (FAF)", "MOLY", 
        "ROSIE", "E", "Sillycat", "Kaisi", "ZANNNNNNNN", "ABDUALLAH", 
        "MIKAY (BATTERY EATER TWINIES)", "ALIYAH", "AYAH", "DANIEL", 
        "ASEEL (MY SIS)", "KHAILD", "AMAL (ASEELS BSF)", "FATMAH/FARAH (MY AUNT)", 
        "HEAIM", "BASMA & MALAK", "OSAMA & SHERIF (PEAKEST UNCLES 4EVER)", 
        "MARCEL", "FERIBSD", "SUMY", "KYOO", "LEXUS", "RAYYAN", "CAIN", 
        "ABEL", "<<OLIVIA>>", "EIAN", "REN", "SWEET POTATO", "DEITY", 
        "SPECIAL: GaroDaemon", "SPECIAL: ANTI-XV", "SPECIAL: Apple eater", 
        "SPECIAL: Bricky (kindred)", "SPECIAL: Kris", "SPECIAL: Panzerkampfwagen VIII manus", "SPECIAL 3x: IRIS"
    };

    int total_names = sizeof(names) / sizeof(names[0]);

    for (int i = 0; i < total_names; i++) {
        fb_puts(names[i], COLOR_WHITE, COLOR_BG);
        if (i < total_names - 1) {
            fb_puts(" ||| ", COLOR_YELLOW, COLOR_BG);
        }
    }
    fb_puts("\n---------------------------\n", COLOR_YELLOW, COLOR_BG);
}

void play_lullaby_sync() {
    fb_puts("[Lullaby Sequence for Baby Sis rendering strings directly to Framebuffer...]\n", COLOR_WHITE, COLOR_BG);
    fb_puts("♪ Twinkle, twinkle, little star... ♪\n", COLOR_YELLOW, COLOR_BG);
}

/* --- String Helpers --- */

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    while (n--) {
        if (*s1 != *s2++) return *(unsigned char*)s1 - *(unsigned char*)--s2;
        if (*s1++ == 0) break;
    }
    return 0;
}

int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* itoa(int value, char* str) {
    int i = 0;
    int isNegative = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (value < 0) {
        isNegative = 1;
        value = -value;
    }

    while (value != 0) {
        int rem = value % 10;
        str[i++] = rem + '0';
        value = value / 10;
    }

    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse(str, i);
    return str;
}

int atoi_custom(char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] < '0' || str[i] > '9') break;
        res = res * 10 + str[i] - '0';
    }
    return sign * res;
}

/* --- Core Shell Command Dispatcher --- */

void shell_dispatch(char* buffer) {
    // Check for an empty entry input
    if (strlen(buffer) == 0) {
        fb_puts("\nScribbleOS / > ", COLOR_YELLOW, COLOR_BG);
        return;
    }

    // Isolate command keyword from its trailing arguments string matching space arrays
    char* args = 0;
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ' ') {
            buffer[i] = '\0';
            args = &buffer[i + 1];
            break;
        }
    }

    // Direct structural commands matching dispatcher string trees
    if (strcmp(buffer, "help") == 0) {
        cmd_help();
    } else if (strcmp(buffer, "cls") == 0) {
        cmd_cls();
    } else if (strcmp(buffer, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(buffer, "neofetch") == 0) {
        cmd_neofetch();
    } else if (strcmp(buffer, "about_dev") == 0) {
        cmd_about_dev();
    } else if (strcmp(buffer, "plane") == 0) {
        draw_custom_plane();
    } else if (strcmp(buffer, "twins") == 0) {
        twins();
    } else if (strcmp(buffer, "lullaby") == 0) {
        play_lullaby_sync();
    } else {
        fb_puts("\nScribbleOS: '", COLOR_RED, COLOR_BG);
        fb_puts(buffer, COLOR_WHITE, COLOR_BG);
        fb_puts("' structural match not found. Type 'help'.\n", COLOR_RED, COLOR_BG);
    }

    // Re-print structural target prompt loop frame boundary
    fb_puts("\nScribbleOS / > ", COLOR_YELLOW, COLOR_BG);
}

/* --- Global Shell Management State --- */
#define CMD_BUFFER_MAX 256
static char shell_buffer[CMD_BUFFER_MAX];
static int shell_buffer_index = 0;

// 1. Initialize global shell indices
void shell_init(void) {
    for (int i = 0; i < CMD_BUFFER_MAX; i++) {
        shell_buffer[i] = '\0';
    }
    shell_buffer_index = 0;
}

// 2. Handle incoming key injections from your testing simulation sequence
void shell_handle_keypress(char key) {
    if (key == '\n') {
        // Enforce safe array ending boundaries
        shell_buffer[shell_buffer_index] = '\0';
        
        // Feed the gathered string to your dispatcher engine
        shell_dispatch(shell_buffer);
        
        // Reset state for the next incoming sequence
        shell_init();
    } else {
        // Safe check preventing random hardware buffer overruns
        if (shell_buffer_index < (CMD_BUFFER_MAX - 1)) {
            shell_buffer[shell_buffer_index] = key;
            shell_buffer_index++;
            
            // Print out character locally to emulate standard interactive feedback echo
            char echo_str[2] = {key, '\0'};
            fb_puts(echo_str, COLOR_WHITE, COLOR_BG);
        }
    }
}

// 3. Freestanding custom memcpy to resolve internal GCC layout assignments
void* memcpy(void* dest, const void* src, unsigned int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

