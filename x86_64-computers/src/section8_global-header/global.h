#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "../section3_io/alifs.h"
#include "../section4_shell/quran.h"
#include "../section4_shell/frames.h"

// Port I/O helper prototypes
// 8-bit
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 16-bit
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 32-bit
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ __volatile__ ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


// Extracted Typedefs (command_func, etc.)
typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;
typedef unsigned int   uint32_t;  // In 32-bit/64-bit GCC, 'int' is 32 bits
typedef unsigned short uint16_t;
typedef void (*command_func)(char*);
typedef unsigned long size_t;
// Extracted Macros
#define ALIFS_H
#define ALIFS_START_LBA 20000  // Safe area on your disk
#define ALIGNMENT 8
#define ALISCR_H
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY   0xEC
#define ATA_CMD_READ       0x20
#define ATA_CMD_WRITE      0x30
#define ATA_REG_COMMAND    (ata_base_port + 7)
#define ATA_REG_DATA       (ata_base_port + 0)
#define ATA_REG_DRV_SEL    (ata_base_port + 6)
#define ATA_REG_FEATURES   (ata_base_port + 1)
#define ATA_REG_LBA_HIGH   (ata_base_port + 5)
#define ATA_REG_LBA_LOW    (ata_base_port + 3)
#define ATA_REG_LBA_MID    (ata_base_port + 4)
#define ATA_REG_SEC_COUNT  (ata_base_port + 2)
#define ATA_REG_STATUS     (ata_base_port + 7)
#define ATA_STATUS_BSY     0x80
#define ATA_STATUS_DF      0x20
#define ATA_STATUS_DRQ     0x08
#define ATA_STATUS_ERR     0x01
#define ATA_TIMEOUT        10000000
#define ATTR_ACCENT 0x1B       // Cyan text on Blue background
#define ATTR_DEFAULT 0x1E      // Yellow text on Blue background
#define ATTR_HEADER 0x1F       // White text on Blue background
#define ATTR_SELECTED 0x70     // Inverted: Black text on Light Gray
#define CMATRIX_COLS 80
#define CMATRIX_ROWS 24 // Leave row 24 safe for status bar!
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71
#define CMOS_INIT_FLAG_REG  0x32  // Flag
#define CMOS_MAGIC_VAL      0xA5  // A distinct byte to signal "Initialized"
#define DVD_COLS 80
#define DVD_ROWS 24 // Leave row 24 safe for the status bar clock
#define FILENAME_LEN 32
#define FRAMES_H
#define HEAP_H
#define HEIGHT 25
#define IO_H
#define LINE_SIZE 64
#define MAX_FILES 12
#define MAX_HISTORY 10
#define MAX_LINES 100
#define MAX_SNAKE_LEN 100
#define MAX_TTYS 10
#define NOTEBOOK_YELLOW 0x1E
#define PASSWORD_CMOS_BASE 0x40
#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_H
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define QR_SIZE 21
#define QURAN_H
#define SHELL_H
#define SYS_EXIT  4
#define SYS_OPEN  2
#define SYS_READ  3
#define SYS_WRITE 1
#define TUI_VIDEO_H
#define VGA_ADDRESS 0xB8000
#define VGA_HEIGHT 25
#define VGA_MEM_ADDR 0xB8000
#define VGA_WIDTH 80
#define VIDEO_SIZE (WIDTH * HEIGHT * 2)
#define WIDTH 80
#define _POSIX_SYSCALL_H

// Extracted Struct & Type Definitions
typedef struct {
    int freq;
    int duration_ms; 
} Note;

typedef struct {
    int surah;
    int ayah;
    const char* text;
} ayah_t;

typedef struct {
    const char* book;
    int chapter;
    int verse;
    const char* text;
} bible_t;

typedef struct block_header {
    size_t size;            // Size of the data block (excluding header)
    int is_free;            // 1 if the block is available for reuse, 0 if used
    struct block_header* next; // Pointer to the next block in the linked list
} block_header_t;

typedef struct command_node {
    char name[32];
    char description[64];
    command_func function;
    struct command_node* next;
} command_node_t;

typedef struct {
    char key[16];
    char value[32];
    int active;
} env_var_t;

typedef struct {
    int active;
    char path[256];
} file_descriptor_t;

typedef struct {
    const char* ar;
    const char* en;
    const char* meaning;
} name_99_t;

typedef struct {
    char task[48];
    int done;
    int active;
} todo_t;

typedef struct {
    unsigned short* buffer;
    int cursor_pos;
    char command_buffer[80];
    int buffer_idx;
    void (*background_task)(int);
} tty_t;


typedef struct {
    // 1. Pushed by isr_syscall:
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, rbp;
    
    // 2. Pushed by the CPU hardware on 'int 0x80':
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IdtPointer;

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed)) tss_t;

typedef struct {
    uint64_t cr3;
    uint64_t cr2;
    uint64_t rax; uint64_t rbx; uint64_t rcx; uint64_t rdx;
    uint64_t rsi; uint64_t rdi; uint64_t rbp;
    uint64_t r8;  uint64_t r9;  uint64_t r10; uint64_t r11;
    uint64_t r12; uint64_t r13; uint64_t r14; uint64_t r15;
    
    // Tracked vectors
    uint64_t exception_vector;
    uint64_t hardware_error_code;
    uint64_t rip; // Address where execution broke
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) CpuPanicState;

// Extracted Global Declarations
extern void alifs_init();
extern int alifs_delete_recursive(char* path);
extern void alifs_read_into_buffer();
extern int atoi_custom(char* s); // Ensure this is in your string/lib code
extern void cmd_ayah();
extern void cmd_disk_wipe();
extern void cmd_install_os();
extern void cmd_neofetch(char* args);
extern void cmd_start_gui();
extern int cmos_get_day();
extern int cmos_get_hour();
extern int cmos_get_min();
extern int cmos_get_month();
extern int cmos_get_sec();
extern void cmos_read_password(char* dest_buffer);
extern void cmos_write_password(const char* encrypted_pass);
extern void draw_custom_plane();
extern char _kernel_end;
extern char current_path[256];
extern int current_tty;
extern int status_bar_enabled;
extern int timezone_offset_seconds;
extern todo_t my_list[10];
extern tty_t ttys[];
extern uint16_t tss_base_low;
extern uint32_t tss_base_upper;
extern uint64_t exception_table[32];
extern uint64_t stack_top;
extern uint8_t  tss_base_high;
extern uint8_t  tss_base_mid;
extern uint8_t _kernel_start;
extern unsigned char current_vga_color;
extern unsigned long long timer_ticks;
extern volatile int is_sleeping;
extern unsigned char get_failed_attempts();
extern unsigned long long get_total_ram_bytes();
extern unsigned int get_uptime_ms();
extern unsigned int get_uptime_seconds();
extern void heap_init();
extern uint32_t ide_get_total_sectors();
extern int ide_read_sector_bytes(uint32_t lba, uint8_t* buffer);
extern int ide_write_sector_bytes(uint32_t lba, uint8_t* buffer);
extern void init_idt(void);
extern void isr_syscall(void);
extern char* itoa(int val, char* s);
extern char kbd_get_char(unsigned char scancode);
extern void load_tss();
extern void lock_system_hardened();
extern void nosound();
extern void patch_gdt_tss();
extern void play_sound(unsigned int nFrequence);
extern void print_to_tty(const char* str, int tty_idx);
extern unsigned char read_cmos(unsigned char reg);
extern void set_failed_attempts(unsigned char count);
extern void setup_tss(uint64_t kernel_stack);
extern void shell_lock();
extern void sleep();
extern void sleep_ms(int ms);
extern void speaker_update();
extern void startup_melody();
extern int strcmp(const char *s1, const char *s2);
extern char* strcpy(char *dest, const char *src);
extern int strncmp(const char *s1, const char *s2, unsigned long n);
extern char *strrchr(const char *str, int character);
extern void switch_tty(int n);
extern void timer_init();
extern void timer_wait_tick();
extern void todo_show();
extern void trigger_ali_morse();
extern void update_hardware_cursor(int pos);
extern void update_hardware_speaker(); // Volume tuner activator
extern void vga_clear();
extern void vga_draw_status_bar();
extern void vga_init_ttys();
extern void vga_putchar(char c);
extern void vga_set_attribute(unsigned char attribute);
extern void vga_set_color(unsigned char color);
extern void vga_set_cursor();
extern void vga_write(const char* data);
extern char wait_for_key();
extern void write_cmos(unsigned char reg, unsigned char val);
extern void* kmalloc();
extern void tui_clear(uint8_t attribute);
extern void tui_print_at(int x, int y, const char* str, uint8_t attribute);
extern void tui_draw_box(int x, int y, int width, int height, const char* title, uint8_t attribute);
extern void init_heap();
extern void pci_scan();
extern void shell_tab_complete();
extern void shell_dispatch();
extern void shell_init();
extern void get_cpu_name();
extern void cmd_run_script();
extern size_t get_heap_usage();
extern void get_cpu_name(char* name);
