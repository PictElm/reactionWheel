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
// Global  declarations                      
//--------------------------------------------

#define HUMAN_MACHINE_INTERFACE_PRIORITY     33


//--------------------------------------------
// Tasks descriptors                          
//--------------------------------------------

RT_TASK HumanMachineInterface_TaskDescriptor;


//-------------------------------------------
//  Semaphores identifiers                   
//-------------------------------------------
// RT_SEM mySemaphore;                        
//     **** This space must be completed  if needed   *****

RT_SEM StartExperiment_Semaphore;
RT_SEM ExitApplication_Semaphore;

//--------------------------------------------
//  Events identifiers
//--------------------------------------------
// RT_EVENT myEvents;
//#define BIT(n) (1<<n)
//#define EVENT_1    BIT(0)
//#define EVENT_2    BIT(1)
//#define EVENT_ALL (EVENT_1 + EVENT_2)
//     **** This space must be completed if needed *****

//--------------------------------------------
// Mutual exclusion semaphores identifiers    
//--------------------------------------------
// RT_MUTEX myMutex;                          
//  ******   This space must be completed if needed ****** 

//--------------------------------------------
// Message queues Identifiers                 
//--------------------------------------------
RT_QUEUE  SensorsMeasurement_Queue;


// Global variables communication and synchronization tasks by shared memory 
//--------------------------------------------------------------------------
static bool AbortExperiment_Flag;

bool ExperimentIsAborted(void) { return AbortExperiment_Flag; }

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


//-----------------------------------------------------------
// functions called from HMI_Utilities

/** This function is called when the user press on Abort during an experiment \n
 * called from function "manageRequest" in file WheelHMI.c  
 */
void AbortExperiment(void)
{
   AbortExperiment_Flag = true;
}
//-----------------------------------------------------------

void StartExperiment(void)
{
   rt_sem_v(&StartExperiment_Semaphore);
}
//----------------------------------------------------------

void ReturnSensorsMeasurement()
{
    float samplesBlock[50 * 4];
    char terminationChar;
    int i = 0;
    
    // read from the queue
    // write into samplesBlock at the right position
    // increment i (for next iteration's position)
    // don't wait for message
    // if no messages, read returns EWOULDBLOCK, so the loop stops
    while(rt_queue_read(&SensorsMeasurement_Queue, samplesBlock + (i*4), 4*sizeof(float), TM_NONBLOCK) != EWOULDBLOCK)
        i++;
   
    terminationChar = ExperimentIsFinished() ? 'S' : 'F';

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
    // *** This space must be completed  if needed   *****

    // ------------------------------------- 
    // Events creation               
    // rt_event_create(xxxx);
    // *** This space must be completed if needed ******
    
    // Message queues creation               
    // ------------------------------------- 
    // TODO: decide on BUFFER_SIZE (like 50 or whever - see ReturnSenorsMeasurment)
    rt_queue_create(&SensorsMeasurement_Queue, "SensorsMeasurement_Queue", 50 * 4*sizeof(float), 50, Q_FIFO);

    // Semaphores creation                 
    // ------------------------------------
    rt_sem_create(&ExitApplication_Semaphore, "Exit", 0, S_FIFO);
    rt_sem_create(&StartExperiment_Semaphore, "StartExperiment", 0, S_PULSE);
    // **** This space must be completed  if needed   ***** 
    

    // Mutual exclusion semaphore creation                     
    //---------------------------------------------------------
    // rt_mutex_create(&myMutex, "mx" );                       
    //    **** This space must be completed  if needed   ***** 

    // Tasks creation                                          
    //---------------------------------------------------------
    //   **** This space must be completed  if needed   ***** 
    
    rt_task_create(&HumanMachineInterface_TaskDescriptor, "ManageRequestTask", DEFAULTSTACKSIZE, HUMAN_MACHINE_INTERFACE_PRIORITY, 0);

    // Tasks starting
    rt_task_start(&HumanMachineInterface_TaskDescriptor, &HumanMachineInterface_Task, NULL);

    //-----------------------------------------------------------
    // Main Task waits on exit semaphore                          
    //-----------------------------------------------------------
    rt_sem_p(&ExitApplication_Semaphore, TM_INFINITE);

    //-----------------------------------------------------------
    // Tasks destruction                                         
    //-----------------------------------------------------------
    // rt_task_ delete(&myTask);                                 
    rt_printf("\r\n \r\n Destruction of \r\n  - Tasks\r\n  - Queues\r\n  - Semaphores\r\n  - Events\r\n  - Mutexes\r\n");
    // **** This space must be completed *****  
    rt_task_delete(&HumanMachineInterface_TaskDescriptor);
    

    //------------------------------------------------------------
    // Semaphores destruction                                     
    //-------------------------------------------------------------
    // rt_sem_ delete(&mySemaphore);                               
    //   **** This space must be completed  if needed   ***** 

    rt_sem_delete(&ExitApplication_Semaphore);
    rt_sem_delete(&StartExperiment_Semaphore);

    //------------------------------------------------------------
    // Events destruction                                 
    //------------------------------------------------------------
    // rt_event_delete(&myEvents);                                 
    // **** This space must be completed  if needed   *****
   
    
    //------------------------------------------------------------
    // Message queues destruction                                 
    //------------------------------------------------------------
    rt_queue_delete(&SensorsMeasurement_Queue);
    

    rt_printf(" Application ..... finished--> exit\r\n");
    // Peripherals uninitialization 
    HardwareTerminate();
    return (0);
}
