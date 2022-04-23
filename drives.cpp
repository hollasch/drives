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
const auto programVersion = L"drives 3.0.0-alpha.4 | 2022-04-22 | https://github.com/hollasch/drives";


//======================================================================================================================

class CommandOptions {
    // This class stores and manages all command line options.

  public:
    wstring programName;           // Name of executable
    bool    printVersion {false};  // True => Print program version
    bool    printHelp {false};     // True => print help information
    bool    printVerbose {false};  // True => Print verbose; include additional information
    bool    printJSON {false};     // True => print results in JSON format
    wchar_t singleDrive {0};       // Specified single drive ('A'-'Z'), else 0

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

bool DriveValid (wchar_t driveLetter) {
    static int logicalDrives = GetLogicalDrives();  // Query system logical drives.

    // Returns true if the drive letter is valid.
    return 0 != (logicalDrives & (1 << (driveLetter - L'A')));
}

//======================================================================================================================

wstring Escape(const wstring& source) {
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

wstring DriveType (UINT type) {
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

//======================================================================================================================

wstring DriveSubstitution(wchar_t driveLetter) {
    // Returns the substitution for the given DOS drive. For example, by using the `subst` command.

    WCHAR drive[] = L"_:";
    const DWORD bufferSize = 4096;
    WCHAR outBuffer[bufferSize];

    drive[0] = driveLetter;

    auto numChars = QueryDosDeviceW(drive, outBuffer, bufferSize);

    // Substituted drives have a device name beginning with "\??\", followed by the full drive path.
    // For example, if X: is a substitute for A:\users\yoda, then the device path would be
    // "\??\A:\users\yoda".
    if (numChars > 4 && outBuffer[0] == '\\' && outBuffer[1] == '?' && outBuffer[2] == '?' && outBuffer[3] == '\\') {
        return {outBuffer + 4};
    }

    return {};
}

//======================================================================================================================

wstring GetNetworkMap(const wstring& driveNoSlash) {
    // Get the network-mapped connection for the specified drive, if any. The drive string should be
    // a string consisting only of the drive letter followed by a colon.

    DWORD netMapBufferSize {MAX_PATH + 1};
    auto retry = false;    // True iff this is a retry.

    vector<wchar_t> netMapBuffer(1 + netMapBufferSize);

    auto result = WNetGetConnectionW (driveNoSlash.c_str(), netMapBuffer.data(), &netMapBufferSize);
    if (result == ERROR_MORE_DATA) {
        netMapBuffer.reserve(netMapBufferSize);
        result = WNetGetConnectionW (driveNoSlash.c_str(), netMapBuffer.data(), &netMapBufferSize);
    }

    if (result != NO_ERROR) {
        // For all error results, return the empty string. Possible errors include ERROR_BAD_DEVICE,
        // ERROR_NOT_CONNECTED, ERROR_CONNECTION_UNAVAIL, ERROR_NO_NETWORK, ERROR_EXTENDED_ERROR,
        // ERROR_NO_NET_OR_BAD_PATH.
        return {};
    }

    return netMapBuffer.data();
}

//======================================================================================================================

struct Thousands {
    int64_t base;
    wstring suffix;
} thousands[] {
    { 1'000'000'000'000'000'000, L" EB" },
    { 1'000'000'000'000'000, L" PB" },
    { 1'000'000'000'000, L" TB" },
    { 1'000'000'000, L" GB" },
    { 1'000'000, L" MB" },
    { 1'000, L" KB" },
};

wstring numberPretty (int64_t value) {
    // Return a pretty-printed string (with thousands suffix) of the input value.

    // Handle the case of numbers less than 1,000 (including negative values).
    if (value < 1'000)
        return std::move(to_wstring(value) + L" B");

    // Identify the proper thousands group of the value.
    const Thousands *group = thousands;
    while (value < group->base)
        ++group;

    // Get the significant digits of the value as a multiplier of the base (KB, MB, GB, ...).
    auto sigDigits = static_cast<double>(value) / group->base;

    wchar_t buffer[] = L"1.234";
    swprintf(buffer, size(buffer), L"%5f", sigDigits);

    return std::move(wstring{buffer} + group->suffix);
}

//======================================================================================================================

class DriveInfo {
  private:

    wchar_t driveLetter;   // Assigned drive letter ['A' .. 'Z']
    int     driveIndex;    // Logical drive index 0=A, 1=B, ..., 25=Z.
    wstring drive;         // Drive string with trailing slash (for example, 'X:\').
    wstring driveNoSlash;  // Drive string with no trailing slash ('X:').
    wstring driveType;     // Type of drive volume

    wstring volumeGUID;    // Unique volume GUID
    wstring netMap;        // If applicable, the network map associated with the drive
    wstring subst;         // Subst redirection

    // Drive Capacity and Use
    DWORD   sectorsPerCluster {0};
    DWORD   bytesPerSector {0};
    DWORD   clustersFree {0};
    DWORD   clustersTotal {0};
    int64_t bytesTotal {0};
    int64_t bytesFree {0};
    double  percentFree {0};

    // Info from GetVolumeInformation
    bool    isVolInfoValid {false};  // True if we got the drive volume information.
    wstring volumeLabel;             // Drive label
    DWORD   serialNumber {0};        // Volume serial number
    DWORD   maxComponentLength {0};  // Maximum length for volume path components
    DWORD   fileSysFlags {0};        // Flags for volume file system
    wstring fileSysName;             // Name of volume file system

    // This class contains the information for a single drive.

  public:

    DriveInfo (wchar_t _driveLetter /* in [L'A', L'Z'] */)
      : driveLetter{_driveLetter},
        driveIndex {_driveLetter - L'A'},
        drive {L"_:\\"},
        driveNoSlash {L"_:"}
    {
        drive[0] = driveNoSlash[0] = L'A' + driveIndex;

        driveType = DriveType(GetDriveTypeW (drive.c_str()));

        wchar_t nameBuffer [MAX_PATH + 1];
        if (GetVolumeNameForVolumeMountPointW (drive.c_str(), nameBuffer, sizeof nameBuffer)) {
            // The standard volume name is of the form "\\?\Volume{GUID}\". Extract just the GUID.

            volumeGUID = nameBuffer;
            auto guidStart = volumeGUID.find_first_of(L'{') + 1;
            auto guidLen = volumeGUID.find_last_of(L'}') - guidStart;
            volumeGUID = volumeGUID.substr(guidStart, guidLen);
        }

        subst = DriveSubstitution(driveLetter);
        netMap = GetNetworkMap(driveNoSlash);

        wchar_t labelBuffer   [MAX_PATH + 1];   // Buffer for volume label
        wchar_t fileSysBuffer [MAX_PATH + 1];   // Buffer for file system name

        isVolInfoValid = (0 != GetVolumeInformationW (
            drive.c_str(), labelBuffer, sizeof labelBuffer, &serialNumber, &maxComponentLength, &fileSysFlags,
            fileSysBuffer, MAX_PATH + 1));

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

        // Get drive capacity information.
        if (GetDiskFreeSpaceW(drive.c_str(), &sectorsPerCluster, &bytesPerSector, &clustersFree, &clustersTotal)) {
            int64_t bytesPerCluster = bytesPerSector * int64_t(sectorsPerCluster);
            bytesTotal = bytesPerCluster * clustersTotal;
            bytesFree  = bytesPerCluster * clustersFree;
        }

        percentFree = 100.0 * static_cast<double>(bytesFree) / static_cast<double>(bytesTotal);
    }

    ~DriveInfo() {}

    size_t WidthVolumeLabel(size_t currentWidth) const {
        return max(volumeLabel.length(), currentWidth);
    }

    size_t WidthDriveType(size_t currentWidth) const {
        return max(driveType.length(), currentWidth);
    }

    size_t WidthFileSysName(size_t currentWidth) const {
        return max(fileSysName.length(), currentWidth);
    }

    void PrintVolumeInformation (
        const CommandOptions& options, size_t widthVolumeLabel, size_t widthDriveType, size_t widthFileSysName
    ) const {
        // Prints human-readable volume information for this drive.

        // Drive Letter

        wcout << driveNoSlash << L' ';

        // Volume Label

        if (volumeLabel.empty())
            wcout << "- ";
        else
            wcout << '"' << volumeLabel << '"';

        if (volumeLabel.length() < widthVolumeLabel)
            wcout << wstring(widthVolumeLabel - volumeLabel.length(), ' ');

        // Volume Serial Number

        if (!isVolInfoValid)
            wcout << L" -        ";
        else {
            wcout << L' ';
            wcout << hex << setw(4) << setfill(L'0') << (serialNumber >> 16) << L'-' << (serialNumber & 0xffff) << dec;
        }

        // Drive Type

        wcout << L"  " << driveType << L' ';
        if (driveType.length() < widthDriveType)
            wcout << wstring(widthDriveType - driveType.length(), ' ');

        // File System Type

        if (!isVolInfoValid)
            wcout << " -";
        else
            wcout << ' ' << fileSysName << ' ';

        if (fileSysName.length() < widthFileSysName)
            wcout << wstring(widthFileSysName - fileSysName.length(), ' ');

        // Drive Substitution or Network Mapping

        if (subst.length()) // Drive substitution, if any.
            wcout << L"  === " << subst;
        else if (netMap.length()) // Mapping, if any.
            wcout << L"  --> " << netMap;
        else if (volumeGUID.length() > 0)
            wcout << L"  " << volumeGUID;

        // Verbose Information

        if (options.printVerbose) {
            wcout << L"\n   " << numberPretty(bytesFree) << " (";

            if (percentFree > 99.99)
                wcout << "100.0";
            else {
                auto priorPrecision = wcout.precision();
                wcout << defaultfloat << setprecision(4) << percentFree << setprecision(priorPrecision);
            }

            wcout << "%) free / " << numberPretty(bytesTotal) << '\n';
        }

        wcout << '\n';
    }

    void PrintJSONVolumeInformation(bool first) const {
        // Prints volume information in JSON format.

        if (!first)
            wcout << ",\n";

        wcout << "  {\n";
        wcout << L"    \"driveLetter\": \"" << driveLetter << "\",\n";

        if (volumeGUID.length() > 0)
            wcout << L"    \"volumeName\": \"\\\\\\\\?\\\\Volume{" << volumeGUID << "}\\\\\",\n";
        else
            wcout << L"    \"volumeName\": null,\n";

        wcout << L"    \"driveType\": \"" << driveType << L"\",\n";

        wcout << L"    \"substituteFor\": ";
        if (!subst.length())
            wcout << "null,\n";
        else
            wcout << "\"" << Escape(subst) << "\",\n";

        wcout << L"    \"networkMapping\": ";
        if (!netMap.length())
            wcout << "null,\n";
        else
            wcout << "\"" << Escape(netMap) << "\",\n";

        if (!isVolInfoValid) {
            wcout << L"    \"serialNumber\": null,\n";
            wcout << L"    \"label\": null,\n";
            wcout << L"    \"maxComponentLength\": null,\n";
            wcout << L"    \"fileSystem\": null,\n";
            wcout << L"    \"fileSystemFlagsValue\": 0,\n";
            wcout << L"    \"fileSystemFlags\": null";
        } else {
            wcout << L"    \"serialNumber\": \"" << hex << setw(4) << setfill(L'0')
                  << (serialNumber >> 16) << L'-' << (serialNumber & 0xffff)
                  << dec << L"\",\n";
            wcout << L"    \"label\": \"" << Escape(volumeLabel) << L"\",\n";
            wcout << L"    \"maxComponentLength\": " << maxComponentLength << ",\n";
            wcout << L"    \"fileSystem\": \"" << fileSysName << "\",\n";
            wcout << L"    \"fileSystemFlagsValue\": \"0x"
                <<hex <<setw(8) <<setfill(L'0') << fileSysFlags <<dec <<"\",\n";

            // These file-system flags are in increasing value order (bit place, right-to-left).
            static const struct {
                const wchar_t* name;
                const long     value;
            } sysFlagBits[] = {
                { L"caseSensitiveSearch",       FILE_CASE_SENSITIVE_SEARCH },
                { L"casePreservedNames",        FILE_CASE_PRESERVED_NAMES },
                { L"unicodeOnDisk",             FILE_UNICODE_ON_DISK },
                { L"persistentACLs",            FILE_PERSISTENT_ACLS },
                { L"fileCompression",           FILE_FILE_COMPRESSION },
                { L"volumeQuotas",              FILE_VOLUME_QUOTAS },
                { L"supportsSparseFiles",       FILE_SUPPORTS_SPARSE_FILES },
                { L"supportsReparsePoints",     FILE_SUPPORTS_REPARSE_POINTS },
                { L"supportsRemoteStorage",     FILE_SUPPORTS_REMOTE_STORAGE },
                { L"returnsCleanupResultInfo",  FILE_RETURNS_CLEANUP_RESULT_INFO },
                { L"supportsPosixUnlinkRename", FILE_SUPPORTS_POSIX_UNLINK_RENAME },
                { L"volumeIsCompressed",        FILE_VOLUME_IS_COMPRESSED },
                { L"supportsObjectIds",         FILE_SUPPORTS_OBJECT_IDS },
                { L"supportsEncryption",        FILE_SUPPORTS_ENCRYPTION },
                { L"namedStreams",              FILE_NAMED_STREAMS },
                { L"readOnlyVolume",            FILE_READ_ONLY_VOLUME },
                { L"sequentialWriteOnce",       FILE_SEQUENTIAL_WRITE_ONCE },
                { L"supportsTransactions",      FILE_SUPPORTS_TRANSACTIONS },
                { L"supportsHardLinks",         FILE_SUPPORTS_HARD_LINKS },
                { L"extendedAttributes",        FILE_SUPPORTS_EXTENDED_ATTRIBUTES },
                { L"supportsOpenByFileId",      FILE_SUPPORTS_OPEN_BY_FILE_ID },
                { L"supportsUSNJournal",        FILE_SUPPORTS_USN_JOURNAL },
                { L"supportsIntegrityStreams",  FILE_SUPPORTS_INTEGRITY_STREAMS },
                { L"supportsBlockRefcounting",  FILE_SUPPORTS_BLOCK_REFCOUNTING },
                { L"supportsSparseVDL",         FILE_SUPPORTS_SPARSE_VDL },
                { L"DAXvolume",                 FILE_DAX_VOLUME },
                { L"supportsGhosting",          FILE_SUPPORTS_GHOSTING },
            };

            wcout << L"    \"fileSystemFlags\": {\n";

            bool first = true;
            for (auto sysFlag : sysFlagBits) {
                if (!first) wcout << L",\n";
                wcout << L"      \"" << sysFlag.name << L"\": " << (fileSysFlags & sysFlag.value ? 1 : 0);
                first = false;
            }

            wcout << L"\n    }";
        }

        // Drive Capacity and Usage
        if (clustersTotal > 0) {
            wcout << L",\n";
            wcout << L"    \"capacityBytes\": " << bytesTotal << ",\n";
            wcout << L"    \"capacityPretty\": \"" << numberPretty(bytesTotal) << "\",\n";
            wcout << L"    \"freeBytes\": " << bytesFree << ",\n";
            wcout << L"    \"freePretty\": \"" << numberPretty(bytesFree) << "\",\n";
            wcout << L"    \"percentFree\": " << percentFree;
        }

        wcout << "\n  }";
    }

};

//======================================================================================================================

void PrintResultsHuman(const CommandOptions& options, vector<DriveInfo>& drives) {
    size_t widthVolumeLabel{0};
    size_t widthDriveType{0};
    size_t widthFileSysName{0};

    for (const auto& drive : drives) {
        widthVolumeLabel = drive.WidthVolumeLabel(widthVolumeLabel);
        widthDriveType   = drive.WidthDriveType(widthDriveType);
        widthFileSysName = drive.WidthFileSysName(widthFileSysName);
    }

    for (const auto& drive : drives)
        drive.PrintVolumeInformation(options, widthVolumeLabel, widthDriveType, widthFileSysName);
}

//======================================================================================================================

void PrintResultsJSON(const CommandOptions& options, vector<DriveInfo>& drives) {
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

This program prints drive information for all devices, network mappings, DOS
devices, and drive substitutions (via the `subst` command).

Unless the `--json` option is supplied, the following drive values will be
printed, in this order:

    - Drive Letter
    - Label
    - Serial Number
    - Type (No root, Removable, Fixed, Remote, CD-ROM, or RAM Disk)
    - File System (for example, NTFS, FAT, or FAT32)
    - Volume GUID, drive substitution or target, or network mapping

The volume GUID can be used in a formal volume name, with the following form:

    \\?\Volume{GUID}\

Options
    [drive]
        Optional drive letter for specific drive report (colon optional). If no
        drive is specified, reports information for all drives.

    --help, -h, /?
        Print help information.

    --json, -j
        Print full drive information in JSON format. To understand the file
        system flags, see documentation for the Windows function
        GetVolumeInformationW().

    --verbose, -v
        Generally, print additional volume information. This switch is ignored
        if the `--json` option is supplied. Additional volume information
        includes the amount of free space and the total drive capacity.

    --version
        Print program version.

)";

//======================================================================================================================

int wmain (int argc, wchar_t* argv[]) {

    // Parse command line options.
    CommandOptions commandOptions;

    if (!commandOptions.parseArguments(argc, argv))
        return 1;

    if (commandOptions.printVersion) {
        if (!commandOptions.printHelp) {
            wcout << programVersion << L'\n';
        } else {
            wcout << helpText << programVersion << L'\n';
        }

        return 0;
    }

    vector<DriveInfo> drives;

    if (commandOptions.singleDrive) {
        if (!DriveValid(commandOptions.singleDrive)) {
            wcout << commandOptions.programName
                  << L": No volume present at drive " << commandOptions.singleDrive << L":." << endl;
            return 1;
        }
        const auto drive = commandOptions.singleDrive;
        if (DriveValid(drive))
            drives.emplace_back(drive);
    } else {
        // Query all drives for volume information.
        for (auto driveLetter = L'A';  driveLetter <= L'Z';  ++driveLetter)
            if (DriveValid(driveLetter))
                drives.emplace_back(driveLetter);
    }

    // For each drive, print volume information.
    if (commandOptions.printJSON)
        PrintResultsJSON(commandOptions, drives);
    else
        PrintResultsHuman(commandOptions, drives);

    return 0;
}
