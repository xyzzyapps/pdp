# pdp

A **headless dataflow core**, forked from [Pure Data](https://github.com/pure-data/pure-data).

`pdp` keeps Pd's message/control dataflow engine — atoms, symbols, classes,
objects, inlets/outlets, connections, the scheduler and clocks — and drops the
GUI, the audio/MIDI backends, and the DSP/signal graph. The result is a small C
library you can embed, plus a command-line harness for testing and scripting.

See [`SPEC.md`](SPEC.md) for the full specification and architecture, and
[`TODO.md`](TODO.md) for the task list.

## Status

Working and tested:

- Headless build of the Pd message core (no GUI/audio/DSP).
- Public C API: graph construction, connections, messaging, a deterministic
  clock, and logging.
- The Tier 0-3 control operators (`bang`, `trigger`, `float`, `+ - * /`,
  `select`, `route`, `send`/`receive`, `metro`, `delay`, `loadbang`, `pack`,
  `unpack`, `list`, `moses`, `print`, and an in-tree `message` box).
- C test suite (CTest) and a Python ctypes integration test.

## Requirements

- **CMake** >= 3.20
- **MSYS2 UCRT64 GCC** (`C:\msys64\ucrt64\bin`) — Pd's scheduler uses pthreads,
  so the MinGW toolchain is used rather than MSVC
- **Ninja**
- **Python** 3.8+ (only for the optional ctypes integration test)

## Build

From the project root (PowerShell):

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
cmake -S . -B build -G Ninja `
  -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" `
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

This produces:

- `build/libpdp_core.a` — vendored Pd subset + headless stubs
- `build/libpdp.a` — the pdp library (static)
- `build/libpdp_shared.dll` — the pdp library (shared, for bindings)
- `build/pdp-cli.exe` — the command-line harness

## Run

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
./build/pdp-cli.exe        # normal
./build/pdp-cli.exe -v     # verbose (trace) logging
```

`pdp-cli` builds a small graph with the public API and fires it:

```
pdp: message -> print ok
verbose(2): print: symbol hello
verbose(2): print: bang   (x4)
pdp: clock advanced to 351 ms
```

## Test

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
Push-Location build; ctest --output-on-failure; Pop-Location
```

The C tests run by default. To include the Python ctypes test, create the
virtualenv first:

```powershell
python -m venv .venv
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe"
cmake --build build
Push-Location build; ctest --output-on-failure; Pop-Location
```

## Use from C

```c
#include "pdp/pdp.h"

int main(void)
{
    t_atom a[1];
    pdp_init();

    pdp_patch *patch = pdp_patch_new();

    SETSYMBOL(&a[0], gensym("hello"));
    pdp_node *msg = pdp_node_new(patch, "message", 1, a);
    pdp_node *prn = pdp_node_new(patch, "print", 0, 0);

    pdp_connect(patch, msg, 0, prn, 0);
    pdp_bang(msg);              /* prints: hello */

    pdp_patch_free(patch);
    return 0;
}
```

Types (`t_atom`, `t_symbol`, `t_float`) come from Pd's `m_pd.h`, which `pdp.h`
includes.

## Repository layout

```
CMakeLists.txt        build definition
include/pdp/pdp.h     public API
src/pdp/              engine, graph, clock, logging, headless stubs
src/operators/        in-tree operators (message box)
tools/pdp-cli/        command-line harness
tests/                C tests + support headers
tests/python/         ctypes integration test
vendor/pure-data/     forked Pd source (edited in place)
SPEC.md               specification / architecture
```

## License and attribution

`pdp` is a fork of Pure Data. Except as otherwise noted, the vendored sources
are Copyright (c) 1997-2024 Miller Puckette and others, under Pd's BSD-style
license (see `vendor/pure-data/LICENSE.txt`). Keep that file and attribution
intact.
