#ifndef __SK6812_I2S_H__
#define __SK6812_I2S_H__

#include <stdint.h>
#include "SK6812_defs.h"

// include C-style header
extern "C"
{
#include "SK6812_dma.h"
};

typedef struct
{
  uint8_t G; // G,R,B order is determined by SK6812B
  uint8_t R;
  uint8_t B;
  uint8_t W;
} pixel;


class SK6812
{
  public:
    SK6812(void);
    ~SK6812(void);
    void init(uint16_t num_leds);
    void show(pixel *);
    void setBrightness(uint8_t brt);

  private:
    uint16_t num_leds;
    uint8_t brt;
    uint32_t *i2s_pixels_buffer[SK6812_DITHER_NUM];
    uint32_t i2s_zeros_buffer[NUM_I2S_ZERO_WORDS];
    sdio_queue_t i2s_zeros_queue[SK6812_DITHER_NUM];
    sdio_queue_t i2s_pixels_queue[SK6812_DITHER_NUM];
};

#endif // __SK6812_I2S_H__
