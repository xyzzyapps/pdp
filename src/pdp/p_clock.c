/* p_clock.c -- deterministic scheduler clock for pdp.
 *
 * Pd's scheduler advances its logical time (pd_this->pd_systime) by
 * SYSTIMEPERTICK each time sched_tick() runs, firing any clocks that are due
 * (this is what drives metro, delay, ...).  pdp drives that explicitly rather
 * than from wall time, so tests and CLIs are deterministic.
 */
#include "pdp/pdp.h"

/* Defined in the vendored Pd scheduler (m_sched.c). */
extern void sched_tick(void);

/* TIMEUNITPERMSEC from m_sched.c (32 * 441): Pd's internal ticks per ms. */
#define PDP_TIMEUNITPERMSEC 14112.0

void pdp_clock_advance(double ms)
{
    double target;
    long guard = 0;

    if (ms <= 0)
        return;

    target = pd_this->pd_systime + ms * PDP_TIMEUNITPERMSEC;
    while (pd_this->pd_systime < target)
    {
        sched_tick();
        if (++guard > 100000000L)
            break;
    }
    pdp_log(PDP_LOG_TRACE, "clock advance %g ms -> %g ms", ms,
        pd_this->pd_systime / PDP_TIMEUNITPERMSEC);
}

double pdp_clock_now(void)
{
    return pd_this->pd_systime / PDP_TIMEUNITPERMSEC;
}
