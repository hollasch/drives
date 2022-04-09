drives -- Print status of active windows drives
====================================================================================================

Description
------------
This command-line tool prints the status of all active drive letters on Windows. It handles local
drives, network-mapped drives, removable drives, and virtual drives (mapped via the `subst`
command).


Usage
------
    drives: Print Windows drive and volume information
    Usage:  drives [--json|-j] [--verbose|-v] [drive]
            [--help|-h|/?] [--version]

    This program prints drive information for all devices, network mappings, DOS
    devices, and drive substitutions.

    Options
        [drive]
            Optional drive letter for specific drive report (colon optional). If no
            drive is specified, reports information for all drives.

        --help, -h, /?
            Print help information.

        --json, -j
            Print full drive information in JSON format. To understand the file
            system flags, see documentation for the Windows function
            GetVolumeInformationW().

        --verbose, -v
            Print additional information. This switch is ignored if the `--json`
            option is supplied.

        --version
            Print program version.


Sample Output
--------------
    C:\> drives
    A: "Data"     69cc-bf14  Fixed     [NTFS]
    B: "Backup"   182b-29c2  Fixed     [NTFS]
    C: "System"   cf99-12f4  Fixed     [NTFS]
    D: "USB"      a0f3-59a6  Removable [FAT32]
    H: "NASStore" 9f8b-fd0f  Remote    [NTFS]  --> \\common\files
    R: -          -          CD-ROM    -
    T: "Data"     fa6f-f71d  Fixed     [NTFS]  === A:\setup
    X: "Scratch"  4fdd-258e  Fixed     [NTFS]  === C:\Users\Fred\scratch

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
