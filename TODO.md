# TODO — pdp

A headless dataflow core, forked from Pure Data. See `SPEC.md` for the design
and `README.md` for build/usage.

## Status

Core complete and tested: headless build, public C API (graph, messaging,
deterministic clock, logging), the Tier 0-3 operators plus an in-tree `message`
box, C + Python tests, and docs.

## Locked decisions

- **Fork**: `pure-data/pure-data`, vendored in place at `vendor/pure-data/`
  (upstream `f3497d4`, 0.57-0), edited as a plain in-tree directory.
- **Language / interface**: C11, public C API (`include/pdp/pdp.h`).
- **Scope**: message/control dataflow only — no GUI, audio, MIDI, or DSP.
- **Minimal core**: do not compile the GUI layer; supply the symbols the message
  core needs as headless stubs (`src/pdp/p_headless.c`).
- **Operators**: accept the superset registered by the six compiled `x_*` files;
  `conf_init()` pruned to those setups. In-tree `message` box
  (`src/operators/pdp_message.c`).
- **Build**: CMake + Ninja + MSYS2 UCRT64 gcc.
- **License**: Pd-style BSD (see `LICENSE`).

## Operator set (design target: Tier 0-3)

- Tier 0: `bang`, `trigger`, `float`, `send`/`receive`, `print`
- Tier 1: `+ - * /`, `select`, `route`, `symbol`, `int`, `message`
- Tier 2: `loadbang`, `metro`, `delay`, `change`
- Tier 3: `pack`, `unpack`, `list`, `moses`

## Future work

- CLI: read a small text graph description and dump a full message trace.
- Cleaner class-existence check in `pdp_node_new`.
- Optional: prune the vendored operator superset to exactly Tier 0-3.
- Optional: golden-trace regression tests.
