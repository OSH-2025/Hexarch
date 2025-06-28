#include <stdlib.h>
#include <string.h>
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS != 0 ) )
    #error configKERNEL_PROVIDED_STATIC_MEMORY cannot be set to 1 when using an MPU port. The vApplicationGet*TaskMemory() functions must be provided manually.
#endif
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#if ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )
    #include <stdio.h>
#endif 
#if ( configUSE_PREEMPTION == 0 )
    #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB )
    #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB )
#else
    #if ( configNUMBER_OF_CORES == 1 )
        #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB ) \
    do {                                                         \
        ( void ) ( pxTCB );                                      \
        portYIELD_WITHIN_API();                                  \
    } while( 0 )
        #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB ) \
    do {                                                        \
        if( pxCurrentTCB->uxPriority < ( pxTCB )->uxPriority )  \
        {                                                       \
            portYIELD_WITHIN_API();                             \
        }                                                       \
        else                                                    \
        {                                                       \
            mtCOVERAGE_TEST_MARKER();                           \
        }                                                       \
    } while( 0 )
    #else 
        #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB )    prvYieldCore( ( pxTCB )->xTaskRunState )
        #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB )     prvYieldForTask( pxTCB )
    #endif 
#endif 
#define taskNOT_WAITING_NOTIFICATION              ( ( uint8_t ) 0 ) 
#define taskWAITING_NOTIFICATION                  ( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED                 ( ( uint8_t ) 2 )
#define tskSTACK_FILL_BYTE                        ( 0xa5U )
#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB    ( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY        ( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB     ( ( uint8_t ) 2 )
#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )
    #define tskSET_NEW_STACKS_TO_KNOWN_VALUE    1
#else
    #define tskSET_NEW_STACKS_TO_KNOWN_VALUE    0
#endif
#define tskRUNNING_CHAR      ( 'X' )
#define tskBLOCKED_CHAR      ( 'B' )
#define tskREADY_CHAR        ( 'R' )
#define tskDELETED_CHAR      ( 'D' )
#define tskSUSPENDED_CHAR    ( 'S' )
#ifdef portREMOVE_STATIC_QUALIFIER
    #define static
#endif
#ifndef configIDLE_TASK_NAME
    #define configIDLE_TASK_NAME    "IDLE"
#endif
#if ( configNUMBER_OF_CORES > 1 )
    #if ( configMAX_TASK_NAME_LEN < 2U )
        #error Minimum required task name length is 2. Please increase configMAX_TASK_NAME_LEN.
    #endif
    #define taskRESERVED_TASK_NAME_LENGTH    2U
#else 
    #if ( configMAX_TASK_NAME_LEN < 1U )
        #error Minimum required task name length is 1. Please increase configMAX_TASK_NAME_LEN.
    #endif
    #define taskRESERVED_TASK_NAME_LENGTH    1U
#endif 
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
    #define taskRECORD_READY_PRIORITY( uxPriority ) \
    do {                                            \
        if( ( uxPriority ) > uxTopReadyPriority )   \
        {                                           \
            uxTopReadyPriority = ( uxPriority );    \
        }                                           \
    } while( 0 ) 
    #if ( configNUMBER_OF_CORES == 1 )
        #define taskSELECT_HIGHEST_PRIORITY_TASK()                                       \
    do {                                                                                 \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                                  \
                                                                                         \
                         \
        while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) != pdFALSE ) \
        {                                                                                \
            configASSERT( uxTopPriority );                                               \
            --uxTopPriority;                                                             \
        }                                                                                \
                                                                                         \
                            \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) ); \
        uxTopReadyPriority = uxTopPriority;                                                   \
    } while( 0 ) 
    #else 
        #define taskSELECT_HIGHEST_PRIORITY_TASK( xCoreID )    prvSelectHighestPriorityTask( xCoreID )
    #endif 
    #define taskRESET_READY_PRIORITY( uxPriority )
    #define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )
#else 
    #define taskRECORD_READY_PRIORITY( uxPriority )    portRECORD_READY_PRIORITY( ( uxPriority ), uxTopReadyPriority )
    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                                  \
    do {                                                                                        \
        UBaseType_t uxTopPriority;                                                              \
                                                                                                \
                                 \
        portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );                          \
        configASSERT( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxTopPriority ] ) ) > 0 ); \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );   \
    } while( 0 )
    #define taskRESET_READY_PRIORITY( uxPriority )                                                     \
    do {                                                                                               \
        if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 ) \
        {                                                                                              \
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );                        \
        }                                                                                              \
    } while( 0 )
#endif 
#define taskSWITCH_DELAYED_LISTS()                                                \
    do {                                                                          \
        List_t * pxTemp;                                                          \
                                                                                  \
         \
        configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList ) ) );               \
                                                                                  \
        pxTemp = pxDelayedTaskList;                                               \
        pxDelayedTaskList = pxOverflowDelayedTaskList;                            \
        pxOverflowDelayedTaskList = pxTemp;                                       \
        xNumOfOverflows = ( BaseType_t ) ( xNumOfOverflows + 1 );                 \
        prvResetNextTaskUnblockTime();                                            \
    } while( 0 )
#define prvAddTaskToReadyList( pxTCB )                                                                     \
    do {                                                                                                   \
        traceMOVED_TASK_TO_READY_STATE( pxTCB );                                                           \
        taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                                                \
        listINSERT_END( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
        tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB );                                                      \
    } while( 0 )
#define prvGetTCBFromHandle( pxHandle )    ( ( ( pxHandle ) == NULL ) ? pxCurrentTCB : ( pxHandle ) )
#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint16_t ) 0x8000U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint32_t ) 0x80000000U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint64_t ) 0x8000000000000000U )
#endif
#define taskTASK_NOT_RUNNING           ( ( BaseType_t ) ( -1 ) )
#define taskTASK_SCHEDULED_TO_YIELD    ( ( BaseType_t ) ( -2 ) )
#if ( configNUMBER_OF_CORES == 1 )
    #define taskTASK_IS_RUNNING( pxTCB )                          ( ( ( pxTCB ) == pxCurrentTCB ) ? ( pdTRUE ) : ( pdFALSE ) )
    #define taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB )    ( ( ( pxTCB ) == pxCurrentTCB ) ? ( pdTRUE ) : ( pdFALSE ) )
#else
    #define taskTASK_IS_RUNNING( pxTCB )                          ( ( ( ( pxTCB )->xTaskRunState >= ( BaseType_t ) 0 ) && ( ( pxTCB )->xTaskRunState < ( BaseType_t ) configNUMBER_OF_CORES ) ) ? ( pdTRUE ) : ( pdFALSE ) )
    #define taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB )    ( ( ( pxTCB )->xTaskRunState != taskTASK_NOT_RUNNING ) ? ( pdTRUE ) : ( pdFALSE ) )
#endif
#define taskATTRIBUTE_IS_IDLE    ( UBaseType_t ) ( 1U << 0U )
#if ( ( configNUMBER_OF_CORES > 1 ) && ( portCRITICAL_NESTING_IN_TCB == 1 ) )
    #define portGET_CRITICAL_NESTING_COUNT( xCoreID )          ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting )
    #define portSET_CRITICAL_NESTING_COUNT( xCoreID, x )       ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting = ( x ) )
    #define portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting++ )
    #define portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting-- )
#endif 
#define taskBITS_PER_BYTE    ( ( size_t ) 8 )
#if ( configNUMBER_OF_CORES > 1 )
    #define prvYieldCore( xCoreID )                                                          \
    do {                                                                                     \
        if( ( xCoreID ) == ( BaseType_t ) portGET_CORE_ID() )                                \
        {                                                                                    \
                     \
            xYieldPendings[ ( xCoreID ) ] = pdTRUE;                                          \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
                             \
            if( pxCurrentTCBs[ ( xCoreID ) ]->xTaskRunState != taskTASK_SCHEDULED_TO_YIELD ) \
            {                                                                                \
                portYIELD_CORE( xCoreID );                                                   \
                pxCurrentTCBs[ ( xCoreID ) ]->xTaskRunState = taskTASK_SCHEDULED_TO_YIELD;   \
            }                                                                                \
        }                                                                                    \
    } while( 0 )
#endif 
typedef struct tskTaskControlBlock       
{
    volatile StackType_t * pxTopOfStack; 
    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS xMPUSettings; 
    #endif
    #if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
        UBaseType_t uxCoreAffinityMask; 
    #endif
    ListItem_t xStateListItem;                  
    ListItem_t xEventListItem;                  
    UBaseType_t uxPriority;                     
    StackType_t * pxStack;                      
    #if ( configNUMBER_OF_CORES > 1 )
        volatile BaseType_t xTaskRunState;      
        UBaseType_t uxTaskAttributes;           
    #endif
    char pcTaskName[ configMAX_TASK_NAME_LEN ]; 
    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
        BaseType_t xPreemptionDisable; 
    #endif
    #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
        StackType_t * pxEndOfStack; 
    #endif
    #if ( portCRITICAL_NESTING_IN_TCB == 1 )
        UBaseType_t uxCriticalNesting; 
    #endif
    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxTCBNumber;  
        UBaseType_t uxTaskNumber; 
    #endif
    #if ( configUSE_MUTEXES == 1 )
        UBaseType_t uxBasePriority; 
        UBaseType_t uxMutexesHeld;
    #endif
    #if ( configUSE_APPLICATION_TASK_TAG == 1 )
        TaskHookFunction_t pxTaskTag;
    #endif
    #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
        void * pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
    #endif
    #if ( configGENERATE_RUN_TIME_STATS == 1 )
        configRUN_TIME_COUNTER_TYPE ulRunTimeCounter; 
    #endif
    #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        configTLS_BLOCK_TYPE xTLSBlock; 
    #endif
    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
        volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
        volatile uint8_t ucNotifyState[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    #endif
    #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        uint8_t ucStaticallyAllocated; 
    #endif
    #if ( INCLUDE_xTaskAbortDelay == 1 )
        uint8_t ucDelayAborted;
    #endif
    #if ( configUSE_POSIX_ERRNO == 1 )
        int iTaskErrno;
    #endif
} tskTCB;
typedef tskTCB TCB_t;
#if ( configNUMBER_OF_CORES == 1 )
    portDONT_DISCARD PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB = NULL;
#else
    portDONT_DISCARD PRIVILEGED_DATA TCB_t * volatile pxCurrentTCBs[ configNUMBER_OF_CORES ];
    #define pxCurrentTCB    xTaskGetCurrentTaskHandle()
#endif
PRIVILEGED_DATA static List_t pxReadyTasksLists[ configMAX_PRIORITIES ]; 
PRIVILEGED_DATA static List_t xDelayedTaskList1;                         
PRIVILEGED_DATA static List_t xDelayedTaskList2;                         
PRIVILEGED_DATA static List_t * volatile pxDelayedTaskList;              
PRIVILEGED_DATA static List_t * volatile pxOverflowDelayedTaskList;      
PRIVILEGED_DATA static List_t xPendingReadyList;                         
#if ( INCLUDE_vTaskDelete == 1 )
    PRIVILEGED_DATA static List_t xTasksWaitingTermination; 
    PRIVILEGED_DATA static volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;
#endif
#if ( INCLUDE_vTaskSuspend == 1 )
    PRIVILEGED_DATA static List_t xSuspendedTaskList; 
#endif
#if ( configUSE_POSIX_ERRNO == 1 )
    int FreeRTOS_errno = 0;
#endif
PRIVILEGED_DATA static volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
PRIVILEGED_DATA static volatile TickType_t xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
PRIVILEGED_DATA static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;
PRIVILEGED_DATA static volatile BaseType_t xSchedulerRunning = pdFALSE;
PRIVILEGED_DATA static volatile TickType_t xPendedTicks = ( TickType_t ) 0U;
PRIVILEGED_DATA static volatile BaseType_t xYieldPendings[ configNUMBER_OF_CORES ] = { pdFALSE };
PRIVILEGED_DATA static volatile BaseType_t xNumOfOverflows = ( BaseType_t ) 0;
PRIVILEGED_DATA static UBaseType_t uxTaskNumber = ( UBaseType_t ) 0U;
PRIVILEGED_DATA static volatile TickType_t xNextTaskUnblockTime = ( TickType_t ) 0U; 
PRIVILEGED_DATA static TaskHandle_t xIdleTaskHandles[ configNUMBER_OF_CORES ];       
static const volatile UBaseType_t uxTopUsedPriority = configMAX_PRIORITIES - 1U;
PRIVILEGED_DATA static volatile UBaseType_t uxSchedulerSuspended = ( UBaseType_t ) 0U;
#if ( configGENERATE_RUN_TIME_STATS == 1 )
PRIVILEGED_DATA static configRUN_TIME_COUNTER_TYPE ulTaskSwitchedInTime[ configNUMBER_OF_CORES ] = { 0U };    
PRIVILEGED_DATA static volatile configRUN_TIME_COUNTER_TYPE ulTotalRunTime[ configNUMBER_OF_CORES ] = { 0U }; 
#endif
static BaseType_t prvCreateIdleTasks( void );
#if ( configNUMBER_OF_CORES > 1 )
    static void prvCheckForRunStateChange( void );
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static void prvYieldForTask( const TCB_t * pxTCB );
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static void prvSelectHighestPriorityTask( BaseType_t xCoreID );
#endif 
#if ( INCLUDE_vTaskSuspend == 1 )
    static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif 
static void prvInitialiseTaskLists( void ) PRIVILEGED_FUNCTION;
static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters ) PRIVILEGED_FUNCTION;
#if ( configNUMBER_OF_CORES > 1 )
    static portTASK_FUNCTION_PROTO( prvPassiveIdleTask, pvParameters ) PRIVILEGED_FUNCTION;
#endif
#if ( INCLUDE_vTaskDelete == 1 )
    static void prvDeleteTCB( TCB_t * pxTCB ) PRIVILEGED_FUNCTION;
#endif
static void prvCheckTasksWaitingTermination( void ) PRIVILEGED_FUNCTION;
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                            const BaseType_t xCanBlockIndefinitely ) PRIVILEGED_FUNCTION;
#if ( configUSE_TRACE_FACILITY == 1 )
    static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t * pxTaskStatusArray,
                                                     List_t * pxList,
                                                     eTaskState eState ) PRIVILEGED_FUNCTION;
#endif
#if ( INCLUDE_xTaskGetHandle == 1 )
    static TCB_t * prvSearchForNameWithinSingleList( List_t * pxList,
                                                     const char pcNameToQuery[] ) PRIVILEGED_FUNCTION;
#endif
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )
    static configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte ) PRIVILEGED_FUNCTION;
#endif
#if ( configUSE_TICKLESS_IDLE != 0 )
    static TickType_t prvGetExpectedIdleTime( void ) PRIVILEGED_FUNCTION;
#endif
static void prvResetNextTaskUnblockTime( void ) PRIVILEGED_FUNCTION;
#if ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 )
    static char * prvWriteNameToBuffer( char * pcBuffer,
                                        const char * pcTaskName ) PRIVILEGED_FUNCTION;
#endif
static void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t * pxNewTCB,
                                  const MemoryRegion_t * const xRegions ) PRIVILEGED_FUNCTION;
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB ) PRIVILEGED_FUNCTION;
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    static TCB_t * prvCreateStaticTask( TaskFunction_t pxTaskCode,
                                        const char * const pcName,
                                        const configSTACK_DEPTH_TYPE uxStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        StackType_t * const puxStackBuffer,
                                        StaticTask_t * const pxTaskBuffer,
                                        TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif 
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    static TCB_t * prvCreateRestrictedStaticTask( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif 
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    static TCB_t * prvCreateRestrictedTask( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif 
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    static TCB_t * prvCreateTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif 
#ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
    static void freertos_tasks_c_additions_init( void ) PRIVILEGED_FUNCTION;
#endif
#if ( configUSE_PASSIVE_IDLE_HOOK == 1 )
    extern void vApplicationPassiveIdleHook( void );
#endif 
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    static size_t prvSnprintfReturnValueToCharsWritten( int iSnprintfReturnValue,
                                                        size_t n );
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static void prvCheckForRunStateChange( void )
    {
        UBaseType_t uxPrevCriticalNesting;
        const TCB_t * pxThisTCB;
        BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
        portASSERT_IF_IN_ISR();
        pxThisTCB = pxCurrentTCBs[ xCoreID ];
        while( pxThisTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD )
        {
            uxPrevCriticalNesting = portGET_CRITICAL_NESTING_COUNT( xCoreID );
            if( uxPrevCriticalNesting > 0U )
            {
                portSET_CRITICAL_NESTING_COUNT( xCoreID, 0U );
                portRELEASE_ISR_LOCK( xCoreID );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            portRELEASE_TASK_LOCK( xCoreID );
            portMEMORY_BARRIER();
            configASSERT( pxThisTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD );
            portENABLE_INTERRUPTS();
            portDISABLE_INTERRUPTS();
            xCoreID = ( BaseType_t ) portGET_CORE_ID();
            portGET_TASK_LOCK( xCoreID );
            portGET_ISR_LOCK( xCoreID );
            portSET_CRITICAL_NESTING_COUNT( xCoreID, uxPrevCriticalNesting );
            if( uxPrevCriticalNesting == 0U )
            {
                portRELEASE_ISR_LOCK( xCoreID );
            }
        }
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static void prvYieldForTask( const TCB_t * pxTCB )
    {
        BaseType_t xLowestPriorityToPreempt;
        BaseType_t xCurrentCoreTaskPriority;
        BaseType_t xLowestPriorityCore = ( BaseType_t ) -1;
        BaseType_t xCoreID;
        const BaseType_t xCurrentCoreID = portGET_CORE_ID();
        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            BaseType_t xYieldCount = 0;
        #endif 
        configASSERT( portGET_CRITICAL_NESTING_COUNT( xCurrentCoreID ) > 0U );
        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            if( pxTCB->uxPriority >= uxTopReadyPriority )
        #else
            if( taskTASK_IS_RUNNING( pxTCB ) == pdFALSE )
        #endif
        {
            xLowestPriorityToPreempt = ( BaseType_t ) pxTCB->uxPriority;
            --xLowestPriorityToPreempt;
            for( xCoreID = ( BaseType_t ) 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
            {
                xCurrentCoreTaskPriority = ( BaseType_t ) pxCurrentTCBs[ xCoreID ]->uxPriority;
                if( ( pxCurrentTCBs[ xCoreID ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                {
                    xCurrentCoreTaskPriority = ( BaseType_t ) ( xCurrentCoreTaskPriority - 1 );
                }
                if( ( taskTASK_IS_RUNNING( pxCurrentTCBs[ xCoreID ] ) != pdFALSE ) && ( xYieldPendings[ xCoreID ] == pdFALSE ) )
                {
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                        if( taskTASK_IS_RUNNING( pxTCB ) == pdFALSE )
                    #endif
                    {
                        if( xCurrentCoreTaskPriority <= xLowestPriorityToPreempt )
                        {
                            #if ( configUSE_CORE_AFFINITY == 1 )
                                if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                            #endif
                            {
                                #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                                    if( pxCurrentTCBs[ xCoreID ]->xPreemptionDisable == pdFALSE )
                                #endif
                                {
                                    xLowestPriorityToPreempt = xCurrentCoreTaskPriority;
                                    xLowestPriorityCore = xCoreID;
                                }
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        if( ( xCurrentCoreTaskPriority > ( ( BaseType_t ) tskIDLE_PRIORITY - 1 ) ) &&
                            ( xCurrentCoreTaskPriority < ( BaseType_t ) pxTCB->uxPriority ) )
                        {
                            prvYieldCore( xCoreID );
                            xYieldCount++;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #endif 
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                if( ( xYieldCount == 0 ) && ( xLowestPriorityCore >= 0 ) )
            #else 
                if( xLowestPriorityCore >= 0 )
            #endif 
            {
                prvYieldCore( xLowestPriorityCore );
            }
            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                if( ( ( pxCurrentTCBs[ xCurrentCoreID ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) == 0U ) &&
                    ( pxTCB->uxPriority > pxCurrentTCBs[ xCurrentCoreID ]->uxPriority ) )
                {
                    configASSERT( ( xYieldPendings[ xCurrentCoreID ] == pdTRUE ) ||
                                  ( taskTASK_IS_RUNNING( pxCurrentTCBs[ xCurrentCoreID ] ) == pdFALSE ) );
                }
            #endif
        }
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static void prvSelectHighestPriorityTask( BaseType_t xCoreID )
    {
        UBaseType_t uxCurrentPriority = uxTopReadyPriority;
        BaseType_t xTaskScheduled = pdFALSE;
        BaseType_t xDecrementTopPriority = pdTRUE;
        TCB_t * pxTCB = NULL;
        #if ( configUSE_CORE_AFFINITY == 1 )
            const TCB_t * pxPreviousTCB = NULL;
        #endif
        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            BaseType_t xPriorityDropped = pdFALSE;
        #endif
        configASSERT( xSchedulerRunning == pdTRUE );
        if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ),
                                     &pxCurrentTCBs[ xCoreID ]->xStateListItem ) == pdTRUE )
        {
            ( void ) uxListRemove( &pxCurrentTCBs[ xCoreID ]->xStateListItem );
            vListInsertEnd( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ),
                            &pxCurrentTCBs[ xCoreID ]->xStateListItem );
        }
        while( xTaskScheduled == pdFALSE )
        {
            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            {
                if( uxCurrentPriority < uxTopReadyPriority )
                {
                    uxCurrentPriority = tskIDLE_PRIORITY;
                }
            }
            #endif
            if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxCurrentPriority ] ) ) == pdFALSE )
            {
                const List_t * const pxReadyList = &( pxReadyTasksLists[ uxCurrentPriority ] );
                const ListItem_t * pxEndMarker = listGET_END_MARKER( pxReadyList );
                ListItem_t * pxIterator;
                xDecrementTopPriority = pdFALSE;
                for( pxIterator = listGET_HEAD_ENTRY( pxReadyList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
                {
                    pxTCB = ( TCB_t * ) listGET_LIST_ITEM_OWNER( pxIterator );
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        if( uxCurrentPriority < uxTopReadyPriority )
                        {
                            if( ( pxTCB->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) == 0U )
                            {
                                continue;
                            }
                        }
                    }
                    #endif 
                    if( pxTCB->xTaskRunState == taskTASK_NOT_RUNNING )
                    {
                        #if ( configUSE_CORE_AFFINITY == 1 )
                            if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                        #endif
                        {
                            pxCurrentTCBs[ xCoreID ]->xTaskRunState = taskTASK_NOT_RUNNING;
                            #if ( configUSE_CORE_AFFINITY == 1 )
                                pxPreviousTCB = pxCurrentTCBs[ xCoreID ];
                            #endif
                            pxTCB->xTaskRunState = xCoreID;
                            pxCurrentTCBs[ xCoreID ] = pxTCB;
                            xTaskScheduled = pdTRUE;
                        }
                    }
                    else if( pxTCB == pxCurrentTCBs[ xCoreID ] )
                    {
                        configASSERT( ( pxTCB->xTaskRunState == xCoreID ) || ( pxTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD ) );
                        #if ( configUSE_CORE_AFFINITY == 1 )
                            if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                        #endif
                        {
                            pxTCB->xTaskRunState = xCoreID;
                            xTaskScheduled = pdTRUE;
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    if( xTaskScheduled != pdFALSE )
                    {
                        break;
                    }
                }
            }
            else
            {
                if( xDecrementTopPriority != pdFALSE )
                {
                    uxTopReadyPriority--;
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        xPriorityDropped = pdTRUE;
                    }
                    #endif
                }
            }
            if( uxCurrentPriority > tskIDLE_PRIORITY )
            {
                uxCurrentPriority--;
            }
            else
            {
                break;
            }
        }
        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
        {
            if( xTaskScheduled == pdTRUE )
            {
                if( xPriorityDropped != pdFALSE )
                {
                    BaseType_t x;
                    for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configNUMBER_OF_CORES; x++ )
                    {
                        if( ( pxCurrentTCBs[ x ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                        {
                            prvYieldCore( x );
                        }
                    }
                }
            }
        }
        #endif 
        #if ( configUSE_CORE_AFFINITY == 1 )
        {
            if( xTaskScheduled == pdTRUE )
            {
                if( ( pxPreviousTCB != NULL ) && ( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxPreviousTCB->uxPriority ] ), &( pxPreviousTCB->xStateListItem ) ) != pdFALSE ) )
                {
                    UBaseType_t uxCoreMap = pxPreviousTCB->uxCoreAffinityMask;
                    BaseType_t xLowestPriority = ( BaseType_t ) pxPreviousTCB->uxPriority;
                    BaseType_t xLowestPriorityCore = -1;
                    BaseType_t x;
                    if( ( pxPreviousTCB->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                    {
                        xLowestPriority = xLowestPriority - 1;
                    }
                    if( ( uxCoreMap & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                    {
                        uxCoreMap &= ~( pxCurrentTCBs[ xCoreID ]->uxCoreAffinityMask );
                    }
                    else
                    {
                    }
                    uxCoreMap &= ( ( 1U << configNUMBER_OF_CORES ) - 1U );
                    for( x = ( ( BaseType_t ) configNUMBER_OF_CORES - 1 ); x >= ( BaseType_t ) 0; x-- )
                    {
                        UBaseType_t uxCore = ( UBaseType_t ) x;
                        BaseType_t xTaskPriority;
                        if( ( uxCoreMap & ( ( UBaseType_t ) 1U << uxCore ) ) != 0U )
                        {
                            xTaskPriority = ( BaseType_t ) pxCurrentTCBs[ uxCore ]->uxPriority;
                            if( ( pxCurrentTCBs[ uxCore ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                            {
                                xTaskPriority = xTaskPriority - ( BaseType_t ) 1;
                            }
                            uxCoreMap &= ~( ( UBaseType_t ) 1U << uxCore );
                            if( ( xTaskPriority < xLowestPriority ) &&
                                ( taskTASK_IS_RUNNING( pxCurrentTCBs[ uxCore ] ) != pdFALSE ) &&
                                ( xYieldPendings[ uxCore ] == pdFALSE ) )
                            {
                                #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                                    if( pxCurrentTCBs[ uxCore ]->xPreemptionDisable == pdFALSE )
                                #endif
                                {
                                    xLowestPriority = xTaskPriority;
                                    xLowestPriorityCore = ( BaseType_t ) uxCore;
                                }
                            }
                        }
                    }
                    if( xLowestPriorityCore >= 0 )
                    {
                        prvYieldCore( xLowestPriorityCore );
                    }
                }
            }
        }
        #endif 
    }
#endif 
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    static TCB_t * prvCreateStaticTask( TaskFunction_t pxTaskCode,
                                        const char * const pcName,
                                        const configSTACK_DEPTH_TYPE uxStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        StackType_t * const puxStackBuffer,
                                        StaticTask_t * const pxTaskBuffer,
                                        TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        configASSERT( puxStackBuffer != NULL );
        configASSERT( pxTaskBuffer != NULL );
        #if ( configASSERT_DEFINED == 1 )
        {
            volatile size_t xSize = sizeof( StaticTask_t );
            configASSERT( xSize == sizeof( TCB_t ) );
            ( void ) xSize; 
        }
        #endif 
        if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
        {
            pxNewTCB = ( TCB_t * ) pxTaskBuffer;
            ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
            pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;
            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif 
            prvInitialiseNewTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
        }
        else
        {
            pxNewTCB = NULL;
        }
        return pxNewTCB;
    }
    TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                    const char * const pcName,
                                    const configSTACK_DEPTH_TYPE uxStackDepth,
                                    void * const pvParameters,
                                    UBaseType_t uxPriority,
                                    StackType_t * const puxStackBuffer,
                                    StaticTask_t * const pxTaskBuffer )
    {
        TaskHandle_t xReturn = NULL;
        TCB_t * pxNewTCB;
        traceENTER_xTaskCreateStatic( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer );
        pxNewTCB = prvCreateStaticTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, &xReturn );
        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif
            prvAddNewTaskToReadyList( pxNewTCB );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_xTaskCreateStatic( xReturn );
        return xReturn;
    }
    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        TaskHandle_t xTaskCreateStaticAffinitySet( TaskFunction_t pxTaskCode,
                                                   const char * const pcName,
                                                   const configSTACK_DEPTH_TYPE uxStackDepth,
                                                   void * const pvParameters,
                                                   UBaseType_t uxPriority,
                                                   StackType_t * const puxStackBuffer,
                                                   StaticTask_t * const pxTaskBuffer,
                                                   UBaseType_t uxCoreAffinityMask )
        {
            TaskHandle_t xReturn = NULL;
            TCB_t * pxNewTCB;
            traceENTER_xTaskCreateStaticAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, uxCoreAffinityMask );
            pxNewTCB = prvCreateStaticTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, &xReturn );
            if( pxNewTCB != NULL )
            {
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;
                prvAddNewTaskToReadyList( pxNewTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            traceRETURN_xTaskCreateStaticAffinitySet( xReturn );
            return xReturn;
        }
    #endif 
#endif 
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    static TCB_t * prvCreateRestrictedStaticTask( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        configASSERT( pxTaskDefinition->puxStackBuffer != NULL );
        configASSERT( pxTaskDefinition->pxTaskBuffer != NULL );
        if( ( pxTaskDefinition->puxStackBuffer != NULL ) && ( pxTaskDefinition->pxTaskBuffer != NULL ) )
        {
            pxNewTCB = ( TCB_t * ) pxTaskDefinition->pxTaskBuffer;
            ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
            pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;
            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif 
            prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
                                  pxTaskDefinition->pcName,
                                  pxTaskDefinition->usStackDepth,
                                  pxTaskDefinition->pvParameters,
                                  pxTaskDefinition->uxPriority,
                                  pxCreatedTask, pxNewTCB,
                                  pxTaskDefinition->xRegions );
        }
        else
        {
            pxNewTCB = NULL;
        }
        return pxNewTCB;
    }
    BaseType_t xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;
        traceENTER_xTaskCreateRestrictedStatic( pxTaskDefinition, pxCreatedTask );
        configASSERT( pxTaskDefinition != NULL );
        pxNewTCB = prvCreateRestrictedStaticTask( pxTaskDefinition, pxCreatedTask );
        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif
            prvAddNewTaskToReadyList( pxNewTCB );
            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }
        traceRETURN_xTaskCreateRestrictedStatic( xReturn );
        return xReturn;
    }
    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateRestrictedStaticAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                           UBaseType_t uxCoreAffinityMask,
                                                           TaskHandle_t * pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;
            traceENTER_xTaskCreateRestrictedStaticAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask );
            configASSERT( pxTaskDefinition != NULL );
            pxNewTCB = prvCreateRestrictedStaticTask( pxTaskDefinition, pxCreatedTask );
            if( pxNewTCB != NULL )
            {
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;
                prvAddNewTaskToReadyList( pxNewTCB );
                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }
            traceRETURN_xTaskCreateRestrictedStaticAffinitySet( xReturn );
            return xReturn;
        }
    #endif 
#endif 
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    static TCB_t * prvCreateRestrictedTask( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        configASSERT( pxTaskDefinition->puxStackBuffer );
        if( pxTaskDefinition->puxStackBuffer != NULL )
        {
            pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );
            if( pxNewTCB != NULL )
            {
                ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
                pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;
                #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
                {
                    pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_ONLY;
                }
                #endif 
                prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
                                      pxTaskDefinition->pcName,
                                      pxTaskDefinition->usStackDepth,
                                      pxTaskDefinition->pvParameters,
                                      pxTaskDefinition->uxPriority,
                                      pxCreatedTask, pxNewTCB,
                                      pxTaskDefinition->xRegions );
            }
        }
        else
        {
            pxNewTCB = NULL;
        }
        return pxNewTCB;
    }
    BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
                                      TaskHandle_t * pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;
        traceENTER_xTaskCreateRestricted( pxTaskDefinition, pxCreatedTask );
        pxNewTCB = prvCreateRestrictedTask( pxTaskDefinition, pxCreatedTask );
        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif 
            prvAddNewTaskToReadyList( pxNewTCB );
            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }
        traceRETURN_xTaskCreateRestricted( xReturn );
        return xReturn;
    }
    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateRestrictedAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                     UBaseType_t uxCoreAffinityMask,
                                                     TaskHandle_t * pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;
            traceENTER_xTaskCreateRestrictedAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask );
            pxNewTCB = prvCreateRestrictedTask( pxTaskDefinition, pxCreatedTask );
            if( pxNewTCB != NULL )
            {
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;
                prvAddNewTaskToReadyList( pxNewTCB );
                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }
            traceRETURN_xTaskCreateRestrictedAffinitySet( xReturn );
            return xReturn;
        }
    #endif 
#endif 
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    static TCB_t * prvCreateTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        #if ( portSTACK_GROWTH > 0 )
        {
            pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );
            if( pxNewTCB != NULL )
            {
                ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
                pxNewTCB->pxStack = ( StackType_t * ) pvPortMallocStack( ( ( ( size_t ) uxStackDepth ) * sizeof( StackType_t ) ) );
                if( pxNewTCB->pxStack == NULL )
                {
                    vPortFree( pxNewTCB );
                    pxNewTCB = NULL;
                }
            }
        }
        #else 
        {
            StackType_t * pxStack;
            pxStack = pvPortMallocStack( ( ( ( size_t ) uxStackDepth ) * sizeof( StackType_t ) ) );
            if( pxStack != NULL )
            {
                pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );
                if( pxNewTCB != NULL )
                {
                    ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
                    pxNewTCB->pxStack = pxStack;
                }
                else
                {
                    vPortFreeStack( pxStack );
                }
            }
            else
            {
                pxNewTCB = NULL;
            }
        }
        #endif 
        if( pxNewTCB != NULL )
        {
            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif 
            prvInitialiseNewTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
        }
        return pxNewTCB;
    }
    BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;
        traceENTER_xTaskCreate( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );
        pxNewTCB = prvCreateTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );
        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif
            prvAddNewTaskToReadyList( pxNewTCB );
            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }
        traceRETURN_xTaskCreate( xReturn );
        return xReturn;
    }
    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateAffinitySet( TaskFunction_t pxTaskCode,
                                           const char * const pcName,
                                           const configSTACK_DEPTH_TYPE uxStackDepth,
                                           void * const pvParameters,
                                           UBaseType_t uxPriority,
                                           UBaseType_t uxCoreAffinityMask,
                                           TaskHandle_t * const pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;
            traceENTER_xTaskCreateAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask );
            pxNewTCB = prvCreateTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );
            if( pxNewTCB != NULL )
            {
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;
                prvAddNewTaskToReadyList( pxNewTCB );
                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }
            traceRETURN_xTaskCreateAffinitySet( xReturn );
            return xReturn;
        }
    #endif 
#endif 
static void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t * pxNewTCB,
                                  const MemoryRegion_t * const xRegions )
{
    StackType_t * pxTopOfStack;
    UBaseType_t x;
    #if ( portUSING_MPU_WRAPPERS == 1 )
        BaseType_t xRunPrivileged;
        if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
        {
            xRunPrivileged = pdTRUE;
        }
        else
        {
            xRunPrivileged = pdFALSE;
        }
        uxPriority &= ~portPRIVILEGE_BIT;
    #endif 
    #if ( tskSET_NEW_STACKS_TO_KNOWN_VALUE == 1 )
    {
        ( void ) memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE, ( size_t ) uxStackDepth * sizeof( StackType_t ) );
    }
    #endif 
    #if ( portSTACK_GROWTH < 0 )
    {
        pxTopOfStack = &( pxNewTCB->pxStack[ uxStackDepth - ( configSTACK_DEPTH_TYPE ) 1 ] );
        pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );
        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0U ) );
        #if ( configRECORD_STACK_HIGH_ADDRESS == 1 )
        {
            pxNewTCB->pxEndOfStack = pxTopOfStack;
        }
        #endif 
    }
    #else 
    {
        pxTopOfStack = pxNewTCB->pxStack;
        pxTopOfStack = ( StackType_t * ) ( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) + portBYTE_ALIGNMENT_MASK ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );
        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0U ) );
        pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( uxStackDepth - ( configSTACK_DEPTH_TYPE ) 1 );
    }
    #endif 
    if( pcName != NULL )
    {
        for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
        {
            pxNewTCB->pcTaskName[ x ] = pcName[ x ];
            if( pcName[ x ] == ( char ) 0x00 )
            {
                break;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1U ] = '\0';
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
    configASSERT( uxPriority < configMAX_PRIORITIES );
    if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
    pxNewTCB->uxPriority = uxPriority;
    #if ( configUSE_MUTEXES == 1 )
    {
        pxNewTCB->uxBasePriority = uxPriority;
    }
    #endif 
    vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
    vListInitialiseItem( &( pxNewTCB->xEventListItem ) );
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );
    listSET_LIST_ITEM_VALUE( &( pxNewTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority );
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xEventListItem ), pxNewTCB );
    #if ( portUSING_MPU_WRAPPERS == 1 )
    {
        vPortStoreTaskMPUSettings( &( pxNewTCB->xMPUSettings ), xRegions, pxNewTCB->pxStack, uxStackDepth );
    }
    #else
    {
        ( void ) xRegions;
    }
    #endif
    #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
    {
        configINIT_TLS_BLOCK( pxNewTCB->xTLSBlock, pxTopOfStack );
    }
    #endif
    #if ( portUSING_MPU_WRAPPERS == 1 )
    {
        #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        {
            #if ( portSTACK_GROWTH < 0 )
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
            }
            #else 
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
            }
            #endif 
        }
        #else 
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
        }
        #endif 
    }
    #else 
    {
        #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        {
            #if ( portSTACK_GROWTH < 0 )
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters );
            }
            #else 
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters );
            }
            #endif 
        }
        #else 
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
        }
        #endif 
    }
    #endif 
    #if ( configNUMBER_OF_CORES > 1 )
    {
        pxNewTCB->xTaskRunState = taskTASK_NOT_RUNNING;
        if( ( ( TaskFunction_t ) pxTaskCode == ( TaskFunction_t ) ( &prvIdleTask ) ) || ( ( TaskFunction_t ) pxTaskCode == ( TaskFunction_t ) ( &prvPassiveIdleTask ) ) )
        {
            pxNewTCB->uxTaskAttributes |= taskATTRIBUTE_IS_IDLE;
        }
    }
    #endif 
    if( pxCreatedTask != NULL )
    {
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
#if ( configNUMBER_OF_CORES == 1 )
    static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
    {
        taskENTER_CRITICAL();
        {
            uxCurrentNumberOfTasks = ( UBaseType_t ) ( uxCurrentNumberOfTasks + 1U );
            if( pxCurrentTCB == NULL )
            {
                pxCurrentTCB = pxNewTCB;
                if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
                {
                    prvInitialiseTaskLists();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                if( xSchedulerRunning == pdFALSE )
                {
                    if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
                    {
                        pxCurrentTCB = pxNewTCB;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            uxTaskNumber++;
            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                pxNewTCB->uxTCBNumber = uxTaskNumber;
            }
            #endif 
            traceTASK_CREATE( pxNewTCB );
            prvAddTaskToReadyList( pxNewTCB );
            portSETUP_TCB( pxNewTCB );
        }
        taskEXIT_CRITICAL();
        if( xSchedulerRunning != pdFALSE )
        {
            taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxNewTCB );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
#else 
    static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
    {
        taskENTER_CRITICAL();
        {
            uxCurrentNumberOfTasks++;
            if( xSchedulerRunning == pdFALSE )
            {
                if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
                {
                    prvInitialiseTaskLists();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            uxTaskNumber++;
            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                pxNewTCB->uxTCBNumber = uxTaskNumber;
            }
            #endif 
            traceTASK_CREATE( pxNewTCB );
            prvAddTaskToReadyList( pxNewTCB );
            portSETUP_TCB( pxNewTCB );
            if( xSchedulerRunning != pdFALSE )
            {
                taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxNewTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
    }
#endif 
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    static size_t prvSnprintfReturnValueToCharsWritten( int iSnprintfReturnValue,
                                                        size_t n )
    {
        size_t uxCharsWritten;
        if( iSnprintfReturnValue < 0 )
        {
            uxCharsWritten = 0;
        }
        else if( iSnprintfReturnValue >= ( int ) n )
        {
            uxCharsWritten = n - 1U;
        }
        else
        {
            uxCharsWritten = ( size_t ) iSnprintfReturnValue;
        }
        return uxCharsWritten;
    }
#endif 
#if ( INCLUDE_vTaskDelete == 1 )
    void vTaskDelete( TaskHandle_t xTaskToDelete )
    {
        TCB_t * pxTCB;
        BaseType_t xDeleteTCBInIdleTask = pdFALSE;
        BaseType_t xTaskIsRunningOrYielding;
        traceENTER_vTaskDelete( xTaskToDelete );
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTaskToDelete );
            configASSERT( pxTCB != NULL );
            if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
            {
                taskRESET_READY_PRIORITY( pxTCB->uxPriority );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
            {
                ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            uxTaskNumber++;
            xTaskIsRunningOrYielding = taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB );
            if( ( xSchedulerRunning != pdFALSE ) && ( xTaskIsRunningOrYielding != pdFALSE ) )
            {
                vListInsertEnd( &xTasksWaitingTermination, &( pxTCB->xStateListItem ) );
                ++uxDeletedTasksWaitingCleanUp;
                traceTASK_DELETE( pxTCB );
                xDeleteTCBInIdleTask = pdTRUE;
                #if ( configNUMBER_OF_CORES == 1 )
                    portPRE_TASK_DELETE_HOOK( pxTCB, &( xYieldPendings[ 0 ] ) );
                #else
                    portPRE_TASK_DELETE_HOOK( pxTCB, &( xYieldPendings[ pxTCB->xTaskRunState ] ) );
                #endif
                #if ( configNUMBER_OF_CORES > 1 )
                {
                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        if( pxTCB->xTaskRunState == ( BaseType_t ) portGET_CORE_ID() )
                        {
                            configASSERT( uxSchedulerSuspended == 0 );
                            taskYIELD_WITHIN_API();
                        }
                        else
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
                        }
                    }
                }
                #endif 
            }
            else
            {
                --uxCurrentNumberOfTasks;
                traceTASK_DELETE( pxTCB );
                prvResetNextTaskUnblockTime();
            }
        }
        taskEXIT_CRITICAL();
        if( xDeleteTCBInIdleTask != pdTRUE )
        {
            prvDeleteTCB( pxTCB );
        }
        #if ( configNUMBER_OF_CORES == 1 )
        {
            if( xSchedulerRunning != pdFALSE )
            {
                if( pxTCB == pxCurrentTCB )
                {
                    configASSERT( uxSchedulerSuspended == 0 );
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        #endif 
        traceRETURN_vTaskDelete();
    }
#endif 
#if ( INCLUDE_xTaskDelayUntil == 1 )
    BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                                const TickType_t xTimeIncrement )
    {
        TickType_t xTimeToWake;
        BaseType_t xAlreadyYielded, xShouldDelay = pdFALSE;
        traceENTER_xTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement );
        configASSERT( pxPreviousWakeTime );
        configASSERT( ( xTimeIncrement > 0U ) );
        vTaskSuspendAll();
        {
            const TickType_t xConstTickCount = xTickCount;
            configASSERT( uxSchedulerSuspended == 1U );
            xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;
            if( xConstTickCount < *pxPreviousWakeTime )
            {
                if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xConstTickCount ) )
                {
                    xShouldDelay = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xConstTickCount ) )
                {
                    xShouldDelay = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            *pxPreviousWakeTime = xTimeToWake;
            if( xShouldDelay != pdFALSE )
            {
                traceTASK_DELAY_UNTIL( xTimeToWake );
                prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        xAlreadyYielded = xTaskResumeAll();
        if( xAlreadyYielded == pdFALSE )
        {
            taskYIELD_WITHIN_API();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_xTaskDelayUntil( xShouldDelay );
        return xShouldDelay;
    }
#endif 
#if ( INCLUDE_vTaskDelay == 1 )
    void vTaskDelay( const TickType_t xTicksToDelay )
    {
        BaseType_t xAlreadyYielded = pdFALSE;
        traceENTER_vTaskDelay( xTicksToDelay );
        if( xTicksToDelay > ( TickType_t ) 0U )
        {
            vTaskSuspendAll();
            {
                configASSERT( uxSchedulerSuspended == 1U );
                traceTASK_DELAY();
                prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
            }
            xAlreadyYielded = xTaskResumeAll();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        if( xAlreadyYielded == pdFALSE )
        {
            taskYIELD_WITHIN_API();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskDelay();
    }
#endif 
#if ( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_xTaskAbortDelay == 1 ) )
    eTaskState eTaskGetState( TaskHandle_t xTask )
    {
        eTaskState eReturn;
        List_t const * pxStateList;
        List_t const * pxEventList;
        List_t const * pxDelayedList;
        List_t const * pxOverflowedDelayedList;
        const TCB_t * const pxTCB = xTask;
        traceENTER_eTaskGetState( xTask );
        configASSERT( pxTCB != NULL );
        #if ( configNUMBER_OF_CORES == 1 )
            if( pxTCB == pxCurrentTCB )
            {
                eReturn = eRunning;
            }
            else
        #endif
        {
            taskENTER_CRITICAL();
            {
                pxStateList = listLIST_ITEM_CONTAINER( &( pxTCB->xStateListItem ) );
                pxEventList = listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) );
                pxDelayedList = pxDelayedTaskList;
                pxOverflowedDelayedList = pxOverflowDelayedTaskList;
            }
            taskEXIT_CRITICAL();
            if( pxEventList == &xPendingReadyList )
            {
                eReturn = eReady;
            }
            else if( ( pxStateList == pxDelayedList ) || ( pxStateList == pxOverflowedDelayedList ) )
            {
                eReturn = eBlocked;
            }
            #if ( INCLUDE_vTaskSuspend == 1 )
                else if( pxStateList == &xSuspendedTaskList )
                {
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL )
                    {
                        #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                        {
                            BaseType_t x;
                            eReturn = eSuspended;
                            for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                            {
                                if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                                {
                                    eReturn = eBlocked;
                                    break;
                                }
                            }
                        }
                        #else 
                        {
                            eReturn = eSuspended;
                        }
                        #endif 
                    }
                    else
                    {
                        eReturn = eBlocked;
                    }
                }
            #endif 
            #if ( INCLUDE_vTaskDelete == 1 )
                else if( ( pxStateList == &xTasksWaitingTermination ) || ( pxStateList == NULL ) )
                {
                    eReturn = eDeleted;
                }
            #endif
            else
            {
                #if ( configNUMBER_OF_CORES == 1 )
                {
                    eReturn = eReady;
                }
                #else 
                {
                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        eReturn = eRunning;
                    }
                    else
                    {
                        eReturn = eReady;
                    }
                }
                #endif 
            }
        }
        traceRETURN_eTaskGetState( eReturn );
        return eReturn;
    }
#endif 
#if ( INCLUDE_uxTaskPriorityGet == 1 )
    UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        traceENTER_uxTaskPriorityGet( xTask );
        portBASE_TYPE_ENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            uxReturn = pxTCB->uxPriority;
        }
        portBASE_TYPE_EXIT_CRITICAL();
        traceRETURN_uxTaskPriorityGet( uxReturn );
        return uxReturn;
    }
#endif 
#if ( INCLUDE_uxTaskPriorityGet == 1 )
    UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_uxTaskPriorityGetFromISR( xTask );
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            uxReturn = pxTCB->uxPriority;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_uxTaskPriorityGetFromISR( uxReturn );
        return uxReturn;
    }
#endif 
#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )
    UBaseType_t uxTaskBasePriorityGet( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        traceENTER_uxTaskBasePriorityGet( xTask );
        portBASE_TYPE_ENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            uxReturn = pxTCB->uxBasePriority;
        }
        portBASE_TYPE_EXIT_CRITICAL();
        traceRETURN_uxTaskBasePriorityGet( uxReturn );
        return uxReturn;
    }
#endif 
#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )
    UBaseType_t uxTaskBasePriorityGetFromISR( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_uxTaskBasePriorityGetFromISR( xTask );
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            uxReturn = pxTCB->uxBasePriority;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_uxTaskBasePriorityGetFromISR( uxReturn );
        return uxReturn;
    }
#endif 
#if ( INCLUDE_vTaskPrioritySet == 1 )
    void vTaskPrioritySet( TaskHandle_t xTask,
                           UBaseType_t uxNewPriority )
    {
        TCB_t * pxTCB;
        UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
        BaseType_t xYieldRequired = pdFALSE;
        #if ( configNUMBER_OF_CORES > 1 )
            BaseType_t xYieldForTask = pdFALSE;
        #endif
        traceENTER_vTaskPrioritySet( xTask, uxNewPriority );
        configASSERT( uxNewPriority < configMAX_PRIORITIES );
        if( uxNewPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
        {
            uxNewPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );
            #if ( configUSE_MUTEXES == 1 )
            {
                uxCurrentBasePriority = pxTCB->uxBasePriority;
            }
            #else
            {
                uxCurrentBasePriority = pxTCB->uxPriority;
            }
            #endif
            if( uxCurrentBasePriority != uxNewPriority )
            {
                if( uxNewPriority > uxCurrentBasePriority )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        if( pxTCB != pxCurrentTCB )
                        {
                            if( uxNewPriority > pxCurrentTCB->uxPriority )
                            {
                                xYieldRequired = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                        }
                    }
                    #else 
                    {
                        xYieldForTask = pdTRUE;
                    }
                    #endif 
                }
                else if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                        if( pxTCB->xPreemptionDisable == pdFALSE )
                    #endif
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
                else
                {
                }
                uxPriorityUsedOnEntry = pxTCB->uxPriority;
                #if ( configUSE_MUTEXES == 1 )
                {
                    if( ( pxTCB->uxBasePriority == pxTCB->uxPriority ) || ( uxNewPriority > pxTCB->uxPriority ) )
                    {
                        pxTCB->uxPriority = uxNewPriority;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    pxTCB->uxBasePriority = uxNewPriority;
                }
                #else 
                {
                    pxTCB->uxPriority = uxNewPriority;
                }
                #endif 
                if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                {
                    listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxNewPriority ) );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
                if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
                {
                    if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        portRESET_READY_PRIORITY( uxPriorityUsedOnEntry, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    prvAddTaskToReadyList( pxTCB );
                }
                else
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    #else
                    {
                        xYieldForTask = pdFALSE;
                    }
                    #endif
                }
                if( xYieldRequired != pdFALSE )
                {
                    taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB );
                }
                else
                {
                    #if ( configNUMBER_OF_CORES > 1 )
                        if( xYieldForTask != pdFALSE )
                        {
                            taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
                        }
                        else
                    #endif 
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                ( void ) uxPriorityUsedOnEntry;
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_vTaskPrioritySet();
    }
#endif 
#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    void vTaskCoreAffinitySet( const TaskHandle_t xTask,
                               UBaseType_t uxCoreAffinityMask )
    {
        TCB_t * pxTCB;
        BaseType_t xCoreID;
        traceENTER_vTaskCoreAffinitySet( xTask, uxCoreAffinityMask );
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            pxTCB->uxCoreAffinityMask = uxCoreAffinityMask;
            if( xSchedulerRunning != pdFALSE )
            {
                if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    xCoreID = ( BaseType_t ) pxTCB->xTaskRunState;
                    if( ( uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) == 0U )
                    {
                        prvYieldCore( xCoreID );
                    }
                }
                else
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        prvYieldForTask( xTask );
                    }
                    #else 
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    #endif 
                }
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_vTaskCoreAffinitySet();
    }
#endif 
#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    UBaseType_t vTaskCoreAffinityGet( ConstTaskHandle_t xTask )
    {
        const TCB_t * pxTCB;
        UBaseType_t uxCoreAffinityMask;
        traceENTER_vTaskCoreAffinityGet( xTask );
        portBASE_TYPE_ENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            uxCoreAffinityMask = pxTCB->uxCoreAffinityMask;
        }
        portBASE_TYPE_EXIT_CRITICAL();
        traceRETURN_vTaskCoreAffinityGet( uxCoreAffinityMask );
        return uxCoreAffinityMask;
    }
#endif 
#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
    void vTaskPreemptionDisable( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskPreemptionDisable( xTask );
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            pxTCB->xPreemptionDisable = pdTRUE;
        }
        taskEXIT_CRITICAL();
        traceRETURN_vTaskPreemptionDisable();
    }
#endif 
#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
    void vTaskPreemptionEnable( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        BaseType_t xCoreID;
        traceENTER_vTaskPreemptionEnable( xTask );
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            pxTCB->xPreemptionDisable = pdFALSE;
            if( xSchedulerRunning != pdFALSE )
            {
                if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    xCoreID = ( BaseType_t ) pxTCB->xTaskRunState;
                    prvYieldCore( xCoreID );
                }
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_vTaskPreemptionEnable();
    }
#endif 
#if ( INCLUDE_vTaskSuspend == 1 )
    void vTaskSuspend( TaskHandle_t xTaskToSuspend )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskSuspend( xTaskToSuspend );
        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTaskToSuspend );
            configASSERT( pxTCB != NULL );
            traceTASK_SUSPEND( pxTCB );
            if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
            {
                taskRESET_READY_PRIORITY( pxTCB->uxPriority );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
            {
                ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            vListInsertEnd( &xSuspendedTaskList, &( pxTCB->xStateListItem ) );
            #if ( configUSE_TASK_NOTIFICATIONS == 1 )
            {
                BaseType_t x;
                for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                {
                    if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                    {
                        pxTCB->ucNotifyState[ x ] = taskNOT_WAITING_NOTIFICATION;
                    }
                }
            }
            #endif 
            #if ( configNUMBER_OF_CORES > 1 )
            {
                if( xSchedulerRunning != pdFALSE )
                {
                    prvResetNextTaskUnblockTime();
                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        if( pxTCB->xTaskRunState == ( BaseType_t ) portGET_CORE_ID() )
                        {
                            configASSERT( uxSchedulerSuspended == 0 );
                            vTaskYieldWithinAPI();
                        }
                        else
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #endif 
        }
        taskEXIT_CRITICAL();
        #if ( configNUMBER_OF_CORES == 1 )
        {
            UBaseType_t uxCurrentListLength;
            if( xSchedulerRunning != pdFALSE )
            {
                taskENTER_CRITICAL();
                {
                    prvResetNextTaskUnblockTime();
                }
                taskEXIT_CRITICAL();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            if( pxTCB == pxCurrentTCB )
            {
                if( xSchedulerRunning != pdFALSE )
                {
                    configASSERT( uxSchedulerSuspended == 0 );
                    portYIELD_WITHIN_API();
                }
                else
                {
                    uxCurrentListLength = listCURRENT_LIST_LENGTH( &xSuspendedTaskList );
                    if( uxCurrentListLength == uxCurrentNumberOfTasks )
                    {
                        pxCurrentTCB = NULL;
                    }
                    else
                    {
                        vTaskSwitchContext();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif 
        traceRETURN_vTaskSuspend();
    }
#endif 
#if ( INCLUDE_vTaskSuspend == 1 )
    static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask )
    {
        BaseType_t xReturn = pdFALSE;
        const TCB_t * const pxTCB = xTask;
        configASSERT( xTask );
        if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList, &( pxTCB->xStateListItem ) ) != pdFALSE )
        {
            if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) == pdFALSE )
            {
                if( listIS_CONTAINED_WITHIN( NULL, &( pxTCB->xEventListItem ) ) != pdFALSE )
                {
                    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                    {
                        BaseType_t x;
                        xReturn = pdTRUE;
                        for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                        {
                            if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                            {
                                xReturn = pdFALSE;
                                break;
                            }
                        }
                    }
                    #else 
                    {
                        xReturn = pdTRUE;
                    }
                    #endif 
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        return xReturn;
    }
#endif 
#if ( INCLUDE_vTaskSuspend == 1 )
    void vTaskResume( TaskHandle_t xTaskToResume )
    {
        TCB_t * const pxTCB = xTaskToResume;
        traceENTER_vTaskResume( xTaskToResume );
        configASSERT( xTaskToResume );
        #if ( configNUMBER_OF_CORES == 1 )
            if( ( pxTCB != pxCurrentTCB ) && ( pxTCB != NULL ) )
        #else
            if( pxTCB != NULL )
        #endif
        {
            taskENTER_CRITICAL();
            {
                if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
                {
                    traceTASK_RESUME( pxTCB );
                    ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );
                    taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            taskEXIT_CRITICAL();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskResume();
    }
#endif 
#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )
    BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
    {
        BaseType_t xYieldRequired = pdFALSE;
        TCB_t * const pxTCB = xTaskToResume;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_xTaskResumeFromISR( xTaskToResume );
        configASSERT( xTaskToResume );
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        {
            if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
            {
                traceTASK_RESUME_FROM_ISR( pxTCB );
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                        {
                            xYieldRequired = pdTRUE;
                            xYieldPendings[ 0 ] = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #endif 
                    ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );
                }
                else
                {
                    vListInsertEnd( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }
                #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PREEMPTION == 1 ) )
                {
                    prvYieldForTask( pxTCB );
                    if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
                #endif 
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_xTaskResumeFromISR( xYieldRequired );
        return xYieldRequired;
    }
#endif 
static BaseType_t prvCreateIdleTasks( void )
{
    BaseType_t xReturn = pdPASS;
    BaseType_t xCoreID;
    char cIdleName[ configMAX_TASK_NAME_LEN ] = { 0 };
    TaskFunction_t pxIdleTaskFunction = NULL;
    UBaseType_t xIdleTaskNameIndex;
    for( xIdleTaskNameIndex = 0U; xIdleTaskNameIndex < ( configMAX_TASK_NAME_LEN - taskRESERVED_TASK_NAME_LENGTH ); xIdleTaskNameIndex++ )
    {
        cIdleName[ xIdleTaskNameIndex ] = configIDLE_TASK_NAME[ xIdleTaskNameIndex ];
        if( cIdleName[ xIdleTaskNameIndex ] == ( char ) 0x00 )
        {
            break;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    cIdleName[ xIdleTaskNameIndex ] = '\0';
    for( xCoreID = ( BaseType_t ) 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
    {
        #if ( configNUMBER_OF_CORES == 1 )
        {
            pxIdleTaskFunction = &prvIdleTask;
        }
        #else 
        {
            if( xCoreID == 0 )
            {
                pxIdleTaskFunction = &prvIdleTask;
            }
            else
            {
                pxIdleTaskFunction = &prvPassiveIdleTask;
            }
        }
        #endif 
        #if ( configNUMBER_OF_CORES > 1 )
        {
            cIdleName[ xIdleTaskNameIndex ] = ( char ) ( xCoreID + '0' );
            cIdleName[ xIdleTaskNameIndex + 1U ] = '\0';
        }
        #endif 
        #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
        {
            StaticTask_t * pxIdleTaskTCBBuffer = NULL;
            StackType_t * pxIdleTaskStackBuffer = NULL;
            configSTACK_DEPTH_TYPE uxIdleTaskStackSize;
            #if ( configNUMBER_OF_CORES == 1 )
            {
                vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize );
            }
            #else
            {
                if( xCoreID == 0 )
                {
                    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize );
                }
                else
                {
                    vApplicationGetPassiveIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize, ( BaseType_t ) ( xCoreID - 1 ) );
                }
            }
            #endif 
            xIdleTaskHandles[ xCoreID ] = xTaskCreateStatic( pxIdleTaskFunction,
                                                             cIdleName,
                                                             uxIdleTaskStackSize,
                                                             ( void * ) NULL,
                                                             portPRIVILEGE_BIT, 
                                                             pxIdleTaskStackBuffer,
                                                             pxIdleTaskTCBBuffer );
            if( xIdleTaskHandles[ xCoreID ] != NULL )
            {
                xReturn = pdPASS;
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        #else 
        {
            xReturn = xTaskCreate( pxIdleTaskFunction,
                                   cIdleName,
                                   configMINIMAL_STACK_SIZE,
                                   ( void * ) NULL,
                                   portPRIVILEGE_BIT, 
                                   &xIdleTaskHandles[ xCoreID ] );
        }
        #endif 
        if( xReturn != pdPASS )
        {
            break;
        }
        else
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                mtCOVERAGE_TEST_MARKER();
            }
            #else
            {
                xIdleTaskHandles[ xCoreID ]->xTaskRunState = xCoreID;
                pxCurrentTCBs[ xCoreID ] = xIdleTaskHandles[ xCoreID ];
            }
            #endif
        }
    }
    return xReturn;
}
void vTaskStartScheduler( void )
{
    BaseType_t xReturn;
    traceENTER_vTaskStartScheduler();
    #if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
    {
        configASSERT( ( sizeof( UBaseType_t ) * taskBITS_PER_BYTE ) >= configNUMBER_OF_CORES );
    }
    #endif 
    xReturn = prvCreateIdleTasks();
    #if ( configUSE_TIMERS == 1 )
    {
        if( xReturn == pdPASS )
        {
            xReturn = xTimerCreateTimerTask();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif 
    if( xReturn == pdPASS )
    {
        #ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
        {
            freertos_tasks_c_additions_init();
        }
        #endif
        portDISABLE_INTERRUPTS();
        #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        {
            configSET_TLS_BLOCK( pxCurrentTCB->xTLSBlock );
        }
        #endif
        xNextTaskUnblockTime = portMAX_DELAY;
        xSchedulerRunning = pdTRUE;
        xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
        portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();
        traceTASK_SWITCHED_IN();
        traceSTARTING_SCHEDULER( xIdleTaskHandles );
        ( void ) xPortStartScheduler();
    }
    else
    {
        configASSERT( xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY );
    }
    ( void ) xIdleTaskHandles;
    ( void ) uxTopUsedPriority;
    traceRETURN_vTaskStartScheduler();
}
void vTaskEndScheduler( void )
{
    traceENTER_vTaskEndScheduler();
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        BaseType_t xCoreID;
        #if ( configUSE_TIMERS == 1 )
        {
            vTaskDelete( xTimerGetTimerDaemonTaskHandle() );
        }
        #endif 
        for( xCoreID = 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
        {
            vTaskDelete( xIdleTaskHandles[ xCoreID ] );
        }
        prvCheckTasksWaitingTermination();
    }
    #endif 
    portDISABLE_INTERRUPTS();
    xSchedulerRunning = pdFALSE;
    vPortEndScheduler();
    traceRETURN_vTaskEndScheduler();
}
void vTaskSuspendAll( void )
{
    traceENTER_vTaskSuspendAll();
    #if ( configNUMBER_OF_CORES == 1 )
    {
        portSOFTWARE_BARRIER();
        uxSchedulerSuspended = ( UBaseType_t ) ( uxSchedulerSuspended + 1U );
        portMEMORY_BARRIER();
    }
    #else 
    {
        UBaseType_t ulState;
        BaseType_t xCoreID;
        portASSERT_IF_IN_ISR();
        if( xSchedulerRunning != pdFALSE )
        {
            ulState = portSET_INTERRUPT_MASK();
            xCoreID = ( BaseType_t ) portGET_CORE_ID();
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0 );
            portSOFTWARE_BARRIER();
            portGET_TASK_LOCK( xCoreID );
            if( uxSchedulerSuspended == 0U )
            {
                prvCheckForRunStateChange();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            xCoreID = ( BaseType_t ) portGET_CORE_ID();
            portGET_ISR_LOCK( xCoreID );
            ++uxSchedulerSuspended;
            portRELEASE_ISR_LOCK( xCoreID );
            portCLEAR_INTERRUPT_MASK( ulState );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif 
    traceRETURN_vTaskSuspendAll();
}
#if ( configUSE_TICKLESS_IDLE != 0 )
    static TickType_t prvGetExpectedIdleTime( void )
    {
        TickType_t xReturn;
        BaseType_t xHigherPriorityReadyTasks = pdFALSE;
        #if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
        {
            if( uxTopReadyPriority > tskIDLE_PRIORITY )
            {
                xHigherPriorityReadyTasks = pdTRUE;
            }
        }
        #else
        {
            const UBaseType_t uxLeastSignificantBit = ( UBaseType_t ) 0x01;
            if( uxTopReadyPriority > uxLeastSignificantBit )
            {
                xHigherPriorityReadyTasks = pdTRUE;
            }
        }
        #endif 
        if( pxCurrentTCB->uxPriority > tskIDLE_PRIORITY )
        {
            xReturn = 0;
        }
        else if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > 1U )
        {
            xReturn = 0;
        }
        else if( xHigherPriorityReadyTasks != pdFALSE )
        {
            xReturn = 0;
        }
        else
        {
            xReturn = xNextTaskUnblockTime;
            xReturn -= xTickCount;
        }
        return xReturn;
    }
#endif 
BaseType_t xTaskResumeAll( void )
{
    TCB_t * pxTCB = NULL;
    BaseType_t xAlreadyYielded = pdFALSE;
    traceENTER_xTaskResumeAll();
    #if ( configNUMBER_OF_CORES > 1 )
        if( xSchedulerRunning != pdFALSE )
    #endif
    {
        taskENTER_CRITICAL();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
            configASSERT( uxSchedulerSuspended != 0U );
            uxSchedulerSuspended = ( UBaseType_t ) ( uxSchedulerSuspended - 1U );
            portRELEASE_TASK_LOCK( xCoreID );
            if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
            {
                if( uxCurrentNumberOfTasks > ( UBaseType_t ) 0U )
                {
                    while( listLIST_IS_EMPTY( &xPendingReadyList ) == pdFALSE )
                    {
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xPendingReadyList ) );
                        listREMOVE_ITEM( &( pxTCB->xEventListItem ) );
                        portMEMORY_BARRIER();
                        listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                        prvAddTaskToReadyList( pxTCB );
                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                            {
                                xYieldPendings[ xCoreID ] = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        #else 
                        {
                        }
                        #endif 
                    }
                    if( pxTCB != NULL )
                    {
                        prvResetNextTaskUnblockTime();
                    }
                    {
                        TickType_t xPendedCounts = xPendedTicks; 
                        if( xPendedCounts > ( TickType_t ) 0U )
                        {
                            do
                            {
                                if( xTaskIncrementTick() != pdFALSE )
                                {
                                    xYieldPendings[ xCoreID ] = pdTRUE;
                                }
                                else
                                {
                                    mtCOVERAGE_TEST_MARKER();
                                }
                                --xPendedCounts;
                            } while( xPendedCounts > ( TickType_t ) 0U );
                            xPendedTicks = 0;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    if( xYieldPendings[ xCoreID ] != pdFALSE )
                    {
                        #if ( configUSE_PREEMPTION != 0 )
                        {
                            xAlreadyYielded = pdTRUE;
                        }
                        #endif 
                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxCurrentTCB );
                        }
                        #endif 
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
    }
    traceRETURN_xTaskResumeAll( xAlreadyYielded );
    return xAlreadyYielded;
}
TickType_t xTaskGetTickCount( void )
{
    TickType_t xTicks;
    traceENTER_xTaskGetTickCount();
    portTICK_TYPE_ENTER_CRITICAL();
    {
        xTicks = xTickCount;
    }
    portTICK_TYPE_EXIT_CRITICAL();
    traceRETURN_xTaskGetTickCount( xTicks );
    return xTicks;
}
TickType_t xTaskGetTickCountFromISR( void )
{
    TickType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    traceENTER_xTaskGetTickCountFromISR();
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
    uxSavedInterruptStatus = portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
    {
        xReturn = xTickCount;
    }
    portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
    traceRETURN_xTaskGetTickCountFromISR( xReturn );
    return xReturn;
}
UBaseType_t uxTaskGetNumberOfTasks( void )
{
    traceENTER_uxTaskGetNumberOfTasks();
    traceRETURN_uxTaskGetNumberOfTasks( uxCurrentNumberOfTasks );
    return uxCurrentNumberOfTasks;
}
char * pcTaskGetName( TaskHandle_t xTaskToQuery )
{
    TCB_t * pxTCB;
    traceENTER_pcTaskGetName( xTaskToQuery );
    pxTCB = prvGetTCBFromHandle( xTaskToQuery );
    configASSERT( pxTCB != NULL );
    traceRETURN_pcTaskGetName( &( pxTCB->pcTaskName[ 0 ] ) );
    return &( pxTCB->pcTaskName[ 0 ] );
}
#if ( INCLUDE_xTaskGetHandle == 1 )
    static TCB_t * prvSearchForNameWithinSingleList( List_t * pxList,
                                                     const char pcNameToQuery[] )
    {
        TCB_t * pxReturn = NULL;
        TCB_t * pxTCB = NULL;
        UBaseType_t x;
        char cNextChar;
        BaseType_t xBreakLoop;
        const ListItem_t * pxEndMarker = listGET_END_MARKER( pxList );
        ListItem_t * pxIterator;
        if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
        {
            for( pxIterator = listGET_HEAD_ENTRY( pxList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
            {
                pxTCB = listGET_LIST_ITEM_OWNER( pxIterator );
                xBreakLoop = pdFALSE;
                for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
                {
                    cNextChar = pxTCB->pcTaskName[ x ];
                    if( cNextChar != pcNameToQuery[ x ] )
                    {
                        xBreakLoop = pdTRUE;
                    }
                    else if( cNextChar == ( char ) 0x00 )
                    {
                        pxReturn = pxTCB;
                        xBreakLoop = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    if( xBreakLoop != pdFALSE )
                    {
                        break;
                    }
                }
                if( pxReturn != NULL )
                {
                    break;
                }
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        return pxReturn;
    }
#endif 
#if ( INCLUDE_xTaskGetHandle == 1 )
    TaskHandle_t xTaskGetHandle( const char * pcNameToQuery )
    {
        UBaseType_t uxQueue = configMAX_PRIORITIES;
        TCB_t * pxTCB;
        traceENTER_xTaskGetHandle( pcNameToQuery );
        configASSERT( strlen( pcNameToQuery ) < configMAX_TASK_NAME_LEN );
        vTaskSuspendAll();
        {
            do
            {
                uxQueue--;
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) &( pxReadyTasksLists[ uxQueue ] ), pcNameToQuery );
                if( pxTCB != NULL )
                {
                    break;
                }
            } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );
            if( pxTCB == NULL )
            {
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxDelayedTaskList, pcNameToQuery );
            }
            if( pxTCB == NULL )
            {
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxOverflowDelayedTaskList, pcNameToQuery );
            }
            #if ( INCLUDE_vTaskSuspend == 1 )
            {
                if( pxTCB == NULL )
                {
                    pxTCB = prvSearchForNameWithinSingleList( &xSuspendedTaskList, pcNameToQuery );
                }
            }
            #endif
            #if ( INCLUDE_vTaskDelete == 1 )
            {
                if( pxTCB == NULL )
                {
                    pxTCB = prvSearchForNameWithinSingleList( &xTasksWaitingTermination, pcNameToQuery );
                }
            }
            #endif
        }
        ( void ) xTaskResumeAll();
        traceRETURN_xTaskGetHandle( pxTCB );
        return pxTCB;
    }
#endif 
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xTaskGetStaticBuffers( TaskHandle_t xTask,
                                      StackType_t ** ppuxStackBuffer,
                                      StaticTask_t ** ppxTaskBuffer )
    {
        BaseType_t xReturn;
        TCB_t * pxTCB;
        traceENTER_xTaskGetStaticBuffers( xTask, ppuxStackBuffer, ppxTaskBuffer );
        configASSERT( ppuxStackBuffer != NULL );
        configASSERT( ppxTaskBuffer != NULL );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 )
        {
            if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB )
            {
                *ppuxStackBuffer = pxTCB->pxStack;
                *ppxTaskBuffer = ( StaticTask_t * ) pxTCB;
                xReturn = pdTRUE;
            }
            else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
            {
                *ppuxStackBuffer = pxTCB->pxStack;
                *ppxTaskBuffer = NULL;
                xReturn = pdTRUE;
            }
            else
            {
                xReturn = pdFALSE;
            }
        }
        #else 
        {
            *ppuxStackBuffer = pxTCB->pxStack;
            *ppxTaskBuffer = ( StaticTask_t * ) pxTCB;
            xReturn = pdTRUE;
        }
        #endif 
        traceRETURN_xTaskGetStaticBuffers( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime )
    {
        UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;
        traceENTER_uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, pulTotalRunTime );
        vTaskSuspendAll();
        {
            if( uxArraySize >= uxCurrentNumberOfTasks )
            {
                do
                {
                    uxQueue--;
                    uxTask = ( UBaseType_t ) ( uxTask + prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &( pxReadyTasksLists[ uxQueue ] ), eReady ) );
                } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );
                uxTask = ( UBaseType_t ) ( uxTask + prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxDelayedTaskList, eBlocked ) );
                uxTask = ( UBaseType_t ) ( uxTask + prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxOverflowDelayedTaskList, eBlocked ) );
                #if ( INCLUDE_vTaskDelete == 1 )
                {
                    uxTask = ( UBaseType_t ) ( uxTask + prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xTasksWaitingTermination, eDeleted ) );
                }
                #endif
                #if ( INCLUDE_vTaskSuspend == 1 )
                {
                    uxTask = ( UBaseType_t ) ( uxTask + prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xSuspendedTaskList, eSuspended ) );
                }
                #endif
                #if ( configGENERATE_RUN_TIME_STATS == 1 )
                {
                    if( pulTotalRunTime != NULL )
                    {
                        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                            portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
                        #else
                            *pulTotalRunTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
                        #endif
                    }
                }
                #else 
                {
                    if( pulTotalRunTime != NULL )
                    {
                        *pulTotalRunTime = 0;
                    }
                }
                #endif 
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        ( void ) xTaskResumeAll();
        traceRETURN_uxTaskGetSystemState( uxTask );
        return uxTask;
    }
#endif 
#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
    #if ( configNUMBER_OF_CORES == 1 )
        TaskHandle_t xTaskGetIdleTaskHandle( void )
        {
            traceENTER_xTaskGetIdleTaskHandle();
            configASSERT( ( xIdleTaskHandles[ 0 ] != NULL ) );
            traceRETURN_xTaskGetIdleTaskHandle( xIdleTaskHandles[ 0 ] );
            return xIdleTaskHandles[ 0 ];
        }
    #endif 
    TaskHandle_t xTaskGetIdleTaskHandleForCore( BaseType_t xCoreID )
    {
        traceENTER_xTaskGetIdleTaskHandleForCore( xCoreID );
        configASSERT( taskVALID_CORE_ID( xCoreID ) == pdTRUE );
        configASSERT( ( xIdleTaskHandles[ xCoreID ] != NULL ) );
        traceRETURN_xTaskGetIdleTaskHandleForCore( xIdleTaskHandles[ xCoreID ] );
        return xIdleTaskHandles[ xCoreID ];
    }
#endif 
#if ( configUSE_TICKLESS_IDLE != 0 )
    void vTaskStepTick( TickType_t xTicksToJump )
    {
        TickType_t xUpdatedTickCount;
        traceENTER_vTaskStepTick( xTicksToJump );
        xUpdatedTickCount = xTickCount + xTicksToJump;
        configASSERT( xUpdatedTickCount <= xNextTaskUnblockTime );
        if( xUpdatedTickCount == xNextTaskUnblockTime )
        {
            configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );
            configASSERT( xTicksToJump != ( TickType_t ) 0 );
            taskENTER_CRITICAL();
            {
                xPendedTicks++;
            }
            taskEXIT_CRITICAL();
            xTicksToJump--;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        xTickCount += xTicksToJump;
        traceINCREASE_TICK_COUNT( xTicksToJump );
        traceRETURN_vTaskStepTick();
    }
#endif 
BaseType_t xTaskCatchUpTicks( TickType_t xTicksToCatchUp )
{
    BaseType_t xYieldOccurred;
    traceENTER_xTaskCatchUpTicks( xTicksToCatchUp );
    configASSERT( uxSchedulerSuspended == ( UBaseType_t ) 0U );
    vTaskSuspendAll();
    taskENTER_CRITICAL();
    {
        xPendedTicks += xTicksToCatchUp;
    }
    taskEXIT_CRITICAL();
    xYieldOccurred = xTaskResumeAll();
    traceRETURN_xTaskCatchUpTicks( xYieldOccurred );
    return xYieldOccurred;
}
#if ( INCLUDE_xTaskAbortDelay == 1 )
    BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
    {
        TCB_t * pxTCB = xTask;
        BaseType_t xReturn;
        traceENTER_xTaskAbortDelay( xTask );
        configASSERT( pxTCB != NULL );
        vTaskSuspendAll();
        {
            if( eTaskGetState( xTask ) == eBlocked )
            {
                xReturn = pdPASS;
                ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                taskENTER_CRITICAL();
                {
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
                        pxTCB->ucDelayAborted = ( uint8_t ) pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();
                prvAddTaskToReadyList( pxTCB );
                #if ( configUSE_PREEMPTION == 1 )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                        {
                            xYieldPendings[ 0 ] = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #else 
                    {
                        taskENTER_CRITICAL();
                        {
                            prvYieldForTask( pxTCB );
                        }
                        taskEXIT_CRITICAL();
                    }
                    #endif 
                }
                #endif 
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        ( void ) xTaskResumeAll();
        traceRETURN_xTaskAbortDelay( xReturn );
        return xReturn;
    }
#endif 
BaseType_t xTaskIncrementTick( void )
{
    TCB_t * pxTCB;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;
    traceENTER_xTaskIncrementTick();
    traceTASK_INCREMENT_TICK( xTickCount );
    if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
    {
        const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;
        xTickCount = xConstTickCount;
        if( xConstTickCount == ( TickType_t ) 0U )
        {
            taskSWITCH_DELAYED_LISTS();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        if( xConstTickCount >= xNextTaskUnblockTime )
        {
            for( ; ; )
            {
                if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
                {
                    xNextTaskUnblockTime = portMAX_DELAY;
                    break;
                }
                else
                {
                    pxTCB = listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
                    xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );
                    if( xConstTickCount < xItemValue )
                    {
                        xNextTaskUnblockTime = xItemValue;
                        break;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        listREMOVE_ITEM( &( pxTCB->xEventListItem ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    prvAddTaskToReadyList( pxTCB );
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                            {
                                xSwitchRequired = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        #else 
                        {
                            prvYieldForTask( pxTCB );
                        }
                        #endif 
                    }
                    #endif 
                }
            }
        }
        #if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > 1U )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #else 
            {
                BaseType_t xCoreID;
                for( xCoreID = 0; xCoreID < ( ( BaseType_t ) configNUMBER_OF_CORES ); xCoreID++ )
                {
                    if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ) ) > 1U )
                    {
                        xYieldPendings[ xCoreID ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
            }
            #endif 
        }
        #endif 
        #if ( configUSE_TICK_HOOK == 1 )
        {
            if( xPendedTicks == ( TickType_t ) 0 )
            {
                vApplicationTickHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif 
        #if ( configUSE_PREEMPTION == 1 )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                if( xYieldPendings[ 0 ] != pdFALSE )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #else 
            {
                BaseType_t xCoreID, xCurrentCoreID;
                xCurrentCoreID = ( BaseType_t ) portGET_CORE_ID();
                for( xCoreID = 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
                {
                    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                        if( pxCurrentTCBs[ xCoreID ]->xPreemptionDisable == pdFALSE )
                    #endif
                    {
                        if( xYieldPendings[ xCoreID ] != pdFALSE )
                        {
                            if( xCoreID == xCurrentCoreID )
                            {
                                xSwitchRequired = pdTRUE;
                            }
                            else
                            {
                                prvYieldCore( xCoreID );
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
            }
            #endif 
        }
        #endif 
    }
    else
    {
        xPendedTicks += 1U;
        #if ( configUSE_TICK_HOOK == 1 )
        {
            vApplicationTickHook();
        }
        #endif
    }
    traceRETURN_xTaskIncrementTick( xSwitchRequired );
    return xSwitchRequired;
}
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                     TaskHookFunction_t pxHookFunction )
    {
        TCB_t * xTCB;
        traceENTER_vTaskSetApplicationTaskTag( xTask, pxHookFunction );
        if( xTask == NULL )
        {
            xTCB = ( TCB_t * ) pxCurrentTCB;
        }
        else
        {
            xTCB = xTask;
        }
        taskENTER_CRITICAL();
        {
            xTCB->pxTaskTag = pxHookFunction;
        }
        taskEXIT_CRITICAL();
        traceRETURN_vTaskSetApplicationTaskTag();
    }
#endif 
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        TaskHookFunction_t xReturn;
        traceENTER_xTaskGetApplicationTaskTag( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        taskENTER_CRITICAL();
        {
            xReturn = pxTCB->pxTaskTag;
        }
        taskEXIT_CRITICAL();
        traceRETURN_xTaskGetApplicationTaskTag( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        TaskHookFunction_t xReturn;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_xTaskGetApplicationTaskTagFromISR( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        {
            xReturn = pxTCB->pxTaskTag;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_xTaskGetApplicationTaskTagFromISR( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                             void * pvParameter )
    {
        TCB_t * xTCB;
        BaseType_t xReturn;
        traceENTER_xTaskCallApplicationTaskHook( xTask, pvParameter );
        if( xTask == NULL )
        {
            xTCB = pxCurrentTCB;
        }
        else
        {
            xTCB = xTask;
        }
        if( xTCB->pxTaskTag != NULL )
        {
            xReturn = xTCB->pxTaskTag( pvParameter );
        }
        else
        {
            xReturn = pdFAIL;
        }
        traceRETURN_xTaskCallApplicationTaskHook( xReturn );
        return xReturn;
    }
#endif 
#if ( configNUMBER_OF_CORES == 1 )
    void vTaskSwitchContext( void )
    {
        traceENTER_vTaskSwitchContext();
        if( uxSchedulerSuspended != ( UBaseType_t ) 0U )
        {
            xYieldPendings[ 0 ] = pdTRUE;
        }
        else
        {
            xYieldPendings[ 0 ] = pdFALSE;
            traceTASK_SWITCHED_OUT();
            #if ( configGENERATE_RUN_TIME_STATS == 1 )
            {
                #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime[ 0 ] );
                #else
                    ulTotalRunTime[ 0 ] = portGET_RUN_TIME_COUNTER_VALUE();
                #endif
                if( ulTotalRunTime[ 0 ] > ulTaskSwitchedInTime[ 0 ] )
                {
                    pxCurrentTCB->ulRunTimeCounter += ( ulTotalRunTime[ 0 ] - ulTaskSwitchedInTime[ 0 ] );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
                ulTaskSwitchedInTime[ 0 ] = ulTotalRunTime[ 0 ];
            }
            #endif 
            taskCHECK_FOR_STACK_OVERFLOW();
            #if ( configUSE_POSIX_ERRNO == 1 )
            {
                pxCurrentTCB->iTaskErrno = FreeRTOS_errno;
            }
            #endif
            taskSELECT_HIGHEST_PRIORITY_TASK();
            traceTASK_SWITCHED_IN();
            portTASK_SWITCH_HOOK( pxCurrentTCB );
            #if ( configUSE_POSIX_ERRNO == 1 )
            {
                FreeRTOS_errno = pxCurrentTCB->iTaskErrno;
            }
            #endif
            #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
            {
                configSET_TLS_BLOCK( pxCurrentTCB->xTLSBlock );
            }
            #endif
        }
        traceRETURN_vTaskSwitchContext();
    }
#else 
    void vTaskSwitchContext( BaseType_t xCoreID )
    {
        traceENTER_vTaskSwitchContext();
        portGET_TASK_LOCK( xCoreID ); 
        portGET_ISR_LOCK( xCoreID );
        {
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0 );
            if( uxSchedulerSuspended != ( UBaseType_t ) 0U )
            {
                xYieldPendings[ xCoreID ] = pdTRUE;
            }
            else
            {
                xYieldPendings[ xCoreID ] = pdFALSE;
                traceTASK_SWITCHED_OUT();
                #if ( configGENERATE_RUN_TIME_STATS == 1 )
                {
                    #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                        portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime[ xCoreID ] );
                    #else
                        ulTotalRunTime[ xCoreID ] = portGET_RUN_TIME_COUNTER_VALUE();
                    #endif
                    if( ulTotalRunTime[ xCoreID ] > ulTaskSwitchedInTime[ xCoreID ] )
                    {
                        pxCurrentTCBs[ xCoreID ]->ulRunTimeCounter += ( ulTotalRunTime[ xCoreID ] - ulTaskSwitchedInTime[ xCoreID ] );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    ulTaskSwitchedInTime[ xCoreID ] = ulTotalRunTime[ xCoreID ];
                }
                #endif 
                taskCHECK_FOR_STACK_OVERFLOW();
                #if ( configUSE_POSIX_ERRNO == 1 )
                {
                    pxCurrentTCBs[ xCoreID ]->iTaskErrno = FreeRTOS_errno;
                }
                #endif
                taskSELECT_HIGHEST_PRIORITY_TASK( xCoreID );
                traceTASK_SWITCHED_IN();
                portTASK_SWITCH_HOOK( pxCurrentTCBs[ portGET_CORE_ID() ] );
                #if ( configUSE_POSIX_ERRNO == 1 )
                {
                    FreeRTOS_errno = pxCurrentTCBs[ xCoreID ]->iTaskErrno;
                }
                #endif
                #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
                {
                    configSET_TLS_BLOCK( pxCurrentTCBs[ xCoreID ]->xTLSBlock );
                }
                #endif
            }
        }
        portRELEASE_ISR_LOCK( xCoreID );
        portRELEASE_TASK_LOCK( xCoreID );
        traceRETURN_vTaskSwitchContext();
    }
#endif 
void vTaskPlaceOnEventList( List_t * const pxEventList,
                            const TickType_t xTicksToWait )
{
    traceENTER_vTaskPlaceOnEventList( pxEventList, xTicksToWait );
    configASSERT( pxEventList );
    vListInsert( pxEventList, &( pxCurrentTCB->xEventListItem ) );
    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
    traceRETURN_vTaskPlaceOnEventList();
}
void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                     const TickType_t xItemValue,
                                     const TickType_t xTicksToWait )
{
    traceENTER_vTaskPlaceOnUnorderedEventList( pxEventList, xItemValue, xTicksToWait );
    configASSERT( pxEventList );
    configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );
    listINSERT_END( pxEventList, &( pxCurrentTCB->xEventListItem ) );
    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
    traceRETURN_vTaskPlaceOnUnorderedEventList();
}
#if ( configUSE_TIMERS == 1 )
    void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                          TickType_t xTicksToWait,
                                          const BaseType_t xWaitIndefinitely )
    {
        traceENTER_vTaskPlaceOnEventListRestricted( pxEventList, xTicksToWait, xWaitIndefinitely );
        configASSERT( pxEventList );
        listINSERT_END( pxEventList, &( pxCurrentTCB->xEventListItem ) );
        if( xWaitIndefinitely != pdFALSE )
        {
            xTicksToWait = portMAX_DELAY;
        }
        traceTASK_DELAY_UNTIL( ( xTickCount + xTicksToWait ) );
        prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );
        traceRETURN_vTaskPlaceOnEventListRestricted();
    }
#endif 
BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
    TCB_t * pxUnblockedTCB;
    BaseType_t xReturn;
    traceENTER_xTaskRemoveFromEventList( pxEventList );
    pxUnblockedTCB = listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
    configASSERT( pxUnblockedTCB );
    listREMOVE_ITEM( &( pxUnblockedTCB->xEventListItem ) );
    if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
    {
        listREMOVE_ITEM( &( pxUnblockedTCB->xStateListItem ) );
        prvAddTaskToReadyList( pxUnblockedTCB );
        #if ( configUSE_TICKLESS_IDLE != 0 )
        {
            prvResetNextTaskUnblockTime();
        }
        #endif
    }
    else
    {
        listINSERT_END( &( xPendingReadyList ), &( pxUnblockedTCB->xEventListItem ) );
    }
    #if ( configNUMBER_OF_CORES == 1 )
    {
        if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
        {
            xReturn = pdTRUE;
            xYieldPendings[ 0 ] = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    #else 
    {
        xReturn = pdFALSE;
        #if ( configUSE_PREEMPTION == 1 )
        {
            prvYieldForTask( pxUnblockedTCB );
            if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
            {
                xReturn = pdTRUE;
            }
        }
        #endif 
    }
    #endif 
    traceRETURN_xTaskRemoveFromEventList( xReturn );
    return xReturn;
}
void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue )
{
    TCB_t * pxUnblockedTCB;
    traceENTER_vTaskRemoveFromUnorderedEventList( pxEventListItem, xItemValue );
    configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );
    listSET_LIST_ITEM_VALUE( pxEventListItem, xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );
    pxUnblockedTCB = listGET_LIST_ITEM_OWNER( pxEventListItem );
    configASSERT( pxUnblockedTCB );
    listREMOVE_ITEM( pxEventListItem );
    #if ( configUSE_TICKLESS_IDLE != 0 )
    {
        prvResetNextTaskUnblockTime();
    }
    #endif
    listREMOVE_ITEM( &( pxUnblockedTCB->xStateListItem ) );
    prvAddTaskToReadyList( pxUnblockedTCB );
    #if ( configNUMBER_OF_CORES == 1 )
    {
        if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
        {
            xYieldPendings[ 0 ] = pdTRUE;
        }
    }
    #else 
    {
        #if ( configUSE_PREEMPTION == 1 )
        {
            taskENTER_CRITICAL();
            {
                prvYieldForTask( pxUnblockedTCB );
            }
            taskEXIT_CRITICAL();
        }
        #endif
    }
    #endif 
    traceRETURN_vTaskRemoveFromUnorderedEventList();
}
void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    traceENTER_vTaskSetTimeOutState( pxTimeOut );
    configASSERT( pxTimeOut );
    taskENTER_CRITICAL();
    {
        pxTimeOut->xOverflowCount = xNumOfOverflows;
        pxTimeOut->xTimeOnEntering = xTickCount;
    }
    taskEXIT_CRITICAL();
    traceRETURN_vTaskSetTimeOutState();
}
void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    traceENTER_vTaskInternalSetTimeOutState( pxTimeOut );
    pxTimeOut->xOverflowCount = xNumOfOverflows;
    pxTimeOut->xTimeOnEntering = xTickCount;
    traceRETURN_vTaskInternalSetTimeOutState();
}
BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                 TickType_t * const pxTicksToWait )
{
    BaseType_t xReturn;
    traceENTER_xTaskCheckForTimeOut( pxTimeOut, pxTicksToWait );
    configASSERT( pxTimeOut );
    configASSERT( pxTicksToWait );
    taskENTER_CRITICAL();
    {
        const TickType_t xConstTickCount = xTickCount;
        const TickType_t xElapsedTime = xConstTickCount - pxTimeOut->xTimeOnEntering;
        #if ( INCLUDE_xTaskAbortDelay == 1 )
            if( pxCurrentTCB->ucDelayAborted != ( uint8_t ) pdFALSE )
            {
                pxCurrentTCB->ucDelayAborted = ( uint8_t ) pdFALSE;
                xReturn = pdTRUE;
            }
            else
        #endif
        #if ( INCLUDE_vTaskSuspend == 1 )
            if( *pxTicksToWait == portMAX_DELAY )
            {
                xReturn = pdFALSE;
            }
            else
        #endif
        if( ( xNumOfOverflows != pxTimeOut->xOverflowCount ) && ( xConstTickCount >= pxTimeOut->xTimeOnEntering ) )
        {
            xReturn = pdTRUE;
            *pxTicksToWait = ( TickType_t ) 0;
        }
        else if( xElapsedTime < *pxTicksToWait )
        {
            *pxTicksToWait -= xElapsedTime;
            vTaskInternalSetTimeOutState( pxTimeOut );
            xReturn = pdFALSE;
        }
        else
        {
            *pxTicksToWait = ( TickType_t ) 0;
            xReturn = pdTRUE;
        }
    }
    taskEXIT_CRITICAL();
    traceRETURN_xTaskCheckForTimeOut( xReturn );
    return xReturn;
}
void vTaskMissedYield( void )
{
    traceENTER_vTaskMissedYield();
    xYieldPendings[ portGET_CORE_ID() ] = pdTRUE;
    traceRETURN_vTaskMissedYield();
}
#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
    {
        UBaseType_t uxReturn;
        TCB_t const * pxTCB;
        traceENTER_uxTaskGetTaskNumber( xTask );
        if( xTask != NULL )
        {
            pxTCB = xTask;
            uxReturn = pxTCB->uxTaskNumber;
        }
        else
        {
            uxReturn = 0U;
        }
        traceRETURN_uxTaskGetTaskNumber( uxReturn );
        return uxReturn;
    }
#endif 
#if ( configUSE_TRACE_FACILITY == 1 )
    void vTaskSetTaskNumber( TaskHandle_t xTask,
                             const UBaseType_t uxHandle )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskSetTaskNumber( xTask, uxHandle );
        if( xTask != NULL )
        {
            pxTCB = xTask;
            pxTCB->uxTaskNumber = uxHandle;
        }
        traceRETURN_vTaskSetTaskNumber();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    static portTASK_FUNCTION( prvPassiveIdleTask, pvParameters )
    {
        ( void ) pvParameters;
        taskYIELD();
        for( ; configCONTROL_INFINITE_LOOP(); )
        {
            #if ( configUSE_PREEMPTION == 0 )
            {
                taskYIELD();
            }
            #endif 
            #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
            {
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) configNUMBER_OF_CORES )
                {
                    taskYIELD();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #endif 
            #if ( configUSE_PASSIVE_IDLE_HOOK == 1 )
            {
                vApplicationPassiveIdleHook();
            }
            #endif 
        }
    }
#endif 
static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
    ( void ) pvParameters;
    portALLOCATE_SECURE_CONTEXT( configMINIMAL_SECURE_STACK_SIZE );
    #if ( configNUMBER_OF_CORES > 1 )
    {
        taskYIELD();
    }
    #endif 
    for( ; configCONTROL_INFINITE_LOOP(); )
    {
        prvCheckTasksWaitingTermination();
        #if ( configUSE_PREEMPTION == 0 )
        {
            taskYIELD();
        }
        #endif 
        #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
        {
            if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) configNUMBER_OF_CORES )
            {
                taskYIELD();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif 
        #if ( configUSE_IDLE_HOOK == 1 )
        {
            vApplicationIdleHook();
        }
        #endif 
        #if ( configUSE_TICKLESS_IDLE != 0 )
        {
            TickType_t xExpectedIdleTime;
            xExpectedIdleTime = prvGetExpectedIdleTime();
            if( xExpectedIdleTime >= ( TickType_t ) configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
            {
                vTaskSuspendAll();
                {
                    configASSERT( xNextTaskUnblockTime >= xTickCount );
                    xExpectedIdleTime = prvGetExpectedIdleTime();
                    configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( xExpectedIdleTime );
                    if( xExpectedIdleTime >= ( TickType_t ) configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
                    {
                        traceLOW_POWER_IDLE_BEGIN();
                        portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
                        traceLOW_POWER_IDLE_END();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif 
        #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PASSIVE_IDLE_HOOK == 1 ) )
        {
            vApplicationPassiveIdleHook();
        }
        #endif 
    }
}
#if ( configUSE_TICKLESS_IDLE != 0 )
    eSleepModeStatus eTaskConfirmSleepModeStatus( void )
    {
        #if ( INCLUDE_vTaskSuspend == 1 )
            const UBaseType_t uxNonApplicationTasks = configNUMBER_OF_CORES;
        #endif 
        eSleepModeStatus eReturn = eStandardSleep;
        traceENTER_eTaskConfirmSleepModeStatus();
        if( listCURRENT_LIST_LENGTH( &xPendingReadyList ) != 0U )
        {
            eReturn = eAbortSleep;
        }
        else if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
        {
            eReturn = eAbortSleep;
        }
        else if( xPendedTicks != 0U )
        {
            eReturn = eAbortSleep;
        }
        #if ( INCLUDE_vTaskSuspend == 1 )
            else if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == ( uxCurrentNumberOfTasks - uxNonApplicationTasks ) )
            {
                eReturn = eNoTasksWaitingTimeout;
            }
        #endif 
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_eTaskConfirmSleepModeStatus( eReturn );
        return eReturn;
    }
#endif 
#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
    void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
                                            BaseType_t xIndex,
                                            void * pvValue )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskSetThreadLocalStoragePointer( xTaskToSet, xIndex, pvValue );
        if( ( xIndex >= 0 ) &&
            ( xIndex < ( BaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS ) )
        {
            pxTCB = prvGetTCBFromHandle( xTaskToSet );
            configASSERT( pxTCB != NULL );
            pxTCB->pvThreadLocalStoragePointers[ xIndex ] = pvValue;
        }
        traceRETURN_vTaskSetThreadLocalStoragePointer();
    }
#endif 
#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
    void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
                                               BaseType_t xIndex )
    {
        void * pvReturn = NULL;
        TCB_t * pxTCB;
        traceENTER_pvTaskGetThreadLocalStoragePointer( xTaskToQuery, xIndex );
        if( ( xIndex >= 0 ) &&
            ( xIndex < ( BaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS ) )
        {
            pxTCB = prvGetTCBFromHandle( xTaskToQuery );
            configASSERT( pxTCB != NULL );
            pvReturn = pxTCB->pvThreadLocalStoragePointers[ xIndex ];
        }
        else
        {
            pvReturn = NULL;
        }
        traceRETURN_pvTaskGetThreadLocalStoragePointer( pvReturn );
        return pvReturn;
    }
#endif 
#if ( portUSING_MPU_WRAPPERS == 1 )
    void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
                                  const MemoryRegion_t * const pxRegions )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskAllocateMPURegions( xTaskToModify, pxRegions );
        pxTCB = prvGetTCBFromHandle( xTaskToModify );
        configASSERT( pxTCB != NULL );
        vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), pxRegions, NULL, 0 );
        traceRETURN_vTaskAllocateMPURegions();
    }
#endif 
static void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;
    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }
    vListInitialise( &xDelayedTaskList1 );
    vListInitialise( &xDelayedTaskList2 );
    vListInitialise( &xPendingReadyList );
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        vListInitialise( &xTasksWaitingTermination );
    }
    #endif 
    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        vListInitialise( &xSuspendedTaskList );
    }
    #endif 
    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}
static void prvCheckTasksWaitingTermination( void )
{
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        TCB_t * pxTCB;
        while( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                taskENTER_CRITICAL();
                {
                    {
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );
                        ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                        --uxCurrentNumberOfTasks;
                        --uxDeletedTasksWaitingCleanUp;
                    }
                }
                taskEXIT_CRITICAL();
                prvDeleteTCB( pxTCB );
            }
            #else 
            {
                pxTCB = NULL;
                taskENTER_CRITICAL();
                {
                    if( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
                    {
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );
                        if( pxTCB->xTaskRunState == taskTASK_NOT_RUNNING )
                        {
                            ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                            --uxCurrentNumberOfTasks;
                            --uxDeletedTasksWaitingCleanUp;
                        }
                        else
                        {
                            taskEXIT_CRITICAL();
                            break;
                        }
                    }
                }
                taskEXIT_CRITICAL();
                if( pxTCB != NULL )
                {
                    prvDeleteTCB( pxTCB );
                }
            }
            #endif 
        }
    }
    #endif 
}
#if ( configUSE_TRACE_FACILITY == 1 )
    void vTaskGetInfo( TaskHandle_t xTask,
                       TaskStatus_t * pxTaskStatus,
                       BaseType_t xGetFreeStackSpace,
                       eTaskState eState )
    {
        TCB_t * pxTCB;
        traceENTER_vTaskGetInfo( xTask, pxTaskStatus, xGetFreeStackSpace, eState );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        pxTaskStatus->xHandle = pxTCB;
        pxTaskStatus->pcTaskName = ( const char * ) &( pxTCB->pcTaskName[ 0 ] );
        pxTaskStatus->uxCurrentPriority = pxTCB->uxPriority;
        pxTaskStatus->pxStackBase = pxTCB->pxStack;
        #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
            pxTaskStatus->pxTopOfStack = ( StackType_t * ) pxTCB->pxTopOfStack;
            pxTaskStatus->pxEndOfStack = pxTCB->pxEndOfStack;
        #endif
        pxTaskStatus->xTaskNumber = pxTCB->uxTCBNumber;
        #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
        {
            pxTaskStatus->uxCoreAffinityMask = pxTCB->uxCoreAffinityMask;
        }
        #endif
        #if ( configUSE_MUTEXES == 1 )
        {
            pxTaskStatus->uxBasePriority = pxTCB->uxBasePriority;
        }
        #else
        {
            pxTaskStatus->uxBasePriority = 0;
        }
        #endif
        #if ( configGENERATE_RUN_TIME_STATS == 1 )
        {
            pxTaskStatus->ulRunTimeCounter = ulTaskGetRunTimeCounter( xTask );
        }
        #else
        {
            pxTaskStatus->ulRunTimeCounter = ( configRUN_TIME_COUNTER_TYPE ) 0;
        }
        #endif
        if( eState != eInvalid )
        {
            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
            {
                pxTaskStatus->eCurrentState = eRunning;
            }
            else
            {
                pxTaskStatus->eCurrentState = eState;
                #if ( INCLUDE_vTaskSuspend == 1 )
                {
                    if( eState == eSuspended )
                    {
                        vTaskSuspendAll();
                        {
                            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                            {
                                pxTaskStatus->eCurrentState = eBlocked;
                            }
                            else
                            {
                                #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                                {
                                    BaseType_t x;
                                    for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                                    {
                                        if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                                        {
                                            pxTaskStatus->eCurrentState = eBlocked;
                                            break;
                                        }
                                    }
                                }
                                #endif 
                            }
                        }
                        ( void ) xTaskResumeAll();
                    }
                }
                #endif 
                taskENTER_CRITICAL();
                {
                    if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) != pdFALSE )
                    {
                        pxTaskStatus->eCurrentState = eReady;
                    }
                }
                taskEXIT_CRITICAL();
            }
        }
        else
        {
            pxTaskStatus->eCurrentState = eTaskGetState( pxTCB );
        }
        if( xGetFreeStackSpace != pdFALSE )
        {
            #if ( portSTACK_GROWTH > 0 )
            {
                pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxEndOfStack );
            }
            #else
            {
                pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxStack );
            }
            #endif
        }
        else
        {
            pxTaskStatus->usStackHighWaterMark = 0;
        }
        traceRETURN_vTaskGetInfo();
    }
#endif 
#if ( configUSE_TRACE_FACILITY == 1 )
    static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t * pxTaskStatusArray,
                                                     List_t * pxList,
                                                     eTaskState eState )
    {
        UBaseType_t uxTask = 0;
        const ListItem_t * pxEndMarker = listGET_END_MARKER( pxList );
        ListItem_t * pxIterator;
        TCB_t * pxTCB = NULL;
        if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
        {
            for( pxIterator = listGET_HEAD_ENTRY( pxList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
            {
                pxTCB = listGET_LIST_ITEM_OWNER( pxIterator );
                vTaskGetInfo( ( TaskHandle_t ) pxTCB, &( pxTaskStatusArray[ uxTask ] ), pdTRUE, eState );
                uxTask++;
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        return uxTask;
    }
#endif 
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )
    static configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte )
    {
        configSTACK_DEPTH_TYPE uxCount = 0U;
        while( *pucStackByte == ( uint8_t ) tskSTACK_FILL_BYTE )
        {
            pucStackByte -= portSTACK_GROWTH;
            uxCount++;
        }
        uxCount /= ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t );
        return uxCount;
    }
#endif 
#if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )
    configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        uint8_t * pucEndOfStack;
        configSTACK_DEPTH_TYPE uxReturn;
        traceENTER_uxTaskGetStackHighWaterMark2( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        #if portSTACK_GROWTH < 0
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
        }
        #else
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
        }
        #endif
        uxReturn = prvTaskCheckFreeStackSpace( pucEndOfStack );
        traceRETURN_uxTaskGetStackHighWaterMark2( uxReturn );
        return uxReturn;
    }
#endif 
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
    UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        uint8_t * pucEndOfStack;
        UBaseType_t uxReturn;
        traceENTER_uxTaskGetStackHighWaterMark( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        #if portSTACK_GROWTH < 0
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
        }
        #else
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
        }
        #endif
        uxReturn = ( UBaseType_t ) prvTaskCheckFreeStackSpace( pucEndOfStack );
        traceRETURN_uxTaskGetStackHighWaterMark( uxReturn );
        return uxReturn;
    }
#endif 
#if ( INCLUDE_vTaskDelete == 1 )
    static void prvDeleteTCB( TCB_t * pxTCB )
    {
        portCLEAN_UP_TCB( pxTCB );
        #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        {
            configDEINIT_TLS_BLOCK( pxTCB->xTLSBlock );
        }
        #endif
        #if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
        {
            vPortFreeStack( pxTCB->pxStack );
            vPortFree( pxTCB );
        }
        #elif ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        {
            if( pxTCB->ucStaticallyAllocated == tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB )
            {
                vPortFreeStack( pxTCB->pxStack );
                vPortFree( pxTCB );
            }
            else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
            {
                vPortFree( pxTCB );
            }
            else
            {
                configASSERT( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB );
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif 
    }
#endif 
static void prvResetNextTaskUnblockTime( void )
{
    if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
    {
        xNextTaskUnblockTime = portMAX_DELAY;
    }
    else
    {
        xNextTaskUnblockTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
    }
}
#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) ) || ( configNUMBER_OF_CORES > 1 )
    #if ( configNUMBER_OF_CORES == 1 )
        TaskHandle_t xTaskGetCurrentTaskHandle( void )
        {
            TaskHandle_t xReturn;
            traceENTER_xTaskGetCurrentTaskHandle();
            xReturn = pxCurrentTCB;
            traceRETURN_xTaskGetCurrentTaskHandle( xReturn );
            return xReturn;
        }
    #else 
        TaskHandle_t xTaskGetCurrentTaskHandle( void )
        {
            TaskHandle_t xReturn;
            UBaseType_t uxSavedInterruptStatus;
            traceENTER_xTaskGetCurrentTaskHandle();
            uxSavedInterruptStatus = portSET_INTERRUPT_MASK();
            {
                xReturn = pxCurrentTCBs[ portGET_CORE_ID() ];
            }
            portCLEAR_INTERRUPT_MASK( uxSavedInterruptStatus );
            traceRETURN_xTaskGetCurrentTaskHandle( xReturn );
            return xReturn;
        }
    #endif 
    TaskHandle_t xTaskGetCurrentTaskHandleForCore( BaseType_t xCoreID )
    {
        TaskHandle_t xReturn = NULL;
        traceENTER_xTaskGetCurrentTaskHandleForCore( xCoreID );
        if( taskVALID_CORE_ID( xCoreID ) != pdFALSE )
        {
            #if ( configNUMBER_OF_CORES == 1 )
                xReturn = pxCurrentTCB;
            #else 
                xReturn = pxCurrentTCBs[ xCoreID ];
            #endif 
        }
        traceRETURN_xTaskGetCurrentTaskHandleForCore( xReturn );
        return xReturn;
    }
#endif 
#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    BaseType_t xTaskGetSchedulerState( void )
    {
        BaseType_t xReturn;
        traceENTER_xTaskGetSchedulerState();
        if( xSchedulerRunning == pdFALSE )
        {
            xReturn = taskSCHEDULER_NOT_STARTED;
        }
        else
        {
            #if ( configNUMBER_OF_CORES > 1 )
                taskENTER_CRITICAL();
            #endif
            {
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    xReturn = taskSCHEDULER_RUNNING;
                }
                else
                {
                    xReturn = taskSCHEDULER_SUSPENDED;
                }
            }
            #if ( configNUMBER_OF_CORES > 1 )
                taskEXIT_CRITICAL();
            #endif
        }
        traceRETURN_xTaskGetSchedulerState( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_MUTEXES == 1 )
    BaseType_t xTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
    {
        TCB_t * const pxMutexHolderTCB = pxMutexHolder;
        BaseType_t xReturn = pdFALSE;
        traceENTER_xTaskPriorityInherit( pxMutexHolder );
        if( pxMutexHolder != NULL )
        {
            if( pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority )
            {
                if( ( listGET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                {
                    listSET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
                if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxMutexHolderTCB->uxPriority ] ), &( pxMutexHolderTCB->xStateListItem ) ) != pdFALSE )
                {
                    if( uxListRemove( &( pxMutexHolderTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        portRESET_READY_PRIORITY( pxMutexHolderTCB->uxPriority, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
                    prvAddTaskToReadyList( pxMutexHolderTCB );
                    #if ( configNUMBER_OF_CORES > 1 )
                    {
                        if( taskTASK_IS_RUNNING( pxMutexHolderTCB ) != pdTRUE )
                        {
                            prvYieldForTask( pxMutexHolderTCB );
                        }
                    }
                    #endif 
                }
                else
                {
                    pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
                }
                traceTASK_PRIORITY_INHERIT( pxMutexHolderTCB, pxCurrentTCB->uxPriority );
                xReturn = pdTRUE;
            }
            else
            {
                if( pxMutexHolderTCB->uxBasePriority < pxCurrentTCB->uxPriority )
                {
                    xReturn = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_xTaskPriorityInherit( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_MUTEXES == 1 )
    BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder )
    {
        TCB_t * const pxTCB = pxMutexHolder;
        BaseType_t xReturn = pdFALSE;
        traceENTER_xTaskPriorityDisinherit( pxMutexHolder );
        if( pxMutexHolder != NULL )
        {
            configASSERT( pxTCB == pxCurrentTCB );
            configASSERT( pxTCB->uxMutexesHeld );
            ( pxTCB->uxMutexesHeld )--;
            if( pxTCB->uxPriority != pxTCB->uxBasePriority )
            {
                if( pxTCB->uxMutexesHeld == ( UBaseType_t ) 0 )
                {
                    if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        portRESET_READY_PRIORITY( pxTCB->uxPriority, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
                    pxTCB->uxPriority = pxTCB->uxBasePriority;
                    listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxTCB->uxPriority );
                    prvAddTaskToReadyList( pxTCB );
                    #if ( configNUMBER_OF_CORES > 1 )
                    {
                        if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
                        }
                    }
                    #endif 
                    xReturn = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_xTaskPriorityDisinherit( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_MUTEXES == 1 )
    void vTaskPriorityDisinheritAfterTimeout( TaskHandle_t const pxMutexHolder,
                                              UBaseType_t uxHighestPriorityWaitingTask )
    {
        TCB_t * const pxTCB = pxMutexHolder;
        UBaseType_t uxPriorityUsedOnEntry, uxPriorityToUse;
        const UBaseType_t uxOnlyOneMutexHeld = ( UBaseType_t ) 1;
        traceENTER_vTaskPriorityDisinheritAfterTimeout( pxMutexHolder, uxHighestPriorityWaitingTask );
        if( pxMutexHolder != NULL )
        {
            configASSERT( pxTCB->uxMutexesHeld );
            if( pxTCB->uxBasePriority < uxHighestPriorityWaitingTask )
            {
                uxPriorityToUse = uxHighestPriorityWaitingTask;
            }
            else
            {
                uxPriorityToUse = pxTCB->uxBasePriority;
            }
            if( pxTCB->uxPriority != uxPriorityToUse )
            {
                if( pxTCB->uxMutexesHeld == uxOnlyOneMutexHeld )
                {
                    configASSERT( pxTCB != pxCurrentTCB );
                    traceTASK_PRIORITY_DISINHERIT( pxTCB, uxPriorityToUse );
                    uxPriorityUsedOnEntry = pxTCB->uxPriority;
                    pxTCB->uxPriority = uxPriorityToUse;
                    if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                    {
                        listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriorityToUse );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
                    {
                        if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                        {
                            portRESET_READY_PRIORITY( pxTCB->uxPriority, uxTopReadyPriority );
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                        prvAddTaskToReadyList( pxTCB );
                        #if ( configNUMBER_OF_CORES > 1 )
                        {
                            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                            {
                                prvYieldCore( pxTCB->xTaskRunState );
                            }
                        }
                        #endif 
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskPriorityDisinheritAfterTimeout();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskYieldWithinAPI( void )
    {
        UBaseType_t ulState;
        traceENTER_vTaskYieldWithinAPI();
        ulState = portSET_INTERRUPT_MASK();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
            {
                portYIELD();
            }
            else
            {
                xYieldPendings[ xCoreID ] = pdTRUE;
            }
        }
        portCLEAR_INTERRUPT_MASK( ulState );
        traceRETURN_vTaskYieldWithinAPI();
    }
#endif 
#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) )
    void vTaskEnterCritical( void )
    {
        traceENTER_vTaskEnterCritical();
        portDISABLE_INTERRUPTS();
        if( xSchedulerRunning != pdFALSE )
        {
            ( pxCurrentTCB->uxCriticalNesting )++;
            if( pxCurrentTCB->uxCriticalNesting == 1U )
            {
                portASSERT_IF_IN_ISR();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskEnterCritical();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskEnterCritical( void )
    {
        traceENTER_vTaskEnterCritical();
        portDISABLE_INTERRUPTS();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
            if( xSchedulerRunning != pdFALSE )
            {
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    portGET_TASK_LOCK( xCoreID );
                    portGET_ISR_LOCK( xCoreID );
                }
                portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID );
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 1U )
                {
                    portASSERT_IF_IN_ISR();
                    if( uxSchedulerSuspended == 0U )
                    {
                        prvCheckForRunStateChange();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        traceRETURN_vTaskEnterCritical();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    UBaseType_t vTaskEnterCriticalFromISR( void )
    {
        UBaseType_t uxSavedInterruptStatus = 0;
        const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
        traceENTER_vTaskEnterCriticalFromISR();
        if( xSchedulerRunning != pdFALSE )
        {
            uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
            {
                portGET_ISR_LOCK( xCoreID );
            }
            portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskEnterCriticalFromISR( uxSavedInterruptStatus );
        return uxSavedInterruptStatus;
    }
#endif 
#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) )
    void vTaskExitCritical( void )
    {
        traceENTER_vTaskExitCritical();
        if( xSchedulerRunning != pdFALSE )
        {
            configASSERT( pxCurrentTCB->uxCriticalNesting > 0U );
            portASSERT_IF_IN_ISR();
            if( pxCurrentTCB->uxCriticalNesting > 0U )
            {
                ( pxCurrentTCB->uxCriticalNesting )--;
                if( pxCurrentTCB->uxCriticalNesting == 0U )
                {
                    portENABLE_INTERRUPTS();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskExitCritical();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskExitCritical( void )
    {
        const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();
        traceENTER_vTaskExitCritical();
        if( xSchedulerRunning != pdFALSE )
        {
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U );
            portASSERT_IF_IN_ISR();
            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U )
            {
                portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID );
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    BaseType_t xYieldCurrentTask;
                    xYieldCurrentTask = xYieldPendings[ xCoreID ];
                    portRELEASE_ISR_LOCK( xCoreID );
                    portRELEASE_TASK_LOCK( xCoreID );
                    portENABLE_INTERRUPTS();
                    if( xYieldCurrentTask != pdFALSE )
                    {
                        portYIELD();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskExitCritical();
    }
#endif 
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskExitCriticalFromISR( UBaseType_t uxSavedInterruptStatus )
    {
        BaseType_t xCoreID;
        traceENTER_vTaskExitCriticalFromISR( uxSavedInterruptStatus );
        if( xSchedulerRunning != pdFALSE )
        {
            xCoreID = ( BaseType_t ) portGET_CORE_ID();
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U );
            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U )
            {
                portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID );
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    portRELEASE_ISR_LOCK( xCoreID );
                    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskExitCriticalFromISR();
    }
#endif 
#if ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 )
    static char * prvWriteNameToBuffer( char * pcBuffer,
                                        const char * pcTaskName )
    {
        size_t x;
        ( void ) strcpy( pcBuffer, pcTaskName );
        for( x = strlen( pcBuffer ); x < ( size_t ) ( ( size_t ) configMAX_TASK_NAME_LEN - 1U ); x++ )
        {
            pcBuffer[ x ] = ' ';
        }
        pcBuffer[ x ] = ( char ) 0x00;
        return &( pcBuffer[ x ] );
    }
#endif 
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    void vTaskListTasks( char * pcWriteBuffer,
                         size_t uxBufferLength )
    {
        TaskStatus_t * pxTaskStatusArray;
        size_t uxConsumedBufferLength = 0;
        size_t uxCharsWrittenBySnprintf;
        int iSnprintfReturnValue;
        BaseType_t xOutputBufferFull = pdFALSE;
        UBaseType_t uxArraySize, x;
        char cStatus;
        traceENTER_vTaskListTasks( pcWriteBuffer, uxBufferLength );
        *pcWriteBuffer = ( char ) 0x00;
        uxArraySize = uxCurrentNumberOfTasks;
        pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) );
        if( pxTaskStatusArray != NULL )
        {
            uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );
            for( x = 0; x < uxArraySize; x++ )
            {
                switch( pxTaskStatusArray[ x ].eCurrentState )
                {
                    case eRunning:
                        cStatus = tskRUNNING_CHAR;
                        break;
                    case eReady:
                        cStatus = tskREADY_CHAR;
                        break;
                    case eBlocked:
                        cStatus = tskBLOCKED_CHAR;
                        break;
                    case eSuspended:
                        cStatus = tskSUSPENDED_CHAR;
                        break;
                    case eDeleted:
                        cStatus = tskDELETED_CHAR;
                        break;
                    case eInvalid: 
                    default:       
                        cStatus = ( char ) 0x00;
                        break;
                }
                if( ( uxConsumedBufferLength + configMAX_TASK_NAME_LEN ) <= uxBufferLength )
                {
                    pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );
                    uxConsumedBufferLength = uxConsumedBufferLength + ( configMAX_TASK_NAME_LEN - 1U );
                    if( uxConsumedBufferLength < ( uxBufferLength - 1U ) )
                    {
                        #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
                            iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                             uxBufferLength - uxConsumedBufferLength,
                                                             "\t%c\t%u\t%u\t%u\t0x%x\r\n",
                                                             cStatus,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCoreAffinityMask );
                        #else 
                            iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                             uxBufferLength - uxConsumedBufferLength,
                                                             "\t%c\t%u\t%u\t%u\r\n",
                                                             cStatus,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber );
                        #endif 
                        uxCharsWrittenBySnprintf = prvSnprintfReturnValueToCharsWritten( iSnprintfReturnValue, uxBufferLength - uxConsumedBufferLength );
                        uxConsumedBufferLength += uxCharsWrittenBySnprintf;
                        pcWriteBuffer += uxCharsWrittenBySnprintf;
                    }
                    else
                    {
                        xOutputBufferFull = pdTRUE;
                    }
                }
                else
                {
                    xOutputBufferFull = pdTRUE;
                }
                if( xOutputBufferFull == pdTRUE )
                {
                    break;
                }
            }
            vPortFree( pxTaskStatusArray );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskListTasks();
    }
#endif 
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configUSE_TRACE_FACILITY == 1 ) )
    void vTaskGetRunTimeStatistics( char * pcWriteBuffer,
                                    size_t uxBufferLength )
    {
        TaskStatus_t * pxTaskStatusArray;
        size_t uxConsumedBufferLength = 0;
        size_t uxCharsWrittenBySnprintf;
        int iSnprintfReturnValue;
        BaseType_t xOutputBufferFull = pdFALSE;
        UBaseType_t uxArraySize, x;
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0;
        configRUN_TIME_COUNTER_TYPE ulStatsAsPercentage;
        traceENTER_vTaskGetRunTimeStatistics( pcWriteBuffer, uxBufferLength );
        *pcWriteBuffer = ( char ) 0x00;
        uxArraySize = uxCurrentNumberOfTasks;
        pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) );
        if( pxTaskStatusArray != NULL )
        {
            uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );
            ulTotalTime /= ( ( configRUN_TIME_COUNTER_TYPE ) 100U );
            if( ulTotalTime > 0U )
            {
                for( x = 0; x < uxArraySize; x++ )
                {
                    ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalTime;
                    if( ( uxConsumedBufferLength + configMAX_TASK_NAME_LEN ) <= uxBufferLength )
                    {
                        pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );
                        uxConsumedBufferLength = uxConsumedBufferLength + ( configMAX_TASK_NAME_LEN - 1U );
                        if( uxConsumedBufferLength < ( uxBufferLength - 1U ) )
                        {
                            if( ulStatsAsPercentage > 0U )
                            {
                                #ifdef portLU_PRINTF_SPECIFIER_REQUIRED
                                {
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%lu\t\t%lu%%\r\n",
                                                                     pxTaskStatusArray[ x ].ulRunTimeCounter,
                                                                     ulStatsAsPercentage );
                                }
                                #else 
                                {
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%u\t\t%u%%\r\n",
                                                                     ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter,
                                                                     ( unsigned int ) ulStatsAsPercentage );
                                }
                                #endif 
                            }
                            else
                            {
                                #ifdef portLU_PRINTF_SPECIFIER_REQUIRED
                                {
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%lu\t\t<1%%\r\n",
                                                                     pxTaskStatusArray[ x ].ulRunTimeCounter );
                                }
                                #else
                                {
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%u\t\t<1%%\r\n",
                                                                     ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter );
                                }
                                #endif 
                            }
                            uxCharsWrittenBySnprintf = prvSnprintfReturnValueToCharsWritten( iSnprintfReturnValue, uxBufferLength - uxConsumedBufferLength );
                            uxConsumedBufferLength += uxCharsWrittenBySnprintf;
                            pcWriteBuffer += uxCharsWrittenBySnprintf;
                        }
                        else
                        {
                            xOutputBufferFull = pdTRUE;
                        }
                    }
                    else
                    {
                        xOutputBufferFull = pdTRUE;
                    }
                    if( xOutputBufferFull == pdTRUE )
                    {
                        break;
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            vPortFree( pxTaskStatusArray );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        traceRETURN_vTaskGetRunTimeStatistics();
    }
#endif 
TickType_t uxTaskResetEventItemValue( void )
{
    TickType_t uxReturn;
    traceENTER_uxTaskResetEventItemValue();
    uxReturn = listGET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ) );
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority ) );
    traceRETURN_uxTaskResetEventItemValue( uxReturn );
    return uxReturn;
}
#if ( configUSE_MUTEXES == 1 )
    TaskHandle_t pvTaskIncrementMutexHeldCount( void )
    {
        TCB_t * pxTCB;
        traceENTER_pvTaskIncrementMutexHeldCount();
        pxTCB = pxCurrentTCB;
        if( pxTCB != NULL )
        {
            ( pxTCB->uxMutexesHeld )++;
        }
        traceRETURN_pvTaskIncrementMutexHeldCount( pxTCB );
        return pxTCB;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWaitOn,
                                      BaseType_t xClearCountOnExit,
                                      TickType_t xTicksToWait )
    {
        uint32_t ulReturn;
        BaseType_t xAlreadyYielded, xShouldBlock = pdFALSE;
        traceENTER_ulTaskGenericNotifyTake( uxIndexToWaitOn, xClearCountOnExit, xTicksToWait );
        configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        if( ( pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] == 0U ) && ( xTicksToWait > ( TickType_t ) 0 ) )
        {
            vTaskSuspendAll();
            {
                taskENTER_CRITICAL();
                {
                    if( pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] == 0U )
                    {
                        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;
                        xShouldBlock = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();
                if( xShouldBlock == pdTRUE )
                {
                    traceTASK_NOTIFY_TAKE_BLOCK( uxIndexToWaitOn );
                    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            xAlreadyYielded = xTaskResumeAll();
            if( ( xShouldBlock == pdTRUE ) && ( xAlreadyYielded == pdFALSE ) )
            {
                taskYIELD_WITHIN_API();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskENTER_CRITICAL();
        {
            traceTASK_NOTIFY_TAKE( uxIndexToWaitOn );
            ulReturn = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];
            if( ulReturn != 0U )
            {
                if( xClearCountOnExit != pdFALSE )
                {
                    pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = ( uint32_t ) 0U;
                }
                else
                {
                    pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = ulReturn - ( uint32_t ) 1;
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
        }
        taskEXIT_CRITICAL();
        traceRETURN_ulTaskGenericNotifyTake( ulReturn );
        return ulReturn;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                       uint32_t ulBitsToClearOnEntry,
                                       uint32_t ulBitsToClearOnExit,
                                       uint32_t * pulNotificationValue,
                                       TickType_t xTicksToWait )
    {
        BaseType_t xReturn, xAlreadyYielded, xShouldBlock = pdFALSE;
        traceENTER_xTaskGenericNotifyWait( uxIndexToWaitOn, ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait );
        configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        if( ( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED ) && ( xTicksToWait > ( TickType_t ) 0 ) )
        {
            vTaskSuspendAll();
            {
                taskENTER_CRITICAL();
                {
                    if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
                    {
                        pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnEntry;
                        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;
                        xShouldBlock = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();
                if( xShouldBlock == pdTRUE )
                {
                    traceTASK_NOTIFY_WAIT_BLOCK( uxIndexToWaitOn );
                    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            xAlreadyYielded = xTaskResumeAll();
            if( ( xShouldBlock == pdTRUE ) && ( xAlreadyYielded == pdFALSE ) )
            {
                taskYIELD_WITHIN_API();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskENTER_CRITICAL();
        {
            traceTASK_NOTIFY_WAIT( uxIndexToWaitOn );
            if( pulNotificationValue != NULL )
            {
                *pulNotificationValue = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];
            }
            if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
            {
                xReturn = pdFALSE;
            }
            else
            {
                pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnExit;
                xReturn = pdTRUE;
            }
            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
        }
        taskEXIT_CRITICAL();
        traceRETURN_xTaskGenericNotifyWait( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                                   UBaseType_t uxIndexToNotify,
                                   uint32_t ulValue,
                                   eNotifyAction eAction,
                                   uint32_t * pulPreviousNotificationValue )
    {
        TCB_t * pxTCB;
        BaseType_t xReturn = pdPASS;
        uint8_t ucOriginalNotifyState;
        traceENTER_xTaskGenericNotify( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue );
        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        configASSERT( xTaskToNotify );
        pxTCB = xTaskToNotify;
        taskENTER_CRITICAL();
        {
            if( pulPreviousNotificationValue != NULL )
            {
                *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
            }
            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;
            switch( eAction )
            {
                case eSetBits:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                    break;
                case eIncrement:
                    ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
                    break;
                case eSetValueWithOverwrite:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    break;
                case eSetValueWithoutOverwrite:
                    if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                    {
                        pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    }
                    else
                    {
                        xReturn = pdFAIL;
                    }
                    break;
                case eNoAction:
                    break;
                default:
                    configASSERT( xTickCount == ( TickType_t ) 0 );
                    break;
            }
            traceTASK_NOTIFY( uxIndexToNotify );
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                prvAddTaskToReadyList( pxTCB );
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );
                #if ( configUSE_TICKLESS_IDLE != 0 )
                {
                    prvResetNextTaskUnblockTime();
                }
                #endif
                taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_xTaskGenericNotify( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                          UBaseType_t uxIndexToNotify,
                                          uint32_t ulValue,
                                          eNotifyAction eAction,
                                          uint32_t * pulPreviousNotificationValue,
                                          BaseType_t * pxHigherPriorityTaskWoken )
    {
        TCB_t * pxTCB;
        uint8_t ucOriginalNotifyState;
        BaseType_t xReturn = pdPASS;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_xTaskGenericNotifyFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken );
        configASSERT( xTaskToNotify );
        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
        pxTCB = xTaskToNotify;
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            if( pulPreviousNotificationValue != NULL )
            {
                *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
            }
            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;
            switch( eAction )
            {
                case eSetBits:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                    break;
                case eIncrement:
                    ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
                    break;
                case eSetValueWithOverwrite:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    break;
                case eSetValueWithoutOverwrite:
                    if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                    {
                        pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    }
                    else
                    {
                        xReturn = pdFAIL;
                    }
                    break;
                case eNoAction:
                    break;
                default:
                    configASSERT( xTickCount == ( TickType_t ) 0 );
                    break;
            }
            traceTASK_NOTIFY_FROM_ISR( uxIndexToNotify );
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );
                    #if ( configUSE_TICKLESS_IDLE != 0 )
                    {
                        prvResetNextTaskUnblockTime();
                    }
                    #endif
                }
                else
                {
                    listINSERT_END( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }
                #if ( configNUMBER_OF_CORES == 1 )
                {
                    if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                    {
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }
                        xYieldPendings[ 0 ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #else 
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        prvYieldForTask( pxTCB );
                        if( xYieldPendings[ portGET_CORE_ID() ] == pdTRUE )
                        {
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                        }
                    }
                    #endif 
                }
                #endif 
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_xTaskGenericNotifyFromISR( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                        UBaseType_t uxIndexToNotify,
                                        BaseType_t * pxHigherPriorityTaskWoken )
    {
        TCB_t * pxTCB;
        uint8_t ucOriginalNotifyState;
        UBaseType_t uxSavedInterruptStatus;
        traceENTER_vTaskGenericNotifyGiveFromISR( xTaskToNotify, uxIndexToNotify, pxHigherPriorityTaskWoken );
        configASSERT( xTaskToNotify );
        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
        pxTCB = xTaskToNotify;
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;
            ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
            traceTASK_NOTIFY_GIVE_FROM_ISR( uxIndexToNotify );
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );
                    #if ( configUSE_TICKLESS_IDLE != 0 )
                    {
                        prvResetNextTaskUnblockTime();
                    }
                    #endif
                }
                else
                {
                    listINSERT_END( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }
                #if ( configNUMBER_OF_CORES == 1 )
                {
                    if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                    {
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }
                        xYieldPendings[ 0 ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #else 
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        prvYieldForTask( pxTCB );
                        if( xYieldPendings[ portGET_CORE_ID() ] == pdTRUE )
                        {
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                        }
                    }
                    #endif 
                }
                #endif 
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        traceRETURN_vTaskGenericNotifyGiveFromISR();
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                             UBaseType_t uxIndexToClear )
    {
        TCB_t * pxTCB;
        BaseType_t xReturn;
        traceENTER_xTaskGenericNotifyStateClear( xTask, uxIndexToClear );
        configASSERT( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        taskENTER_CRITICAL();
        {
            if( pxTCB->ucNotifyState[ uxIndexToClear ] == taskNOTIFICATION_RECEIVED )
            {
                pxTCB->ucNotifyState[ uxIndexToClear ] = taskNOT_WAITING_NOTIFICATION;
                xReturn = pdPASS;
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_xTaskGenericNotifyStateClear( xReturn );
        return xReturn;
    }
#endif 
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                            UBaseType_t uxIndexToClear,
                                            uint32_t ulBitsToClear )
    {
        TCB_t * pxTCB;
        uint32_t ulReturn;
        traceENTER_ulTaskGenericNotifyValueClear( xTask, uxIndexToClear, ulBitsToClear );
        configASSERT( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        taskENTER_CRITICAL();
        {
            ulReturn = pxTCB->ulNotifiedValue[ uxIndexToClear ];
            pxTCB->ulNotifiedValue[ uxIndexToClear ] &= ~ulBitsToClear;
        }
        taskEXIT_CRITICAL();
        traceRETURN_ulTaskGenericNotifyValueClear( ulReturn );
        return ulReturn;
    }
#endif 
#if ( configGENERATE_RUN_TIME_STATS == 1 )
    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimeCounter( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0, ulTimeSinceLastSwitchedIn = 0, ulTaskRunTime = 0;
        traceENTER_ulTaskGetRunTimeCounter( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        taskENTER_CRITICAL();
        {
            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
            {
                #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
                #else
                    ulTotalTime = portGET_RUN_TIME_COUNTER_VALUE();
                #endif
                #if ( configNUMBER_OF_CORES == 1 )
                    ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ 0 ];
                #else
                    ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ pxTCB->xTaskRunState ];
                #endif
            }
            ulTaskRunTime = pxTCB->ulRunTimeCounter + ulTimeSinceLastSwitchedIn;
        }
        taskEXIT_CRITICAL();
        traceRETURN_ulTaskGetRunTimeCounter( ulTaskRunTime );
        return ulTaskRunTime;
    }
#endif 
#if ( configGENERATE_RUN_TIME_STATS == 1 )
    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimePercent( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        configRUN_TIME_COUNTER_TYPE ulTotalTime, ulReturn, ulTaskRunTime;
        traceENTER_ulTaskGetRunTimePercent( xTask );
        ulTaskRunTime = ulTaskGetRunTimeCounter( xTask );
        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
            portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
        #else
            ulTotalTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
        #endif
        ulTotalTime /= ( configRUN_TIME_COUNTER_TYPE ) 100;
        if( ulTotalTime > ( configRUN_TIME_COUNTER_TYPE ) 0 )
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );
            ulReturn = ulTaskRunTime / ulTotalTime;
        }
        else
        {
            ulReturn = 0;
        }
        traceRETURN_ulTaskGetRunTimePercent( ulReturn );
        return ulReturn;
    }
#endif 
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )
    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounter( void )
    {
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0, ulTimeSinceLastSwitchedIn = 0, ulIdleTaskRunTime = 0;
        BaseType_t i;
        traceENTER_ulTaskGetIdleRunTimeCounter();
        taskENTER_CRITICAL();
        {
            #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
            #else
                ulTotalTime = portGET_RUN_TIME_COUNTER_VALUE();
            #endif
            for( i = 0; i < ( BaseType_t ) configNUMBER_OF_CORES; i++ )
            {
                if( taskTASK_IS_RUNNING( xIdleTaskHandles[ i ] ) == pdTRUE )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                        ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ 0 ];
                    #else
                        ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ xIdleTaskHandles[ i ]->xTaskRunState ];
                    #endif
                }
                else
                {
                    ulTimeSinceLastSwitchedIn = 0;
                }
                ulIdleTaskRunTime += ( xIdleTaskHandles[ i ]->ulRunTimeCounter + ulTimeSinceLastSwitchedIn );
            }
        }
        taskEXIT_CRITICAL();
        traceRETURN_ulTaskGetIdleRunTimeCounter( ulIdleTaskRunTime );
        return ulIdleTaskRunTime;
    }
#endif 
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )
    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercent( void )
    {
        configRUN_TIME_COUNTER_TYPE ulTotalTime, ulReturn;
        configRUN_TIME_COUNTER_TYPE ulRunTimeCounter = 0;
        traceENTER_ulTaskGetIdleRunTimePercent();
        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
            portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
        #else
            ulTotalTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
        #endif
        ulTotalTime *= configNUMBER_OF_CORES;
        ulTotalTime /= ( configRUN_TIME_COUNTER_TYPE ) 100;
        if( ulTotalTime > ( configRUN_TIME_COUNTER_TYPE ) 0 )
        {
            ulRunTimeCounter = ulTaskGetIdleRunTimeCounter();
            ulReturn = ulRunTimeCounter / ulTotalTime;
        }
        else
        {
            ulReturn = 0;
        }
        traceRETURN_ulTaskGetIdleRunTimePercent( ulReturn );
        return ulReturn;
    }
#endif 
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                            const BaseType_t xCanBlockIndefinitely )
{
    TickType_t xTimeToWake;
    const TickType_t xConstTickCount = xTickCount;
    List_t * const pxDelayedList = pxDelayedTaskList;
    List_t * const pxOverflowDelayedList = pxOverflowDelayedTaskList;
    #if ( INCLUDE_xTaskAbortDelay == 1 )
    {
        pxCurrentTCB->ucDelayAborted = ( uint8_t ) pdFALSE;
    }
    #endif
    if( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
    {
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        if( ( xTicksToWait == portMAX_DELAY ) && ( xCanBlockIndefinitely != pdFALSE ) )
        {
            listINSERT_END( &xSuspendedTaskList, &( pxCurrentTCB->xStateListItem ) );
        }
        else
        {
            xTimeToWake = xConstTickCount + xTicksToWait;
            listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );
            if( xTimeToWake < xConstTickCount )
            {
                traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST();
                vListInsert( pxOverflowDelayedList, &( pxCurrentTCB->xStateListItem ) );
            }
            else
            {
                traceMOVED_TASK_TO_DELAYED_LIST();
                vListInsert( pxDelayedList, &( pxCurrentTCB->xStateListItem ) );
                if( xTimeToWake < xNextTaskUnblockTime )
                {
                    xNextTaskUnblockTime = xTimeToWake;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
    }
    #else 
    {
        xTimeToWake = xConstTickCount + xTicksToWait;
        listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );
        if( xTimeToWake < xConstTickCount )
        {
            traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST();
            vListInsert( pxOverflowDelayedList, &( pxCurrentTCB->xStateListItem ) );
        }
        else
        {
            traceMOVED_TASK_TO_DELAYED_LIST();
            vListInsert( pxDelayedList, &( pxCurrentTCB->xStateListItem ) );
            if( xTimeToWake < xNextTaskUnblockTime )
            {
                xNextTaskUnblockTime = xTimeToWake;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        ( void ) xCanBlockIndefinitely;
    }
    #endif 
}
#if ( portUSING_MPU_WRAPPERS == 1 )
    xMPU_SETTINGS * xTaskGetMPUSettings( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        traceENTER_xTaskGetMPUSettings( xTask );
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );
        traceRETURN_xTaskGetMPUSettings( &( pxTCB->xMPUSettings ) );
        return &( pxTCB->xMPUSettings );
    }
#endif 
#ifdef FREERTOS_MODULE_TEST
    #include "tasks_test_access_functions.h"
#endif
#if ( configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1 )
    #include "freertos_tasks_c_additions.h"
    #ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
        static void freertos_tasks_c_additions_init( void )
        {
            FREERTOS_TASKS_C_ADDITIONS_INIT();
        }
    #endif
#endif 
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
    void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                        StackType_t ** ppxIdleTaskStackBuffer,
                                        configSTACK_DEPTH_TYPE * puxIdleTaskStackSize )
    {
        static StaticTask_t xIdleTaskTCB;
        static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];
        *ppxIdleTaskTCBBuffer = &( xIdleTaskTCB );
        *ppxIdleTaskStackBuffer = &( uxIdleTaskStack[ 0 ] );
        *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    }
    #if ( configNUMBER_OF_CORES > 1 )
        void vApplicationGetPassiveIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                                   StackType_t ** ppxIdleTaskStackBuffer,
                                                   configSTACK_DEPTH_TYPE * puxIdleTaskStackSize,
                                                   BaseType_t xPassiveIdleTaskIndex )
        {
            static StaticTask_t xIdleTaskTCBs[ configNUMBER_OF_CORES - 1 ];
            static StackType_t uxIdleTaskStacks[ configNUMBER_OF_CORES - 1 ][ configMINIMAL_STACK_SIZE ];
            *ppxIdleTaskTCBBuffer = &( xIdleTaskTCBs[ xPassiveIdleTaskIndex ] );
            *ppxIdleTaskStackBuffer = &( uxIdleTaskStacks[ xPassiveIdleTaskIndex ][ 0 ] );
            *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
        }
    #endif 
#endif 
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) && ( configUSE_TIMERS == 1 ) )
    void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                         StackType_t ** ppxTimerTaskStackBuffer,
                                         configSTACK_DEPTH_TYPE * puxTimerTaskStackSize )
    {
        static StaticTask_t xTimerTaskTCB;
        static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
        *ppxTimerTaskTCBBuffer = &( xTimerTaskTCB );
        *ppxTimerTaskStackBuffer = &( uxTimerTaskStack[ 0 ] );
        *puxTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
    }
#endif 
void vTaskResetState( void )
{
    BaseType_t xCoreID;
    #if ( configNUMBER_OF_CORES == 1 )
    {
        pxCurrentTCB = NULL;
    }
    #endif 
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;
    }
    #endif 
    #if ( configUSE_POSIX_ERRNO == 1 )
    {
        FreeRTOS_errno = 0;
    }
    #endif 
    uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
    xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
    uxTopReadyPriority = tskIDLE_PRIORITY;
    xSchedulerRunning = pdFALSE;
    xPendedTicks = ( TickType_t ) 0U;
    for( xCoreID = 0; xCoreID < configNUMBER_OF_CORES; xCoreID++ )
    {
        xYieldPendings[ xCoreID ] = pdFALSE;
    }
    xNumOfOverflows = ( BaseType_t ) 0;
    uxTaskNumber = ( UBaseType_t ) 0U;
    xNextTaskUnblockTime = ( TickType_t ) 0U;
    uxSchedulerSuspended = ( UBaseType_t ) 0U;
    #if ( configGENERATE_RUN_TIME_STATS == 1 )
    {
        for( xCoreID = 0; xCoreID < configNUMBER_OF_CORES; xCoreID++ )
        {
            ulTaskSwitchedInTime[ xCoreID ] = 0U;
            ulTotalRunTime[ xCoreID ] = 0U;
        }
    }
    #endif 
}
