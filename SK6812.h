// SK6812.h

#ifndef __SK6812_H__
#define __SK6812_H__

// Temporal Dithering
// Dithering preserves color and light when brightness is low.
// Sometimes this can cause undesirable flickering.
// 1 = Disable temporal dithering
// 2, 6, 8 = Enable temporal dithering (larger values = more dithering)
#define SK6812_DITHER_NUM (8)

#define SK6812_USE_INTERRUPT (0) // not supported yet

#endif

// end of file
