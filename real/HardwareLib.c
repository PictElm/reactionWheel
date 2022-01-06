// ***********************************************************
// *                I S A E - S U P A E R O                  *
// *                                                         *
// *               Reaction Wheel Application                *
// *                      Version 2021                       *
// *                                                         *
// ***********************************************************

#include "ReactionWheel.h"

#include "BB_PWM.h"
#include "BB_EQEP.h"
#include "BB_I2C.h"
#include "MPU9250.h"
#include "ADS1115.h"

/* Peripherals assignments for Reaction Wheel Hardware */

#define GYRO_I2CBUS      I2C2
#define GYRO_ADDRESS     MPU9250_DEFAULT_ADDRESS

#define POSITION_I2CBUS  I2C1
#define POSITION_ADDRESS ADS1115_ADDRESS_ADDR_GND

#define CURRENT_I2CBUS   I2C1
#define CURRENT_ADDRESS  ADS1115_ADDRESS_ADDR_VDD

/* local declarations */

struct {
    float motorSpeedOffset;       // default : 0.0
    float motorSpeedGain;         // default : -0.62832 (2*pi/100)
    float platformSpeedOffset;    // default : 0.0
    float platformSpeedGain;      // default : -1.0
    float platformPositionOffset; // default : 0.0
    float platformPositionGain;   // default : 1.256 (pi/2.5)
    float motorCurrentOffset;     // default : 0.0
    float motorCurrentGain;       // default : 1.6 (1/0.625)
    float motorCommandOffset;     // default : 0.0
    float motorCommandGain;       // default : 1.0
} calibration;

struct PWM     motorCommand;
struct EQEP    motorSpeedSensor;
struct ADS1115 motorCurrentSensor;
struct ADS1115 platformPotentiometer;
struct MPU9250 platformGyrometer;

/** Fill calibration parameters with new values.
 * @param array_of_float : array with 10 values.
 */
void updateCalibration(float array_of_float[])
{
    calibration.motorSpeedOffset       = array_of_float[0];
    calibration.motorSpeedGain         = array_of_float[1];
    calibration.platformSpeedOffset    = array_of_float[2];
    calibration.platformSpeedGain      = array_of_float[3];
    calibration.platformPositionOffset = array_of_float[4];
    calibration.platformPositionGain   = array_of_float[5];
    calibration.motorCurrentOffset     = array_of_float[6];
    calibration.motorCurrentGain       = array_of_float[7];
    calibration.motorCommandOffset     = array_of_float[8];
    calibration.motorCommandGain       = array_of_float[9];
}

/** Read calibration parameters from a text file.
 * @param filename : fullpath  to text file.
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int readCalibration(const char * filename)
{
    int i;
    char line[128];
    float value[10];
    FILE * fs = fopen(filename, "r");
    if (fs == NULL) return (EXIT_FAILURE);
    for (i = 0; i < 10; i++) {
        if ( fgets(line, 127, fs) == NULL ) break;
        if ( sscanf(line, "%f", value + i) != 1 ) break;
    }
    fclose(fs);
    if ( i == 10 ) {
        updateCalibration(value);
        return (EXIT_SUCCESS);
    } else
        return (EXIT_FAILURE);
}

/** Apply calibration */
float pulseToMotorSpeed(float pulses)
{
    return (pulses - calibration.motorSpeedOffset) * calibration.motorSpeedGain;
}

/** Apply calibration */
float radsToPlatformSpeed(float rads)
{
    return (rads - calibration.platformSpeedOffset) * calibration.platformSpeedGain;
}

/** Apply calibration */
float voltToPlatformPosition(float volts)
{
    return (volts - calibration.platformPositionOffset) * calibration.platformPositionGain;
}

/** Apply calibration */
float voltToMotorCurrent(float volts)
{
    return (volts - calibration.motorCurrentOffset) * calibration.motorCurrentGain;
}

/** Acquire all sensors with calibration.
 * @param sensor : array of float that receive calibrated sensors values
 * <li> sensor[0] = motor speed (rad/s)
 * <li> sensor[1] = platform speed (rad/s)
 * <li> sensor[2] = platform position (rad)
 * <li> sensor[3] = motor current (A)
 */
void SensorAcquisition(float sensor[4])
{
    sensor[0] = pulseToMotorSpeed     ( EQEP_getSpeed(&motorSpeedSensor)         );
    sensor[1] = radsToPlatformSpeed   ( MPU9250_getRotationZ(&platformGyrometer) );
    sensor[2] = voltToPlatformPosition( ADS1115_read(&platformPotentiometer)     );
    sensor[3] = voltToMotorCurrent    ( ADS1115_read(&motorCurrentSensor)        );
}

/** Acquire all sensors with calibration.
 * @param sample : pointer to struct that receive calibrated sensors values 
 * <li> motor speed (rad/s)
 * <li> platform speed (rad/s)
 * <li> platform position (rad)
 * <li> motor current (A)
 */
void SampleAcquisition(SampleType *sample)
{
    float sensor[4];
    SensorAcquisition(sensor);
    sample->motorSpeed       = sensor[0];
    sample->platformSpeed    = sensor[1];
    sample->platformPosition = sensor[2];
    sample->motorCurrent     = sensor[3];
}

/** Open and initialize all devices.\n
 * 
 * Exit application if devices are already in use.
 * 
 * Set motor command to zero.\n\n
 * Initialize calibration parameters values
 * with local file "/usr/local/etc/calibration.txt".\n
 * Lock Hardware for other processes.
 */
void HardwareInitialize(void)
{
    /* Disable buffering on stdout to have immediate display with ssh */
    setbuf(stdout, NULL);
#if CONFIG_XENO_VERSION_MAJOR == 2
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT | MCL_FUTURE);
    /* Perform auto-init of rt_print buffers */
    rt_print_auto_init(true);
    /* Cleanup rt_print buffers at exit */
    atexit(rt_print_cleanup);
    // Turns the current Linux task into a native Xenomai task (lowest priority). 
    static RT_TASK appTask;
    rt_task_shadow(&appTask, "reactionwheel", T_LOPRIO, 0);
#endif    
    /* Try to create a Lock in shared mem
     * Exit application with error if Lock already exist.
     */
    int lockfd = shm_open("/wheelLock", O_EXCL | O_RDWR | O_CREAT, 0666);	// try to create Lock
    if (lockfd < 0) {
        perror(" Hardware is used by another process");
        exit(EXIT_FAILURE);
    }
    close(lockfd);
    /* Initialize PWM at 20Khz */
    PWM_initialize(&motorCommand, PWM0A);
    PWM_stop(&motorCommand);
    PWM_setPolarity(&motorCommand, activeHigh);
    PWM_setDutycycle(&motorCommand,  0.0);
    PWM_setFrequency(&motorCommand, 20000);
    PWM_setDutycycle(&motorCommand, 100.0);
    PWM_run(&motorCommand);
    /* Initialize EQEP for speed measuring at 25Hz rate */
    EQEP_initialize(&motorSpeedSensor, EQEP0);
    EQEP_setPeriod(&motorSpeedSensor, 0.04);
    EQEP_setMode(&motorSpeedSensor, relative);
    EQEP_run(&motorSpeedSensor);
    /* Initialize Gyrometer with default values */
    MPU9250_initialize(&platformGyrometer, GYRO_I2CBUS, GYRO_ADDRESS);
    MPU9250_configure(&platformGyrometer);
    /* Initialize ADC on potentiometer with default values */
    ADS1115_initialize(&platformPotentiometer, POSITION_I2CBUS, POSITION_ADDRESS);
    ADS1115_configure(&platformPotentiometer);
    /* Initialize ADC on current sensor with default values */
    ADS1115_initialize(&motorCurrentSensor, CURRENT_I2CBUS, CURRENT_ADDRESS);
    ADS1115_configure(&motorCurrentSensor);

    /* Initialize calibration parameters */
    if ( readCalibration("/usr/local/etc/calibration.txt") != EXIT_SUCCESS ) {
        printf("Failed to read /usr/local/etc/calibration.txt\r\n");
        printf("Using factory calibration parameters.\r\n");

        /* Motor integrated encoder have 100 steps/round */
        calibration.motorSpeedOffset = 0.0;
        calibration.motorSpeedGain   = -2.0 * M_PI / 100.0;

        /* MPU9250 Gyro Fullscale is +/-250°/s */
        calibration.platformSpeedOffset = 0.0;
        calibration.platformSpeedGain   = -1.0;

        /* Potentiometer on ADS1115 +/-2.5V => +/-pi */
        calibration.platformPositionOffset = 0.0;
        calibration.platformPositionGain   = M_PI / 2.5;

        /* Current sensor (LTS6-NP) on ADS1115 +/-2.5V 625mV/A */
        calibration.motorCurrentOffset = 0.0;
        calibration.motorCurrentGain   = 1.0 / 0.625;

        /* PWM at 10KHz 0%=-0.6A 100%=+0.6A */
        calibration.motorCommandOffset = 0.0;
        calibration.motorCommandGain   = 1.0;
    }
}

/** Stop motor command.\n
 * Close all devices.\n
 * Release Hardware for other processes.
 */
void HardwareTerminate(void)
{
    ADS1115_terminate(&motorCurrentSensor);
    ADS1115_terminate(&platformPotentiometer);
    MPU9250_terminate(&platformGyrometer);
    EQEP_stop(&motorSpeedSensor);
    EQEP_terminate(&motorSpeedSensor);
    PWM_stop(&motorCommand);
    PWM_terminate(&motorCommand);
    shm_unlink("/wheelLock");	// Release Lock
}

/** Set motor command to Zero.
 */
void HardwareReset(void)
{
    ApplySetpointCurrent(0);
}

/** Apply motor command with saturation at +/-0.6A
 * @param current : desired current value in Ampere
 */
void ApplySetpointCurrent(float current)
{
    float duty;
    current = (current - calibration.motorCommandOffset) * calibration.motorCommandGain;
    /* process PWM duty cycle from current */
    duty = 100.0 * (current + 0.6) / 1.2;
    /* apply saturation */
    duty = (duty <  0.1 ?  0.1 : duty);
    duty = (duty > 99.9 ? 99.9 : duty);

    PWM_setDutycycle(&motorCommand, duty);
}
