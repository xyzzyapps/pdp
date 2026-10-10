/* pdp_message.c -- a minimal in-tree "message" box.
 *
 * The upstream `message` class lives in the GUI layer (g_text.c), which pdp
 * does not compile (see TODO.md).  This is a small headless replacement with
 * the same essential behaviour: it stores a constant message and, when banged,
 * emits it:
 *
 *   - no atoms            -> bang
 *   - one float           -> float
 *   - one symbol          -> symbol
 *   - many atoms, 1st sym -> "anything" with that selector
 *   - otherwise           -> list
 *
 * `$`-substitution and the `set`/`add`/`add2` editors are out of scope; `set`
 * is supported so contents can be replaced.
 */
#include <string.h>

#include "m_pd.h"

typedef struct _pdp_message
{
    t_object x_obj;
    int      x_n;      /* number of atoms in the stored message */
    t_atom  *x_vec;    /* the stored message, or NULL if empty */
} t_pdp_message;

static t_class *pdp_message_class;

/* Replace the stored message contents. */
static void pdp_message_set(t_pdp_message *x, t_symbol *s, int argc,
    t_atom *argv)
{
    (void)s;

    if (x->x_vec)
    {
        t_freebytes(x->x_vec, x->x_n * (int)sizeof(t_atom));
        x->x_vec = 0;
    }
    x->x_n = argc;
    if (argc > 0)
    {
        x->x_vec = (t_atom *)t_getbytes(argc * (int)sizeof(t_atom));
        memcpy(x->x_vec, argv, argc * (size_t)sizeof(t_atom));
    }
}

/* Emit the stored message out outlet 0. */
static void pdp_message_emit(t_pdp_message *x)
{
    if (x->x_n <= 0)
        outlet_bang(x->x_obj.ob_outlet);
    else if (x->x_n == 1)
    {
        if (x->x_vec[0].a_type == A_FLOAT)
            outlet_float(x->x_obj.ob_outlet, x->x_vec[0].a_w.w_float);
        else if (x->x_vec[0].a_type == A_SYMBOL)
            outlet_symbol(x->x_obj.ob_outlet, x->x_vec[0].a_w.w_symbol);
        else
            outlet_list(x->x_obj.ob_outlet, &s_list, x->x_n, x->x_vec);
    }
    else if (x->x_vec[0].a_type == A_SYMBOL)
        outlet_anything(x->x_obj.ob_outlet, x->x_vec[0].a_w.w_symbol,
            x->x_n - 1, x->x_vec + 1);
    else
        outlet_list(x->x_obj.ob_outlet, &s_list, x->x_n, x->x_vec);
}

static void pdp_message_bang(t_pdp_message *x)
{
    pdp_message_emit(x);
}

static void pdp_message_free(t_pdp_message *x)
{
    if (x->x_vec)
        t_freebytes(x->x_vec, x->x_n * (int)sizeof(t_atom));
}

static void *pdp_message_new(t_symbol *s, int argc, t_atom *argv)
{
    t_pdp_message *x = (t_pdp_message *)pd_new(pdp_message_class);

    x->x_n = 0;
    x->x_vec = 0;
    outlet_new(&x->x_obj, &s_list);
    if (argc > 0)
        pdp_message_set(x, s, argc, argv);
    return x;
}

void pdp_message_setup(void)
{
    pdp_message_class = class_new(gensym("message"),
        (t_newmethod)pdp_message_new, (t_method)pdp_message_free,
        sizeof(t_pdp_message), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(pdp_message_class, (t_method)pdp_message_bang);
    class_addmethod(pdp_message_class, (t_method)pdp_message_set,
        gensym("set"), A_GIMME, 0);
}
