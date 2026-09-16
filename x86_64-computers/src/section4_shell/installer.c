#include "../section8_global-header/global.h"
#include "grub_payload.h"


void cmd_install_os() {
    vga_set_color(0x0B); // Cyan
    vga_write("\n[ ScribbleOS 4.0 Self-Hosting Installer ]\n");
    vga_set_color(0x0F); // White

    // --- 1. BUILD SYSTEM CHECK ---
    if (scribbleos4_bin_len <= 1) {
        vga_set_color(0x0C); // Red
        vga_write("CRITICAL ERROR: Kernel payload is empty (1 byte)!\n");
        vga_write("Fix: Run 'make clean' and then 'make' twice.\n");
        return;
    }
    // --- 2. WIPE DISK --- 
    cmd_disk_wipe();
    // --- 3. INSTALL GRUB MBR (SECTOR 0) ---
    vga_write("Step 1: Installing GRUB MBR... ");
    uint8_t sector_0[512];
    
    // FIX: Wipe the buffer completely clean first to remove real hardware garbage
    for (int i = 0; i < 512; i++) {
        sector_0[i] = 0x00;
    }

    // FIX: Copy all 512 bytes of GRUB's boot.img instead of truncating at 440
    // This includes GRUB's internal jump offsets and safe empty structures
    for (int i = 0; i < 512; i++) {
        sector_0[i] = boot_img[i];
    }

    // FIX: Inject a guaranteed active partition entry so the physical BIOS doesn't skip it
    sector_0[446] = 0x80; // Bootable flag (Active)
    sector_0[447] = 0x01; // Starting Head
    sector_0[448] = 0x01; // Starting Sector
    sector_0[449] = 0x00; // Starting Cylinder
    sector_0[450] = 0x83; // Linux/Raw filesystem type
    sector_0[451] = 0xFE; // Ending Head
    sector_0[452] = 0xFF; // Ending Sector
    sector_0[453] = 0xFF; // Ending Cylinder
    
    // Starting LBA Sector (Sector 2048, where ScribbleOS lives)
    sector_0[454] = 0x00; 
    sector_0[455] = 0x08; 
    sector_0[456] = 0x00; 
    sector_0[457] = 0x00;

    // Force the standard boot signature explicitly just in case
    sector_0[510] = 0x55;
    sector_0[511] = 0xAA;

    ide_write_sector_bytes(0, sector_0);
    vga_write("DONE\n");

    // --- 4. INSTALL GRUB CORE (SECTOR 1+) ---
    vga_write("Step 2: Writing GRUB Core... ");
    uint32_t core_sectors = (grub_core_img_len + 511) / 512;
    for (uint32_t i = 0; i < core_sectors; i++) {
        ide_write_sector_bytes(1 + i, &grub_core_img[i * 512]);
    }
    vga_write("DONE\n");

    // --- 5. INSTALL AERO1EOS KERNEL (SECTOR 2048) ---
    vga_write("Step 3: Deploying Kernel to Sector 2048... ");
    uint32_t k_sectors = (scribbleos4_bin_len + 511) / 512;

    for (uint32_t i = 0; i < k_sectors; i++) {
        ide_write_sector_bytes(2048 + i, &scribbleos4_bin[i * 512]);
        if (i % 25 == 0) vga_putchar('.');
    }
    vga_write(" DONE\n");

    // --- 6. SUCCESS SUMMARY ---
    vga_set_color(0x0A); // Green
    vga_write("\nSUCCESS! ScribbleOS 4.0 is now on (hd0).\n");
    vga_set_color(0x0F); // White
    
    vga_write("Reboot and type this into GRUB:\n");
    vga_set_color(0x0E); // Yellow
    vga_write("multiboot2 (hd0)2048+");
    
    char count_str[16];
    itoa(k_sectors, count_str);
    vga_write(count_str);
    
    vga_write("\nboot\n");
    vga_set_color(0x1E); 
}
