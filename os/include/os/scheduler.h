/**
 * \file scheduler.h
 * \brief Interfaces for the process scheduler.
 *
 * \author Luca Tagliavini
 * \author Stefano Volpe
 * \date 20-03-2022
 */

#ifndef PANDOS_SCHEDULER_H
#define PANDOS_SCHEDULER_H

#include "os/list.h"
#include "os/pcb.h"
#include "os/types.h"

extern int running_count;
extern int blocked_count;
extern list_head ready_queue_lo, ready_queue_hi;
extern pcb_t *active_process;
extern pcb_t *yield_process;
extern cpu_t start_tod;
extern cpu_t last_plt;
extern state_t *wait_state;

/**
 * \brief Spawns a process and returns the allocated structure.
 * \param[in] priority The proprity of the spawned process. Either 1 for high or
 * 0 for low.
 * \return The allocated process descriptor.
 */
extern pcb_t *spawn_process(bool priority);
extern int kill_progeny(pcb_t *p);

extern void enqueue_process(pcb_t *p);
extern pcb_t *dequeue_process(pcb_t *p);
extern pcb_t *const find_process(pandos_pid_t pid);

extern void init_scheduler();
void schedule(pcb_t *pcb, bool enqueue);
void scheduler_panic(const char *fmt, ...);

#endif /* PANDOS_SCHEDULER_H */
