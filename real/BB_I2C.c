/* 
 * File:   BB_I2C.h
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 17 mai 2016, 13:33
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "BB_I2C.h"

/* Local defines */
#define I2C_MAX_NAME_SIZE       (16)
#define NUM_MAX_BUSSES          (3)

static int i2c_file[NUM_MAX_BUSSES] = {-1, -1, -1};
static int i2c_refCount[NUM_MAX_BUSSES] = {0, 0, 0};
static uint8_t i2c_SlaveAddress[NUM_MAX_BUSSES] = {0xff, 0xff, 0xff};

/** Open I2C bus
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @return file descriptor, or -1 if an error occurred
 */
static int I2C_open(const unsigned int bus)
{

    char buf[I2C_MAX_NAME_SIZE];
    snprintf(buf, sizeof (buf), "/dev/i2c-%d", bus);

    int fd = open(buf, O_RDWR);
    if (fd < 0) perror("I2C_open/open");
    return fd;
}

/** Close I2C bus
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 */
static void I2C_close(const unsigned int bus)
{
    if (close(i2c_file[bus]) < 0) {
        /* EBADF, EINTR, EIO: In all cases, descriptor is torn down */
        perror("I2C_close/close");
    }
}

/** Select slave i2c peripheral
 * 
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @param address : i2c address of slave
 * @return errorcode
 */
int I2C_setSlave(const unsigned int bus, const uint8_t address)
{
    if (i2c_SlaveAddress[bus] == address) return I2C_SUCCESS;
    /* Set slave address if necessary */
    int err = ioctl(i2c_file[bus], I2C_SLAVE, address);
    if (err < 0) {
        perror("I2C_setSlave/ioctl");
        return err;
    }
    i2c_SlaveAddress[bus] = address;
    return err;
}

/** Return available status of an I2C bus
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @return 1 if available, 0 otherwise
 */
int I2C_isBusAvailable(const unsigned int bus)
{
    char buf[I2C_MAX_NAME_SIZE];
    snprintf(buf, sizeof (buf), "/dev/i2c-%d", bus);

    struct stat statBuf;
    if (stat(buf, &statBuf) == -1) {
        return 0;
    }
    return 1;
}

/**  Initialize I2C bus
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @return errorcode
 */
int I2C_init(const unsigned int bus)
{
    if (i2c_file[bus] < 0) {
        i2c_file[bus] = I2C_open(bus);
        if (i2c_file[bus] < 0) {
            perror("I2C_init/I2C_open");
            return ERR_I2C_INIT;
        }
    }
    i2c_refCount[bus]++;
    return I2C_SUCCESS;
}

/** Read from I2C device
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @param address : i2c address of slave
 * @param data : pointer to buffer
 * @param count : number of bytes to read
 * @return errorcode
 */
int I2C_read(const unsigned int bus, const uint8_t address, void *data, const int count)
{

    if (I2C_setSlave(bus, address) < 0) return ERR_I2C_INIT;

    int ret = read(i2c_file[bus], data, count);
    if (ret == -1) {
        perror("I2C_read/read");
        return ERR_I2C_READ_READ;
    }
    return ret;
}

/** Read a register on the I2C device
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @param address : i2c address of slave
 * @param reg : register offset
 * @param data : pointer to buffer
 * @param count : number of bytes to read
 * @return errorcode
 */
int I2C_readRegister(const unsigned int bus, const uint8_t address, const uint8_t reg, void *data, const int count)
{

    if (I2C_setSlave(bus, address) < 0) return ERR_I2C_INIT;

    /* Send register address we want to read */
    int ret = write(i2c_file[bus], (void *) &reg, 1);
    if (ret != 1) {
        perror("I2C_readRegister/write");
        return ERR_I2C_READ_REGISTER_WRITE;
    }

    /* Read register value */
    ret = read(i2c_file[bus], data, count);
    if (ret == -1) {
        perror("I2C_readRegister/read");
        return ERR_I2C_READ_REGISTER_READ;
    }
    return ret;
}

/** Write to I2C device
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @param address : i2c address of slave
 * @param data : pointer to buffer
 * @param count : number of bytes to write
 * @return errorcode
 */
int I2C_write(const unsigned int bus, const uint8_t address, void *data, const int count)
{

    if (I2C_setSlave(bus, address) < 0) return ERR_I2C_INIT;

    int ret = write(i2c_file[bus], data, count);
    if (ret != count) {
        perror("I2C_write/write");
        return ERR_I2C_WRITE_WRITE;
    }

    return I2C_SUCCESS;
}

/** Write to a register on the I2C device
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @param address : i2c address of slave
 * @param reg : register offset
 * @param data : pointer to buffer
 * @param count : number of bytes to write (max 6)
 * @return errorcode
 */
int I2C_writeRegister(const unsigned int bus, const uint8_t address, const uint8_t reg, void *data, const int count)
{

    if (I2C_setSlave(bus, address) < 0) return ERR_I2C_INIT;

    /* Check that we do not overrun buffer */
    if (count > 6) {
        return ERR_I2C_WRITE_REGISTER_WRITE;
    }
    int i;
    uint8_t buf[8];

    buf[0] = reg;
    for (i = 0; i < count; i++) {
        buf[i + 1] = ((uint8_t *) data)[i];
    }
    int ret = write(i2c_file[bus], buf, count + 1);
    if (ret == -1) {
        perror("I2C_write/write");
        return ERR_I2C_WRITE_REGISTER_WRITE;
    }

    return I2C_SUCCESS;
}

/** Close I2C bus (if not in use anymore).
 * @param bus : I2C bus number
 * @see I2C0,I2C1,I2C2
 * @return errorcode
 */
int I2C_terminate(const unsigned int bus)
{
    if (i2c_file[bus] < 0) return I2C_SUCCESS;

    i2c_refCount[bus]--;
    if (i2c_refCount[bus] == 0) {
        I2C_close(bus);
        i2c_file[bus] = -1;
        i2c_SlaveAddress[bus] = 0xff;
        return I2C_SUCCESS;
    }
    return I2C_SUCCESS;
}
