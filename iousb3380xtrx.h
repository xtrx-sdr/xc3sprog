#ifndef __IO_USB3380XTRX__
#define __IO_USB3380XTRX__

#ifdef USE_LIBUSB3380
#include "iobase.h"
#include <libusb3380.h>

class IOUSB3380XTRX : public IOBase
{
 public:
  IOUSB3380XTRX();
  int Init(struct cable_t *cable, char const *serial, unsigned int freq);
  virtual ~IOUSB3380XTRX();

 protected:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi, bool dotdo);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);

  libusb3380_context_t* ctx;
  unsigned gpio_state;
  unsigned gpio_state_data_prev;
};

#endif //DUSE_LIBUSB3380

#endif
