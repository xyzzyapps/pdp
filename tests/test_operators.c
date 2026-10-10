/* test_operators.c -- a sample of the built-in control operators. */
#include "support/pdp_test.h"
#include "support/pdp_test_sink.h"

int main(void)
{
    pdp_patch *patch;
    pdp_node *s, *n;
    t_atom a[1];

    pdp_init();
    pdptest_sink_setup();
    patch = pdp_patch_new();
    s = pdp_node_new(patch, "pdptest_sink", 0, 0);

    /* + 3: float 4 in -> float 7 out */
    SETFLOAT(&a[0], 3.);
    n = pdp_node_new(patch, "+", 1, a);
    PDP_CHECK(n != 0);
    pdp_connect(patch, n, 0, s, 0);
    pdptest_sink_reset();
    pdp_float(n, 4.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);
    PDP_CHECK(pdptest_sink_sel == gensym("float"));
    PDP_CHECK(pdptest_sink_argv[0].a_w.w_float == 7.);

    /* float: holds and emits its value */
    n = pdp_node_new(patch, "float", 0, 0);
    PDP_CHECK(n != 0);
    pdp_connect(patch, n, 0, s, 0);
    pdptest_sink_reset();
    pdp_float(n, 5.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);
    PDP_CHECK(pdptest_sink_argv[0].a_w.w_float == 5.);

    /* select 3: emits bang when its value arrives */
    SETFLOAT(&a[0], 3.);
    n = pdp_node_new(patch, "select", 1, a);
    PDP_CHECK(n != 0);
    pdp_connect(patch, n, 0, s, 0);
    pdptest_sink_reset();
    pdp_float(n, 3.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);
    PDP_CHECK(pdptest_sink_sel == gensym("bang"));

    pdp_patch_free(patch);
    PDP_TEST_MAIN_END();
}
