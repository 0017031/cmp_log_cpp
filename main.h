//
// Created by baic on 2016-8-11.

#ifndef C_HASH_CMP_LOG_MAIN_H
#define C_HASH_CMP_LOG_MAIN_H

#include "spdlog/spdlog.h"
#include "args/args.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_type_defines
namespace stdfs = std::experimental::filesystem;
using RegexRawLines = std::vector<std::string>;
using LineNr = int ;
using HashValue = size_t ;
using Position =  std::ios::pos_type ;
using PAIR_LineInfo = std::pair<LineNr, Position> ;
using LineHashes = std::vector<HashValue> ; ///< use vector to store hashes. Remember to sort it befor set_difference.
using MAP_HashAndLine = std::map<HashValue, PAIR_LineInfo> ;
using DiffResult = std::pair<LineNr, std::string>;
using DiffResultLines = std::vector<DiffResult> ;
using LogFileList = std::vector<std::string> ;

/// @typedef MAP_HashAndLine : hashValue maps to {LineNr, Position/offset_in_file};
/// so when reading lines, we could quickly loacte lineNumber/postion from hash


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_string_resources

/*
const std::string FullHelpMessage = {
    R"rawstring(Compare common-log-files

positional arguments:
  f1                    the 1st(left)  file/dir to compare
  f2                    the 2nd(right) file/dir to compare

optional arguments:
  -h                    show this help message and exit
  -O OUTPUT
                        file to store the diff summary
  -L FILE_LIST
                        a file which lists all the file names (which exist in both dir1 and dir2) to be compared (one name perl line)
example:
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2 -L fileList.txt -O diff_summary.txt
)rawstring"
};
*/

///@breif a list of raw regex expressions
const RegexRawLines Raw_filter_list{
    "^#",
    R"rawstring(MAOV\|StdProperty\|Verifier Configuration File)rawstring",
    R"rawstring(MAOV\|StdProperty\|Verification Date & Time)rawstring",
    R"rawstring(MAOV\|StdVersion\|)rawstring",
    R"rawstring(MODEL\|StdProperty\|Simulation Date & Time)rawstring",
    R"rawstring(MODEL\|StdVersion\|CEL)rawstring",
    R"rawstring(MODEL\|StdVersion\|ModelExecutable)rawstring",
    R"rawstring(MODEL\|StdVersion\|ModelLibrary)rawstring",
    R"rawstring(SortLog\|StdProperty\|Filtered Events File)rawstring",
    R"rawstring(SortLog\|StdProperty\|sort_log.pl)rawstring",
    R"rawstring(MODEL\|ProcessHRVSegmentDone)rawstring",
    R"rawstring(MAOVStatistics.+NotExercised)rawstring",
};
const std::string RawStr_epilog_CallingExample{
    R"rawstring(example:
    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt -T log
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt -L fileList.txt
    )rawstring"};                                        ///< command line examples
const std::string defaultOutputFile{"diff_summary.txt"}; ///< default outpu files

const std::string defaultSubDir{"lines_diff"}; ///< default sub dir

///@breif the name for the logger [https://github.com/gabime/spdlog, local: w:/github-libs/spdlog (with my own changes for multi-log-sink)]
const std::string LoggerName{"myLogger1"};
constexpr char prefixMarks[]{'-', '+'}; ///< prefix when printing the diff-lines
constexpr int _left__ = 0;
constexpr int _right_ = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_types_and_classes
/*! @brief to indicate which kind of comparison when parsing the parameters
 */
enum class CompareType {
    file,          ///< compare file1 vs. file2
    dir,           ///< compare dir1 vs. dir2 (for all files in dir, same-name Vs. same-name)
    dir_with_list, ///< dir1 vs. dir2, only for files in the file-list
    unknown,
};

/*! @brief store the parsing result for parameters
 */
struct myParameter {
    /*! @brief parse the command line parameters. p.valid is updated according to parsing result.
    *
    * @param argc [in] same as the one fed to main()
    * @param argv [in] same as the one fed to main()
    */
    myParameter(const int argc, const char *const *argv);

    stdfs::path path_left_;   ///< name for f1/dir1
    stdfs::path path_right;   ///< name for f2/dir2
    stdfs::path listFilePath; ///< list_file, when comparing @ref dir_with_list
    std::string fileType;     ///< file with which extionson to compare("log" for *.log), if not given, compare all

    stdfs::path outputFilePath; ///< default: diff_summary.txt (and another diff_summary_brief.txt)

	bool valid{ false };               ///< whether the instance is valid
	CompareType howToCompare{ CompareType::unknown }; ///< which kind of comparison

    /* The order of initialization is the order that the members are declared in the class, not the order of the initialization list.
     So, ALWAYS initialize member variables in the order they're declared.*/
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_big_functions

/*! @brief compare logfiles by computing the hash of each lines,
 *  then use set-math-operation to find out unique lines
 *
 * @param  [in] files // left, right
 */
void compare_logFile(const std::array<stdfs::path, 2> &files) noexcept;

/*!
 * @brief compare two log directories, assuming they are already both valid.
 * @param [in] dirs // left, right
 * @param [in] file_type_to_compare // e.g. -T txt , -T log
 */
void compare_dir(const std::array<stdfs::path, 2> &dirs, const std::string &file_type_to_compare) noexcept;

/*! setup logger, with 3 sinks: console, full-summary-file, brief-file
 *
 * @param diff_record_file [in] name of the diff-result file
 * @return none
 */
void setup_logger(const std::string &diff_record_file = defaultOutputFile) noexcept;

/*! @brief compute hashes line by line, store the result in a sorted-container of hashes, and a map
 *
 * @param file  //path
 * @return  pair<Hashes, MAP_HashAndLine>
 */
std::pair<LineHashes, MAP_HashAndLine> doHashLines(const stdfs::path &file) noexcept;

/*! @brief find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements
 */
LineHashes operator-(const LineHashes &setA, const LineHashes &setB) noexcept;

/*! @brief find the unique elements betwen two sets of logfiles, case insensitive
*
* @param listA
* @param listB
* @return the unique elements
*/
LogFileList operator-(const LogFileList &listA, const LogFileList &listB) noexcept;

/*! @brief get lines from the hash-values
 *
 * @param hashes [in] the hashes
 * @param file [in] the file
 * @param myMap [in] the map which contains the hash<=>[lineNr, offset] infomation.
 * @return the lines
 */
DiffResultLines getLinesFromHash(const LineHashes &hashes, const stdfs::path &file, const MAP_HashAndLine &myMap) noexcept;

/*! @brief filter lines with regex, and sort them in the end.
 *
 * @param lines [in] the lines
 * @param rawRegexList [in] the regex in raw-list
 * @return the result, the filtered lines, sorted.
 */
DiffResultLines regexFilterLines(const DiffResultLines &lines, const RegexRawLines &rawRegexList) noexcept;

/*!
 * @brief get the names of the regular-file in a directory
 * @param folder
 * @param fileExt
 * @return a vector of the file names
 */
LogFileList getFilesFromDir(const stdfs::path folder, const std::string &fileExt) noexcept;

bool iCompString(std::string s0, std::string s1) noexcept;

std::string getBase(const std::string &s) noexcept;

std::string &removeLastSlash(std::string &s) noexcept;

#endif // C_HASH_CMP_LOG_MAIN_H
