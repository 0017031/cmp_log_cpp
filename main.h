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
typedef pair<LineNr, Position> PAIR_LineInfo;
typedef size_t HashValue;
typedef map<HashValue, PAIR_LineInfo> MAP_HashAndLine;
typedef set<HashValue> HashSet;

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

/*!
 * @brief compute hash for each line of the file, store the hash-set and hash-map.
 * @param fileName [in] name of the file
 * @param set [out] hash-set, a "set" of the hash values
 * @param map [out] a "map"; l_hash ~MappingTo~ (lineNumber, position)
 */
void cchash(const string &fileName,
    HashSet &set,
    MAP_HashAndLine &map)
;


/*! get the file name (last) in the string. (f.txt in "c:\f.txt")
 *
 * @param s [in] input string
 * @return file namle (last) in the string
 */
string getFileName(const string &s)
{
    string sep = "\\/";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(found + 1);
    }
    return s;
}

/*! get the base name. ( ffff in ffff.txt)
 *
 * @param s [in] input string
 * @return base name
 */
string getBaseName(const string &s)
{
    string sep = ".";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(0, found);
    }
    return s;
}

/*! strip the trailing /r/n of a string
 *
 * @param s [in] input string
 * @return the result
 */
string stripCRLF(const string &s)
{
    string sep = "\r\n";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(0, found);
    }
    return s;
}

#ifndef __linux
#include <io.h>
#define access    _access_s
#else
#include <unistd.h>
#endif

/*! Check if a file exists.
 *
 * @param [in] fileName
 * @return True if it exists.
 */
bool FileExists(const std::string &fileName)
{
    return access(fileName.c_str(), 0) == 0;
}

/*! Check if a file can be read
 *
 * @param [in] fileName
 * @return True if it can be read.
 */
bool FileCanBeRead(const std::string &fileName)
{
    return access(fileName.c_str(), 4) == 0;
}

/*! check if it is a directory
 *
 * @param [in] theName
 * @return True if it is an directory
 */
bool IsDir(const std::string &theName)
{
    auto attributes = GetFileAttributes(theName.c_str());
    //cout << "file:" << theName << " attrib:" << attributes << endl;
    return (FILE_ATTRIBUTE_DIRECTORY == attributes);
}

#endif //C_HASH_CMP_LOG_MAIN_H

