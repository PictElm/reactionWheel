/* 
 * File:   MPU9250.c
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 31 mai 2016, 10:11
 */

#include <stdio.h>
#include <endian.h>
#include <math.h>
#include "BB_I2C.h"
#include "MPU9250.h"

/** Prepare I2C device for general usage.
 * @param dev : pointer to MPU9250 struct
 * @param bus : I2C bus number
 * @param address : I2C device address
 * @return I2C errorcode defined in BB_I2C.h
 */
int MPU9250_initialize(struct MPU9250 *dev, int bus, uint8_t address)
{
    dev->bus = bus;
    dev->address = address;
    dev->gyro_fullscale = 250.0 * M_PI / 180.0;	/* rad/s */
    if (!I2C_isBusAvailable(dev->bus)) {
        perror("MPU9250_initialize/I2C_isBusAvailable");
        return ERR_I2C_INIT;
    }
    return I2C_init(dev->bus);
}

/** Write Configuration Register.
 * @p Gyrometer LowPassFilter = 98Hz
 * @param dev : pointer to MPU9250 struct
 */
void MPU9250_configure(struct MPU9250 *dev)
{
#define MPU9250_DLPF_BW_256         0x00
#define MPU9250_DLPF_BW_188         0x01
#define MPU9250_DLPF_BW_98          0x02
#define MPU9250_DLPF_BW_42          0x03
#define MPU9250_DLPF_BW_20          0x04
#define MPU9250_DLPF_BW_10          0x05
#define MPU9250_DLPF_BW_5           0x06
    uint8_t config = MPU9250_DLPF_BW_98;
    I2C_writeRegister(dev->bus, dev->address, MPU9250_RA_CONFIG, &config, 1);
}

/** Close I2C device 
 */
void MPU9250_terminate(struct MPU9250 *dev)
{
    I2C_terminate(dev->bus);
}

/** Test MPU9250 presence.
 * @return 1 on success
 */
int MPU9250_isConnected(struct MPU9250 *dev)
{
    uint8_t reg = 0x00;
    I2C_readRegister(dev->bus, dev->address, MPU9250_RA_WHO_AM_I, &reg, 1);
    return (reg == 0x71);
}

/** Read Gyro X axis.
 * @param dev : pointer to MPU9250 struct
 * @return result in Radian/s
 */
double MPU9250_getRotationX(struct MPU9250 *dev)
{
    int16_t reg = 0;
    double rads;
    I2C_readRegister(dev->bus, dev->address, MPU9250_RA_GYRO_XOUT_H, &reg , 2);
    reg = be16toh(reg);   /* convert from BigEndian */
    rads = ((double) reg) * dev->gyro_fullscale / 32768.0;
    return rads;
}

/** Read Gyro Y axis.
 * @param dev : pointer to MPU9250 struct
 * @return result in Radian/s
 */
double MPU9250_getRotationY(struct MPU9250 *dev)
{
    int16_t reg = 0;
    double rads;
    I2C_readRegister(dev->bus, dev->address, MPU9250_RA_GYRO_YOUT_H, &reg , 2);
    reg = be16toh(reg);   /* convert from BigEndian */
    rads = ((double) reg) * dev->gyro_fullscale / 32768.0;
    return rads;
}

/** Read Gyro Z axis.
 * @param dev : pointer to MPU9250 struct
 * @return result in Radian/s
 */
double MPU9250_getRotationZ(struct MPU9250 *dev)
{
    int16_t reg = 0;
    double rads;
    I2C_readRegister(dev->bus, dev->address, MPU9250_RA_GYRO_ZOUT_H, &reg , 2);
    reg = be16toh(reg);   /* convert from BigEndian */
    rads = ((double) reg) * dev->gyro_fullscale / 32768.0;
    return rads;
}

/** Read Internal Temperature.
 * @param dev : pointer to MPU9250 struct
 * @return result in °C
 */
float MPU9250_getTemperature(struct MPU9250 *dev)
{
    int16_t reg = 0;
    float degC;
    I2C_readRegister(dev->bus, dev->address, MPU9250_RA_TEMP_OUT_H, &reg , 2);
    reg = be16toh(reg);   /* convert from BigEndian */
    degC = ((float) reg) / 333.87 + 21.0;
    return degC;
}
