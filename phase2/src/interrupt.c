/**
 * \file interrupt.c
 * \brief Interrupt and trap handler
 *
 * \author Alessandro Frau
 * \author Gianmaria Rovelli
 * \author Luca Tagliavini
 * \date 20-03-2022
 */
#include "interrupt.h"
#include "native_scheduler.h"
#include "os/const.h"
#include "os/scheduler.h"
#include "os/semaphores.h"
#include "os/util.h"
#include <umps/arch.h>
#include <umps/libumps.h>

static inline memaddr *get_terminal_transm_status(int devicenumber)
{
    return (memaddr *)TERMINAL_TRANSM_STATUS(devicenumber);
}

static inline memaddr *get_terminal_recv_status(int devicenumber)
{
    return (memaddr *)TERMINAL_RECV_STATUS(devicenumber);
}

static inline memaddr *get_terminal_transm_command(int devicenumber)
{
    return (memaddr *)TERMINAL_TRANSM_COMMAND(devicenumber);
}

static inline memaddr *get_terminal_recv_command(int devicenumber)
{
    return (memaddr *)TERMINAL_RECV_COMMAND(devicenumber);
}

/*find_device_number() viene utilizzato per identificare il numero del device
 * che ha sollevato l'interrupt */
int find_device_number(memaddr *bitmap)
{
    int device_n = 0;

    while (*bitmap > 1) {
        device_n++;
        *bitmap >>= 1;
    }
    return device_n;
}

static inline scheduler_control_t interrupt_ipi()
{
    /* ACK_IPI; */
    /* TODO */

    /* TODO: Change this return, I just guessed */
    return CONTROL_PRESERVE(active_process);
}

static inline scheduler_control_t interrupt_local_timer()
{
    reset_plt();
    return CONTROL_RESCHEDULE;
}

static inline scheduler_control_t interrupt_timer()
{
    reset_timer();
    pcb_t *p;
    while ((p = V(&timer_semaphore)) != NULL) {
        // verbose("REMOVE from clock %d\n", act_pid);
    }
    timer_semaphore = 0;
    return CONTROL_PRESERVE(active_process);
}

static inline scheduler_control_t interrupt_generic(int cause)
{

    /* TODO */
    int il = IL_DISK;
    int *sem[] = {disk_semaphores, tape_semaphores, ethernet_semaphores,
                  printer_semaphores};
    /* inverse priority */
    for (int i = IL_DISK; i < IL_PRINTER; i++) {
        if (CAUSE_IP_GET(cause, i)) {
            il = i;
            break;
        }
    }

    int devicenumber = find_device_number((memaddr *)CDEV_BITMAP_ADDR(il));
    pcb_t *p = V(&sem[il - IL_DISK][devicenumber]);
    scheduler_control_t ctrl = mask_V(p);
    /* 		if( (unblocked = V(&sem_ethernet[devicenumber])) !=
        NULL){
                                    //Se un processo ha chiamato la SYS dei
        TAPE, V() lo sblocca e aggiorna lo status unblocked->p_s.reg_v0 =
        *DEVICE_STATUS(il, devicenumber);
                                    insertProcQ(&ready_queue[unblocked->numCPU],
        unblocked);
                            } */

    /* ACK al device */
    *DEVICE_COMMAND(il, devicenumber) = DEV_C_ACK;
    return ctrl;
}

static inline scheduler_control_t interrupt_terminal()
{

    /*Scandiamo la BITMAP_TERMINALDEVICE per identificare quale terminale ha
     * sollevato l'interrupt*/
    int devicenumber =
        find_device_number((memaddr *)CDEV_BITMAP_ADDR(IL_TERMINAL));

    /* TODO : order is important, check */
    memaddr *(*get_status[2])(int dev) = {get_terminal_transm_status,
                                          get_terminal_recv_status};
    memaddr *(*get_cmd[2])(int dev) = {get_terminal_transm_command,
                                       get_terminal_recv_command};
    int *sem[] = {termw_semaphores, termr_semaphores};

    // pandos_kfprintf(&kverb, "\n[-] TERM INT START (%d)\n", act_pid);
    for (int i = 0; i < 2; i++) {
        int status = *get_status[i](devicenumber);
        if ((status & TERMSTATMASK) != DEV_S_READY) {
            pcb_t *p = V(&sem[i][devicenumber]);
            scheduler_control_t ctrl = mask_V(p);
            // int pid = 0;
            if (p == NULL) {
                active_process->p_s.reg_v0 = status;
                // pid = active_process->p_pid;
            } else {
                p->p_s.reg_v0 = status;
                // pid = p->p_pid;
                // pandos_kfprintf(&kverb, "   SIZE (%p)\n",
                // list_size(&ready_queue_lo));
            }

            // pandos_kfprintf(&kverb, "   STATUS of (%d) (%p)\n", pid,
            // status);

            *get_cmd[i](devicenumber) = DEV_C_ACK;
            /* do the first one */
            return ctrl;
        }
    }
    pandos_kfprintf(&kverb, "WTF TERMINAL DID NOT RETURN\n");
    /* PANIC()?*/
    PANIC();
    /* Just to return something */
    return CONTROL_BLOCK;
}

scheduler_control_t interrupt_handler()
{
    int cause = getCAUSE();
    int act_pid = -1;
    if (active_process != NULL) {
        act_pid = active_process->p_pid;
    }

    if (CAUSE_IP_GET(cause, IL_IPI)) {
        pandos_kprintf(">> INTERRUPT: IL_IPI");
        return interrupt_ipi();
    } else if (CAUSE_IP_GET(cause, IL_LOCAL_TIMER)) {
        pandos_kprintf("<< INTERRUPT(LOCAL_TIMER, (%d) , %d)\n", act_pid,
                       (int)getTIMER());
        return interrupt_local_timer();
    } else if (CAUSE_IP_GET(cause, IL_TIMER)) {
        pandos_kprintf("<< INTERRUPT(TIMER, (%d))\n", act_pid);
        return interrupt_timer();
    } else if (CAUSE_IP_GET(cause, IL_DISK) || CAUSE_IP_GET(cause, IL_FLASH) ||
               CAUSE_IP_GET(cause, IL_ETHERNET) ||
               CAUSE_IP_GET(cause, IL_PRINTER)) {
        pandos_kprintf("<< INTERRUPT(GENERIC)\n");

        return interrupt_generic(cause);

    } else if (CAUSE_IP_GET(cause, IL_TERMINAL)) {
        pandos_kprintf("<< INTERRUPT(TERMINAL, (%d))\n", act_pid);
        return interrupt_terminal();
    } else {
        pandos_kprintf("<< INTERRUPT(UNKNOWN)\n");
    }

    /* The newly unblocked pcb is enqueued back on the Ready Queue and control
     * is returned to the Current Process unless the newly unblocked process
     * has higher prority of the Current Process.
     * */

    return CONTROL_RESCHEDULE;
}
