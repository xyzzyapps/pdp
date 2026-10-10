/* test_clock.c -- deterministic clock driving metro and delay. */
#include "support/pdp_test.h"
#include "support/pdp_test_sink.h"

int main(void)
{
    pdp_patch *patch;
    pdp_node *metro, *delay, *s;
    t_atom a[1];

    pdp_init();
    pdptest_sink_setup();
    patch = pdp_patch_new();
    s = pdp_node_new(patch, "pdptest_sink", 0, 0);

    /* metro 100: bangs on start, then at 100, 200 */
    SETFLOAT(&a[0], 100.);
    metro = pdp_node_new(patch, "metro", 1, a);
    PDP_CHECK(metro != 0);
    pdp_connect(patch, metro, 0, s, 0);
    pdptest_sink_reset();
    pdp_bang(metro);
    pdp_clock_advance(250.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 3);
    pdp_send(metro, "stop", 0, 0);

    /* delay 100: nothing at +50, one bang at +60 */
    delay = pdp_node_new(patch, "delay", 1, a);
    PDP_CHECK(delay != 0);
    pdp_connect(patch, delay, 0, s, 0);
    pdptest_sink_reset();
    pdp_bang(delay);
    pdp_clock_advance(50.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 0);
    pdp_clock_advance(60.);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);

    pdp_patch_free(patch);
    PDP_TEST_MAIN_END();
}
