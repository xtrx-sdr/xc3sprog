// internal 8051 uC in usb3380

#ifdef __SDCC
#define SFR(X, p)   __sfr  __at (p) X
#else
#error define SFR for your compiler!
#endif

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;

// USB3380 specific
SFR(IRQ0A, 0xE8);

SFR(B, 0xF0);
SFR(CFGCTL0, 0xF2);
SFR(IFSTAT, 0xF3);
SFR(CFGADDR0, 0xF4);
SFR(CFGADDR1, 0xF5);

SFR(IRQ0B, 0xF8);
SFR(CFGDATA0, 0xF9);
SFR(CFGDATA1, 0xFA);
SFR(CFGDATA2, 0xFB);
SFR(CFGDATA3, 0xFC);

enum {
    CSR_START = (1<<6),
    CSR_MM_CFG = (1<<4),

    CSR_WRITE = (0<<7),
    CSR_READ = (1<<7),
};

static void CsrWait(void)
{
    while ((CFGCTL0 & CSR_START) == CSR_START);
}

static void CsrWrite16(uint16_t addr, uint16_t data)
{
    uint8_t* p = (uint8_t*)&data;

    CFGADDR0 = addr;
    CFGADDR1 = (addr>>8);

    CFGDATA0 = data;
    CFGDATA1 = (data>>8);

    CFGCTL0 = CSR_WRITE | CSR_START | CSR_MM_CFG | 0x3;
    CsrWait();
}

static void CsrRead32Req(uint16_t addr)
{
    CFGADDR0 = addr;
    CFGADDR1 = (addr>>8);

    CFGCTL0 = CSR_READ | CSR_START | CSR_MM_CFG | 0xf;
    CsrWait();
}

static uint16_t CsrWaitRead16(uint16_t addr)
{
    uint16_t data;

    CFGADDR0 = addr;
    CFGADDR1 = (addr>>8);

    CFGCTL0 = CSR_READ | CSR_START | CSR_MM_CFG | 0x3;
    CsrWait();

    data = CFGDATA0;
    data |= (CFGDATA1) << 8;

    return data;
}

enum {
	TCK = 1<<0,
	TDO = 1<<1,
	TDI = 1<<2,
	TMS = 1<<3,
};

enum {
	GPIO_DIRS = (TCK | TMS | TDI) << 4,
};

// ep FIFO registers (GPEP0 OUT)
enum {
	GP_NUM = 0x20,
};
enum {
	G_EP_RSP = 0x300 + 0x04 + GP_NUM,
	G_EP_STAT = 0x300 + 0x0C + GP_NUM,
	G_EP_AVAIL = 0x300 + 0x10 + GP_NUM,
	G_EP_DATA = 0x300 + 0x14 + GP_NUM,
	G_EP_DATA1 = 0x300 + 0x18 + GP_NUM,
};

enum {
	FIFO_EMPTY = 1<<10,
	FIFO_FULL = 1<<11,
};

#define CSR_WAIT()

static void jtag_tdi_byteout(uint8_t byte)
{
    uint8_t i;
    uint8_t b = byte;

    for (i = 0; i < 8; i++, b >>= 1) {
        CFGDATA0 &= ~TCK;
        CFGCTL0 |= CSR_START;
        CSR_WAIT();

        if (b & 1) {
            CFGDATA0 |= TDI;
        } else {
            CFGDATA0 &= ~TDI;
        }
        CFGCTL0 |= CSR_START;
        CSR_WAIT();

        CFGDATA0 |= TCK;
        CFGCTL0 |= CSR_START;
        CSR_WAIT();
    }
}


void main()
{
    for (;;) {
        uint16_t ep_stat;
        uint16_t avail;

        ep_stat = CsrWaitRead16(G_EP_STAT);
        if ((ep_stat & (FIFO_EMPTY)) == FIFO_EMPTY) {
            volatile uint16_t v;
            for (v = 65535; v != 0; v--);
            continue;
        }
        CsrWrite16(G_EP_STAT, ep_stat);

        avail = CsrWaitRead16(G_EP_AVAIL);
        for (; avail >= 4; avail -= 4) {
            uint8_t ep_data0, ep_data1, ep_data2, ep_data3;

            CsrRead32Req(G_EP_DATA);
            ep_data0 = CFGDATA0;
            ep_data1 = CFGDATA1;
            ep_data2 = CFGDATA2;
            ep_data3 = CFGDATA3;

            CFGADDR0 = 0x50;
            CFGADDR1 = 0;
            CFGCTL0 = CSR_WRITE | CSR_MM_CFG | 0x1;
            CFGDATA0 = GPIO_DIRS;

            jtag_tdi_byteout(ep_data0);
            jtag_tdi_byteout(ep_data1);
            jtag_tdi_byteout(ep_data2);
            jtag_tdi_byteout(ep_data3);
        }
    }
}
