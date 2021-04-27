#ifndef __SPI_UTIL_H__
#define __SPI_UTIL_H__

#include "rpihw.h"

typedef struct __spi_device
{
    int spi_bus;
    int spi_cs;
    int dev_handle;
    const rpi_hw_t *rpi_hw; 
    uint16_t count;
    uint8_t *buffer;
} spi_device;

int spi_transfer(spi_device *spi_dev);

int spi_init(spi_device *spi_dev);

#endif