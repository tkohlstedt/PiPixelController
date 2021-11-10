#ifndef __PIXELCONTROLLER_H__
#define __PIXELCONTROLLER_H__

#include "gpio.h"
#include "rpihw.h"
#include "spi_util.h"

#define MAX_UNIVERSES 48
#define UNIVERSE_SIZE 512
#define START_UNIVERSE 200
#define PIXEL_BUFFER_SIZE MAX_UNIVERSES * UNIVERSE_SIZE
#define MAX_WORKERS 10
#define MAX_CHANNELS_PER_DEVICE 3600
#define CONTROLLER_NAME "PiPixelController\0"
#define PIXEL_PORTS 4

typedef struct __string_ctrl
{
   int channel_count[8];
   int start_channel[8];
   spi_device * spi_dev;
} string_ctrl;

typedef struct __thread_ctrl
{
   int pixel_ports;
   char *buffer;
   string_ctrl led_string[4];
} thread_ctrl;


#endif