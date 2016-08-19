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

#include "supportFunctions.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define my_type_defines
typedef std::vector<std::string> RegexRawLines;
typedef int LineNr;
typedef size_t HashValue;
typedef std::ios::pos_type Position;
typedef std::pair<LineNr, Position> PAIR_LineInfo;
typedef std::set<HashValue> HashSet; ///< a "set" of hashes
typedef std::map<HashValue, PAIR_LineInfo> MAP_HashAndLine; ///<  hashValue maps to {LineNr, Position/offset_in_file}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::pair<LineNr, std::string> DiffResult;
typedef std::vector<DiffResult> DiffResultLines;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define my_string_resources
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
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt -L fileList.txt
    )rawstring"
}; ///< command line examples
const std::string defaultOutputFile{"diff_summary.txt"}; ///< default outpu files

///@breif the name for the logger [https://github.com/gabime/spdlog, local: w:\github-libs\spdlog (with my own changes)]
const std::string LoggerName{"myLogger1"};
const char prefixMarks[]{'-', '+'}; ///< prefix when printing the diff-lines

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define my_types_and_classes
/*! @brief to indicate which kind of comparison when parsing the parameters
 */
enum CompareType {
  file,           ///< compare file1 vs. file2
  dir,            ///< compare dir1 vs. dir2 (for all files in dir, same-name Vs. same-name)
  dir_with_list   ///< dir1 vs. dir2, only for files in the file-list
};

/*! @brief store the parsing result for parameters
 */
struct myParameter {
public:
  bool valid;                 ///< whether it is valid
  CompareType howToCompare;   ///< which kind of comparison
  std::string f1_name;        ///< name for f1/dir1
  std::string f2_name;        ///< name for f2/dir2
  std::string listFile_name;  ///< list_file, when comparing @ref dir_with_list
  std::string outputFile_name;///< default: diff_summary.txt (and another diff_summary_brief.txt)

  /// initialize to default values
  myParameter() :
      valid{false},
      outputFile_name{defaultOutputFile},
      f1_name{}, //to check, use: f1_name.empty()
      f2_name{},
      listFile_name{} {};

  /*! @brief parse the command line parameters. p.valid is updated according to parsing result.
   *
   * @param argc [in] same as the one fed to main()
   * @param argv [in] same as the one fed to main()
   * @return none
   */
  myParameter(const int argc, const char *const *argv) {
    args::ArgumentParser parser("Compare common-log-files.", RawStr_epilog_CallingExample);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string>
        output_file(parser, "OUTPUT", "OUTPUT:file to store the diff summary", {'O', "output"});
    args::ValueFlag<std::string> cmp_file_list(parser,
                                               "FILE_LIST",
                                               "FILE_LIST:a file of all the file names (which exist in both dir1 and dir2) to be compared, one name perl line)",
                                               {'L', "file_list"});
    args::Positional<std::string> f1(parser, "f1", "the 1st(left)  file/dir to compare");
    args::Positional<std::string> f2(parser, "f2", "the 1st(left)  file/dir to compare");

    /* parse parameters */
    try {
      parser.ParseCLI(argc, argv);
      if (!f1 || !f2) //I do need both f1 and f2
      {
        std::cerr << "\n! Missing parameters. Both f1 and f2 are needed\n" << std::endl;
        std::cout << "===========" << std::endl;
        std::cout << parser;
        this->valid = false;
        return;
      }
    }
    catch (args::Help) {
      std::cout << "===========" << std::endl;
      std::cout << parser;
      this->valid = false;
      return;
    }
    catch (args::ParseError e) {
      std::cerr << "===========" << std::endl;
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      this->valid = false;
      return;
    }

    /* validate parameters */
    if (output_file) {
      this->outputFile_name = args::get(output_file);
      std::cout << "output_file: " << this->outputFile_name << std::endl;
    } else {
      this->outputFile_name = defaultOutputFile;
    }
    if (cmp_file_list) {
      this->listFile_name = args::get(cmp_file_list);
      std::cout << "cmp_file_list: " << this->listFile_name << std::endl;
    }

    this->f1_name = f1.Get();
    this->f2_name = f2.Get();

    /* both f1 and f2 should be valid */
    if (!FileCanBeRead(this->f1_name)) {
      std::cerr << "\n! Can't open " << this->f1_name << ", \nplease check your input.\n" << std::endl;
      std::cout << "===========" << std::endl;
      std::cout << parser;
      this->valid = false;
      return;
    } else if (!FileCanBeRead(this->f2_name)) {
      std::cerr << "\n! Can't open " << this->f2_name << ", \nplease check your input.\n" << std::endl;
      std::cout << "===========" << std::endl;
      std::cout << parser;
      this->valid = false;
      return;
    }

    if (IsDir(this->f1_name) && IsDir(this->f2_name)) {
      this->howToCompare = CompareType::dir;
      this->valid = true;
      //todo: check list-file, for comparing with list

    } else if (!IsDir(this->f1_name) && !IsDir(this->f2_name)) {
      this->howToCompare = CompareType::file;
      this->valid = true;
    } else {
      this->valid = false;
    }
    return;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define my_big_functions

/*! @brief compare logfiles by computing the hash of each lines,
 *  then use set-math-operation to find out unique lines
 *
 * @param file_left_ [in]
 * @param file_right [in]
 */
void hash_and_compare_log(const std::string file_left_, const std::string file_right);

/*! @brief compute hashes line by line, store the result in a set, and a map
 *
 * @param fileName
 * @return  pair<HashSet, MAP_HashAndLine>
 */
std::pair<HashSet, MAP_HashAndLine> doHashLines(const std::string &fileName);

/*! @brief find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements stored in a hashset
 */
HashSet operator-(const HashSet &setA, const HashSet &setB);
/*! @brief find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements stored in a hashset
 */
HashSet getUniqueElements(const HashSet &setA, const HashSet &setB);

/*! @brief get lines from the hash-values
 *
 * @param fileName [in] the file
 * @param hashSet [in] the hashes
 * @param myMap [in] the map which contains the hash<=>[lineNr, offset] infomation.
 * @return the lines
 */
DiffResultLines getLinesFromHash(const std::string &fileName, const HashSet &hashSet, const MAP_HashAndLine &myMap);

/*! @brief filter lines with regex
 *
 * @param lines [in] the lines
 * @param rawRegexList [in] the regex in raw-list
 * @return the result, the filtered lines, sorted.
 */
DiffResultLines regexFilterLines(const DiffResultLines &lines, const RegexRawLines &rawRegexList);

/*! setup logger, with 3 sinks: console, full-summary-file, brief-file
 *
 * @param diff_record_file [in] name of the diff-result file
 * @return none
 */
void setup_logger(std::string diff_record_file = "diff_summary.txt");

#endif //C_HASH_CMP_LOG_MAIN_H

