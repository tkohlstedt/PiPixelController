#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "mailbox.h"
#include "PiPixelController.h"
#include "spi_util.h"

#define REFRESH_TIME            50000

typedef struct __control_buffer
{
    uint16_t channel_count[8];
    uint8_t  channel_data[MAX_CHANNELS_PER_DEVICE];
} control_buffer;

control_buffer payload;

/**
 * Provides monotonic timestamp in microseconds.
 *
 * @returns  Current timestamp in microseconds or 0 on error.
 */
static uint64_t get_microsecond_timestamp()
{
    struct timespec t;

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &t) != 0) {
        return 0;
    }

    return (uint64_t) t.tv_sec * 1000000 + t.tv_nsec / 1000;
}



void *worker(void *worker_config)
{
    int ret;
    uint64_t current_timestamp;
    uint64_t previous_timestamp;
    uint64_t time_diff;
    thread_ctrl *config = worker_config;
    int a;
    int b;
 
    memset(&payload,0,sizeof(payload));

    while(1)
    {
        previous_timestamp = get_microsecond_timestamp();

        // process string 0
        if(config->pixel_ports > 0)
        {
            b = 0;
            for(a=0;a<8;a++)
            {
                payload.channel_count[a] = config->led_string[0].channel_count[a];
                if(config->led_string[0].channel_count[a])
                {
                    memcpy(&payload.channel_data[b],&config->buffer[config->led_string[0].start_channel[a]],config->led_string[0].channel_count[a]);
                    b = b + config->led_string[0].channel_count[a];
                }            
            }

            config->led_string[0].spi_dev->buffer = (uint8_t*)&payload;
            config->led_string[0].spi_dev->count = sizeof(payload);
            spi_transfer(config->led_string[0].spi_dev);
        }
        // process string 1
        if(config->pixel_ports > 1)
        {
            b = 0;
            for(a=0;a<8;a++)
            {
                payload.channel_count[a] = config->led_string[1].channel_count[a];
                if(config->led_string[1].channel_count[a])
                {
                    memcpy(&payload.channel_data[b],&config->buffer[config->led_string[1].start_channel[a]],config->led_string[1].channel_count[a]);
                    b = b + config->led_string[1].channel_count[a];
                }            
            }
            config->led_string[1].spi_dev->buffer = (uint8_t*)&payload;
            config->led_string[1].spi_dev->count = sizeof(payload);
            spi_transfer(config->led_string[1].spi_dev);
        }
        // process string 2
        if(config->pixel_ports > 2)
        {
            b = 0;
            for(a=0;a<8;a++)
            {
                payload.channel_count[a] = config->led_string[2].channel_count[a];
                if(config->led_string[2].channel_count[a])
                {
                    memcpy(&payload.channel_data[b],&config->buffer[config->led_string[2].start_channel[a]],config->led_string[2].channel_count[a]);
                    b = b + config->led_string[2].channel_count[a];
                }            
            }

            config->led_string[2].spi_dev->buffer = (uint8_t*)&payload;
            config->led_string[2].spi_dev->count = sizeof(payload);
            spi_transfer(config->led_string[2].spi_dev);
        }
        // process string 3
        if(config->pixel_ports > 3)
        {
            b = 0;
            for(a=0;a<8;a++)
            {
                payload.channel_count[a] = config->led_string[3].channel_count[a];
                if(config->led_string[3].channel_count[a])
                {
                    memcpy(&payload.channel_data[b],&config->buffer[config->led_string[3].start_channel[a]],config->led_string[3].channel_count[a]);
                    b = b + config->led_string[3].channel_count[a];
                }            
            }

            config->led_string[3].spi_dev->buffer = (uint8_t*)&payload;
            config->led_string[3].spi_dev->count = sizeof(payload);
            spi_transfer(config->led_string[3].spi_dev);
        }
        current_timestamp = get_microsecond_timestamp();
        time_diff = current_timestamp - previous_timestamp;

        if (REFRESH_TIME > time_diff) 
        {
            usleep(REFRESH_TIME - time_diff);
        }
    }
}

