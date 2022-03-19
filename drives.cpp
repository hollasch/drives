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
#include <vector>

using namespace std;

// Program Version (using the semantic versioning scheme)
const auto programVersion = L"drives 3.0.0-alpha.1 | 2021-04-21 | https://github.com/hollasch/drives";

const unsigned short NumPossibleDrives {26}; // Number of Possible Drives


//======================================================================================================================

class CommandOptions {
    // This class stores and manages all command line options.

  public:
    wstring        programName;           // Name of executable
    bool           printVersion {false};  // True => Print program version
    bool           printHelp {false};     // True => print help information
    bool           printVerbose {false};  // True => Print verbose; include additional information
    bool           printJSON {false};     // True => print results in JSON format
    wchar_t        singleDrive {0};       // Specified single drive ('A'-'Z'), else 0

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
                const bool driveStringTailValid = (token[1] == 0 || token[1] == L':');

                if (driveLetterInRange && driveStringTailValid) {
                    singleDrive = towupper(token[0]);
                } else {
                    wcerr << programName << L": ERROR: Unexpected argument (" << token << ").\n";
                    return false;
                }

            } else if (0 == wcsncmp(token, L"--", wcslen(L"--"))) {

                wstring tokenString {token};

                // Double-dash switches

                if (tokenString == L"--help")
                    printHelp = true;
                else if (tokenString == L"--json")
                    printJSON = true;
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

                    case L'j': case L'J':
                        printJSON = true;
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

//======================================================================================================================

bool DriveValid (DWORD logicalDrives, wchar_t driveLetter) {
    // Returns true if the drive letter is valid.
    return 0 != (logicalDrives & (1 << (driveLetter - L'A')));
}

//======================================================================================================================

wstring escaped(const wstring& source) {
    // Return the source string with backslashes escaped ("\" -> "\\")
    wstring result;
    for (auto c : source) {
        if (c == '\\')
            result += '\\';
        result += c;
    }
    return result;
}

//======================================================================================================================

class DriveInfo {
  private:

    int      driveIndex;        // Logical drive index 0=A, 1=B, ..., 25=Z.
    wstring  drive;             // Drive string with trailing slash (for example, 'X:\').
    wstring  driveNoSlash;      // Drive string with no trailing slash ('X:').
    wstring  driveDesc;         // Type of drive volume

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
    DriveInfo (wchar_t _driveLetter /* in [L'A', L'Z'] */)
      : driveIndex {_driveLetter - L'A'},
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

    void GetMaxFieldLengths (size_t &maxLenVolumeLabel, size_t &maxLenDriveDesc) const {
        // Computes the maximum field lengths, incorporating the length of this drive's fields.

        maxLenVolumeLabel = max (maxLenVolumeLabel, volumeLabel.length());
        maxLenDriveDesc   = max (maxLenDriveDesc,   driveDesc.length());
    }

    void PrintVolumeInformation (
        const CommandOptions& options, size_t maxLenVolumeLabel, size_t maxLenDriveDesc
    ) const {
        // Prints human-readable volume information for this drive.
        //
        // options:            Program options
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
        if (options.printVerbose && volumeName.length())
            wcout << L"\n   " << volumeName << "\n";

        wcout << endl;
    }

    void PrintJSONVolumeInformation(bool first) const {
        // Prints volume information in JSON format.

        if (!first)
            wcout << ",\n";

        wcout << "  {\n";

        wcout << L"    \"driveType\": \"" << driveDesc << L"\",\n";

        if (isVolInfoValid) {
            wcout << L"    \"label\": \"" << escaped(volumeLabel) << L"\",\n";

            wcout << L"    \"serialNumber\": \"" << hex;
            wcout << setw(4) << setfill(L'0') << (serialNumber >> 16) << L'-';
            wcout << setw(4) << setfill(L'0') << (serialNumber & 0xffff);
            wcout << dec << L"\",\n";

            wcout << L"    \"maxComponentLength\": " << maxComponentLength << ",\n";
            wcout << L"    \"fileSystem\": \"" << fileSysName << "\",\n";
            wcout << L"    \"fileSysFlagsValue\": \"0x"
                <<hex <<setw(8) <<setfill(L'0') << fileSysFlags <<dec <<"\",\n";

            wcout << L"    \"fileSysFlags\": {\n";
            bool first = true;
            auto flagPrint = [&first](const DriveInfo* info, wstring desc, DWORD flag) {
                if (!first) wcout << ",\n";
                wcout << L"      \"" << desc << "\": " << ((info->fileSysFlags & flag) != 0);
                first = false;
            };

            flagPrint (this, L"flagFileNamedStreams",          FILE_NAMED_STREAMS);
            flagPrint (this, L"flagFileSupportsObjectIDs",     FILE_SUPPORTS_OBJECT_IDS);
            flagPrint (this, L"flagFileSupportsReparsePoints", FILE_SUPPORTS_REPARSE_POINTS);
            flagPrint (this, L"flagFileSupportsSparseFiles",   FILE_SUPPORTS_SPARSE_FILES);
            flagPrint (this, L"flagFileVolumeQuotas",          FILE_VOLUME_QUOTAS);
            flagPrint (this, L"flagFSCaseSensitive",           FS_CASE_SENSITIVE);
            flagPrint (this, L"flagFSFileCompression",         FS_FILE_COMPRESSION);
            flagPrint (this, L"flagFSFileEncryption",          FS_FILE_ENCRYPTION);
            flagPrint (this, L"flagFSUnicodeStoredOnDisk",     FS_UNICODE_STORED_ON_DISK);
            flagPrint (this, L"flagFSCaseIsPreserved",         FS_CASE_IS_PRESERVED);
            flagPrint (this, L"flagFSPersistentACLs",          FS_PERSISTENT_ACLS);
            flagPrint (this, L"flagFSVolIsCompressed",         FS_VOL_IS_COMPRESSED);

            wcout << L"\n    },\n";

        } else {

            wcout << driveNoSlash << L"    \"label\": null,\n";
            wcout << driveNoSlash << L"    \"serialNumber\": null,\n";
            wcout << driveNoSlash << L"    \"maxComponentLength\": null,\n";
            wcout << driveNoSlash << L"    \"fileSystem\": null,\n";
            wcout << driveNoSlash << L"    \"fileSysFlags\": null,\n";
        }

        wcout << L"    \"name\": \"" << escaped(volumeName) << "\",\n";

        wcout << L"    \"driveSubst\": ";
        if (!subst.length())
            wcout << "null,\n";
        else
            wcout << "\"" << escaped(subst) << "\",\n";

        wcout << L"    \"netMap\": ";
        if (!netMap.length())
            wcout << "null\n";
        else
            wcout << "\"" << escaped(netMap) << "\"\n";

        wcout << "  }";
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

void GetDriveSubstitutions (wstring programName, wstring (&substitutions)[NumPossibleDrives]) {
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

//======================================================================================================================

void PrintResultsHuman(const CommandOptions& options, vector<DriveInfo>& drives, int logicalDrives) {
    size_t maxLenVolumeLabel {0};
    size_t maxLenDriveDesc {0};

    for (const auto& drive : drives)
        drive.GetMaxFieldLengths(maxLenVolumeLabel, maxLenDriveDesc);

    for (const auto& drive : drives)
        drive.PrintVolumeInformation(options, maxLenVolumeLabel, maxLenDriveDesc);
}

//======================================================================================================================

void PrintResultsJSON(const CommandOptions& options, vector<DriveInfo>& drives, int logicalDrives) {
    wcout << "[\n";

    bool first = true;
    for (const auto& drive : drives) {
        drive.PrintJSONVolumeInformation(first);
        first = false;
    }

    wcout << "\n]" << endl;
}

//======================================================================================================================

wchar_t* helpText = LR"(
drives: Print Windows drive and volume information
usage : drives  [--json|-j] [--verbose|-v] [drive]
                [--help|-h|/?] [--version]

This program also prints all network mappings and drive substitutions (see the
'subst' command).

Options
    [drive]
        Optional drive letter for specific drive report (colon optional).

    --help, -h, /?
        Print help information.

    --json, -j
        Print results in JSON format.

    --verbose, -v
        Print full drive information for the human-readable format. This has no
        effect if --json is supplied.

    --version
        Print program version.

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

    int logicalDrives = GetLogicalDrives();         // Query system logical drives.
    vector<DriveInfo> drives;
    wstring driveSubstitutions[NumPossibleDrives];  // Drive Substituttions

    GetDriveSubstitutions (commandOptions.programName, driveSubstitutions);

    // Handle single-drive reporting.

    wchar_t minDrive{L'A'}, maxDrive{L'Z'};

    if (commandOptions.singleDrive) {
        if (!DriveValid(logicalDrives, commandOptions.singleDrive)) {
            wchar_t driveLetter = commandOptions.singleDrive;
            wcout << commandOptions.programName << L": No volume present at drive " << driveLetter << L":." << endl;
            exit(1);
        }
        minDrive = maxDrive = commandOptions.singleDrive;
    }

    // Query all drives for volume information, and get maximum field lengths.

    for (auto driveLetter = minDrive;  driveLetter <= maxDrive;  ++driveLetter) {
        if (!DriveValid(logicalDrives, driveLetter))
            continue;

        auto driveInfo = new DriveInfo{driveLetter};
        driveInfo->LoadVolumeInformation(commandOptions.programName, driveSubstitutions);
        drives.push_back(*driveInfo);
    }

    // For each drive, print volume information.

    if (commandOptions.printJSON)
        PrintResultsJSON(commandOptions, drives, logicalDrives);
    else
        PrintResultsHuman(commandOptions, drives, logicalDrives);
}
