# pdp — Specification (SRS)

Status: living document. Keep in sync with the code.

## 1. Purpose

`pdp` is a **headless dataflow core**, forked from Pure Data (Pd). It keeps
Pd's message/control dataflow engine — atoms, symbols, classes, objects,
inlets/outlets, connections, the message scheduler and clocks — and discards
everything that is not core:

- no GUI (no tcl/tk frontend, no canvas drawing),
- no audio or MIDI backends,
- no DSP / signal graph.

It is packaged as a small C library with a clean C API, plus a command-line
harness for testing and scripting.

## 2. Goals and non-goals

**Goals**

- Reuse Pd's proven message-passing semantics rather than reimplementing them.
- Build headless, deterministically, and testably (virtual clock).
- Expose a small, stable C API; usable from C and from language bindings.
- Stay a fork: the vendored Pd sources are edited in place.

**Non-goals**

- Loading or saving `.pd` patch files.
- The DSP/signal graph (`~` objects), audio I/O, MIDI I/O.
- GUI, canvas, arrays, data structures, `expr`.
- GUI/audio-related objects.

## 3. Architecture

Layered. Each layer only depends on the ones below it.

```
+---------------------------------------------------------------+
| Embedders / language bindings (C, Python ctypes, ...)          |
+---------------------------------------------------------------+
| Public API                         include/pdp/pdp.h           |
|   pdp_init                                                     |
|   pdp_patch_new/free                                           |
|   pdp_node_new/free/class                                      |
|   pdp_connect / pdp_disconnect                                 |
|   pdp_bang/float/symbol/list/anything/send                     |
|   pdp_clock_advance / pdp_clock_now                            |
|   pdp_log_set_level/set_handler, pdp_log                       |
+---------------------------------------------------------------+
| pdp implementation                 src/pdp, src/operators      |
|   p_engine.c   lifecycle + operator registration               |
|   p_graph.c    patch / node / connection / messaging           |
|   p_clock.c    deterministic clock                             |
|   p_log.c      logging                                         |
|   operators/pdp_message.c   in-tree "message" box              |
+---------------------------------------------------------------+
| Vendored Pd message core           vendor/pure-data/src        |
|   m_pd.c m_class.c m_obj.c m_atom.c m_memory.c m_binbuf.c      |
|   m_conf.c m_glob.c m_sched.c                                  |
|   s_print.c s_utf8.c s_path.c                                  |
|   x_interface.c x_connective.c x_time.c x_arithmetic.c         |
|   x_misc.c x_list.c                                            |
|   p_headless.c   stubs for GUI/interaction/audio/MIDI/DSP      |
+---------------------------------------------------------------+
```

### 3.1 The graph lives partly in the GUI layer

In Pd the dataflow **connection primitive** (`t_outconnect`, `obj_connect`,
`obj_disconnect`, traversal, `inlet_new`/`outlet_new`) is in `m_obj.c` and is
already headless. The **container** (`t_glist`, the canvas) lives in the GUI
layer (`g_canvas.h`/`g_canvas.c`). The message core references the canvas in
only a handful of places, and pdp supplies those as stubs (`p_headless.c`)
rather than compiling the GUI. See decision D1.

Objects do **not** require a canvas to exist; `pdp_patch` is therefore a plain
ownership container, not a canvas.

### 3.2 Runtime dataflow

```
   pdp_bang(node)                                   pdp_clock_advance(ms)
        |                                                    |
        v                                                    v
   pd_bang(t_pd)  --->  outlet(s)  --->  inlet(s) --->  ...   sched_tick()
        |                  (obj_connect wiring)                |
        v                                                      v
   class method dispatch                                 due clocks fire
   (c_bangmethod / c_anymethod / ...)                    (metro, delay)
```

Message delivery, fan-out ordering, hot/cold inlets, and method dispatch are
Pd's, unmodified.

## 4. Public API

Header: `include/pdp/pdp.h`. Types (`t_atom`, `t_symbol`, `t_float` and the
`SETFLOAT`/`SETSYMBOL`/`A_*` macros) come from Pd's `m_pd.h`, which `pdp.h`
includes.

### 4.1 Lifecycle

```c
void pdp_init(void);   /* idempotent; call before creating nodes */
```

`pdp_init` calls Pd's `pd_init()` (brings up the message system, class registry
and scheduler) and then registers pdp's own operators.

### 4.2 Graph

```c
pdp_patch *pdp_patch_new(void);
void       pdp_patch_free(pdp_patch *patch);

pdp_node  *pdp_node_new(pdp_patch *patch, const char *classname,
                        int argc, const t_atom *argv);   /* NULL on failure */
void       pdp_node_free(pdp_node *node);
const char *pdp_node_class(const pdp_node *node);

int        pdp_connect(pdp_patch *p, pdp_node *src, int outno,
                       pdp_node *sink, int inno);        /* 0 ok, -1 error */
int        pdp_disconnect(pdp_patch *p, pdp_node *src, int outno,
                          pdp_node *sink, int inno);
```

- `pdp_node_new` creates the object by **class name**, exactly as a patch does
  (via Pd's `pd_objectmaker`). Unknown classes return `NULL`.
- Connections reuse `obj_connect`/`obj_disconnect`.
- `pdp_node_free` first disconnects the node (outgoing and incoming) and then
  frees it, so teardown never leaves dangling connections.
- `pdp_patch_free` frees all nodes then the container.

### 4.3 Messaging

```c
void pdp_bang(pdp_node *node);
void pdp_float(pdp_node *node, t_float f);
void pdp_symbol(pdp_node *node, t_symbol *s);
void pdp_list(pdp_node *node, int argc, const t_atom *argv);
void pdp_anything(pdp_node *node, t_symbol *selector, int argc,
                  const t_atom *argv);
void pdp_send(pdp_node *node, const char *selector, int argc,
              const t_atom *argv);   /* arbitrary method selector */
```

These inject a message into a node's leftmost inlet (Pd semantics).

### 4.4 Deterministic clock

```c
void   pdp_clock_advance(double ms);  /* step the scheduler clock by ms */
double pdp_clock_now(void);           /* scheduler time in ms */
```

Pd's scheduler advances logical time (`pd_this->pd_systime`) by
`SYSTIMEPERTICK` per `sched_tick()`, firing due clocks. `pdp_clock_advance`
drives `sched_tick()` until the target time is reached — **no wall-clock**, so
`metro`/`delay` are deterministic and testable.

### 4.5 Logging

```c
typedef enum { PDP_LOG_ERROR, PDP_LOG_WARN, PDP_LOG_INFO,
               PDP_LOG_DEBUG, PDP_LOG_TRACE } pdp_log_level;
typedef void (*pdp_log_handler)(pdp_log_level, const char *, void *);

void pdp_log_set_level(pdp_log_level level);
pdp_log_level pdp_log_get_level(void);
void pdp_log_set_handler(pdp_log_handler handler, void *userdata);
void pdp_log(pdp_log_level level, const char *fmt, ...);
```

Default threshold is `PDP_LOG_WARN`; default sink writes to stderr. The graph,
messaging and clock paths log at `debug`/`trace`.

## 5. Operator set

Two sources:

1. **Vendored `x_*` operators** (decision D2: accept the superset from the
   compiled files):
   - `x_interface.c`: `print`, `trace`
   - `x_connective.c`: `int`/`i`, `float`/`f`, `symbol`, `bang`/`b`, `send`/`s`,
     `receive`/`r`, `select`/`sel`, `route`, `pack`, `unpack`, `trigger`/`t`,
     `spigot`, `moses`, `until`, `makefilename`, `swap`, `change`, `value`/`v`
   - `x_arithmetic.c`: `clip`, `+ - * /`, `pow`, `max`, `min`, `log`,
     `== != > < >= <=`, `& && | ||`, `<< >> % mod div`,
     `sin cos tan atan atan2 sqrt exp abs`, `wrap`
   - `x_time.c`: `delay`/`del`, `metro`, `line`, `timer`, `pipe`
   - `x_misc.c`: `random`, `loadbang`, `namecanvas`, `cputime`, `realtime`,
     `oscparse`, `oscformat`, `fudiparse`, `fudiformat`
   - `x_list.c`: `list` and the `list *` family
2. **pdp in-tree operators** (`src/operators/`):
   - `pdp_message.c`: `message` — a minimal headless message box (upstream's is
     in the GUI layer; see decision D3).

The **minimal Tier 0-3 set** the library is designed around:

- Tier 0: `bang`, `trigger`, `float`, `send`/`receive`, `print`
- Tier 1: `+ - * /`, `select`, `route`, `symbol`, `int`, `message`
- Tier 2: `loadbang`, `metro`, `delay`, `change`
- Tier 3: `pack`, `unpack`, `list`, `moses`

## 6. Build

- CMake (>= 3.20), C11.
- Toolchain: MSYS2 UCRT64 gcc (Pd's scheduler uses pthreads). Ninja generator.
- Global compile definitions: `PD`, `PD_INTERNAL` (so `m_pd.h`'s `EXTERN`
  resolves consistently), plus `WINVER=0x0502` and `-mms-bitfields` on MinGW.
- Targets:
  - `pdp_core` — static: vendored Pd subset + `p_headless.c`.
  - `pdp` — static: public API + in-tree operators.
  - `pdp_shared` — shared (DLL) of the same sources, for bindings; MinGW
    exports all symbols (`-Wl,--export-all-symbols`).
  - `pdp-cli` — command-line harness.
  - `test_graph`, `test_message`, `test_clock`, `test_operators` — C tests.
- `enable_testing()` + CTest; the Python ctypes test is added when
  `.venv/Scripts/python.exe` exists.

## 7. Testing

- **C tests** (CTest): tiny assert harness (`tests/support/pdp_test.h`) plus a
  recording sink operator (`tests/support/pdp_test_sink.h`) that captures the
  last message received, so dataflow can be asserted.
- **Python integration test** (`tests/python/test_ctypes.py`): loads
  `pdp_shared` via ctypes and exercises the public ABI.
- Virtual clock makes `metro`/`delay` deterministic (e.g. `metro 100` over
  250 ms yields exactly 3 bangs: one on start, then 100 and 200).

## 8. Design decisions

- **D1 — minimal core, not the GUI layer.** Do not compile `g_*`; supply the
  canvas/interaction/audio/MIDI/DSP symbols the core references as headless
  stubs in `src/pdp/p_headless.c`. (Alternative rejected: compile the GUI layer
  headless, libpd-style.)
- **D2 — accept the operator superset** from the six compiled `x_*` files
  rather than pruning their `*_setup()` functions. `conf_init()` in `m_conf.c`
  was pruned to call only those six setups (+ `m_pd_setup`).
- **D3 — in-tree `message` box.** Upstream's `message` class is in `g_text.c`
  (GUI). pdp implements a minimal replacement.
- **D4 — reuse Pd types** (`t_atom`/`t_symbol`) in the public API rather than
  opaque handles.
- **D5 — `pdp_patch`/`pdp_node` handles** rather than exposing raw
  `pd_new`/`obj_connect` (though `m_pd.h`/`m_imp.h` remain available).
- **D6 — forked in place.** `vendor/pure-data` is a plain in-tree directory
  (nested `.git` removed), upstream commit `f3497d4` (0.57-0).

## 9. Directory layout

```
pdp/
  CMakeLists.txt
  README.md  SPEC.md  TODO.md
  include/pdp/pdp.h              public API
  src/pdp/                       p_engine.c p_graph.c p_clock.c p_log.c
                                 p_headless.c (stubs)
  src/operators/pdp_message.c    in-tree operators
  tools/pdp-cli/main.c           command-line harness
  tests/                         C tests + support headers
  tests/python/test_ctypes.py    ctypes integration test
  vendor/pure-data/              forked Pd source (edited in place)
  vendor/UPSTREAM.md             provenance
```

## 10. Known limitations / future work

- No `.pd` file loading/saving.
- No DSP/signal graph, audio or MIDI.
- `pdp_node_new` detects creation failure by comparing `pd_newest()` before and
  after; a cleaner class-existence check is possible.
- The vendored operator superset (D2) means more classes than the Tier 0-3 set.
- Thread safety: `sys_lock`/`sys_unlock` are no-ops (single-threaded headless).
