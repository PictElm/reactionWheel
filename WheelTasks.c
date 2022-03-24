// ***********************************************************
// *                I S A E - S U P A E R O                  *
// *                                                         *
// *               Reaction Wheel Application                *
// *                      Version 2021                       *
// *                                                         *
// * Student version                                         *
// ***********************************************************

#include "ReactionWheel.h"

//--------------------------------------------
// Global declarations
//--------------------------------------------

#define HUMAN_MACHINE_INTERFACE_PRIORITY     33
#define TIME_MANAGEMENT_PRIORITY             42
#define ACQUIRE_AND_LAW_PRIORITY             10

#define TIME_MANAGEMENT_QUEUE_BUFFER_SIZE 4

#define AP_FLAG 0
#define LP_FLAG 1
#define I_FLAG 2
#define F_FLAG 3

//--------------------------------------------
// Tasks descriptors
//--------------------------------------------
RT_TASK HumanMachineInterface_TaskDescriptor;
// Added (resp. Task 2 and Task 3)
RT_TASK TimeManagement_TaskDescriptor;
RT_TASK AcquireAndLaw_TaskDescriptor;


//-------------------------------------------
// Semaphores identifiers
//-------------------------------------------
// Added is waited on before starting the experiment (in TimeManagment_Task)
RT_SEM StartExperiment_Semaphore;
// waited on in the main function, triggers the release of resources
RT_SEM ExitApplication_Semaphore;

//--------------------------------------------
// Message queues Identifiers
//--------------------------------------------
RT_QUEUE SensorsMeasurement_Queue;
// Added (contains flags AP_FLAG, LP_FLAG, I_FLAG and F_FLAG,
// witten to in TimeManagment_Task and read from AcquireAndLow_Task)
RT_QUEUE TimeManagement_Queue;

// Global variables communication and synchronization tasks by shared memory
//--------------------------------------------------------------------------
static bool AbortExperiment_Flag;
// Added (reset when starting the experiment,
// set when the experiment is over or aborted)
static bool Finished_Flag;

bool ExperimentIsAborted(void) { return AbortExperiment_Flag; }
bool ExperimentIsFinished(void) { return Finished_Flag; } // Added

// declared in WheelHMI.c
extern ExperimentParametersType ExperimentParameters;

// **************************************************************************
// *
// *                  The Tasks
// *
// **************************************************************************

void HumanMachineInterface_Task()
{
    rt_printf("Starting Human/Machine Interface task\r\n");
    ManageRequest();
}

void AcquireAndLaw_Task();

// Added (Task 2)
void TimeManagement_Task()
{
    rt_printf("Starting Time Management task\r\n");

    // Turn into periodic task : 1ms
    rt_task_set_periodic(NULL,TM_NOW,1000000);

    char AP_flag = AP_FLAG;
    char LP_flag = LP_FLAG;
    char I_flag = I_FLAG;
    char F_flag = F_FLAG;

    while (true) {
        // wait for the experiment to be started
        rt_sem_p(&StartExperiment_Semaphore, TM_INFINITE);
        Finished_Flag = false;
        // start Task 3
        rt_task_start(&AcquireAndLaw_TaskDescriptor, &AcquireAndLaw_Task, NULL);

        // time counter thingy
        int t = 0;
        // Push I to queue for Task 3 to read
        rt_queue_write(&TimeManagement_Queue, &I_flag, sizeof(char), Q_NORMAL);

        while (ConnectionIsActive() && t < ExperimentParameters.duration && !ExperimentIsAborted()) {
            rt_task_wait_period(NULL); // Pending the next period in 1ms

            t++;

            if (t % ExperimentParameters.lawPeriod == 0) {
                // Push LP to queue for Task 3 to read
               rt_queue_write(&TimeManagement_Queue, &LP_flag, sizeof(char), Q_NORMAL);
            }

            if (t % ExperimentParameters.acquisitionPeriod == 0) {
                // Push AP to queue for Task 3 to read
                rt_queue_write(&TimeManagement_Queue, &AP_flag, sizeof(char), Q_NORMAL);
            }

        }

        // the experiment is finished
        Finished_Flag = true;
        // Push F to queue
        rt_queue_write(&TimeManagement_Queue, &F_flag, sizeof(char), Q_NORMAL);
    }
}

// Added
void LawFunction()
{
    // read the values from the sensor...
    SampleType Ploudleretour;
    SampleAcquisition(&Ploudleretour);

    // ... and forward it to the law computation
    float WheelCommand = ComputeLaw(Ploudleretour);
    // then apply the command
    ApplySetpointCurrent(WheelCommand);
}

// Added
void CurvesAcquisition()
{
    // read the values form the sensor...
    float SensorsValues[4];
    SensorAcquisition(&SensorsValues);
    // ... and push them into the queue for the HMI task to read
    rt_queue_write(&SensorsMeasurement_Queue, &SensorsValues, 4*sizeof(float), Q_NORMAL);
}

// Added
void Finishing()
{
    rt_printf("Finishing\r\n");
    ApplySetpointCurrent(0); // stops the motor (command to 0)
}


// Added
void Initializing()
{
    rt_printf("Initializing\r\n");
    // the experiment needs to be initialize with an initial value from the sensor
    SampleType Ploudleretour;
    SampleAcquisition(&Ploudleretour);
    InitializeExperiment(Ploudleretour);
}

// Added (Task 3)
void AcquireAndLaw_Task()
{
    char MsgBlock; // the received action to do (from Task 2)

    rt_printf("Acquire And Law Task\r\n");

    while (1) {
        // wait for something to do
        rt_queue_read(&TimeManagement_Queue, &MsgBlock, sizeof(char), TM_INFINITE);

        // just do it
        switch (MsgBlock) {
            case AP_FLAG:
                CurvesAcquisition();
                break;
            case LP_FLAG:
                LawFunction();
                break;
            case I_FLAG:
                Initializing();
                break;
            case F_FLAG:
                Finishing();
                break;
        }
    }
}


//-----------------------------------------------------------
// functions called from HMI_Utilities

/** This function is called when the user press on Abort during an experiment
 * called from function "manageRequest" in file WheelHMI.c
 */
void AbortExperiment(void)
{
    AbortExperiment_Flag = true;
}
//-----------------------------------------------------------

void StartExperiment(void)
{
    rt_printf("Starting Experiment\n\r");
    AbortExperiment_Flag = false; // Added (reset experiment flag)
    rt_sem_v(&StartExperiment_Semaphore);
}
//----------------------------------------------------------

// Modified
// is called form the HMI when the ground station asks for the measurments
void ReturnSensorsMeasurement()
{
    float samplesBlock[50 * 4];
    char terminationChar;
    int i = 0;

    RT_QUEUE_INFO queueInfo;

    // read everything in the queue
    rt_queue_inquire(&SensorsMeasurement_Queue, &queueInfo);
    for (i = 0; i < queueInfo.nmessages; i++) {
        rt_queue_read(&SensorsMeasurement_Queue, samplesBlock + (i*4), 4*sizeof(float), TM_NONBLOCK);
    }

    // send (the 'F' flag is to be used only when the experiment is over)
    terminationChar = ExperimentIsFinished() ? 'F' : 'S';
    WriteRealArray(terminationChar, samplesBlock, i * 4);
}

//--------------------------------------------------------------------


/** Linux Signals Handler
 */
void stopNow(int sig)
{
    rt_sem_v(&ExitApplication_Semaphore); // give stop semaphore to main task
}

//--------------------------------------------------------------------
//
//  main()
//
//  Description: create and kill all the application's components
//
//--------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Kernel and peripherals initialization
    HardwareInitialize();
    rt_printf("---- Starting Realtime Application ----\r\n");

    // Signal manager to stop the application
    signal(SIGINT,  stopNow); // Interrupt from keyboard
    signal(SIGTERM, stopNow); // Termination signal

    // Global variables initialization
    // -------------------------------

    // -------------------------------------
    // Message queues creation
    // -------------------------------------
    rt_queue_create(&SensorsMeasurement_Queue, "SensorsMeasurement_Queue", 50 * 4*sizeof(float), 50, Q_FIFO);
    rt_queue_create(&TimeManagement_Queue, "Queue TimeManagement", TIME_MANAGEMENT_QUEUE_BUFFER_SIZE * sizeof(char), TIME_MANAGEMENT_QUEUE_BUFFER_SIZE, Q_FIFO); // Ajouté par Florian & Xavier

    // Semaphores creation
    // ------------------------------------
    rt_sem_create(&ExitApplication_Semaphore, "Exit", 0, S_FIFO);
    rt_sem_create(&StartExperiment_Semaphore, "StartExperiment", 0, S_PULSE);

    // Tasks creation
    //---------------------------------------------------------

    rt_task_create(&HumanMachineInterface_TaskDescriptor, "ManageRequestTask", DEFAULTSTACKSIZE, HUMAN_MACHINE_INTERFACE_PRIORITY, 0);

    // Added
    rt_task_create(&TimeManagement_TaskDescriptor, "TimeManagement", DEFAULTSTACKSIZE, TIME_MANAGEMENT_PRIORITY, 0);
    rt_task_create(&AcquireAndLaw_TaskDescriptor, "AcquireAndLaw", DEFAULTSTACKSIZE, ACQUIRE_AND_LAW_PRIORITY, 0);

    // Tasks starting
    rt_task_start(&HumanMachineInterface_TaskDescriptor, &HumanMachineInterface_Task, NULL);

    // Added
    rt_task_start(&TimeManagement_TaskDescriptor, &TimeManagement_Task, NULL);

    //-----------------------------------------------------------
    // Main Task waits on exit semaphore
    //-----------------------------------------------------------
    rt_sem_p(&ExitApplication_Semaphore, TM_INFINITE);

    rt_printf("\r\n \r\n Destruction of \r\n  - Tasks\r\n  - Queues\r\n  - Semaphores\r\n  - Events\r\n  - Mutexes\r\n");

    //-----------------------------------------------------------
    // Tasks destruction
    //-----------------------------------------------------------
    rt_task_delete(&HumanMachineInterface_TaskDescriptor);

    // Added
    rt_task_delete(&TimeManagement_TaskDescriptor);
    rt_task_delete(&AcquireAndLaw_TaskDescriptor);

    //------------------------------------------------------------
    // Semaphores destruction
    //-------------------------------------------------------------
    rt_sem_delete(&ExitApplication_Semaphore);
    rt_sem_delete(&StartExperiment_Semaphore);

    //------------------------------------------------------------
    // Events destruction
    //------------------------------------------------------------
    // rt_event_delete(&myEvents);

    //------------------------------------------------------------
    // Message queues destruction
    //------------------------------------------------------------
    rt_queue_delete(&SensorsMeasurement_Queue);
    rt_queue_delete(&TimeManagement_Queue); // Added

    rt_printf(" Application ..... finished--> exit\r\n");
    // Peripherals uninitialization
    HardwareTerminate();
    return 0;
}
