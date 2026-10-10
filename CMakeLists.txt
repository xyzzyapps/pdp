cmake_minimum_required(VERSION 3.20)
project(pdp LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(PDP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(PD_SRC   "${PDP_ROOT}/vendor/pure-data/src")

# ---------------------------------------------------------------------------
# Pd build definitions.  These MUST be global so that m_pd.h's EXTERN macro
# resolves consistently everywhere it is included (otherwise pd_post/post are
# seen as dllimport in some translation units and import/export mismatches
# appear at link time).
# ---------------------------------------------------------------------------
add_compile_definitions(PD PD_INTERNAL)
if(MINGW)
    add_compile_definitions(WINVER=0x0502)
    add_compile_options(-mms-bitfields)
endif()

# ---------------------------------------------------------------------------
# Vendored Pd message core (headless subset -- Option 2).
# GUI (g_*), audio/MIDI backends, and DSP operator sources are NOT built.
# ---------------------------------------------------------------------------
set(PDP_PD_CORE_SRC
    m_pd.c m_class.c m_obj.c m_atom.c m_memory.c m_binbuf.c
    m_conf.c m_glob.c m_sched.c
    s_print.c s_utf8.c s_path.c
    # Minimal control-flow operator set (Tier 0-3), pruned via conf_init().
    x_interface.c x_connective.c x_time.c x_arithmetic.c x_misc.c x_list.c
)
list(TRANSFORM PDP_PD_CORE_SRC PREPEND "${PD_SRC}/")

add_library(pdp_core STATIC
    ${PDP_PD_CORE_SRC}
    ${PDP_ROOT}/src/pdp/p_headless.c
)
target_include_directories(pdp_core PUBLIC "${PD_SRC}")
target_link_libraries(pdp_core PUBLIC pthread)

# ---------------------------------------------------------------------------
# pdp -- our own library: public API + in-tree operators, over the core.
# Built both static (for the CLI and C tests) and shared (for language
# bindings such as the Python ctypes integration test).
# ---------------------------------------------------------------------------
set(PDP_API_SRC
    ${PDP_ROOT}/src/pdp/p_engine.c
    ${PDP_ROOT}/src/pdp/p_graph.c
    ${PDP_ROOT}/src/pdp/p_clock.c
    ${PDP_ROOT}/src/pdp/p_log.c
    ${PDP_ROOT}/src/operators/pdp_message.c
)

add_library(pdp STATIC ${PDP_API_SRC})
target_include_directories(pdp PUBLIC
    "${PDP_ROOT}/include"
    "${PD_SRC}"
)
target_link_libraries(pdp PUBLIC pdp_core)

add_library(pdp_shared SHARED ${PDP_API_SRC})
target_include_directories(pdp_shared PUBLIC
    "${PDP_ROOT}/include"
    "${PD_SRC}"
)
target_link_libraries(pdp_shared PUBLIC pdp_core)
if(MINGW)
    # Export the public symbols so bindings can resolve them.
    target_link_options(pdp_shared PRIVATE -Wl,--export-all-symbols)
endif()

# ---------------------------------------------------------------------------
# Command-line harness.
# ---------------------------------------------------------------------------
add_executable(pdp-cli "${PDP_ROOT}/tools/pdp-cli/main.c")
target_link_libraries(pdp-cli PRIVATE pdp)

# ---------------------------------------------------------------------------
# C tests (CTest).
# ---------------------------------------------------------------------------
enable_testing()
foreach(t graph message clock operators)
    add_executable(test_${t} "${PDP_ROOT}/tests/test_${t}.c")
    target_include_directories(test_${t} PRIVATE "${PDP_ROOT}/tests")
    target_link_libraries(test_${t} PRIVATE pdp)
    add_test(NAME ${t} COMMAND test_${t})
endforeach()

# Python ctypes integration test (only if the virtualenv exists).
if(EXISTS "${PDP_ROOT}/.venv/Scripts/python.exe")
    add_test(NAME python_ctypes
        COMMAND "${PDP_ROOT}/.venv/Scripts/python.exe"
                "${PDP_ROOT}/tests/python/test_ctypes.py")
endif()
