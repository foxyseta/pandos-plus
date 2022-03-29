/**
 * \file native_scheduler.h
 * \brief Implementation of native scheduler utilities.
 *
 * \author Luca Tagliavini
 * \date 22-03-2022
 */

#ifndef PANDOS_NATIVE_SCHEDULER_H
#define PANDOS_NATIVE_SCHEDULER_H

#include "os/types.h"

extern state_t *wait_state;

extern void reset_timer();
extern void reset_plt();

#endif /* PANDOS_NATIVE_SCHEDULER_H */
