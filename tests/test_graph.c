/* test_graph.c -- graph model: patch, node, connection, messaging. */
#include <string.h>

#include "support/pdp_test.h"
#include "support/pdp_test_sink.h"

int main(void)
{
    pdp_patch *patch;
    pdp_node *b, *s;

    pdp_init();
    pdptest_sink_setup();

    patch = pdp_patch_new();
    PDP_CHECK(patch != 0);

    b = pdp_node_new(patch, "bang", 0, 0);
    s = pdp_node_new(patch, "pdptest_sink", 0, 0);
    PDP_CHECK(b != 0);
    PDP_CHECK(s != 0);
    PDP_CHECK(pdp_node_class(b) && !strcmp(pdp_node_class(b), "bang"));

    /* connect and deliver a bang */
    PDP_CHECK_EQ_INT(pdp_connect(patch, b, 0, s, 0), 0);
    pdptest_sink_reset();
    pdp_bang(b);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 1);
    PDP_CHECK(pdptest_sink_sel == gensym("bang"));

    /* disconnect stops delivery */
    PDP_CHECK_EQ_INT(pdp_disconnect(patch, b, 0, s, 0), 0);
    pdptest_sink_reset();
    pdp_bang(b);
    PDP_CHECK_EQ_INT(pdptest_sink_count, 0);

    /* unknown class fails cleanly, no node leaked into the patch */
    PDP_CHECK(pdp_node_new(patch, "no_such_class_xyz", 0, 0) == 0);

    /* freeing a node must not disturb the rest of the graph */
    pdp_node_free(b);
    PDP_CHECK(pdp_node_class(s) && !strcmp(pdp_node_class(s), "pdptest_sink"));

    pdp_patch_free(patch);
    PDP_TEST_MAIN_END();
}
