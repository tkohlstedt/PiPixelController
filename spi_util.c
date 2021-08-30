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

#define TARGET_FREQ             800000
#define MAX_BUFFER_SIZE         3616
#define SPI_MODE                SPI_MODE_0
#define SPI_BITS                8

int spi_init(spi_device *spi_dev)
{
    int spi_fd;
    gpio_t *gpio;
    static uint8_t mode = SPI_MODE;  // this was previously unset
    static uint8_t bits = SPI_BITS;
    uint32_t speed = TARGET_FREQ * 6;
    char devname[20];

/*
    int pins[7] = {10,0,0,2,6,14,20};
    int functions[7] = {0,0,0,3,3,3,3};

    uint32_t base = spi_dev->rpi_hw->periph_base;
    int pinnum = pins[spi_dev->spi_bus];
 */
    sprintf(devname,"/dev/spidev%i.%i",spi_dev->spi_bus,spi_dev->spi_cs);
    printf("%s\n\r",devname);

    spi_fd = open(devname, O_RDWR);
    if (spi_fd < 0) {
        fprintf(stderr, "Cannot open %s. spi_bcm2835 module not loaded?\n",devname);
        return -1;
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

    memset(&tr, 0, sizeof(struct spi_ioc_transfer));
    tr.tx_buf = (unsigned long)spi_dev->buffer;
    tr.rx_buf = 0;
    tr.len = spi_dev->count;
    
    ret = ioctl(spi_dev->dev_handle, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
    {
        fprintf(stderr, "Can't send spi message %i \n\r",ret);
        return -1;
    }

    return 0;
}