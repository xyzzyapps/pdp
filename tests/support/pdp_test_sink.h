/* pdp_test_sink.h -- a test-only operator that records the last message it
 * receives, so tests can observe what flowed through the graph.
 *
 * Header-only with static state: each test executable is a single translation
 * unit, so it gets its own copy.  Call pdptest_sink_setup() once after
 * pdp_init(), then create a node named "pdptest_sink".
 */
#ifndef PDP_TEST_SINK_H
#define PDP_TEST_SINK_H

#include "pdp/pdp.h"

#define PDPTEST_SINK_MAXARGS 32

static t_symbol *pdptest_sink_sel;
static int       pdptest_sink_argc;
static t_atom    pdptest_sink_argv[PDPTEST_SINK_MAXARGS];
static int       pdptest_sink_count;

typedef struct _pdptest_sink { t_object x_obj; } t_pdptest_sink;

static t_class *pdptest_sink_class;

static void pdptest_sink_record(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    pdptest_sink_sel = s;
    pdptest_sink_argc = argc;
    for (i = 0; i < argc && i < PDPTEST_SINK_MAXARGS; i++)
        pdptest_sink_argv[i] = argv[i];
    pdptest_sink_count++;
}

static void pdptest_sink_reset(void)
{
    pdptest_sink_sel = 0;
    pdptest_sink_argc = 0;
    pdptest_sink_count = 0;
}

static void pdptest_sink_bang(t_pdptest_sink *x)
{
    (void)x;
    pdptest_sink_record(&s_bang, 0, 0);
}

static void pdptest_sink_float(t_pdptest_sink *x, t_float f)
{
    t_atom a;
    (void)x;
    SETFLOAT(&a, f);
    pdptest_sink_record(&s_float, 1, &a);
}

static void pdptest_sink_symbol(t_pdptest_sink *x, t_symbol *s)
{
    t_atom a;
    (void)x;
    SETSYMBOL(&a, s);
    pdptest_sink_record(&s_symbol, 1, &a);
}

static void pdptest_sink_list(t_pdptest_sink *x, t_symbol *s, int argc,
    t_atom *argv)
{
    (void)x;
    pdptest_sink_record(s, argc, argv);
}

static void pdptest_sink_any(t_pdptest_sink *x, t_symbol *s, int argc,
    t_atom *argv)
{
    (void)x;
    pdptest_sink_record(s, argc, argv);
}

static void *pdptest_sink_new(void)
{
    return pd_new(pdptest_sink_class);
}

static void pdptest_sink_setup(void)
{
    pdptest_sink_class = class_new(gensym("pdptest_sink"),
        (t_newmethod)pdptest_sink_new, 0, sizeof(t_pdptest_sink),
        CLASS_DEFAULT, 0);
    class_addbang(pdptest_sink_class, (t_method)pdptest_sink_bang);
    class_addfloat(pdptest_sink_class, (t_method)pdptest_sink_float);
    class_addsymbol(pdptest_sink_class, (t_method)pdptest_sink_symbol);
    class_addlist(pdptest_sink_class, (t_method)pdptest_sink_list);
    class_addanything(pdptest_sink_class, (t_method)pdptest_sink_any);
}

#endif /* PDP_TEST_SINK_H */
