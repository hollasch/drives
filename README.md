`drives` — print status of active Windows drive letters
================================================================================

Description
-----------

This command-line tool prints the status of all active drive letters on Windows.
It handles local drives, network-mapped drives, and removable drives. This
command takes no arguments.


Usage
-----

    drives 1.0
    drives: Print drive and volume information.
    Usage:  drives [/?|-h|--help] [--version] [-v|--verbose] [-p|--parseable]
            [drive]

    Single letter options may use either dashes (-) or slashes (/) as option
    prefixes, and are case insensitive. This program also prints all network
    mappings and drive substitutions (see the 'subst' command).

    --help / -h       Print help information.

    --verbose / -v    Print verbose; print additional information (only affects
                      human format).

    --version         Print program version.

    --parseable / -p  Print results in machine-parseable format.

    [drive]           Drive letter for single drive report.


Sample Output
-------------

    C:\> drives
    A: "Data"    3642-e068  Fixed     [NTFS]
    C: "System"  b8c4-ce9e  Fixed     [NTFS]
    D: -         -          CD-ROM    -
    E: -         -          Removable -
    X: "Data"    3642-e068  Fixed     [NTFS] --> A:\setup\apps


Developing
----------

This project is managed with Visual Studio, and is built as a 64-bit
application. However, it is structured in a way that is friendly to add build
setups from other environments. Here's the overall tree structure:

    +---src
    ^---build
    ^---out
        ^---x64
            ^---Debug
            |   ^---intermediate
            |       ^---drives.tlog
            ^---Release
                ^---intermediate
                    ^---drives.tlog

Pure source files are in the `src` directory. The `build` directory contains all
of the configuration for build tools. For now, it just has the Visual Studio
build environment and an `nmake`-style makefile. All generated build output goes
to the `out` directory (it's always safe to delete the entire `out` directory).

The makefile just builds netuse, an experimental tool for now.


--------------------------------------------------------------------------------
Steve Hollasch <steve@hollasch.net>  
https://github.com/hollasch/drives  
2016 Oct 16
