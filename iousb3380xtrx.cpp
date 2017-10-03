#ifdef USE_LIBUSB3380
#include "iousb3380xtrx.h"

#include <unistd.h>
#include <stdio.h>

#include "usb3380_jtag_prog.h"

// XTRX - USB3380
//
// GPIO0-TCK
// GPIO1-TDO
// GPIO2-TDI
// GPIO3-TMS

enum {
	TCK = 1<<0,
	TDO = 1<<1,
	TDI = 1<<2,
	TMS = 1<<3,
};

IOUSB3380XTRX::IOUSB3380XTRX()
 : ctx(NULL), gpio_state(TDI | TMS), gpio_state_data_prev(~0)
{
    usb3380_set_loglevel(USB3380_LOG_INFO);
}

enum {
    GP_NUM = 0x20,
};

int IOUSB3380XTRX::Init(struct cable_t *cable, char const *serial, unsigned int freq)
{
	int res;
	res = usb3380_context_init(&ctx);
	if (res)
		return res;

    res = usb3380_mcu_reset(ctx, 1);
    if (res) {
        fprintf(stderr, "UNABLE TO RESET MCU\n");
        return res;
    }

    res = usb3380_csr_mm_cfg_write(ctx, 0x300 + 0x04 + GP_NUM, (1<<2) | (1<<1));
    if (res)
        return res;

	res = usb3380_gpio_dir(ctx, TCK | TMS | TDI);
    if (res)
        return res;

    res = usb3380_csr_mm_mcu_copy(ctx,
                                  0,
                                  (const uint32_t*)usb3380_jtag_prog_bin,
                                  (usb3380_jtag_prog_bin_len + 3) / 4);
    if (res) {
        fprintf(stderr, "UNABLE TO UPLOAD MCU PROGRAMM\n");
        return res;
    }

    res = usb3380_mcu_reset(ctx, 0);
    if (res)
        return res;

	return res;
}

IOUSB3380XTRX::~IOUSB3380XTRX()
{
    usb3380_mcu_reset(ctx, 1);
	usb3380_context_free(ctx);
}

void IOUSB3380XTRX::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{
    //fprintf(stderr, "TXRX len=%d L=%d O=%d\n", length, last, (tdo) ? 1 : 0);
    if ((length > 32) && (length % 32 == 0) && (tdo == NULL)) {
        // Hardware acceleration
        int written, res;
        int p = 0;
        int rem = length;

        do {
            p = rem / 8;
            if (p > 4096)
                p = 4096;

            res = usb3380_gpep_write_no(ctx, 0x2, tdi + (length - rem) / 8, p, &written, 5000);
            if (res) {
                fprintf(stderr, "UNABLE TO STREAM TDI BLOCK len=%d bits\n", length);
                return;
            }

            rem -= p * 8;

            fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                    "Loading %02.1f%%", 100.0*(length-rem)/length);
        } while (rem > 0);

        uint32_t a;
        do {
            sleep(1);

            res = usb3380_csr_mm_cfg_read(ctx, 0x300 + 0x10 + GP_NUM, &a);
            if (res)
                return;

        } while (a != 0);
        fprintf(stderr, " of %d bits\n", length);
    } else {
        int i=0;
        int j=0;
        unsigned char tdo_byte=0;
        unsigned char tdi_byte;
        if (tdi)
            tdi_byte = tdi[j];

        while(i<length-1){
            tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1, tdo ? true : false)<<(i%8));
            if (tdi)
                tdi_byte=tdi_byte>>1;
            i++;
            if((i%8)==0){ // Next byte
                if(tdo)
                    tdo[j]=tdo_byte; // Save the TDO byte
                tdo_byte=0;
                j++;
                if (tdi)
                    tdi_byte=tdi[j]; // Get the next TDI byte
            }
        };
        tdo_byte=tdo_byte+(txrx(last, (tdi_byte&1)==1, tdo ? true : false)<<(i%8));
        if(tdo)
            tdo[j]=tdo_byte;

        gpio_state &= ~TCK;
        usb3380_gpio_out(ctx, gpio_state);
    }
}

void IOUSB3380XTRX::tx_tms(unsigned char *pat, int length, int force)
{
	int i;
	unsigned char tms;
	for (i = 0; i < length; i++)
	{
		if ((i & 0x7) == 0)
			tms = pat[i>>3];
		tx((tms & 0x01), true);
		tms = tms >> 1;
	}

	gpio_state &= ~TCK;
	usb3380_gpio_out(ctx, gpio_state);
}

void IOUSB3380XTRX::tx(bool tms, bool tdi)
{
	gpio_state &= ~TCK;
	usb3380_gpio_out(ctx, gpio_state);

	if(tdi)
		gpio_state |= TDI;
	else
		gpio_state &= ~TDI;

	if(tms)
		gpio_state |= TMS;
	else
		gpio_state &= ~TMS;

	if (gpio_state_data_prev != gpio_state) {
		usb3380_gpio_out(ctx, gpio_state);
		gpio_state_data_prev = gpio_state;
	}

	gpio_state |= TCK;
	usb3380_gpio_out(ctx, gpio_state);
}


bool IOUSB3380XTRX::txrx(bool tms, bool tdi, bool dotdo)
{
	tx(tms, tdi);
	if (dotdo) {
		uint8_t in;
		usb3380_gpio_in(ctx, &in);

		return ((in & TDO) ? true : false);
	}
	return false;
}

#endif //USE_LIBUSB3380
