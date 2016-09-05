//
// Created by baic on 2016-8-11.
// interface definitions
//

#ifndef C_HASH_CMP_LOG_MAIN_H
#define C_HASH_CMP_LOG_MAIN_H

#include <future>
#include <set>
#include <iostream>
#include <fstream>
#include <regex>
#include <experimental/filesystem>

//#include "supportFunctions.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_type_defines
namespace stdFs = std::experimental::filesystem;
typedef std::vector<std::string> RegexRawLines;
typedef int LineNr;
typedef size_t HashValue;
typedef std::ios::pos_type Position;
typedef std::pair<LineNr, Position> PAIR_LineInfo;
typedef std::vector<HashValue> LineHashes; ///< use vector to store hashes. Remember to sort it befor set_difference.
typedef std::map<HashValue, PAIR_LineInfo> MAP_HashAndLine;
typedef std::pair<LineNr, std::string> DiffResult;
typedef std::vector<DiffResult> DiffResultLines;
typedef std::vector<std::string> LogFileList;

/// @typedef MAP_HashAndLine : hashValue maps to {LineNr, Position/offset_in_file};
/// so when reading lines, we could quickly loacte lineNumber/postion from hash

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_string_resources
//constexpr std::string FullHelpMessage = {
//    R"rawstring(Compare common-log-files
//
//positional arguments:
//  f1                    the 1st(left)  file/dir to compare
//  f2                    the 2nd(right) file/dir to compare
//
//optional arguments:
//  -h                    show this help message and exit
//  -O OUTPUT
//                        file to store the diff summary
//  -L FILE_LIST
//                        a file which lists all the file names (which exist in both dir1 and dir2) to be compared (one name perl line)
//example:
//    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
//    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
//    c_hash_cmp_log.exe file1 file2 -L fileList.txt -O diff_summary.txt
//)rawstring"
//};

///@breif a list of raw regex expressions
const RegexRawLines Raw_filter_list{
	"^#",
	//R"rawstring(MAOV\|StdProperty\|Verifier Configuration File)rawstring",
	//R"rawstring(MAOV\|StdProperty\|Verification Date & Time)rawstring",
	//R"rawstring(MAOV\|StdVersion\|)rawstring",
	//R"rawstring(MODEL\|StdProperty\|Simulation Date & Time)rawstring",
	//R"rawstring(MODEL\|StdVersion\|CEL)rawstring",
	//R"rawstring(MODEL\|StdVersion\|ModelExecutable)rawstring",
	//R"rawstring(MODEL\|StdVersion\|ModelLibrary)rawstring",
	//R"rawstring(SortLog\|StdProperty\|Filtered Events File)rawstring",
	//R"rawstring(SortLog\|StdProperty\|sort_log.pl)rawstring",
};
const std::string RawStr_epilog_CallingExample{
	R"rawstring(example:
    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt -T log
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt -L fileList.txt
    )rawstring"
}; ///< command line examples
const std::string defaultOutputFile{ "diff_summary.txt" }; ///< default outpu files

///@breif the name for the logger [https://github.com/gabime/spdlog, local: w:\github-libs\spdlog (with my own changes)]
const std::string LoggerName{ "myLogger1" };
const char prefixMarks[]{ '-', '+' }; ///< prefix when printing the diff-lines
const int _left__ = 0;
const int _right_ = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_types_and_classes
/*! @brief to indicate which kind of comparison when parsing the parameters
 */
enum CompareType
{
	file,           ///< compare file1 vs. file2
	dir,            ///< compare dir1 vs. dir2 (for all files in dir, same-name Vs. same-name)
	dir_with_list,   ///< dir1 vs. dir2, only for files in the file-list
	unknown,
};

/*! @brief store the parsing result for parameters
 */
struct myParameter
{
public:

	/*! @brief parse the command line parameters. p.valid is updated according to parsing result.
	*
	* @param argc [in] same as the one fed to main()
	* @param argv [in] same as the one fed to main()
	*/
	myParameter(const int argc, const char *const *argv);


	stdFs::path path_left_;     ///< name for f1/dir1
	stdFs::path path_right;     ///< name for f2/dir2
	stdFs::path listFilePath;   ///< list_file, when comparing @ref dir_with_list
	stdFs::path fileType;       ///< file with which extionson to compare("log" for *.log), if not given, compare all

	stdFs::path outputFilePath; ///< default: diff_summary.txt (and another diff_summary_brief.txt)

	bool valid;                 ///< whether it is valid
	CompareType howToCompare;   ///< which kind of comparison

	/* The order of initialization is the order that the members are declared in the class, not the order of the initialization list.
	 So, ALWAYS initialize member variables in the order they're declared.*/

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define my_big_functions

/*! @brief compare logfiles by computing the hash of each lines,
 *  then use set-math-operation to find out unique lines
 *
 * @param file_left_ [in]
 * @param file_right [in]
 */
void compare_logFile(const std::vector<std::string> &file);

/*!
 * @brief compare two log directories, assuming they are already both valid.
 * @param dir_left_
 * @param dir_right
 */
void compare_dir(const std::vector<std::string> &dir, const std::string &file_type_to_compare = "");

/*! setup logger, with 3 sinks: console, full-summary-file, brief-file
 *
 * @param diff_record_file [in] name of the diff-result file
 * @return none
 */
void setup_logger(const std::string &diff_record_file = defaultOutputFile);

/*! @brief compute hashes line by line, store the result in a sorted-container of hashes, and a map
 *
 * @param fileName
 * @return  pair<Hashes, MAP_HashAndLine>
 */
std::pair<LineHashes, MAP_HashAndLine> doHashLines(const std::string &fileName);

/*! @brief find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements
 */
LineHashes operator-(const LineHashes &setA, const LineHashes &setB);

/*! @brief get lines from the hash-values
 *
 * @param hashes [in] the hashes
 * @param fileName [in] the file
 * @param myMap [in] the map which contains the hash<=>[lineNr, offset] infomation.
 * @return the lines
 */
DiffResultLines getLinesFromHash(const LineHashes &hashes, const std::string &fileName, const MAP_HashAndLine &myMap);

/*! @brief filter lines with regex, and sort them in the end.
 *
 * @param lines [in] the lines
 * @param rawRegexList [in] the regex in raw-list
 * @return the result, the filtered lines, sorted.
 */
DiffResultLines regexFilterLines(const DiffResultLines &lines, const RegexRawLines &rawRegexList);

/*!
 * @brief get the names of the regular-file in a directory
 * @param folder
 * @return a vector of the file names
 */
LogFileList getFilesFromDir(const std::string folder, const std::string &fileExt = "");

LogFileList operator-(const LogFileList &listA, const LogFileList &listB);

bool iCompString(std::string s0, std::string s1);

std::string getBase(const std::string &s);

std::string &removeLastSlash(std::string &s);

#endif //C_HASH_CMP_LOG_MAIN_H

