//
// Created by baic on 2016-8-22.
//

//#include "main.h"
#include <iostream>
#include <fstream>

#include <fileapi.h> //windows

std::string getFileName(const std::string &s) {
  std::string sep = "\\/";

  size_t found = s.find_last_of(sep);
  if (found != std::string::npos) {
    return s.substr(found + 1); //from the "sep" to string-end, [sep+1, end)
  }
  return s;
}

std::string getBaseName(const std::string &s) {
  std::string sep = ".";

  size_t found = s.find_last_of(sep);
  if (found != std::string::npos) {
    return s.substr(0, found); //from string-start to "sep", [start, sep)
  }
  return s;
}

std::string stripCRLF(const std::string &s) {
  std::string sep = "\r\n";

  size_t found = s.find_last_of(sep);
  if (found != std::string::npos) {
    return s.substr(0, found); // [start, sep)
  }
  return s;
}

#ifndef __linux
#include <io.h>
#include <vector>

#define access    _access_s // https://msdn.microsoft.com/en-us/library/a2xs1dts.aspx
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

#ifndef R_OK
#define R_OK 4
#endif


bool FileExists(const std::string &fileName) {
  return access(fileName.c_str(), F_OK) == 0; // 00: Existence only.
}

bool FileCanBeRead(const std::string &fileName) {
  return access(fileName.c_str(), R_OK) == 0; // 04: Read permission.
}

bool IsDir(const std::string &theName) {
  auto attributes = GetFileAttributes(theName.c_str());
  //std::cout << "file:" << theName << " attrib:" << attributes << std::endl;
  return (FILE_ATTRIBUTE_DIRECTORY == attributes);
}

#include <Windows.h>

std::vector<std::string> getFiles(std::string folder) {
  std::vector<std::string> names;
  std::string search_path = folder + "/*.*";
  WIN32_FIND_DATA fd;
  HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      // read all (real) files in current folder
      // , delete '!' read other 2 default folder . and ..
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        names.push_back(fd.cFileName);
      }
    } while (::FindNextFile(hFind, &fd));
    ::FindClose(hFind);
  }
  return names;
}

#if 0 //my trial codes
void printHash(const std::string &fileName, const pair<LineHashes, MAP_HashAndLine> &r)
{
    ifstream inFile(fileName.c_str(), ios_base::binary); //must open as binary, then position is correct.
    auto myLogger = spdlog::get(LoggerName);

    std::cout << "hash_size:" << r.first.size();
    std::cout << "\tmap_size:" << r.second.size() << std::endl;

    //for (auto h: r.first)
    //{
    //    std::cout << h << std::endl;
    //}

    for (auto h: r.second)
    {
        //std::cout << h.first << ", " << h.second.first << ", " << h.second.second ;

        try //try to find previous line
        {
            PAIR_LineInfo l_Info = h.second/*map.at(l_hash)*/;  //throw "std::out_of_range" if not found
            std::string myline;
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
    std::string f_left{"W:/tools_baichun/log_cmp_easy/d1/t.log"};
    std::string f_right{"W:/tools_baichun/log_cmp_easy/d2/t.log"};

    auto aRet0 = async(ddhash, f_left);
    auto aRet1 = async(ddhash, f_right);

    pair<LineHashes, MAP_HashAndLine> r0 = aRet0.get();
    pair<LineHashes, MAP_HashAndLine> r1 = aRet1.get();

    printHash(f_left, r0);
    printHash(f_right, r1);

    return;
}
#endif

#if 0 //async() no good here, because ResultSetAndMap[] in use.

/*! @brief find the unique elements betwen setA and setB
 *
 * @param setA
 * @param setB
 * @return the unique elements
 */
LineHashes getUniqueElements(const LineHashes &setA, const LineHashes &setB);

LineHashes getUniqueElements(const LineHashes &setA, const LineHashes &setB) {
  return setA - setB;
}
auto a1 = async(launch::async, getUniqueElements, ResultSetAndMap[_left__].first, ResultSetAndMap[_right_].first);
auto a2 = async(launch::async, getUniqueElements, ResultSetAndMap[_right_].first, ResultSetAndMap[_left__].first);
LineHashes uniqHashes[2]{a1.get(), a2.get()};
#endif

#if 0 //def REPORT_REPEATED_LINES
if (ret.second) //ret.second: a bool that is true if the element was actually inserted
        {
            map[l_hash] = PAIR_LineInfo {lineNumber, position}; // l_hash ~mapped-to~ (lineNumber, position)

        } else /* report if found same line (there is nothing to do about hash-collision, unless to change the hash-function) */
        {
            auto myLogger = spdlog::get(LoggerName);
            //myLogger->error("found a identical line: {}, {}#, {}", fileName, lineNumber, line);
            myLogger->error("same line?: #{}, {}", lineNumber, line);
            try //try to find previous line
            {
                PAIR_LineInfo l_Info = map.at(l_hash);  //throw "std::out_of_range" if not found
                Position posOriginal = inFile.tellg();
                string previousLine;
                getline(inFile.seekg(l_Info.second), previousLine);
                myLogger->error("prev line : #{}, {}\n", l_Info.first, previousLine);
                inFile.seekg(posOriginal);              //remember to restore file position!
            }
            catch (const std::out_of_range &oor)
            {
                myLogger->error("\tCan't find previous line");
            }
    }
#endif

#if 0
vector<string> files[2];      //good
files[_left__] = getFiles(dir_left_);
files[_right_] = getFiles(dir_right);

vector<string> files[2]{      //good, with calling "GetFileAttributes" on each ent->fd_name
    DirectoryReader_dirent(dir_left_).listFiles(),
    DirectoryReader_dirent(dir_right).listFiles(),
};

vector<string> files[2]{      //good
    DirectoryReader_winAPI(dir_left_.string(), default_file_ext).listFiles(),
    DirectoryReader_winAPI(dir_right.string(), default_file_ext).listFiles(),
};

#endif


#if 0

/*!
 * @brief get ".log", from ".log" "log"
 * @param s
 * @return the extension without '.'
 */
std::string parseFileExtInput(std::string &s);


string parseFileExtInput(string &s)
{
  std::string sep = ".";
  size_t found = s.find_last_of(sep);
  if (std::string::npos == found) { //not found
    return '.' + s;
  }
  else { //found '.'
    return s.substr(found); //from "sep" to string-end, [sep, end)
  }
}
#endif