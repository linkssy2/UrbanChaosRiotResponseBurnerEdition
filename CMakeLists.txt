cmake_minimum_required(VERSION 3.20)
project(UrbanChaosBurnerEdition LANGUAGES NONE)

# The maintained builder owns the pinned XboxXBERecomp SDK and qualified title inputs.
find_program(UCRR_PWSH NAMES pwsh REQUIRED)
add_custom_target(UrbanChaosXbeTrial ALL
    COMMAND "${CMAKE_COMMAND}" -E env UCRR_XBE_RELEASE=1
        "${UCRR_PWSH}" -NoProfile -File
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_animation_resume.ps1"
        -Script "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_xbe_game_trial.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM)
