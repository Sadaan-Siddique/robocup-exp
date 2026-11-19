#ifndef DMA_ASSIGNMENT_H
#define DMA_ASSIGNMENT_H

#include <stdbool.h>
#include <stdint.h>

#define BATCH_SIZE 32u

enum {
    DMA_ENABLE = 1u << 0,
    DMA_P2M = 1u << 1,
    DMA_WIDTH_16 = 1u << 2,
    DMA_DST_INC = 1u << 3,
    DMA_SRC_INC = 1u << 4,
    DMA_COMPLETE_IE = 1u << 5,
    DMA_ERROR_IE = 1u << 6
};
enum { DMA_COMPLETE = 1u << 0, DMA_ERROR = 1u << 1 };

typedef struct {
    volatile uintptr_t SRC;
    volatile uintptr_t DST;
    volatile uint32_t COUNT; /* Number of 16-bit samples remaining, not bytes. */
    volatile uint32_t CTRL;
    volatile uint32_t STATUS; /* Read flags; acknowledge with dma_clear_flags(). */
} DmaRegisters;

/* Simulator model: one-shot transfers stop on completion or error.
 * SRC and DST retain their configured base addresses; COUNT decreases.
 * DMA writes the buffer before raising completion. Calls execute serially. */
extern DmaRegisters DMA;
extern volatile uint16_t SENSOR_DATA;

typedef struct {
    uint16_t min;
    uint16_t max;
    uint32_t sum;
    uint32_t mean; /* Floor of sum / BATCH_SIZE. */
} BatchResult;

/* Supplied by the simulator. Flags persist until explicitly acknowledged. */
void dma_clear_flags(uint32_t flags);

/* notify... is ISR-only. take... is worker-only and nonblocking:
 * return true and clear a pending notification, or return false if none.
 * Interrupts and task steps execute serially in this simulator. */
void notify_worker_from_isr(void);
bool take_worker_notification(void);

/* Worker-only output hooks. A completed batch produces exactly one of these. */
void report_result(const BatchResult *result);
void report_error(void);

/* Implement these in solution.c. */
void app_init(void);
bool app_start(void);
void app_dma_irq(void);
void app_task_step(void);

#endif
