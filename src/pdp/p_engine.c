/* p_engine.c -- pdp core initialisation.
 *
 * Brings up the vendored Pd message system and registers pdp's own operators.
 * This is the one place the public API's lifecycle is anchored.
 */
#include "pdp/pdp.h"

/* Provided by the vendored Pd core (m_pd.c). */
extern void pd_init(void);

/* Provided by src/operators/pdp_message.c. */
void pdp_message_setup(void);

void pdp_init(void)
{
    static int initialised = 0;

    if (initialised)
        return;
    initialised = 1;

    pd_init();
    pdp_message_setup();
    pdp_log(PDP_LOG_INFO, "pdp core initialised");
}
