//==================================================================================================
//
//  drives
//
//  Experimental program to print out volume information about all drives.
//
//==================================================================================================

#include <stdlib.h>
#include <stdio.h>

#define _WIN32_WINNT 0x501   // Windows XP or Greater

#include <windows.h>

// Program Parameters

#define fPrintFileSysInfo 0

void     ExpandFileSysFlags (const wchar_t *prefix, DWORD flags);
wchar_t *DriveDesc (UINT type);


//==================================================================================================
// Simple string print helper function.
//==================================================================================================

static void print (wchar_t *string)
{
    fputws (string, stdout);
}


//==================================================================================================
// Main Program Entry Point
//==================================================================================================

int main (int, char* [])
{
    DWORD logicalDrives = GetLogicalDrives();

    wchar_t drive[]  = L"A:\\";
    wchar_t volumeLabel[256];
    wchar_t filesysname [256];

    DWORD    netmap_size = 256;
    wchar_t *netmap = new wchar_t [netmap_size];

    // Get maximum field lengths.

    size_t maxLabelLen = 0;

    for (drive[0]=L'A';  drive[0] <= L'Z';  ++ drive[0])
    {
        DWORD serialNumber = 0;
        DWORD maxComponentLength = 0;
        DWORD fileSysFlags = 0;

        auto retval = GetVolumeInformation (
                          drive, volumeLabel, sizeof(volumeLabel), &serialNumber, &maxComponentLength,
                          &fileSysFlags, filesysname, sizeof(filesysname));

        if (retval != 0)
            maxLabelLen = max (maxLabelLen, wcslen(volumeLabel));
    }

    // Print information for each drive.

    for (drive[0]='A';  drive[0] <= 'Z';  ++ drive[0])
    {
        UINT type = GetDriveType (drive);

        if (type == DRIVE_NO_ROOT_DIR)
        {
            if (logicalDrives & (1 << (drive[0]-'A')))
            {
                wprintf (L"%c:   Drive type is %s, but GetLogicalDrives reports "
                         L"no drive there.\n\n", drive[0], DriveDesc(type));
            }
            continue;
        }

        DWORD serialNumber = 0;
        DWORD maxComponentLength = 0;
        DWORD fileSysFlags = 0;

        auto retval = GetVolumeInformation (
                          drive, volumeLabel, sizeof(volumeLabel), &serialNumber, &maxComponentLength,
                          &fileSysFlags, filesysname, sizeof(filesysname));
        auto fVolInfoValid = (retval != 0);

        // Print drive letter.

        wprintf (L"%c: ", drive[0]);

        // Print volume label.

        size_t labelLen = 0;
        if (fVolInfoValid && volumeLabel[0])
        {
            wprintf (L"\"%s\"", volumeLabel);
            labelLen = wcslen(volumeLabel);
        }
        else
        {
            print (L"  ");    // Make up for no double quote characters.
        }

        // Pad the label to fit the maximum label length.

        if (labelLen < maxLabelLen)
            wprintf (L"%*s", maxLabelLen - labelLen, L"");

        // Print the drive serial number.

        if (!fVolInfoValid)
            print (L"             ");
        else
            wprintf (L"  %04x-%04x  ", serialNumber >> 16, serialNumber & 0xffff);

        // Print the unique volume name.

        const int volumeNameMaxLen = 50;
        wchar_t volumeName [volumeNameMaxLen] = L"";

        if (0 == GetVolumeNameForVolumeMountPoint (drive, volumeName, volumeNameMaxLen))
            if (volumeName[0] != 0)
                wprintf (L"    Unique Volume Name: %s\n", volumeName);

        // Print the drive description.

        wprintf (L"%s  ", DriveDesc(type));

        // Print the file system type.

        if (fVolInfoValid && filesysname[0])
            wprintf (L"[%s]  ", filesysname);

        // Print the net connection, if any.

        DWORD netConnRetVal;

        do {
            size_t driveNoSlashSize = wcslen(drive) + 1;
            wchar_t *driveNoSlash = new wchar_t[driveNoSlashSize];
            wcscpy_s (driveNoSlash, driveNoSlashSize, drive);
            wchar_t *endPtr = driveNoSlash + wcslen(driveNoSlash) - 1;
            while ((endPtr >= driveNoSlash) && (*endPtr == '\\'))
            {
                *endPtr = 0;
                --endPtr;
            }

            netConnRetVal = WNetGetConnectionW (driveNoSlash, netmap, &netmap_size);

            if (netConnRetVal == NO_ERROR)
            {   wprintf (L"--> \"%s\"  ", netmap);
            }
            else if (netConnRetVal == ERROR_MORE_DATA)
            {   delete netmap;
                netmap = new wchar_t [netmap_size];
            }

        } while (netConnRetVal == ERROR_MORE_DATA);

        // Print the volume information.

        if (fVolInfoValid)
        {
            #if PrintFileSystemInfo
            {
                wprintf (L"\n    Max Component Len = %d, FSys Flags = 0x%08x\n", maxComponentLength, fileSysFlags);
                ExpandFileSysFlags (L"    ", fileSysFlags);
            }
            #endif
        }
        wprintf (L"\n");
    }
}



//==================================================================================================
// Prints detailed information from the file system flags.
//==================================================================================================

void ExpandFileSysFlags (const wchar_t *prefix, DWORD flags)
{
    wprintf (L"%s%s:  Supports named streams\n",
        prefix, (flags & FILE_NAMED_STREAMS) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports object identifiers\n",
        prefix, (flags & FILE_SUPPORTS_OBJECT_IDS) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports re-parse points\n",
        prefix, (flags & FILE_SUPPORTS_REPARSE_POINTS) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports sparse files\n",
        prefix, (flags & FILE_SUPPORTS_SPARSE_FILES) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports disk quotas\n",
        prefix, (flags & FILE_VOLUME_QUOTAS) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports case-sensitive file names\n",
        prefix, (flags & FS_CASE_SENSITIVE) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports file-based compression\n",
        prefix, (flags & FS_FILE_COMPRESSION) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports Encrypted File System (EFS)\n",
        prefix, (flags & FS_FILE_ENCRYPTION) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Supports Unicode file names\n",
        prefix, (flags & FS_UNICODE_STORED_ON_DISK) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Preserves file name case\n",
        prefix, (flags & FS_CASE_IS_PRESERVED) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Preserves and enforces access control lists (ACLs)\n",
        prefix, (flags & FS_PERSISTENT_ACLS) ? L"Yes" : L"No ");

    wprintf (L"%s%s:  Volume is compressed\n",
        prefix, (flags & FS_VOL_IS_COMPRESSED) ? L"Yes" : L"No ");

    return;
}



//==================================================================================================
// Returns the string value for drive type values.
//==================================================================================================

wchar_t *DriveDesc (UINT type)
{
    switch (type)
    {
        case DRIVE_UNKNOWN:      return L"Unknown  ";
        case DRIVE_NO_ROOT_DIR:  return L"No root  ";
        case DRIVE_REMOVABLE:    return L"Removable";
        case DRIVE_FIXED:        return L"Fixed    ";
        case DRIVE_REMOTE:       return L"Remote   ";
        case DRIVE_CDROM:        return L"CD-ROM   ";
        case DRIVE_RAMDISK:      return L"RAM Disk ";
    }
    return L"ERR{Unknown Drive Type}";
}
