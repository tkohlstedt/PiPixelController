#ifndef __PIXELCONTROLLER_H__
#define __PIXELCONTROLLER_H__

#include "gpio.h"
#include "rpihw.h"
#include "spi_util.h"

#define MAX_UNIVERSES 15
#define UNIVERSE_SIZE 512
#define START_UNIVERSE 200
#define PIXEL_BUFFER_SIZE MAX_UNIVERSES * UNIVERSE_SIZE
#define MAX_WORKERS 10
#define MAX_CHANNELS_PER_DEVICE 3600

typedef struct __string_ctrl
{
   int channel_count[8];
   int start_channel[8];
   spi_device * spi_dev;
} string_ctrl;

typedef struct __thread_ctrl
{
   char *buffer;
   string_ctrl led_string[2];
} thread_ctrl;


#endif