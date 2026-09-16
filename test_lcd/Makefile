# test_lcd 子目录 Makefile
CC      ?= arm-linux-gnueabihf-gcc
TARGETS := fb_info lcd_test bmp_show

.PHONY: all clean

all: $(TARGETS)

%: %.c
	$(CC) $< -o $@

clean:
	rm -f $(TARGETS)
