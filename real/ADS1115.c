/*
 * File:   ADS1115.c
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 1 juin 2017, 07:35
 */

#include <stdio.h>
#include <endian.h>
#include "BB_I2C.h"
#include "ADS1115.h"

/** Prepare I2C device for general usage.
 * @param dev : pointer to ADS1115 struct
 * @param bus : I2C bus number
 * @param address : I2C device address
 * @return I2C errorcode defined in BB_I2C.h
 */
int ADS1115_initialize(struct ADS1115 *dev, int bus, uint8_t address)
{
    dev->bus = bus;
    dev->address = address;
    dev->fullscale = 2.048;	// default at power on
    if (!I2C_isBusAvailable(dev->bus)) {
        perror("ADS1115_initialize/I2C_isBusAvailable");
        return ERR_I2C_INIT;
    }
    return I2C_init(dev->bus);
}

/** Write Configuration Register.
 * @p AN0-AN1 128Hz continous mode, full scale = +/-4.096 Volts
 * @param dev : pointer to ADS1115 struct
 */
void ADS1115_configure(struct ADS1115 *dev)
{
    // configure for AN0-AN1 +-4.096V 128Hz
    uint16_t config = ADS1115_MUX_P0_N1 | ADS1115_PGA_4P096 | ADS1115_MODE_CONTINUOUS | ADS1115_RATE_128 | ADS1115_COMP_QUE_DISABLE;
    uint16_t reg = htobe16(config);
    I2C_writeRegister(dev->bus, dev->address, ADS1115_RA_CONFIG, &reg, 2);
    dev->fullscale = 4.096;
}

/** Read conversion result.
 * @param dev : pointer to ADS1115 struct
 * @return Conversion result in Volts
 */
double ADS1115_read(struct ADS1115 *dev)
{
    int16_t reg = 0;
    double voltage;
    I2C_readRegister(dev->bus, dev->address, ADS1115_RA_CONVERSION, &reg, 2);
    reg = be16toh(reg); /* convert from BigEndian */
    voltage = ((double) reg) * dev->fullscale / 32768.0;
    return voltage;
}

/** Close I2C device 
 */
void ADS1115_terminate(struct ADS1115 *dev)
{
    I2C_terminate(dev->bus);
}
