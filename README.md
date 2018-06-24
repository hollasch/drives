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


Developing
-----------
This project is managed with Visual Studio, and is built as a 64-bit
application. However, it is structured in a way that is friendly to add build
setups from other environments. Here's the overall tree structure:

    +---build
    +---out
    |   +---x64
    |       +---Debug
    |       |   +---intermediate
    |       |       +---drives.tlog
    |       |
    |       +---Release
    |           +---intermediate
    |               +---drives.tlog
    +---src

The `build` directory contains all of the configuration for build tools. For
now, it just has the Visual Studio build environment and an nmake-style
makefile.

The `out` directory receives all generated build output (it's always safe to
delete the entire `out` directory).

The `src` directory contains source C++ files.


--------------------------------------------------------------------------------
Steve Hollasch <steve@hollasch.net><br>
https://github.com/hollasch/drives
