/* 
 * File:   BB_EQEP.c
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 3 juin 2016, 13:16
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <wordexp.h>
#include <sys/stat.h>
#include "BB_EQEP.h"

/** The paths for the eqep entries on the beaglebones are
 * "/sys/devices/ocp.* /{epwmss_addr}.epwmss/{epwmss_addr+0x180}.eqep/"
 */
static const unsigned long epwmssAddress[3] = {0x48300000, 0x48302000, 0x48304000};

/** Perform path expansion like a posix-shell.
 * @param pattern : path string  with wildcards
 * @param path : string buffer to receive expanded path
 * @param maxpathsize : size of \a path buffer
 * @return errorcode
 */
static int EQEP_WildcardPath(const char *pattern, char *path, size_t maxpathsize)
{
    int errorcode;
    wordexp_t p;
    struct stat st;

    errorcode = wordexp(pattern, &p, 0);
    if (!errorcode) {
        if (p.we_wordc > 0) {
            /* test if file exists */
            errorcode = stat(p.we_wordv[0], &st);
            if (!errorcode) {
                strncpy(path, p.we_wordv[0], maxpathsize);
            }
        } else {
            errorcode = -1;
        }
    }
    wordfree(&p);
    return errorcode;
}

/** Prepare for general usage.
 * @param dev : pointer to EQEP struct
 * @param module : eqep module name
 * @see EQEP0,EQEP1,EQEP2
 * @return errorcode
 */
int EQEP_initialize(struct EQEP *dev, enum eqepName module)
{
    int errorcode;
    char pattern[EQEP_MAX_PATH_SIZE];
    snprintf(pattern, EQEP_MAX_PATH_SIZE, "/sys/devices/ocp.*/%8lx.epwmss/%8lx.eqep", epwmssAddress[module], epwmssAddress[module] + 0x180);
    errorcode = EQEP_WildcardPath(pattern, dev->basePath, EQEP_MAX_PATH_SIZE);
    if (!errorcode) {
        /* open position file for reading */
        snprintf(pattern, EQEP_MAX_PATH_SIZE, "%s/%s", dev->basePath, "position");
        dev->positionFile = open(pattern, O_RDONLY | O_NONBLOCK);
        if (dev->positionFile < 0) {
            errorcode = ERR_EQEP_INIT;
            perror("EQEP_initialize/open");
        }
    } else {
        perror("EQEP_initialize/WildcardPath");
    }
    return errorcode;
}

/** Set EQEP module mode.
 * @param dev : pointer to EQEP struct
 * @param mode : absolute or relative
 */
void EQEP_setMode(struct EQEP *dev, enum eqepModeType mode)
{
    char fileName[EQEP_MAX_PATH_SIZE];
    char strValue[64] = "";
    int fd, len;
    snprintf(fileName, EQEP_MAX_PATH_SIZE, "%s/%s", dev->basePath, "mode");
    len = snprintf(strValue, sizeof (strValue), "%d", mode);
    /* write mode */
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("EQEP_setMode/open");
        return;
    }
    if (write(fd, strValue, len) != len) {
        perror("EQEP_setMode/write");
    }
    close(fd);
}

/** Turn on the EQEP measure.
 * @param dev : pointer to EQEP struct
 */
void EQEP_run(struct EQEP *dev)
{
    char fileName[EQEP_MAX_PATH_SIZE];
    char strValue[2] = "1";
    int fd;
    snprintf(fileName, EQEP_MAX_PATH_SIZE, "%s/%s", dev->basePath, "enabled");
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("EQEP_run/open");
        return;
    }
    if (write(fd, strValue, 1) != 1) {
        perror("EQEP_run/write");
    }
    close(fd);
}

/** Turn off the EQEP measure.
 * @param dev : pointer to EQEP struct
 */
void EQEP_stop(struct EQEP *dev)
{
    char fileName[EQEP_MAX_PATH_SIZE];
    char strValue[2] = "0";
    int fd;
    snprintf(fileName, EQEP_MAX_PATH_SIZE, "%s/%s", dev->basePath, "enabled");
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("EQEP_stop/open");
        return;
    }
    if (write(fd, strValue, 1) != 1) {
        perror("EQEP_stop/write");
    }
    close(fd);
}

/** Set reset period for relative mode.
 * @param dev : pointer to EQEP struct
 * @param period : in seconds
 */
void EQEP_setPeriod(struct EQEP *dev, double period)
{
    char fileName[EQEP_MAX_PATH_SIZE];
    char strValue[64] = "";
    int fd, len;
    unsigned long periodns = 1e9 * period;
    snprintf(fileName, EQEP_MAX_PATH_SIZE, "%s/%s", dev->basePath, "period");
    len = snprintf(strValue, sizeof (strValue), "%lu", periodns);
    /* write period */
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("EQEP_setPeriod/open");
        return;
    }
    if (write(fd, strValue, len) != len) {
        perror("EQEP_setPeriod/write");
    }
    dev->period = period;
    close(fd);
}

/** Set reset frequency for relative mode.
 * @param dev : pointer to EQEP struct
 * @param freq : in Hz
 */
void EQEP_setFrequency(struct EQEP *dev, double freq)
{
    EQEP_setPeriod(dev, 1.0 / freq);
}

/** Read position (in absolute mode).
 * @param dev : pointer to EQEP struct
 * @return absolute position : in pulse (x4)
 */
int32_t EQEP_getPosition(struct EQEP *dev)
{
    char strValue[64] = "";
    long position = 0;
    if (lseek(dev->positionFile, 0L, SEEK_SET) != 0) {
        perror("EQEP_getPosition/lseek");
        return position;
    }
    if (read(dev->positionFile, strValue, sizeof (strValue) - 1) <= 0) {
        perror("EQEP_getPosition/read");
        return position;
    }
    if (sscanf(strValue, "%ld", &position) != 1) {
        perror("EQEP_getPosition/sscanf");
    }
    return position;
}

/** Read speed (in relative mode).
 * @param dev : pointer to EQEP struct
 * @return speed : in pulse / second
 */
double EQEP_getSpeed(struct EQEP *dev)
{
    int32_t pos = EQEP_getPosition(dev) / 4;
    return ((double) pos) / dev->period;
}

/** Close the device.
 * @param dev : pointer to EQEP struct
 */
void EQEP_terminate(struct EQEP *dev)
{
    close(dev->positionFile);
}
