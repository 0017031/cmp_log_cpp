//
// Created by baic on 2016-8-15.
//

#ifndef C_HASH_CMP_LOG_INPUTPARSER_H
#define C_HASH_CMP_LOG_INPUTPARSER_H

class InputParser
{
 public:
  InputParser(int &argc, char **argv)
  {
      for (int i = 1; i < argc; ++i)
          this->tokens.push_back(std::string(argv[i]));
  }

  /*! find an option in argv, return the next string as its value
   *
   * @param option [in] the option to find (e.g. "-h" "-L" "-O")
   * @return the string, if it exist. otherwise, an empty string "".
   */
  const std::string getCmdOption(const std::string &option) const
  {
      std::vector<std::string>::const_iterator itr;
      itr = std::find(this->tokens.begin(), this->tokens.end(), option);
      if (itr != this->tokens.end() && ++itr != this->tokens.end())
      {
          return *itr;
      }
      return "";
  }

  /*! Check if an option exist in argv
   *
   * @param option [in] the option to check
   * @return true/false
   */
  bool cmdOptionExists(const std::string &option) const
  {
      return std::find(this->tokens.begin(), this->tokens.end(), option)
          != this->tokens.end();
  }

  /*! pop an option and its value from argv/"tokens"
   *
   * @param option [in] the option to check
   * @return value of the option
   * @sideeffect both the "option" and its "value" are removed from argv/"tokens"
   */
  std::string popOption(const std::string &option)
  {
      if (cmdOptionExists(option))
      {
          std::string optValue = getCmdOption(option) ;
          tokens.erase(std::remove(tokens.begin(), tokens.end(), option), tokens.end());
          tokens.erase(std::remove(tokens.begin(), tokens.end(), optValue), tokens.end());
      }
      return "";
  }

 private:
  std::vector<std::string> tokens;
};

#endif //C_HASH_CMP_LOG_INPUTPARSER_H
