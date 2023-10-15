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
int spi_bus;
int acn_active;
const char *controller_name;
int pixel_port_count;
spi_device spi0_0_dev;
spi_device spi0_1_dev;
spi_device spi0_2_dev;
spi_device spi0_3_dev;

int main()
{
    // Check what hardware is running
//    rpi_hw = rpi_hw_detect();
//    printf("%s\n\r",rpi_hw->desc);


    config_init(&cfg);
    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, "/etc/PiPixelController.cfg"))
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
    if(config_lookup_string(&cfg, "controller_name", &controller_name))
    {
        printf("Controller Name: %s\n", controller_name);
        strncpy(zcpp_listen_param.controller_name,controller_name,32);
    }
    else
    {
        fprintf(stderr, "No 'controller_name' setting in configuration file.\n");
        strcpy(zcpp_listen_param.controller_name, CONTROLLER_NAME);
    }
    if(config_lookup_int(&cfg,"pixel_ports",&pixel_port_count))
        printf("Pixel ports: %i\n", pixel_port_count);
    else
    {
        fprintf(stderr,"No 'pixel_ports' setting in configuration file.\n");
        pixel_port_count = PIXEL_PORTS;
    }
    if(config_lookup_int(&cfg,"spi_bus",&spi_bus))
        printf("SPI Bus: %i\n", spi_bus);
    else
    {
        fprintf(stderr,"No 'spi_bus' setting in configuration file.  Defaulting to Bus %d\n",DEFAULT_SPI_BUS);
        spi_bus = DEFAULT_SPI_BUS;
    }
    if(config_lookup_int(&cfg,"listen_acn",&acn_active))
        printf("ACN Active: %i\n", acn_active);
    else
    {
        fprintf(stderr,"No 'listen_acn' setting in configuration file.  Defaulting to Active %d\n",DEFAULT_ACN_ACTIVE);
        acn_active = DEFAULT_ACN_ACTIVE;
    }
    // allocate and clear the pixelbuffer
    pixelBuffer = malloc(PIXEL_BUFFER_SIZE);
    memset(pixelBuffer,0,PIXEL_BUFFER_SIZE);

    // setup the universes to listen for
    listen_param.universeStart = universe_start;
    listen_param.universeEnd = universe_start + universe_count - 1;
    listen_param.buffer = pixelBuffer;

    // Initialize the SPIs
    int spi_stat;
    if(pixel_port_count > 0)
    {
        spi0_0_dev.rpi_hw = rpi_hw;
        spi0_0_dev.spi_bus = spi_bus;
        spi0_0_dev.spi_cs = 0;
        spi0_0_dev.count = MAX_CHANNELS_PER_DEVICE;
        spi_stat = spi_init(&spi0_0_dev);
        printf("SPI Init returned %i \n\r",spi_stat);
    }
    if(pixel_port_count > 1)
    {    
        spi0_1_dev.rpi_hw = rpi_hw;
        spi0_1_dev.spi_bus = spi_bus;
        spi0_1_dev.spi_cs  = 1;
        spi0_1_dev.count = MAX_CHANNELS_PER_DEVICE;
        spi_stat = spi_init(&spi0_1_dev);
        printf("SPI Init returned %i \n\r",spi_stat);
    }
    if(pixel_port_count >2)
    {    
        spi0_2_dev.rpi_hw = rpi_hw;
        spi0_2_dev.spi_bus = spi_bus;
        spi0_2_dev.spi_cs = 2;
        spi0_2_dev.count = MAX_CHANNELS_PER_DEVICE;
        spi_stat = spi_init(&spi0_2_dev);
        printf("SPI Init returned %i \n\r",spi_stat);
    }
    if(pixel_port_count>3)
    {
        spi0_3_dev.rpi_hw = rpi_hw,
        spi0_3_dev.spi_bus = spi_bus,
        spi0_3_dev.spi_cs  = 3,
        spi0_3_dev.count = MAX_CHANNELS_PER_DEVICE,
        spi_stat = spi_init(&spi0_3_dev);
        printf("SPI Init returned %i \n\r",spi_stat);
    }

    // start the listener
    if(acn_active){
        return_val = pthread_create(&threads[0],NULL,acn_listen,(void *)&listen_param);
        printf("ACN start returned %i \n\r",return_val);
    }
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
    workers[0].pixel_ports = pixel_port_count;
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