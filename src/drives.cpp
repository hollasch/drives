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

#include <string>

using namespace std;

// Program Parameters

#define PrintFileSysInfo  0
#define PrintNewDriveInfo 1

void     ExpandFileSysFlags (const wchar_t *prefix, DWORD flags);
wchar_t *DriveDesc (UINT type);



class DriveInfo
{
    // This class contains the information for a single drive.

  public:
    DriveInfo (wchar_t driveLetter)
      : isVolInfoValid(false),
        netMap(nullptr),
        serialNumber(0),
        maxComponentLength(0),
        fileSysFlags(0),
        driveType(DRIVE_UNKNOWN)
    {
        wcscpy_s (drive, sizeof drive, L"_:\\");
        drive[0]        = driveLetter;

        wcscpy_s (driveNoSlash, sizeof driveNoSlash, L"_:");
        driveNoSlash[0] = driveLetter;

        volumeLabel[0] = 0;
        volumeName[0]  = 0;
        fileSysName[0] = 0;
    }

    ~DriveInfo() { }

    void LoadVolumeInformation ()
    {
        driveType = GetDriveType (drive);

        isVolInfoValid = (0 != GetVolumeInformation (
            drive, volumeLabel, sizeof volumeLabel, &serialNumber, &maxComponentLength, &fileSysFlags,
            fileSysName, sizeof fileSysName));
        
        if (!isVolInfoValid)
        {
            volumeLabel[0]     = 0;
            serialNumber       = 0;
            maxComponentLength = 0;
            fileSysFlags       = 0;
            fileSysName[0]     = 0;
        }

        if (0 == GetVolumeNameForVolumeMountPoint (drive, volumeName, sizeof volumeName))
            volumeName[0] = 0;

        // Get the network-mapped connection, if any.

        DWORD netMapBufferSize = MAX_PATH + 1;
        netMap = new wchar_t[1 + netMapBufferSize];
        auto retry = false;    // True iff this is a retry.

        do {
            switch (WNetGetConnectionW (driveNoSlash, netMap, &netMapBufferSize))
            {
                case NO_ERROR:
                    break;

                case ERROR_MORE_DATA:
                    delete[] netMap;
                    netMap = retry ? nullptr : new wchar_t[netMapBufferSize];
                    retry = !retry;
                    break;

                default:
                case ERROR_BAD_DEVICE:
                case ERROR_NOT_CONNECTED:
                case ERROR_CONNECTION_UNAVAIL:
                case ERROR_NO_NETWORK:
                case ERROR_EXTENDED_ERROR:
                case ERROR_NO_NET_OR_BAD_PATH:
                    delete[] netMap;
                    netMap = nullptr;
            }

        } while (retry);
    }

    void PrintVolumeInformation ()
    {
        wprintf (L"%c:  \"%s\"  %04x-%04x  %s  [%s] --> %s\n",
            drive[0],
            volumeLabel[0] ? volumeLabel : L"",
            serialNumber >> 16, serialNumber & 0xffff,
            DriveDesc(driveType),
            fileSysName,
            netMap
        );

        /*
        wprintf(L"// Drive \"%s\" / \"%s\":\n"
                L"    infoValid: %s,\n"
                L"    label: \"%s\",\n"
                L"    name: \"%s\",\n"
                L"    fileSysName: \"%s\",\n"
                L"    netMap: \"%s\",\n"
                L"    serialNumber: %016x,\n"
                L"    maxComponentLength: %d,\n"
                L"    fileSysFlags: %016x,\n"
                L"    driveType: %016x\n",
            drive, driveNoSlash, isVolInfoValid ? L"true" : L"false", volumeLabel, volumeName, fileSysName,
            netMap ? netMap : L"<null>", serialNumber, maxComponentLength, fileSysFlags, driveType);
        */
    }

  private:
    wchar_t  drive[1 + sizeof L"X:\\"];        // Drive string with trailing slash (for example, 'X:\').
    wchar_t  driveNoSlash[1 + sizeof L"X:"];   // Drive string with no trailing slash ('X:').
    bool     isVolInfoValid;                   // True if we got the drive volume information.
    wchar_t  volumeLabel[MAX_PATH + 1];        // Drive label
    wchar_t  volumeName[MAX_PATH + 1];         // Unique volume name
    wchar_t  fileSysName[MAX_PATH + 1];        // Name of volume file system
    wchar_t *netMap;                           // If applicable, the network map associated with the drive
    DWORD    serialNumber;                     // Volume serial number
    DWORD    maxComponentLength;               // Maximum length for volume path components
    DWORD    fileSysFlags;                     // Flags for volume file system
    UINT     driveType;                        // Type of drive volume
};


static void print (wchar_t *string)
{
    // Simple string print helper function.
    fputws (string, stdout);
}


int main (int, char* [])
{
    //==============================================================================================
    // Main Program Entry Point
    //==============================================================================================

    DWORD logicalDrives = GetLogicalDrives();

    wchar_t drive[]  = L"A:\\";
    wchar_t volumeLabel[MAX_PATH + 1];
    wchar_t fileSysName [256];

    DWORD netmapSize = 256;
    auto  netmap = new wchar_t[netmapSize];

    // Iterate through the drives to find the maximum volume label length.

    size_t maxLabelLen = 0;

    for (drive[0]=L'A';  drive[0] <= L'Z';  ++ drive[0])
    {
        DWORD serialNumber = 0;
        DWORD maxComponentLength = 0;
        DWORD fileSysFlags = 0;

        auto retval = GetVolumeInformation (
                          drive, volumeLabel, sizeof(volumeLabel), &serialNumber, &maxComponentLength,
                          &fileSysFlags, fileSysName, sizeof(fileSysName));

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
                wprintf (L"%c:   Drive type is DRIVE_NO_ROOT_DIR, "
                         L"but GetLogicalDrives reports a drive there.\n\n", drive[0]);
            }
            continue;
        }

        DWORD serialNumber = 0;
        DWORD maxComponentLength = 0;
        DWORD fileSysFlags = 0;

        auto retval = GetVolumeInformation (
                          drive, volumeLabel, sizeof(volumeLabel), &serialNumber, &maxComponentLength,
                          &fileSysFlags, fileSysName, sizeof(fileSysName));

        auto isVolInfoValid = (retval != 0);

        // Print drive letter.

        wprintf (L"%c: ", drive[0]);

        // Print volume label.

        size_t labelLen = 0;
        if (isVolInfoValid && volumeLabel[0])
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

        if (isVolInfoValid)
            wprintf (L"  %04x-%04x  ", serialNumber >> 16, serialNumber & 0xffff);
        else
            print (L"             ");

        // Print the unique volume name.

        wchar_t volumeName[MAX_PATH + 1] = L"";

        if (0 == GetVolumeNameForVolumeMountPoint (drive, volumeName, sizeof volumeName))
            if (volumeName[0] != 0)
                wprintf (L"    Unique Volume Name: %s\n", volumeName);

        // Print the drive description.

        wprintf (L"%s  ", DriveDesc(type));

        // Print the file system type.

        if (isVolInfoValid && fileSysName[0])
            wprintf (L"[%s]  ", fileSysName);

        // Print the net connection, if any.

        DWORD netConnRetVal;
        auto  attempts = 0;

        do {
            ++attempts;

            // Create string of the drive with no trailing slash.
            wchar_t driveNoSlash[] = L"*:";
            driveNoSlash[0] = drive[0];

            netConnRetVal = WNetGetConnectionW (driveNoSlash, netmap, &netmapSize);

            if (netConnRetVal == NO_ERROR)
            {   wprintf (L"--> \"%s\"  ", netmap);
            }
            else if (netConnRetVal == ERROR_MORE_DATA)
            {   delete netmap;
                netmap = new wchar_t[netmapSize];
            }

        } while ((netConnRetVal == ERROR_MORE_DATA) && (attempts < 2));

        wprintf (L"\n");

        // Print the volume information.

        #if PrintFileSysInfo
        {
            if (isVolInfoValid)
            {
                wprintf (L"    Max Component Len = %d, FSys Flags = 0x%08x\n\n", maxComponentLength, fileSysFlags);
                ExpandFileSysFlags (L"    ", fileSysFlags);
            }
        }
        #endif

        #if PrintNewDriveInfo
        {
            auto driveInfo = new DriveInfo(drive[0]);
            driveInfo->LoadVolumeInformation();
            driveInfo->PrintVolumeInformation();
            delete driveInfo;
        }
        #endif
    }
}



void ExpandFileSysFlags (const wchar_t *prefix, DWORD flags)
{
    //==============================================================================================
    // Prints detailed information from the file system flags.
    //==============================================================================================

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



wchar_t *DriveDesc (UINT type)
{
    //==============================================================================================
    // Returns the string value for drive type values.
    //==============================================================================================

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
