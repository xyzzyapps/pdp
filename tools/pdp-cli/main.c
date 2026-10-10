/* pdp-cli -- command-line harness for the pdp headless dataflow core.
 *
 * Smoke test for the public API: build a small graph with no canvas and no
 * GUI, fire messages through it, and drive the deterministic clock.
 */
#include <stdio.h>
#include <string.h>

#include "pdp/pdp.h"

int main(int argc, char **argv)
{
    pdp_patch *patch;
    pdp_node *m, *p, *metro;
    t_atom a[1];

    if (argc > 1 && !strcmp(argv[1], "-v"))
        pdp_log_set_level(PDP_LOG_TRACE);

    pdp_init();
    post("pdp: headless core initialised");

    patch = pdp_patch_new();

    /* message "hello" -> print */
    SETSYMBOL(&a[0], gensym("hello"));
    m = pdp_node_new(patch, "message", 1, a);
    p = pdp_node_new(patch, "print", 0, 0);
    if (!m || !p)
    {
        post("pdp: node creation failed");
        return 1;
    }
    pdp_connect(patch, m, 0, p, 0);
    pdp_bang(m);
    post("pdp: message -> print ok");

    /* metro 100 -> print; advancing 350 ms should fire ~3 bangs */
    SETFLOAT(&a[0], 100.);
    metro = pdp_node_new(patch, "metro", 1, a);
    pdp_connect(patch, metro, 0, p, 0);
    pdp_bang(metro);
    pdp_clock_advance(350.);
    post("pdp: clock advanced to %.0f ms", pdp_clock_now());

    pdp_patch_free(patch);
    post("pdp: done");
    return 0;
}
