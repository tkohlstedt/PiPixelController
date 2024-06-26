#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "gpio.h"
#include "mailbox.h"
#include "spi_util.h"
#include "wiringPi.h"

#define TARGET_FREQ             800000
#define MAX_BUFFER_SIZE         3616
#define SPI_MODE                SPI_MODE_0
#define SPI_BITS                8

int wiringPi_init = 0;
int spi_fd;

#ifdef ORANGEPI
    int pins[7] = {15,16,0,0,0,0,0};
#endif


int spi_init(spi_device *spi_dev)
{
    gpio_t *gpio;
    static uint8_t mode;
    static uint8_t bits = SPI_BITS;
    uint32_t speed = TARGET_FREQ * 6;
    char devname[20];

#ifdef ORANGEPI
    strcpy(devname,"/dev/spidev1.0");
#else
    sprintf(devname,"/dev/spidev%i.%i",spi_dev->spi_bus,spi_dev->spi_cs);
#endif

    if(spi_dev->device_type = PIC){
        mode = SPI_MODE_0;
    }else{
        mode = SPI_MODE_1;
    }
    
    printf("%s\n\r",devname);

    if (!wiringPi_init){
        wiringPiSetup();
        wiringPi_init = 1;
        spi_fd = open(devname, O_RDWR);
        if (spi_fd < 0) {
            fprintf(stderr, "Cannot open %s. spi_bcm2835 module not loaded?\n",devname);
            return -1;
        }
    }

    spi_dev->dev_handle = spi_fd;

    // SPI mode
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0)
    {
        return -1;
    }
    if (ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) < 0)
    {
        return -1;
    }

    // Bits per word
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
    {
        return -1;
    }
    if (ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
    {
        return -1;
    }

    // Max speed Hz
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    {
        return -1;
    }
    if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
    {
        return -1;
    }

#ifdef ORANGEPI
        pinMode (pins[spi_dev->spi_cs], OUTPUT);
        printf("Setting Pinmode %d\n",pins[spi_dev->spi_cs]);
#endif
        // Set SPI-MOSI pin
/*
    gpio = mapmem(GPIO_OFFSET + base, sizeof(gpio_t), DEV_GPIOMEM);
    if (!gpio)
    {
        return -1;
    }
    gpio_function_set(gpio, pinnum, functions[spi_dev->spi_bus]);	// SPI-MOSI ALT0
*/
    // Allocate SPI transmit buffer
 /*
    spi_dev->buffer = malloc((MAX_BUFFER_SIZE));
    if (spi_dev->buffer == NULL)
    {
        return -1;
    }
        // Clear out the buffer
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
    {
        spi_dev->buffer[i] = 0x0;
    }
*/

    return 0;
}


int spi_transfer(spi_device *spi_dev)
{
    int ret;
    struct spi_ioc_transfer tr;

/*
    int functions[7] = {0,0,0,3,3,3,3};

    uint32_t base = spi_dev->rpi_hw->periph_base;
 */
   int pinnum = pins[spi_dev->spi_cs];
uint8_t *buff;
#ifdef ORANGEPI
    digitalWrite(pinnum,LOW);
#endif
    memset(&tr, 0, sizeof(struct spi_ioc_transfer));
    tr.tx_buf = (unsigned long)spi_dev->buffer;
    tr.rx_buf = 0;
    tr.len = spi_dev->count;

buff=spi_dev->buffer;
/*
printf("%i:%i:%i:%i:%i:%i:%i:%i:",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],buff[6],buff[7]);
printf("%i:%i:%i:%i:%i:%i:%i:%i\n",buff[8],buff[9],buff[10],buff[11],buff[12],buff[13],buff[14],buff[15]);
*/    
    ret = ioctl(spi_dev->dev_handle, SPI_IOC_MESSAGE(1), &tr);

#ifdef ORANGEPI
    digitalWrite(pinnum,HIGH);
#endif

    if (ret < 1)
    {
        fprintf(stderr, "Can't send spi message %i \n\r",ret);
        return -1;
    }

    return 0;
}