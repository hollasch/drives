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
      : drive(L"_:\\"),
        driveNoSlash(L"_:"),
        isVolInfoValid(false),
        serialNumber(0),
        maxComponentLength(0),
        fileSysFlags(0),
        driveType(DRIVE_UNKNOWN)
    {
        drive[0]        = driveLetter;
        driveNoSlash[0] = driveLetter;
    }

    ~DriveInfo() { }


    void LoadVolumeInformation ()
    {
        // Loads the volume information for this drive. Note that the drive letter was passed in at construction.

        driveType = GetDriveType (drive.c_str());

        wchar_t labelBuffer   [MAX_PATH + 1];   // Buffer for volume label
        wchar_t fileSysBuffer [MAX_PATH + 1];   // Buffer for file system name

        isVolInfoValid = (0 != GetVolumeInformation (
            drive.c_str(), labelBuffer, sizeof labelBuffer, &serialNumber, &maxComponentLength, &fileSysFlags,
            fileSysBuffer, sizeof fileSysBuffer));
        
        if (isVolInfoValid)
        {
            volumeLabel = labelBuffer;
            fileSysName = fileSysBuffer;
        }
        else
        {
            volumeLabel.clear();
            fileSysName.clear();

            serialNumber       = 0;
            maxComponentLength = 0;
            fileSysFlags       = 0;
        }
        
        wchar_t nameBuffer [MAX_PATH + 1];
        if (GetVolumeNameForVolumeMountPoint (drive.c_str(), nameBuffer, sizeof nameBuffer))
            volumeName = nameBuffer;
        else
            volumeName.clear();

        // Get the network-mapped connection, if any.

        DWORD netMapBufferSize = MAX_PATH + 1;
        auto netMapBuffer = new wchar_t[1 + netMapBufferSize];
        auto retry = false;    // True iff this is a retry.

        do {
            switch (WNetGetConnectionW (driveNoSlash.c_str(), netMapBuffer, &netMapBufferSize))
            {
                case NO_ERROR:
                    break;

                case ERROR_MORE_DATA:
                    delete[] netMapBuffer;
                    netMapBuffer = retry ? nullptr : new wchar_t[netMapBufferSize];
                    retry = !retry;
                    break;

                default:
                case ERROR_BAD_DEVICE:
                case ERROR_NOT_CONNECTED:
                case ERROR_CONNECTION_UNAVAIL:
                case ERROR_NO_NETWORK:
                case ERROR_EXTENDED_ERROR:
                case ERROR_NO_NET_OR_BAD_PATH:
                    netMapBuffer[0] = 0;
            }

        } while (retry);

        if (netMapBuffer && netMapBuffer[0])
            netMap = netMapBuffer;
        else
            netMap.clear();

        delete[] netMapBuffer;
    }


    void GetMaxFieldLengths (size_t &maxLenVolumeLabel, size_t &maxLenDriveDesc)
    {
        // Computes the maximum field lengths, incorporating the length of this drive's fields.

        maxLenVolumeLabel = max (maxLenVolumeLabel, volumeLabel.length());
        maxLenDriveDesc   = max (maxLenDriveDesc, DriveDesc(driveType).length());
    }


    void PrintVolumeInformation (size_t maxLenVolumeLabel, size_t maxLenDriveDesc)
    {
        // Prints human-readable volume information for this drive.
        //
        // maxLenVolumeLabel:  Maximum string length for all volume labels.


        wcout << driveNoSlash << L' ';

        // Print the volume label.

        maxLenVolumeLabel += 2;     // Add room for quotes to volume label.
        auto lenVolumeLabel = (volumeLabel[0] == 0) ? 0 : 2 + volumeLabel.length();

        wstring formattedVolumeLabel;

        if (volumeLabel.length())
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
        wstring driveDesc = DriveDesc(driveType);
        driveDesc.append (maxLenDriveDesc - driveDesc.length(), L' ');
        wcout << driveDesc;

        // File System Type
        if (isVolInfoValid)
            wcout << L" [" << fileSysName << L"] ";
        else
            wcout << L" -      ";

        // Mapping, if any.
        if (netMap.length())
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


    void PrintParseableVolumeInformation()
    {
        // Prints the information for this volume in a machine-parseable format.

        wcout << driveNoSlash << L"driveType: \"" << DriveDesc(driveType) << L"\"" << endl;

        if (isVolInfoValid)
        {
            wcout << driveNoSlash << L"label: \"" << volumeLabel << L"\"" << endl;

            wcout << driveNoSlash << L"serialNumber: \"" << hex;
            wcout << setw(4) << setfill(L'0') << (serialNumber >> 16) << L'-';
            wcout << setw(4) << setfill(L'0') << (serialNumber & 0xffff);
            wcout << dec << L"\"" << endl;

            wcout << driveNoSlash << L"maxComponentLength: " << maxComponentLength << endl;
            wcout << driveNoSlash << L"fileSystem: \"" << fileSysName << "\"" << endl;
            wcout << driveNoSlash << L"fileSysFlags: " <<hex <<setw(8) <<setfill(L'0') << fileSysFlags <<dec << endl;

            auto flagPrint = [](DriveInfo* info, wstring desc, DWORD flag) {
                wcout << info->driveNoSlash << desc << L": " << ((info->fileSysFlags & flag) != 0) << endl;
            };

            flagPrint (this, L"flagFileNamedStreams", FILE_NAMED_STREAMS);
            flagPrint (this, L"flagFileSupportsObjectIDs", FILE_SUPPORTS_OBJECT_IDS);
            flagPrint (this, L"flagFileSupportsReparsePoints", FILE_SUPPORTS_REPARSE_POINTS);
            flagPrint (this, L"flagFileSupportsSparseFiles", FILE_SUPPORTS_SPARSE_FILES);
            flagPrint (this, L"flagFileVolumeQuotas", FILE_VOLUME_QUOTAS);
            flagPrint (this, L"flagFSCaseSensitive", FS_CASE_SENSITIVE);
            flagPrint (this, L"flagFSFileCompression", FS_FILE_COMPRESSION);
            flagPrint (this, L"flagFSFileEncryption", FS_FILE_ENCRYPTION);
            flagPrint (this, L"flagFSUnicodeStoredOnDisk", FS_UNICODE_STORED_ON_DISK);
            flagPrint (this, L"flagFSCaseIsPreserved", FS_CASE_IS_PRESERVED);
            flagPrint (this, L"flagFSPersistentACLs", FS_PERSISTENT_ACLS);
            flagPrint (this, L"flagFSVolIsCompressed", FS_VOL_IS_COMPRESSED);
        }
        else
        {   wcout << driveNoSlash << L"label: null" << endl;
            wcout << driveNoSlash << L"serialNumber: null" << endl;
            wcout << driveNoSlash << L"maxComponentLength: null" << endl;
            wcout << driveNoSlash << L"fileSystem: null" << endl;
            wcout << driveNoSlash << L"fileSysFlags: null" << endl;
            wcout << driveNoSlash << L"flagFileNamedStreams: null" << endl;
            wcout << driveNoSlash << L"flagFileSupportsObjectIDs: null" << endl;
            wcout << driveNoSlash << L"flagFileSupportsReparsePoints: null" << endl;
            wcout << driveNoSlash << L"flagFileSupportsSparseFiles: null" << endl;
            wcout << driveNoSlash << L"flagFileVolumeQuotas: null" << endl;
            wcout << driveNoSlash << L"flagFSCaseSensitive: null" << endl;
            wcout << driveNoSlash << L"flagFSFileCompression: null" << endl;
            wcout << driveNoSlash << L"flagFSFileEncryption: null" << endl;
            wcout << driveNoSlash << L"flagFSUnicodeStoredOnDisk: null" << endl;
            wcout << driveNoSlash << L"flagFSCaseIsPreserved: null" << endl;
            wcout << driveNoSlash << L"flagFSPersistentACLs: null" << endl;
            wcout << driveNoSlash << L"flagFSVolIsCompressed: null" << endl;
        }

        wcout << driveNoSlash << L"name: \"" << volumeName << "\"" << endl;

        wcout << driveNoSlash << L"netMap: ";
        if (netMap.length())
            wcout << "\"" << netMap << "\"" << endl;
        else
            wcout << "null" << endl;

        wcout << driveNoSlash << L"subst: ";
        if (subst.length())
            wcout << "\"" << subst << "\"" << endl;
        else
            wcout << "null" << endl;
    }


  private:

    wstring  drive;                 // Drive string with trailing slash (for example, 'X:\').
    wstring  driveNoSlash;          // Drive string with no trailing slash ('X:').
    UINT     driveType;             // Type of drive volume

    // Info from GetVolumeInformation
    bool     isVolInfoValid;        // True if we got the drive volume information.
    wstring  volumeLabel;           // Drive label
    DWORD    serialNumber;          // Volume serial number
    DWORD    maxComponentLength;    // Maximum length for volume path components
    DWORD    fileSysFlags;          // Flags for volume file system
    wstring  fileSysName;           // Name of volume file system

    wstring  volumeName;            // Unique volume name
    wstring  netMap;                // If applicable, the network map associated with the drive
    wstring  subst;                 // Subst redirection


    static wstring DriveDesc (UINT type)
    {
        // Returns the string value for drive type values.

        switch (type)
        {
            case DRIVE_NO_ROOT_DIR:  return L"No root";
            case DRIVE_REMOVABLE:    return L"Removable";
            case DRIVE_FIXED:        return L"Fixed";
            case DRIVE_REMOTE:       return L"Remote";
            case DRIVE_CDROM:        return L"CD-ROM";
            case DRIVE_RAMDISK:      return L"RAM Disk";
        }

        return L"Unknown";
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
    wstring programName;     // Name of executable
    bool    printHelp;       // True => print help information
    bool    printParseable;  // True => print results in machine-parseable format
    wstring drive;           // Specified single drive, else null

    static wchar_t* helpText;

    CommandOptions()
      : printHelp(false),
        printParseable(false)
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
                    printHelp = true;
                else if (tokenString == L"--parseable")
                    printParseable = true;
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

                    case L'p': case L'P':
                        printParseable = true;
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


wchar_t* CommandOptions::helpText =
L"\n"
L"drives: Print drive and volume information.\n"
L"Usage:  drives [/?|-h|--help] [-p|--parseable]\n"
L"\n"
L"Single letter options may use either dashes (-) or slashes (/) as option\n"
L"prefixes, and are case insensitive.\n"
L"\n"
L"-h            Print out help information.\n"
L"--help\n"
L"\n"
L"-p            Print results in machine-parseable format.\n"
L"--parseable\n"
L"\n"
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
        wcout << CommandOptions::helpText;
        exit(0);
    }

    DWORD logicalDrives = GetLogicalDrives();

    DriveInfo* driveInfo [NumPossibleDrives];

    // Query all drives for volume information, and get maximum field lengths.
    unsigned short driveIndex;
    wchar_t        driveLetter;

    size_t maxLenVolumeLabel = 0;
    size_t maxLenDriveDesc = 0;

    for (driveIndex = 0, driveLetter = L'A';  driveIndex < NumPossibleDrives;  ++driveIndex, ++driveLetter)
    {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        driveInfo[driveIndex] = new DriveInfo(driveLetter);
        driveInfo[driveIndex]->LoadVolumeInformation();
        driveInfo[driveIndex]->GetMaxFieldLengths(maxLenVolumeLabel, maxLenDriveDesc);
    }

    for (driveIndex = 0, driveLetter = L'A';  driveIndex < NumPossibleDrives;  ++driveIndex, ++driveLetter)
    {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        if (commandOptions.printParseable)
            driveInfo[driveIndex]->PrintParseableVolumeInformation();
        else
            driveInfo[driveIndex]->PrintVolumeInformation(maxLenVolumeLabel, maxLenDriveDesc);

        delete driveInfo[driveIndex];
    }
}
