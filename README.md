drives: Print status of active Windows drive letters
================================================================================

DESCRIPTION
-----------

This command-line tool prints the status of all active drive letters on Windows.
It handles local drives, network-mapped drives, and removable drives. This
command takes no arguments.


BUILDING
--------

This project is managed with Visual Studio. It is built as a 64-bit application.

There's also a makefile that will build netuse, an experimental tool for now.


SAMPLE OUTPUT
-------------

    C:\> drives
    A: "Data"    3642-e068  Fixed     [NTFS]
    C: "System"  b8c4-ce9e  Fixed     [NTFS]
    D: -         -          CD-ROM    -
    E: -         -          Removable -
    X: "Data"    3642-e068  Fixed     [NTFS] --> A:\setup\apps


--------------------------------------------------------------------------------
Steve Hollasch <steve@hollasch.net>
https://github.com/hollasch/drives
2016 Oct 16
