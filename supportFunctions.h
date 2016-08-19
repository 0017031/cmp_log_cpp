//
// Created by baic on 2016-8-22.
//

#ifndef C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H
#define C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H

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
#endif //# my_small_functions

#endif //C_HASH_CMP_LOG_SUPPORTFUNCTIONS_H
