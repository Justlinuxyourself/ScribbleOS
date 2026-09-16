#include "section8_global-header/global.h"
/* src/kernel.c 
Copyright (c) 2026 Ali  
All rights reserved.
*/
#include "section1_cpu/io.h"
#include "section1_cpu/heap.h"
#include "section4_shell/shell.h"
#include "section6_pci/pci.h"
#include <stdbool.h>
// typedef unsigned long long uint64_t;
bool ata_probe(uint16_t port);
#define CMOS_INIT_FLAG_REG  0x32  // Flag
#define CMOS_MAGIC_VAL      0xA5  // A distinct byte to signal "Initialized"
int strcmp_custom(char* s1, char* s2) {
    int i = 0;
    while (s1[i] != '\0' || s2[i] != '\0') {
        if (s1[i] != s2[i]) return 0;
        i++;
    }
    return 1;
}

/* * permanent_lockout_siren: 
 * A siren that never stops. Call this when strikes >= 3.
 */
void permanent_lockout_siren() {
    while(1) {
        // High Tone (1200 Hz)
        play_sound(1200);
        sleep(1);

        // Low Tone (800 Hz)
        play_sound(800);
        sleep(1);
    }
}
void emergency_siren_slide() {
    while(1) {
        // Slide Up
        for (int freq = 400; freq < 1200; freq += 10) {
            play_sound(freq);
            sleep(1); // Very short sleep for a smooth slide
        }
        // Slide Down
        for (int freq = 1200; freq > 400; freq -= 10) {
            play_sound(freq);
            sleep(1);
        }
    }
}
// Helper to flash the screen and play a sound
void morse_pulse(int ms) {
    // 0x70 is light gray background, 0x07 is black background
    vga_set_attribute(0x70); 
     
    sleep_ms(ms);
    
    vga_set_attribute(0x07);
     
    sleep_ms(100); // Tiny gap
}

void trigger_ali_morse() {
    // A: .- (Dot, Dash)
    morse_pulse(100); morse_pulse(300);
    sleep_ms(300);

    // L: .-.. (Dot, Dash, Dot, Dot)
    morse_pulse(100); morse_pulse(300); morse_pulse(100); morse_pulse(100);
    sleep_ms(300);

    // I: .. (Dot, Dot)
    morse_pulse(100); morse_pulse(100);
    vga_write("PUT PASSWORD ATFER SCREEN IS CLEARED IN 3 SECS IF UR IN LOCK SCREEN EVEN IF THERES NO password: PROMPT\n");
    sleep_ms(3000);
    vga_clear();
}
void bootup_screen() {
    for (int i = 0; i < 7; i++) {
        vga_write("
    }                                                                              \n");
    vga_write("               ____            _ _     _     _       ___  ____                \n");
    vga_write("              / ___|  ___ _ __(_) |__ | |__ | | ___ / _ \/ ___|               \n");
    vga_write("              \___ \ / __| '__| | '_ \| '_ \| |/ _ \ | | \___ \               \n");
    vga_write("               ___) | (__| |  | | |_) | |_) | |  __/ |_| |___) |              \n");
    vga write("              |____/ \___|_|  |_|_.__/|_.__/|_|\___|\___/|____/               \n");
    vga_write("                                                                              \n");
    for (int i = 0; i < 8; i++) {
        vga_write("
    }                                                                              \n");
}

void lock_system_hardened() {
    char live_secret[11] = {0}; 
    cmos_read_password(live_secret);
    // If the first register is completely blank (0x00), skip the lock screen entirely
    if (live_secret[0] == '\0') {
        return;
    }
    while (inb(0x64) & 0x01) { inb(0x60); }
    
    char input[11] = {0};           
    char encrypted_input[11] = {0}; 
    int idx = 0;
    int strikes = get_failed_attempts(); 
    int clock_ticks = 0;

    vga_clear();

    while (1) {
        if (strikes >= 3) {
            vga_clear();
            vga_write("!!! SECURITY BREACH DETECTED !!!\n");
            vga_write("System has been permanently locked.\n");
            permanent_lockout_siren(); 
        }
        
        vga_write("          Enter Password: ");

        while (1) {
            timer_wait_tick();
            clock_ticks++;
            if (clock_ticks >= 100) {
                vga_draw_status_bar();
                clock_ticks = 0;
            }

            if (inb(0x64) & 0x01) {
                unsigned char scancode = inb(0x60);
                char c = kbd_get_char(scancode);
                if (c == 0) continue; 

                if (c == '\n') {
                    input[idx] = '\0';

                    // In-line XOR cipher execution
                    int i = 0;
                    while (input[i] != '\0' && i < 10) {
                        encrypted_input[i] = input[i] ^ 0x80;
                        i++;
                    }
                    encrypted_input[i] = '\0';

                    // Re-read active CMOS password state right before checking
                    cmos_read_password(live_secret);

                    if (strcmp_custom(encrypted_input, live_secret) == 1) {
                        set_failed_attempts(0); 
                        vga_clear();
                        return; 
                    } else {
                        strikes++;
                        set_failed_attempts(strikes); 
                        vga_write("\n          [ ACCESS DENIED ] - Strike ");
                        char s_buf[4];
                        vga_write(itoa(strikes, s_buf));
                        vga_write("/3\n");
                        
                        // Zero out sensitive plaintext/ciphertext buffers immediately on failure
                        for(int k = 0; k < 11; k++) {
                            input[k] = 0;
                            encrypted_input[k] = 0;
                        }
                        idx = 0;
                        break; 
                    }
                } 
                else if (c == '\b') {
                    if (idx > 0) { idx--; vga_putchar('\b'); }
                } 
                else if (c >= ' ' && idx < 10) { 
                    input[idx++] = c;
                    vga_putchar('*'); 
                }
            }
        }
    }
}




void todo_init() {
    for(int i = 0; i < 10; i++) {
        my_list[i].active = 0;
        my_list[i].task[0] = '\0'; // Manually kill any garbage data
    }
}

void print_dec(unsigned int val) {
    if (val == 0) {
        vga_putchar('0');
        return;
    }

    char buf[12];
    int i = 0;
    while (val > 0) {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (--i >= 0) {
        vga_putchar(buf[i]);
    }
}

void log_verbose(const char* subsystem, const char* msg) {
    int h = cmos_get_hour();
    int m = cmos_get_min();
    int s = cmos_get_sec();
    
    // Determine AM/PM
    const char* period = (h >= 12) ? "PM" : "AM";
    
    // Convert 24h to 12h format
    if (h == 0) h = 12;
    else if (h > 12) h -= 12;

    vga_write("[");
    if (h < 10) vga_putchar('0');
    print_dec(h); vga_putchar(':');
    
    if (m < 10) vga_putchar('0');
    print_dec(m); vga_putchar(':');
    
    if (s < 10) vga_putchar('0');
    print_dec(s);
    
    vga_write(" ");
    vga_write(period);
    vga_write("] [");
    
    vga_write(subsystem);
    vga_write("] ");
    vga_write(msg);
    vga_write("\n");
}

void kernel_main() {
    if (read_cmos(CMOS_INIT_FLAG_REG) != CMOS_MAGIC_VAL) {
        // CMOS contains garbage! Clean it up once.
        char clear_pass[11] = {0};
        cmos_write_password(clear_pass);
        
        // Mark it as initialized so it never wipes again
        write_cmos(CMOS_INIT_FLAG_REG, CMOS_MAGIC_VAL);
    }
    vga_clear();
    
    // 1. Initial Identity
    log_verbose("BOOT", "AliOS 4.0 Kernel Initializing...");

    // 2. CPU Identification
    char cpu_name[49];
    get_cpu_name(cpu_name);
    log_verbose("CPU", cpu_name);

    // 3. Hardware Bus Probe
    log_verbose("PCI", "Probing configuration space...");
    pci_scan(); 

    // 4. Subsystem Initialization
    log_verbose("SHELL", "Loading command environment...");
    shell_init(); 

    log_verbose("TODO", "Cleaning garbage data...");
    todo_init();

    log_verbose("TTY", "Mapping virtual terminals...");
    vga_init_ttys();
    
    log_verbose("IDT", "Registering interrupt gates...");
    init_idt();
    
    log_verbose("TIMER", "Calibrating PIT...");
    timer_init();
    
    log_verbose("ATA", "Probing ATA...");
    
    if (ata_probe(0x1F0)) {
    vga_write("DISK FOUND ON PRIM! (0x1F0)\n");
    } else if (ata_probe(0x170)) {
    vga_write("DISK FOUND ON SEC! (0x170)\n");
    } else {
    vga_write("ERROR! NO DISK FOUND!\n");
    }
    log_verbose("TSS", "SETUP TSS...");
    setup_tss((uint64_t)&stack_top);
    log_verbose("TSS", "PATCH TSS...");
    patch_gdt_tss();
    log_verbose("TSS", "LOAD TSS...");
    load_tss();
    log_verbose("HEAP", "INIT HEAP...");
    init_heap();
    log_verbose("KRNL", "Kernel size is...");
    char size_buf[12];
    vga_write(itoa((int)(_kernel_end - _kernel_start), size_buf));
    vga_write(" bytes\n");
    // 5. Final Stage
    log_verbose("SYS", "Initialization sequence complete.");
    vga_clear();
    bootup_screen();
    vga_clear();
    startup_melody();

    while (inb(0x64) & 0x01) { inb(0x60); }

    lock_system_hardened();
    
    vga_write("ScribbleOS 4 - made by a 12yo - Multi-TTY Mode\n");
    vga_write("System Ready. Use Ctrl+Alt+F1-F10 to switch.\n");
    vga_write("Press Enter.");

    int clock_ticks = 0;

    while(1) {
        // FIXED: This ensures 1 loop = 1 hardware tick (10ms)
        timer_wait_tick(); 
        speaker_update();
        clock_ticks++;
        if (clock_ticks >= 100) { 
            vga_draw_status_bar();
            clock_ticks = 0;
        }

        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            tty_t* active = &ttys[current_tty];

            if (scancode == 0x0F) {
                shell_tab_complete(active->command_buffer, &active->buffer_idx);
            } 
            else {
                char c = kbd_get_char(scancode);
                if (c == 0) continue; 

                if (c == '\n') {
                    active->command_buffer[active->buffer_idx] = '\0';
                    shell_dispatch(active->command_buffer);
                    active->buffer_idx = 0;
                    vga_draw_status_bar();
                } 
                else if (c == '\b') {
                    if (active->buffer_idx > 0) {
                        active->buffer_idx--;
                        vga_putchar('\b');
                    }
                } 
                else if (c >= ' ' && active->buffer_idx < 79) {
                    active->command_buffer[active->buffer_idx++] = c;
                    vga_putchar(c);
                }
            }
        }
    }
}
