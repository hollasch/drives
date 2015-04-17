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
#include <iostream>
#include <iomanip>

using namespace std;

// Program Parameters

#define PrintFileSysInfo  0


const int NumPossibleDrives = 26;    // Number of Possible Drives



bool DriveValid (DWORD logicalDrives, unsigned short driveIndex)
{
    return 0 != (logicalDrives & (1 << driveIndex));
}



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

    void GetMaxFieldLengths (size_t &maxLenVolumeLabel)
    {
        maxLenVolumeLabel = max (maxLenVolumeLabel, wcslen(volumeLabel));
    }

    void PrintVolumeInformation (size_t maxLenVolumeLabel)
    {
        wcout << drive[0] << L": ";

        // Print the volume label.

        maxLenVolumeLabel += 2;     // Add room for quotes to volume label.
        auto lenVolumeLabel = (volumeLabel[0] == 0) ? 0 : 2 + wcslen(volumeLabel);

        wstring formattedVolumeLabel;

        if (volumeLabel[0])
            formattedVolumeLabel.append(L"\"").append(volumeLabel).append(L"\"");
        else
        {
            formattedVolumeLabel += L"-";
            lenVolumeLabel = 1;
        }
        formattedVolumeLabel.append(maxLenVolumeLabel - lenVolumeLabel, L' ');

        wcout << formattedVolumeLabel << L" ";

        // Print the volume serial number.

        if (!isVolInfoValid)
            wcout << L" -          ";
        else
        {
            wcout << hex << L' ';
            wcout << setw(4) << setfill(L'0') << (serialNumber >> 16) << L'-';
            wcout << setw(4) << setfill(L'0') << (serialNumber & 0xffff);
            wcout << L"  " << dec;
        }

        // Drive Type
        wcout << DriveDesc(driveType);

        // File System Type
        if (isVolInfoValid)
            wcout << L" [" << fileSysName << L"]  ";
        else
            wcout << L" -       ";

        // Mapping, if any.
        if (netMap)
            wcout << L"--> " << netMap;

        wcout << endl;

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


    static wchar_t *DriveDesc (UINT type)
    {
        // Returns the string value for drive type values.

        switch (type)
        {
            case DRIVE_NO_ROOT_DIR:  return L"No root  ";
            case DRIVE_REMOVABLE:    return L"Removable";
            case DRIVE_FIXED:        return L"Fixed    ";
            case DRIVE_REMOTE:       return L"Remote   ";
            case DRIVE_CDROM:        return L"CD-ROM   ";
            case DRIVE_RAMDISK:      return L"RAM Disk ";
        }

        return L"Unknown  ";
    }


    #if PrintFileSysInfo
        static void ExpandFileSysFlags (const wchar_t *prefix, DWORD flags)
        {
            // Prints detailed information from the file system flags.

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
    #endif
};



//======================================================================================================================

class CommandOptions
{
    // This class stores and manages all command line options.

  public:
    wchar_t*  programName;  // Name of executable
    bool      printHelp;    // True => print help information
    wchar_t** drive;        // Specified single drive, else null

    CommandOptions()
      : printHelp(false),
        drive(nullptr)
    {
    }


    bool parseArguments (int argCount, wchar_t* argTokens[])
    {
        // Parse the command line into the individual command options.

        programName = argTokens[0];

        for (int argIndex = 1;  argIndex < argCount;  ++argIndex)
        {
            wchar_t* token = argTokens[argIndex];

            if ((token[0] != L'/') && (token[0] != L'-'))
            {
                // Non switches

                wcerr << programName << L": ERROR: Unexpected argument (" << token << ").\n";
                return false;
            }
            else if (0 == wcsncmp(token, L"--", wcslen(L"--")))
            {
                wstring tokenString = token;
                
                // Double-dash switches

                if (tokenString == L"--help")
                {
                    printHelp = true;
                }
                else
                {
                    wcerr << programName << L": ERROR: Unrecognized option (" << token << L").\n";
                    return false;
                }
            }
            else
            {
                // Single letter switches

                if (!token[1])
                {
                    wcerr << programName << L": ERROR: Missing option letter for '" << token[0] << L"'.\n";
                    return false;
                }
                ++token;

                do switch(*token)
                {
                    case L'h': case L'H': case L'?':
                        printHelp = true;
                        break;

                    default:
                        wcerr << programName << L": ERROR: Unrecognized option (" << *token << L").\n";
                        return false;

                } while (*++token);
            }
        }

        return true;
    }
};



//======================================================================================================================

static wchar_t helpText[] =
L"\n"
L"drives: Print drive and volume information.\n"
L"Usage:  drives [/?|-h|--help]\n"
L"\n"
L"Single letter options may use either dashes (-) or slashes (/) as option\n"
L"prefixes, and are case insensitive.\n"
L"\n"
L"-h      Print out help information\n"
L"--help\n"
;



//======================================================================================================================

int wmain (int argc, wchar_t* argv[])
{
    // Main Program Entry Point

    CommandOptions commandOptions;

    if (!commandOptions.parseArguments(argc, argv))
        exit(1);

    if (commandOptions.printHelp)
    {
        cout << helpText;
        exit(0);
    }

    DWORD logicalDrives = GetLogicalDrives();

    DriveInfo* driveInfo [NumPossibleDrives];

    // Query all drives for volume information, and get maximum field lengths.
    unsigned short driveIndex;
    wchar_t        driveLetter;

    size_t maxLenVolumeLabel = 0;

    for (driveIndex = 0, driveLetter = L'A';  driveIndex < NumPossibleDrives;  ++driveIndex, ++driveLetter)
    {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        driveInfo[driveIndex] = new DriveInfo(driveLetter);
        driveInfo[driveIndex]->LoadVolumeInformation();
        driveInfo[driveIndex]->GetMaxFieldLengths(maxLenVolumeLabel);
    }

    for (driveIndex = 0, driveLetter = L'A';  driveIndex < NumPossibleDrives;  ++driveIndex, ++driveLetter)
    {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        driveInfo[driveIndex]->PrintVolumeInformation(maxLenVolumeLabel);

        delete driveInfo[driveIndex];
    }
}
