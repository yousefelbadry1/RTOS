#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include <time.h>
#include "semphr.h"
#include "diag/trace.h"
#define CCM_RAM __attribute__((section(".ccmram")))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#define QUEUE_LENGTH            3
#define ITEM_SIZE               sizeof(char *)
#define TSLEEP_ARRAY_SIZE 6

const uint32_t TSLEEP_MIN[TSLEEP_ARRAY_SIZE] = {50, 80, 110, 140, 170, 200};
const uint32_t TSLEEP_MAX[TSLEEP_ARRAY_SIZE] = {150, 200, 250, 300, 350, 400};
uint8_t tsleep_index = 0;  // Initialize index to 0 (choose the first set of values initially)

#define TRECEIVER_PERIOD        pdMS_TO_TICKS(100)   // Receiver task period in ticks (100 msec)

QueueHandle_t xQueue;   // Queue for communication
SemaphoreHandle_t xSender1Semaphore,xSender2Semaphore,xSender3Semaphore, xReceiverSemaphore;   // Semaphores for task synchronization
TimerHandle_t xSenderTimer1;
TimerHandle_t xSenderTimer2;
TimerHandle_t xSenderTimer3;
TimerHandle_t xReceiverTimer;

int32_t totalSent= 0;
int32_t totalBlocked = 0;
int32_t totalReceived = 0;

int32_t sentCount1 = 0, sentCount2 = 0, sentCount3 = 0,blocked1=0,blocked2=0,blocked3=0;   // Sent message counters per sender task
TickType_t totalSentTime1 = 0, totalSentTime2 = 0, totalSentTime3 = 0;
// Function prototypes
static void vSenderTask1(void *pvParameters);
static void vSenderTask2(void *pvParameters);
static void vSenderTask3(void *pvParameters);
static void vReceiverTask(void *pvParameters);
static void Sender1TimerCallback(TimerHandle_t xTimer);
static void Sender2TimerCallback(TimerHandle_t xTimer);
static void Sender3TimerCallback(TimerHandle_t xTimer);
static void prvReceiverTimerCallback(TimerHandle_t xTimer);
void vReset(void);

// Function to generate random period for Tsender
uint32_t getRandomTSenderPeriod(void) {
    return (rand() % (TSLEEP_MAX[tsleep_index] - TSLEEP_MIN[tsleep_index] + 1)) + TSLEEP_MIN[tsleep_index];
}

static void vSenderTask1(void *pvParameters) {
    while (1) {
    	TickType_t startTime = xTaskGetTickCount();

    	TickType_t xLastWakeTime;
    	    TickType_t xPeriod = pdMS_TO_TICKS(getRandomTSenderPeriod());

    	    xLastWakeTime = xTaskGetTickCount();
        xSemaphoreTake(xSender1Semaphore, portMAX_DELAY);
            char message[20];
            itoa(xTaskGetTickCount(), message, 10);
            if (xQueueSend(xQueue, message, 0) == pdPASS) {
            	/*
            	trace_puts(message);
            	*/
                totalSent++;
                sentCount1++;
                totalSentTime1 += (xTaskGetTickCount() - startTime);
            } else {
                totalBlocked++;
                blocked1++;
            }
    }
}
static void vSenderTask2(void *pvParameters) {
    while (1) {
    	TickType_t startTime = xTaskGetTickCount();
    	TickType_t xLastWakeTime;
        TickType_t xPeriod = pdMS_TO_TICKS(getRandomTSenderPeriod());
        xLastWakeTime = xTaskGetTickCount();
        xSemaphoreTake(xSender2Semaphore, portMAX_DELAY);
            char message[20];
            itoa(xTaskGetTickCount(), message, 10);
            if (xQueueSend(xQueue, message, 0) == pdPASS) {
            	/*
            	trace_puts(message);
            	*/
                totalSent++;
                sentCount2++;
                totalSentTime2 += (xTaskGetTickCount() - startTime);
            } else {
                totalBlocked++;
                blocked2++;
            }
    }
}
static void vSenderTask3(void *pvParameters) {
    while (1) {
    	TickType_t startTime = xTaskGetTickCount();
    	TickType_t xLastWakeTime;
        TickType_t xPeriod = pdMS_TO_TICKS(getRandomTSenderPeriod());
        xLastWakeTime = xTaskGetTickCount();
        xSemaphoreTake(xSender3Semaphore, portMAX_DELAY);
            char message[20];
            itoa(xTaskGetTickCount(), message, 10);
            if (xQueueSend(xQueue, message, 0) == pdPASS) {
            	/*
            	trace_puts(message);
            	*/
                totalSent++;
                sentCount3++;
                totalSentTime3 += (xTaskGetTickCount() - startTime);
            } else {
                totalBlocked++;
                blocked3++;
            }
    }
}
static void vReceiverTask(void *pvParameters) {
    while (1) {
        xSemaphoreTake(xReceiverSemaphore, portMAX_DELAY);
            char receivedMessage[20];
            if (xQueueReceive(xQueue, receivedMessage, 0) == pdPASS) {
                totalReceived++;
                trace_printf("total received=%i\n",totalReceived);
            }
    }
}

static void Sender1TimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xSender1Semaphore);
    xTimerChangePeriod( xSenderTimer1,getRandomTSenderPeriod() , 0 );
}

static void Sender2TimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xSender2Semaphore);
    xTimerChangePeriod( xSenderTimer2,getRandomTSenderPeriod(), 0 );
}
static void Sender3TimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xSender3Semaphore);
    xTimerChangePeriod( xSenderTimer3,getRandomTSenderPeriod(), 0 );
}
static void prvReceiverTimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xReceiverSemaphore);
    if (totalReceived >= 1000){
        vReset();
    }
}



void vReset(void) {
    // Print statistics
	trace_printf("iteration number: %i\n", tsleep_index);
	trace_printf("Total sent: %i\n", totalSent);
	trace_printf("Total blocked: %i\n", totalBlocked);
	trace_printf("Sender Task 1: sent: %i\n", sentCount1);
	trace_printf("Sender Task 1: blocked: %i\n", blocked1);
	trace_printf("Sender Task 2: sent: %i\n", sentCount2);
	trace_printf("Sender Task 2: blocked: %i\n", blocked2);
	trace_printf("Sender Task 3: sent: %i\n", sentCount3);
	trace_printf("Sender Task 3: blocked: %i\n", blocked3);
	        uint32_t averageTime1 = totalSentTime1 / sentCount1;
	        trace_printf("Average message time sent (Sender Task 1): %u ticks\n", averageTime1);

	        uint32_t averageTime2 = totalSentTime2 / sentCount2;
	        trace_printf("Average message time sent (Sender Task 2): %u ticks\n", averageTime2);

	        uint32_t averageTime3 = totalSentTime3 / sentCount3;
	        trace_printf("Average message time sent (Sender Task 3): %u ticks\n", averageTime3);

	        uint32_t averageTotal = (averageTime1+averageTime2+averageTime3)/3;
	        trace_printf("Average message time sent Total: %u ticks\n", averageTotal);
    // Reset counters
    totalSent = 0;
    totalBlocked = 0;
    totalReceived = 0;
    sentCount1 = 0;
    sentCount2 = 0;
    sentCount3 = 0;
    blocked1=0;
    blocked2=0;
    blocked3=0;
    totalSentTime1 = 0;
    totalSentTime2 = 0;
    totalSentTime3 = 0;
    // Clear the queue
    xQueueReset(xQueue);
    tsleep_index++;
    if(tsleep_index==6){
    	char mess[]="Game Over";
    	trace_puts(mess);
    exit(0);}



    // Configure Tsender values for next iteration

    // Destroy timers and print "Game Over" message if all values in the arrays are used
}
int main(void) {
    // Seed random number generator
    srand(time(NULL));
    // Create the queue
    xQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
    // Create semaphores
    xSender1Semaphore = xSemaphoreCreateBinary();
    xSender2Semaphore = xSemaphoreCreateBinary();
    xSender3Semaphore = xSemaphoreCreateBinary();
    xReceiverSemaphore = xSemaphoreCreateBinary();

    // Create sender tasks
    xTaskCreate(vSenderTask1, "Sender1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vSenderTask2, "Sender2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vSenderTask3, "Sender3", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    // Create receiver task
    xTaskCreate(vReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    // Create and start sender and receiver timers
    xSenderTimer1 = xTimerCreate("SenderTimer1", pdMS_TO_TICKS(getRandomTSenderPeriod()), pdTRUE, 0, Sender1TimerCallback);
    xSenderTimer2 = xTimerCreate("SenderTimer2", pdMS_TO_TICKS(getRandomTSenderPeriod()), pdTRUE, 0, Sender2TimerCallback);
    xSenderTimer3 = xTimerCreate("SenderTimer3", pdMS_TO_TICKS(getRandomTSenderPeriod()), pdTRUE, 0, Sender3TimerCallback);
    xReceiverTimer = xTimerCreate("ReceiverTimer", TRECEIVER_PERIOD, pdTRUE, 0, prvReceiverTimerCallback);

    if (xSenderTimer1 != NULL && xReceiverTimer != NULL) {
    	trace_puts("timers start successfully");
           xTimerStart(xSenderTimer1, 0);
           xTimerStart(xSenderTimer2, 0);
           xTimerStart(xSenderTimer3, 0);
           xTimerStart(xReceiverTimer, 0);
       } else {
           printf("Failed to create/start timers\n");
           vTaskDelay(portMAX_DELAY);
       }
    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should not reach here
    for (;;);

    return 0;
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
