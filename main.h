//
// Created by baic on 2016-8-11.
//

#ifndef C_HASH_CMP_LOG_MAIN_H
#define C_HASH_CMP_LOG_MAIN_H

#include <set>
#include <iostream>
#include <fstream>
#include <regex>

#include "pchheader.h"

using namespace std;

typedef int LineNr;
typedef ios::pos_type Position;
typedef pair<LineNr, Position> cPAIR_LineInfo;
typedef size_t HashValue;
typedef map<HashValue, cPAIR_LineInfo> cMAP_HashAndLine;
typedef set<HashValue> cHashSet;

class InputParser
{
 public:
  InputParser(int &argc, char **argv)
  {
      for (int i = 1; i < argc; ++i)
          this->tokens.push_back(std::string(argv[i]));
  }
  /// @author iain
  const std::string &getCmdOption(const std::string &option) const
  {
      std::vector<std::string>::const_iterator itr;
      itr = std::find(this->tokens.begin(), this->tokens.end(), option);
      if (itr != this->tokens.end() && ++itr != this->tokens.end())
      {
          return *itr;
      }
      return "";
  }
  /// @author iain
  bool cmdOptionExists(const std::string &option) const
  {
      return std::find(this->tokens.begin(), this->tokens.end(), option)
          != this->tokens.end();
  }
 private:
  std::vector<std::string> tokens;
};

/*! compare two logfiles, assuing both input files are valid.
 *
 * @param file_left [in] name of the left/old file
 * @param file_right [in] name of the right/new file
 * @return none
 */
void hash_compare_log_file(string file_left, string file_right);

/*! setup logger, with 3 sinks: console, full-summary-file, brief-file
 *
 * @param diff_record_file [in] name of the diff-result file
 * @return none
 */
void setup_logger(string diff_record_file = "diff_summary.txt");

/*! get the file name (last) in the string. (f.txt in "c:\f.txt")
 *
 * @param s [in] input string
 * @return file namle (last) in the string
 */
string getFileName(const string &s);

/*! get the base name. ( ffff in ffff.txt)
 *
 * @param s [in] input string
 * @return base name
 */
string getBaseName(const string &s);

/*! strip the trailing /r/n of a string
 *
 * @param s [in] input string
 * @return the result
 */
string stripCRLF(const string &s);

#endif //C_HASH_CMP_LOG_MAIN_H
