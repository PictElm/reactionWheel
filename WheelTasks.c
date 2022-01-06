// ***********************************************************
// *                I S A E - S U P A E R O                  *
// *                                                         *
// *               Reaction Wheel Application                *
// *                      Version 2021                       *
// *                                                         *
// * Student version                                         *
// ***********************************************************

#include "ReactionWheel.h"
#include

//--------------------------------------------
// Global  declarations                      
//--------------------------------------------

#define HUMAN_MACHINE_INTERFACE_PRIORITY     33

// Ajouté par Xavier & Florian
// TODO: Priorities to be defined
#define TIME_MANAGEMENT_PRIORITY     33
#define CURVES_ACQUISITION_PRIORITY     33
#define LAW_FUNCTION_PRIORITY     33
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
RT_TASK CurvesAcquisition_TaskDescriptor;
RT_TASK LawFunction_TaskDescriptor;
RT_TASK Timer1ms_TaskDescriptor;


//-------------------------------------------
//  Semaphores identifiers                   
//-------------------------------------------
// RT_SEM mySemaphore;                        
//     **** This space must be completed  if needed   *****

RT_SEM StartExperiment_Semaphore;
RT_SEM ExitApplication_Semaphore;

// Ajouté par Xavier & Florian
RT_SEM Timer1ms_Semaphore;

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
// Ajouté par Xavier & Florian
RT_QUEUE  queue_TimeManagement;                


// Global variables communication and synchronization tasks by shared memory 
//--------------------------------------------------------------------------
// bool myFlag
//**** This space must be completed if needed *****  
static bool Finished_Flag;

bool ConnectionIsFinished(void)
{
    return(Finished_Flag);
}


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

// Ajouté par Xavier & Florian
void TimeManagement_Task()
{
    rt_printf("Starting Time Management task\r\n");

	char AP_flag = AF_FLAG;
	char LP_flag = LP_FLAG;
	char I_flag = I_FLAG;
	char F_flag = F_FLAG;

    Finished_Flag = false;

    while(true) { 
		
	
        rt_sem_p(&StartExperiment_Semaphore, TM_INFINITE);

        int t=0; 
        //Push I to queue
		rt_queue_write(&queue_TimeManagement, &I_flag, sizeof(char), Q_NORMAL);

        while(ConnectionIsActive() && t < ExperimentParameters.duration && !ExperimentIsAborted()) {
			// Attente du semaphore du timer 1ms
			rt_sem_p(&Timer1ms_Semaphore, 2000000); //timeout = 2ms
			
            t++;
			
            if (t % ExperimentParameters.lawPeriod == 0){ 
                //Push LP to queue
				rt_queue_write(&queue_TimeManagement, &LP_flag, sizeof(char), Q_NORMAL);
            } 

            if (t % ExperimentParameters.acquisitionPeriod == 0){ 
                //Push AP to queue 
				rt_queue_write(&queue_TimeManagement, &AP_flag, sizeof(char), Q_NORMAL);
            } 

        } 
		
        Finished_Flag = true;
        // Push F to queue 
		rt_queue_write(&queue_TimeManagement, &F_flag, sizeof(char), Q_NORMAL);
    } 
}

void Timer1ms_Task(){
	rt_sem_v(&Timer1ms_Semaphore);	
}

// Ajouté par Xavier & Florian
void LawFunction_Task()
{


}

// Ajouté par Xavier & Florian
void CurvesAcquisition_Task()
{


}


//-----------------------------------------------------------
// functions called from HMI_Utilities

/** This function is called when the user press on Abort during an experiment \n
 * called from function "manageRequest" in file WheelHMI.c  
 */
void AbortExperiment(void)
{
   
}
//-----------------------------------------------------------

void StartExperiment(void)
{
   
}
//----------------------------------------------------------

void ReturnSensorsMeasurement()
{
    float samplesBlock[50 * 4];
    char terminationChar;
    int i;
    
    for(i = 0; i < 10; i++) {
        samplesBlock[(i * 4)]     = 0.1;
        samplesBlock[(i * 4) + 1] = 0.2;
        samplesBlock[(i * 4) + 2] = 0.3;
        samplesBlock[(i * 4) + 3] = 0.4;
    }
   
    terminationChar = 'S';  // terminationChar = 'F' if the experiment is finished

    WriteRealArray(terminationChar, samplesBlock, 10 * 4);

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
    // *** This space must be completed  if needed   ******

    rt_queue_create(&queue_TimeManagement, "Queue TimeManagement", TIME_MANAGEMENT_QUEUE_BUFFER_SIZE * sizeof(char), TIME_MANAGEMENT_QUEUE_BUFFER_SIZE, Q_FIFO);

    // Semaphores creation                 
    // ------------------------------------
    rt_sem_create(&ExitApplication_Semaphore, "Exit", 0, S_FIFO);

	// Ajouté par Florian & Xavier
    rt_sem_create(&Timer1ms_Semaphore);
    // **** This space must be completed  if needed   ***** 
    

    // Mutual exclusion semaphore creation                     
    //---------------------------------------------------------
    // rt_mutex_create(&myMutex, "mx" );                       
    //    **** This space must be completed  if needed   ***** 

    // Tasks creation                                          
    //---------------------------------------------------------
    //   **** This space must be completed  if needed   ***** 
    
    rt_task_create(&HumanMachineInterface_TaskDescriptor, "ManageRequestTask", DEFAULTSTACKSIZE, HUMAN_MACHINE_INTERFACE_PRIORITY, 0);


    // Ajouté par Florian & Xavier,
	// TODO: mode=0 à modifier et à faire valider
    rt_task_create(&TimeManagement_TaskDescriptor, "TimeManagement", DEFAULTSTACKSIZE, TIME_MANAGEMENT_PRIORITY, 0);
    rt_task_create(&CurvesAcquisition_TaskDescriptor, "CurvesAcquisition", DEFAULTSTACKSIZE, CURVES_ACQUISITION_PRIORITY, 0);
    rt_task_create(&LawFunction_TaskDescriptor, "LawFunction", DEFAULTSTACKSIZE, LAW_FUNCTION_PRIORITY, 0);
    rt_task_create(&Timer1ms_TaskDescriptor, "Timer1ms", DEFAULTSTACKSIZE, LAW_FUNCTION_PRIORITY, 0);

	
    // Tasks starting
    rt_task_start(&HumanMachineInterface_TaskDescriptor, &HumanMachineInterface_Task, NULL);

    // Ajouté par Florian & Xavier
	rt_task_set_periodic(&Timer1ms_TaskDescriptor,TM_NOW,1000000); // activation each 1ms
    rt_task_start(&Timer1ms_TaskDescriptor, &Timer1ms_Task, NULL);
    rt_task_start(&TimeManagement_TaskDescriptor, &TimeManagement_Task, NULL);
    rt_task_start(&CurvesAcquisition_TaskDescriptor, &CurvesAcquisition_Task, NULL);
    rt_task_start(&LawFunction_TaskDescriptor, &LawFunction_Task, NULL);

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
    rt_task_delete(&Timer1ms_TaskDescriptor);	
    rt_task_delete(&TimeManagement_TaskDescriptor);
    rt_task_delete(&CurvesAcquisition_TaskDescriptor);
    rt_task_delete(&LawFunction_TaskDescriptor);
    

    //------------------------------------------------------------
    // Semaphores destruction                                     
    //-------------------------------------------------------------
    // rt_sem_ delete(&mySemaphore);                               
    //   **** This space must be completed  if needed   ***** 

    rt_sem_delete(&ExitApplication_Semaphore);
	
	// Ajouté par Florian & Xavier
    rt_sem_delete(&Timer1ms_Semaphore);

    //------------------------------------------------------------
    // Events destruction                                 
    //------------------------------------------------------------
    // rt_event_delete(&myEvents);                                 
    // **** This space must be completed  if needed   *****
   
    
    //------------------------------------------------------------
    // Message queues destruction                                 
    //------------------------------------------------------------
    // rt_queue_delete(&myQueue);                                 
    // **** This space must be completed  if needed   *****
	
	// Ajouté par Florian & Xavier
    rt_queue_delete(&queue_TimeManagement);

    rt_printf(" Application ..... finished--> exit\r\n");
    // Peripherals uninitialization 
    HardwareTerminate();
    return (0);
}
