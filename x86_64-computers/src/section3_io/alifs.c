#include "../section8_global-header/global.h"
#include "alifs.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
// Static buffer to hold the Inode Table during operations
static uint8_t inode_sector[512] __attribute__((aligned(8)));
// Static buffer for reading file content
static char file_content_buffer[512] __attribute__((aligned(8)));

char *strrchr(const char *str, int character) {
    char *last = NULL;
    while (*str != '\0') {
        if (*str == (char)character) {
            last = (char *)str;
        }
        str++;
    }
    if ((char)character == '\0') {
        return (char *)str;
    }
    return last;
}

char *strchr(const char *str, int character) {
    while (*str != '\0') {
        if (*str == (char)character) {
            return (char *)str;
        }
        str++;
    }

    if ((char)character == '\0') {
        return (char *)str;
    }

    return NULL;
}

void alifs_format() {
    vga_write("Formatting AliFS...\n");
    for (int i = 0; i < 512; i++) inode_sector[i] = 0;
    
    // Write empty Inode Table to LBA 20001 (Table Location)
    ide_write_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    vga_write("AliFS Initialized at LBA 20000.\n");
}


int alifs_create(char* name, char* data) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    char full_path[FILENAME_LEN];
    // Force absolute paths: "/filename" or "/dir/filename"
    if (strcmp(current_path, "/") == 0) {
        full_path[0] = '/';
        strcpy(full_path + 1, name);
    } else {
        strcpy(full_path, current_path);
        int len = strlen(full_path);
        full_path[len] = '/';
        strcpy(full_path + len + 1, name);
    }

    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        // STRICT: Only match if the full path is identical
        if (inodes[i].active && strcmp(inodes[i].filename, full_path) == 0) {
            slot = i; break;
        }
        if (slot == -1 && !inodes[i].active) slot = i;
    }

    if (slot == -1) return -1;

    strcpy(inodes[slot].filename, full_path);
    inodes[slot].start_lba = ALIFS_START_LBA + 2 + slot;
    inodes[slot].size = strlen(data);
    inodes[slot].active = 1;
    inodes[slot].is_dir = 0;

    uint8_t write_bounce[512] = {0};
    int data_len = strlen(data) > 511 ? 511 : strlen(data);
    for (int b = 0; b < data_len; b++) write_bounce[b] = (uint8_t)data[b];
    
    ide_write_sector_bytes(inodes[slot].start_lba, write_bounce);
    ide_write_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    return 0;
}


void alifs_list() {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    vga_write("\nListing: "); vga_write(current_path); vga_write("\n");
    
    int path_len = strlen(current_path);
    int count = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (!inodes[i].active) continue;

        // Skip the current directory itself so we only list contents
        if (strcmp(inodes[i].filename, current_path) == 0) continue;

        // Check if it's a direct child
        // If current_path is "/", just check if it starts with "/"
        // Otherwise check if it starts with "current_path/"
        int is_child = 0;
        if (strcmp(current_path, "/") == 0) {
            if (inodes[i].filename[0] == '/' && strchr(inodes[i].filename + 1, '/') == NULL) is_child = 1;
        } else {
            if (strncmp(inodes[i].filename, current_path, path_len) == 0 && 
                inodes[i].filename[path_len] == '/' && 
                strchr(inodes[i].filename + path_len + 1, '/') == NULL) is_child = 1;
        }

        if (is_child) {
            vga_write(inodes[i].is_dir ? "<DIR> " : "      ");
            vga_write(strrchr(inodes[i].filename, '/') + 1); // Print only the name
            vga_write("\n");
            count++;
        }
    }
    if (count == 0) vga_write("(Empty)\n");
}

char* alifs_read(char* name) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    char full_path[FILENAME_LEN];
    if (strcmp(current_path, "/") == 0) {
        full_path[0] = '/';
        strcpy(full_path + 1, name);
    } else {
        strcpy(full_path, current_path);
        int len = strlen(full_path);
        full_path[len] = '/';
        strcpy(full_path + len + 1, name);
    }

    for (int i = 0; i < MAX_FILES; i++) {
        // Only allow match if the full path is exactly correct
        if (inodes[i].active && strcmp(inodes[i].filename, full_path) == 0) {
            ide_read_sector_bytes(inodes[i].start_lba, (uint8_t*)file_content_buffer);
            return file_content_buffer;
        }
    }
    return 0;
}

int alifs_mkdir(char* name) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    // 1. Build full path
    char full_name[FILENAME_LEN];
    if (strcmp(current_path, "/") == 0) {
        full_name[0] = '/';
        strcpy(full_name + 1, name);
    } else {
        strcpy(full_name, current_path);
        int len = strlen(full_name);
        full_name[len] = '/';
        strcpy(full_name + len + 1, name);
    }

    int slot = -1;

    // 2. Scan the table using FULL_NAME
    for (int i = 0; i < MAX_FILES; i++) {
        // USE full_name HERE
        if (inodes[i].active && strcmp(inodes[i].filename, full_name) == 0) {
            vga_write("Error: Directory or file already exists.\n");
            return -1;
        }

        if (slot == -1 && !inodes[i].active) {
            slot = i;
        }
    }

    if (slot == -1) {
        vga_write("Error: Inode table full. Cannot create directory.\n");
        return -1;
    }

    // 3. Fill metadata using FULL_NAME
    strcpy(inodes[slot].filename, full_name); // USE full_name HERE
    inodes[slot].start_lba = 0;
    inodes[slot].size = 0;
    inodes[slot].active = 1;
    inodes[slot].is_dir = 1;

    ide_write_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    vga_write("Directory created successfully.\n");
    return 0;
}

int alifs_is_directory(char* name) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;
    for (int i = 0; i < MAX_FILES; i++) {
        if (inodes[i].active && strcmp(inodes[i].filename, name) == 0) {
            return inodes[i].is_dir;
        }
    }
    return 0;
}
void alifs_read_into_buffer(char* name, uint8_t* target) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    for (int i = 0; i < MAX_FILES; i++) {
        if (inodes[i].active && strcmp(inodes[i].filename, name) == 0) {
            ide_read_sector_bytes(inodes[i].start_lba, target);
            return;
        }
    }
}

int alifs_delete_recursive(char* path) {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    alifs_inode_t* inodes = (alifs_inode_t*)inode_sector;

    int path_len = strlen(path);
    int deleted_count = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (!inodes[i].active) continue;

        // 1. Is it the exact path we want to delete?
        int is_match = (strcmp(inodes[i].filename, path) == 0);
        
        // 2. Is it a child? (Starts with path + '/')
        // We ensure path_len is safe by checking if it's not the root
        int is_child = 0;
        if (path_len > 1) { // Assuming path isn't just "/"
            is_child = (strncmp(inodes[i].filename, path, path_len) == 0 && 
                        inodes[i].filename[path_len] == '/');
        }

        // If it's the folder OR a child, wipe it
        if (is_match || is_child) {
            inodes[i].active = 0;
            // Clear metadata
            for(int j = 0; j < FILENAME_LEN; j++) inodes[i].filename[j] = 0;
            deleted_count++;
        }
    }

    if (deleted_count == 0) {
        vga_write("Error: Path not found.\n");
        return -1;
    }

    ide_write_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
    vga_write("Deleted successfully.\n");
    return 0;
}
void alifs_init() {
    ide_read_sector_bytes(ALIFS_START_LBA + 1, inode_sector);
}
