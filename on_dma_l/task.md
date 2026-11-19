# DMA/Concurrency

Receive **32 unsigned 16-bit distance readings** using simulated DMA, then calculate their statistics in a worker task. The simulator models registers and an ISR-to-worker handoff; it does not run FreeRTOS.

## Implement

Edit **`solution.c` only**. Use the interface in `dma.h`.

| Function            | Required behavior                                                                                                                                                                                                                                      |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `app_init()`      | Reset the driver to idle.                                                                                                                                                                                                                              |
| `app_start()`     | Start a batch and return `true` if idle. Otherwise return `false` without changing the transfer or pending result.                                                                                                                                 |
| `app_dma_irq()`   | Acknowledge completion/error flags using `dma_clear_flags()` and call `notify_worker_from_isr()`. Ignore interrupts with no relevant flag.                                                                                                         |
| `app_task_step()` | Consume a notification using `take_worker_notification()`; return if none. Report min, max, sum, and integer mean (`sum / 32`) using `report_result()`, or discard a failed batch and call `report_error()`. Report once, then return to idle. |

**DMA configuration:** peripheral-to-memory, 16-bit width, source fixed at `SENSOR_DATA`, destination incrementing, count 32, completion/error interrupts enabled. Use a persistent RAM buffer.

## Please note:

- Let DMA move the samples; no manual sensor reads or polling loops.
- Calculate statistics only in the worker, after completion notification.
- Reserve the buffer until the worker finishes; reject new batches meanwhile.
- Error takes priority if both completion and error are flagged.
- Support repeated batches and recovery after errors. Every function must return promptly.

## Quick knowledge check:

1. Why is the source fixed and destination incrementing?
2. Why must the buffer remain reserved until processing finishes?
3. Which FreeRTOS ISR-safe mechanism would wake the worker, and where would it block?

## Submit

Update `solution.c` and open a PR
