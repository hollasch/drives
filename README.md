`drives` — print status of active Windows drive letters
====================================================================================================

Description
------------
This command-line tool prints the status of all active drive letters on Windows. It handles local
drives, network-mapped drives, removable drives, and virtual drives (mapped via the `subst`
command).


Usage
------
    drives v2.0.0 - print Windows drive and volume information
    Source: https://github.com/hollasch/drives

    Usage: drives [/?|-h|--help] [--version]
           [-v|--verbose] [-p|--parseable] [drive]

    This program also prints all network mappings and drive substitutions
    (see the 'subst' command).

    [drive]
        Optional drive letter for specific drive report (colon optional)

    --help, -h, /?
        Print help information.

    --verbose, -v
        Print verbose; print additional information (only affects human format).

    --version
        Print program version.

    --parseable, -p
        Print results in machine-parseable format.


Sample Output
--------------
    C:\> drives
    A: "Data"    3642-e068  Fixed     [NTFS]
    C: "System"  b8c4-ce9e  Fixed     [NTFS]
    D: -         -          CD-ROM    -
    E: -         -          Removable -
    X: "Data"    3642-e068  Fixed     [NTFS] --> A:\setup\apps


Building
----------
This project uses the CMake build tool. CMake is a meta-build system that locates and uses your
local development tools to build the project if possible.

To build, first install [CMake][https://cmake.org/]. Then go to the project root directory and run
the following command:

    cmake -B build

This will locate your installed development tools and configure your project build in the `build/`
directory. After that, whenever you want a new build, run this command:

    cmake --build build

This will build a debug version of the project, located in `build/Debug/`. To build a release
version, run

    cmake --build build --config release

You can find the built release executable in `build/Release/`.


--------------------------------------------------------------------------------
Steve Hollasch <steve@hollasch.net><br>
https://github.com/hollasch/drives
