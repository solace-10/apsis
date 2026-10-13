# Keeping CMake's own warnings useful in a project that vendors its dependencies.
#
# Everything FetchExternalDependencies.cmake pulls in brings an upstream CMakeLists that nobody
# consuming pandora can fix, and between them they emit a dozen-odd warnings: CMP0115 in bullet3,
# pre-3.10 cmake_minimum_required in clipp, freetype, glm and xxhash, syntax warnings in dawn.
# Left alone they bury the warnings that are actionable, and a project that has learned to scroll
# past its configure output has stopped reading it.
#
# The suppression has to be global - the consuming project configures with -Wno-dev/-Wno-deprecated,
# normally via the `warnings` field of its CMakePresets.json. Scoping it to the dependencies instead
# does not work: cmake_diagnostic() only governs diagnostics raised while a directory is being
# processed, and these are not. The bullet3 CMP0115 warnings are raised at generate time (which is
# why FetchExternalDependencies.cmake silences those by pinning the policy instead), and dawn's
# syntax warnings come from the listfile parser, which never consults the diagnostic state at all.
#
# So the warnings are turned off for everyone and turned back on for the code the project owns,
# which is what the macro below is for.

# Re-enable CMake's author, policy and deprecation warnings for the calling directory and every
# subdirectory added after this point.
#
# Call it from your top-level CMakeLists *after* add_subdirectory(pandora), and before adding your
# own subdirectories - the diagnostic state a subdirectory records is the one in force while it is
# processed, so calling any earlier would bring the dependencies' warnings back with it.
#
# This is a macro rather than a function on purpose: it must run in the caller's diagnostic scope,
# and a function would get its own.
#
# Requires CMake 4.4 for cmake_diagnostic(). On older versions this is a no-op and the project's own
# CMakeLists stay quiet along with the dependencies, which is the lesser of the two problems.
macro(pandora_enable_project_diagnostics)
    if(COMMAND cmake_diagnostic)
        cmake_diagnostic(PROMOTE CMD_AUTHOR WARN RECURSE)
    endif()
endmacro()
