//
// Created by baic on 2016-8-22.
//

#ifndef C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H
#define C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H

#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define my_small_functions
#ifdef my_small_functions

/*! get the file name (last) in the std::string. (f.txt in "c:\f.txt")
 *
 * @param s [in] input std::string
 * @return file namle (last) in the std::string
 */
std::string getFileName(const std::string &s);

/*! get the base name. ( ffff in ffff.txt)
 *
 * @param s [in] input std::string
 * @return base name
 */
std::string getBaseName(const std::string &s);

/*! strip the trailing /r/n of a std::string
 *
 * @param s [in] input std::string
 * @return the result
 */
std::string stripCRLF(const std::string &s);

/*! Check if a file exists.
 *
 * @param [in] fileName
 * @return True if it exists.
 */
bool FileExists(const std::string &fileName);

/*! Check if a file can be read
 *
 * @param [in] fileName
 * @return True if it can be read.
 */
bool FileCanBeRead(const std::string &fileName);

/*! check if it is a directory
 *
 * @param [in] theName
 * @return True if it is an directory
 */
bool IsDir(const std::string &theName);

/*!
 * @brief list all the files in dir
 * @param folder [in] the directory
 * @return  the file names stored in a vector
 */
std::vector<std::string> getFiles(std::string folder);

/*!
 * @brief exception for list-directory
 */
class DirectoryException : public std::logic_error {
public:
  explicit DirectoryException(const std::string &what_arg)
      : std::logic_error(what_arg) {}
};

/*!
 * @brief list directory with dirent.h
 */
#include <dirent.h>
#include <iomanip>

class DirectoryReader_dirent {
public:
  /*!
   * @brief constructor, explicit a input path. Open the dir, then store the handle to myDir
   * @param path
   */
  explicit DirectoryReader_dirent(std::string path) : myDir(opendir(path.c_str()))
  {
    if (NULL == myDir)
    {
      std::string err{"Can't open directory " + path};
      throw DirectoryException(err);
    }
  }

/*!
 * @brief destructor, close myDir
 */
  ~DirectoryReader_dirent() { closedir(myDir); }

  /*!
   * @brief list all the files under the dir
   * @return the file-names
   */
  std::vector<std::string> listFiles() const
  {
    std::vector<std::string> files;
    struct dirent *ent;
    while (NULL != (ent = readdir(myDir)))
    {
#ifdef  _DIRENT_HAVE_D_TYPE
      if (ent->d_type == DT_REG) // no ent->d_type under mingwin-w64. So "lstat()"
#else
      /*
      std::cout << ent->d_name << " 0x" << std::setfill('0') << std::setw(2) << std::hex
                << GetFileAttributes(ent->d_name) << std::endl;
      */
      //https://sourceforge.net/p/mingw-w64/mailman/message/25812209/
      if (GetFileAttributes(ent->d_name) != FILE_ATTRIBUTE_DIRECTORY)
#endif
      {
        files.push_back(ent->d_name);
      }
    }
    return files;
  }

private:
  DIR *myDir;
};

/*!
 * @brief list directory with winAPI: FindFirstFile, FindNextFile
 */
#include <Windows.h>

class DirectoryReader_winAPI {
public:
  /*!
   * @brief constructor, explicit a input path. FindFirstFile(), then store the handle to myHandle
   * @param folder
   * @param file_ext [in] Which extension of files to search. For example "*.log" for log files
   */
  explicit DirectoryReader_winAPI(const std::string folder, const std::string file_ext)
      : myHandle{FindFirstFile((folder + '/' + file_ext).c_str(), &fd)}
  {
    if (INVALID_HANDLE_VALUE == myHandle)
    {
      std::string err{"Can't open directory " + folder + ", error:" + std::to_string(GetLastError())};
      throw DirectoryException(err);
    }
  }

/*!
 * @brief destructor, close myHandle
 */
  ~DirectoryReader_winAPI()
  {
    if (INVALID_HANDLE_VALUE != myHandle)
    {
      ::FindClose(myHandle);
    }
  }

  /*!
   * @brief list all the files under the dir
   * @return the file-names
   */
  std::vector<std::string> listFiles()
  {
    std::vector<std::string> files;
    if (INVALID_HANDLE_VALUE != myHandle)
    {
      do
      {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        { ; //if dir, do nothing.
        }
        else
        {
          files.push_back(fd.cFileName);
        }
      } while (::FindNextFile(myHandle, &fd));
    }
    return files;
  }

private:
  WIN32_FIND_DATA fd; ///< receives information about a found file or directory
  HANDLE myHandle; ///< a search handle used in a subsequent call to FindNextFile or FindClose

};

#endif //# my_small_functions

#endif //C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H
