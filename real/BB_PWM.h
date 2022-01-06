/* 
 * File:   BB_PWM.h
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 23 mai 2016, 09:13
 */

#ifndef BB_PWM_H
#define BB_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enum is used for selecting PWM name.
 */
enum pwmName {
    P8_13 = 0,
    P8_19 = 1,
    P9_14 = 2,
    P9_16 = 3,
    P9_21 = 4,
    P9_22 = 5,
    P9_42 = 6,

    PWM2B = P8_13,
    PWM2A = P8_19,
    PWM1A = P9_14,
    PWM1B = P9_16,
    PWM0B = P9_21,
    PWM0A = P9_22,
    ECAP0 = P9_42,
} ;

/**
 * This enum is used for setting PWM polarity.
 */
enum polarityType {
    activeHigh = 0,
    activeLow = 1
} ;

#define PWM_MAX_PATH_SIZE 64

struct PWM {
    char basePath[PWM_MAX_PATH_SIZE];
    int dutyFile;
    unsigned long period;
} ;

int  PWM_initialize(struct PWM *dev, enum pwmName pin);
void PWM_terminate(struct PWM *dev);

void PWM_run(struct PWM *dev);
void PWM_stop(struct PWM *dev);

void PWM_setPeriod(struct PWM *dev, double period);
void PWM_setDutycycle(struct PWM *dev, double percent);
void PWM_setPolarity(struct PWM *dev, enum polarityType pol);
void PWM_setFrequency(struct PWM *dev, double freq);

#ifdef __cplusplus
}
#endif

#endif /* BB_PWM_H */

