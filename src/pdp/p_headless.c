/* p_headless.c -- headless stubs for the Pd GUI / interaction / backend layer.
 *
 * The vendored Pd message core (m_*), the minimal control operators (x_*) and
 * the supporting s_* files reference a number of symbols that live in layers
 * this project deliberately does NOT compile:
 *
 *   - the GUI / canvas layer        (g_canvas.c, g_scalar.c, g_text.c, ...)
 *   - the interaction layer         (s_inter.c, s_main.c, s_loader.c)
 *   - the audio and MIDI backends   (s_audio*.c, s_midi*.c)
 *   - the DSP engine                (d_ugen.c, ...)
 *
 * See TODO.md, "Option 2: minimal core".  This file supplies those symbols so
 * the core links and runs headless.  Everything here is either inert (we never
 * drive a GUI or audio device) or trivially "empty" (no GUI => 0, no file
 * loading => NULL).
 *
 * Keep in sync with the undefined symbols the linker reports when the vendored
 * file list in CMakeLists.txt changes.
 */
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "m_pd.h"
#include "g_canvas.h"
#include "s_stuff.h"

/* ================================================================== */
/* Interaction layer (s_inter.c / s_main.c / s_inter_gui.c)           */
/* ================================================================== */

double sys_getrealtime(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static int initialised = 0;
    LARGE_INTEGER now;
    if (!initialised)
    {
        QueryPerformanceFrequency(&freq);
        initialised = 1;
    }
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
#endif
}

void sys_lock(void) {}
void sys_unlock(void) {}
void sys_microsleep(void) {}

/* No GUI: nothing to poll, no Tk process, no drawing. */
int  sys_pollgui(void) { return 0; }
int  sys_havegui(void) { return 0; }
int  sys_havetkproc(void) { return 0; }
void sys_vgui(const char *fmt, ...) { (void)fmt; }
void sys_gui(const char *s) { (void)s; }
void pdgui_vmess(const char *message, const char *format, ...)
{
    (void)message; (void)format;
}

/* The GUI message queue is never populated without a GUI. */
void messqueue_dispatch(void) {}

/* Single-threaded headless use: the global lock is a no-op. */
void pd_globallock(void) {}
void pd_globalunlock(void) {}

void s_inter_newpdinstance(void) {}

/* Global flags normally defined in s_main.c. */
int sys_verbose;
int sys_defaultfont;
int sys_schedadvance;
int sys_nosleep = 0;
int sys_hipriority = -1;

/* ================================================================== */
/* Audio backend (s_audio.c) -- inert: we never open a device         */
/* ================================================================== */

int  audio_isopen(void) { return 0; }
int  audio_shouldkeepopen(void) { return 0; }
int  sys_send_dacs(void) { return 0; }
int  sys_try_reopen_audio(void) { return 0; }
void sys_do_close_audio(void) {}
void sys_do_reopen_audio(void) {}

/* ================================================================== */
/* MIDI backend (s_midi.c) -- inert                                   */
/* ================================================================== */

void sys_initmidiqueue(void) {}
void sys_pollmidiqueue(void) {}
void sys_reopen_midi(void) {}
void sys_close_midi(void) {}
void x_midi_newpdinstance(void) {}

/* ================================================================== */
/* DSP engine (d_ugen.c) -- inert: message/control flow only          */
/* ================================================================== */

void dsp_tick(void) {}
void d_ugen_newpdinstance(void) {}

/* ================================================================== */
/* Canvas layer (g_canvas.c / g_editor.c / g_text.c / g_scalar.c)     */
/* ================================================================== */

t_class *canvas_class;

void canvas_settracing(int onoff) { (void)onoff; }
void canvas_update_dsp(void) {}
int  canvas_suspend_dsp(void) { return 0; }
void canvas_resume_dsp(int oldstate) { (void)oldstate; }
void canvas_closebang(t_canvas *x) { (void)x; }
void canvas_popabstraction(t_canvas *x) { (void)x; }
t_glist *canvas_getcurrent(void) { return 0; }
void canvas_finderror(const void *error_object) { (void)error_object; }
int  canvas_getdollarzero(void) { return 0; }
void canvas_initbang(t_canvas *x) { (void)x; }

/* Only referenced as a dispatch sentinel by m_class.c; never invoked. */
t_canvas *canvas_new(void *dummy, t_symbol *sel, int argc, t_atom *argv)
{
    (void)dummy; (void)sel; (void)argc; (void)argv;
    return 0;
}

/* File-open helper used by the loader; no filesystem patch loading. */
int canvas_open(const t_canvas *x, const char *name, const char *ext,
    char *dirresult, char **nameresult, unsigned int size, int bin)
{
    (void)x; (void)name; (void)ext; (void)dirresult; (void)nameresult;
    (void)size; (void)bin;
    return -1;
}

void g_canvas_newpdinstance(void) {}
void g_canvas_freepdinstance(void) {}

/* Zeroed widget behaviour: only its address is taken (m_class.c). */
const t_widgetbehavior text_widgetbehavior = { 0, 0, 0, 0, 0, 0, 0 };

/* gpointers (arrays / data structures) are out of scope. */
void gpointer_init(t_gpointer *gp) { (void)gp; }
void gpointer_copy(const t_gpointer *gpfrom, t_gpointer *gpto)
{
    (void)gpfrom; (void)gpto;
}
void gpointer_unset(t_gpointer *gp) { (void)gp; }
int  gpointer_check(const t_gpointer *gp, int headok)
{
    (void)gp; (void)headok;
    return 0;
}

/* Arrays (g_array.c) are out of scope. */
void garray_init(void) {}

/* ================================================================== */
/* Loader (s_loader.c) -- no external library loading                 */
/* ================================================================== */

int sys_load_lib(t_canvas *canvas, const char *classname)
{
    (void)canvas; (void)classname;
    return 0;
}

/* ================================================================== */
/* File / patch loading (s_file.c) -- out of scope                    */
/* ================================================================== */

t_pd *glob_evalfile(t_pd *ignore, t_symbol *name, t_symbol *dir)
{
    (void)ignore; (void)name; (void)dir;
    return 0;
}

void text_save(t_gobj *z, t_binbuf *b) { (void)z; (void)b; }

/* ================================================================== */
/* GUI menu / dialog callbacks (s_main.c, g_canvas.c).                */
/* Never invoked headless; present only to satisfy method pointers.   */
/* ================================================================== */

void glob_addtopath(t_pd *dummy, t_symbol *path, t_float saveit)
{
    (void)dummy; (void)path; (void)saveit;
}
void glob_watchdog(void *dummy) { (void)dummy; }
void glob_setfilename(void *dummy, t_symbol *name, t_symbol *dir)
{
    (void)dummy; (void)name; (void)dir;
}
void glob_exit(void *dummy, t_float status)
{
    (void)dummy; (void)status;
}

#define PDP_STUB_DIALOG(name) \
    void name(void *dummy, t_symbol *s, int argc, t_atom *argv) \
    { (void)dummy; (void)s; (void)argc; (void)argv; }

PDP_STUB_DIALOG(glob_audio_dialog)
PDP_STUB_DIALOG(glob_audio_properties)
PDP_STUB_DIALOG(glob_audio_setapi)
PDP_STUB_DIALOG(glob_closesubs)
PDP_STUB_DIALOG(glob_colors)
PDP_STUB_DIALOG(glob_dsp)
PDP_STUB_DIALOG(glob_foo)
PDP_STUB_DIALOG(glob_forgetpreferences)
PDP_STUB_DIALOG(glob_initfromgui)
PDP_STUB_DIALOG(glob_key)
PDP_STUB_DIALOG(glob_loadpreferences)
PDP_STUB_DIALOG(glob_menunew)
PDP_STUB_DIALOG(glob_midi_dialog)
PDP_STUB_DIALOG(glob_midi_properties)
PDP_STUB_DIALOG(glob_midi_setapi)
PDP_STUB_DIALOG(glob_open)
PDP_STUB_DIALOG(glob_path_dialog)
PDP_STUB_DIALOG(glob_ping)
PDP_STUB_DIALOG(glob_quit)
PDP_STUB_DIALOG(glob_rescanaudio)
PDP_STUB_DIALOG(glob_rescanmidi)
PDP_STUB_DIALOG(glob_savepreferences)
PDP_STUB_DIALOG(glob_start_path_dialog)
PDP_STUB_DIALOG(glob_start_preference_dialog)
PDP_STUB_DIALOG(glob_start_startup_dialog)
PDP_STUB_DIALOG(glob_startup_dialog)
PDP_STUB_DIALOG(glob_verifyquit)
PDP_STUB_DIALOG(glob_vis)
