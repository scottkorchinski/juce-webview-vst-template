This folder contains shared CMake helpers used by the template build.

- `cpm.cmake` bootstraps the vendored `CPM.cmake` dependency manager into `libs/cpm/`.
- The root `CMakeLists.txt` includes it before fetching JUCE.

These files are part of the reusable template infrastructure and usually do not need per-plugin edits.
