#include <regex>
#include <sstream>

std::vector<std::string> StringUtils::split(
    const std::string &str,
    const std::vector<std::string> &delimiters)
{

  std::regex rgx(join(escapeStrings(delimiters), "|"));

  std::sregex_token_iterator
      first{begin(str), end(str), rgx, -1},
      last;

  return {first, last};
}

std::vector<std::string> StringUtils::split(const std::string &str,
                                            const std::string &delimiter)
{
  std::vector<std::string> delimiters = {delimiter};
  return split(str, delimiters);
}

std::string join(
    const std::vector<std::string> &tokens,
    const std::string &delimiter)
{

  std::stringstream stream;

  stream << tokens.front();

  std::for_each(
      begin(tokens) + 1,
      end(tokens),
      [&](const std::string &elem) {
        stream << delimiter << elem;
      }
  );

  return stream.str();
}