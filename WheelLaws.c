// ***********************************************************
// *                I S A E - S U P A E R O                  *
// *                                                         *
// *               Reaction Wheel Application                *
// *                      Version 2021                       *
// *                                                         *
// * Student version                                         *
// ***********************************************************

#include "ReactionWheel.h"

// Local constants
#define VIT_MAX  0.5            // rd/s
#define ACCEL    (VIT_MAX/10)   // rd/s2  

// Local variables
float   Y_n0, Y_n1, Y_n2;
float   U_n0, U_n1, U_n2;
float   Ki, Kp, Kv;             // Law coefficients
float   A, B, C, D, E;
int     LawParameter;
float   PeriodLaw;
float error_before, current_before, int_before, error_before2, current_before2;

// Global variables
// declared in WheelHMI.c 
extern ExperimentParametersType ExperimentParameters;

/** Calls InitializeLaw and InitializeSetpoint functions
 * @param sample : sensors values\n
 * @Note This function must be called before each experiment
 * */
void InitializeExperiment(SampleType sample)
{
    InitializeLaw();
    InitializeSetpoint(sample.platformPosition);
}

/** Initialize the law coefficients and parameters
 * 
 * The structure expParameters is initialized by the function UpdateExperimentParameters (in WheelHMI.c file) \n
 * This function is called by "InitializeExperiment" 
 */
void InitializeLaw(void)
{
    Y_n1 = 0;
    U_n1 = 0;
    U_n2 = 0;
    Y_n2 = 0;
    
    error_before=0;
    current_before=0;
    int_before=0;
    error_before2=0;
    current_before2=0;
    
    LawParameter = ExperimentParameters.law;
    PeriodLaw = (float) ((ExperimentParameters.lawPeriod) / 1000.0);
    switch(LawParameter) {
    case 10:
        Kv = ExperimentParameters.lawCoeff[0];
        break;
    case 11:
        Ki = ExperimentParameters.lawCoeff[0];
        Kp = ExperimentParameters.lawCoeff[1];
        Kv = ExperimentParameters.lawCoeff[2];
        break;
    case 12:
        A = ExperimentParameters.lawCoeff[0];
        B = ExperimentParameters.lawCoeff[1];
        C = ExperimentParameters.lawCoeff[2];
        Kv = ExperimentParameters.lawCoeff[3];
        break;
    case 20:
        A = ExperimentParameters.lawCoeff[0];
        B = ExperimentParameters.lawCoeff[1];
        C = ExperimentParameters.lawCoeff[2];
        break;
    case 21:
        A = ExperimentParameters.lawCoeff[0];
        B = ExperimentParameters.lawCoeff[1];
        C = ExperimentParameters.lawCoeff[2];
        D = ExperimentParameters.lawCoeff[3];
        E = ExperimentParameters.lawCoeff[4];
        break;
    case 50:
        Kp = ExperimentParameters.lawCoeff[0];
        Kv = ExperimentParameters.lawCoeff[1];
        break;
    case 51:
        Ki =  ExperimentParameters.lawCoeff[0];
        Kp =  ExperimentParameters.lawCoeff[1];
        Kv =  ExperimentParameters.lawCoeff[2];
        break;
    }
 
}

/** Compute a new command 
 * @param measure : sensors values\n
 * @return The command to be sent to the motor
 * <li> This function must be called at each law period
 * <li> The setpoint is updated each time this function is called
 */
float ComputeLaw(SampleType measure)
{
    float wheelCommand, speed, position, error;
    float currentSetpoint;

    speed    = measure.platformSpeed;
    position = measure.platformPosition;


    currentSetpoint = ComputeNewSetpoint();

    error    = currentSetpoint - position;
    switch(LawParameter) {
    default:
    case 0:	   // openLoop
        wheelCommand = currentSetpoint;
        break;

    case 10:   // speed loop
        Kv = 12;
        wheelCommand = (currentSetpoint - speed) * Kv;
        break;

    case 11:   // continuous position loop
        Kp = 2;
        Kv = 2;

        U_n0 =  (error) * Ki;
        Y_n0 = Y_n1 + ((PeriodLaw / 2.0)*(U_n0 + U_n1));
        Y_n1 = Y_n0;
        U_n1 = U_n0;
        wheelCommand = (((error * Kp) + Y_n0) - speed) * Kv;
        break;
    case 12:  // discreet position loop
        

        break;
    case 20:   // lead compensator loop
        A = 3.242;
        B = -3.019;
        C = -0.6557;
        
        wheelCommand =  A*error + B*error_before - C*current_before;
        current_before = wheelCommand;
        error_before = error;
        
        break;
    case 21:   // lead  lag compensator loop
        
        A = 3.265;
        B = -6.213;
        C = 2.954;
        D = -1.641;
        E = 0.6463;
        
        wheelCommand = A*error + B*error_before + C*error_before2 - D*current_before - E*current_before2;
        current_before2 = current_before;
        error_before2 = error_before;
        current_before = wheelCommand;
        error_before = error;        
        
        break;
    case 50:   // state feedback
        Kp = 3.7616;
        Kv = 1.9724;
        
        wheelCommand = Kp * (error)- Kv*speed;
        
        break;

    case 51:   // state feedback with integrator
       
        Ki = 18.8082; // integral gain
        Kp = 13.6236;
        Kv = 4.6107;
        float threshold = 0.1745;       //10° in radians
        
        float integrator = PeriodLaw/2 * (error+error_before) + int_before;
        
        // If error is superior to threshold (10°), the integrator is disabled 
        // to get an equivalent of state feedback without integrator but with 
        // Kp and Kv from state feedback with integrator to ensure coefficient 
        // continuity
        if (error > threshold || error < -threshold) {
            integrator = currentSetpoint / Ki * Kp;
        }
        
        wheelCommand = Ki*integrator - Kp*position - Kv*speed;
        int_before = integrator;
        error_before = error;
                
        
        break;
    }//  end of switch (lawParameter)
    return(wheelCommand);
}
