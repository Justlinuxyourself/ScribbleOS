# --- ScribbleOS 4 Master Build System ---
SRCDIR = src
OBJDIR = obj
BIN = scribbleos4.bin
ISO = scribbleos4.iso
PAYLOAD_H = $(SRCDIR)/section4_shell/grub_payload.h
CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld
C_SOURCES = $(shell find $(SRCDIR) -name '*.c')
ASM_SOURCES = $(shell find $(SRCDIR) -name '*.asm')

OBJ = $(C_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJ += $(ASM_SOURCES:$(SRCDIR)/%.asm=$(OBJDIR)/%.o)

CFLAGS = -m64 -c -ffreestanding -fno-stack-protector -Iinclude -Wall -Wextra -w
LDFLAGS = -n -T linker.ld --build-id=none -z max-page-size=0x1000 --no-warn-rwx-segments

all: $(ISO)

# --- PASS 1: Generate a dummy header so the first compilation doesn't fail ---
$(PAYLOAD_H):
	@echo "--- CREATING BOOTSTRAP PAYLOAD ---"
	@mkdir -p $(SRCDIR)/section4_shell
	@echo "unsigned char scribbleos4_bin[] = {0x00};" > $(PAYLOAD_H)
	@echo "unsigned int scribbleos4_bin_len = 1;" >> $(PAYLOAD_H)
	@echo "unsigned char boot_img[] = {0x00};" >> $(PAYLOAD_H)
	@echo "unsigned int boot_img_len = 1;" >> $(PAYLOAD_H)
	@echo "unsigned char grub_core_img[] = {0x00};" >> $(PAYLOAD_H)
	@echo "unsigned int grub_core_img_len = 1;" >> $(PAYLOAD_H)

# --- PASS 2: Link the binary ---
$(BIN): $(PAYLOAD_H) $(OBJ)
	@echo "--- LINKING AERO1EOS BINARY ---"
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)

# --- PASS 3: Replace the dummy header with the REAL data ---
# This rule is called manually or via a special target to avoid the circular loop
update_payload: $(BIN)
	@echo "--- EMBEDDING REAL KERNEL INTO PAYLOAD ---"
	grub-mkimage -o grub_core.img -O i386-pc -c early_grub.cfg -p /boot/grub biosdisk part_msdos fat normal multiboot multiboot2
	cp /usr/lib/grub/i386-pc/boot.img .
	@echo "/* Real ScribbleOS Payload */" > $(PAYLOAD_H)
	xxd -i boot.img >> $(PAYLOAD_H)
	xxd -i grub_core.img >> $(PAYLOAD_H)
	xxd -i $(BIN) >> $(PAYLOAD_H)
	@rm boot.img grub_core.img
	@# Recompile only the installer with the real data
	$(CC) $(CFLAGS) $(SRCDIR)/section4_shell/installer.c -o $(OBJDIR)/section4_shell/installer.o
	@# Final Link
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJDIR)/section1_cpu/boot.o $(filter-out $(OBJDIR)/section1_cpu/boot.o, $(OBJ))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.asm
	@mkdir -p $(@D)
	nasm -f elf64 $< -o $@

$(ISO): $(BIN)
	@$(MAKE) update_payload
	@echo "--- CREATING FINAL ISO ---"
	@mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

clean:
	rm -rf $(OBJDIR) $(BIN) $(ISO) isodir $(PAYLOAD_H)

.PHONY: all clean update_payload run _check-qemu _create-disk

run: _check-qemu _create-disk
	@echo "Starting QEMU with sound support..."
	qemu-system-x86_64 -m 64M \
	  -drive file=scribbleos.img,format=raw \
	  -cdrom scribbleos4.iso \
	  -boot d \
	  -audiodev $$(if [ -n "$$PIPEWIRE_REMOTE" ] || [ -S /run/user/$$(id -u)/pipewire-0 ]; then echo "pipewire,id=snd0"; elif pactl info >/dev/null 2>&1; then echo "pa,id=snd0"; else echo "alsa,id=snd0"; fi) \
	  -device intel-hda \
	  -device hda-duplex,audiodev=snd0

_check-qemu:
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 is not installed." && exit 1)

_create-disk:
	@if [ ! -f scribbleos.img ]; then \
		echo "Creating disk image scribbleos.img..."; \
		qemu-img create -f raw scribbleos.img 10G; \
	fi
