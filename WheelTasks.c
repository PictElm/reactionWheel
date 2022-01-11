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
// Ajouté par Xavier & Florian
RT_TASK TimeManagement_TaskDescriptor;
RT_TASK AcquireAndLaw_TaskDescriptor;


//-------------------------------------------
//  Semaphores identifiers                   
//-------------------------------------------
// RT_SEM mySemaphore;                        
//     **** This space must be completed  if needed   *****

RT_SEM StartExperiment_Semaphore;
RT_SEM ExitApplication_Semaphore;
//RT_SEM Timer1ms_Semaphore; // Ajouté par Xavier & Florian

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

RT_QUEUE  TimeManagement_Queue;    // Ajouté par Xavier & Florian           

// Global variables communication and synchronization tasks by shared memory 
//--------------------------------------------------------------------------
static bool AbortExperiment_Flag;
static bool Finished_Flag; // Ajouté par Xavier & Florian

bool ExperimentIsAborted(void) { return AbortExperiment_Flag; }
bool ExperimentIsFinished(void) { return(Finished_Flag); } // Ajouté par Xavier & Florian

// les lignes suivantes ont été supprimées par Celestin & Jordan - TODO: A confirmer que c'est bien une erreur ?
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

// Ajouté par Xavier & Florian
void TimeManagement_Task()
{
    rt_printf("Starting Time Management task\r\n");
    
    /* Turn into periodic task : 1ms */
    rt_task_set_periodic(NULL,TM_NOW,1000000);

    char AP_flag = AP_FLAG;
    char LP_flag = LP_FLAG;
    char I_flag = I_FLAG;
    char F_flag = F_FLAG;

    

    while(true) { 


        rt_sem_p(&StartExperiment_Semaphore, TM_INFINITE);
        Finished_Flag = false;
        rt_task_start(&AcquireAndLaw_TaskDescriptor, &AcquireAndLaw_Task, NULL);

        int t=0; 
        //Push I to queue
	rt_queue_write(&TimeManagement_Queue, &I_flag, sizeof(char), Q_NORMAL);

        while(ConnectionIsActive() && t < ExperimentParameters.duration && !ExperimentIsAborted()) {
            rt_task_wait_period(NULL); /* Pending the next périod in 1ms */

            t++;

            if (t % ExperimentParameters.lawPeriod == 0){ 
                //Push LP to queue
		rt_queue_write(&TimeManagement_Queue, &LP_flag, sizeof(char), Q_NORMAL);
            } 

            if (t % ExperimentParameters.acquisitionPeriod == 0){ 
                //Push AP to queue 
		rt_queue_write(&TimeManagement_Queue, &AP_flag, sizeof(char), Q_NORMAL);
            } 

        } 

        Finished_Flag = true;
        // Push F to queue 
        rt_queue_write(&TimeManagement_Queue, &F_flag, sizeof(char), Q_NORMAL);
    } 
}

/*void Timer1ms_Task(){
	rt_sem_v(&Timer1ms_Semaphore);	
}*/

// Ajouté par Xavier & Florian
void LawFunction()
{
    rt_printf("Law Function function\r\n");
        
}

// Ajouté par Xavier & Florian
void CurvesAcquisition()
{
    rt_printf("Curves Acquisition function\r\n");
    
}

// Ajouté par Xavier & Florian
void Finishing()
{
    rt_printf("Finishing function\r\n");
    
}


// Ajouté par Xavier & Florian
void Initializing()
{
    rt_printf("Initializing function\r\n");
    
}

void AcquireAndLaw_Task()
{    
    char MsgBlock;
    
    rt_printf("Acquire And Law Task\r\n");
    int i;
    
    while (1){
        rt_queue_read(&TimeManagement_Queue, &MsgBlock, sizeof(char), TM_INFINITE);  
        rt_printf("Flag %d\n\r", MsgBlock);

        switch(MsgBlock){
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
    rt_printf("StartExperiment\n\r");
    AbortExperiment_Flag = false;  // Ajouté par Florian pour initialisation
    rt_sem_v(&StartExperiment_Semaphore);
}
//----------------------------------------------------------

void ReturnSensorsMeasurement()
{
    float samplesBlock[50 * 4];
    char terminationChar;
    int i = 0;
    
    RT_QUEUE_INFO queueInfo;

    // read from the queue
    // write into samplesBlock at the right position
    // increment i (for next iteration's position)
    // don't wait for message
    // if no messages, read returns EWOULDBLOCK, so the loop stops
    
    rt_queue_inquire(&SensorsMeasurement_Queue, &queueInfo);
    for (i=0; i<queueInfo.nmessages; i++){
        rt_queue_read(&SensorsMeasurement_Queue, samplesBlock + (i*4), 4*sizeof(float), TM_NONBLOCK);       
    }    
    
   
    // Truc de base
    /*for(i = 0;i<10;i++)
    {
        samplesBlock[ (i * 4)+0] = 0.1;
        samplesBlock[ (i * 4)+1] = 0.2;
        samplesBlock[ (i * 4)+2] = 0.3;
        samplesBlock[ (i * 4)+3] = 0.4;
    }*/
    
    
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
    // *** This space must be completed  if needed   *****

    // ------------------------------------- 
    // Events creation               
    // rt_event_create(xxxx);
    // *** This space must be completed if needed ******

    // Message queues creation               
    // ------------------------------------- 
    // TODO: decide on BUFFER_SIZE (like 50 or whever - see ReturnSenorsMeasurment)
    rt_queue_create(&SensorsMeasurement_Queue, "SensorsMeasurement_Queue", 50 * 4*sizeof(float), 50, Q_FIFO);
    rt_queue_create(&TimeManagement_Queue, "Queue TimeManagement", TIME_MANAGEMENT_QUEUE_BUFFER_SIZE * sizeof(char), TIME_MANAGEMENT_QUEUE_BUFFER_SIZE, Q_FIFO);  // Ajouté par Florian & Xavier

    // Semaphores creation                 
    // ------------------------------------
    rt_sem_create(&ExitApplication_Semaphore, "Exit", 0, S_FIFO);
    rt_sem_create(&StartExperiment_Semaphore, "StartExperiment", 0, S_PULSE);
    //rt_sem_create(&Timer1ms_Semaphore, "Timer1ms", 0, S_FIFO); // Ajouté par Florian & Xavier
    // **** This space must be completed  if needed   ***** 


    // Mutual exclusion semaphore creation                     
    //---------------------------------------------------------
    // rt_mutex_create(&myMutex, "mx" );                       
    //    **** This space must be completed  if needed   ***** 

    // Tasks creation                                          
    //---------------------------------------------------------
    //   **** This space must be completed  if needed   ***** 

    rt_task_create(&HumanMachineInterface_TaskDescriptor, "ManageRequestTask", DEFAULTSTACKSIZE, HUMAN_MACHINE_INTERFACE_PRIORITY, 0);

    // Ajouté par Florian & Xavier,
    // TODO: mode=0 à modifier et à faire valider
    rt_task_create(&TimeManagement_TaskDescriptor, "TimeManagement", DEFAULTSTACKSIZE, TIME_MANAGEMENT_PRIORITY, 0);
    rt_task_create(&AcquireAndLaw_TaskDescriptor, "AcquireAndLaw", DEFAULTSTACKSIZE, ACQUIRE_AND_LAW_PRIORITY, 0);

    // Tasks starting
    rt_task_start(&HumanMachineInterface_TaskDescriptor, &HumanMachineInterface_Task, NULL);

    // Ajouté par Florian & Xavier
    rt_task_start(&TimeManagement_TaskDescriptor, &TimeManagement_Task, NULL);
    

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

    // Ajouté par Florian & Xavier
    rt_task_delete(&TimeManagement_TaskDescriptor);
    rt_task_delete(&AcquireAndLaw_TaskDescriptor);

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
    rt_queue_delete(&TimeManagement_Queue); // Ajouté par Florian & Xavier


    rt_printf(" Application ..... finished--> exit\r\n");
    // Peripherals uninitialization 
    HardwareTerminate();
    return (0);
}
