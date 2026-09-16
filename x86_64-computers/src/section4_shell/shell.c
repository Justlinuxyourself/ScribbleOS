#include "../section8_global-header/global.h"
/* 
Copyright (c) 2026 Ali  
All rights reserved.
*/
#include "shell.h"
#include "../section1_cpu/heap.h"
#include "../section1_cpu/io.h"
#include "aliscr.h"
#include "frames.h"
#include "quran.h"
#include "../section3_io/alifs.h"
#include <stdint.h>
#define NOTEBOOK_YELLOW 0x1E
#define VGA_ADDRESS 0xB8000
#define MAX_TTYS 10
volatile int is_sleeping = 0;
static command_node_t* command_list = 0;
int alifs_is_directory(char* name);
// Simple PRNG state
static uint32_t next_rand = 1;
// kernel start and end
char current_path[256] = "/"; // Start at root
env_var_t env_table[10]; // Store up to 10 variables in RAM
// some structs are down with the code that uses it bc i didnt plan for it, it just popped up in my head
todo_t my_list[10]; // 10 slots for your daily goals
// i hope this works

#define QR_SIZE 21

// Rebuilt: Version 1 QR matrix (21x21) encoding "https://h1.nu/a9"
const unsigned char alios_discord_qr[QR_SIZE][QR_SIZE] = {
    {1,1,1,1,1,1,1,0,1,1,0,0,1,0,1,1,1,1,1,1,1}, // 0
    {1,0,0,0,0,0,1,0,0,1,0,1,0,0,1,0,0,0,0,0,1}, // 1
    {1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1}, // 2
    {1,0,1,1,1,0,1,0,0,0,0,1,0,0,1,0,1,1,1,0,1}, // 3
    {1,0,1,1,1,0,1,0,1,1,1,0,0,0,1,0,1,1,1,0,1}, // 4
    {1,0,0,0,0,0,1,0,0,0,1,1,1,0,1,0,0,0,0,0,1}, // 5
    {1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1}, // 6
    {0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0}, // 7
    {1,1,0,1,1,1,0,0,0,0,1,1,1,1,1,1,0,1,0,1,1}, // 8
    {0,0,1,0,0,1,1,1,0,1,0,1,0,1,1,0,1,1,0,0,0}, // 9
    {1,1,0,0,1,0,0,0,1,0,1,0,1,0,1,1,0,0,1,1,1}, // 10
    {1,1,1,0,0,1,1,1,0,1,1,0,0,0,0,0,1,0,1,0,0}, // 11
    {0,1,0,1,1,0,1,1,1,1,0,0,1,1,0,1,1,0,0,1,0}, // 12
    {0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,1,1,1,0,0}, // 13
    {1,1,1,1,1,1,1,0,1,1,0,1,0,0,1,1,0,1,1,1,0}, // 14
    {1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,0,1,0,1}, // 15
    {1,0,1,1,1,0,1,0,1,0,1,0,0,0,1,0,0,1,0,0,1}, // 16
    {1,0,1,1,1,0,1,0,1,1,1,1,0,1,1,0,1,1,0,1,0}, // 17
    {1,0,1,1,1,0,1,0,0,0,1,0,0,1,1,1,0,0,1,1,1}, // 18
    {1,0,0,0,0,0,1,0,1,0,0,1,1,1,1,1,0,1,0,0,0}, // 19
    {1,1,1,1,1,1,1,0,1,1,1,0,1,0,0,0,1,0,1,1,1}  // 20
};


#define WIDTH 80
#define HEIGHT 25

#define CMATRIX_COLS 80
#define CMATRIX_ROWS 24 // Leave row 24 safe for status bar!
#define DVD_COLS 80
#define DVD_ROWS 24 // Leave row 24 safe for the status bar clock
#define MAX_HISTORY 10
char history[MAX_HISTORY][80];
int history_idx = 0;
int history_count = 0;
#define MAX_SNAKE_LEN 100
#include "../section7_posix/syscall.h"
int fd_result = -1;

/* --- String Helpers --- */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Helper: Reverse a string in place */
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

/* itoa: Convert integer to string (Base 10 only) */
char* itoa(int value, char* str) {
    int i = 0;
    int isNegative = 0;

    /* Handle 0 explicitly */
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    /* Handle negative numbers */
    if (value < 0) {
        isNegative = 1;
        value = -value;
    }

    /* Process individual digits in Base 10 */
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
// Simple atoi implementation to convert string to integer
int atoi_custom(char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++; // Just skip the plus sign and stay positive
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] < '0' || str[i] > '9') break;
        res = res * 10 + str[i] - '0';
    }
    return sign * res;
}
/* itohex: Convert integer to Hexadecimal string */
char* itohex(unsigned long value, char* str) {
    char* hex_chars = "0123456789ABCDEF";
    int i = 0;
    
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    while (value > 0) {
        str[i++] = hex_chars[value % 16];
        value /= 16;
    }
    str[i] = '\0';
    reverse(str, i);
    return str;
}

char* ltoa(long value, char* str) {
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
/* --- Built-in Commands --- */
void cmd_help(char* args) {
    (void)args;
    
    vga_write("\nScribbleOS 4 Commands: ");
    
    command_node_t* curr = command_list;
    while (curr) {
        vga_write(curr->name);
        vga_write(" - ");
        vga_write(curr->description);
        
        curr = curr->next;
        
        if (curr) {
            vga_write(" | ");
        }
    }
    vga_write("\n"); // Final newline to keep the shell prompt clean
}


void cmd_cls(char* args) {
    vga_clear();

}

void cmd_echo(char* args) {
    if (args) {
//        vga_write("\n");
        vga_write(args);
    }
}

void cmd_neofetch(char* args) {
    char cpu_name[49];
    get_cpu_name(cpu_name); 
    char mem_str[16];
    itoa(get_heap_usage(), mem_str);
    
    // Get the actual hex address of the heap
    char heap_addr_buf[16];
    char size_buf[12];
    itohex((unsigned long)&_kernel_end, heap_addr_buf);
    
    vga_write("   ______      ScribbleOS 4.0\n");
    vga_write("  / ____/      ----------\n");
    vga_write(" / /  __       CPU: "); vga_write(cpu_name); vga_write("\n");
    vga_write("/ /__/ /       MEM: "); vga_write(mem_str); vga_write(" bytes used\n");
    
    // NO MORE HARDCODING: Show the real address
    vga_write("\\____ /        HEAP: 0x"); vga_write(heap_addr_buf); vga_write("\n");
    vga_write("               KERNEL SIZE:");
    vga_write(itoa((int)(_kernel_end - _kernel_start), size_buf));
    vga_write(" bytes\n");
    vga_write("               MODE: 64-bit Long Mode\n");
}

void cmd_uptime(char* args) {
    char sec_str[16];
    itoa(get_uptime_seconds(), sec_str); // Use the real timer data
    
    vga_write("uptime: ");
    vga_write(sec_str);
    vga_write(" seconds.\n");
}
void cmd_free(char* args) {
    unsigned int total = (unsigned int)get_total_ram_bytes();
    unsigned int used = get_heap_usage();
    unsigned int free = total - used;

    char t_str[16], u_str[16], f_str[16];
    
    // No more dividing by 1024!
    itoa(total, t_str);
    itoa(used, u_str);
    itoa(free, f_str);

    vga_write("\nMemory Usage (Bytes):");
    vga_write("\n  Total: "); vga_write(t_str);
    vga_write("\n  Used:  "); vga_write(u_str);
    vga_write("\n  Free:  "); vga_write(f_str);
    vga_write("\n");
}
 
void shell_cmd_timezone(char* arg) {
    if (arg == 0 || arg[0] == '\0') {
        vga_write("Usage: timezone [hours] [seconds]\n");
        return;
    }

    int h = 0, s = 0;
    char* second_part = 0;

    // Split "hours" and "seconds"
    for (int i = 0; arg[i]; i++) {
        if (arg[i] == ' ') {
            arg[i] = '\0';
            second_part = &arg[i+1];
            break;
        }
    }

    h = atoi_custom(arg); 
    if (second_part) s = atoi_custom(second_part);

    // Logic: If hours are negative, seconds should usually be subtracted too
    // Example: GMT-5:30 means -5 hours AND -30 minutes
    int total;
    if (h < 0) {
        total = (h * 3600) - s; 
    } else {
        total = (h * 3600) + s;
    }

    timezone_offset_seconds = total;

    vga_write("Timezone offset set to ");
    char buf[16];
    vga_write(itoa(timezone_offset_seconds, buf));
    vga_write(" seconds.\n");


    // CRITICAL: Refresh the screen so you see the change!
    vga_draw_status_bar(); 
}

void shell_lock() {
    lock_system_hardened();
}
void cmd_test(char* args) {
    vga_write("Calibrating timer (5s wait)...\n");
    for(int i = 5; i > 0; i--) {
        char buf[4];
        itoa(i, buf);
        vga_write(buf);
        vga_write("... ");
        sleep(1);
    }
    vga_write("\nTest complete.\n");
}
void cmd_beep(){
    vga_write("Beeping...");
    play_sound(1000);
    sleep(1);
    nosound();
}
void cmd_about_dev() {
	vga_write("Hi my name is ali, i am 12 years old, and i like anything that has engines :3");
}
void draw_custom_plane() {
   
    vga_write("            __\\/__\n");
    vga_write("           `==/\\==` \n");
    vga_write(" ____________/__\\____________\n");
    vga_write("/____________________________\\\n");
    vga_write("  __||__||__/.--.\\__||__||__\n");
    vga_write(" /__|___|___( >< )___|___|__\\\n");
    vga_write("           _/`--`\\_\n");
    vga_write("          (/------\\)\n");
}

void twins() {
    vga_write("\n--- THE TWINS & LEGENDS ---\n");
    
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
        "SPECIAL: Bricky (kindred)", "SPECIAL: Kris", "SPECIAL: Panzerkampfwagen VIII manus", "SPECIAL 2X: MACH10", "SPECIAL 2X: YUNIA", "SPECIAL FOR MAKING ME A EXCEPTION ON A SERVER: NANA"
    };

    int total_names = sizeof(names) / sizeof(names[0]);

    for (int i = 0; i < total_names; i++) {
        vga_write(names[i]);
        if (i < total_names - 1) {
            vga_write(" ||| ");
        }
    }
    vga_write("\n---------------------------\n");
}


void sys_sleep() {
    is_sleeping = 1;
    vga_clear();
    vga_write("SYSTEM SLEEPING... Press ANY key to wake.");

    // We CANNOT use hlt if we don't have ISRs set up.
    // Instead, we just loop and poll the keyboard.
    while(is_sleeping) {
        // Look at the keyboard status port (0x64)
        // Bit 0 is set if there is data in the buffer
        if (inb(0x64) & 1) {
            unsigned char scan = inb(0x60);
            if (scan < 0x80) { // Any key "Make" code
                is_sleeping = 0;
            }
        }
        
        // Give the CPU a tiny rest without fully halting
        __asm__ volatile("pause"); 
    }

    vga_clear();
    vga_draw_status_bar();
    lock_system_hardened();
}
void command_calc(char* args) {
    if (args == 0 || *args == '\0') {
        vga_write("Usage: calc [num1] [op] [num2]\n");
        vga_write("Example: calc 5 a 10\n");
        return;
    }

    char* part1 = args;
    char* part2 = 0;
    char* part3 = 0;

    // 1. Find the first space to get the operator
    for (int i = 0; args[i]; i++) {
        if (args[i] == ' ') {
            args[i] = '\0';     // Terminate num1
            part2 = &args[i+1]; // Start of operator
            break;
        }
    }

    if (!part2) { vga_write("Error: Missing operator.\n"); return; }

    // 2. Find the second space to get the second number
    for (int i = 0; part2[i]; i++) {
        if (part2[i] == ' ') {
            part2[i] = '\0';    // Terminate operator
            part3 = &part2[i+1]; // Start of num2
            break;
        }
    }

    if (!part3) { vga_write("Error: Missing second number.\n"); return; }

    int n1 = atoi_custom(part1);
    char op = part2[0]; 
    int n2 = atoi_custom(part3);
    int result = 0;

    if (op == 'a')      result = n1 + n2;
    else if (op == 's') result = n1 - n2;
    else if (op == 'm') result = n1 * n2;
    else if (op == 'd') {
        if (n2 == 0) { vga_write("Error: Div by 0\n"); return; }
        result = n1 / n2;
    } else {
        vga_write("Error: Use a/s/m/d\n");
        return;
    }

    char res_buffer[32];
    itoa(result, res_buffer);
    
    vga_write("Result: ");
    vga_write(res_buffer);
    vga_write("\n");
}

void cmd_peek(char* args) {
    if (args == 0 || *args == '\0') {
        vga_write("Usage: peek [hex_address]\nExample: peek 0xB8000\n");
        return;
    }

    // Simple hex string to long converter
    unsigned long addr = 0;
    int start = 0;
    if (args[0] == '0' && (args[1] == 'x' || args[1] == 'X')) start = 2;

    for (int i = start; args[i] != '\0'; i++) {
        addr *= 16;
        if (args[i] >= '0' && args[i] <= '9') addr += (args[i] - '0');
        else if (args[i] >= 'a' && args[i] <= 'f') addr += (args[i] - 'a' + 10);
        else if (args[i] >= 'A' && args[i] <= 'F') addr += (args[i] - 'A' + 10);
    }

    unsigned char* ptr = (unsigned char*)addr;
    
    // We will dump 8 lines (128 bytes total) 
    int LINES_TO_DUMP = 8; 

    for (int line = 0; line < LINES_TO_DUMP; line++) {
        unsigned long current_line_addr = addr + (line * 16);
        
        // 1. Print the Address Header (e.g., 000B8000)
        char addr_buf[17]; // Fits 64-bit hex
        itohex(current_line_addr, addr_buf);
        
        // Pad address to 8 characters for clean alignment
        int addr_len = 0;
        while(addr_buf[addr_len] != '\0') addr_len++;
        for(int p = 0; p < (8 - addr_len); p++) vga_write("0");
        
        vga_write(addr_buf);
        vga_write("  "); // Space between address and hex

        // 2. Hex Dump Column (16 bytes)
        for (int i = 0; i < 16; i++) {
            int idx = (line * 16) + i;
            char buf[3];
            itohex(ptr[idx], buf);
            
            if (ptr[idx] < 16) vga_write("0"); // Hex padding
            vga_write(buf);
            vga_write(" ");

            if (i == 7) {
                vga_write(" ");
            }
        }

        vga_write(" |"); // ASCII boundary separator

        // 3. ASCII Printable Column
        for (int i = 0; i < 16; i++) {
            int idx = (line * 16) + i;
            unsigned char c = ptr[idx];

            // Only print readable ASCII characters; replace control/garbage bytes with a dot '.'
            if (c >= 32 && c <= 126) {
                char ascii_char[2] = { (char)c, '\0' };
                vga_write(ascii_char);
            } else {
                vga_write(".");
            }
        }
        
        vga_write("|\n"); // Close ASCII block and wrap line
    }
}
void cmd_poke(char* args) {
    if (args == 0 || *args == '\0') {
        vga_write("Usage: poke [addr] [val]\nExample: poke 0xB8000 0x41\n");
        return;
    }

    char* addr_str = args;
    char* val_str = 0;

    // 1. Split the string at the space
    for (int i = 0; args[i]; i++) {
        if (args[i] == ' ') {
            args[i] = '\0';
            val_str = &args[i+1];
            break;
        }
    }

    if (!val_str) {
        vga_write("Error: Missing value.\n");
        return;
    }

    // 2. Parse Address (Hex or Dec)
    unsigned long addr = 0;
    int i = 0;
    if (addr_str[0] == '0' && (addr_str[1] == 'x' || addr_str[1] == 'X')) {
        i = 2;
        while (addr_str[i]) {
            addr *= 16;
            if (addr_str[i] >= '0' && addr_str[i] <= '9') addr += (addr_str[i] - '0');
            else if (addr_str[i] >= 'a' && addr_str[i] <= 'f') addr += (addr_str[i] - 'a' + 10);
            else if (addr_str[i] >= 'A' && addr_str[i] <= 'F') addr += (addr_str[i] - 'A' + 10);
            i++;
        }
    } else {
        addr = (unsigned long)atoi_custom(addr_str);
    }

    // 3. Parse Value (Hex or Dec)
    unsigned char val = 0;
    if (val_str[0] == '0' && (val_str[1] == 'x' || val_str[1] == 'X')) {
        int j = 2;
        while (val_str[j]) {
            val *= 16;
            if (val_str[j] >= '0' && val_str[j] <= '9') val += (val_str[j] - '0');
            else if (val_str[j] >= 'a' && val_str[j] <= 'f') val += (val_str[j] - 'a' + 10);
            else if (val_str[j] >= 'A' && val_str[j] <= 'F') val += (val_str[j] - 'A' + 10);
            j++;
        }
    } else {
        val = (unsigned char)atoi_custom(val_str);
    }

    // 4. The Poke: Write to raw memory
    unsigned char* ptr = (unsigned char*)addr;
    *ptr = val;

    vga_write("Memory modified at 0x");
    vga_write(addr_str);
    vga_write("\n");
}

void cmd_ayah() {
    // Array of Ayahs stored in the Kernel Data Segment
    ayah_t quran_db[] = {
        {94, 5, "For indeed, with hardship [will be] ease."},
        {2, 152, "So remember Me; I will remember you."},
        {3, 139, "So do not weaken and do not grieve."},
        {2, 286, "Allah does not charge a soul except with that within its capacity."},
        {50, 16, "And We are closer to him than [his] jugular vein."}
    };

    // Calculate total entries in the database
    int db_size = sizeof(quran_db) / sizeof(ayah_t);

    // Use CMOS seconds to pick a random index
    int r = cmos_get_sec() % db_size;

    // Buffers for itoa conversion
    char s_str[8], a_str[8];

    vga_write("\n");
    // Print Format -> (SurahNum):(AyahNum) (Text)
    vga_write(itoa(quran_db[r].surah, s_str));
    vga_write(":");
    vga_write(itoa(quran_db[r].ayah, a_str));
    vga_write(" ");
    vga_write(quran_db[r].text);
    vga_write("\n");
}

void cmd_verse() {
    bible_t bible_db[] = {
        {"Psalms", 23, 1, "The Lord is my shepherd; I shall not want."},
        {"John", 1, 5, "The light shines in the darkness, and the darkness has not overcome it."},
        {"Philippians", 4, 13, "I can do all things through Christ who strengthens me."},
        {"Matthew", 5, 9, "Blessed are the peacemakers, for they shall be called sons of God."},
        {"Proverbs", 3, 5, "Trust in the Lord with all your heart and lean not on your own understanding."}
    };

    int db_size = sizeof(bible_db) / sizeof(bible_t);
    int r = cmos_get_sec() % db_size;

    char c_str[8], v_str[8];

    vga_write("\n");
    // Format: Book Chapter:Verse - Text
    vga_write(bible_db[r].book);
    vga_write(" ");
    vga_write(itoa(bible_db[r].chapter, c_str));
    vga_write(":");
    vga_write(itoa(bible_db[r].verse, v_str));
    vga_write(" - ");
    vga_write(bible_db[r].text);
    vga_write("\n");
}
void cmd_set(char* args) {
    if (args == 0 || *args == '\0') {
        vga_write("Usage: set [key] [value]\n");
        return;
    }

    char* key = args;
    char* val = 0;
    
    for (int i = 0; args[i]; i++) {
        if (args[i] == ' ') {
            args[i] = '\0'; 
            val = &args[i+1]; 
            break;
        }
    }

    if (!val || *val == '\0') {
        vga_write("Error: Missing value for variable.\n");
        return;
    }

    // Save to the table
    for(int i = 0; i < 10; i++) {
        // Look for empty slot or overwrite existing key
        if(!env_table[i].active || strcmp(env_table[i].key, key) == 0) {
            strcpy(env_table[i].key, key);
            strcpy(env_table[i].value, val);
            env_table[i].active = 1;
            vga_write("Variable set.");
            return;
        }
    }
    vga_write("Error: Environment table full!");
}

void cmd_get(char* key) {
    // 1. SAFETY CHECK: If user just types 'get' with no name
    if (key == 0 || key[0] == '\0') {
        vga_write("Usage: get [key]\n");
        return;
    }

    for(int i = 0; i < 10; i++) {
        if(env_table[i].active) {
            // 2. Double check the stored key exists before comparing
            if (env_table[i].key[0] != '\0' && strcmp(env_table[i].key, key) == 0) {
                vga_write(env_table[i].value);
                vga_write("\n");
                return;
            }
        }
    }
    vga_write("Error: Variable not found.\n");
}

void todo_add(char* text) {
    for(int i = 0; i < 10; i++) {
        if(!my_list[i].active) {
            strcpy(my_list[i].task, text);
            my_list[i].done = 0;
            my_list[i].active = 1;
            vga_write("Task added to ScribbleOS list.\n");
            return;
        }
    }
    vga_write("Error: Your brain (list) is full!\n");
}
void todo_show() {
    int found = 0;
    for(int i = 0; i < 10; i++) {
        // Only print if the slot is explicitly marked active
        if(my_list[i].active == 1 && my_list[i].task[0] != '\0') {
            vga_write("- ");
            vga_write(my_list[i].task);
            vga_write("\n");
            found = 1;
        }
    }
    if(!found) vga_write("No tasks found.\n");
}

void draw_menu_item(int id, int selected, const char* text) {
    if (id == selected) {
        // Highlighting logic
        vga_write(" > ");               // Arrow pointer
        vga_set_color(0x70);            // Invert: Black text on Light Gray background
        vga_write(text);
        vga_set_color(NOTEBOOK_YELLOW); // Reset to standard ScribbleOS Yellow/Blue
    } else {
        vga_write("   ");               // Spacer for non-selected items
        vga_write(text);
    }
    vga_write("\n");
}
void cmd_menu(char* args) {
    int selected = 1;
    int total_options = 7; 
    int running = 1;

    vga_clear();

    while (running) {
        // Jump back to the top-left to overwrite, not scroll
        vga_set_cursor(0, 0); 

        // Header Section
        vga_set_color(0x1F); // White on Blue (Status Bar Style)
        vga_write("========================================\n");
        vga_write("          ScribbleOS 4.0 - TOOLBOX           \n");
        vga_write("      (Use Arrows to Move, Enter)       \n");
        vga_write("========================================\n\n");
        vga_set_color(NOTEBOOK_YELLOW);

        // Draw all buttons
        draw_menu_item(1, selected, "[ 1. SYSTEM INFO (NEOFETCH) ]");
        draw_menu_item(2, selected, "[ 2. TO-DO LIST (TDSHW)     ]");
        draw_menu_item(3, selected, "[ 3. CALCULATOR (CALC)      ]");
        draw_menu_item(4, selected, "[ 4. QURAN AYAH             ]");
        draw_menu_item(5, selected, "[ 5. DRAW PLANE ART         ]");
        draw_menu_item(6, selected, "[ 6. LOCK SYSTEM            ]");
        draw_menu_item(7, selected, "[ 7. EXIT MENU              ]");
        
        vga_write("\n========================================\n");

        // Polling loop for Keyboard Input
        while (!(inb(0x64) & 0x01)) {
            vga_draw_status_bar(); // Keeps the clock ticking!
        }

        unsigned char scancode = inb(0x60);

        // Arrow and Input Logic
        if (scancode == 0x48) {        // UP ARROW
            if (selected > 1) selected--;
        } 
        else if (scancode == 0x50) {   // DOWN ARROW
            if (selected < total_options) selected++;
        } 
        else if (scancode == 0x1C) {   // ENTER KEY
            vga_clear(); 
            
            // saExecute the selected tool
            if (selected == 1) { cmd_neofetch(0); sleep_ms(1000); vga_clear(); }
            if (selected == 2) { todo_show(); sleep_ms(1000); vga_clear(); }
            if (selected == 3) { vga_write("Use it in terminal\n"); sleep_ms(1000); vga_clear(); }
            if (selected == 4) { cmd_ayah(); sleep_ms(1000); vga_clear(); }
            if (selected == 5) { draw_custom_plane(); sleep_ms(1000); vga_clear(); }
            if (selected == 6) { shell_lock(); sleep_ms(1000); vga_clear(); }
            if (selected == 7) { running = 0; }

            // If we didn't exit, wait for a key before returning to menu
            if (running && selected != 7) {
                // deleted bc of sleep and this shit doesnt work
                while (!(inb(0x64) & 0x01)); 
                inb(0x60); // Flush the buffer
            }
        }
        else if (scancode == 0x01) {   // ESC KEY
            running = 0;
        }
    }

    vga_clear();
    vga_write("Returned to ScribbleOS Shell.\n> ");
}
void play_bad_apple() {
     unsigned short* vga_hardware = (unsigned short*)VGA_ADDRESS;
     for (int f = 0; f < APPLE_FRAME_COUNT; f++) {
         const char* frame = apple_frames[f];
         for (int i = 0; i < 1920; i++) { // Exactly 24 rows
             vga_hardware[i] = (unsigned short)frame[i] | (unsigned short)0x0F << 8;
         }
         // The Status Bar at row 24 is safe!
         if (f % 5 == 0) vga_draw_status_bar(); 
         sleep_ms(250);
     }
 }
void cmd_read_disk(char* args) {
	if (args == 0 || *args == '\0') {
		vga_write("Usage: read_sector [sector_num]\n");
		return;
	}
	uint32_t lba = (uint32_t)atoi_custom(args);
	uint8_t sector_buffer[512];
	ide_read_sector_bytes(lba, sector_buffer);
	vga_write("\nLBA Sector: ");
	vga_write(args);
	vga_write("\n------------------------------------------\n");
	for (int i = 0; i < 128; i++) {
		char hex_buf[4];
		itohex(sector_buffer[i], hex_buf);
		if (sector_buffer[i] < 0x10)
		vga_write("0");
		vga_write(hex_buf);
		vga_write(" ");
		if ((i + 1) % 16 == 0) {
			vga_write(" | ");
			for (int j = i - 15; j <= i; j++) {
				char c = (char)sector_buffer[j];
				if (c >= 32 && c <= 126)
				vga_putchar(c);
				else vga_putchar('.');
			}
			vga_write("\n");
		}
	}
	vga_write("------------------------------------------\n");
}
void cmd_write_disk(char* args) {
    if (args == 0 || *args == '\0') {
        vga_write("Usage: write_sector <lba> <string>\n");
        vga_write("Example: write_sector 10 Hello_ScribbleOS\n");
        return;
    }

    char* lba_str = args;
    char* data_str = 0;

    // Correctly loop over the string to locate the true data boundaries
    for (int i = 0; lba_str[i] != '\0'; i++) {
        if (lba_str[i] == ' ') {
            lba_str[i] = '\0';
            data_str = &lba_str[i + 1];
            break;
        }
    }

    // Handle extra spaces if the user types multiple spaces before the word
    if (data_str) {
        while (*data_str == ' ') {
            data_str++;
        }
    }

    if (!data_str || *data_str == '\0') {
        vga_write("Error: Missing data string.\n");
        return;
    }

    uint32_t lba = (uint32_t)atoi_custom(lba_str);

    // Dedicated, isolated 512-byte buffer
    static uint8_t sector_buffer[512];
    for (int i = 0; i < 512; i++) {
        sector_buffer[i] = 0;
    }

    // Safely copy string payload into the padded sector block
    int len = 0;
    while (data_str[len] != '\0' && len < 512) {
        sector_buffer[len] = (uint8_t)data_str[len];
        len++;
    }

    vga_write("ScribbleOS Disk: Writing to LBA ");
    vga_write(lba_str); 
    vga_write("... ");

    ide_write_sector_bytes(lba, sector_buffer);

    vga_write("DONE!!\n");
}
uint32_t ide_calculate_usage_FULL() {
    uint8_t buffer[512];
    uint32_t used_count = 0;
    uint32_t total_sectors = ide_get_total_sectors();

    vga_write("Scanning disk... "); // Warning for the user

    for (uint32_t s = 0; s < total_sectors; s++) {
        ide_read_sector_bytes(s, buffer);
        
        // Check if sector is non-zero
        for (int i = 0; i < 512; i++) {
            if (buffer[i] != 0) {
                used_count++;
                break; 
            }
        }

        // Progress indicator every 1000 sectors so i know it's alive
        if (s % 1000 == 0) vga_putchar('.'); 
    }
    
    vga_write(" DONE\n");
    return used_count;
}

void print_disk_info(char* args) {
    uint32_t total_sectors = ide_get_total_sectors();
    if (total_sectors == 0) {
        vga_write("Error: No drive found.\n");
        return;
    }

    // WARNING: This will be slow on large disks!
    uint32_t used_sectors = ide_calculate_usage_FULL(); 
    uint32_t free_sectors = total_sectors - used_sectors;

    char unit = 'm'; 
    if (args && args[0] != '\0') unit = args[0];

    // Using 64-bit math to be safe
    uint64_t total_f, used_f, free_f;
    const char* label;

    if (unit == 'k') {
        total_f = (uint64_t)total_sectors / 2;
        used_f  = (uint64_t)used_sectors / 2;
        label   = " KB";
    } else if (unit == 'g') {
        total_f = (uint64_t)total_sectors / 2097152;
        used_f  = (uint64_t)used_sectors / 2097152;
        label   = " GB";
    } else {
        total_f = (uint64_t)total_sectors / 2048;
        used_f  = (uint64_t)used_sectors / 2048;
        label   = " MB";
    }

    // Floor protection: if sectors > 0, show at least 1 unit
    if (used_sectors > 0 && used_f == 0) used_f = 1;
    free_f = total_f - used_f;

    char b1[20], b2[20], b3[20];
    vga_write("\nTotal: "); vga_write(itoa((int)total_f, b1)); vga_write(label);
    vga_write("\nUsed:  "); vga_write(itoa((int)used_f, b2));  vga_write(label);
    vga_write("\nFree:  "); vga_write(itoa((int)free_f, b3));  vga_write(label);
    vga_write("\n");
}
void cmd_disk_wipe(char* args) {
    uint32_t total_sectors = ide_get_total_sectors();
    uint8_t zero_buffer[512];
    
    // Fill buffer with zeros once
    for (int i = 0; i < 512; i++) zero_buffer[i] = 0;

    vga_write("CRITICAL: Wiping Disk... ");

    for (uint32_t s = 0; s < total_sectors; s++) {
        ide_write_sector_bytes(s, zero_buffer);
        
        // Progress bar every 5%
        if (s % (total_sectors / 20) == 0) vga_write("#");
    }

    vga_write("\nDisk Erased Successfully.\n");
}

void seed_rand(uint32_t seed) {
    next_rand = seed;
}

uint8_t get_rand_byte() {
    // Simple LCG formula
    next_rand = next_rand * 1103515245 + 12345;
    return (uint8_t)(next_rand / 65536) % 256;
}

void cmd_disk_random(char* args) {
    uint32_t total_sectors = ide_get_total_sectors();
    uint8_t rand_buffer[512];
    
    // Seed the randomizer using current seconds
    seed_rand(cmos_get_sec());

    vga_write("Writing Random Noise... ");

    for (uint32_t s = 0; s < total_sectors; s++) {
        // Fill buffer with new random noise for every sector
        for (int i = 0; i < 512; i++) {
            rand_buffer[i] = get_rand_byte();
        }

        ide_write_sector_bytes(s, rand_buffer);

        if (s % (total_sectors / 20) == 0) vga_write("?");
    }

    vga_write("\nDisk Randomized.\n");
}


void cmd_disk_speed(char* args) {
    uint32_t total_sectors = ide_get_total_sectors();
    
    // We need at least 10MB of space to run a proper test
    if (total_sectors < 40000) {
        vga_write("Disk too small for reliable speed test.\n");
        return;
    }

    uint8_t buffer[512];
    // Fill buffer with a pattern so the disk actually has to work
    for(int i = 0; i < 512; i++) buffer[i] = (uint8_t)(i % 255);

    // Test 4MB (8192 sectors)
    uint32_t test_sectors = 8192; 
    uint32_t safe_offset = 20000; // Start 10MB into the disk
    
    vga_write("--- ScribbleOS 4.0 I/O Benchmark (4MB Test) ---\n");
    vga_write("Target: LBA "); 
    char lba_buf[16]; vga_write(itoa(safe_offset, lba_buf));
    vga_write("\n\n");

    // --- WRITE TEST ---
    vga_write("Testing Write Speed... ");
    unsigned int start_w = get_uptime_ms(); 
    for (uint32_t s = 0; s < test_sectors; s++) {
        ide_write_sector_bytes(safe_offset + s, buffer);
        
        // Minor progress update every 1MB
        if (s % 2048 == 0 && s > 0) vga_putchar('#');
    }
    unsigned int time_w = get_uptime_ms() - start_w;
    vga_write(" Done.\n");

    // --- READ TEST ---
    vga_write("Testing Read Speed...  ");
    unsigned int start_r = get_uptime_ms();
    for (uint32_t s = 0; s < test_sectors; s++) {
        ide_read_sector_bytes(safe_offset + s, buffer);
        
        if (s % 2048 == 0 && s > 0) vga_putchar('#');
    }
    unsigned int time_r = get_uptime_ms() - start_r;
    vga_write(" Done.\n\n");

    // --- RESULTS CALCULATION ---
    char buf_w[16], buf_r[16], ms_buf[16];

    // Formula: (4MB * 1000) / time_in_ms = MB/s
    vga_write("RESULTS:\n");
    
    if (time_w > 0) {
        uint32_t speed_w = (4 * 1000) / time_w;
        vga_write("  Write: "); vga_write(itoa(speed_w, buf_w)); vga_write(" MB/s ");
        vga_write("("); vga_write(itoa(time_w, ms_buf)); vga_write(" ms)\n");
    } else {
        vga_write("  Write: Too fast to measure (< 1ms)\n");
    }

    if (time_r > 0) {
        uint32_t speed_r = (4 * 1000) / time_r;
        vga_write("  Read:  "); vga_write(itoa(speed_r, buf_r)); vga_write(" MB/s ");
        vga_write("("); vga_write(itoa(time_r, ms_buf)); vga_write(" ms)\n");
    } else {
        vga_write("  Read:  Too fast to measure (< 1ms)\n");
    }
    
    vga_write("\nNote: Speeds are limited by PIO Mode overhead.\n");
}

char* get_filename_arg(char* args) {
    if (args == 0 || *args == '\0') return 0;
    while (*args == ' ') args++; // Skip leading spaces
    if (*args == '\0') return 0;
    return args;
}

void kgets_multiline(char* buffer, int max_len) {
    int i = 0;
    vga_write("(Press ESC to save and exit)\n> ");

    while (i < max_len - 1) {
        char c = wait_for_key();

        // 27 is the ASCII/Scancode often used for ESC
        if (c == 27) { 
            buffer[i] = '\0';
            vga_write("\n[Saving...]\n");
            break;
        } 
        
        else if (c == '\n') {
            buffer[i++] = '\n';
            vga_putchar('\n');
            vga_write("> "); // Visual cue for new line
        } 
        
        else if (c == '\b') {
            if (i > 0) {
                if (buffer[i-1] == '\n') {
                    // Logic for backspacing a newline is tricky in VGA
                    // For now, let's just prevent backspacing past a newline
                    continue; 
                }
                i--;
                vga_putchar('\b');
            }
        } 
        
        else if (c >= 32 && c <= 126) {
            buffer[i++] = c;
            vga_putchar(c);
        }
    }
    buffer[i] = '\0';
}

void kgets(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = wait_for_key();

        if (c == '\n') {
            buffer[i] = '\0';
            vga_putchar('\n');
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                // Standard VGA backspace: move back, print space, move back
                vga_putchar('\b'); 
            }
        } else if (c >= 32 && c <= 126) { // Printable characters only
            buffer[i++] = c;
            vga_putchar(c);
        }
    }
    buffer[i] = '\0';
}
void cmd_edit(char* args) {
    char* filename = get_filename_arg(args);

    if (!filename || *filename == '\0') {
        vga_write("Usage: edit <filename>\n");
        return;
    }

    vga_write("\n--- ScribbleOS 4 Multiline Notebook ---\n");
    vga_write("File: "); vga_write(filename); vga_write("\n");

    static char note_content[512];
    for(int i = 0; i < 512; i++) note_content[i] = 0;

    kgets_multiline(note_content, 511);

    // Fallback protection: if text is empty, prevent empty drive loops
    if (strlen(note_content) == 0) {
        strcpy(note_content, " "); // Force at least one valid character space
    }

    if (alifs_create(filename, note_content) == 0) {
        vga_write("Note saved to AliFS.\n");
    } else {
        vga_write("Error: Sector write failed.\n");
    }
}


void cmd_touch(char* args) {
    char* filename = get_filename_arg(args);

    if (!filename || *filename == '\0') {
        vga_write("Usage: touch <filename>\n");
        return;
    }

    if (alifs_create(filename, "") == 0) {
        vga_write("Created empty file: ");
        vga_write(filename);
        vga_write("\n");
    } else {
        vga_write("Error: Could not create file.\n");
    }
}
void cmd_cat(char* args) {
    char* filename = get_filename_arg(args);
    if (!filename || *filename == '\0') {
        vga_write("Usage: cat <filename>\n");
        return;
    }

    char* content = alifs_read(filename); 

    if (content) {
        vga_write("\n--- "); vga_write(filename); vga_write(" ---\n");
        vga_write(content);
        vga_write("\n------------------\n");
    } else {
        vga_write("Error: File not found.\n");
    }
}
void cmd_format(char* args) {
    vga_write("\n--- AliFS File System Format ---\n");
    vga_write("WARNING: This will clear the Inode table at LBA 20001.\n");
    vga_write("All files in the Notebook will be lost.\n");
    vga_write("Type 'CONFIRM' to proceed: ");

    char confirm[10];
    kgets(confirm, 9);

    if (strcmp(confirm, "CONFIRM") == 0) {
        vga_write("Formatting... ");
        alifs_format();
        vga_write("SUCCESS.\n");
    } else {
        vga_write("Format aborted.\n");
    }
}
void cmd_ls(char* args) {
    // alifs_list() handles the disk reading and VGA printing internally
    alifs_list();
}

void cmd_cd(char* args) {
    // 1. Handle root return
    if (args == 0 || args[0] == '\0' || strcmp(args, "/") == 0) {
        strcpy(current_path, "/");
        vga_write("Returned to root.\n");
        return;
    }

    // 2. Handle '..' (Go up one level)
    if (strcmp(args, "..") == 0) {
        if (strcmp(current_path, "/") == 0) {
            vga_write("Already at root.\n");
            return;
        }

        // Find the last slash to trim the path
        char* last_slash = strrchr(current_path, '/');
        
        if (last_slash == current_path) {
            // We are at /folder, so go back to /
            strcpy(current_path, "/");
        } else {
            // Trim path: "/a/b" becomes "/a"
            *last_slash = '\0';
        }
        vga_write("Moved up to: "); vga_write(current_path); vga_write("\n");
        return;
    }

    if (alifs_is_directory(args)) {
        if (strcmp(current_path, "/") == 0) {
            char temp[256];
            temp[0] = '/';
            strcpy(&temp[1], args);
            strcpy(current_path, temp);
        } else {
            int len = strlen(current_path);
            current_path[len] = '/';
            strcpy(&current_path[len+1], args);
        }
    } else {
        vga_write("Error: Directory not found.\n");
    }
}


void cmd_mkdir(char* args) {
    char* dirname = get_filename_arg(args);
    if (!dirname) {
        vga_write("Usage: mkdir <name>\n");
        return;
    }

    if (alifs_mkdir(dirname) == 0) {
        vga_write("Directory created.\n");
    } else {
        vga_write("Error: Could not create directory.\n");
    }
}

void aosdcserver() {
    vga_write("https://discord.gg/ymxpjGq9Gu");
}
void cmd_asma(char* args) {
    name_99_t names[] = {
        {"Ar-Rahman", "The Beneficent", "He who wills goodness and mercy for all His creatures."},
        {"Ar-Rahim", "The Merciful", "He who acts with extreme kindness."},
        {"Al-Malik", "The Eternal Lord", "The Sovereign Lord, The One with complete Dominion."},
        {"Al-Quddus", "The Most Sacred", "The One who is pure from any imperfection."},
        {"As-Salam", "The Embodiment of Peace", "The One who frees His servants from all danger."},
        {"Al-Mu'min", "The Infuser of Faith", "The One who witnessed for Himself and whose help is explained."},
        {"Al-Muhaymin", "The Preserver of Safety", "The One who witnesses the evolution of His creatures."},
        {"Al-Aziz", "The Mighty One", "The Victorious One where no resistance can be raised."},
        {"Al-Jabbar", "The Omnipotent One", "The Irresistible Subduer."},
        {"Al-Mutakabbir", "The Dominant One", "The One who is proud and beyond every creation."}
    };

    int db_size = sizeof(names) / sizeof(name_99_t);
    
    // Seed the randomness using your CMOS and Uptime
    int r = (cmos_get_sec() + (get_uptime_ms() % 100)) % db_size;

    vga_write("\n--- [ ASMA-UL-HUSNA ] ---\n");
    vga_write("Name: "); vga_write(names[r].ar);
    vga_write(" ("); vga_write(names[r].en); vga_write(")\n");
    vga_write("Meaning: "); vga_write(names[r].meaning);
    vga_write("\n-------------------------\n");
}
void install_aos() {
  cmd_install_os();
}
void cmd_divbyzero(char* args) {
    volatile int x = 0;
    volatile int y = 5; 
    
    vga_write("\n  [!] EXECUTING ILLEGAL INSTRUCTION: DIVIDE BY ZERO\n");    
    volatile int result = y / x; 
    
    (void)result; 
}
void gui() {
  cmd_start_gui();
}
/* Helper to print the full 0-F color lookup table */
void print_color_table() {
    vga_write(" Hex | Color Name         | Hex | Color Name\n");
    vga_write("-----+--------------------+-----+--------------------\n");
    vga_write("  0  | Black              |  8  | Dark Gray\n");
    vga_write("  1  | Blue               |  9  | Light Blue\n");
    vga_write("  2  | Green              |  A  | Light Green\n");
    vga_write("  3  | Cyan               |  B  | Light Cyan\n");
    vga_write("  4  | Red                |  C  | Light Red\n");
    vga_write("  5  | Magenta            |  D  | Light Magenta\n");
    vga_write("  6  | Brown              |  E  | Yellow\n");
    vga_write("  7  | Light Gray         |  F  | Bright White\n");
}

/* Helper to convert a single character ('0'-'F') into its integer hex value */
int parse_single_hex_char(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid hex character
}

/* The interactive color wizard command */
void cmd_color(char* args) {
    (void)args; // Ignore any string arguments typed after the command
    
    char input_buf[10];
    int fg = -1;
    int bg = -1;

    // --- STEP 1: FOREGROUND (TEXT) SELECTION ---
    print_color_table();
    vga_write("\nWhat color do you want the text to be? (Enter 0-F): ");
    
    kgets(input_buf, 9);
    fg = parse_single_hex_char(input_buf[0]);

    if (fg < 0 || fg > 15) {
        vga_write("Error: Invalid color selection code. Aborting.\n");
        return;
    }

    vga_write("\n");

    // --- STEP 2: BACKGROUND SELECTION ---
    print_color_table();
    vga_write("\nWhat color do you want the background to be? (Enter 0-F): ");
    
    kgets(input_buf, 9);
    bg = parse_single_hex_char(input_buf[0]);

    if (bg < 0 || bg > 15) {
        vga_write("Error: Invalid color selection code. Aborting.\n");
        return;
    }

    // --- STEP 3: BUILD AND SET ATTRIBUTE BYTE ---
    // Shift background to high nibble, leave foreground in low nibble
    unsigned char final_attribute = (unsigned char)((bg << 4) | fg);

    vga_set_color(final_attribute);

    vga_write("\nMatrix updated successfully!\n");
}


// Helper to draw a single module at a specific coordinate
static inline void draw_qr_module(unsigned short* vga, int row, int col, int bit) {
    // 1 = Black module (0x00 background), 0 = White background (0xF0)
    unsigned char bg = (bit == 1) ? 0x00 : 0xF0;
    unsigned short cell = (unsigned short)' ' | (bg << 8);

    // Calculate buffer index
    int index = (row * WIDTH) + (col * 2);

    // Fill 2x1 cell in VGA buffer
    vga[index] = cell;
    vga[index + 1] = cell;
}

void display_discord_qr() {
    unsigned short* vga_hardware = (unsigned short*)VGA_ADDRESS;
    tty_t* active = &ttys[current_tty];

    // 1. Save current state
    int old_status_bar = status_bar_enabled;
    status_bar_enabled = 0; // Disable status bar so it doesn't overwrite
    
    // 2. Clear screen to a neutral white for the QR quiet zone
    unsigned short white_cell = (unsigned short)' ' | (0xF0 << 8);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        vga_hardware[i] = white_cell;
        active->buffer[i] = white_cell;
    }

    // 3. Draw QR code
    // Version 1 is 21x21. Starting at 19 gives perfect horizontal centering.
    const int start_row = 2;
    const int start_col = 19; 

    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            draw_qr_module(vga_hardware, start_row + y, start_col + x, alios_discord_qr[y][x]);
            draw_qr_module(active->buffer, start_row + y, start_col + x, alios_discord_qr[y][x]);
        }
    }

    // 4. Wait for user interaction or timeout
    // Using a loop to check for a keypress keeps the system responsive
    for (int i = 0; i < 100; i++) { // Roughly 10 seconds
        if (inb(0x64) & 1) break; // Exit early if key pressed
        sleep_ms(100);
    }
    // Flush key if pressed
    if (inb(0x64) & 1) inb(0x60); 

    // 5. Restore state and clean up
    status_bar_enabled = old_status_bar;
    vga_clear(); 
}

void enable_status_bar() {
  status_bar_enabled = 1;
}
void disable_status_bar() {
  status_bar_enabled = 0;
}
void killscreen() {
    vga_set_color(0x47); // Red background, light gray text
    vga_clear();         // Flash screen red
    
    // Print the skull
    vga_write("             __             \n");
    vga_write("          ,g$$$$Sk.         \n");
    vga_write("        ,d$$$$$$$$$$k.      \n");
    vga_write("      ,?^?$?°\'  \'?$SS$$L.   \n");
    vga_write("     ,?  $SL._  ,d$iIS$$SL  \n");
    vga_write("    j$Su%$$$$$$$$?:iIS$$Sb  \n");
    vga_write("   :?°?^4$S$$\"°?$SL:iIS$SI: \n");
    vga_write("   :\'   \',\'?$\' .  \'?LiS$$SI:\n");
    vga_write("   \'                ?kiSSI? \n");
    vga_write("        :.       $k _ \'?Si7 \n");
    vga_write("       .,_,_     i$S%7·:i?\' \n");
    vga_write("      ?%uS%uo d$$?\'·?°      \n");
    vga_write("      $$$$$$$$$$i           \n");
    vga_write("      ·?$$S?                \n");

    // ENTER THE INFINITE LOOP
    while(1) {
        // 1. Flash to inverted colors
        vga_set_color(0x74); 
        for(volatile int d = 0; d < 80000000; d++); 

        // 2. Flash back to red
        vga_set_color(0x47);
        for(volatile int d = 0; d < 80000000; d++); 

        // 3. Run the dropping siren audio swoop
        for (int freq = 2000; freq > 100; freq -= 5) {
            play_sound(freq);
            for(volatile int i = 0; i < 200000; i++); 
        }
        
        // 4. Low flatline drone before the loop restarts
        play_sound(80); 
        for(volatile int i = 0; i < 60000000; i++);
    }
}
void cmd_cmatrix(char* args) {
    (void)args;

    // Track the row index for the falling drop in each column
    int matrix_pos[CMATRIX_COLS];
    // Track how fast or delayed each column is
    int matrix_delay[CMATRIX_COLS];

    // Seed local PRNG state using kernel hooks
    uint32_t local_rand = cmos_get_sec() + get_uptime_ms();

    // Clear the screen completely to black first (attribute 0x00)
    vga_set_color(0x00);
    vga_clear();

    // Initialize columns at random starting rows above the screen
    for (int i = 0; i < CMATRIX_COLS; i++) {
        local_rand = local_rand * 1103515245 + 12345;
        matrix_pos[i] = -((local_rand / 65536) % CMATRIX_ROWS);
        matrix_delay[i] = (local_rand % 3); 
    }

    // Drain any leftover scancodes from pressing Enter
    while (inb(0x64) & 0x01) {
        inb(0x60); 
    }

    volatile unsigned short* vga = (volatile unsigned short*)VGA_ADDRESS;
    int running = 1;
    uint32_t loops = 0;

    while (running) {
        // Keep clock ticking if the status bar is enabled
        if (loops % 1 == 0) {
            vga_draw_status_bar();
        }

        for (int c = 0; c < CMATRIX_COLS; c++) {
            // Speed control based on the column's assigned delay factor
            if (loops % (matrix_delay[c] + 1) != 0) continue;

            int current_row = matrix_pos[c];

            // 1. Draw leading edge (Bright White on Black: 0x0F)
            if (current_row >= 0 && current_row < CMATRIX_ROWS) {
                local_rand = local_rand * 1103515245 + 12345;
                char glyph = 33 + ((local_rand / 65536) % 93); // Printable ASCII
                vga[(current_row * CMATRIX_COLS) + c] = (unsigned short)glyph | (0x0F << 8);
            }

            // 2. Fade the previous head to Light Green (0x0A)
            if (current_row - 1 >= 0 && current_row - 1 < CMATRIX_ROWS) {
                int idx = ((current_row - 1) * CMATRIX_COLS) + c;
                vga[idx] = (vga[idx] & 0x00FF) | (0x0A << 8);
            }

            // 3. Fade older trailing characters to Dark Green (0x02)
            if (current_row - 4 >= 0 && current_row - 4 < CMATRIX_ROWS) {
                int idx = ((current_row - 4) * CMATRIX_COLS) + c;
                vga[idx] = (vga[idx] & 0x00FF) | (0x02 << 8);
            }

            // 4. Wipe the tail of the stream to Black space
            if (current_row - 12 >= 0 && current_row - 12 < CMATRIX_ROWS) {
                vga[((current_row - 12) * CMATRIX_COLS) + c] = (unsigned short)' ' | (0x00 << 8);
            }

            // Advance the stream
            matrix_pos[c]++;

            // Loop reset once the trailing dark space passes off-screen
            if (matrix_pos[c] - 12 >= CMATRIX_ROWS) {
                matrix_pos[c] = 0;
                local_rand = local_rand * 1103515245 + 12345;
                matrix_delay[c] = (local_rand % 3);
            }
        }

        // Check the keyboard data port (0x64). Exit instantly if any key is typed.
        if (inb(0x64) & 0x01) {
            unsigned char code = inb(0x60);
            // Only exit if it's a "Make code" (key down, below 0x80)
            if (code < 0x80) {
                running = 0;
                break;
            }
        }

        // Execution delay loop balanced for slower mobile emulators
        for (volatile int d = 0; d < 1200000; d++);
        loops++;
    }

    // Clean up and return to default colors
    vga_set_color(NOTEBOOK_YELLOW);
    vga_clear();
}

void cmd_dvd(char* args) {
    (void)args;

    // Starting positions (centered)
    int x = 33;
    int y = 10;
    
    // Box dimensions expanded to perfectly fit ScribbleOS
    int box_w = 14;
    int box_h = 3;

    // Movement vectors
    int dx = 1;
    int dy = 1;

    // Color array to cycle through on bounces (excluding black)
    unsigned char colors[] = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x02, 0x03, 0x05, 0x06};
    int color_index = 0;
    unsigned char current_color = colors[color_index];

    // Clear screen entirely
    vga_set_color(0x00);
    vga_clear();

    // === CRITICAL FLUSH ===
    // Drain the keyboard controller completely before starting
    while (inb(0x64) & 0x01) { 
        inb(0x60); 
    }

    volatile unsigned short* vga = (volatile unsigned short*)VGA_ADDRESS;
    int running = 1;
    uint32_t loops = 0;

    while (running) {
        if (loops % 1 == 0) {
            vga_draw_status_bar();
        }

        // 1. ERASE the old box position
        for (int row = 0; row < box_h; row++) {
            for (int col = 0; col < box_w; col++) {
                int tx = x + col;
                int ty = y + row;
                if (tx >= 0 && tx < DVD_COLS && ty >= 0 && ty < DVD_ROWS) {
                    vga[(ty * DVD_COLS) + tx] = (unsigned short)' ' | (0x00 << 8);
                }
            }
        }

        // 2. MOVE the box position
        x += dx;
        y += dy;

        // 3. COLLISION DETECTION & COLOR CHANGE
        int bounced = 0;

        if (x <= 0) {
            x = 0;
            dx = 1;
            bounced = 1;
        } else if (x + box_w >= DVD_COLS) {
            x = DVD_COLS - box_w;
            dx = -1;
            bounced = 1;
        }

        if (y <= 0) {
            y = 0;
            dy = 1;
            bounced = 1;
        } else if (y + box_h >= DVD_ROWS) {
            y = DVD_ROWS - box_h;
            dy = -1;
            bounced = 1;
        }

        if (bounced) {
            color_index = (color_index + 1) % (sizeof(colors));
            current_color = colors[color_index];
        }

        // 4. DRAW the ScribbleOS box position
        for (int row = 0; row < box_h; row++) {
            for (int col = 0; col < box_w; col++) {
                int tx = x + col;
                int ty = y + row;
                
                if (tx >= 0 && tx < DVD_COLS && ty >= 0 && ty < DVD_ROWS) {
                    char display_char = ' ';
                    
                    // Borders
                    if (row == 0 || row == box_h - 1) display_char = '-';
                    if (col == 0 || col == box_w - 1) display_char = '|';
                    if ((row == 0 || row == box_h - 1) && (col == 0 || col == box_w - 1)) display_char = '+';
                    
                    // Injecting "ScribbleOS" into the center row
                    if (row == 1 && col == 3) { vga[(ty * DVD_COLS) + tx] = 'A' | (current_color << 8); col++; }
                    else if (row == 1 && col == 4) { vga[(ty * DVD_COLS) + tx] = 'e' | (current_color << 8); col++; }
                    else if (row == 1 && col == 5) { vga[(ty * DVD_COLS) + tx] = 'r' | (current_color << 8); col++; }
                    else if (row == 1 && col == 6) { vga[(ty * DVD_COLS) + tx] = 'o' | (current_color << 8); col++; }
                    else if (row == 1 && col == 7) { vga[(ty * DVD_COLS) + tx] = '1' | (current_color << 8); col++; }
                    else if (row == 1 && col == 8) { vga[(ty * DVD_COLS) + tx] = 'E' | (current_color << 8); col++; }
                    else if (row == 1 && col == 9) { vga[(ty * DVD_COLS) + tx] = 'O' | (current_color << 8); col++; }
                    else if (row == 1 && col == 10) { vga[(ty * DVD_COLS) + tx] = 'S' | (current_color << 8); col++; }
                    else {
                        vga[(ty * DVD_COLS) + tx] = (unsigned short)display_char | (current_color << 8);
                    }
                }
            }
        }

        // 5. SECURE BREAK LOOP ON ACTUAL NEW KEYPRESS ONLY
        if (inb(0x64) & 0x01) {
            unsigned char code = inb(0x60);
            // Only exit if it's a "Make code" (key down, below 0x80)
            if (code < 0x80) {
                running = 0;
                break;
            }
        }

        // Delay loop for Limbo (Adjust this value up or down to speed/slow it)
        for (volatile int d = 0; d < 4000000; d++);
        loops++;
    }

    // Reset shell layout back to original notebook theme state cleanly
    vga_set_color(NOTEBOOK_YELLOW);
    vga_clear();
}
void cmd_socials(char* args) {
    (void)args; // Unused for this command

    vga_write("\n=================== ScribbleOS Developer Socials ===================\n");
    vga_write("  GitHub:    justlinuxyourself\n");
    vga_write("  Discord:   alithealiosowner\n");
    vga_write("  TikTok:    hisswx9\n");
    vga_write("  Insta:     alithefukinglinuxlover\n");
    vga_write("  Snapchat:  a56225047\n");
    vga_write("  Roblox:    justlinuxyourself\n");
    vga_write("  Chess.com: alitheosdev\n");
    vga_write("  Email:     alithefukinglinuxlover@gmail.com\n");
    vga_write("  Twitter/X: alitheAOSowner\n");
    vga_write("===============================================================\n\n");
}
void play_lullaby_sync() {
    // A longer, repeating lullaby melody
    // Frequencies: G4=392, A4=440, B4=494, C5=523, D5=587, E5=659
    
    // Repeat the sequence a few times to make it long
    for (int loop = 0; loop < 3; loop++) {
        // Part 1: Gentle start
        play_sound(392); sleep_ms(500);
        play_sound(392); sleep_ms(500);
        play_sound(440); sleep_ms(500);
        play_sound(392); sleep_ms(500);
        
        // Part 2: The climb
        play_sound(523); sleep_ms(600);
        play_sound(494); sleep_ms(700);
        
        // Short pause (stutter point)
        nosound(); sleep_ms(200);
        
        // Part 3: The descent
        play_sound(392); sleep_ms(500);
        play_sound(392); sleep_ms(500);
        play_sound(440); sleep_ms(500);
        play_sound(392); sleep_ms(500);
        
        // Part 4: Ending phrase
        play_sound(587); sleep_ms(600);
        play_sound(523); sleep_ms(800);
        
        // Long pause before repeating
        nosound(); sleep_ms(100);
    }
    
    // Final stop
    nosound();
}
void add_to_history(char* cmd) {
    // Copy command into the current slot
    for(int i = 0; i < 80; i++) {
        history[history_idx][i] = cmd[i];
        if(cmd[i] == '\0') break;
    }
    
    history_idx = (history_idx + 1) % MAX_HISTORY;
    if (history_count < MAX_HISTORY) history_count++;
}
void cmd_history(char* args) {
    vga_write("\n--- Command History ---\n");
    
    // We loop through the total number of commands stored
    for (int i = 0; i < history_count; i++) {
        // Print the sequence number (i + 1)
        char s_buf[8];
        vga_write("#");
        vga_write(itoa(i + 1, s_buf));
        vga_write(": ");
        
        // Print the command stored at index i
        vga_write(history[i]);
        vga_write("\n");
    }
    vga_write("> ");
}
void cmd_birthday(char* args) {
    // 1. Define birthday (Feb 13, 2026 = 31+13 = 44th day)
    int birth_day_of_year = 44;
    
    // 2. Fetch current date from CMOS
    int curr_month = cmos_get_month();
    int curr_day = cmos_get_day();
    
    // 3. Days in each month (non-leap year)
    int days_in_months[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int curr_day_of_year = 0;
    for (int i = 1; i < curr_month; i++) {
        curr_day_of_year += days_in_months[i];
    }
    curr_day_of_year += curr_day;
    
    int age_days = curr_day_of_year - birth_day_of_year;
    
    // 4. Output results
    vga_write("\n--- ScribbleOS Birthday Info ---\n");
    vga_write("Birthday: February 13, 2026\n");
    vga_write("Age: "); 
    char buf[8];
    vga_write(itoa(age_days, buf));
    vga_write(" days old.\n");
    vga_write("------------------------------\n");
}

unsigned char check_key() {
    // Only return if the buffer has data (bit 0 is set)
    if (inb(0x64) & 0x01) {
        return inb(0x60);
    }
    return 0; // Nothing pressed
}


#define MAX_SNAKE_LEN 100

void cmd_snake(char* args) {
    unsigned short* vga = (unsigned short*)VGA_ADDRESS;
    int body_x[MAX_SNAKE_LEN], body_y[MAX_SNAKE_LEN];
    int length = 3, head = 0;
    int x = 40, y = 12, dx = 1, dy = 0;
    int ax = 10, ay = 10;
    int running = 1;

    // Initialize body (starting at 40,12)
    for(int i = 0; i < MAX_SNAKE_LEN; i++) { body_x[i] = 0; body_y[i] = 0; }
    for(int i = 0; i < length; i++) { 
        body_x[i] = x - i; 
        body_y[i] = y; 
    }

    vga_clear();
    // Draw initial state
    vga[(ay * 80) + ax] = 0x1C40; // Apple

    while (running) {
        // 1. Input Logic (Filter out key releases)
        unsigned char key = check_key();
        if (key > 0 && key < 0x80) {
            if (key == 0x01) running = 0;              // ESC
            else if (key == 0x11) { dx = 0; dy = -1; } // W
            else if (key == 0x1E) { dx = -1; dy = 0; } // A
            else if (key == 0x1F) { dx = 0; dy = 1; }  // S
            else if (key == 0x20) { dx = 1; dy = 0; }  // D
        }

        // 2. Calculate next position
        int nx = body_x[head] + dx;
        int ny = body_y[head] + dy;

        // 3. Collision Check
        if (nx <= 0 || nx >= 79 || ny <= 0 || ny >= 23) {
            running = 0; // Hit wall
        } else {
            // 4. Update head
            head = (head + 1) % MAX_SNAKE_LEN;
            body_x[head] = nx;
            body_y[head] = ny;

            // 5. Apple logic
            if (nx == ax && ny == ay) {
                if (length < MAX_SNAKE_LEN - 1) length++;
                ax = (cmos_get_sec() % 77) + 1;
                ay = (get_uptime_ms() % 21) + 1;
                vga[(ay * 80) + ax] = 0x1C40; // New Apple
            } else {
                // 6. Erase tail
                int tail = (head - length + 1 + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
                vga[(body_y[tail] * 80) + body_x[tail]] = 0x1F20; // 0x0F20 is space
            }

            // 7. Draw Head
            vga[(ny * 80) + nx] = 0x1F4F; 
        }

        sleep_ms(150);
    }
    vga_clear();
    vga_write("Game Over! Returned to shell.\n");
}
void run_full_posix_test() {
    vga_write("--- POSIX BRIDGE TEST START ---\n");

    // TEST 1: Syscall Write
    const char* msg = "SUCCESS: Syscall WRITE reached kernel!\n";
    __asm__ volatile (
      "mov $1, %%rax;"
      "mov $1, %%rbx;"
      "mov %0, %%rcx;"    // Pass pointer in RCX
      "mov $40, %%rdx;"
      "int $0x80;"
      : 
      : "r"(msg)          // The compiler will put the address of msg in a register
      : "rax", "rbx", "rcx", "rdx"
      );


    long fd_result;
    const char* path = "test.txt"; // Define the pointer explicitly
    __asm__ volatile (
      "mov $2, %%rax;"
      "mov %0, %%rbx;"    // Move pointer to RBX
      "int $0x80;"
      "mov %%rax, %1;"
      : "=m"(fd_result)
      : "r"(path)         // Compiler puts address of "test.txt" in path
      : "rax", "rbx"
      );


    if (fd_result >= 0) {
        vga_write("SUCCESS: AliFS file opened!\n");
        vga_write("fd_result: ");
        char buf[16];
        vga_write(ltoa(fd_result, buf));
        vga_write("\n");
    } else {
        vga_write("FAILURE: Could not open AliFS file.\n");
    }
}

void ddddmvse() {
  vga_write("DU ");
  play_sound(220);
  sleep(2);
  nosound();
  sleep_ms(100);
  
  vga_write("DU ");
  play_sound(220);
  sleep(2);
  nosound();
  sleep_ms(100);
  
  vga_write("DU ");
  play_sound(220);
  sleep(2);
  nosound();
  sleep_ms(100);
  
  vga_write("DU! ");
  play_sound(330);
  sleep(4);
  nosound();
  sleep_ms(100);
  
  vga_write("MAX ");
  play_sound(330);
  sleep(1);
  nosound();
  sleep_ms(100);
  
  vga_write("VER");
  play_sound(330);
  sleep(2);
  nosound();
  sleep_ms(100);
  
  vga_write("STAPP");
  play_sound(262);
  sleep(1);
  nosound();
  sleep_ms(100);
  
  vga_write("EN");
  play_sound(247);
  sleep(1);
  nosound();
  sleep_ms(100);
}
void cmd_remv(char* args) {
    // 1. Basic validation: ensure an argument was provided
    if (args == 0 || args[0] == '\0') {
        vga_write("Usage: remv <path>\n");
        return;
    }

    // 2. Safety: Prevent accidental deletion of the root directory
    if (strcmp(args, "/") == 0) {
        vga_write("Error: Cannot delete root directory.\n");
        return;
    }

    // 3. Logic to determine the target path
    // If the path is relative (e.g., 'mydir'), prepend current_path
    char target_path[256];
    if (args[0] == '/') {
        // Absolute path provided
        strcpy(target_path, args);
    } else {
        // Relative path: current_path + / + args
        if (strcmp(current_path, "/") == 0) {
            target_path[0] = '/';
            strcpy(target_path + 1, args);
        } else {
            strcpy(target_path, current_path);
            int len = strlen(target_path);
            target_path[len] = '/';
            strcpy(target_path + len + 1, args);
        }
    }

    // 4. Execute the deletion
    if (alifs_delete_recursive(target_path) == 0) {
        vga_write("Deleted: ");
        vga_write(target_path);
        vga_write("\n");
    } else {
        vga_write("Error: Could not delete path.\n");
    }
}

void cmd_pwd(char* args) {
    (void)args; // Silence unused warning
    vga_write(current_path);
    vga_write("\n");
}


void cmd_printto(char* args) {
    if (!args || *args == '\0') {
        vga_write("Usage: printto [tty] [message]\n");
        return;
    }

    char* tty_str = args;
    char* message = 0;

    // Split the first space to separate TTY index from the message
    for (int i = 0; args[i]; i++) {
        if (args[i] == ' ') {
            args[i] = '\0';
            message = &args[i+1];
            break;
        }
    }

    if (!message || *message == '\0') {
        vga_write("Error: Missing message.\n");
        return;
    }

    // Convert string digit to integer
    int target_tty = tty_str[0] - '0';

    // Validate
    if (target_tty < 0 || target_tty >= MAX_TTYS) {
        vga_write("Error: Invalid TTY index.\n");
        return;
    }

    // Call the print function
    print_to_tty(message, target_tty);
}

void cmd_all_ascii() {
    // Buffer for a single character string
    char str[2] = {0, 0};
    
    // Total usable space = 24 rows * 80 columns = 1920 characters
    for (int i = 0; i < 1920; i++) {
        // Cycle through printable ASCII characters 33-126
        str[0] = (char)((i % 94) + 33);
        
        // Print to the current TTY buffer
        vga_write(str);
    }
}
void cmd_get_ayah(char* args) {
    if (args == 0 || args[0] == '\0') {
        vga_write("Usage: getayah [surah] [ayah]\nExample: getayah 1 1\n");
        return;
    }

    char* surah_str = args;
    char* ayah_str = 0;

    // Split the input at the space
    for (int i = 0; args[i]; i++) {
        if (args[i] == ' ') {
            args[i] = '\0';
            ayah_str = &args[i+1];
            break;
        }
    }

    if (!ayah_str) {
        vga_write("Error: Please provide both Surah and Ayah numbers.\n");
        return;
    }

    int target_surah = atoi_custom(surah_str);
    int target_ayah = atoi_custom(ayah_str);

    int found = 0;
    int total_ayahs = sizeof(quran) / sizeof(Ayah);

    for (int i = 0; i < total_ayahs; i++) {
        if (quran[i].surah == target_surah && quran[i].ayah == target_ayah) {
            vga_write("\n");
            vga_write(quran[i].text);
            vga_write("\n");
            found = 1;
            break;
        }
    }

    if (!found) {
        vga_write("Verse not found.\n");
    }
}

void cmd_passwd(char* args) {
    char* arg1 = args;
    while (*arg1 == ' ') arg1++; // Skip any leading spaces in the argument block

    // If the string is empty, dump instructions immediately
    if (*arg1 == '\0') {
        vga_write("Usage:\n");
        vga_write("  passwd set <new_password>     - Create/change password (Max 10 chars)\n");
        vga_write("  passwd remove                 - Disable lockscreen (requires password)\n");
        return;
    }

    // Isolate the subcommand token by finding the trailing space or end of string
    char sub_cmd[16];
    int token_len = 0;
    while (arg1[token_len] != ' ' && arg1[token_len] != '\0' && token_len < 15) {
        sub_cmd[token_len] = arg1[token_len];
        token_len++;
    }
    sub_cmd[token_len] = '\0';

    // Set a pointer for the 'set' payload if present
    char* new_pass_ptr = &arg1[token_len];
    while (*new_pass_ptr == ' ') new_pass_ptr++; // Skip whitespace to isolate password

    char current_cmos_pass[11];
    cmos_read_password(current_cmos_pass);

    // --- 2. SUBCOMMAND: REMOVE ---
    if (strcmp(sub_cmd, "remove") == 0) {
        if (current_cmos_pass[0] == '\0') {
            vga_write("Lock screen is already disabled.\n");
            return;
        }

        vga_write("Enter CURRENT password to confirm removal: ");

        char confirm_input[11];
        int idx = 0;
        int clock_ticks = 0;

        // Manual Hardware Input Loop for Masked Password Verification
        while (1) {
            timer_wait_tick();
            if (++clock_ticks >= 100) {
                vga_draw_status_bar();
                clock_ticks = 0;
            }

            if (inb(0x64) & 0x01) {
                unsigned char scancode = inb(0x60);
                char c = kbd_get_char(scancode); // Convert scancode to ASCII
                if (c == 0) continue;

                if (c == '\n' || c == '\r') {
                    confirm_input[idx] = '\0';
                    vga_putchar('\n');
                    break;
                } 
                else if (c == '\b') {
                    if (idx > 0) { 
                        idx--; 
                        vga_putchar('\b'); 
                    }
                } 
                else if (c >= ' ' && idx < 10) {
                    confirm_input[idx++] = c;
                    vga_putchar('*'); // Mask identity safely on VGA
                }
            }
        }

        // Inline XOR cipher conversion
        char encrypted_confirm[11];
        int i = 0;
        while (confirm_input[i] != '\0' && i < 10) {
            encrypted_confirm[i] = confirm_input[i] ^ 0x80;
            i++;
        }
        encrypted_confirm[i] = '\0';

        if (strcmp(encrypted_confirm, current_cmos_pass) == 0) {
            char empty_pass[11] = {0};
            cmos_write_password(empty_pass);
            vga_write("Password removed from CMOS NVRAM. Lock disabled.\n");
        } else {
            vga_write("[ ACCESS DENIED ] Incorrect password. Action aborted.\n");
        }
        return;
    }

    // --- 3. SUBCOMMAND: SET ---
    if (strcmp(sub_cmd, "set") == 0) {
        if (*new_pass_ptr == '\0') {
            vga_write("Error: Please specify a password. (e.g., 'passwd set Aero1')\n");
            return;
        }

        // Check bounds manually on the arguments string
        int pass_len = 0;
        while (new_pass_ptr[pass_len] != '\0' && new_pass_ptr[pass_len] != ' ') {
            pass_len++;
        }

        if (pass_len > 10) {
            vga_write("Error: Password too long for CMOS registers! Max 10 characters.\n");
            return;
        }

        // Isolate the new clean string
        char clean_new_pass[11];
        for (int i = 0; i < pass_len; i++) {
            clean_new_pass[i] = new_pass_ptr[i];
        }
        clean_new_pass[pass_len] = '\0';

        // Require password verification ONLY if a current active password block is configured
        if (current_cmos_pass[0] != '\0') {
            vga_write("Enter CURRENT password to authorize change: ");
            
            char verify_input[11];
            int idx = 0;
            int clock_ticks = 0;
            while (1) {
                timer_wait_tick();
                if (++clock_ticks >= 100) {
                    vga_draw_status_bar();
                    clock_ticks = 0;
                }

                if (inb(0x64) & 0x01) {
                    unsigned char scancode = inb(0x60);
                    char c = kbd_get_char(scancode);
                    if (c == 0) continue;

                    if (c == '\n' || c == '\r') {
                        verify_input[idx] = '\0';
                        vga_putchar('\n');
                        break;
                    } 
                    else if (c == '\b') {
                        if (idx > 0) { 
                            idx--; 
                            vga_putchar('\b'); 
                        }
                    } 
                    else if (c >= ' ' && idx < 10) {
                        verify_input[idx++] = c;
                        vga_putchar('*');
                    }
                }
            }

            // Inline XOR checking routine
            char encrypted_verify[11];
            int i = 0;
            while (verify_input[i] != '\0' && i < 10) {
                encrypted_verify[i] = verify_input[i] ^ 0x80;
                i++;
            }
            encrypted_verify[i] = '\0';

            if (strcmp(encrypted_verify, current_cmos_pass) != 0) {
                vga_write("[ ACCESS DENIED ] Verification failed. Password unchanged.\n");
                return;
            }
        }

        // Encrypt our targeted text and commit strings down to CMOS banks
        char encrypted_new[11];
        int i = 0;
        while (clean_new_pass[i] != '\0' && i < 10) {
            encrypted_new[i] = clean_new_pass[i] ^ 0x80;
            i++;
        }
        encrypted_new[i] = '\0';

        cmos_write_password(encrypted_new);
        vga_write("Password safely encrypted and burned into hardware CMOS NVRAM storage.\n");
        return;
    }

    vga_write("Unknown subcommand. Type 'passwd' without arguments to see options.\n");
}


/* --- Shell Logic --- */
void shell_register_command(const char* name, const char* desc, command_func func) {
    command_node_t* new_node = (command_node_t*)kmalloc(sizeof(command_node_t));
    
    int i = 0;
    while(name[i] && i < 31) { new_node->name[i] = name[i]; i++; }
    new_node->name[i] = '\0';

    i = 0;
    while(desc[i] && i < 63) { new_node->description[i] = desc[i]; i++; }
    new_node->description[i] = '\0';

    new_node->function = func;
    new_node->next = command_list;
    command_list = new_node;
}

void shell_init() {
    shell_register_command("help", "List all available commands", cmd_help);
    shell_register_command("cls",  "Clear the notebook screen",   cmd_cls);
    shell_register_command("echo", "Print text to the screen",    cmd_echo);
    shell_register_command("sysinfo", "Display dynamic system info", cmd_neofetch);
    shell_register_command("uptime", "Show how long ScribbleOS has been running", cmd_uptime);
    shell_register_command("free", "Check dynamic RAM usage", cmd_free);
    shell_register_command("timezone", "Adjust the status bar clock offset", shell_cmd_timezone);
    shell_register_command("lock", "Locks the system", shell_lock);
    shell_register_command("test",     "Verify timer calibration",    cmd_test);
    shell_register_command("beep", "Play a system alert sound", cmd_beep);
    shell_register_command("about_dev", "About Dev", cmd_about_dev);
    shell_register_command("plane", "Show a art of a plane", draw_custom_plane);
    shell_register_command("twins", "Shows my twins names", twins);
    shell_register_command("sleep", "Sleep", sys_sleep);
    shell_register_command("calc", "Calculator", command_calc);
    shell_register_command("peek", "Inspect raw memory addresses", cmd_peek);
    shell_register_command("poke", "Write to memory addrs", cmd_poke);
    shell_register_command("run", "Execute AliScript code", cmd_run_script);
    shell_register_command("ayah", "Choose Random Quran Ayah and Print it (im turning into terry davis)", cmd_ayah);
    shell_register_command("verse", "Choose Random Bible Verse and Print it", cmd_verse);
    shell_register_command("set", "Set VAR", cmd_set);
    shell_register_command("get", "Get VAR", cmd_get);
    shell_register_command("tdadd", "Add to ToDo List", todo_add);
    shell_register_command("tdshw", "Show ToDo List", todo_show);
    shell_register_command("menu", "ScribbleOS Menu", cmd_menu);
    shell_register_command("badapple", "Bad Apple", play_bad_apple);
    shell_register_command("read_sector", "Read Sector IDE", cmd_read_disk);
    shell_register_command("write_sector", "Write Sector IDE", cmd_write_disk);
    shell_register_command("sfree", "Storage info: df [k|m|g]", print_disk_info);
    shell_register_command("dwipe", "Erase the whole disk (zero out)", cmd_disk_wipe);
    shell_register_command("dshred", "Fill the disk with random noise", cmd_disk_random);
    shell_register_command("rwsp", "R/W Disk Speed Test", cmd_disk_speed);
    shell_register_command("lidi", "List files on AliFS", cmd_ls);   
    shell_register_command("crfi", "Create a new empty file", cmd_touch);
    shell_register_command("editfi", "Write text to a file", cmd_edit);
    shell_register_command("refi", "Read file content", cmd_cat);
    shell_register_command("fmrt","Wipe and init AliFS", cmd_format);
    shell_register_command("mkdir", "Create a new directory", cmd_mkdir);
    shell_register_command("gtdi", "Go To DIrectory", cmd_cd);
    shell_register_command("aosdcserv", "ScribbleOS Discord Server", display_discord_qr);
    shell_register_command("asma", "Random Name of Allah and its meaning", cmd_asma);
    shell_register_command("install", "Install ScribbleOS", install_aos);
    shell_register_command("divbyzero", "DivbyZero", cmd_divbyzero);
    shell_register_command("gui", "GUI", gui);
    shell_register_command("color", "Interactive text and background color customization wizard", cmd_color);
    shell_register_command("disablestat", "DISABLE STATus bar", disable_status_bar);
    shell_register_command("enablestat", "ENABLE STATus bar", enable_status_bar);
    shell_register_command("killscreen", "KillScreen", killscreen);
    shell_register_command("cmatrix", "Matrix digital rain screen effect", cmd_cmatrix);
    shell_register_command("ss", "Bouncing ScribbleOS logo screensaver", cmd_dvd);
    shell_register_command("socials", "Display ScribbleOS creator contact", cmd_socials);
    shell_register_command("lullaby", "Lullaby (made it for my baby sis)", play_lullaby_sync);
    shell_register_command("history", "HISTORY", cmd_history);
    shell_register_command("birthday", "Show OS age and birthday", cmd_birthday);
    shell_register_command("snake", "Play Snake game", cmd_snake);
    shell_register_command("testposix", "TEST POSIX", run_full_posix_test);
    shell_register_command("max", "DUDUDUDUUUUU MAX VERSTAPPEN", ddddmvse);
    shell_register_command("remv", "REMOVE A FILE/DIRECTORY", cmd_remv);
    shell_register_command("wrdi", "WoRking DIrectory (current directory)", cmd_pwd);
    shell_register_command("printto", "Print to Another TTY", cmd_printto);
    shell_register_command("ascii", "Show All printable ASCII chars", cmd_all_ascii);
    shell_register_command("getayah", "Fetch a specific Quran verse", cmd_get_ayah);
    shell_register_command("passwd", "Set/Remove Passwoes", cmd_passwd);
}

void shell_dispatch(char* buffer) {
    // 1. Copy command into the current history index
        for (int i = 0; i < 79; i++) {
        history[history_idx][i] = buffer[i];
        if (buffer[i] == '\0') break;
    }
    history[history_idx][79] = '\0'; // Safety null-terminate

    // 2. Move index and update count
    history_idx++;
    if (history_idx >= MAX_HISTORY) {
        history_idx = 0; // Ring buffer wrap-around
    }
    
    // Increment history_count only until we fill the buffer
    if (history_count < MAX_HISTORY) {
        history_count++;
    }
    // If the user just hits enter, just print a new prompt on a new line
    if (strlen(buffer) == 0) {
        vga_write("\nScribbleOS:");
        vga_write(current_path);
        vga_write("> ");
        return;
    }

    char* args = 0;
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ' ') {
            buffer[i] = '\0';
            args = &buffer[i+1];
            break;
        }
    }

    command_node_t* curr = command_list;
    while (curr) {
        if (strcmp(curr->name, buffer) == 0) {
            vga_write("\n"); // Move to new line before command output
            curr->function(args);
            vga_write("\nScribbleOS:");
            vga_write(current_path);
            vga_write("> ");
            return;
        }
        curr = curr->next;
    }

    // If command not found
    vga_write("\nScribbleOS: '");
    vga_write(buffer);
    vga_write("' not found. Type 'help'.\n ");
    vga_write("\nScribbleOS:");
    vga_write(current_path);
    vga_write("> ");
}
void shell_tab_complete(char* buffer, int* len) {
    command_node_t* curr = command_list;
    while (curr) {
        if (strncmp(curr->name, buffer, *len) == 0) {
            char* rest = curr->name + *len;
            vga_write(rest);
            while (*rest) buffer[(*len)++] = *rest++;
            buffer[*len] = '\0';
            return;
        }
        curr = curr->next;
    }
}
