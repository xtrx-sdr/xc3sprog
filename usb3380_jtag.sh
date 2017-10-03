#!/bin/sh

mkdir -p xtrx_mcu_build
( cd xtrx_mcu_build; \
  sdcc -mmcs51 --iram-size 256 --xram-size 0 --code-size 8192 --model-small --opt-code-size --fomit-frame-pointer ../usb3380_jtag.c -o usb3380_jtag_prog.ihex; \
  objcopy -I ihex -O binary usb3380_jtag_prog.ihex usb3380_jtag_prog.bin; \
  xxd -i usb3380_jtag_prog.bin ../usb3380_jtag_prog.h; )
