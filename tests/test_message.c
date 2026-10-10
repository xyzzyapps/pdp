/* test_message.c -- the in-tree message box operator. */
#include "support/pdp_test.h"
#include "support/pdp_test_sink.h"

int main(void)
{
    pdp_patch *patch;
    pdp_node *m, *s;
    t_atom a[2];

    pdp_init();
    pdptest_sink_setup();
    patch = pdp_patch_new();
    s = pdp_node_new(patch, "pdptest_sink", 0, 0);

    /* one symbol -> symbol */
    SETSYMBOL(&a[0], gensym("hello"));
    m = pdp_node_new(patch, "message", 1, a);
    PDP_CHECK(m != 0);
    pdp_connect(patch, m, 0, s, 0);
    pdptest_sink_reset();
    pdp_bang(m);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);
    PDP_CHECK(pdptest_sink_sel == gensym("symbol"));
    PDP_CHECK_EQ_INT(pdptest_sink_argc, 1);
    PDP_CHECK(pdptest_sink_argv[0].a_w.w_symbol == gensym("hello"));

    /* one float -> float */
    SETFLOAT(&a[0], 42.);
    pdp_send(m, "set", 1, a);
    pdptest_sink_reset();
    pdp_bang(m);
    PDP_CHECK(pdptest_sink_sel == gensym("float"));
    PDP_CHECK(pdptest_sink_argv[0].a_w.w_float == 42.);

    /* empty -> bang */
    pdp_send(m, "set", 0, 0);
    pdptest_sink_reset();
    pdp_bang(m);
    PDP_CHECK(pdptest_sink_sel == gensym("bang"));

    /* "foo 1 2" -> anything foo with two args */
    SETSYMBOL(&a[0], gensym("foo"));
    SETFLOAT(&a[1], 1.);
    pdp_send(m, "set", 2, a);
    pdptest_sink_reset();
    pdp_bang(m);
    PDP_CHECK(pdptest_sink_sel == gensym("foo"));
    PDP_CHECK_EQ_INT(pdptest_sink_argc, 1);
    PDP_CHECK(pdptest_sink_argv[0].a_w.w_float == 1.);

    pdp_patch_free(patch);
    PDP_TEST_MAIN_END();
}
