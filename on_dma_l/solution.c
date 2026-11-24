#include "dma.h"

// Implement the driver here
// add any localfile states or helpers
static uint16_t buffer[BATCH_SIZE];

enum Driver_State
{
    IDLE, 
    BUSY,
    DONE
};
static volatile enum Driver_State state;
static volatile bool hasBatchPassed = false;

/* Reset the driver to the idle state. */
void app_init(void)
{
    state = IDLE;
    hasBatchPassed = false;
}

/* Start a new 32-sample DMA batch.
 * Returns false and changes nothing if a batch is still running or waiting
 * for the worker. Configures the DMA: peripheral-to-memory, 16-bit width,
 * fixed source (SENSOR_DATA), incrementing destination (buffer), and
 * completion/error interrupts enabled. */
bool app_start(void)
{
    if (state != IDLE)
        return false;

    DMA.SRC = (uintptr_t)&SENSOR_DATA; 

    DMA.DST = (uintptr_t)buffer; 

    DMA.COUNT = BATCH_SIZE;

    state = BUSY;

    DMA.CTRL = DMA_ENABLE | DMA_P2M | DMA_WIDTH_16 | DMA_DST_INC | DMA_COMPLETE_IE | DMA_ERROR_IE;

    return true;
}

/* DMA interrupt handler (ISR). Kept short: clears the flags, records success
 * or failure (error takes priority), and wakes the worker. Ignores interrupts
 * with no relevant flag, or that arrive when no transfer is running. */
void app_dma_irq(void) // irq -> interrupt request
{
    uint32_t status = DMA.STATUS; 

    if ((status & (DMA_COMPLETE | DMA_ERROR)) == 0)
        return;

    dma_clear_flags((status & (DMA_COMPLETE | DMA_ERROR)));

    if (state != BUSY)
        return;
    
    if ((status & DMA_ERROR) != 0)
        hasBatchPassed = false;
    else
        hasBatchPassed = true;

    state = DONE;

    notify_worker_from_isr();
}

/* Worker task step. If notified, reports min, max, sum and mean of the batch
 * (or an error for a failed batch), then frees the buffer by returning to
 * IDLE. Returns immediately if there is no notification. */
void app_task_step(void)
{
    
    if (!take_worker_notification())
        return;

    
    if (!hasBatchPassed)
        report_error();
    else
    {
        BatchResult result;
        result.min = buffer[0];
        result.max = buffer[0];
        result.sum = 0;

        for (uint32_t i = 0; i < BATCH_SIZE; i++)
        {
            if (buffer[i] < result.min)
                result.min = buffer[i];
            if (buffer[i] > result.max)
                result.max = buffer[i];

            result.sum += buffer[i];
        }
        result.mean = result.sum / BATCH_SIZE;
        report_result(&result);
    }

    state = IDLE;
}


// Q1: The source stays fixed because SENSOR_DATA is one register at one address; incrementing it would read unrelated memory, then it flags so DMA copies data from there and place it in RAM's contiguous memory allocated to 32 readings. Then the flag goes off and receiver address is incremented by two as previous value should be retained and every reading is of 2 bytes.

// Q2: Buffer(memory allocated for readings in RAM) should remain reserved so that any other batch operation running parallely shouldn't overwrite any of the readings in that memory else the worker would calc. wrong min, max, means etc. So app_start() rejects new batches until the worker finishes and returns the state to IDLE  

// Q3: In FreeRTOS, the ISR wakes the worker using vTaskNotifyGiveFromISR() (notify_worker_from_isr() in this simulator). The worker uses ulTaskNotifyTake() (take_worker_notification() in this simulator) to receive the notification that a full batch of 32 readings has arrived, and it stays blocked (sleeping) in ulTaskNotifyTake() without using the CPU.
