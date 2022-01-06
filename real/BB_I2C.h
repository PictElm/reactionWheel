/* 
 * File:   BB_I2C.h
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 17 mai 2016, 13:33
 */

#ifndef BB_I2C_H_
#define BB_I2C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */

#define I2C_SUCCESS                   (0)
#define ERR_I2C_BASE                  (-2)
#define ERR_I2C_INIT                  (ERR_I2C_BASE)
#define ERR_I2C_READ_READ             (ERR_I2C_BASE-1)
#define ERR_I2C_WRITE_WRITE           (ERR_I2C_BASE-2)
#define ERR_I2C_READ_REGISTER_WRITE   (ERR_I2C_BASE-3)
#define ERR_I2C_READ_REGISTER_READ    (ERR_I2C_BASE-4)
#define ERR_I2C_WRITE_REGISTER_WRITE  (ERR_I2C_BASE-5)

/**
 * This defines are used for i2c bus number.\n
 * Linux Beaglebone devices mapping :
 * <li>I2C0 mapped to /dev/i2c-0 
 * <li>I2C2 mapped to /dev/i2c-1
 * <li>I2C1 mapped to /dev/i2c-2
 */
#define I2C0 0
#define I2C1 2
#define I2C2 1

/* I2C function interface */
int I2C_isBusAvailable(const unsigned int bus);
int I2C_init(const unsigned int bus);
int I2C_terminate(const unsigned int bus);

int I2C_read(const unsigned int bus, const uint8_t address, void *data, const int count);
int I2C_write(const unsigned int bus, const uint8_t address, void *data, const int count);
int I2C_readRegister(const unsigned int bus, const uint8_t address, const uint8_t reg, void *data, const int count);
int I2C_writeRegister(const unsigned int bus, const uint8_t address, const uint8_t reg, void *data, const int count);

#ifdef __cplusplus
}
#endif

#endif

