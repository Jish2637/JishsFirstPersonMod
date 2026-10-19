Historical prototype only. The working test integration is now in src/release_updates.cpp, src/update_core.cpp and src/update_helper.cpp. See docs/UPDATER_TEST1.md.

# Experimental release updater, NOT integrated

This is a preliminary WinHTTP/GitHub metadata and download experiment. It is not compiled into the mod, does not stage or install archives, does not verify the downloaded ZIP's SHA-256 digest, and does not implement DLL lifecycle management. It must not be shipped or enabled until those capabilities have been written, audited, and tested on Windows.

Safe hot-reloading is blocked by the companion's permanent module pin and native hooks that do not yet implement a complete, quiescent shutdown. A Windows/MSVC build is required by CMakeLists.txt, unavailable in the current environment.

The existing 1.35.0 mod and its build configuration are intentionally unchanged.
