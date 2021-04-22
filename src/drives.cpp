//==================================================================================================
//
//  drives
//
//  Command-line tool to print out volume information about all drives. See "::helpText" below for
//  usage information.
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

// Program Version (using the semantic versioning scheme)
const auto programVersion = L"drives v3.0.0-alpha | 2021-04-21 | https://github.com/hollasch/drives";

const unsigned short NumPossibleDrives {26}; // Number of Possible Drives
const unsigned short DriveIndexNone {99};    // Drive Index for None/Null/Invalid Drive



bool DriveValid (DWORD logicalDrives, unsigned short driveIndex) {
    // Returns true if the logical drive index is valid.
    return 0 != (logicalDrives & (1 << driveIndex));
}



class DriveInfo {
  private:

    unsigned short driveIndex;        // Logical drive index 0=A, 1=B, ..., 25=Z.
    wstring  drive;                   // Drive string with trailing slash (for example, 'X:\').
    wstring  driveNoSlash;            // Drive string with no trailing slash ('X:').
    wstring  driveDesc;               // Type of drive volume

    // Info from GetVolumeInformation
    bool     isVolInfoValid {false};  // True if we got the drive volume information.
    wstring  volumeLabel;             // Drive label
    DWORD    serialNumber {0};        // Volume serial number
    DWORD    maxComponentLength {0};  // Maximum length for volume path components
    DWORD    fileSysFlags {0};        // Flags for volume file system
    wstring  fileSysName;             // Name of volume file system

    wstring  volumeName;              // Unique volume name
    wstring  netMap;                  // If applicable, the network map associated with the drive
    wstring  subst;                   // Subst redirection


    // This class contains the information for a single drive.

  public:
    DriveInfo (unsigned short _driveIndex /* in [0,26) */)
      : driveIndex {_driveIndex},
        drive {L"_:\\"},
        driveNoSlash {L"_:"}
    {
        drive[0] = driveNoSlash[0] = L'A' + driveIndex;
    }

    ~DriveInfo() {}


    void LoadVolumeInformation (wstring programName, wstring driveSubstitutions[NumPossibleDrives]) {

        // Loads the volume information for this drive. Note that the drive letter was passed in at construction.

        driveDesc = DriveDesc (GetDriveTypeW (drive.c_str()));

        wchar_t labelBuffer   [MAX_PATH + 1];   // Buffer for volume label
        wchar_t fileSysBuffer [MAX_PATH + 1];   // Buffer for file system name

        isVolInfoValid = (0 != GetVolumeInformationW (
            drive.c_str(), labelBuffer, sizeof labelBuffer, &serialNumber, &maxComponentLength, &fileSysFlags,
            fileSysBuffer, sizeof fileSysBuffer));

        if (isVolInfoValid) {
            volumeLabel = labelBuffer;
            fileSysName = fileSysBuffer;
        } else {
            volumeLabel.clear();
            fileSysName.clear();

            serialNumber       = 0;
            maxComponentLength = 0;
            fileSysFlags       = 0;
        }

        wchar_t nameBuffer [MAX_PATH + 1];
        if (GetVolumeNameForVolumeMountPointW (drive.c_str(), nameBuffer, sizeof nameBuffer))
            volumeName = nameBuffer;
        else
            volumeName.clear();

        subst = driveSubstitutions[driveIndex];

        GetNetworkMap();
    }


    static void GetDriveSubstitutions (wstring programName, wstring (&substitutions)[NumPossibleDrives]) {
        // Get information on any substitutions for this drive.

        // Execute the 'subst' command to scrape existing drive substitutions.
        FILE *results = _wpopen (L"subst", L"rt");
        if (!results) {
            wcerr << programName << L": ERROR: 'subst' command failed." << endl;
            return;
        }

        // Parse the output line-by-line to get each substitution.
        wchar_t buffer[4096];

        while (!feof(results)) {
            if (fgetws (buffer, sizeof(buffer), results) == nullptr)
                continue;

            // Scan past the "X:\ => " leader.
            auto ptr = buffer;
            while (*ptr && *ptr != L'>')
                ++ptr;

            if (*++ptr != L' ') continue;
            ++ptr;

            // Trim the trailing end-of-line characters.
            wstring sub = ptr;
            sub.erase (sub.find_last_not_of(L" \n\r\t") + 1);

            // Save off the substitution target.
            unsigned short driveIndex = buffer[0] - L'A';
            substitutions[driveIndex] = sub;
        }

        _pclose(results);
    }


    void GetNetworkMap() {
        // Get the network-mapped connection, if any.

        DWORD netMapBufferSize {MAX_PATH + 1};
        auto netMapBuffer = new wchar_t[1 + netMapBufferSize];
        auto retry = false;    // True iff this is a retry.

        do {
            switch (WNetGetConnectionW (driveNoSlash.c_str(), netMapBuffer, &netMapBufferSize)) {
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


    void GetMaxFieldLengths (size_t &maxLenVolumeLabel, size_t &maxLenDriveDesc) {
        // Computes the maximum field lengths, incorporating the length of this drive's fields.

        maxLenVolumeLabel = max (maxLenVolumeLabel, volumeLabel.length());
        maxLenDriveDesc   = max (maxLenDriveDesc,   driveDesc.length());
    }


    void PrintVolumeInformation (bool verbose, size_t maxLenVolumeLabel, size_t maxLenDriveDesc) {
        // Prints human-readable volume information for this drive.
        //
        // verbose:            True => print additional volume information.
        // maxLenVolumeLabel:  Maximum string length for all volume labels.
        // maxLenDriveDesc:    Maximum string length for all drive type strings.

        wcout << driveNoSlash << L' ';

        // Print the volume label.

        maxLenVolumeLabel += 2;     // Add room for quotes to volume label.
        auto lenVolumeLabel = (volumeLabel[0] == 0) ? 0 : 2 + volumeLabel.length();

        wstring formattedVolumeLabel;

        if (volumeLabel.length())
            formattedVolumeLabel.append(L"\"").append(volumeLabel).append(L"\"");
        else {
            formattedVolumeLabel += L"-";
            lenVolumeLabel = 1;
        }
        formattedVolumeLabel.append(maxLenVolumeLabel - lenVolumeLabel, L' ');

        wcout << formattedVolumeLabel << L" ";

        // Print the volume serial number.

        if (!isVolInfoValid)
            wcout << L" -          ";
        else {
            wcout << hex << L' ';
            wcout << setw(4) << setfill(L'0') << (serialNumber >> 16) << L'-';
            wcout << setw(4) << setfill(L'0') << (serialNumber & 0xffff);
            wcout << L"  " << dec;
        }

        // Drive Type
        auto driveDescPadded = driveDesc;
        driveDescPadded.append (maxLenDriveDesc - driveDesc.length(), L' ');
        wcout << driveDescPadded;

        // File System Type
        if (isVolInfoValid)
            wcout << L" [" << fileSysName << L"] ";
        else
            wcout << L" -      ";

        // Drive substitution, if any.
        if (subst.length())
            wcout << L"--> " << subst;

        // Mapping, if any.
        if (netMap.length())
            wcout << L"--> " << netMap;

        // Print additional information if requested.
        if (verbose && volumeName.length())
            wcout << endl << L"   " << volumeName << endl;

        wcout << endl;
    }


    void PrintParseableVolumeInformation() {
        // Prints the information for this volume in a machine-parseable format.

        wcout << driveNoSlash << L"driveType: \"" << driveDesc << L"\"" << endl;

        if (isVolInfoValid) {
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

        } else {

            wcout << driveNoSlash << L"label: null" << endl;
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

        wcout << driveNoSlash << L"driveSubst: ";
        if (!subst.length())
            wcout << "null" << endl;
        else
            wcout << "\"" << subst << "\"" << endl;

        wcout << driveNoSlash << L"netMap: ";
        if (!netMap.length())
            wcout << "null" << endl;
        else
            wcout << "\"" << netMap << "\"" << endl;
    }


  private:   // Helper Methods

    static wstring DriveDesc (UINT type) {
        // Returns the string value for drive type values.

        switch (type) {
            case DRIVE_NO_ROOT_DIR:  return L"No root";
            case DRIVE_REMOVABLE:    return L"Removable";
            case DRIVE_FIXED:        return L"Fixed";
            case DRIVE_REMOTE:       return L"Remote";
            case DRIVE_CDROM:        return L"CD-ROM";
            case DRIVE_RAMDISK:      return L"RAM Disk";
        }

        return L"Unknown";
    }
};



//======================================================================================================================

class CommandOptions {
    // This class stores and manages all command line options.

  public:
    wstring        programName;               // Name of executable
    bool           printVersion {false};      // True => Print program version
    bool           printHelp {false};         // True => print help information
    bool           printVerbose {false};      // True => Print verbose; include additional information
    bool           printParseable {false};    // True => print results in machine-parseable format
    unsigned short singleDriveIndex {DriveIndexNone};  // Specified single drive, else null

    CommandOptions() {}

    bool parseArguments (int argCount, wchar_t* argTokens[]) {
        // Parse the command line into the individual command options.

        programName = argTokens[0];

        for (int argIndex = 1;  argIndex < argCount;  ++argIndex) {
            auto token = argTokens[argIndex];

            if (token[0] == L'/' && token[1] == L'?' && token[2] == 0) {
                printHelp = true;
                continue;
            }

            if (token[0] != L'-') {
                // Non switches

                // Allowable drive formats: 'X', 'X:.*'.

                const bool driveLetterInRange =  ((L'A' <= token[0]) && (token[0] <= L'Z'))
                                              || ((L'a' <= token[0]) && (token[0] <= L'z'));
                const bool driveStringTailValid = (token[1] == 0) || (token[1] == L':');

                if (driveLetterInRange && driveStringTailValid) {
                    singleDriveIndex = towupper(token[0]) - L'A';
                } else {
                    wcerr << programName << L": ERROR: Unexpected argument (" << token << ").\n";
                    return false;
                }

            } else if (0 == wcsncmp(token, L"--", wcslen(L"--"))) {

                wstring tokenString {token};

                // Double-dash switches

                if (tokenString == L"--help")
                    printHelp = true;
                else if (tokenString == L"--parseable")
                    printParseable = true;
                else if (tokenString == L"--verbose")
                    printVerbose = true;
                else if (tokenString == L"--version")
                    printVersion = true;
                else {
                    wcerr << programName << L": ERROR: Unrecognized option (" << token << L").\n";
                    return false;
                }

            } else {

                // Single letter switches

                if (!token[1]) {
                    wcerr << programName << L": ERROR: Missing option letter for '" << token[0] << L"'.\n";
                    return false;
                }
                ++token;

                do switch(*token) {
                    case L'h': case L'H': case L'?':
                        printHelp = true;
                        break;

                    case L'p': case L'P':
                        printParseable = true;
                        break;

                    case L'v': case L'V':
                        printVerbose = true;
                        break;

                    default:
                        wcerr << programName << L": ERROR: Unrecognized option (" << *token << L").\n";
                        return false;

                } while (*++token);
            }
        }

        printVersion = printVersion || printHelp;

        return true;
    }
};


wchar_t* helpText = LR"(
drives: Print Windows drive and volume information
Usage:  drives [/?|-h|--help] [--version] [-v|--verbose] [-p|--parseable] [drive]

This program also prints all network mappings and drive substitutions (see the
'subst' command).

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

)";



//======================================================================================================================

int wmain (int argc, wchar_t* argv[]) {

    // Parse command line options.
    CommandOptions commandOptions;

    if (!commandOptions.parseArguments(argc, argv))
        exit(1);

    if (commandOptions.printVersion) {
        if (!commandOptions.printHelp) {
            wcout << programVersion << L'\n';
        } else {
            wcout << helpText << programVersion << L'\n';
        }

        exit(0);
    }

    auto logicalDrives = GetLogicalDrives();        // Query system logical drives.
    DriveInfo* driveInfo [NumPossibleDrives];       // Create drive info for each possible drive.
    wstring driveSubstitutions[NumPossibleDrives];  // Drive Substituttions

    DriveInfo::GetDriveSubstitutions (commandOptions.programName, driveSubstitutions);

    unsigned short minDriveIndex {0};
    unsigned short maxDriveIndex {NumPossibleDrives - 1};

    // Handle single-drive reporting.

    if (commandOptions.singleDriveIndex != DriveIndexNone) {
        if (!DriveValid(logicalDrives, commandOptions.singleDriveIndex)) {
            wchar_t driveLetter = L'A' + commandOptions.singleDriveIndex;
            wcout << commandOptions.programName << L": No volume present at drive " << driveLetter << L":." << endl;
            exit(1);
        }
        minDriveIndex = maxDriveIndex = commandOptions.singleDriveIndex;
    }

    // Query all drives for volume information, and get maximum field lengths.

    unsigned short driveIndex;    // Drive numerical index, [0,26).

    size_t maxLenVolumeLabel {0};
    size_t maxLenDriveDesc {0};

    for (driveIndex = minDriveIndex;  driveIndex <= maxDriveIndex;  ++driveIndex) {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        driveInfo[driveIndex] = new DriveInfo(driveIndex);
        driveInfo[driveIndex]->LoadVolumeInformation (commandOptions.programName, driveSubstitutions);
        driveInfo[driveIndex]->GetMaxFieldLengths(maxLenVolumeLabel, maxLenDriveDesc);
    }

    // For each drive, print volume information.

    for (driveIndex = minDriveIndex;  driveIndex <= maxDriveIndex;  ++driveIndex) {
        if (!DriveValid(logicalDrives, driveIndex))
            continue;

        if (commandOptions.printParseable)
            driveInfo[driveIndex]->PrintParseableVolumeInformation();
        else
            driveInfo[driveIndex]->
                PrintVolumeInformation(commandOptions.printVerbose, maxLenVolumeLabel, maxLenDriveDesc);

        delete driveInfo[driveIndex];
    }
}
