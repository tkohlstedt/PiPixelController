#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "acn.h"
#include "rpihw.h"
#include "pixelworker.h"
#include "PiPixelController.h"
#include "zcpp_implementation.h"
#include "spi_util.h"
#include <libconfig.h>

const rpi_hw_t *rpi_hw;

acnParam listen_param;
zcppParam zcpp_listen_param;

char *pixelBuffer;

thread_ctrl workers[MAX_WORKERS];
pthread_t threads[MAX_WORKERS + 1];
void *pthread_return_val;

int return_val;
char cfg_path[50];
config_t cfg;
config_setting_t *setting;
const char *str;
int universe_start,universe_count,universe_size;

int main()
{
    // Check what hardware is running
    rpi_hw = rpi_hw_detect();
    printf("%s\n\r",rpi_hw->desc);

    // Initialize the SPIs
    spi_device spi0_0_dev  = {
        .rpi_hw = rpi_hw,
        .spi_bus = 0,
        .spi_cs = 0,
        .count = MAX_CHANNELS_PER_DEVICE,
    };
    int spi_stat = spi_init(&spi0_0_dev);
    printf("SPI Init returned %i \n\r",spi_stat);

    spi_device spi0_1_dev = {
        .rpi_hw = rpi_hw,
        .spi_bus = 0,
        .spi_cs  = 1,
        .count = MAX_CHANNELS_PER_DEVICE,
    };
    spi_stat = spi_init(&spi0_1_dev);
    printf("SPI Init returned %i \n\r",spi_stat);

    spi_device spi0_2_dev  = {
        .rpi_hw = rpi_hw,
        .spi_bus = 0,
        .spi_cs = 2,
        .count = MAX_CHANNELS_PER_DEVICE,
    };
    spi_stat = spi_init(&spi0_2_dev);
    printf("SPI Init returned %i \n\r",spi_stat);

    spi_device spi0_3_dev = {
        .rpi_hw = rpi_hw,
        .spi_bus = 0,
        .spi_cs  = 3,
        .count = MAX_CHANNELS_PER_DEVICE,
    };
    spi_stat = spi_init(&spi0_3_dev);
    printf("SPI Init returned %i \n\r",spi_stat);

    config_init(&cfg);
    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, "/home/ubuntu/PiPixelController/PiPixelController.cfg"))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }
    if(config_lookup_int(&cfg, "universe_start", &universe_start))
        printf("Start Universe: %i\n", universe_start);
    else
    {
        fprintf(stderr, "No 'universe_start' setting in configuration file.\n");
        universe_start = START_UNIVERSE;
    }
    if(config_lookup_int(&cfg, "universe_count", &universe_count))
        printf("Universe Count: %i\n", universe_count);
    else
    {
        fprintf(stderr, "No 'universe_count' setting in configuration file.\n");
        universe_count = MAX_UNIVERSES;
    }
    if(config_lookup_int(&cfg, "universe_size", &universe_size))
        printf("Universe size: %i\n", universe_size);
    else
    {
        fprintf(stderr, "No 'universe_size' setting in configuration file.\n");
        universe_size = UNIVERSE_SIZE;
    }

    // allocate and clear the pixelbuffer
    pixelBuffer = malloc(universe_count * universe_size);
    memset(pixelBuffer,0,universe_count * universe_size);

    // setup the universes to listen for
    listen_param.universeStart = universe_start;
    listen_param.universeEnd = universe_start + universe_count - 1;
    listen_param.buffer = pixelBuffer;

    // start the listener
    return_val = pthread_create(&threads[0],NULL,acn_listen,(void *)&listen_param);
    printf("ACN start returned %i \n\r",return_val);

    // read the channel configurations from the config file
    for(int bus_ctr =0;bus_ctr<1;bus_ctr++)
    {
        memset(&workers[bus_ctr],0,sizeof(thread_ctrl));
        for(int cs_ctr = 0;cs_ctr<4;cs_ctr++)
        {
            sprintf(cfg_path,"bus%d.cs%d",bus_ctr,cs_ctr);
                    setting = config_lookup(&cfg, cfg_path);
            if(setting != NULL)
            {
                int count = config_setting_length(setting);
                int i;
                for(i = 0; i < count; ++i)
                {
                    config_setting_t *channels = config_setting_get_elem(setting, i);

                    /* Only output the record if all of the expected fields are present. */
                    int led_str,start_channel,channel_count;

                    if(!(config_setting_lookup_int(channels, "string", &led_str)
                        && config_setting_lookup_int(channels, "start", &start_channel)
                        && config_setting_lookup_int(channels,"channels", &channel_count)))
                        continue;
                    workers[bus_ctr].led_string[cs_ctr].channel_count[led_str] = channel_count;
                    workers[bus_ctr].led_string[cs_ctr].start_channel[led_str] = start_channel;
                    printf("%3d  %3d  %3d  %3d  %3d\n", bus_ctr,cs_ctr,led_str,start_channel,channel_count);
                }
            }
        }
    }

    // start the output worker(s)
    workers[0].buffer = pixelBuffer;
    workers[0].led_string[0].spi_dev = &spi0_0_dev;
    workers[0].led_string[1].spi_dev = &spi0_1_dev;
    workers[0].led_string[2].spi_dev = &spi0_2_dev;
    workers[0].led_string[3].spi_dev = &spi0_3_dev;
    
    return_val = pthread_create(&threads[3],NULL,worker,(void *)&workers[0]);
    printf("Worker 0 returned %i \n\r",return_val);

    // start the ZCPP multicast listener
    zcpp_listen_param.buffer = pixelBuffer;
    zcpp_listen_param.hwconfig = &workers[0];
    return_val = pthread_create(&threads[1],NULL,zcpp_multicast_listen,&zcpp_listen_param );
    printf("ZCPP start returned %i \n\r",return_val);

    // start the ZCPP unicast listener
//   return_val = pthread_create(&threads[2],NULL,zcpp_listen,&zcpp_listen_param);

    while(1)
    {
        usleep(100000);
    }
    pthread_exit(NULL);
}