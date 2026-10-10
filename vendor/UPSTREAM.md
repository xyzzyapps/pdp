# Vendored upstream

This directory contains a fork of **Pure Data (Pd)**, vendored in-tree and
edited in place.

- Upstream: https://github.com/pure-data/pure-data
- Commit:   `f3497d41dd28d4ec1906a0601ff4c0a661c3e5d9`
- Version:  0.57-0
- Date:     2026-10-05
- Method:   `git clone --depth 1`, then the nested `.git` was removed so the
  source is tracked as part of this repository (a fork edited in place, not a
  submodule).

Only a subset of `vendor/pure-data/src/` is compiled; see the top-level
`CMakeLists.txt`. The GUI (`g_*`, `tcl/`), audio/MIDI backends, and DSP operator
sources are intentionally not built.
