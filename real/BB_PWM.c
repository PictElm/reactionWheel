/* 
 * File:   BB_PWM.c
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 23 mai 2016, 09:13
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
#include "BB_PWM.h"

/**
 * This string array is used for mapping pwm name.
 */
static const char pwmNameMap[7][6] = {
    "P8_13",
    "P8_19",
    "P9_14",
    "P9_16",
    "P9_21",
    "P9_22",
    "P9_42",
};

/** Perform path expansion like a posix-shell.
 * @param pattern : path string  with wildcards
 * @param path : string buffer to receive expanded path
 * @param maxpathsize : size of \a path buffer
 * @return errorcode
 */
static int PWM_WildcardPath(const char *pattern, char *path, size_t maxpathsize)
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
 * @param dev : pointer to PWM struct
 * @param pin : output pin name
 * @see PWM0A
 * @return errorcode
 */
int PWM_initialize(struct PWM *dev, enum pwmName pin)
{
    int errorcode;
    char pattern[PWM_MAX_PATH_SIZE];
    snprintf(pattern, PWM_MAX_PATH_SIZE, "/sys/devices/ocp.*/pwm_test_%s.*", pwmNameMap[pin]);
    errorcode = PWM_WildcardPath(pattern, dev->basePath, PWM_MAX_PATH_SIZE);
    if (!errorcode) {
        /* open duty file for writing */
        snprintf(pattern, PWM_MAX_PATH_SIZE, "%s/%s", dev->basePath, "duty");
        dev->dutyFile = open(pattern, O_WRONLY | O_NONBLOCK);
        if (dev->dutyFile < 0) {
            perror("PWM_initialize/open");
            errorcode = errno;
        }
    } else {
        perror("PWM_initialize/WildcardPath");
    }
    return errorcode;
}

/** Turn on the PWM.
 * @param dev : pointer to PWM struct
 */
void PWM_run(struct PWM *dev)
{
    char fileName[PWM_MAX_PATH_SIZE];
    char strValue[2] = "1";
    int fd;
    snprintf(fileName, PWM_MAX_PATH_SIZE, "%s/%s", dev->basePath, "run");
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("PWM_run/open");
        return;
    }
    if (write(fd, strValue, 1) != 1) {
        perror("PWM_run/write");
    }
    close(fd);
}

/** Turn off the PWM.
 * @param dev : pointer to PWM struct
 */
void PWM_stop(struct PWM *dev)
{
    char fileName[PWM_MAX_PATH_SIZE];
    char strValue[2] = "0";
    int fd;
    snprintf(fileName, PWM_MAX_PATH_SIZE, "%s/%s", dev->basePath, "run");
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("PWM_stop/open");
        return;
    }
    if (write(fd, strValue, 1) != 1) {
        perror("PWM_stop/write");
    }
    close(fd);
}

/** Set the PWM active polarity.
 * @param dev : pointer to PWM struct
 * @param pol : activeHigh / activeLow
 */
void PWM_setPolarity(struct PWM *dev, enum polarityType pol)
{
    char fileName[PWM_MAX_PATH_SIZE];
    char strValue[8] = "";
    int fd, len;
    snprintf(fileName, PWM_MAX_PATH_SIZE, "%s/%s", dev->basePath, "polarity");
    len = snprintf(strValue, sizeof (strValue), "%d", pol);
    /* write polarity */
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("PWM_setPolarity/open");
        return;
    }
    if (write(fd, strValue, len) != len) {
        perror("PWM_setPolarity/write");
    }
    close(fd);
}

/** Set the PWM base period.
 * @param dev : pointer to PWM struct
 * @param period : period value in Second
 */
void PWM_setPeriod(struct PWM *dev, double period)
{
    char fileName[PWM_MAX_PATH_SIZE];
    char strValue[64] = "";
    int fd, len;
    dev->period = 1e9 * period;
    snprintf(fileName, PWM_MAX_PATH_SIZE, "%s/%s", dev->basePath, "period");
    len = snprintf(strValue, sizeof (strValue), "%lu", dev->period);
    /* write period */
    fd = open(fileName, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("PWM_setPeriod/open");
        return;
    }
    if (write(fd, strValue, len) != len) {
        perror("PWM_setPeriod/write");
    }
    close(fd);
}

/** Set the PWM base frequency.
 * @param dev : pointer to PWM struct
 * @param freq : frequency value in Hz
 */
void PWM_setFrequency(struct PWM *dev, double freq)
{
    PWM_setPeriod(dev, 1.0 / freq);
}

/** Set the PWM duty cycle.
 * @param dev : pointer to PWM struct
 * @param percent : duty cycle value in % ( 0 to 100 )
 */
void PWM_setDutycycle(struct PWM *dev, double percent)
{
    char strValue[64] = "";
    int len;
    unsigned long duty;
    percent = fmax(percent, 0.0);
    percent = fmin(percent, 100.0);
    duty = ((double) dev->period) * (percent / 100.0);
    len = snprintf(strValue, sizeof (strValue), "%lu", duty);
    /* write duty */
    if (write(dev->dutyFile, strValue, len) != len) {
        perror("PWM_setDutycycle/write");
    }
}

/** Close the device.
 * @param dev : pointer to PWM struct
 */
void PWM_terminate(struct PWM *dev)
{
    close(dev->dutyFile);
}
