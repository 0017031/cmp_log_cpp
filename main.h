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
 * @param file_left_ [in] name of the left/old file
 * @param file_right [in] name of the right/new file
 * @return none
 */
void hash_and_compare_log(const string file_left_, const string file_right);

/*! setup logger, with 3 sinks: console, full-summary-file, brief-file
 *
 * @param diff_record_file [in] name of the diff-result file
 * @return none
 */
void setup_logger(string diff_record_file = "diff_summary.txt");

/*! @breif find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements stored in a hashset
 */
HashSet getUniqueElements(const HashSet &setA, const HashSet &setB)
{
    HashSet result;
    set_difference(
        setA.begin(), setA.end(),
        setB.begin(), setB.end(),
        inserter(result, result.end()));
    return result;
}

/*! @breif find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements stored in a hashset
 */
HashSet operator-(const HashSet &setA, const HashSet &setB)
{
    HashSet result;
    set_difference(
        setA.begin(), setA.end(),
        setB.begin(), setB.end(),
        inserter(result, result.end()));
    return result;
}


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

#if 0 //my trial codes
void printHash(const string &fileName, const pair<HashSet, MAP_HashAndLine> &r)
{
    ifstream inFile(fileName.c_str(), ios_base::binary); //must open as binary, then position is correct.
    auto myLogger = spdlog::get(LoggerName);

    cout << "hash_size:" << r.first.size();
    cout << "\tmap_size:" << r.second.size() << endl;

    //for (auto h: r.first)
    //{
    //    cout << h << endl;
    //}

    for (auto h: r.second)
    {
        //cout << h.first << ", " << h.second.first << ", " << h.second.second ;

        try //try to find privious line
        {
            PAIR_LineInfo l_Info = h.second/*map.at(l_hash)*/;  //throw "std::out_of_range" if not found
            string myline;
            getline(inFile.seekg(l_Info.second), myline);
            myLogger->error("#{},\t{}", l_Info.first, myline);
        }
        catch (const out_of_range &oor)
        {
            myLogger->error("\tCan't find line");
        }
    }
}
void test_hash(void)
{
    setup_logger();
    string f_left{"W:/tools_baichun/log_cmp_easy/d1/t.log"};
    string f_right{"W:/tools_baichun/log_cmp_easy/d2/t.log"};

    auto aRet0 = async(ddhash, f_left);
    auto aRet1 = async(ddhash, f_right);

    pair<HashSet, MAP_HashAndLine> r0 = aRet0.get();
    pair<HashSet, MAP_HashAndLine> r1 = aRet1.get();

    printHash(f_left, r0);
    printHash(f_right, r1);

    return;
}
#endif

#endif //C_HASH_CMP_LOG_MAIN_H

