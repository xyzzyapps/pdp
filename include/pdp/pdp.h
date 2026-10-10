/* pdp.h -- public C API for the pdp headless dataflow core.
 *
 * pdp is a fork of Pure Data reduced to its message/control dataflow engine:
 * no GUI, no audio, no DSP.
 *
 * Types come straight from Pd's m_pd.h (t_atom, t_symbol, t_float and the
 * SETFLOAT/SETSYMBOL/A_FLOAT macros), so operators and messages are written in
 * familiar Pd style.
 */
#ifndef PDP_H
#define PDP_H

#include "m_pd.h"   /* t_atom, t_symbol, t_float, atom macros */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

/* Initialise the pdp headless core.  Idempotent.  Must be called before
 * creating any nodes. */
void pdp_init(void);

/* ------------------------------------------------------------------ */
/* Graph                                                              */
/* ------------------------------------------------------------------ */

typedef struct _pdp_patch pdp_patch;   /* owns nodes; groups a graph   */
typedef struct _pdp_node  pdp_node;    /* an object instance (t_object)*/

pdp_patch  *pdp_patch_new(void);
void        pdp_patch_free(pdp_patch *patch);

/* Create a node from a class name, exactly as a patch does
 * (e.g. "bang", "float", "+", "metro", "message").  argc/argv are the
 * creation arguments; either may be zero/NULL.  Returns NULL on failure. */
pdp_node   *pdp_node_new(pdp_patch *patch, const char *classname,
                         int argc, const t_atom *argv);
void        pdp_node_free(pdp_node *node);

/* The class name the node was created with. */
const char *pdp_node_class(const pdp_node *node);

/* Connect src outlet outno to sink inlet inno.
 * Return 0 on success, -1 on error. */
int         pdp_connect(pdp_patch *patch, pdp_node *src, int outno,
                        pdp_node *sink, int inno);
int         pdp_disconnect(pdp_patch *patch, pdp_node *src, int outno,
                           pdp_node *sink, int inno);

/* ------------------------------------------------------------------ */
/* Messaging into a node                                              */
/* ------------------------------------------------------------------ */

void        pdp_bang(pdp_node *node);
void        pdp_float(pdp_node *node, t_float f);
void        pdp_symbol(pdp_node *node, t_symbol *s);
void        pdp_list(pdp_node *node, int argc, const t_atom *argv);
void        pdp_anything(pdp_node *node, t_symbol *selector,
                         int argc, const t_atom *argv);

/* Send an arbitrary method selector (e.g. "stop", "set"). */
void        pdp_send(pdp_node *node, const char *selector,
                     int argc, const t_atom *argv);

/* ------------------------------------------------------------------ */
/* Deterministic clock                                                */
/* ------------------------------------------------------------------ */

/* Advance the scheduler clock by `ms` milliseconds, firing any due clocks
 * (metro, delay).  Uses the virtual clock, not wall time, so runs are
 * deterministic. */
void        pdp_clock_advance(double ms);

/* Current scheduler time in milliseconds. */
double      pdp_clock_now(void);

/* ------------------------------------------------------------------ */
/* Logging                                                            */
/* ------------------------------------------------------------------ */

typedef enum
{
    PDP_LOG_ERROR = 0,
    PDP_LOG_WARN  = 1,
    PDP_LOG_INFO  = 2,
    PDP_LOG_DEBUG = 3,
    PDP_LOG_TRACE = 4
} pdp_log_level;

/* Called for every record that passes the current threshold. */
typedef void (*pdp_log_handler)(pdp_log_level level, const char *message,
                                void *userdata);

void          pdp_log_set_level(pdp_log_level level);
pdp_log_level pdp_log_get_level(void);

/* Install a custom sink; pass NULL to restore the default (stderr). */
void          pdp_log_set_handler(pdp_log_handler handler, void *userdata);

void          pdp_log(pdp_log_level level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PDP_H */
