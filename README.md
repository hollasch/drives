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
    usage : drives  [--json|-j] [--verbose|-v] [drive]
                    [--help|-h|/?] [--version]

    This program prints drive information for all devices, network mappings, DOS
    devices, and drive substitutions (via the `subst` command).

    Unless the `--json` option is supplied, the following drive values will be
    printed, in this order:

        - Drive Letter
        - Label
        - Serial Number
        - Type (No root, Removable, Fixed, Remote, CD-ROM, or RAM Disk)
        - File System (for example, NTFS, FAT, or FAT32)
        - Volume GUID, drive substitution or target, or network mapping

    The volume GUID can be used in a formal volume name, with the following form:

        \\?\Volume{GUID}\

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
            Generally, print additional volume information. This switch is ignored
            if the `--json` option is supplied. Additional volume information
            includes the amount of free space and the total drive capacity.

        --version
            Print program version.

    drives v3.0.0 | 2022-04-22 | https://github.com/hollasch/drives

Sample Output
--------------

    C:\> drives
    A: "Data"    69cc-bf14  Fixed      NTFS    0f355eb5-6b2d-4583-8c15-350f09a59add
    B: "Backup"  182b-29c2  Fixed      NTFS    65b25216-ace2-4bc0-a768-00cd5ce46082
    C: "System"  cf99-12f4  Fixed      NTFS    c25514ce-c902-460a-aee5-a15d63e154ca
    D: "ESD-USB" a0f3-59a6  Fixed      FAT32   ca8f1d7e-f70c-40c3-b71f-6aad1b4f4848
    E: -         9f8b-fd0f  Removable  FAT32   2b3413e6-3be6-4376-8d46-282aed06561b
    F: "Data"    fa6f-f71d  Fixed      NTFS    === A:\setup\tools
    G: "Games"   4fdd-258e  Fixed      NTFS    b4f4619b-838c-484e-96f8-05a1bfd96a7a
    R: -         -          CD-ROM     -
    S: "Scratch" 0083-3922  Fixed      NTFS    ce38d56e-b4c0-477f-b77c-866b629c0c87
    V: "NAS3"    7f10-6416  Remote     NTFS    --> \\common\files
    X: "Data"    2175-ff77  Remote     NTFS    --> \\klaatu\a$


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
