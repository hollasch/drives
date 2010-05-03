/*==============================================================================

    drives

    Experimental program to print out volume information about all drives.

==============================================================================*/

#include <stdlib.h>
#include <stdio.h>

#define _WIN32_WINNT 0x501   // Windows XP or Greater

#include <windows.h>

// Program Parameters

const bool fPrintFileSysInfo = false;

void  ExpandFileSysFlags (const char *prefix, DWORD flags);
char *DriveDesc (UINT type);

inline static void print (char *string) { fputs (string, stdout); }


//==================================================================================================

int main (int, char* [])
{
    DWORD logdrives = GetLogicalDrives();

    char    drive[]  = "A:\\";
    wchar_t wdrive[] = L"A:";

    DWORD    netmap_size = 256;
    wchar_t *netmap = new wchar_t [netmap_size];

    // Get maximum field lengths.

    int maxLabelLen = 0;

    for (drive[0]='A';  drive[0] <= 'Z';  ++ drive[0])
    {
        wdrive[0] = L'A' + (drive[0] - 'A');

        char  volLabel[256];
        DWORD serialnum = 0;
        DWORD maxcomponentlen = 0;
        DWORD filesysflags = 0;
        char  filesysname [256];

        bool fVolInfoValid =
            0 != GetVolumeInformation (
                     drive, volLabel, sizeof(volLabel), &serialnum,
                     &maxcomponentlen, &filesysflags,
                     filesysname, sizeof(filesysname));

        int labelLen = strlen(volLabel);

        if (maxLabelLen < labelLen)
            maxLabelLen = labelLen;
    }

    // Print information for each drive.

    for (drive[0]='A';  drive[0] <= 'Z';  ++ drive[0])
    {
        wdrive[0] = L'A' + (drive[0] - 'A');

        UINT type = GetDriveType (drive);

        if (type == DRIVE_NO_ROOT_DIR)
        {
            if (logdrives & (1 << (drive[0]-'A')))
            {
                printf ("%c:   Drive type is %s, but GetLogicalDrives reports "
                        "no drive there.\n\n", drive[0], DriveDesc(type));
            }
            continue;
        }

        char  volLabel[256];
        DWORD serialnum = 0;
        DWORD maxcomponentlen = 0;
        DWORD filesysflags = 0;
        char  filesysname [256];

        bool fVolInfoValid =
            0 != GetVolumeInformation (
                     drive, volLabel, sizeof(volLabel), &serialnum,
                     &maxcomponentlen, &filesysflags,
                     filesysname, sizeof(filesysname));

        // Print drive letter.

        printf ("%c: ", drive[0]);

        // Print volume label.

        int labelLen = 0;
        if (fVolInfoValid && volLabel[0])
        {
            printf ("\"%s\"", volLabel);
            labelLen = strlen(volLabel);
        }
        else
        {
            print  ("  ");
        }

        // Pad the label to fit the maximum label length.

        if (labelLen < maxLabelLen);
            printf ("%*s", maxLabelLen - labelLen + 2, "  ");

        // Print the drive serial number.

        if (!fVolInfoValid)
            print ("           ");
        else
        {
            printf ("%04x-%04x  ", serialnum >> 16, serialnum & 0xffff);
        }

        // Print the unique volume name.

        const int max_volname = 50;
        char volname [max_volname] = "";

        if ((0 == GetVolumeNameForVolumeMountPoint (drive, volname, max_volname)) && (volname[0] != 0))
        {
            printf ("    Unique Volume Name: %s\n", volname);
        }

        // Print the drive description.

        printf ("%s  ", DriveDesc(type));

        // Print the file system type.

        if (fVolInfoValid && filesysname[0])
            printf ("[%s]  ", filesysname);

        // Print the net connection, if any.

        DWORD netConnRetVal;

        do {
            netConnRetVal = WNetGetConnectionW (wdrive, netmap, &netmap_size);

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
            if (fPrintFileSysInfo)
            {
                printf ("\n    Max Component Len = %d, FSys Flags = 0x%08x\n",
                    maxcomponentlen, filesysflags);

                ExpandFileSysFlags ("    ", filesysflags);
            }
        }
        printf ("\n");
    }
}



//==================================================================================================

void ExpandFileSysFlags (const char *prefix, DWORD flags)
{
    printf ("%s%s:  Supports named streams\n",
        prefix, (flags & FILE_NAMED_STREAMS) ? "Yes" : "No ");

    printf ("%s%s:  Supports object identifiers\n",
        prefix, (flags & FILE_SUPPORTS_OBJECT_IDS) ? "Yes" : "No ");

    printf ("%s%s:  Supports re-parse points\n",
        prefix, (flags & FILE_SUPPORTS_REPARSE_POINTS) ? "Yes" : "No ");

    printf ("%s%s:  Supports sparse files\n",
        prefix, (flags & FILE_SUPPORTS_SPARSE_FILES) ? "Yes" : "No ");

    printf ("%s%s:  Supports disk quotas\n",
        prefix, (flags & FILE_VOLUME_QUOTAS) ? "Yes" : "No ");

    printf ("%s%s:  Supports case-sensitive file names\n",
        prefix, (flags & FS_CASE_SENSITIVE) ? "Yes" : "No ");

    printf ("%s%s:  Supports file-based compression\n",
        prefix, (flags & FS_FILE_COMPRESSION) ? "Yes" : "No ");

    printf ("%s%s:  Supports Encrypted File System (EFS)\n",
        prefix, (flags & FS_FILE_ENCRYPTION) ? "Yes" : "No ");

    printf ("%s%s:  Supports Unicode file names\n",
        prefix, (flags & FS_UNICODE_STORED_ON_DISK) ? "Yes" : "No ");

    printf ("%s%s:  Preserves file name case\n",
        prefix, (flags & FS_CASE_IS_PRESERVED) ? "Yes" : "No ");

    printf ("%s%s:  Preserves and enforces access control lists (ACLs)\n",
        prefix, (flags & FS_PERSISTENT_ACLS) ? "Yes" : "No ");

    printf ("%s%s:  Volume is compressed\n",
        prefix, (flags & FS_VOL_IS_COMPRESSED) ? "Yes" : "No ");

    return;
}



/* ========================================================================= */

char *DriveDesc (UINT type)
{
    switch (type)
    {
        case DRIVE_UNKNOWN:      return "Unknown  ";
        case DRIVE_NO_ROOT_DIR:  return "No root  ";
        case DRIVE_REMOVABLE:    return "Removable";
        case DRIVE_FIXED:        return "Fixed    ";
        case DRIVE_REMOTE:       return "Remote   ";
        case DRIVE_CDROM:        return "CD-ROM   ";
        case DRIVE_RAMDISK:      return "RAM Disk ";
    }
    return "Unknown type";
}
