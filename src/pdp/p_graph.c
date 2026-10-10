/* p_graph.c -- pdp graph model: patches, nodes, connections and messaging.
 *
 * A pdp_patch is a lightweight container that owns pdp_nodes.  A pdp_node
 * wraps a Pd t_object created by class name.  Connections reuse Pd's own
 * obj_connect()/obj_disconnect() wiring, so message delivery semantics are
 * exactly Pd's.
 *
 * Nodes do NOT require a canvas to exist (we stubbed the GUI canvas out), so
 * a patch is just ownership + a place to hang convenience API from.
 */
#include <stdlib.h>

#include "pdp/pdp.h"
#include "m_imp.h"   /* obj_connect, obj_disconnect, obj_*traverseoutlet */

struct _pdp_node
{
    t_object  *obj;        /* the underlying Pd object            */
    t_symbol  *classname;  /* the name it was created with        */
    pdp_patch *owner;      /* owning patch, or NULL               */
    pdp_node  *next;       /* next node in the owning patch       */
};

struct _pdp_patch
{
    pdp_node *nodes;
    int       nnodes;
};

/* ------------------------------------------------------------------ */
/* Teardown helpers                                                   */
/* ------------------------------------------------------------------ */

/* Disconnect every connection that leaves `n`. */
static void node_disconnect_from(pdp_node *n)
{
    int outno = 0;

    for (;;)
    {
        t_outlet *op = 0;
        t_outconnect *oc = obj_starttraverseoutlet(n->obj, &op, outno);
        if (!op)
            break;
        while (oc)
        {
            t_object *dest;
            t_inlet *inl;
            int which;
            oc = obj_nexttraverseoutlet(oc, &dest, &inl, &which);
            obj_disconnect(n->obj, outno, dest, which);
            /* the connection list changed; restart this outlet */
            oc = obj_starttraverseoutlet(n->obj, &op, outno);
        }
        outno++;
    }
}

/* Disconnect every connection in `patch` that arrives at `n`. */
static void node_disconnect_into(pdp_patch *patch, pdp_node *n)
{
    pdp_node *m;

    if (!patch)
        return;
    for (m = patch->nodes; m; m = m->next)
    {
        int outno = 0;
        if (m == n)
            continue;
        for (;;)
        {
            t_outlet *op = 0;
            t_outconnect *oc = obj_starttraverseoutlet(m->obj, &op, outno);
            if (!op)
                break;
            while (oc)
            {
                t_object *dest;
                t_inlet *inl;
                int which;
                oc = obj_nexttraverseoutlet(oc, &dest, &inl, &which);
                if (dest == n->obj)
                {
                    obj_disconnect(m->obj, outno, dest, which);
                    oc = obj_starttraverseoutlet(m->obj, &op, outno);
                }
            }
            outno++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Patch                                                              */
/* ------------------------------------------------------------------ */

pdp_patch *pdp_patch_new(void)
{
    return (pdp_patch *)calloc(1, sizeof(pdp_patch));
}

void pdp_patch_free(pdp_patch *patch)
{
    if (!patch)
        return;
    while (patch->nodes)
        pdp_node_free(patch->nodes);
    free(patch);
}

/* ------------------------------------------------------------------ */
/* Node                                                               */
/* ------------------------------------------------------------------ */

pdp_node *pdp_node_new(pdp_patch *patch, const char *classname,
    int argc, const t_atom *argv)
{
    t_symbol *sel;
    t_pd *before, *pd;
    pdp_node *n;

    if (!classname)
        return 0;

    sel = gensym(classname);
    before = pd_newest();
    pd_typedmess(&pd_objectmaker, sel, argc, (t_atom *)argv);
    pd = pd_newest();
    if (!pd || pd == before)
    {
        pdp_log(PDP_LOG_ERROR, "could not create object '%s'", classname);
        return 0;
    }

    n = (pdp_node *)calloc(1, sizeof(pdp_node));
    n->obj = (t_object *)pd;
    n->classname = sel;
    n->owner = patch;
    if (patch)
    {
        n->next = patch->nodes;
        patch->nodes = n;
        patch->nnodes++;
    }
    pdp_log(PDP_LOG_DEBUG, "node %p: class '%s' argc=%d", (void *)n,
        classname, argc);
    return n;
}

void pdp_node_free(pdp_node *node)
{
    if (!node)
        return;

    node_disconnect_from(node);
    node_disconnect_into(node->owner, node);

    if (node->owner)
    {
        pdp_node **pp = &node->owner->nodes;
        while (*pp && *pp != node)
            pp = &(*pp)->next;
        if (*pp)
            *pp = node->next;
        node->owner->nnodes--;
    }
    pd_free((t_pd *)node->obj);
    free(node);
}

const char *pdp_node_class(const pdp_node *node)
{
    return (node && node->classname) ? node->classname->s_name : 0;
}

/* ------------------------------------------------------------------ */
/* Connections                                                        */
/* ------------------------------------------------------------------ */

int pdp_connect(pdp_patch *patch, pdp_node *src, int outno,
    pdp_node *sink, int inno)
{
    int rc;
    (void)patch;
    if (!src || !sink)
        return -1;
    rc = obj_connect(src->obj, outno, sink->obj, inno) ? 0 : -1;
    pdp_log(PDP_LOG_DEBUG, "connect %p[%d] -> %p[%d]: %s", (void *)src, outno,
        (void *)sink, inno, rc ? "failed" : "ok");
    return rc;
}

int pdp_disconnect(pdp_patch *patch, pdp_node *src, int outno,
    pdp_node *sink, int inno)
{
    (void)patch;
    if (!src || !sink)
        return -1;
    obj_disconnect(src->obj, outno, sink->obj, inno);
    pdp_log(PDP_LOG_DEBUG, "disconnect %p[%d] -> %p[%d]", (void *)src, outno,
        (void *)sink, inno);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Messaging                                                          */
/* ------------------------------------------------------------------ */

void pdp_bang(pdp_node *node)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send 'bang' -> %p", (void *)node);
    pd_bang((t_pd *)node->obj);
}

void pdp_float(pdp_node *node, t_float f)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send 'float %g' -> %p", (double)f, (void *)node);
    pd_float((t_pd *)node->obj, f);
}

void pdp_symbol(pdp_node *node, t_symbol *s)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send 'symbol %s' -> %p",
        s ? s->s_name : "(null)", (void *)node);
    pd_symbol((t_pd *)node->obj, s);
}

void pdp_list(pdp_node *node, int argc, const t_atom *argv)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send 'list' argc=%d -> %p", argc, (void *)node);
    pd_list((t_pd *)node->obj, &s_list, argc, (t_atom *)argv);
}

void pdp_anything(pdp_node *node, t_symbol *selector, int argc,
    const t_atom *argv)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send '%s' argc=%d -> %p",
        selector ? selector->s_name : "(null)", argc, (void *)node);
    pd_anything((t_pd *)node->obj, selector, argc, (t_atom *)argv);
}

void pdp_send(pdp_node *node, const char *selector, int argc,
    const t_atom *argv)
{
    if (!node)
        return;
    pdp_log(PDP_LOG_TRACE, "send '%s' argc=%d -> %p", selector, argc,
        (void *)node);
    pd_typedmess((t_pd *)node->obj, gensym(selector), argc, (t_atom *)argv);
}
