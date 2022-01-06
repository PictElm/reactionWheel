/** 
 * File:   BB_EQEP.h
 * 
 * Author: Laurent Alloza <laurent.alloza at isae.fr>
 *
 * Created on 31 mai 2016, 15:18
 */

#ifndef BB_EQEP_H
#define BB_EQEP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
#define EQEP_SUCCESS                   (0)
#define EQEP_FAIL                      (-1)
#define ERR_EQEP_BASE                  (-2)
#define ERR_EQEP_INIT                  (ERR_EQEP_BASE)
#define ERR_EQEP_READ                  (ERR_EQEP_BASE-1)
#define ERR_EQEP_WRITE                 (ERR_EQEP_BASE-2)

/**
 * This enum is used for eqep device names
 *
 *              InA   InB   Ind   Str   \n
 * EQEP0 mapped P9_42 P9_27 P9_41 P9_25 \n
 * EQEP1 mapped P8_33 P8_35 P8_31 P8_32 \n
 * EQEP2 mapped P8_41 P8_42 P8_39 P8_40 \n
 */
enum eqepName {
    EQEP0 = 0,
    EQEP1 = 1,
    EQEP2 = 2,
} ;

/**
 * This enum is used for setting EQEP mode.
 * <li> absolute : position measurement
 * <li> relative : speed measurement
 */
enum eqepModeType {
    absolute = 0,
    relative = 1
} ;

#define EQEP_MAX_PATH_SIZE 64

struct EQEP {
    char basePath[EQEP_MAX_PATH_SIZE];
    int positionFile;
    double period;
} ;

int  EQEP_initialize(struct EQEP *dev, enum eqepName module);
void EQEP_terminate(struct EQEP *dev);

void EQEP_setMode(struct EQEP *dev, enum eqepModeType mode);
void EQEP_setPeriod(struct EQEP *dev, double period);
void EQEP_setFrequency(struct EQEP *dev, double freq);

void EQEP_run(struct EQEP *dev);
void EQEP_stop(struct EQEP *dev);

int32_t EQEP_getPosition(struct EQEP *dev);
double  EQEP_getSpeed(struct EQEP *dev);

#ifdef __cplusplus
}
#endif

#endif /* BB_EQEP_H */

