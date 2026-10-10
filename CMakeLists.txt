cmake_minimum_required(VERSION 3.20)

project(urban_chaos_riot_response_recompiled LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_executable(urban_chaos_launcher WIN32 src/portable_launcher.c)
target_link_libraries(urban_chaos_launcher PRIVATE user32)
set_target_properties(urban_chaos_launcher PROPERTIES OUTPUT_NAME "Urban Chaos")
add_executable(urban_chaos_baseline_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_baseline_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1)
target_link_libraries(urban_chaos_baseline_launcher PRIVATE user32)
set_target_properties(urban_chaos_baseline_launcher PROPERTIES OUTPUT_NAME "Urban Chaos Baseline")
add_executable(urban_chaos_diagnostics_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_diagnostics_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1 UCRR_PACKAGE_DIAGNOSTICS=1)
target_link_libraries(urban_chaos_diagnostics_launcher PRIVATE user32)
set_target_properties(urban_chaos_diagnostics_launcher PROPERTIES OUTPUT_NAME "Urban Chaos Diagnostics")
add_executable(urban_chaos_performance_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_performance_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1 UCRR_PACKAGE_CPU20=1)
target_link_libraries(urban_chaos_performance_launcher PRIVATE user32)
set_target_properties(urban_chaos_performance_launcher PROPERTIES OUTPUT_NAME "Urban Chaos Performance")
add_executable(urban_chaos_gpu_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_gpu_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1 UCRR_PACKAGE_CPU20=1 UCRR_PACKAGE_RAWGPU=1)
target_link_libraries(urban_chaos_gpu_launcher PRIVATE user32)
set_target_properties(urban_chaos_gpu_launcher PROPERTIES OUTPUT_NAME "Urban Chaos GPU Candidate")
add_executable(urban_chaos_cpu_headroom_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_cpu_headroom_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1 UCRR_PACKAGE_CPU20=1 UCRR_PACKAGE_RAWGPU=1 UCRR_PACKAGE_CPU_HEADROOM=1)
target_link_libraries(urban_chaos_cpu_headroom_launcher PRIVATE user32)
set_target_properties(urban_chaos_cpu_headroom_launcher PROPERTIES OUTPUT_NAME "Urban Chaos CPU Bundle")
add_executable(urban_chaos_audit_launcher WIN32 src/portable_launcher.c)
target_compile_definitions(urban_chaos_audit_launcher PRIVATE UCRR_PACKAGE_RETAINED18=1 UCRR_PACKAGE_CPU20=1 UCRR_PACKAGE_RAWGPU=1 UCRR_PACKAGE_CPU_HEADROOM=1 UCRR_PACKAGE_AUDIT=1)
target_link_libraries(urban_chaos_audit_launcher PRIVATE user32)
set_target_properties(urban_chaos_audit_launcher PROPERTIES OUTPUT_NAME "Urban Chaos Audit Candidate")

add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set(XBOXRECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xboxrecomp"
    CACHE PATH "Path to the xboxrecomp toolkit")
add_subdirectory("${XBOXRECOMP_DIR}" "${CMAKE_BINARY_DIR}/xboxrecomp")
option(UCRR_BOUNDED_VERTEX "Experimental bounded CPU vertex specialization" OFF)
option(UCRR_BOUNDED_VERTEX_TRACE "Count specialization eligibility (diagnostic only)" OFF)
option(UCRR_INLINE_VERTEX "Experimental exact inline CPU program specialization" OFF)
option(UCRR_INLINE_VERTEX_TRACE "Count inline specialization eligibility" OFF)
option(UCRR_TEXTURE_LOOKUP_HINT "Experimental full-identity ordinary texture lookup hint" OFF)
option(UCRR_TEXTURE_LOOKUP_AB "Diagnostic alternating texture lookup paths" OFF)
option(UCRR_RAM_WATCH_SHADOW "Experimental physical lighting write watch (shadow verification)" OFF)
if(UCRR_TEXTURE_LOOKUP_AB AND NOT UCRR_TEXTURE_LOOKUP_HINT)
    message(FATAL_ERROR "Texture lookup AB requires hint candidate")
endif()
if(UCRR_TEXTURE_LOOKUP_HINT)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_TEXTURE_LOOKUP_HINT=1)
    if(UCRR_TEXTURE_LOOKUP_AB)
        target_compile_definitions(xbox_nv2a PRIVATE UCRR_TEXTURE_LOOKUP_AB=1)
    endif()
endif()
if(UCRR_INLINE_VERTEX AND UCRR_BOUNDED_VERTEX)
    message(FATAL_ERROR "Test CPU vertex candidates separately")
endif()
if(UCRR_INLINE_VERTEX)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/vertex_decode_checks/inline_specialized_eval.h")
        message(FATAL_ERROR "Run prepare_inline_specialized.py before inline vertex build")
    endif()
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_INLINE_VERTEX=1)
    target_include_directories(xbox_nv2a PRIVATE "${CMAKE_BINARY_DIR}/vertex_decode_checks")
    if(UCRR_INLINE_VERTEX_TRACE)
        target_compile_definitions(xbox_nv2a PRIVATE UCRR_INLINE_VERTEX_TRACE=1)
    endif()
endif()
if(UCRR_BOUNDED_VERTEX)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/vertex_decode_checks/bounded_specialized_eval.h")
        message(FATAL_ERROR "Run prepare_vertex_decode_cache.py before bounded vertex build")
    endif()
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_BOUNDED_VERTEX=1)
    target_include_directories(xbox_nv2a PRIVATE "${CMAKE_BINARY_DIR}/vertex_decode_checks")
    if(UCRR_BOUNDED_VERTEX_TRACE)
        target_compile_definitions(xbox_nv2a PRIVATE UCRR_BOUNDED_VERTEX_TRACE=1)
    endif()
endif()

file(GLOB RECOMP_GEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/*.c")

if(NOT RECOMP_GEN_SOURCES)
    message(FATAL_ERROR
        "No lifted sources found. Run scripts/analyze.ps1 before configuring.")
endif()

# Keep a console attached during bring-up for runtime diagnostics.
set(UCRR_GUEST_LAYOUT "DEFAULT" CACHE STRING "Experimental guest object order: DEFAULT or PALETTE_LAST")
set_property(CACHE UCRR_GUEST_LAYOUT PROPERTY STRINGS DEFAULT PALETTE_LAST)
if(NOT UCRR_GUEST_LAYOUT MATCHES "^(DEFAULT|PALETTE_LAST)$")
    message(FATAL_ERROR "Invalid guest layout")
endif()
if(UCRR_GUEST_LAYOUT STREQUAL "PALETTE_LAST")
    set(layout_palette "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0007.c")
    list(FIND RECOMP_GEN_SOURCES "${layout_palette}" layout_index)
    if(layout_index LESS 0)
        message(FATAL_ERROR "Palette source missing for layout experiment")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${layout_index})
    list(APPEND RECOMP_GEN_SOURCES "${layout_palette}")
endif()
option(UCRR_COMPACT_PALETTE "Experimental size policy for guest B9450 only" OFF)
if(UCRR_COMPACT_PALETTE)
    if(NOT MSVC)
        message(FATAL_ERROR "Compact palette is verified only with MSVC")
    endif()
    include(scripts/cmake/compact_palette.cmake)
endif()
option(UCRR_XMV_SIMD "Retained integer SSE2 helpers in XMV translation unit" ON)
set(UCRR_LOCAL_PALETTE_SOURCE "" CACHE FILEPATH "Isolated native-local palette test TU; empty keeps original")
if(UCRR_LOCAL_PALETTE_SOURCE)
    if(UCRR_COMPACT_PALETTE OR NOT UCRR_GUEST_LAYOUT STREQUAL "DEFAULT" OR NOT UCRR_PGO_PHASE STREQUAL "OFF")
        message(FATAL_ERROR "Local palette test must be isolated from other guest experiments")
    endif()
    if(NOT IS_ABSOLUTE "${UCRR_LOCAL_PALETTE_SOURCE}" OR NOT EXISTS "${UCRR_LOCAL_PALETTE_SOURCE}")
        message(FATAL_ERROR "Local palette test requires an existing absolute derived source")
    endif()
    set(local_palette_original "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0007.c")
    list(FIND RECOMP_GEN_SOURCES "${local_palette_original}" local_palette_index)
    if(local_palette_index LESS 0)
        message(FATAL_ERROR "Local palette original source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${local_palette_index})
    list(INSERT RECOMP_GEN_SOURCES ${local_palette_index} "${UCRR_LOCAL_PALETTE_SOURCE}")
endif()
option(UCRR_BOUNDS_LOCAL_CANDIDATE "Build runtime-selectable closed bounds leaf candidate" OFF)
if(UCRR_BOUNDS_LOCAL_CANDIDATE)
    if(NOT UCRR_LOCAL_PALETTE_SOURCE)
        message(FATAL_ERROR "Bounds candidate requires retained palette base")
    endif()
    set(bounds_source "${CMAKE_BINARY_DIR}/guest_candidates/recomp_0007_bounds.c")
    add_custom_command(OUTPUT "${bounds_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_bounds_local.py"
            --base "${UCRR_LOCAL_PALETTE_SOURCE}" --output "${bounds_source}"
        DEPENDS "${UCRR_LOCAL_PALETTE_SOURCE}" scripts/prepare_bounds_local.py
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${UCRR_LOCAL_PALETTE_SOURCE}" bounds_index)
    if(bounds_index LESS 0)
        message(FATAL_ERROR "Retained palette TU not found")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${bounds_index})
    list(INSERT RECOMP_GEN_SOURCES ${bounds_index} "${bounds_source}")
endif()
option(UCRR_NATIVE_BOUNDS_LEAF "Build guarded native B2A40 SSE candidate" OFF)
if(UCRR_NATIVE_BOUNDS_LEAF)
    enable_language(ASM_MASM)
    if(UCRR_BOUNDS_LOCAL_CANDIDATE)
        set(native_bounds_base "${bounds_source}")
    elseif(UCRR_LOCAL_PALETTE_SOURCE)
        set(native_bounds_base "${UCRR_LOCAL_PALETTE_SOURCE}")
    else()
        set(native_bounds_base "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0007.c")
    endif()
    set(native_bounds_source "${CMAKE_BINARY_DIR}/guest_candidates/recomp_0007_native_bounds.c")
    set(native_bounds_asm "${CMAKE_BINARY_DIR}/guest_candidates/native_bounds.asm")
    add_custom_command(OUTPUT "${native_bounds_source}" "${native_bounds_asm}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_bounds.py"
            --base "${native_bounds_base}" --output "${native_bounds_source}" --asm "${native_bounds_asm}"
        DEPENDS "${native_bounds_base}" scripts/prepare_native_bounds.py
            src/native_bounds_leaf.h analysis/disasm/asm/text.asm VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${native_bounds_base}" native_bounds_index)
    if(native_bounds_index LESS 0)
        message(FATAL_ERROR "Native leaf base not found")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${native_bounds_index})
    list(INSERT RECOMP_GEN_SOURCES ${native_bounds_index} "${native_bounds_source}")
    add_library(native_bounds_asm OBJECT "${native_bounds_asm}")
    list(APPEND RECOMP_GEN_SOURCES $<TARGET_OBJECTS:native_bounds_asm>)
endif()
option(UCRR_GUEST_STREAM_SWITCH "Experimental exact guest stream target switch" OFF)
if(UCRR_GUEST_STREAM_SWITCH)
    set(stream_source "${CMAKE_BINARY_DIR}/guest_candidates/recomp_0006_stream.c")
    if(NOT EXISTS "${stream_source}")
        message(FATAL_ERROR "Generate guest stream switch source before build")
    endif()
    list(FIND RECOMP_GEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0006.c" stream_index)
    if(stream_index LESS 0)
        message(FATAL_ERROR "Guest stream source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${stream_index})
    list(INSERT RECOMP_GEN_SOURCES ${stream_index} "${stream_source}")
endif()
option(UCRR_CPU_HEADROOM_GUEST "Build opt-in COPY and EMITTER CPU headroom candidates" OFF)
if(UCRR_CPU_HEADROOM_GUEST)
    set(cpu_bundle_dir "${CMAKE_BINARY_DIR}/guest_candidates/cpu_bundle")
    set(cpu_bundle_source "${cpu_bundle_dir}/recomp_0037_cpu_bundle.c")
    add_custom_command(OUTPUT "${cpu_bundle_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_cpu_bundle_guest.py" --output-dir "${cpu_bundle_dir}"
        DEPENDS scripts/prepare_cpu_bundle_guest.py src/cpu_bundle_guest.h
            src/recomp/gen/recomp_0027.c src/recomp/gen/recomp_0036.c src/recomp/gen/recomp_0037.c VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0037.c" cpu_bundle_index)
    if(cpu_bundle_index LESS 0)
        message(FATAL_ERROR "CPU headroom guest source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${cpu_bundle_index})
    list(INSERT RECOMP_GEN_SOURCES ${cpu_bundle_index} "${cpu_bundle_source}")
endif()
option(UCRR_BODY_RETENTION "Candidate: bounded 32-body / 120-second retention" OFF)
option(UCRR_SCENE_SETUP_CANDIDATE "Scoped scene lookup and guarded matrix preparation" OFF)
option(UCRR_SCENE_SCAN_CANDIDATE "Callback-bounded fused scene membership traversal" OFF)
if(UCRR_SCENE_SCAN_CANDIDATE AND NOT UCRR_SCENE_SETUP_CANDIDATE)
    message(FATAL_ERROR "Scene scan requires retained scene setup candidate")
endif()
if(UCRR_SCENE_SETUP_CANDIDATE)
    if(UCRR_NATIVE_BOUNDS_LEAF)
        set(scene_base7 "${native_bounds_source}")
    elseif(UCRR_BOUNDS_LOCAL_CANDIDATE)
        set(scene_base7 "${bounds_source}")
    elseif(UCRR_LOCAL_PALETTE_SOURCE)
        set(scene_base7 "${UCRR_LOCAL_PALETTE_SOURCE}")
    else()
        set(scene_base7 "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0007.c")
    endif()
    if(UCRR_GUEST_STREAM_SWITCH)
        set(scene_base6 "${stream_source}")
    else()
        set(scene_base6 "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0006.c")
    endif()
    set(scene_dir "${CMAKE_BINARY_DIR}/guest_candidates/scene_setup")
    set(scene_scan_args)
    if(UCRR_SCENE_SCAN_CANDIDATE)
        list(APPEND scene_scan_args --scan)
    endif()
    add_custom_command(OUTPUT "${scene_dir}/recomp_0006_scene.c" "${scene_dir}/recomp_0007_scene.c"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_scene_setup.py"
            --base6 "${scene_base6}" --base7 "${scene_base7}" --output-dir "${scene_dir}" ${scene_scan_args}
        DEPENDS "${scene_base6}" "${scene_base7}" scripts/prepare_scene_setup.py scripts/prepare_scene_scan.py scripts/scene_getter_transform.py src/scene_setup_guard.h src/scene_getter_certificate.h
        VERBATIM)
    foreach(chunk 6 7)
        list(FIND RECOMP_GEN_SOURCES "${scene_base${chunk}}" scene_index)
        if(scene_index LESS 0)
            message(FATAL_ERROR "Scene candidate base ${chunk} not found")
        endif()
        list(REMOVE_AT RECOMP_GEN_SOURCES ${scene_index})
        list(INSERT RECOMP_GEN_SOURCES ${scene_index} "${scene_dir}/recomp_000${chunk}_scene.c")
    endforeach()
endif()
option(UCRR_PALETTE_AFFINE_CANDIDATE "Opt-in actual ordinary-RAM palette admission" OFF)
if(UCRR_PALETTE_AFFINE_CANDIDATE)
    if(NOT UCRR_LOCAL_PALETTE_SOURCE OR NOT UCRR_NATIVE_BOUNDS_LEAF OR
       NOT UCRR_SCENE_SETUP_CANDIDATE OR NOT UCRR_SCENE_SCAN_CANDIDATE)
        message(FATAL_ERROR "Palette affine requires the checksum-pinned retained scene chain")
    endif()
    set(palette_runtime_dir "${CMAKE_BINARY_DIR}/guest_candidates/palette_affine")
    set(palette_runtime_source "${palette_runtime_dir}/recomp_0007_palette_affine.c")
    set(palette_runtime_base "${scene_dir}/recomp_0007_scene.c")
    add_custom_command(OUTPUT "${palette_runtime_source}"
        "${palette_runtime_dir}/palette_runtime_guard.inc"
        "${palette_runtime_dir}/palette_runtime_dispatch.inc"
        "${palette_runtime_dir}/palette_runtime_manifest.json"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/attribution_atlas/prepare_palette_runtime.py"
            --input "${palette_runtime_base}" --output-dir "${palette_runtime_dir}"
        DEPENDS "${palette_runtime_base}"
            scripts/attribution_atlas/prepare_palette_runtime.py
            scripts/attribution_atlas/prepare_palette_affine.py
            scripts/attribution_atlas/prepare_palette_reason.py
            scripts/attribution_atlas/palette_affine_runtime.h
            scripts/palette_finite_guard.h scripts/palette_ordered_math.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${palette_runtime_base}" palette_runtime_index)
    if(palette_runtime_index LESS 0)
        message(FATAL_ERROR "Retained scene palette source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${palette_runtime_index})
    list(INSERT RECOMP_GEN_SOURCES ${palette_runtime_index} "${palette_runtime_source}")
endif()
option(UCRR_CUSTOM_LIGHTING "Separate approximate depth-based point lighting experiment" OFF)
if(UCRR_CUSTOM_LIGHTING)
    if(NOT UCRR_SCENE_SETUP_CANDIDATE OR NOT UCRR_SCENE_SCAN_CANDIDATE OR
       UCRR_LIGHT_RESTORATION OR UCRR_SCENE96_FUSION OR
       UCRR_MATERIAL_OPCODE_PROBE OR UCRR_MATERIAL_EXECUTOR_EXPERIMENT OR UCRR_FEATURE_REMOVAL OR
       UCRR_LIGHT_REMOVAL OR UCRR_SHADOW_REMOVAL OR UCRR_DECAL_REMOVAL OR UCRR_SPECULAR_REMOVAL OR
       UCRR_GLOW_REMOVAL OR UCRR_PARTICLE_RESTORATION OR UCRR_DECAL_RESTORATION)
        message(FATAL_ERROR "Custom lighting requires isolated retained scene chain, all removal/restoration experiments off")
    endif()
    set(custom_lighting_dir "${CMAKE_BINARY_DIR}/guest_candidates/custom_lighting")
    foreach(chunk IN ITEMS 6 7)
        set(custom_lighting_base "${scene_dir}/recomp_000${chunk}_scene.c")
        if(chunk EQUAL 7 AND UCRR_PALETTE_AFFINE_CANDIDATE)
            set(custom_lighting_base "${palette_runtime_source}")
        endif()
        set(custom_lighting_source "${custom_lighting_dir}/recomp_000${chunk}_custom_lighting.c")
        add_custom_command(OUTPUT "${custom_lighting_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/custom_lighting_guest_checks/prepare_custom_lighting_guest.py"
                --input "${custom_lighting_base}" --output "${custom_lighting_source}" --chunk "${chunk}"
            DEPENDS "${custom_lighting_base}" scripts/custom_lighting_guest_checks/prepare_custom_lighting_guest.py
                scripts/scene_getter_transform.py src/scene_getter_certificate.h
                "src/recomp/gen/recomp_000${chunk}.c" analysis/custom_lighting/guest/custom_lighting_guest.h VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${custom_lighting_base}" custom_lighting_index)
        if(custom_lighting_index LESS 0)
            message(FATAL_ERROR "Custom lighting scene source missing")
        endif()
        list(REMOVE_AT RECOMP_GEN_SOURCES ${custom_lighting_index})
        list(INSERT RECOMP_GEN_SOURCES ${custom_lighting_index} "${custom_lighting_source}")
    endforeach()
    add_custom_command(OUTPUT "${custom_lighting_dir}/point_lighting_embedded.h"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/custom_lighting_checks/embed.py"
            --output "${custom_lighting_dir}/point_lighting_embedded.h"
        DEPENDS scripts/custom_lighting_checks/embed.py analysis/custom_lighting/point_lighting.hlsl VERBATIM)
    add_custom_target(custom_lighting_shader DEPENDS "${custom_lighting_dir}/point_lighting_embedded.h")
    add_dependencies(xbox_nv2a custom_lighting_shader)
    target_sources(xbox_nv2a PRIVATE analysis/custom_lighting/point_lighting_host.c
        "${custom_lighting_dir}/point_lighting_embedded.h")
    target_include_directories(xbox_nv2a PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/custom_lighting"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/custom_lighting/guest" "${custom_lighting_dir}")
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_CUSTOM_LIGHTING=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_CUSTOM_LIGHTING=1)
endif()
option(UCRR_LIGHT_RESTORATION "Opt-in exact receiver inverse reuse with lighting retained" OFF)
if(UCRR_LIGHT_RESTORATION)
    if(UCRR_PALETTE_AFFINE_CANDIDATE)
        set(light_restoration_base "${palette_runtime_source}")
    elseif(UCRR_SCENE_SETUP_CANDIDATE)
        set(light_restoration_base "${scene_dir}/recomp_0007_scene.c")
    elseif(UCRR_NATIVE_BOUNDS_LEAF)
        set(light_restoration_base "${native_bounds_source}")
    elseif(UCRR_BOUNDS_LOCAL_CANDIDATE)
        set(light_restoration_base "${bounds_source}")
    elseif(UCRR_LOCAL_PALETTE_SOURCE)
        set(light_restoration_base "${UCRR_LOCAL_PALETTE_SOURCE}")
    else()
        set(light_restoration_base "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0007.c")
    endif()
    set(light_restoration_source "${CMAKE_BINARY_DIR}/guest_candidates/effect_restoration/recomp_0007_light.c")
    add_custom_command(OUTPUT "${light_restoration_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_light_restoration.py"
            --input "${light_restoration_base}" --output "${light_restoration_source}"
        DEPENDS "${light_restoration_base}" scripts/prepare_light_restoration.py src/light_restoration.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${light_restoration_base}" light_restoration_index)
    if(light_restoration_index LESS 0)
        message(FATAL_ERROR "Light restoration base missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${light_restoration_index})
    list(INSERT RECOMP_GEN_SOURCES ${light_restoration_index} "${light_restoration_source}")
    set_property(SOURCE "${light_restoration_source}" APPEND PROPERTY COMPILE_DEFINITIONS UCRR_LIGHT_RESTORATION=1)
endif()
option(UCRR_SCENE96_FUSION "Opt-in whole secondary scene traversal fusion" OFF)
if(UCRR_SCENE96_FUSION)
    if(NOT UCRR_SCENE_SETUP_CANDIDATE OR NOT UCRR_SCENE_SCAN_CANDIDATE)
        message(FATAL_ERROR "Scene96 requires retained scene setup and scan helpers")
    endif()
    set(scene96_base "${scene_dir}/recomp_0006_scene.c")
    set(scene96_source "${CMAKE_BINARY_DIR}/guest_candidates/scene96/recomp_0006_scene96.c")
    add_custom_command(OUTPUT "${scene96_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_scene96_checks.py" --base "${scene96_base}" --output "${scene96_source}"
        DEPENDS "${scene96_base}" scripts/prepare_scene96_checks.py scripts/prepare_scene_scan.py scripts/prepare_scene_setup.py
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${scene96_base}" scene96_index)
    if(scene96_index LESS 0)
        message(FATAL_ERROR "Scene96 base missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${scene96_index})
    list(INSERT RECOMP_GEN_SOURCES ${scene96_index} "${scene96_source}")
endif()
option(UCRR_MATERIAL_OPCODE_PROBE "Bounded observation-only material opcode diagnostic" OFF)
option(UCRR_MATERIAL_EXECUTOR_EXPERIMENT "Opt-in material register localization; known NaN divergence" OFF)
if(UCRR_MATERIAL_OPCODE_PROBE)
    if(UCRR_MATERIAL_EXECUTOR_EXPERIMENT OR UCRR_GUEST_STREAM_SWITCH OR UCRR_SCENE96_FUSION)
        message(FATAL_ERROR "Opcode observer requires original material interpreter")
    endif()
    if(UCRR_SCENE_SETUP_CANDIDATE)
        set(opcode_probe_base "${scene_dir}/recomp_0006_scene.c")
    else()
        set(opcode_probe_base "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0006.c")
    endif()
    set(opcode_probe_source "${CMAKE_BINARY_DIR}/guest_candidates/material_opcode/recomp_0006_probe.c")
    add_custom_command(OUTPUT "${opcode_probe_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_material_opcode_probe.py"
            --base "${opcode_probe_base}" --output "${opcode_probe_source}"
        DEPENDS "${opcode_probe_base}" scripts/prepare_material_opcode_probe.py src/material_opcode_probe.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${opcode_probe_base}" opcode_probe_index)
    if(opcode_probe_index LESS 0)
        message(FATAL_ERROR "Opcode observer source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${opcode_probe_index})
    list(INSERT RECOMP_GEN_SOURCES ${opcode_probe_index} "${opcode_probe_source}")
endif()
if(UCRR_MATERIAL_EXECUTOR_EXPERIMENT)
    if(UCRR_SCENE96_FUSION)
        message(FATAL_ERROR "Keep Scene96 separate from material executor")
    endif()
    if(UCRR_GUEST_STREAM_SWITCH)
        message(FATAL_ERROR "Material experiment requires original dispatch, not stream-switch candidate")
    endif()
    if(UCRR_SCENE_SETUP_CANDIDATE)
        set(material_executor_base "${scene_dir}/recomp_0006_scene.c")
    else()
        set(material_executor_base "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0006.c")
    endif()
    set(material_executor_dir "${CMAKE_BINARY_DIR}/guest_candidates/material_executor")
    set(material_executor_source "${material_executor_dir}/recomp_0006_material_executor.c")
    add_custom_command(OUTPUT "${material_executor_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_material_executor.py"
            --base "${material_executor_base}" --output-dir "${material_executor_dir}"
        DEPENDS "${material_executor_base}" scripts/prepare_material_executor.py src/recomp/gen/recomp_0006.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${material_executor_base}" material_executor_index)
    if(material_executor_index LESS 0)
        message(FATAL_ERROR "Material executor base source missing")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${material_executor_index})
    list(INSERT RECOMP_GEN_SOURCES ${material_executor_index} "${material_executor_source}")
endif()
if(UCRR_BODY_RETENTION)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/guest_candidates/recomp_0009_bodies.c"
               "${CMAKE_BINARY_DIR}/guest_candidates/recomp_0025_bodies.c"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_body_retention.py"
                --output-dir "${CMAKE_BINARY_DIR}/guest_candidates"
        DEPENDS scripts/prepare_body_retention.py src/recomp/gen/recomp_0009.c src/recomp/gen/recomp_0025.c
        VERBATIM)
    foreach(body_chunk IN ITEMS 0009 0025)
        set(body_source "${CMAKE_BINARY_DIR}/guest_candidates/recomp_${body_chunk}_bodies.c")
        list(FIND RECOMP_GEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_${body_chunk}.c" body_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${body_index})
        list(INSERT RECOMP_GEN_SOURCES ${body_index} "${body_source}")
    endforeach()
endif()
option(UCRR_PALETTE_FUSION_GUEST "Default-off palette upload register fusion experiment" OFF)
option(UCRR_PREPARATION_TRANSACTION "Default-off synchronous draw preparation transaction" OFF)
if(UCRR_PREPARATION_TRANSACTION)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_PREPARATION_TRANSACTION=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_PREPARATION_TRANSACTION=1)
endif()
if(UCRR_PALETTE_FUSION_GUEST)
    set(palette_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0007[^/]*\\.c$")
            list(APPEND palette_matches "${source}")
        endif()
    endforeach()
    list(LENGTH palette_matches palette_count)
    if(NOT palette_count EQUAL 1)
        message(FATAL_ERROR "Palette fusion needs one selected chunk7")
    endif()
    list(GET palette_matches 0 palette_base7)
    set(palette_fusion_dir "${CMAKE_BINARY_DIR}/guest_candidates/palette_fusion")
    set(palette_fusion_source "${palette_fusion_dir}/recomp_0007_palette.c")
    add_custom_command(OUTPUT "${palette_fusion_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_palette_fusion.py"
            --base7 "${palette_base7}"
            --base36 "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_0036.c"
            --output-dir "${palette_fusion_dir}"
        DEPENDS "${palette_base7}" src/recomp/gen/recomp_0036.c
            scripts/prepare_palette_fusion.py src/palette_fusion_candidate.h src/recomp_manual.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${palette_base7}" palette_fusion_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${palette_fusion_index})
    list(INSERT RECOMP_GEN_SOURCES ${palette_fusion_index} "${palette_fusion_source}")
endif()
option(UCRR_SCENE_BRANCHLESS_GUEST "Default-off conditional child selection in retained scene scan" OFF)
if(UCRR_SCENE_BRANCHLESS_GUEST)
    set(branchless_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0006[^/]*\\.c$")
            list(APPEND branchless_matches "${source}")
        endif()
    endforeach()
    list(LENGTH branchless_matches branchless_count)
    if(NOT branchless_count EQUAL 1)
        message(FATAL_ERROR "Branchless scene candidate needs one selected chunk6")
    endif()
    list(GET branchless_matches 0 branchless_base6)
    set(branchless_dir "${CMAKE_BINARY_DIR}/guest_candidates/scene_branchless")
    set(branchless_source "${branchless_dir}/recomp_0006_branchless.c")
    add_custom_command(OUTPUT "${branchless_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_scene_branchless.py"
            --base6 "${branchless_base6}" --output-dir "${branchless_dir}"
        DEPENDS "${branchless_base6}" scripts/prepare_scene_branchless.py src/scene_branchless_candidate.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${branchless_base6}" branchless_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${branchless_index})
    list(INSERT RECOMP_GEN_SOURCES ${branchless_index} "${branchless_source}")
endif()
option(UCRR_HYBRID_NATIVE_LIGHTING "Isolated native scenery lighting in CONTROL" OFF)
if(UCRR_HYBRID_NATIVE_LIGHTING)
    if(UCRR_CUSTOM_LIGHTING OR UCRR_SCENE_BRANCHLESS_GUEST OR UCRR_LIGHTING_LIST_PROBE OR
       UCRR_MATERIAL_OPCODE_PROBE OR UCRR_MATERIAL_EXECUTOR_EXPERIMENT OR UCRR_SCENE96_FUSION)
        message(FATAL_ERROR "Hybrid lighting needs the unmodified retained scene visitor")
    endif()
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(hybrid_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0006[^/]*\\.c$")
            list(APPEND hybrid_matches "${source}")
        endif()
    endforeach()
    list(LENGTH hybrid_matches hybrid_count)
    if(NOT hybrid_count EQUAL 1)
        message(FATAL_ERROR "Hybrid lighting needs one selected chunk6")
    endif()
    list(GET hybrid_matches 0 hybrid_base)
    set(hybrid_source "${CMAKE_BINARY_DIR}/guest_candidates/hybrid/recomp_0006_hybrid.c")
    add_custom_command(OUTPUT "${hybrid_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_hybrid_lighting.py"
            --input "${hybrid_base}" --output "${hybrid_source}"
        DEPENDS "${hybrid_base}" scripts/prepare_hybrid_lighting.py scripts/prepare_scene_scan.py
            src/hybrid_lighting_guest.h src/recomp/gen/recomp_0006.c src/recomp/gen/recomp_0007.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${hybrid_base}" hybrid_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${hybrid_index})
    list(INSERT RECOMP_GEN_SOURCES ${hybrid_index} "${hybrid_source}")
    set(scenery_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0007[^/]*\\.c$")
            list(APPEND scenery_matches "${source}")
        endif()
    endforeach()
    list(LENGTH scenery_matches scenery_count)
    if(NOT scenery_count EQUAL 1)
        message(FATAL_ERROR "Hybrid scenery needs one selected chunk7")
    endif()
    list(GET scenery_matches 0 scenery_base)
    set(scenery_source "${CMAKE_BINARY_DIR}/guest_candidates/hybrid/recomp_0007_scenery.c")
    add_custom_command(OUTPUT "${scenery_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_hybrid_lighting.py"
            --scenery --input "${scenery_base}" --output "${scenery_source}"
        DEPENDS "${scenery_base}" scripts/prepare_hybrid_lighting.py src/hybrid_lighting_guest.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${scenery_base}" scenery_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${scenery_index})
    list(INSERT RECOMP_GEN_SOURCES ${scenery_index} "${scenery_source}")
    set(flashlight_source "${CMAKE_BINARY_DIR}/guest_candidates/hybrid/recomp_0007_flashlight.c")
    add_custom_command(OUTPUT "${flashlight_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_flashlight_material.py"
            --base7 "${scenery_source}" --output "${flashlight_source}"
        DEPENDS "${scenery_source}" scripts/prepare_flashlight_material.py src/flashlight_material_cache.h
        VERBATIM)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${scenery_index})
    list(INSERT RECOMP_GEN_SOURCES ${scenery_index} "${flashlight_source}")
    add_subdirectory(native/archive_reader hybrid-archive)
    add_subdirectory(native/texture_asset hybrid-texture)
    add_library(uc_hybrid_native STATIC
        native/runtime/renderer.cpp native/runtime/assets.cpp native/runtime/integration_lighting.cpp
        native/runtime/integration_shadows.cpp native/runtime/control_lighting_bridge.cpp
        native/scene_asset.c native/level_asset.c native/material_asset.c native/texture_sequence.c
        native/material_prepass.c native/material_aux_prepass.c native/material_normal_prepass.c native/material_alpha_prepass.c native/normal_palette.c
        native/model_bounds.c native/model_light_membership.c native/point_light_registry.c
        native/point_light_geometry.c native/point_light_receiver.c native/point_light_pixel.c native/point_light_textures.c native/spot_light.c)
    target_compile_definitions(uc_hybrid_native PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(uc_hybrid_native PRIVATE /W4 /WX /O2 /EHsc- /fp:strict)
    target_link_libraries(uc_hybrid_native PUBLIC uc_native_texture d3d11 dxgi d3dcompiler bcrypt)
    target_include_directories(xbox_nv2a PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/native/runtime")
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_HYBRID_NATIVE_LIGHTING=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_HYBRID_NATIVE_LIGHTING=1)
    target_link_libraries(xbox_nv2a PUBLIC uc_hybrid_native)
endif()
option(UCRR_HYBRID_NATIVE_PARTICLES "Isolated source particle rendering in the native-lighting hybrid" OFF)
option(UCRR_HYBRID_PC_FRONTEND "Native PC menus in the isolated hybrid" OFF)
if(UCRR_HYBRID_PC_FRONTEND)
    if(NOT UCRR_HYBRID_NATIVE_LIGHTING)
        message(FATAL_ERROR "Hybrid PC frontend requires hybrid lighting")
    endif()
    target_sources(uc_hybrid_native PRIVATE native/runtime/control_frontend_bridge.cpp
        native/runtime/frontend.cpp native/runtime/movie.cpp native/runtime/dialogue_text.cpp
        native/runtime/hud_keys.c native/font.c)
    target_link_libraries(uc_hybrid_native PUBLIC mfplat mfreadwrite mfuuid ole32 propsys xaudio2 gdi32 user32)
    set(pc_orders_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0012[^/]*\\.c$")
            list(APPEND pc_orders_matches "${source}")
        endif()
    endforeach()
    list(LENGTH pc_orders_matches pc_orders_count)
    if(NOT pc_orders_count EQUAL 1)
        message(FATAL_ERROR "PC orders HUD needs exactly one selected chunk12")
    endif()
    list(GET pc_orders_matches 0 pc_orders_base)
    set(pc_orders_source "${CMAKE_BINARY_DIR}/guest_candidates/pc_frontend/recomp_0012_orders.c")
    add_custom_command(OUTPUT "${pc_orders_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_pc_orders_hud.py"
            --input "${pc_orders_base}" --output "${pc_orders_source}"
        DEPENDS "${pc_orders_base}" scripts/prepare_pc_orders_hud.py VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${pc_orders_base}" pc_orders_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${pc_orders_index})
    list(INSERT RECOMP_GEN_SOURCES ${pc_orders_index} "${pc_orders_source}")
    foreach(movie_chunk 5 23)
        if(movie_chunk EQUAL 5)
            set(movie_stem "0005")
        else()
            set(movie_stem "0023")
        endif()
        set(movie_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_${movie_stem}[^/]*\\.c$")
                list(APPEND movie_matches "${source}")
            endif()
        endforeach()
        list(LENGTH movie_matches movie_count)
        if(NOT movie_count EQUAL 1)
            message(FATAL_ERROR "Native movie needs one selected chunk ${movie_stem}")
        endif()
        list(GET movie_matches 0 movie_base)
        set(movie_source "${CMAKE_BINARY_DIR}/guest_candidates/pc_frontend/recomp_${movie_stem}_movie.c")
        add_custom_command(OUTPUT "${movie_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_hybrid_frontend_movie.py"
                --input "${movie_base}" --output "${movie_source}" --chunk ${movie_chunk}
            DEPENDS "${movie_base}" scripts/prepare_hybrid_frontend_movie.py VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${movie_base}" movie_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${movie_index})
        list(INSERT RECOMP_GEN_SOURCES ${movie_index} "${movie_source}")
    endforeach()
    foreach(pointer_chunk 23 24)
        set(pointer_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_00${pointer_chunk}[^/]*\\.c$")
                list(APPEND pointer_matches "${source}")
            endif()
        endforeach()
        list(LENGTH pointer_matches pointer_count)
        if(NOT pointer_count EQUAL 1)
            message(FATAL_ERROR "Menu pointer needs one selected chunk ${pointer_chunk}")
        endif()
        list(GET pointer_matches 0 pointer_base)
        set(pointer_source "${CMAKE_BINARY_DIR}/guest_candidates/pc_frontend/recomp_00${pointer_chunk}_pointer.c")
        add_custom_command(OUTPUT "${pointer_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_menu_dialog.py"
                --input "${pointer_base}" --output "${pointer_source}" --chunk ${pointer_chunk}
            DEPENDS "${pointer_base}" scripts/prepare_menu_dialog.py src/menu_dialog_guest.inc VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${pointer_base}" pointer_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${pointer_index})
        list(INSERT RECOMP_GEN_SOURCES ${pointer_index} "${pointer_source}")
    endforeach()
endif()
if(UCRR_HYBRID_NATIVE_PARTICLES)
    if(NOT UCRR_HYBRID_NATIVE_LIGHTING)
        message(FATAL_ERROR "Hybrid native particles require UCRR_HYBRID_NATIVE_LIGHTING")
    endif()
    set(hybrid_particle_source "${CMAKE_BINARY_DIR}/guest_candidates/hybrid_particles/recomp_0006_hybrid_particles.c")
    add_custom_command(OUTPUT "${hybrid_particle_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_hybrid_particles.py"
            --input "${hybrid_source}" --output "${hybrid_particle_source}"
        DEPENDS "${hybrid_source}" scripts/prepare_hybrid_particles.py
            src/hybrid_particles_guest.h src/recomp/gen/recomp_0006.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${hybrid_source}" hybrid_particle_index)
    if(hybrid_particle_index LESS 0)
        message(FATAL_ERROR "Hybrid particles need the selected hybrid-lighting chunk6")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${hybrid_particle_index})
    list(INSERT RECOMP_GEN_SOURCES ${hybrid_particle_index} "${hybrid_particle_source}")
    target_sources(uc_hybrid_native PRIVATE native/runtime/control_particle_bridge.cpp
        native/runtime/particle_billboards.cpp native/particle_billboard.c native/particle_billboard_batch.c)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_HYBRID_NATIVE_PARTICLES=1)
endif()
option(UCRR_HYBRID_EXTENDED "Combined native model, flashlight, shadow and decal candidate" OFF)
if(UCRR_HYBRID_EXTENDED)
    if(NOT UCRR_HYBRID_NATIVE_LIGHTING OR NOT UCRR_HYBRID_NATIVE_PARTICLES)
        message(FATAL_ERROR "Extended hybrid requires native lighting and particles")
    endif()
    target_compile_definitions(uc_hybrid_native PRIVATE UCRR_HYBRID_EXTENDED=1)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_HYBRID_EXTENDED=1)
    target_sources(uc_hybrid_native PRIVATE native/runtime/control_quad_bridge.cpp
        native/runtime/decal_quads.cpp native/runtime/particle_quads.cpp native/particle_quad_batch.c
        native/runtime/control_model_bridge.cpp native/control_model_receiver.c native/point_light_vertex.c native/model_visibility.c native/model_asset.c)
    set(hybrid_model_source "${CMAKE_BINARY_DIR}/guest_candidates/hybrid_models/recomp_0006_hybrid_models.c")
    add_custom_command(OUTPUT "${hybrid_model_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_hybrid_models.py"
            --input "${hybrid_particle_source}" --output "${hybrid_model_source}"
        DEPENDS "${hybrid_particle_source}" scripts/prepare_hybrid_models.py
            src/hybrid_models_guest.h src/recomp/gen/recomp_0006.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${hybrid_particle_source}" hybrid_model_index)
    if(hybrid_model_index LESS 0)
        message(FATAL_ERROR "Extended hybrid needs the selected lighting/particle chunk6")
    endif()
    list(REMOVE_AT RECOMP_GEN_SOURCES ${hybrid_model_index})
    list(INSERT RECOMP_GEN_SOURCES ${hybrid_model_index} "${hybrid_model_source}")
endif()
option(UCRR_NATIVE_DEBRIS_GUEST "Opt-in native fragment bounce with original collision queries" OFF)
if(UCRR_NATIVE_DEBRIS_GUEST)
    set(debris_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0002[^/]*\\.c$")
            list(APPEND debris_matches "${source}")
        endif()
    endforeach()
    list(LENGTH debris_matches debris_count)
    if(NOT debris_count EQUAL 1)
        message(FATAL_ERROR "Native debris requires exactly one selected chunk2")
    endif()
    list(GET debris_matches 0 debris_base)
    set(debris_source "${CMAKE_BINARY_DIR}/guest_candidates/native_debris/recomp_0002_native_debris.c")
    add_custom_command(OUTPUT "${debris_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_debris.py"
            --input "${debris_base}" --output "${debris_source}"
        DEPENDS "${debris_base}" scripts/prepare_native_debris.py src/hybrid_debris_guest.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${debris_base}" debris_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${debris_index})
    list(INSERT RECOMP_GEN_SOURCES ${debris_index} "${debris_source}")
endif()
option(UCRR_NATIVE_TRIANGLE_GUEST "Opt-in native triangle casts with CONTROL rounding" OFF)
if(UCRR_NATIVE_TRIANGLE_GUEST)
    set(triangle_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0030[^/]*\\.c$")
            list(APPEND triangle_matches "${source}")
        endif()
    endforeach()
    list(LENGTH triangle_matches triangle_count)
    if(NOT triangle_count EQUAL 1)
        message(FATAL_ERROR "Native triangle requires exactly one selected chunk30")
    endif()
    list(GET triangle_matches 0 triangle_base)
    set(triangle_source "${CMAKE_BINARY_DIR}/guest_candidates/native_triangle/recomp_0030_native_triangle.c")
    add_custom_command(OUTPUT "${triangle_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_triangle.py"
            --input "${triangle_base}" --output "${triangle_source}"
        DEPENDS "${triangle_base}" scripts/prepare_native_triangle.py src/hybrid_triangle_guest.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${triangle_base}" triangle_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${triangle_index})
    list(INSERT RECOMP_GEN_SOURCES ${triangle_index} "${triangle_source}")
endif()
option(UCRR_NATIVE_MOPP_TERMINAL_GUEST "Opt-in native MOPP terminal driver with original collision services" OFF)
if(UCRR_NATIVE_MOPP_TERMINAL_GUEST)
    set(mopp_terminal_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0031[^/]*\\.c$")
            list(APPEND mopp_terminal_matches "${source}")
        endif()
    endforeach()
    list(LENGTH mopp_terminal_matches mopp_terminal_count)
    if(NOT mopp_terminal_count EQUAL 1)
        message(FATAL_ERROR "Native MOPP terminal requires exactly one selected chunk31")
    endif()
    list(GET mopp_terminal_matches 0 mopp_terminal_base)
    set(mopp_terminal_source "${CMAKE_BINARY_DIR}/guest_candidates/native_mopp_terminal/recomp_0031_native_mopp_terminal.c")
    add_custom_command(OUTPUT "${mopp_terminal_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_mopp.py"
            --input "${mopp_terminal_base}" --output "${mopp_terminal_source}"
        DEPENDS "${mopp_terminal_base}" scripts/prepare_native_mopp.py src/hybrid_mopp_terminal.h
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${mopp_terminal_base}" mopp_terminal_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${mopp_terminal_index})
    list(INSERT RECOMP_GEN_SOURCES ${mopp_terminal_index} "${mopp_terminal_source}")
endif()
option(UCRR_NATIVE_MESH_FILTER_GUEST "Opt-in native storage-mesh collision filter and closest-hit adapters" OFF)
if(UCRR_NATIVE_MESH_FILTER_GUEST)
    if(NOT UCRR_NATIVE_TRIANGLE_GUEST)
        message(FATAL_ERROR "Native mesh filter needs the validated native mesh material adapter")
    endif()
    set(mesh_filter_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0025[^/]*\\.c$")
            list(APPEND mesh_filter_matches "${source}")
        endif()
    endforeach()
    list(LENGTH mesh_filter_matches mesh_filter_count)
    if(NOT mesh_filter_count EQUAL 1)
        message(FATAL_ERROR "Native mesh filter requires exactly one selected chunk25")
    endif()
    list(GET mesh_filter_matches 0 mesh_filter_base)
    set(mesh_filter_source "${CMAKE_BINARY_DIR}/guest_candidates/native_mesh_filter/recomp_0025_native_mesh_filter.c")
    add_custom_command(OUTPUT "${mesh_filter_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_filter.py"
            --input "${mesh_filter_base}" --output "${mesh_filter_source}"
        DEPENDS "${mesh_filter_base}" scripts/prepare_native_filter.py src/hybrid_collision_guest.h
            src/recomp/gen/recomp_0006.c src/recomp/gen/recomp_0022.c src/recomp/gen/recomp_0030.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${mesh_filter_base}" mesh_filter_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${mesh_filter_index})
    list(INSERT RECOMP_GEN_SOURCES ${mesh_filter_index} "${mesh_filter_source}")
endif()
option(UCRR_NATIVE_SCENERY_GUEST "Opt-in native scenery rays with original broadphase and gameplay" OFF)
if(UCRR_NATIVE_SCENERY_GUEST)
    if(NOT UCRR_NATIVE_TRIANGLE_GUEST OR NOT UCRR_NATIVE_MESH_FILTER_GUEST)
        message(FATAL_ERROR "Native scenery needs the validated mesh, triangle and hit adapters")
    endif()
    set(scenery_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0031[^/]*\\.c$")
            list(APPEND scenery_matches "${source}")
        endif()
    endforeach()
    list(LENGTH scenery_matches scenery_count)
    if(NOT scenery_count EQUAL 1)
        message(FATAL_ERROR "Native scenery requires exactly one selected chunk31")
    endif()
    list(GET scenery_matches 0 scenery_base)
    set(scenery_source "${CMAKE_BINARY_DIR}/guest_candidates/native_scenery/recomp_0031_native_scenery.c")
    add_custom_command(OUTPUT "${scenery_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_scenery.py"
            --input "${scenery_base}" --output "${scenery_source}"
        DEPENDS "${scenery_base}" scripts/prepare_native_scenery.py src/hybrid_scenery_guest.h
            src/recomp/gen/recomp_0003.c src/recomp/gen/recomp_0005.c
            src/recomp/gen/recomp_0006.c src/recomp/gen/recomp_0010.c
            src/recomp/gen/recomp_0018.c
            src/recomp/gen/recomp_0022.c src/recomp/gen/recomp_0025.c
            src/recomp/gen/recomp_0030.c src/recomp/gen/recomp_0031.c src/recomp/gen/recomp_0033.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${scenery_base}" scenery_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${scenery_index})
    list(INSERT RECOMP_GEN_SOURCES ${scenery_index} "${scenery_source}")
endif()
option(UCRR_NATIVE_BODY_GUEST "Opt-in native collision-body preparation with original callbacks" OFF)
if(UCRR_NATIVE_BODY_GUEST)
    foreach(chunk IN ITEMS 0025 0031)
        set(body_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_${chunk}[^/]*\\.c$")
                list(APPEND body_matches "${source}")
            endif()
        endforeach()
        list(LENGTH body_matches body_count)
        if(NOT body_count EQUAL 1)
            message(FATAL_ERROR "Native body preparation requires one selected chunk${chunk}")
        endif()
        list(GET body_matches 0 body_base)
        set(body_source "${CMAKE_BINARY_DIR}/guest_candidates/native_body/recomp_${chunk}_native_body.c")
        add_custom_command(OUTPUT "${body_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_body.py"
                --input "${body_base}" --output "${body_source}"
            DEPENDS "${body_base}" scripts/prepare_native_body.py src/hybrid_body_guest.h
                src/recomp/gen/recomp_0022.c src/recomp/gen/recomp_0025.c src/recomp/gen/recomp_0031.c
            VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${body_base}" body_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${body_index})
        list(INSERT RECOMP_GEN_SOURCES ${body_index} "${body_source}")
    endforeach()
endif()
option(UCRR_NATIVE_CAPSULE_GUEST "Opt-in native capsule plane preparation with original contact handling" OFF)
option(UCRR_NATIVE_RELATIVE_GUEST "Opt-in native relative body transforms with exact guest state" OFF)
option(UCRR_NATIVE_SOLVER_GUEST "Opt-in exact native contact impulse runs with original solver orchestration" OFF)
option(UCRR_NATIVE_SOLVER_PAIR_GUEST "Opt-in exact native initial/repeated paired contacts with original solver orchestration" OFF)
option(UCRR_NATIVE_PARTICLE_MOTION_GUEST "Opt-in exact native particle motion with original emission and lifecycle" OFF)
option(UCRR_NATIVE_PARTICLE_RECORD_GUEST "Opt-in exact ordinary particle records with original material and GPU submission" OFF)
option(UCRR_NATIVE_PARTICLE_ROTATING_GUEST "Opt-in exact native rotating particle records with original material and GPU submission" OFF)
option(UCRR_NATIVE_PARTICLE_BIRTH_GUEST "Opt-in native particle birth calculations with original shared RNG and lifecycle" OFF)
option(UCRR_NATIVE_PARTICLE_OWNER_GUEST "Opt-in native particle manager update and cleanup with original system callbacks" OFF)
option(UCRR_NATIVE_PARTICLE_SYSTEM_GUEST "Opt-in native particle system orchestration with original emitters and sources" OFF)
option(UCRR_NATIVE_EFFECT_QUOTA_GUEST "Opt-in native persistent impact quota with original world ownership" OFF)
option(UCRR_NATIVE_CONTACT_MANAGER_GUEST "Opt-in native zero-contact manager teardown with original world ownership" OFF)
option(UCRR_NATIVE_CONTACT_INSTALLED_GUEST "Opt-in native installed-contact manager teardown with original constraint removal" OFF)
foreach(stage IN ITEMS capsule relative contact_manager contact_installed solver solver_pair particle_motion particle_record particle_rotating particle_birth particle_owner particle_system effect_quota)
    string(TOUPPER "${stage}" stage_upper)
    if(UCRR_NATIVE_${stage_upper}_GUEST)
        if(stage STREQUAL "relative" OR stage STREQUAL "contact_manager" OR stage STREQUAL "contact_installed")
            set(chunk 0029)
        elseif(stage STREQUAL "particle_motion" OR stage STREQUAL "particle_record" OR stage STREQUAL "particle_rotating" OR stage STREQUAL "particle_birth" OR stage STREQUAL "particle_owner" OR stage STREQUAL "particle_system")
            set(chunk 0021)
        elseif(stage STREQUAL "effect_quota")
            set(chunk 0002)
        else()
            set(chunk 0031)
        endif()
        set(numeric_script "scripts/prepare_native_${stage}.py")
        if(stage STREQUAL "solver")
            set(numeric_script "scripts/prepare_native_solver_contact.py")
        endif()
        set(numeric_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_${chunk}[^/]*\\.c$")
                list(APPEND numeric_matches "${source}")
            endif()
        endforeach()
        list(LENGTH numeric_matches numeric_count)
        if(NOT numeric_count EQUAL 1)
            message(FATAL_ERROR "Native ${stage} requires one selected chunk${chunk}")
        endif()
        list(GET numeric_matches 0 numeric_base)
        set(numeric_source "${CMAKE_BINARY_DIR}/guest_candidates/native_${stage}/recomp_${chunk}_native_${stage}.c")
        add_custom_command(OUTPUT "${numeric_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/${numeric_script}"
                --input "${numeric_base}" --output "${numeric_source}"
            DEPENDS "${numeric_base}" "${numeric_script}" "src/hybrid_${stage}_guest.h"
                scripts/prepare_native_solver_contact.py
                scripts/prepare_native_contact_manager.py
                src/recomp/gen/recomp_0000.c src/recomp/gen/recomp_0002.c
                src/recomp/gen/recomp_0003.c src/recomp/gen/recomp_0005.c
                src/recomp/gen/recomp_0006.c
                src/recomp/gen/recomp_0015.c src/recomp/gen/recomp_0018.c src/recomp/gen/recomp_0019.c src/recomp/gen/recomp_0021.c src/recomp/gen/recomp_0027.c
                src/recomp/gen/recomp_0028.c src/recomp/gen/recomp_0029.c src/recomp/gen/recomp_0030.c src/recomp/gen/recomp_0031.c src/recomp/gen/recomp_0033.c src/recomp/gen/recomp_0034.c
            VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${numeric_base}" numeric_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${numeric_index})
        list(INSERT RECOMP_GEN_SOURCES ${numeric_index} "${numeric_source}")
    endif()
endforeach()
option(UCRR_NATIVE_ANIMATION_GUEST "Opt-in native pose calculations with original animation callbacks" OFF)
if(UCRR_NATIVE_ANIMATION_GUEST)
    foreach(chunk IN ITEMS 0007 0017)
        set(animation_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_${chunk}[^/]*\\.c$")
                list(APPEND animation_matches "${source}")
            endif()
        endforeach()
        list(LENGTH animation_matches animation_count)
        if(NOT animation_count EQUAL 1)
            message(FATAL_ERROR "Native animation requires one selected chunk${chunk}")
        endif()
        list(GET animation_matches 0 animation_base)
        set(animation_source "${CMAKE_BINARY_DIR}/guest_candidates/native_animation/recomp_${chunk}_native_animation.c")
        add_custom_command(OUTPUT "${animation_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_animation.py"
                --input "${animation_base}" --output "${animation_source}"
            DEPENDS "${animation_base}" scripts/prepare_native_animation.py src/hybrid_animation_guest.h
                scripts/palette_finite_guard.h scripts/palette_ordered_math.h
                src/recomp/gen/recomp_0003.c src/recomp/gen/recomp_0005.c src/recomp/gen/recomp_0007.c
                src/recomp/gen/recomp_0017.c src/recomp/gen/recomp_0018.c
                src/recomp/gen/recomp_0027.c src/recomp/gen/recomp_0033.c
            VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${animation_base}" animation_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${animation_index})
        list(INSERT RECOMP_GEN_SOURCES ${animation_index} "${animation_source}")
    endforeach()
endif()
option(UCRR_NATIVE_ANIMATION_FINISH_GUEST "Opt-in native animation finalization with original callbacks and ABI suffix" OFF)
option(UCRR_NATIVE_ANIMATION_SAMPLER_GUEST "Opt-in native animation sampling prefix with original cache and callbacks" OFF)
option(UCRR_NATIVE_VISIBILITY_GUEST "Opt-in exact native model classification with original collection and materials" OFF)
option(UCRR_NATIVE_ANIMATION_LINKED_GUEST "Opt-in native linked pose calculations with original recursive ABI" OFF)
option(UCRR_NATIVE_ANIMATION_FACE_GUEST "Opt-in native face pose prefix with original modifier callbacks and ABI" OFF)
option(UCRR_NATIVE_AUDIO_SOURCE_GUEST "Opt-in native quiescent audio source destructor with original buffer/device callbacks" OFF)
option(UCRR_NATIVE_RESOURCE_STREAM_GUEST "Opt-in native raw resource stream ownership with original file services" OFF)
option(UCRR_NATIVE_ANIMATION_OWNER_GUEST "Opt-in native controller list update with original timing and event callbacks" OFF)
option(UCRR_NATIVE_ANIMATION_LIFECYCLE_GUEST "Opt-in native controller insertion and cleanup with original callbacks" OFF)
option(UCRR_NATIVE_ANIMATION_CONSTRUCT_GUEST "Opt-in native animation descriptor and owner construction with original allocators" OFF)
option(UCRR_NATIVE_RESOURCE_PROVIDER_GUEST "Opt-in native raw resource opening with original file/archive services" OFF)
option(UCRR_NATIVE_RESOURCE_LOOKUP_GUEST "Opt-in native named resource lookup ownership with original tree/string services" OFF)
foreach(stage IN ITEMS animation_finish animation_sampler animation_owner animation_lifecycle animation_construct visibility animation_linked animation_face audio_source resource_stream resource_provider resource_lookup)
    string(TOUPPER "${stage}" stage_upper)
    if(NOT UCRR_NATIVE_${stage_upper}_GUEST)
        continue()
    endif()
    if(stage STREQUAL "visibility" OR stage STREQUAL "animation_linked")
        set(chunk 0007)
    elseif(stage STREQUAL "animation_face")
        set(chunk 0026)
    elseif(stage STREQUAL "audio_source")
        set(chunk 0022)
    elseif(stage STREQUAL "resource_stream" OR stage STREQUAL "resource_provider" OR stage STREQUAL "resource_lookup")
        set(chunk 0005)
    else()
        set(chunk 0017)
    endif()
    set(finish_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_${chunk}[^/]*\\.c$")
            list(APPEND finish_matches "${source}")
        endif()
    endforeach()
    list(LENGTH finish_matches finish_count)
    if(NOT finish_count EQUAL 1)
        message(FATAL_ERROR "Native ${stage} requires one selected chunk${chunk}")
    endif()
    list(GET finish_matches 0 finish_base)
    set(finish_source "${CMAKE_BINARY_DIR}/guest_candidates/native_${stage}/recomp_${chunk}_native_${stage}.c")
    add_custom_command(OUTPUT "${finish_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_${stage}.py"
            --input "${finish_base}" --output "${finish_source}"
        DEPENDS "${finish_base}" "scripts/prepare_native_${stage}.py" "src/hybrid_${stage}_guest.h"
            native/model_pose.h
            src/hybrid_animation_finish_math.h src/animation_blend_probe.h src/recomp_manual.c
            src/recomp/recomp_types.h src/recomp/gen/recomp_dispatch.c
            src/recomp/gen/recomp_0000.c src/recomp/gen/recomp_0006.c
            src/recomp/gen/recomp_0003.c src/recomp/gen/recomp_0005.c src/recomp/gen/recomp_0007.c
            src/recomp/gen/recomp_0017.c src/recomp/gen/recomp_0018.c src/recomp/gen/recomp_0027.c
            src/recomp/gen/recomp_0021.c src/recomp/gen/recomp_0022.c src/recomp/gen/recomp_0026.c src/recomp/gen/recomp_0028.c
            src/recomp/gen/recomp_0033.c src/recomp/gen/recomp_0034.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${finish_base}" finish_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${finish_index})
    list(INSERT RECOMP_GEN_SOURCES ${finish_index} "${finish_source}")
endforeach()
option(UCRR_NATIVE_PARTICLE_EMITTER_GUEST "Opt-in native burst, continuous and finite particle emitter ownership" OFF)
option(UCRR_NATIVE_PARTICLE_EMITTER_EXTRA_GUEST "Opt-in native conditional and custom fire particle emitters" OFF)
option(UCRR_NATIVE_PARTICLE_SOURCE_MATH_GUEST "Opt-in native particle source interpolation and fixed array construction" OFF)
option(UCRR_NATIVE_PARTICLE_SOURCE_GUEST "Opt-in native fixed, attached and segment particle sources" OFF)
option(UCRR_NATIVE_PARTICLE_MATERIAL_GUEST "Opt-in native particle material setup and cleanup" OFF)
option(UCRR_NATIVE_PARTICLE_ROTATING_DRAW_GUEST "Opt-in native complete rotating particle draw owner" OFF)
option(UCRR_NATIVE_PARTICLE_REGISTRY_GUEST "Opt-in native particle renderer registration and removal" OFF)
option(UCRR_NATIVE_PARTICLE_QUAD_GUEST "Opt-in native complete shared quad particle draw owner" OFF)
option(UCRR_NATIVE_PARTICLE_TRIANGLE_DRAW_GUEST "Opt-in native complete triangle particle draw owner" OFF)
option(UCRR_NATIVE_PARTICLE_LINE_GEOMETRY_GUEST "Opt-in native complete Line particle geometry" OFF)
option(UCRR_NATIVE_PARTICLE_LINE_GUEST "Opt-in native complete Line particle draw owner" OFF)
option(UCRR_NATIVE_PARTICLE_SUBMISSION_GUEST "Opt-in native particle visibility submission and bounds copy" OFF)
option(UCRR_NATIVE_PARTICLE_RENDERER_GUEST "Opt-in native ordinary particle draw ownership" OFF)
option(UCRR_NATIVE_PARTICLE_FACTORY_GUEST "Opt-in native particle descriptor-to-emitter and family creation" OFF)
option(UCRR_NATIVE_PARTICLE_TREE_OWNER_GUEST "Opt-in native particle cache node insertion and erase ownership" OFF)
option(UCRR_NATIVE_CHAINSAW_SPARKS_GUEST "Opt-in native chainsaw spark lifecycle" OFF)
option(UCRR_NATIVE_HAZE_ROOM_GUEST "Opt-in native room haze lifecycle" OFF)
option(UCRR_NATIVE_PARTICLE_PARSER_GUEST "Opt-in native particle asset parsing" OFF)
option(UCRR_NATIVE_PARTICLE_PROPERTY_GUEST "Opt-in native particle property readers" OFF)
option(UCRR_NATIVE_BULLET_TRACER_OWNER_GUEST "Opt-in native bullet tracer lifecycle" OFF)
option(UCRR_NATIVE_FIRE_JET_GUEST "Opt-in native fire-jet lifecycle with original hazard policy" OFF)
option(UCRR_NATIVE_GRAPHICS_REFERENCE_GUEST "Opt-in native graphics reference and retirement owners" OFF)
option(UCRR_NATIVE_RENDER_TARGET_GUEST "Opt-in native color/depth target binding" OFF)
option(UCRR_NATIVE_DEBRIS_OWNER_GUEST "Opt-in native debris ownership and ordered scalar correction" OFF)
option(UCRR_NATIVE_DECAL_OWNER_GUEST "Opt-in native decal ownership and original primary deleting entry" OFF)
option(UCRR_NATIVE_SURFACE_OWNER_GUEST "Opt-in native texture surface creation and layout" OFF)
option(UCRR_NATIVE_PUSHBUFFER_REFILL_GUEST "Opt-in native command buffer refill and progress selection" OFF)
option(UCRR_NATIVE_SURFACE_LAYOUT_ACCESS_GUEST "Qualified texture layout access callers" OFF)
option(UCRR_NATIVE_EFFECT_SUPPORT_GUEST "Native effect setup and resource support" OFF)
option(UCRR_NATIVE_EFFECT_DESCRIPTOR_GUEST "Native authored effect descriptor copies" OFF)
option(UCRR_NATIVE_RESOURCE_PATH_GUEST "Opt-in native resource name ordering" OFF)
option(UCRR_NATIVE_VIEWPORT_STATE_GUEST "Opt-in native viewport and target state" OFF)
option(UCRR_NATIVE_SURFACE_ACCESS_GUEST "Opt-in native surface access with original synchronization" OFF)
option(UCRR_NATIVE_FX_FIRE_GUEST "Opt-in native fire lifecycle with original damage decisions" OFF)
option(UCRR_NATIVE_MATERIAL_WRITERS_GUEST "Opt-in native material and immediate command writers" OFF)
option(UCRR_NATIVE_SMOKE_ROOM_GUEST "Opt-in native SmokeRoom lifecycle" OFF)
option(UCRR_NATIVE_EXPANDING_SMOKE_GUEST "Opt-in native expanding smoke dynamics and ownership" OFF)
option(UCRR_NATIVE_TEXTURE_BIND_GUEST "Opt-in native texture-stage binding ownership" OFF)
option(UCRR_NATIVE_FX_ELECTRICITY_GUEST "Opt-in native electrical effect lifecycle with original gameplay decisions" OFF)
option(UCRR_NATIVE_BULLET_TRACER_RENDER_GUEST "Opt-in native original bullet tracer preparation and geometry" OFF)
option(UCRR_NATIVE_PARTICLE_ACOS_GUEST "Opt-in native finite particle birth acos service" OFF)
option(UCRR_NATIVE_PARTICLE_STRETCH_FILL_GUEST "Opt-in native complete stretched particle geometry and projection" OFF)
option(UCRR_NATIVE_PARTICLE_QUAD_FILL_GUEST "Opt-in native complete rotating-quad geometry and angle" OFF)
option(UCRR_NATIVE_PARTICLE_ROTATING_FILL_GUEST "Opt-in native complete rotating particle geometry with original scalar operand order" OFF)
option(UCRR_NATIVE_PARTICLE_PACKET_GUEST "Opt-in native graphics packet reserve and commit wrappers" OFF)
option(UCRR_NATIVE_PARTICLE_RANDOM_GUEST "Opt-in native shared random services for proven particle callers" OFF)
option(UCRR_NATIVE_PARTICLE_MOTION_BLUR_FILL_GUEST "Opt-in native complete motion-blur particle geometry" OFF)
option(UCRR_NATIVE_PARTICLE_BOUNDS_GUEST "Opt-in native particle simulation bounds construction" OFF)
option(UCRR_NATIVE_PARTICLE_TREE_LINKS_GUEST "Opt-in native particle-tree rotations and predecessor" OFF)
option(UCRR_NATIVE_PARTICLE_TREE_GUEST "Opt-in native particle resource-tree iteration and node lifetime" OFF)
option(UCRR_NATIVE_PARTICLE_SIMULATION_GUEST "Opt-in native complete particle record simulation" OFF)
option(UCRR_NATIVE_PARTICLE_STATUS_GUEST "Opt-in native particle family update and finished methods" OFF)
option(UCRR_NATIVE_PARTICLE_CONTROL_GUEST "Opt-in native particle control and manager resource ownership" OFF)
option(UCRR_NATIVE_PARTICLE_BIRTH_OWNER_GUEST "Opt-in native complete particle birth with original shared RNG and CRT" OFF)
option(UCRR_NATIVE_PARTICLE_LIFETIME_GUEST "Opt-in native six-family particle construction and cleanup" OFF)
option(UCRR_NATIVE_PARTICLE_RESOURCE_GUEST "Opt-in native particle resource load and lifetime ownership" OFF)
option(UCRR_NATIVE_PARTICLE_ASSET_GUEST "Opt-in native particle descriptor-cache lookup" OFF)
option(UCRR_NATIVE_EXPLOSION_VISUAL_GUEST "Opt-in native camera and distortion-ring visual ownership" OFF)
option(UCRR_NATIVE_EXPLOSION_FACTORY_GUEST "Opt-in native registered camera and distortion-ring factories" OFF)
option(UCRR_NATIVE_PARTICLE_ENTITY_GUEST "Opt-in native mission particle visual ownership" OFF)
option(UCRR_NATIVE_GUN_FLARE_RENDER_GUEST "Opt-in native muzzle-flare rendering geometry" OFF)
option(UCRR_NATIVE_PARTICLE_PREPARE_GUEST "Opt-in native five-family particle drawing preparation" OFF)
option(UCRR_NATIVE_PARTICLE_CURVE_GUEST "Opt-in native shared particle curve and parameter processing" OFF)
option(UCRR_NATIVE_PARTICLE_SPRITE_FILL_GUEST "Opt-in native complete ordinary particle geometry" OFF)
option(UCRR_NATIVE_GUN_FLARE_GUEST "Opt-in native muzzle-flare visual ownership" OFF)
option(UCRR_NATIVE_SHIELD_SPARK_GUEST "Opt-in native shield spark visual lifecycle" OFF)
option(UCRR_NATIVE_PARTICLE_PROBABILITY_GUEST "Opt-in native Lightning probability and original random stream" OFF)
option(UCRR_NATIVE_LIGHTNING_BEAM_RENDER_GUEST "Opt-in native complete lightning beam geometry and preparation" OFF)
option(UCRR_NATIVE_LIGHTNING_BEAM_GUEST "Opt-in native LightningBeam visual lifecycle and endpoints" OFF)
option(UCRR_NATIVE_LIGHT_FLASH_GUEST "Opt-in native transient light ownership with original light services" OFF)
option(UCRR_CRT_POW_BRANCH_FIX "Byte-proven CRT power branch consumes callee-returned ZF" OFF)
if(UCRR_CRT_POW_BRANCH_FIX)
    set(pow_matches "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0028[^/]*\\.c$")
            list(APPEND pow_matches "${source}")
        endif()
    endforeach()
    list(LENGTH pow_matches pow_count)
    if(NOT pow_count EQUAL 1)
        message(FATAL_ERROR "CRT power correction requires one selected chunk0028")
    endif()
    list(GET pow_matches 0 pow_base)
    set(pow_source "${CMAKE_BINARY_DIR}/guest_candidates/crt_pow/recomp_0028_crt_pow.c")
    add_custom_command(OUTPUT "${pow_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_crt_pow_branch.py"
            --input "${pow_base}" --output "${pow_source}"
        DEPENDS "${pow_base}" scripts/prepare_crt_pow_branch.py src/recomp/gen/recomp_0028.c
        VERBATIM)
    list(FIND RECOMP_GEN_SOURCES "${pow_base}" pow_index)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${pow_index})
    list(INSERT RECOMP_GEN_SOURCES ${pow_index} "${pow_source}")
endif()

# These owners span original chunks and must follow the existing inner kernels.
set(UC_NATIVE_EFFECT_STAGES particle_emitter particle_emitter_extra particle_source particle_material particle_renderer particle_submission particle_line particle_quad particle_triangle_draw particle_rotating_draw particle_registry particle_factory particle_asset particle_resource particle_lifetime particle_birth_owner explosion_visual explosion_factory light_flash particle_entity particle_control gun_flare particle_line_geometry particle_status particle_simulation particle_tree gun_flare_render particle_sprite_fill particle_prepare particle_curve particle_bounds particle_tree_links lightning_beam particle_random particle_motion_blur_fill shield_spark particle_probability particle_rotating_fill particle_packet particle_source_math particle_quad_fill particle_stretch_fill lightning_beam_render bullet_tracer_render particle_tree_owner particle_acos chainsaw_sparks haze_room particle_parser particle_property bullet_tracer_owner texture_bind fx_electricity smoke_room expanding_smoke material_writers fx_fire fire_jet graphics_reference debris_owner render_target decal_owner surface_owner pushbuffer_refill surface_access viewport_state resource_path effect_descriptor effect_support)
foreach(stage IN LISTS UC_NATIVE_EFFECT_STAGES)
    string(TOUPPER "${stage}" stage_upper)
    if(NOT UCRR_NATIVE_${stage_upper}_GUEST)
        continue()
    endif()
    if(stage STREQUAL "particle_emitter")
        set(owner_chunks 0001 0002 0015 0021 0023 0027)
    elseif(stage STREQUAL "particle_emitter_extra")
        set(owner_chunks 0001 0008 0014 0019)
    elseif(stage STREQUAL "particle_source")
        set(owner_chunks 0001 0016 0019)
    elseif(stage STREQUAL "particle_material" OR stage STREQUAL "particle_renderer" OR stage STREQUAL "particle_submission" OR stage STREQUAL "particle_line" OR stage STREQUAL "particle_quad" OR stage STREQUAL "particle_triangle_draw" OR stage STREQUAL "particle_rotating_draw" OR stage STREQUAL "decal_owner")
        set(owner_chunks 0006)
    elseif(stage STREQUAL "gun_flare" OR stage STREQUAL "gun_flare_render" OR stage STREQUAL "fx_fire")
        set(owner_chunks 0014)
    elseif(stage STREQUAL "particle_bounds" OR stage STREQUAL "particle_random" OR stage STREQUAL "particle_probability")
        set(owner_chunks 0027)
    elseif(stage STREQUAL "particle_control")
        set(owner_chunks 0019 0021)
    elseif(stage STREQUAL "particle_registry")
        set(owner_chunks 0005)
    elseif(stage STREQUAL "particle_factory" OR stage STREQUAL "particle_asset" OR stage STREQUAL "particle_lifetime" OR stage STREQUAL "particle_resource" OR stage STREQUAL "particle_birth_owner" OR stage STREQUAL "particle_line_geometry" OR stage STREQUAL "particle_status" OR stage STREQUAL "particle_simulation" OR stage STREQUAL "particle_tree" OR stage STREQUAL "particle_sprite_fill" OR stage STREQUAL "particle_prepare" OR stage STREQUAL "particle_curve" OR stage STREQUAL "particle_tree_links" OR stage STREQUAL "particle_motion_blur_fill" OR stage STREQUAL "particle_rotating_fill" OR stage STREQUAL "particle_quad_fill" OR stage STREQUAL "particle_stretch_fill" OR stage STREQUAL "particle_tree_owner" OR stage STREQUAL "particle_parser" OR stage STREQUAL "particle_property")
        set(owner_chunks 0021)
    elseif(stage STREQUAL "explosion_factory")
        set(owner_chunks 0009 0016)
    elseif(stage STREQUAL "bullet_tracer_render" OR stage STREQUAL "bullet_tracer_owner")
        set(owner_chunks 0004)
    elseif(stage STREQUAL "lightning_beam_render")
        set(owner_chunks 0013)
    elseif(stage STREQUAL "lightning_beam")
        set(owner_chunks 0013 0015)
    elseif(stage STREQUAL "particle_source_math")
        set(owner_chunks 0000 0008)
    elseif(stage STREQUAL "particle_acos")
        set(owner_chunks 0028)
    elseif(stage STREQUAL "effect_support")
        set(owner_chunks 0021 0028)
    elseif(stage STREQUAL "effect_descriptor")
        set(owner_chunks 0011)
    elseif(stage STREQUAL "resource_path")
        set(owner_chunks 0033)
    elseif(stage STREQUAL "particle_packet" OR stage STREQUAL "pushbuffer_refill")
        set(owner_chunks 0037)
    elseif(stage STREQUAL "chainsaw_sparks")
        set(owner_chunks 0016)
    elseif(stage STREQUAL "fire_jet")
        set(owner_chunks 0008 0015 0019)
    elseif(stage STREQUAL "smoke_room")
        set(owner_chunks 0010)
    elseif(stage STREQUAL "expanding_smoke")
        set(owner_chunks 0015 0016)
    elseif(stage STREQUAL "material_writers" OR stage STREQUAL "surface_owner" OR stage STREQUAL "surface_access" OR stage STREQUAL "viewport_state")
        set(owner_chunks 0036 0037)
    elseif(stage STREQUAL "texture_bind" OR stage STREQUAL "graphics_reference" OR stage STREQUAL "render_target")
        set(owner_chunks 0036)
    elseif(stage STREQUAL "debris_owner")
        set(owner_chunks 0002)
    elseif(stage STREQUAL "haze_room" OR stage STREQUAL "fx_electricity")
        set(owner_chunks 0009 0014)
    elseif(stage STREQUAL "shield_spark")
        set(owner_chunks 0011 0014)
    elseif(stage STREQUAL "light_flash")
        set(owner_chunks 0011 0013)
    elseif(stage STREQUAL "particle_entity")
        set(owner_chunks 0011 0012)
    elseif(stage STREQUAL "explosion_visual")
        set(owner_chunks 0009 0015 0016)
    else()
        message(FATAL_ERROR "No source chunks registered for native ${stage}")
    endif()
    foreach(chunk IN LISTS owner_chunks)
        set(owner_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_${chunk}[^/]*\\.c$")
                list(APPEND owner_matches "${source}")
            endif()
        endforeach()
        list(LENGTH owner_matches owner_count)
        if(NOT owner_count EQUAL 1)
            message(FATAL_ERROR "Native ${stage} requires one selected chunk${chunk}")
        endif()
        list(GET owner_matches 0 owner_base)
        set(owner_source "${CMAKE_BINARY_DIR}/guest_candidates/native_${stage}/recomp_${chunk}_native_${stage}.c")
        set(owner_args "")
        if(stage STREQUAL "surface_owner" OR stage STREQUAL "surface_access" OR stage STREQUAL "viewport_state" OR stage STREQUAL "effect_support")
            string(REGEX REPLACE "^0+" "" owner_chunk "${chunk}")
            set(owner_args --chunk "${owner_chunk}")
        endif()
        add_custom_command(OUTPUT "${owner_source}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_${stage}.py"
                --input "${owner_base}" --output "${owner_source}" ${owner_args}
            DEPENDS "${owner_base}" "scripts/prepare_native_${stage}.py" "src/hybrid_${stage}_guest.h"
                scripts/prepare_native_explosion_visual.py scripts/prepare_native_particle_emitter.py
                game_files/Default.xbe analysis/xbe_analysis.json src/recomp_manual.c src/recomp/recomp_types.h
                src/recomp/gen/recomp_0017.c src/recomp/gen/recomp_0024.c src/recomp/gen/recomp_0025.c
                src/recomp/gen/recomp_0000.c src/recomp/gen/recomp_0001.c src/recomp/gen/recomp_0002.c src/recomp/gen/recomp_0003.c
                src/recomp/gen/recomp_0004.c src/recomp/gen/recomp_0005.c src/recomp/gen/recomp_0006.c src/recomp/gen/recomp_0007.c src/recomp/gen/recomp_0008.c
                src/recomp/gen/recomp_0010.c src/recomp/gen/recomp_0009.c src/recomp/gen/recomp_0014.c src/recomp/gen/recomp_0015.c src/recomp/gen/recomp_0016.c
                src/recomp/gen/recomp_0011.c src/recomp/gen/recomp_0012.c src/recomp/gen/recomp_0013.c
                src/recomp/gen/recomp_0018.c src/recomp/gen/recomp_0019.c src/recomp/gen/recomp_0020.c src/recomp/gen/recomp_0021.c src/recomp/gen/recomp_0023.c
                src/recomp/gen/recomp_0026.c src/recomp/gen/recomp_0027.c src/recomp/gen/recomp_0028.c src/recomp/gen/recomp_0033.c src/recomp/gen/recomp_0034.c
                src/recomp/gen/recomp_0036.c src/recomp/gen/recomp_0037.c
            VERBATIM)
        list(FIND RECOMP_GEN_SOURCES "${owner_base}" owner_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${owner_index})
        list(INSERT RECOMP_GEN_SOURCES ${owner_index} "${owner_source}")
    endforeach()
endforeach()
if(UCRR_NATIVE_DECAL_OWNER_GUEST)
    set(decal_dispatch_base "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen/recomp_dispatch.c")
    set(decal_dispatch_source "${CMAKE_BINARY_DIR}/guest_candidates/native_decal_owner/recomp_dispatch_native_decal_owner.c")
    list(FIND RECOMP_GEN_SOURCES "${decal_dispatch_base}" decal_dispatch_index)
    if(decal_dispatch_index LESS 0)
        message(FATAL_ERROR "Native decal ownership requires the original dispatch source")
    endif()
    add_custom_command(OUTPUT "${decal_dispatch_source}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_decal_dispatch.py"
            --input "${decal_dispatch_base}" --output "${decal_dispatch_source}"
        DEPENDS "${decal_dispatch_base}" scripts/prepare_native_decal_dispatch.py scripts/prepare_native_decal_owner.py
            game_files/Default.xbe analysis/xbe_analysis.json
            src/recomp/gen/recomp_0005.c src/recomp/gen/recomp_0006.c
            src/recomp/recomp_types.h src/recomp/recomp_call_cache.h src/recomp_manual.c
        VERBATIM)
    list(REMOVE_AT RECOMP_GEN_SOURCES ${decal_dispatch_index})
    list(INSERT RECOMP_GEN_SOURCES ${decal_dispatch_index} "${decal_dispatch_source}")
endif()
option(UCRR_LIGHTING_LIST_PROBE "Isolated F8 census for pending lighting investigations" OFF)
if(UCRR_LIGHTING_LIST_PROBE)
    if(UCRR_PALETTE_FUSION_GUEST OR UCRR_SCENE_BRANCHLESS_GUEST)
        message(FATAL_ERROR "Guest candidates and observer derivation require separate builds")
    endif()
    if(UCRR_PLAY_ONLY OR NOT UCRR_FEATURE_REMOVAL)
        message(FATAL_ERROR "Lighting census requires diagnostic recorder boundaries")
    endif()
    foreach(chunk IN ITEMS 6 7)
        set(coverage_matches "")
        foreach(source IN LISTS RECOMP_GEN_SOURCES)
            if(source MATCHES "/recomp_000${chunk}[^/]*\\.c$")
                list(APPEND coverage_matches "${source}")
            endif()
        endforeach()
        list(LENGTH coverage_matches coverage_count)
        if(NOT coverage_count EQUAL 1)
            message(FATAL_ERROR "Lighting census needs one selected chunk ${chunk}")
        endif()
        list(GET coverage_matches 0 coverage_base${chunk})
    endforeach()
    set(coverage_dir "${CMAKE_BINARY_DIR}/guest_candidates/lighting_coverage")
    add_custom_command(OUTPUT "${coverage_dir}/recomp_0006_coverage.c" "${coverage_dir}/recomp_0007_coverage.c"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_guest_lighting_coverage.py"
            --base6 "${coverage_base6}" --base7 "${coverage_base7}" --output-dir "${coverage_dir}"
        DEPENDS "${coverage_base6}" "${coverage_base7}" scripts/prepare_guest_lighting_coverage.py src/guest_lighting_coverage.h
        VERBATIM)
    foreach(chunk IN ITEMS 6 7)
        list(FIND RECOMP_GEN_SOURCES "${coverage_base${chunk}}" coverage_index)
        list(REMOVE_AT RECOMP_GEN_SOURCES ${coverage_index})
        list(INSERT RECOMP_GEN_SOURCES ${coverage_index} "${coverage_dir}/recomp_000${chunk}_coverage.c")
    endforeach()
endif()
include(scripts/particles_integration.cmake)
include(scripts/combined_migration.cmake)
if(UCRR_XMV_SIMD AND MSVC)
    set(xmv_sources "")
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        if(source MATCHES "/recomp_0037[^/]*\\.c$")
            list(APPEND xmv_sources "${source}")
        endif()
    endforeach()
    list(LENGTH xmv_sources xmv_count)
    if(NOT xmv_count EQUAL 1)
        message(FATAL_ERROR "XMV SIMD requires one final selected chunk0037")
    endif()
    set_property(SOURCE ${xmv_sources} APPEND PROPERTY COMPILE_DEFINITIONS RECOMP_GENERATED_CODE)
    set_property(SOURCE ${xmv_sources} APPEND PROPERTY COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/xmv_simd.h")
endif()
set(UCRR_OVERLAY_FONT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LatoLatin-Regular.ttf")
configure_file(src/xbox_overlay_assets.rc.in xbox_overlay_assets.rc @ONLY)
add_executable(${PROJECT_NAME}
    "${CMAKE_CURRENT_BINARY_DIR}/xbox_overlay_assets.rc"
    src/main.c
    src/xbox_host.manifest
    src/recomp_manual.c
    ${RECOMP_GEN_SOURCES}
    ${UCRR_COMBINED_HELPERS}
)
if(UCRR_NATIVE_DEBRIS_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_debris_guest.c native/explosion_debris.c native/game_random.c)
    set_source_files_properties(src/hybrid_debris_guest.c native/explosion_debris.c native/game_random.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_TRIANGLE_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_triangle_guest.c native/collision_triangle.c native/collision_mesh.c)
    set_source_files_properties(src/hybrid_triangle_guest.c native/collision_triangle.c native/collision_mesh.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_MESH_FILTER_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_collision_guest.c native/collision_hit.c)
    set_source_files_properties(src/hybrid_collision_guest.c native/collision_hit.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_SCENERY_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_scenery_guest.c native/collision_mopp.c)
    set_source_files_properties(src/hybrid_scenery_guest.c native/collision_mopp.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_BODY_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_body_guest.c native/collision_body.c)
    set_source_files_properties(src/hybrid_body_guest.c native/collision_body.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_ANIMATION_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_animation_guest.c
        native/animation_blend.c native/animation_skeleton.c native/model_pose.c)
    set_source_files_properties(src/hybrid_animation_guest.c native/animation_blend.c
        native/animation_skeleton.c native/model_pose.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_ANIMATION_FINISH_GUEST OR UCRR_NATIVE_ANIMATION_SAMPLER_GUEST)
    if(NOT UCRR_NATIVE_ANIMATION_GUEST)
        target_sources(${PROJECT_NAME} PRIVATE native/animation_blend.c)
    endif()
    set_source_files_properties(native/animation_blend.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
    foreach(stage IN ITEMS animation_finish animation_sampler)
        string(TOUPPER "${stage}" stage_upper)
        if(UCRR_NATIVE_${stage_upper}_GUEST)
            target_sources(${PROJECT_NAME} PRIVATE "src/hybrid_${stage}_guest.c")
            set_source_files_properties("src/hybrid_${stage}_guest.c" PROPERTIES COMPILE_OPTIONS "/fp:strict")
        endif()
    endforeach()
endif()
if(UCRR_NATIVE_ANIMATION_SAMPLER_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE native/animation_asset.c)
    set_source_files_properties(native/animation_asset.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_ANIMATION_LINKED_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_animation_linked_guest.c)
    if(NOT UCRR_NATIVE_ANIMATION_GUEST)
        target_sources(${PROJECT_NAME} PRIVATE native/model_pose.c)
    endif()
    set_source_files_properties(src/hybrid_animation_linked_guest.c native/model_pose.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_PARTICLE_RECORD_GUEST OR UCRR_NATIVE_PARTICLE_ROTATING_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE native/particle_draw.c)
    set_source_files_properties(native/particle_draw.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_VISIBILITY_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_visibility_guest.c)
    set_source_files_properties(src/hybrid_visibility_guest.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_ANIMATION_FACE_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_animation_face_guest.c native/character_face.c)
    if(NOT UCRR_NATIVE_DEBRIS_GUEST)
        target_sources(${PROJECT_NAME} PRIVATE native/game_random.c)
    endif()
    set_source_files_properties(src/hybrid_animation_face_guest.c native/character_face.c native/game_random.c
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
foreach(stage IN ITEMS audio_source resource_stream resource_provider resource_lookup animation_owner animation_lifecycle animation_construct)
    string(TOUPPER "${stage}" stage_upper)
    if(UCRR_NATIVE_${stage_upper}_GUEST)
        target_sources(${PROJECT_NAME} PRIVATE "src/hybrid_${stage}_guest.c")
        set_source_files_properties("src/hybrid_${stage}_guest.c" PROPERTIES COMPILE_OPTIONS "/fp:strict")
        if(stage STREQUAL "animation_construct" OR stage STREQUAL "resource_lookup")
            set_source_files_properties("src/hybrid_${stage}_guest.c" PROPERTIES COMPILE_OPTIONS "/fp:precise;/Ob1")
        endif()
    endif()
endforeach()
foreach(stage IN ITEMS capsule relative contact_manager contact_installed solver solver_pair particle_motion particle_record particle_rotating particle_birth particle_owner particle_system effect_quota)
    string(TOUPPER "${stage}" stage_upper)
    if(UCRR_NATIVE_${stage_upper}_GUEST)
        target_sources(${PROJECT_NAME} PRIVATE "src/hybrid_${stage}_guest.c")
        set_source_files_properties("src/hybrid_${stage}_guest.c" PROPERTIES COMPILE_OPTIONS "/fp:strict")
        if(stage STREQUAL "particle_system" OR stage STREQUAL "effect_quota")
            set_source_files_properties("src/hybrid_${stage}_guest.c" PROPERTIES COMPILE_OPTIONS "/fp:precise;/Ob1")
        endif()
    endif()
endforeach()
foreach(stage IN LISTS UC_NATIVE_EFFECT_STAGES)
    string(TOUPPER "${stage}" stage_upper)
    if(UCRR_NATIVE_${stage_upper}_GUEST)
        set(helper_source "${CMAKE_CURRENT_SOURCE_DIR}/src/hybrid_${stage}_guest.c")
        if(stage STREQUAL "particle_random" AND UCRR_MIGRATION_PARTICLES_GUEST)
            set(helper_source "${CMAKE_CURRENT_SOURCE_DIR}/scripts/migration_particles_lightning/hybrid_particle_random_guest.c")
            if(UCRR_COMBINED_MIGRATION AND UCRR_COMBINED_PARTICLE_RNG)
                set(helper_source "${UCRR_COMBINED_PARTICLE_RNG}")
            endif()
        endif()
        if(stage STREQUAL "surface_owner" AND UCRR_NATIVE_SURFACE_LAYOUT_ACCESS_GUEST)
            set(helper_source "${CMAKE_BINARY_DIR}/guest_candidates/native_surface_layout_access/hybrid_surface_owner_guest.c")
            add_custom_command(OUTPUT "${helper_source}"
                COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/python.exe" -B
                    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_native_surface_layout_access.py"
                    --input "${CMAKE_CURRENT_SOURCE_DIR}/src/hybrid_surface_owner_guest.c" --output "${helper_source}"
                DEPENDS src/hybrid_surface_owner_guest.c scripts/prepare_native_surface_layout_access.py VERBATIM)
        endif()
        target_sources(${PROJECT_NAME} PRIVATE "${helper_source}")
        set_source_files_properties("${helper_source}" PROPERTIES COMPILE_OPTIONS "/fp:precise;/Ob1")
    endif()
endforeach()
if(UCRR_MIGRATION_PARTICLES_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE
        src/hybrid_effect_properties_guest.c
        src/migration_particles_lightning_guest.c
        src/migration_particles_flare_guest.c
        src/migration_particles_visual_routes_guest.c
        src/migration_particles_fire_cleanup_guest.c
        src/migration_particles_flare_recipe_guest.c
        src/migration_particles_listeners_guest.c
        src/migration_particles_control_leaves_guest.c
        src/migration_particles_smoke_registry_guest.c
        src/migration_particles_lifetime_leaves_guest.c
        src/migration_particles_boot_guest.c
        src/migration_particles_smoke_spheres_guest.c
        scripts/migration_particles_boot/manual_cleanup.c)
    set_source_files_properties(
        src/hybrid_effect_properties_guest.c
        src/migration_particles_lightning_guest.c
        src/migration_particles_flare_guest.c
        src/migration_particles_visual_routes_guest.c
        src/migration_particles_fire_cleanup_guest.c
        src/migration_particles_flare_recipe_guest.c
        src/migration_particles_listeners_guest.c
        src/migration_particles_control_leaves_guest.c
        src/migration_particles_smoke_registry_guest.c
        src/migration_particles_lifetime_leaves_guest.c
        src/migration_particles_boot_guest.c
        src/migration_particles_smoke_spheres_guest.c
        scripts/migration_particles_boot/manual_cleanup.c
        PROPERTIES COMPILE_OPTIONS "/fp:precise;/Ob1")
endif()
if(UCRR_NATIVE_PARTICLE_SIMULATION_GUEST AND NOT UCRR_NATIVE_PARTICLE_MOTION_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_particle_motion_guest.c)
    set_source_files_properties(src/hybrid_particle_motion_guest.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_NATIVE_PARTICLE_BIRTH_OWNER_GUEST AND NOT UCRR_NATIVE_PARTICLE_BIRTH_GUEST)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_particle_birth_guest.c)
    set_source_files_properties(src/hybrid_particle_birth_guest.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_HYBRID_NATIVE_LIGHTING)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_lighting_guest.c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_HYBRID_NATIVE_LIGHTING=1)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "${UC_ZLIB_DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif()
if(UCRR_HYBRID_PC_FRONTEND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_HYBRID_PC_FRONTEND=1)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_HYBRID_PC_FRONTEND=1)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_frontend_movie.c)
endif()
if(UCRR_HYBRID_NATIVE_PARTICLES)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_particles_guest.c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_HYBRID_NATIVE_PARTICLES=1)
endif()
if(UCRR_HYBRID_EXTENDED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_HYBRID_EXTENDED=1)
    target_sources(${PROJECT_NAME} PRIVATE src/hybrid_models_guest.c src/hybrid_models_prepare.c
        src/hybrid_models_spot.c src/hybrid_casters_guest.c)
    set_source_files_properties(src/hybrid_lighting_guest.c src/hybrid_models_guest.c src/hybrid_models_prepare.c
        src/hybrid_models_spot.c src/hybrid_casters_guest.c PROPERTIES COMPILE_OPTIONS "/fp:strict")
endif()
if(UCRR_LIGHTING_LIST_PROBE)
    target_sources(${PROJECT_NAME} PRIVATE src/guest_lighting_coverage.c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_LIGHTING_LIST_PROBE=1)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_LIGHTING_LIST_PROBE=1)
endif()
if(UCRR_CUSTOM_LIGHTING)
    target_sources(${PROJECT_NAME} PRIVATE analysis/custom_lighting/guest/custom_lighting_guest.c)
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/analysis/custom_lighting/guest")
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_CUSTOM_LIGHTING=1)
endif()

option(UCRR_PARTICLE_RESTORATION "Opt-in exact particle command writer with effects retained" OFF)
option(UCRR_DECAL_RESTORATION "Opt-in exact decal command writer with effects retained" OFF)
foreach(effect IN ITEMS LIGHT PARTICLE DECAL)
    if(UCRR_${effect}_RESTORATION)
        string(TOLOWER "${effect}" effect_lower)
        target_sources(${PROJECT_NAME} PRIVATE "src/${effect_lower}_restoration.c")
        set_property(SOURCE src/main.c src/recomp_manual.c "src/${effect_lower}_restoration.c"
            APPEND PROPERTY COMPILE_DEFINITIONS "UCRR_${effect}_RESTORATION=1")
        target_compile_definitions(xbox_d3d8 PRIVATE "UCRR_${effect}_RESTORATION=1")
    endif()
endforeach()
if(UCRR_PARTICLE_RESTORATION)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(particle_restoration_source "${CMAKE_BINARY_DIR}/guest_candidates/effect_restoration/particle_restoration_draw.c")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_particle_restoration.py"
        --output "${particle_restoration_source}" RESULT_VARIABLE particle_restoration_result)
    if(NOT particle_restoration_result EQUAL 0)
        message(FATAL_ERROR "Particle restoration derivation failed")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${particle_restoration_source}")
    set_property(SOURCE "${particle_restoration_source}" APPEND PROPERTY COMPILE_DEFINITIONS UCRR_PARTICLE_RESTORATION=1)
endif()
if(UCRR_DECAL_RESTORATION)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_decal_restoration.py"
        RESULT_VARIABLE decal_restoration_result)
    if(NOT decal_restoration_result EQUAL 0)
        message(FATAL_ERROR "Decal restoration derivation failed")
    endif()
    set_property(SOURCE src/decal_restoration.c APPEND PROPERTY COMPILE_DEFINITIONS RECOMP_GENERATED_CODE)
endif()
option(UCRR_FEATURE_REMOVAL "Separate opt-in particle and body blob-shadow render admission cuts" OFF)
if(UCRR_FEATURE_REMOVAL)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(registration_source "${CMAKE_CURRENT_BINARY_DIR}/individual_removal/particle_registration.c")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_particle_registration.py" --output "${registration_source}" RESULT_VARIABLE registration_result)
    if(NOT registration_result EQUAL 0)
        message(FATAL_ERROR "Particle registration derivation failed")
    endif()
    set(sky_source "${CMAKE_CURRENT_BINARY_DIR}/individual_removal/sky_visibility.c")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_sky_visibility.py" --output "${sky_source}" RESULT_VARIABLE sky_result)
    if(NOT sky_result EQUAL 0)
        message(FATAL_ERROR "Sky visibility derivation failed")
    endif()
      target_sources(${PROJECT_NAME} PRIVATE src/feature_removal.c "${registration_source}" "${sky_source}")
      set(glass_source "${CMAKE_CURRENT_BINARY_DIR}/individual_removal/glass_probe.c")
      execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_glass_probe.py" --output "${glass_source}" RESULT_VARIABLE glass_result)
      if(NOT glass_result EQUAL 0)
          message(FATAL_ERROR "Glass probe derivation failed")
      endif()
      target_sources(${PROJECT_NAME} PRIVATE "${glass_source}")
    set_property(SOURCE src/main.c src/recomp_manual.c src/recomp/gen/recomp_0006.c APPEND PROPERTY COMPILE_DEFINITIONS UCRR_FEATURE_REMOVAL=1)
    foreach(particle_source IN LISTS RECOMP_GEN_SOURCES)
        if(particle_source MATCHES "recomp_0006")
            set_property(SOURCE "${particle_source}" APPEND PROPERTY COMPILE_DEFINITIONS UCRR_FEATURE_REMOVAL=1)
        endif()
    endforeach()
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_FEATURE_REMOVAL=1)
endif()

option(UCRR_LIGHT_REMOVAL "Separate opt-in selected light Begin admission removal" OFF)
if(UCRR_LIGHT_REMOVAL)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_light_removal.py" --check
        RESULT_VARIABLE light_removal_check)
    if(NOT light_removal_check EQUAL 0)
        message(FATAL_ERROR "Selected light removal derivation is stale or inconsistent")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE src/light_removal.c src/light_removal_begin.c)
    set_property(SOURCE src/main.c src/recomp_manual.c src/light_removal.c src/light_removal_begin.c APPEND PROPERTY COMPILE_DEFINITIONS UCRR_LIGHT_REMOVAL=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_LIGHT_REMOVAL=1)
endif()

option(UCRR_DECAL_REMOVAL "Separate opt-in owned decal-manager render admission removal" OFF)
if(UCRR_DECAL_REMOVAL)
    target_sources(${PROJECT_NAME} PRIVATE src/decal_removal.c)
    set_property(SOURCE src/main.c src/recomp_manual.c src/decal_removal.c APPEND PROPERTY COMPILE_DEFINITIONS UCRR_DECAL_REMOVAL=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_DECAL_REMOVAL=1)
endif()

option(UCRR_SPECULAR_REMOVAL "Separate opt-in selected light specular render-read removal" OFF)
if(UCRR_SPECULAR_REMOVAL)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(specular_removal_source "${CMAKE_CURRENT_BINARY_DIR}/individual_removal/specular_material.c")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_specular_removal.py" --output "${specular_removal_source}"
        RESULT_VARIABLE specular_removal_check)
    if(NOT specular_removal_check EQUAL 0)
        message(FATAL_ERROR "Selected specular removal derivation failed")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE src/specular_removal.c "${specular_removal_source}")
    set_property(SOURCE src/main.c src/recomp_manual.c src/specular_removal.c "${specular_removal_source}" APPEND PROPERTY COMPILE_DEFINITIONS UCRR_SPECULAR_REMOVAL=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_SPECULAR_REMOVAL=1)
endif()

option(UCRR_GLOW_REMOVAL "Separate opt-in bounded glow filter and additive-composite removal" OFF)
if(UCRR_GLOW_REMOVAL)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(glow_removal_source "${CMAKE_CURRENT_BINARY_DIR}/individual_removal/glow_stage.c")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_glow_removal.py" --output "${glow_removal_source}"
        RESULT_VARIABLE glow_removal_check)
    if(NOT glow_removal_check EQUAL 0)
        message(FATAL_ERROR "Bounded glow removal derivation failed")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE src/glow_removal.c "${glow_removal_source}")
    set_property(SOURCE src/main.c src/recomp_manual.c src/glow_removal.c "${glow_removal_source}" APPEND PROPERTY COMPILE_DEFINITIONS UCRR_GLOW_REMOVAL=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_GLOW_REMOVAL=1)
endif()

option(UCRR_SHADOW_REMOVAL "Separate opt-in stencil-shadow cast removal, preserving lighting" OFF)
if(UCRR_SHADOW_REMOVAL)
    target_sources(${PROJECT_NAME} PRIVATE src/shadow_removal.c)
    set_property(SOURCE src/main.c src/recomp_manual.c src/shadow_removal.c APPEND PROPERTY COMPILE_DEFINITIONS UCRR_SHADOW_REMOVAL=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_SHADOW_REMOVAL=1)
endif()

if(UCRR_PALETTE_AFFINE_CANDIDATE)
    target_sources(${PROJECT_NAME} PRIVATE scripts/attribution_atlas/palette_affine_runtime.c)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${palette_runtime_dir}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/attribution_atlas"
        "${XBOXRECOMP_DIR}/src/kernel")
    set_property(SOURCE src/main.c APPEND PROPERTY COMPILE_DEFINITIONS UCRR_PALETTE_AFFINE_CANDIDATE=1)
endif()
if(UCRR_MATERIAL_OPCODE_PROBE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_MATERIAL_OPCODE_PROBE=1)
endif()
option(UCRR_LAZY_MIP_LAYOUT "Experimental upload-only mip layout construction" OFF)
option(UCRR_MIXED_COMMAND_PROBE "Bounded mixed decoder route diagnostic" OFF)
option(UCRR_TYPED_TEXTURE_ONLY "Diagnostic removal of early 2D work for typed lighting" OFF)
option(UCRR_CAUSAL_TRACE "Bounded nested CPU/resource attribution diagnostic" OFF)
if(UCRR_CAUSAL_TRACE)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_CAUSAL_TRACE=1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_CAUSAL_TRACE=1)
endif()
if(UCRR_TYPED_TEXTURE_ONLY)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_TYPED_TEXTURE_ONLY=1)
endif()
option(UCRR_PROGRAM_PACKET "Experimental ordered program-word packet handler" OFF)
option(UCRR_PREPARATION_COMMAND "Experimental independent preparation and command reductions" OFF)
if(UCRR_PREPARATION_COMMAND)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_PREPARATION_COMMAND=1)
endif()
if(UCRR_PROGRAM_PACKET)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_PROGRAM_PACKET=1)
endif()
if(UCRR_MIXED_COMMAND_PROBE)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_MIXED_COMMAND_PROBE=1)
endif()
if(UCRR_LAZY_MIP_LAYOUT)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_LAZY_MIP_LAYOUT=1)
endif()
option(UCRR_CERTIFICATE_READS "Experimental x64 fenced certificate observations" OFF)
if(UCRR_CERTIFICATE_READS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_CERTIFICATE_READS=1)
endif()
option(UCRR_GUEST_ADDRESS_SPLIT "Experimental ordinary guest address fast branch" OFF)
if(UCRR_GUEST_ADDRESS_SPLIT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UCRR_GUEST_ADDRESS_SPLIT=1)
endif()
option(UCRR_RELEASE_NO_LOG "Isolated RC experiment: compile out runtime log output" OFF)
option(UCRR_LEAN_RUNTIME "Compile out diagnostic-only hot-path work; preserve runtime semantics" OFF)
option(UCRR_PLAY_ONLY "Disable diagnostic recorders and observation counters for the playable package" OFF)
if(UCRR_PLAY_ONLY)
    if(NOT UCRR_RELEASE_NO_LOG OR NOT UCRR_LEAN_RUNTIME)
        message(FATAL_ERROR "Play-only requires the existing no-log and lean runtime policies")
    endif()
    foreach(play_target ${PROJECT_NAME} xbox_kernel xbox_d3d8 xbox_dsound xbox_apu xbox_nv2a xbox_input)
        target_compile_definitions(${play_target} PRIVATE UCRR_PLAY_ONLY=1)
    endforeach()
endif()
if(UCRR_LEAN_RUNTIME)
    foreach(lean_target ${PROJECT_NAME} xbox_kernel xbox_d3d8 xbox_dsound xbox_apu xbox_nv2a xbox_input)
        target_compile_definitions(${lean_target} PRIVATE UCRR_LEAN_RUNTIME=1)
    endforeach()
endif()
option(UCRR_RC_BASELINE "Use pre-equality-candidate shader comparison in RC experiment" OFF)
if(UCRR_RC_BASELINE)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_RC_BASELINE=1)
endif()
if(UCRR_RELEASE_NO_LOG)
    if(NOT MSVC)
        message(FATAL_ERROR "No-log RC currently verified only with MSVC")
    endif()
    foreach(rc_target ${PROJECT_NAME} xbox_kernel xbox_d3d8 xbox_dsound xbox_apu xbox_nv2a xbox_input)
        target_compile_definitions(${rc_target} PRIVATE UCRR_RELEASE_NO_LOG=1)
        target_compile_options(${rc_target} PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/release_no_log.h")
    endforeach()
endif()

# Controlled PGO experiments. Default scope retains the graphics-only workflow;
# guest scope is a separate opt-in and never instruments the graphics libraries.
option(UCRR_DMA_DIAGNOSTIC "Bounded DMA lookup workload/equivalence probe; not an FPS build" OFF)
if(UCRR_DMA_DIAGNOSTIC)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_DMA_DIAGNOSTIC=1)
endif()
option(UCRR_GRAPHICS_LTO "Cross-file optimization of NV2A/D3D runtime only" OFF)
if(UCRR_GRAPHICS_LTO)
    if(NOT MSVC)
        message(FATAL_ERROR "Graphics LTO currently requires MSVC")
    endif()
    foreach(lto_target xbox_nv2a xbox_d3d8)
        target_compile_options(${lto_target} PRIVATE $<$<CONFIG:RelWithDebInfo>:/GL>)
    endforeach()
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:RelWithDebInfo>:/LTCG>)
endif()
set(UCRR_PGO_PHASE "OFF" CACHE STRING "PGO: OFF, GENERATE, USE")
set_property(CACHE UCRR_PGO_PHASE PROPERTY STRINGS OFF GENERATE USE)
set(UCRR_PGO_PROFILE "" CACHE FILEPATH "Absolute .pgd path for selected PGO scope")
set(UCRR_PGO_SCOPE "GRAPHICS" CACHE STRING "Profile graphics runtime or generated GUEST code")
set_property(CACHE UCRR_PGO_SCOPE PROPERTY STRINGS GRAPHICS GUEST)
if(NOT UCRR_PGO_SCOPE MATCHES "^(GRAPHICS|GUEST)$")
    message(FATAL_ERROR "Invalid UCRR_PGO_SCOPE")
endif()
if(NOT UCRR_PGO_PHASE MATCHES "^(OFF|GENERATE|USE)$")
    message(FATAL_ERROR "Invalid UCRR_PGO_PHASE")
endif()
if(NOT UCRR_PGO_PHASE STREQUAL "OFF")
    if(NOT MSVC)
        message(FATAL_ERROR "This PGO workflow requires MSVC")
    endif()
    if(NOT IS_ABSOLUTE "${UCRR_PGO_PROFILE}" OR NOT UCRR_PGO_PROFILE MATCHES "\\.pgd$")
        message(FATAL_ERROR "PGO requires an absolute .pgd path")
    endif()
    if(UCRR_PGO_PHASE STREQUAL "USE" AND NOT EXISTS "${UCRR_PGO_PROFILE}")
        message(FATAL_ERROR "Cannot USE a missing PGO profile")
    endif()
    if(UCRR_PGO_SCOPE STREQUAL "GUEST")
        if(UCRR_GRAPHICS_LTO OR UCRR_COMPACT_PALETTE OR NOT UCRR_GUEST_LAYOUT STREQUAL "DEFAULT")
            message(FATAL_ERROR "Guest PGO must be isolated from graphics LTO, compact palette and layout candidates")
        endif()
        if(NOT RECOMP_GEN_SOURCES)
            message(FATAL_ERROR "Guest PGO requires generated sources")
        endif()
        set_property(SOURCE ${RECOMP_GEN_SOURCES} APPEND PROPERTY
            COMPILE_OPTIONS $<$<CONFIG:RelWithDebInfo>:/GL>)
    else()
        foreach(pgo_target xbox_nv2a xbox_d3d8)
            target_compile_options(${pgo_target} PRIVATE $<$<CONFIG:RelWithDebInfo>:/GL>)
        endforeach()
    endif()
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:RelWithDebInfo>:/LTCG>)
    if(UCRR_PGO_PHASE STREQUAL "GENERATE")
        if(UCRR_PGO_SCOPE STREQUAL "GUEST")
            target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RelWithDebInfo>:/FASTGENPROFILE:EXACT,PGD=${UCRR_PGO_PROFILE}>")
        else()
            target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RelWithDebInfo>:/GENPROFILE:EXACT,PGD=${UCRR_PGO_PROFILE}>")
        endif()
    else()
        target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RelWithDebInfo>:/USEPROFILE:PGD=${UCRR_PGO_PROFILE}>")
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen"
    "${XBOXRECOMP_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    xboxrecomp
    d3d11
    dxgi
    dxguid
    xinput
    winmm
    dbghelp
)
option(UCRR_TEXTURE_REASON_CAPTURE "F8-only texture fallback reason counters (diagnostic build)" OFF)
if(UCRR_TEXTURE_REASON_CAPTURE)
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_TEXTURE_REASON_CAPTURE=1)
    target_compile_definitions(xbox_d3d8 PRIVATE UCRR_TEXTURE_REASON_CAPTURE=1)
endif()
if(UCRR_RAM_WATCH_SHADOW)
    target_sources(${PROJECT_NAME} PRIVATE src/ram_write_watch.c src/ram_write_watch_second.c src/resource_revisions.c)
    foreach(watch_target ${PROJECT_NAME} xbox_kernel xbox_nv2a)
        target_compile_definitions(${watch_target} PRIVATE UCRR_RAM_WATCH_SHADOW=1)
        target_include_directories(${watch_target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${XBOXRECOMP_DIR}/src/kernel")
    endforeach()
    target_compile_definitions(xbox_nv2a PRIVATE UCRR_RESOURCE_REVISIONS=1)
    set_source_files_properties(src/ram_write_watch.c src/ram_write_watch_second.c PROPERTIES COMPILE_DEFINITIONS UCRR_RESOURCE_REVISIONS=1)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /bigobj
        $<$<CONFIG:Debug>:/Z7>
        $<$<CONFIG:Debug>:/Od>
        # The generated guest CPU code is the dominant startup cost.  This
        # project intentionally clears CMake's global configuration flags, so
        # release-like builds need their optimization level set explicitly.
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/Z7>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        /LARGEADDRESSAWARE
        $<$<CONFIG:Debug>:/DEBUG:FULL>
        $<$<CONFIG:Debug>:/OPT:NOREF>
        $<$<CONFIG:Debug>:/OPT:NOICF>
        $<$<CONFIG:Debug>:/MAP>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL>
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:ICF>
    )
endif()
