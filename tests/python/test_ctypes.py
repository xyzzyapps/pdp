"""Integration test for the pdp shared library via ctypes.

Proves the public C ABI is usable from outside C: load the built shared
library, initialise the core, build a graph by class name, connect nodes, and
fire messages.

Run with the project virtualenv:

    .venv\\Scripts\\python.exe tests\\python\\test_ctypes.py

No third-party dependencies (stdlib unittest + ctypes only).
"""

import ctypes
import glob
import os
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def find_shared_library():
    patterns = [
        "build/**/libpdp_shared.dll",
        "build/**/pdp_shared.dll",
        "build/**/libpdp_shared.so",
        "build/**/libpdp_shared.dylib",
    ]
    for pattern in patterns:
        hits = glob.glob(os.path.join(REPO_ROOT, pattern), recursive=True)
        if hits:
            return hits[0]
    raise RuntimeError(
        "pdp shared library not found under build/; build the project first"
    )


def _prepare_dll_search_path(lib_path):
    """Make dependent runtime DLLs (MinGW) findable on Windows."""
    if os.name != "nt":
        return
    candidates = [
        os.path.dirname(lib_path),
        r"C:\msys64\ucrt64\bin",
    ]
    for d in candidates:
        if os.path.isdir(d):
            try:
                os.add_dll_directory(d)
            except (AttributeError, OSError):
                pass


class PdpLibrary:
    """Thin ctypes binding for the subset of pdp used by these tests."""

    def __init__(self, path):
        _prepare_dll_search_path(path)
        self.lib = ctypes.CDLL(path)

        def sig(name, restype, argtypes):
            fn = getattr(self.lib, name)
            fn.restype = restype
            fn.argtypes = argtypes
            return fn

        vp = ctypes.c_void_p
        self.pdp_init = sig("pdp_init", None, [])
        self.pdp_patch_new = sig("pdp_patch_new", vp, [])
        self.pdp_patch_free = sig("pdp_patch_free", None, [vp])
        self.pdp_node_new = sig("pdp_node_new", vp,
                                [vp, ctypes.c_char_p, ctypes.c_int, vp])
        self.pdp_node_free = sig("pdp_node_free", None, [vp])
        self.pdp_node_class = sig("pdp_node_class", ctypes.c_char_p, [vp])
        self.pdp_connect = sig("pdp_connect", ctypes.c_int,
                               [vp, vp, ctypes.c_int, vp, ctypes.c_int])
        self.pdp_disconnect = sig("pdp_disconnect", ctypes.c_int,
                                  [vp, vp, ctypes.c_int, vp, ctypes.c_int])
        self.pdp_bang = sig("pdp_bang", None, [vp])
        self.pdp_float = sig("pdp_float", None, [vp, ctypes.c_float])
        self.pdp_clock_advance = sig("pdp_clock_advance", None, [ctypes.c_double])
        self.pdp_clock_now = sig("pdp_clock_now", ctypes.c_double, [])


class TestPdpCtypes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pdp = PdpLibrary(find_shared_library())
        cls.pdp.pdp_init()

    def test_init_is_idempotent(self):
        # Calling twice must not crash or reinitialise.
        self.pdp.pdp_init()
        self.pdp.pdp_init()

    def test_graph_lifecycle(self):
        patch = self.pdp.pdp_patch_new()
        self.assertTrue(patch)

        b = self.pdp.pdp_node_new(patch, b"bang", 0, None)
        p = self.pdp.pdp_node_new(patch, b"print", 0, None)
        self.assertTrue(b)
        self.assertTrue(p)
        self.assertEqual(self.pdp.pdp_node_class(b), b"bang")

        self.assertEqual(self.pdp.pdp_connect(patch, b, 0, p, 0), 0)
        self.pdp.pdp_bang(b)
        self.assertEqual(self.pdp.pdp_disconnect(patch, b, 0, p, 0), 0)

        self.pdp.pdp_patch_free(patch)

    def test_unknown_class_returns_null(self):
        patch = self.pdp.pdp_patch_new()
        n = self.pdp.pdp_node_new(patch, b"no_such_class_xyz", 0, None)
        self.assertFalse(n)
        self.pdp.pdp_patch_free(patch)

    def test_clock_advances(self):
        before = self.pdp.pdp_clock_now()
        self.pdp.pdp_clock_advance(100.0)
        after = self.pdp.pdp_clock_now()
        self.assertGreaterEqual(after, before)
        self.assertGreaterEqual(after - before, 99.0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
