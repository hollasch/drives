Change Log -- drives
====================================================================================================

# v3.0.0  (in progress)

## Removed
  - Dropped the old machine-parseable output option in exchange for the new JSON output option.

## Changed
  - Converted project from a fixed Visual Studio build to use CMake instead, assuming `build/` as
    the build directory.
  - Lots of code refactoring and rewrites.
  - Print drive GUID in human output instead of verbose output.

## Added
  - New JSON output option: `--json`.
  - Report drive capacity and usage in verbose output.


----------------------------------------------------------------------------------------------------
# v2.0.0  (2018-06-23)

## Removed
  - No more support for `/p`, `/v`, or `/h`. Use `-p`, `-v` or `-h` only. `/?` support remains.

## Changed
  - Converted to MIT license
  - Upgraded to Visual Studio 2017
  - Targets Windows SDK 10.0.17134.0


----------------------------------------------------------------------------------------------------
# v1.0.0  (2016-10-16)

## New
  - First release
