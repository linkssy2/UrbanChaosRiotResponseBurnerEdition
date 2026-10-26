cmake_minimum_required(VERSION 3.20)
project(xbe_coop_preview_assets_checks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
add_subdirectory(../../native/texture_asset texture)
add_executable(test_xbe_coop_preview_assets ../test_xbe_coop_preview_assets.cpp ../xbe_coop_preview_assets.cpp
    ../../native/model_asset.c ../../native/model_pose.c ../../native/model_render.c
    ../../native/material_asset.c ../../native/texture_sequence.c
    ../../native/animation_asset.c ../../native/animation_sample.c
    ../../native/animation_blend.c ../../native/animation_skeleton.c ../../native/animation_binding.c)
target_include_directories(test_xbe_coop_preview_assets PRIVATE ../..)
target_link_libraries(test_xbe_coop_preview_assets PRIVATE uc_native_texture)
target_compile_options(test_xbe_coop_preview_assets PRIVATE /W4 /WX /O2 /fp:strict)
target_compile_definitions(test_xbe_coop_preview_assets PRIVATE _CRT_SECURE_NO_WARNINGS)
add_custom_command(TARGET test_xbe_coop_preview_assets POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${UC_ZLIB_DLL}" "$<TARGET_FILE_DIR:test_xbe_coop_preview_assets>")
