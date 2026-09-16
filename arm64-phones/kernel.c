#include <stdint.h>

extern void fb_clear(uint32_t color);
extern void fb_puts(const char* str, uint32_t fg, uint32_t bg);
extern void shell_init(void);
extern void shell_handle_keypress(char key);

// Custom delaying loop utility function to emulate typing paces
void sleep_ticks(volatile int count) {
    while(count--) {
        asm volatile("nop");
    }
}

void kernel_main() {
    fb_clear(0x000033); // Set background canvas color

    fb_puts("ScribbleOS Core Operating System Boot Sequence\n", 0x00FF00, 0x000033);
    fb_puts("Loading Core Modules... Done.\n", 0xFFFFFF, 0x000033);
    fb_puts("ScribbleOS > ", 0xFFFF00, 0x000033);

    shell_init();

    // Simulated Execution Run: Let's automatically test out your new shell configurations!
    sleep_ticks(20000000);
    shell_handle_keypress('h');
    sleep_ticks(5000000);
    shell_handle_keypress('e');
    sleep_ticks(5000000);
    shell_handle_keypress('l');
    sleep_ticks(5000000);
    shell_handle_keypress('p');
    sleep_ticks(10000000);
    shell_handle_keypress('\n'); // Submit "help" command string

    // Run custom text retrieval checks automatically
    sleep_ticks(20000000);
    shell_handle_keypress('p');
    shell_handle_keypress('l');
    shell_handle_keypress('a');
    shell_handle_keypress('n');
    shell_handle_keypress('e');
    shell_handle_keypress('\n');

    // Idle state loop holding system execution safely
    while (1) {
        asm volatile("wfi");
    }
}

