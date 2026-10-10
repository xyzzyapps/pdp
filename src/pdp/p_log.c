/* p_log.c -- lightweight, dependency-free logging for pdp.
 *
 * A threshold plus an optional pluggable sink.  The default sink writes to
 * stderr.  pdp's own operations (graph construction, connections, message
 * injection, clock advances) log here so a run can be traced end to end.
 */
#include <stdarg.h>
#include <stdio.h>

#include "pdp/pdp.h"

static pdp_log_level pdp_log_threshold = PDP_LOG_WARN;
static pdp_log_handler pdp_log_userhandler = 0;
static void *pdp_log_userdata = 0;

static const char *pdp_log_levelname(pdp_log_level level)
{
    switch (level)
    {
        case PDP_LOG_ERROR: return "error";
        case PDP_LOG_WARN:  return "warn";
        case PDP_LOG_INFO:  return "info";
        case PDP_LOG_DEBUG: return "debug";
        case PDP_LOG_TRACE: return "trace";
        default:            return "?";
    }
}

static void pdp_log_default(pdp_log_level level, const char *message,
    void *userdata)
{
    (void)userdata;
    fprintf(stderr, "[pdp][%s] %s\n", pdp_log_levelname(level), message);
}

void pdp_log_set_level(pdp_log_level level)
{
    pdp_log_threshold = level;
}

pdp_log_level pdp_log_get_level(void)
{
    return pdp_log_threshold;
}

void pdp_log_set_handler(pdp_log_handler handler, void *userdata)
{
    pdp_log_userhandler = handler;
    pdp_log_userdata = userdata;
}

void pdp_log(pdp_log_level level, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    if (level > pdp_log_threshold)
        return;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (pdp_log_userhandler)
        pdp_log_userhandler(level, buf, pdp_log_userdata);
    else
        pdp_log_default(level, buf, 0);
}
