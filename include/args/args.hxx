/* Copyright (c) 2016 Taylor C. Richberger <taywee@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef ARGS_HXX
#define ARGS_HXX


/** \file args.hxx
 * \brief this single-header lets you use all of the args functionality
 *
 * The important stuff is done inside the args namespace
 */

#include <algorithm>
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>

#ifdef ARGS_TESTNAMESPACE
namespace argstest
{
#else

/** \namespace args
 * \brief contains all the functionality of the args library
 */
namespace args {
#endif

using ParameterDesc = std::tuple<std::string, std::string, unsigned int>;

/** Getter to grab the value from the argument type.
 *
 * If the Get() function of the type returns a reference, so does this, and
 * the value will be modifiable.
 */
template<typename Option>
auto get(Option &option) -> decltype(option.Get()) {
  return option.Get();
}

/** (INTERNAL) Count UTF-8 glyphs
 *
 * This is not reliable, and will fail for combinatory glyphs, but it's
 * good enough here for now.
 *
 * \param string The string to count glyphs from
 * \return The UTF-8 glyphs in the string
 */
std::string::size_type Glyphs(const std::string &string) {
  std::string::size_type length = 0;
  for (const char c: string) {
    if ((c & 0xc0) != 0x80) {
      ++length;
    }
  }
  return length;
}

/** (INTERNAL) Wrap a string into a vector of lines
 *
 * This is quick and hacky, but works well enough.  You can specify a
 * different width for the first line
 *
 * \param width The width of the body
 * \param firstlinewidth the widtho f the first line, defaults to the width of the body
 * \return the vector of lines
 */
std::vector<std::string>
Wrap(const std::string &in,
     const std::string::size_type width,
     std::string::size_type firstlinewidth = 0) {
  // Preserve existing line breaks
  const auto newlineloc = in.find('\n');
  if (newlineloc != in.npos) {
    auto first = Wrap(std::string(in, 0, newlineloc), width);
    auto second = Wrap(std::string(in, newlineloc + 1), width);
    first.insert(
        std::end(first),
        std::make_move_iterator(std::begin(second)),
        std::make_move_iterator(std::end(second)));
    return first;
  }
  if (firstlinewidth == 0) {
    firstlinewidth = width;
  }
  auto currentwidth = firstlinewidth;

  std::istringstream stream(in);
  std::vector<std::string> output;
  std::ostringstream line;
  std::string::size_type linesize = 0;
  while (stream) {
    std::string item;
    stream >> item;
    auto itemsize = Glyphs(item);
    if ((linesize + 1 + itemsize) > currentwidth) {
      if (linesize > 0) {
        output.push_back(line.str());
        line.str(std::string());
        linesize = 0;
        currentwidth = width;
      }
    }
    if (itemsize > 0) {
      if (linesize) {
        ++linesize;
        line << " ";
      }
      line << item;
      linesize += itemsize;
    }
  }
  if (linesize > 0) {
    output.push_back(line.str());
  }
  return output;
}

#ifdef ARGS_NOEXCEPT
/// Error class, for when ARGS_NOEXCEPT is defined
enum class Error
{
    None,
    Usage,
    Parse,
    Validation,
    Map,
    Extra,
    Help
};
#else

/** Base error class
 */
class Error : public std::runtime_error {
public:
  Error(const std::string &problem) : std::runtime_error(problem) {}

  virtual ~Error() {};
};

/** Errors that occur during usage
 */
class UsageError : public Error {
public:
  UsageError(const std::string &problem) : Error(problem) {}

  virtual ~UsageError() {};
};

/** Errors that occur during regular parsing
 */
class ParseError : public Error {
public:
  ParseError(const std::string &problem) : Error(problem) {}

  virtual ~ParseError() {};
};

/** Errors that are detected from group validation after parsing finishes
 */
class ValidationError : public Error {
public:
  ValidationError(const std::string &problem) : Error(problem) {}

  virtual ~ValidationError() {};
};

/** Errors in map lookups
 */
class MapError : public ParseError {
public:
  MapError(const std::string &problem) : ParseError(problem) {}

  virtual ~MapError() {};
};

/** Error that occurs when a singular flag is specified multiple times
 */
class ExtraError : public ParseError {
public:
  ExtraError(const std::string &problem) : ParseError(problem) {}

  virtual ~ExtraError() {};
};

/** An exception that indicates that the user has requested help
 */
class Help : public Error {
public:
  Help(const std::string &flag) : Error(flag) {}

  virtual ~Help() {};
};

#endif

/** A simple unified option type for unified initializer lists for the Matcher class.
 */
struct EitherFlag {
  const bool isShort;
  const char shortFlag;
  const std::string longFlag;

  EitherFlag(const std::string &flag) : isShort(false), shortFlag(), longFlag(flag) {}

  EitherFlag(const char *flag) : isShort(false), shortFlag(), longFlag(flag) {}

  EitherFlag(const char flag) : isShort(true), shortFlag(flag), longFlag() {}

  /** Get just the long flags from an initializer list of EitherFlags
   */
  static std::unordered_set<std::string> GetLong(std::initializer_list<EitherFlag> flags) {
    std::unordered_set<std::string> longFlags;
    for (const EitherFlag &flag: flags) {
      if (!flag.isShort) {
        longFlags.insert(flag.longFlag);
      }
    }
    return longFlags;
  }

  /** Get just the short flags from an initializer list of EitherFlags
   */
  static std::unordered_set<char> GetShort(std::initializer_list<EitherFlag> flags) {
    std::unordered_set<char> shortFlags;
    for (const EitherFlag &flag: flags) {
      if (flag.isShort) {
        shortFlags.insert(flag.shortFlag);
      }
    }
    return shortFlags;
  }
};

/** A class of "matchers", specifying short and flags that can possibly be
 * matched.
 *
 * This is supposed to be constructed and then passed in, not used directly
 * from user code.
 */
class Matcher {
private:
  const std::unordered_set<char> shortFlags;
  const std::unordered_set<std::string> longFlags;

public:
  /** Specify short and long flags separately as iterators
   *
   * ex: `args::Matcher(shortFlags.begin(), shortFlags.end(), longFlags.begin(), longFlags.end())`
   */
  template<typename ShortIt, typename LongIt>
  Matcher(ShortIt shortFlagsStart,
          ShortIt shortFlagsEnd,
          LongIt longFlagsStart,
          LongIt longFlagsEnd) :
      shortFlags(shortFlagsStart, shortFlagsEnd),
      longFlags(longFlagsStart, longFlagsEnd) {}

  /** Specify short and long flags separately as iterables
   *
   * ex: `args::Matcher(shortFlags, longFlags)`
   */
  template<typename Short, typename Long>
  Matcher(Short &&shortIn,
          Long &&longIn) :
      shortFlags(std::begin(shortIn), std::end(shortIn)), longFlags(std::begin(longIn), std::end(longIn)) {}

  /** Specify a mixed single initializer-list of both short and long flags
   *
   * This is the fancy one.  It takes a single initializer list of
   * any number of any mixed kinds of flags.  Chars are
   * automatically interpreted as short flags, and strings are
   * automatically interpreted as long flags:
   *
   *     args::Matcher{'a'}
   *     args::Matcher{"foo"}
   *     args::Matcher{'h', "help"}
   *     args::Matcher{"foo", 'f', 'F', "FoO"}
   */
  Matcher(std::initializer_list<EitherFlag> in) :
      shortFlags(EitherFlag::GetShort(in)), longFlags(EitherFlag::GetLong(in)) {}

  Matcher(Matcher &&other) : shortFlags(std::move(other.shortFlags)), longFlags(std::move(other.longFlags)) {}

  ~Matcher() {}

  /** (INTERNAL) Check if there is a match of a short flag
   */
  bool Match(const char flag) const {
    return shortFlags.find(flag) != shortFlags.end();
  }

  /** (INTERNAL) Check if there is a match of a long flag
   */
  bool Match(const std::string &flag) const {
    return longFlags.find(flag) != longFlags.end();
  }

  /** (INTERNAL) Get all flag strings as a vector, with the prefixes embedded
   */
  std::vector<std::string> GetFlagStrings(const std::string &shortPrefix,
                                          const std::string &longPrefix) const {
    std::vector<std::string> flagStrings;
    flagStrings.reserve(shortFlags.size() + longFlags.size());
    for (const char flag: shortFlags) {
      flagStrings.emplace_back(shortPrefix + std::string(1, flag));
    }
    for (const std::string &flag: longFlags) {
      flagStrings.emplace_back(longPrefix + flag);
    }
    return flagStrings;
  }

  /** (INTERNAL) Get all flag strings as a vector, with the prefixes and names embedded
   */
  std::vector<std::string>
  GetFlagStrings(const std::string &shortPrefix,
                 const std::string &longPrefix,
                 const std::string &name,
                 const std::string &shortSeparator,
                 const std::string longSeparator) const {
    const std::string bracedname(name);
    std::vector<std::string> flagStrings;
    flagStrings.reserve(shortFlags.size() + longFlags.size());
    for (const char flag: shortFlags) {
      flagStrings.emplace_back(shortPrefix + std::string(1, flag) + shortSeparator + ' ' + bracedname);
    }
    for (const std::string &flag: longFlags) {
      flagStrings.emplace_back(longPrefix + flag + longSeparator + bracedname);
    }
    return flagStrings;
  }
};

/** Base class for all match types
 */
class Base {
protected:
  bool matched;
  const std::string help;
#ifdef ARGS_NOEXCEPT
  /// Only for ARGS_NOEXCEPT
  Error error;
#endif

public:
  Base(const std::string &help) : matched(false), help(help) {}

  virtual ~Base() {}

  virtual bool Matched() const noexcept {
    return matched;
  }

  operator bool() const noexcept {
    return Matched();
  }

  virtual std::tuple<std::string, std::string>
  GetDescription(const std::string &shortPrefix,
                 const std::string &longPrefix,
                 const std::string &shortSeparator,
                 const std::string &longSeparator) const {
    std::tuple<std::string, std::string> description;
    std::get<1>(description) = help;
    return description;
  }

  virtual void Reset() noexcept {
    matched = false;
#ifdef ARGS_NOEXCEPT
    error = Error::None;
#endif
  }

#ifdef ARGS_NOEXCEPT
  /// Only for ARGS_NOEXCEPT
  virtual Error GetError() const
  {
      return error;
  }
#endif
};

/** Base class for all match types that have a name
 */
class NamedBase : public Base {
protected:
  const std::string name;
  bool kickout;

public:
  NamedBase(const std::string &name,
            const std::string &help) : Base(help), name(name), kickout(false) {}

  virtual ~NamedBase() {}

  virtual std::tuple<std::string, std::string>
  GetDescription(const std::string &shortPrefix,
                 const std::string &longPrefi,
                 const std::string &shortSeparator,
                 const std::string &longSeparator) const override {
    std::tuple<std::string, std::string> description;
    std::get<0>(description) = Name();
    std::get<1>(description) = help;
    return description;
  }

  virtual std::string Name() const {
    return name;
  }

  /// Sets a kick-out value for building subparsers
  void KickOut(bool kickout) noexcept {
    this->kickout = kickout;
  }

  /// Gets the kick-out value for building subparsers
  bool KickOut() const noexcept {
    return kickout;
  }
};

/** Base class for all flag options
 */
class FlagBase : public NamedBase {
private:
  const bool extraError;

protected:
  const Matcher matcher;

public:
  FlagBase(const std::string &name,
           const std::string &help,
           Matcher &&matcher,
           const bool extraError = false)
      : NamedBase(name, help), extraError(extraError), matcher(std::move(matcher)) {}

  virtual ~FlagBase() {}

  virtual FlagBase *Match(const std::string &flag) {
    if (matcher.Match(flag)) {
      if (extraError && matched) {
#ifdef ARGS_NOEXCEPT
        error = Error::Extra;
#else
        std::ostringstream problem;
        problem << "Flag '" << flag << "' was passed multiple times, but is only allowed to be passed once";
        throw ExtraError(problem.str());
#endif
      }
      matched = true;
      return this;
    }
    return nullptr;
  }

  virtual FlagBase *Match(const char flag) {
    if (matcher.Match(flag)) {
      if (extraError && matched) {
#ifdef ARGS_NOEXCEPT
        error = Error::Extra;
#else
        std::ostringstream problem;
        problem << "Flag '" << flag << "' was passed multiple times, but is only allowed to be passed once";
        throw ExtraError(problem.str());
#endif
      }
      matched = true;
      return this;
    }
    return nullptr;
  }

  virtual std::tuple<std::string, std::string>
  GetDescription(const std::string &shortPrefix,
                 const std::string &longPrefix,
                 const std::string &shortSeparator,
                 const std::string &longSeparator) const override {
    std::tuple<std::string, std::string> description;
    const auto flagStrings = matcher.GetFlagStrings(shortPrefix, longPrefix);
    std::ostringstream flagstream;
    for (auto it = std::begin(flagStrings); it != std::end(flagStrings); ++it) {
      if (it != std::begin(flagStrings)) {
        flagstream << ", ";
      }
      flagstream << *it;
    }
    std::get<0>(description) = flagstream.str();
    std::get<1>(description) = help;
    return description;
  }
};

/** Base class for value-accepting flag options
 */
class ValueFlagBase : public FlagBase {
public:
  ValueFlagBase(const std::string &name,
                const std::string &help,
                Matcher &&matcher,
                const bool extraError = false) : FlagBase(name, help, std::move(matcher), extraError) {}

  virtual ~ValueFlagBase() {}

  virtual void ParseValue(const std::string &value) = 0;

  virtual std::tuple<std::string, std::string>
  GetDescription(const std::string &shortPrefix,
                 const std::string &longPrefix,
                 const std::string &shortSeparator,
                 const std::string &longSeparator) const override {
    std::tuple<std::string, std::string> description;
    const auto flagStrings = matcher.GetFlagStrings(shortPrefix, longPrefix, Name(), shortSeparator,
                                                    longSeparator);
    std::ostringstream flagstream;
    for (auto it = std::begin(flagStrings); it != std::end(flagStrings); ++it) {
      if (it != std::begin(flagStrings)) {
        flagstream << ", ";
      }
      flagstream << *it;
    }
    std::get<0>(description) = flagstream.str();
    std::get<1>(description) = help;
    return description;
  }
};

/** Base class for positional options
 */
class PositionalBase : public NamedBase {
protected:
  bool ready;

public:
  PositionalBase(const std::string &name,
                 const std::string &help) : NamedBase(name, help), ready(true) {}

  virtual ~PositionalBase() {}

  bool Ready() {
    return ready;
  }

  virtual void ParseValue(const std::string &value) = 0;

  virtual void Reset() noexcept override {
    matched = false;
    ready = true;
#ifdef ARGS_NOEXCEPT
    error = Error::None;
#endif
  }
};

/** Class for all kinds of validating groups, including ArgumentParser
 */
class Group : public Base {
private:
  std::vector<Base *> children;
  std::function<bool(const Group &)> validator;

public:
  /** Default validators
   */
  struct Validators {
    static bool Xor(const Group &group) {
      return group.MatchedChildren() == 1;
    }

    static bool AtLeastOne(const Group &group) {
      return group.MatchedChildren() >= 1;
    }

    static bool AtMostOne(const Group &group) {
      return group.MatchedChildren() <= 1;
    }

    static bool All(const Group &group) {
      return group.Children().size() == group.MatchedChildren();
    }

    static bool AllOrNone(const Group &group) {
      return (All(group) || None(group));
    }

    static bool AllChildGroups(const Group &group) {
      return std::find_if(std::begin(group.Children()), std::end(group.Children()),
                          [](const Base *child) -> bool {
                            return dynamic_cast<const Group *>(child) && !child->Matched();
                          }) == std::end(group.Children());
    }

    static bool DontCare(const Group &group) {
      return true;
    }

    static bool CareTooMuch(const Group &group) {
      return false;
    }

    static bool None(const Group &group) {
      return group.MatchedChildren() == 0;
    }
  };

  /// If help is empty, this group will not be printed in help output
  Group(const std::string &help = std::string(),
        const std::function<bool(const Group &)> &validator = Validators::DontCare) : Base(help),
                                                                                      validator(validator) {}

  /// If help is empty, this group will not be printed in help output
  Group(Group &group,
        const std::string &help = std::string(),
        const std::function<bool(const Group &)> &validator = Validators::DontCare) : Base(help),
                                                                                      validator(validator) {
    group.Add(*this);
  }

  virtual ~Group() {}

  /** Return the first FlagBase that matches flag, or nullptr
   *
   * \param flag The flag with prefixes stripped
   * \return the first matching FlagBase pointer, or nullptr if there is no match
   */
  template<typename T>
  FlagBase *Match(const T &flag) {
    for (Base *child: children) {
      if (FlagBase *flagBase = dynamic_cast<FlagBase *>(child)) {
        if (FlagBase *match = flagBase->Match(flag)) {
          return match;
        }
      } else if (Group *group = dynamic_cast<Group *>(child)) {
        if (FlagBase *match = group->Match(flag)) {
          return match;
        }
      }
    }
    return nullptr;
  }

  /** Get the next ready positional, or nullptr if there is none
   *
   * \return the first ready PositionalBase pointer, or nullptr if there is no match
   */
  PositionalBase *GetNextPositional() {
    for (Base *child: children) {
      auto next = dynamic_cast<PositionalBase *>(child);
      auto group = dynamic_cast<Group *>(child);
      if (group) {
        next = group->GetNextPositional();
      }
      if (next && next->Ready()) {
        return next;
      }
    }
    return nullptr;
  }

  /** Get whether this has any FlagBase children
   *
   * \return Whether or not there are any FlagBase children
   */
  bool HasFlag() const {
    for (Base *child: children) {
      if (dynamic_cast<FlagBase *>(child)) {
        return true;
      }
      if (auto group = dynamic_cast<Group *>(child)) {
        if (group->HasFlag()) {
          return true;
        }
      }
    }
    return false;
  }

  /** Append a child to this Group.
   */
  void Add(Base &child) {
    children.emplace_back(&child);
  }

  /** Get all this group's children
   */
  const std::vector<Base *> &Children() const {
    return children;
  }

  /** Count the number of matched children this group has
   */
  unsigned MatchedChildren() const {
    return
        static_cast<unsigned >(
          std::count_if(
              std::begin(children),
              std::end(children),
             [](const Base *child) { return child->Matched(); })
        );
  }



  /** Whether or not this group matches validation
   */
  virtual bool Matched() const noexcept override {
    return validator(*this);
  }

  /** Get validation
   */
  bool Get() const {
    return Matched();
  }

  /** Get all the child Positional-Parameter for help generation
   */
  std::vector<ParameterDesc> GetPositionalParameterDescriptions
      (const std::string &shortPrefix,
       const std::string &longPrefix,
       const std::string &shortSeparator,
       const std::string &longSeparator,
       const unsigned int indent = 0) const {
    std::vector<ParameterDesc> descriptions;
    for (const auto &child: children) {
      const auto group = dynamic_cast<Group *>(child);
      const auto positional = dynamic_cast<PositionalBase *>(child);
      if (group) {
        // Push that group description on the back if not empty
        unsigned char addindent = 0;
        if (!group->help.empty()) {
          descriptions.emplace_back(group->help, "", indent);
          addindent = 1;
        }
        auto groupDescriptions = group->GetPositionalParameterDescriptions(
            shortPrefix, longPrefix, shortSeparator, longSeparator, indent + addindent);
        descriptions.insert(
            std::end(descriptions),
            std::make_move_iterator(std::begin(groupDescriptions)),
            std::make_move_iterator(std::end(groupDescriptions)));
      } else if (positional) {
        const auto
            description = positional->GetDescription(shortPrefix, longPrefix, shortSeparator,
                                                     longSeparator);
        descriptions.emplace_back(std::get<0>(description), std::get<1>(description), indent);
      }
    }
    return descriptions;
  }

  /** Get all the child Flag-Parameter for help generation
   */
  std::vector<ParameterDesc> GetFlagParameterDescriptions
      (const std::string &shortPrefix,
       const std::string &longPrefix,
       const std::string &shortSeparator,
       const std::string &longSeparator,
       const unsigned int indent = 0) const {
    std::vector<ParameterDesc> descriptions;
    for (const auto &child: children) {
      const auto group = dynamic_cast<Group *>(child);
      const auto flag = dynamic_cast<FlagBase *>(child);
      if (group) {
        // Push that group description on the back if not empty
        unsigned char addindent = 0;
        if (!group->help.empty()) {
          descriptions.emplace_back(group->help, "", indent);
          addindent = 1;
        }
        auto groupDescriptions = group->GetFlagParameterDescriptions(
            shortPrefix, longPrefix, shortSeparator, longSeparator, indent + addindent);
        descriptions.insert(
            std::end(descriptions),
            std::make_move_iterator(std::begin(groupDescriptions)),
            std::make_move_iterator(std::end(groupDescriptions)));
      } else if (flag) {
        const auto
            description = flag->GetDescription(shortPrefix, longPrefix, shortSeparator, longSeparator);
        descriptions.emplace_back(std::get<0>(description), std::get<1>(description), indent);
      }
    }
    return descriptions;
  }

  /** Get all the child descriptions for help generation
   */
  std::vector<ParameterDesc> GetChildDescriptions(
      const std::string &shortPrefix,
      const std::string &longPrefix,
      const std::string &shortSeparator,
      const std::string &longSeparator,
      const unsigned int indent = 0) const {
    std::vector<ParameterDesc> descriptions;
    for (const auto &child: children) {
      if (const auto group = dynamic_cast<Group *>(child)) {
        // Push that group description on the back if not empty
        unsigned char addindent = 0;
        if (!group->help.empty()) {
          descriptions.emplace_back(group->help, "", indent);
          addindent = 1;
        }
        auto groupDescriptions = group->GetChildDescriptions(shortPrefix, longPrefix, shortSeparator,
                                                             longSeparator, indent + addindent);
        descriptions.insert(
            std::end(descriptions),
            std::make_move_iterator(std::begin(groupDescriptions)),
            std::make_move_iterator(std::end(groupDescriptions)));
      } else if (const auto named = dynamic_cast<NamedBase *>(child)) {
        const auto description = named->GetDescription(shortPrefix, longPrefix, shortSeparator,
                                                       longSeparator);
        descriptions.emplace_back(std::get<0>(description), std::get<1>(description), indent);
      }
    }
    return descriptions;
  }

  /** Get the names of positional parameters
   */
  std::vector<std::string> GetPosNames() const {
    std::vector<std::string> names;
    for (const auto &child: children) {
      if (const Group *group = dynamic_cast<Group *>(child)) {
        auto groupNames = group->GetPosNames();
        names.insert(
            std::end(names),
            std::make_move_iterator(std::begin(groupNames)),
            std::make_move_iterator(std::end(groupNames)));
      } else if (const PositionalBase *pos = dynamic_cast<PositionalBase *>(child)) {
        names.emplace_back(pos->Name());
      }
    }
    return names;
  }

  virtual void Reset() noexcept override {
    for (auto &child: children) {
      child->Reset();
    }
#ifdef ARGS_NOEXCEPT
    error = Error::None;
#endif
  }

#ifdef ARGS_NOEXCEPT
  /// Only for ARGS_NOEXCEPT
  virtual Error GetError() const override
  {
      if (error != Error::None)
      {
          return error;
      }

      auto it = std::find_if(std::begin(children), std::end(children), [](const Base *child){return child->GetError() != Error::None;});
      if (it == std::end(children))
      {
          return Error::None;
      } else
      {
          return (*it)->GetError();
      }
  }
#endif

};

/** The main user facing command line argument parser class
 */
class ArgumentParser : public Group {
private:
  std::string prog;
  std::string proglinePostfix;
  std::string description;
  std::string epilog;

  std::string longprefix;
  std::string shortprefix;

  std::string longseparator;

  std::string terminator;

  bool allowJoinedShortValue;
  bool allowJoinedLongValue;
  bool allowSeparateShortValue;
  bool allowSeparateLongValue;

  void getHelpInfoFromDesc(std::ostream &help,
                           const std::vector<ParameterDesc> &descriptions) const {
    for (const auto descript: descriptions) {
      const auto groupindent = get<2>(descript) * helpParams.eachgroupindent;
      const auto flags = Wrap(
          get<0>(descript),
          helpParams.width - (helpParams.flagindent + helpParams.helpindent + helpParams.gutter));
      const auto info = Wrap(
          get<1>(descript),
          helpParams.width - (helpParams.helpindent + groupindent));

      std::string::size_type flagssize = 0;
      for (auto flagsit = begin(flags); flagsit != end(flags); ++flagsit) {
        if (flagsit != begin(flags)) {
          help << '\n';
        }
        help << std::string(groupindent + helpParams.flagindent, ' ') << *flagsit;
        flagssize = Glyphs(*flagsit);
      }

      auto infoit = begin(info);
      // groupindent is on both sides of this inequality, and therefore can be removed
      if ((helpParams.flagindent + flagssize + helpParams.gutter) > helpParams.helpindent
          || infoit == end(info)) {
        help << '\n';
      } else {
        // groupindent is on both sides of the minus sign, and therefore doesn't actually need to be in here
        help << std::string(helpParams.helpindent - (helpParams.flagindent + flagssize), ' ')
             << *infoit
             << '\n';
        ++infoit;
      }
      for (; infoit != end(info); ++infoit) {
        help << std::string(groupindent + helpParams.helpindent, ' ') << *infoit << '\n';
      }
    }
    return;
  }

public:
  /** A simple structure of parameters for easy user-modifyable help menus
   */
  struct HelpParams {
    /** The width of the help menu
     */
    unsigned int width = 80;
    /** The indent of the program line
     */
    unsigned int progindent = 2;
    /** The indent of the program trailing lines for long parameters
     */
    unsigned int progtailindent = 4;
    /** The indent of the description and epilogs
     */
    unsigned int descriptionindent = 4;
    /** The indent of the flags
     */
    unsigned int flagindent = 6;
    /** The indent of the flag descriptions
     */
    unsigned int helpindent = 40;
    /** The additional indent each group adds
     */
    unsigned int eachgroupindent = 2;

    /** The minimum gutter between each flag and its help
     */
    unsigned int gutter = 1;

    /** Show the terminator when both options and positional parameters are present
     */
    bool showTerminator = true;

    /** Show the [OPTIONS] on the prog line when this is true
     */
    bool showProglineOptions = true;

    /** Show the positionals on the prog line when this is true
     */
    bool showProglinePositionals = true;
  } helpParams;

  ArgumentParser(const std::string &description,
                 const std::string &epilog = std::string()) :
      Group("", Group::Validators::AllChildGroups),
      description(description),
      epilog(epilog),
      longprefix("--"),
      shortprefix("-"),
      longseparator("="),
      terminator("--"),
      allowJoinedShortValue(true),
      allowJoinedLongValue(true),
      allowSeparateShortValue(true),
      allowSeparateLongValue(true) {}

  /** The program name for help generation
   */
  const std::string &Prog() const { return prog; }

  /** The program name for help generation
   */
  void Prog(const std::string &prog) { this->prog = prog; }

  /** The description that appears on the prog line after options
   */
  const std::string &ProglinePostfix() const { return proglinePostfix; }

  /** The description that appears on the prog line after options
   */
  void ProglinePostfix(const std::string &proglinePostfix) { this->proglinePostfix = proglinePostfix; }

  /** The description that appears above options
   */
  const std::string &Description() const { return description; }

  /** The description that appears above options
   */
  void Description(const std::string &description) { this->description = description; }

  /** The description that appears below options
   */
  const std::string &Epilog() const { return epilog; }

  /** The description that appears below options
   */
  void Epilog(const std::string &epilog) { this->epilog = epilog; }

  /** The prefix for long flags
   */
  const std::string &LongPrefix() const { return longprefix; }

  /** The prefix for long flags
   */
  void LongPrefix(const std::string &longprefix) { this->longprefix = longprefix; }

  /** The prefix for short flags
   */
  const std::string &ShortPrefix() const { return shortprefix; }

  /** The prefix for short flags
   */
  void ShortPrefix(const std::string &shortprefix) { this->shortprefix = shortprefix; }

  /** The separator for long flags
   */
  const std::string &LongSeparator() const { return longseparator; }

  /** The separator for long flags
   */
  void LongSeparator(const std::string &longseparator) {
    if (longseparator.empty()) {
#ifdef ARGS_NOEXCEPT
      error = Error::Usage;
#else
      throw UsageError("longseparator can not be set to empty");
#endif
    } else {
      this->longseparator = longseparator;
    }
  }

  /** The terminator that forcibly separates flags from positionals
   */
  const std::string &Terminator() const { return terminator; }

  /** The terminator that forcibly separates flags from positionals
   */
  void Terminator(const std::string &terminator) { this->terminator = terminator; }

  /** Get the current argument separation parameters.
   *
   * See SetArgumentSeparations for details on what each one means.
   */
  void GetArgumentSeparations(
      bool &allowJoinedShortValue,
      bool &allowJoinedLongValue,
      bool &allowSeparateShortValue,
      bool &allowSeparateLongValue) const {
    allowJoinedShortValue = this->allowJoinedShortValue;
    allowJoinedLongValue = this->allowJoinedLongValue;
    allowSeparateShortValue = this->allowSeparateShortValue;
    allowSeparateLongValue = this->allowSeparateLongValue;
  }

  /** Change allowed option separation.
   *
   * \param allowJoinedShortValue Allow a short flag that accepts an argument to be passed its argument immediately next to it (ie. in the same argv field)
   * \param allowJoinedLongValue Allow a long flag that accepts an argument to be passed its argument separated by the longseparator (ie. in the same argv field)
   * \param allowSeparateShortValue Allow a short flag that accepts an argument to be passed its argument separated by whitespace (ie. in the next argv field)
   * \param allowSeparateLongValue Allow a long flag that accepts an argument to be passed its argument separated by whitespace (ie. in the next argv field)
   */
  void SetArgumentSeparations(
      const bool allowJoinedShortValue,
      const bool allowJoinedLongValue,
      const bool allowSeparateShortValue,
      const bool allowSeparateLongValue) {
    this->allowJoinedShortValue = allowJoinedShortValue;
    this->allowJoinedLongValue = allowJoinedLongValue;
    this->allowSeparateShortValue = allowSeparateShortValue;
    this->allowSeparateLongValue = allowSeparateLongValue;
  }

  /** Pass the help menu into an ostream
   */
  void Help(std::ostream &help) const {
    bool hasoptions = false;
    bool hasarguments = false;

    const auto description = Wrap(this->description, helpParams.width - helpParams.descriptionindent);
    const auto epilog = Wrap(this->epilog, helpParams.width - helpParams.descriptionindent);
    std::ostringstream prognameline;
    prognameline << prog;
    if (HasFlag()) {
      hasoptions = true;
      if (helpParams.showProglineOptions) {
        prognameline << " [OPTIONS]";
      }
    }
    for (const std::string &posname: GetPosNames()) {
      hasarguments = true;
      if (helpParams.showProglinePositionals) {
        prognameline << " " << posname;
      }
    }
    if (!proglinePostfix.empty()) {
      prognameline << ' ' << proglinePostfix;
    }
    const auto proglines = Wrap(prognameline.str(), helpParams.width - (helpParams.progindent + 4),
                                helpParams.width - helpParams.progindent);
    auto progit = std::begin(proglines);
    if (progit != std::end(proglines)) {
      help << std::string(helpParams.progindent, ' ') << *progit << '\n';
      ++progit;
    }
    for (; progit != std::end(proglines); ++progit) {
      help << std::string(helpParams.progtailindent, ' ') << *progit << '\n';
    }

    help << '\n';

    for (const auto &line: description) {
      help << std::string(helpParams.descriptionindent, ' ') << line << "\n";
    }
    help << "\n";
    help << std::string(helpParams.progindent, ' ') << "OPTIONS:\n\n";

    auto shortSeparator = allowJoinedShortValue ? "" : " ";
    auto longSeparator = allowJoinedLongValue ? longseparator : " ";
    //auto descriptions = GetChildDescriptions(shortprefix, longprefix, shortSeparator, longSeparator);

    /* seperate positional and flag parameters, add an blank line in bewteen. */
    auto descriptions = GetPositionalParameterDescriptions(shortprefix, longprefix, shortSeparator,
                                                           longSeparator);
    getHelpInfoFromDesc(help, descriptions);
    help << std::endl;
    descriptions = GetFlagParameterDescriptions(shortprefix, longprefix, shortSeparator, longSeparator);
    getHelpInfoFromDesc(help, descriptions);

    if (hasoptions && hasarguments && helpParams.showTerminator) {
      help << std::endl;
      for (const auto &item: Wrap(std::string("\"") + terminator +
                                      "\" can be used to terminate flag options and force all following arguments to be treated as positional options",
                                  helpParams.width - helpParams.flagindent)) {
        help << std::string(helpParams.flagindent, ' ') << item << '\n';
      }
    }

    help << "\n";
    for (const auto &line: epilog) {
      help << std::string(helpParams.descriptionindent, ' ') << line << "\n";
    }
  }

  /** Generate a help menu as a string.
   *
   * \return the help text as a single string
   */
  std::string Help() const {
    std::ostringstream help;
    Help(help);
    return help.str();
  }

  /** Parse all arguments.
   *
   * \param begin an iterator to the beginning of the argument list
   * \param end an iterator to the past-the-end element of the argument list
   * \return the iterator after the last parsed value.  Only useful for kick-out
   */
  template<typename It>
  It ParseArgs(It begin,
               It end) {
    // Reset all Matched statuses and errors
    Reset();
    bool terminated = false;

    // Check all arg chunks
    for (auto it = begin; it != end; ++it) {
      const auto &chunk = *it;

      if (!terminated && chunk == terminator) {
        terminated = true;
        // If a long arg was found
      } else if (!terminated && chunk.find(longprefix) == 0 && chunk.size() > longprefix.size()) {
        const auto argchunk = chunk.substr(longprefix.size());
        // Try to separate it, in case of a separator:
        const auto separator = longseparator.empty() ? argchunk.npos : argchunk.find(longseparator);
        // If the separator is in the argument, separate it.
        const auto arg = (separator != argchunk.npos ?
                          std::string(argchunk, 0, separator)
                                                     : argchunk);

        if (auto base = Match(arg)) {
          if (auto argbase = dynamic_cast<ValueFlagBase *>(base)) {
            if (separator != argchunk.npos) {
              if (allowJoinedLongValue) {
                argbase->ParseValue(argchunk.substr(separator + longseparator.size()));
              } else {
#ifdef ARGS_NOEXCEPT
                error = Error::Parse;
                return it;
#else
                std::ostringstream problem;
                problem << "Flag '" << arg
                        << "' was passed a joined argument, but these are disallowed";
                throw ParseError(problem.str());
#endif
              }
            } else {
              ++it;
              if (it == end) {
#ifdef ARGS_NOEXCEPT
                error = Error::Parse;
                return it;
#else
                std::ostringstream problem;
                problem << "Flag '" << arg << "' requires an argument but received none";
                throw ParseError(problem.str());
#endif
              }

              if (allowSeparateLongValue) {
                argbase->ParseValue(*it);
              } else {
#ifdef ARGS_NOEXCEPT
                error = Error::Parse;
                return it;
#else
                std::ostringstream problem;
                problem << "Flag '" << arg
                        << "' was passed a separate argument, but these are disallowed";
                throw ParseError(problem.str());
#endif
              }
            }
          } else if (separator != argchunk.npos) {
#ifdef ARGS_NOEXCEPT
            error = Error::Parse;
            return it;
#else
            std::ostringstream problem;
            problem << "Passed an argument into a non-argument flag: " << chunk;
            throw ParseError(problem.str());
#endif
          }

          if (base->KickOut()) {
            return ++it;
          }
        } else {
#ifdef ARGS_NOEXCEPT
          error = Error::Parse;
          return it;
#else
          std::ostringstream problem;
          problem << "Unknown parameter: '" << arg << "'";
          throw ParseError(problem.str());
#endif
        }
        // Check short args
      } else if (!terminated && chunk.find(shortprefix) == 0 && chunk.size() > shortprefix.size()) {
        const auto argchunk = chunk.substr(shortprefix.size());
        for (auto argit = std::begin(argchunk); argit != std::end(argchunk); ++argit) {
          const auto arg = *argit;

          if (auto base = Match(arg)) {
            if (auto argbase = dynamic_cast<ValueFlagBase *>(base)) {
              const std::string value(++argit, std::end(argchunk));
              if (!value.empty()) {
                if (allowJoinedShortValue) {
                  argbase->ParseValue(value);
                } else {
#ifdef ARGS_NOEXCEPT
                  error = Error::Parse;
                  return it;
#else
                  std::ostringstream problem;
                  problem << "Flag '" << arg
                          << "' was passed a joined argument, but these are disallowed";
                  throw ParseError(problem.str());
#endif
                }
              } else {
                ++it;
                if (it == end) {
#ifdef ARGS_NOEXCEPT
                  error = Error::Parse;
                  return it;
#else
                  std::ostringstream problem;
                  problem << "Flag '" << arg << "' requires an argument but received none";
                  throw ParseError(problem.str());
#endif
                }

                if (allowSeparateShortValue) {
                  argbase->ParseValue(*it);
                } else {
#ifdef ARGS_NOEXCEPT
                  error = Error::Parse;
                  return it;
#else
                  std::ostringstream problem;
                  problem << "Flag '" << arg
                          << "' was passed a separate argument, but these are disallowed";
                  throw ParseError(problem.str());
#endif
                }
              }
              // Because this argchunk is done regardless
              break;
            }

            if (base->KickOut()) {
              return ++it;
            }
          } else {
#ifdef ARGS_NOEXCEPT
            error = Error::Parse;
            return it;
#else
            std::ostringstream problem;
            problem << "Unknown flag: " << arg;
            throw ParseError(problem.str());
#endif
          }
        }
      } else {
        auto pos = GetNextPositional();
        if (pos) {
          pos->ParseValue(chunk);

          if (pos->KickOut()) {
            return ++it;
          }
        } else {
#ifdef ARGS_NOEXCEPT
          error = Error::Parse;
          return it;
#else
          std::ostringstream problem;
          problem << "Unknown positional arguments: " << chunk;
          throw ParseError(problem.str());
#endif
        }
      }
    }
    if (!Matched()) {
#ifdef ARGS_NOEXCEPT
      error = Error::Validation;
#else
      std::ostringstream problem;
      problem << "Group validation failed somewhere!";
      throw ValidationError(problem.str());
#endif
    }
    return end;
  }

  /** Parse all arguments.
   *
   * \param args an iterable of the arguments
   * \return the iterator after the last parsed value.  Only useful for kick-out
   */
  template<typename T>
  auto ParseArgs(const T &args) -> decltype(std::begin(args)) {
    return ParseArgs(std::begin(args), std::end(args));
  }

  /** Convenience function to parse the CLI from argc and argv
   *
   * Just assigns the program name and vectorizes arguments for passing into ParseArgs()
   *
   * \return whether or not all arguments were parsed.  This works for detecting kick-out, but is generally useless as it can't do anything with it.
   */
  bool ParseCLI(const int argc,
                const char *const *argv) {
    if (prog.empty()) {
      prog.assign(argv[0]);
    }
    const std::vector<std::string> args(argv + 1, argv + argc);
    return ParseArgs(args) == std::end(args);
  }
};

std::ostream &operator<<(std::ostream &os,
                         const ArgumentParser &parser) {
  parser.Help(os);
  return os;
}

/** Boolean argument matcher
 */
class Flag : public FlagBase {
public:
  Flag(Group &group,
       const std::string &name,
       const std::string &help,
       Matcher &&matcher,
       const bool extraError = false) : FlagBase(name, help, std::move(matcher), extraError) {
    group.Add(*this);
  }

  virtual ~Flag() {}

  /** Get whether this was matched
   */
  virtual bool Get() const {
    return Matched();
  }
};

/** Help flag class
 *
 * Works like a regular flag, but throws an instance of Help when it is matched
 */
class HelpFlag : public Flag {
public:
  HelpFlag(Group &group,
           const std::string &name,
           const std::string &help,
           Matcher &&matcher) : Flag(group, name,
                                     help,
                                     std::move(
                                         matcher)) {}

  virtual ~HelpFlag() {}

  virtual FlagBase *Match(const std::string &arg) override {
    if (FlagBase::Match(arg)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Help;
#else
      throw Help(arg);
#endif
      return this;
    }
    return nullptr;
  }

  virtual FlagBase *Match(const char arg) override {
    if (FlagBase::Match(arg)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Help;
#else
      throw Help(std::string(1, arg));
#endif
      return this;
    }
    return nullptr;
  }

  /** Get whether this was matched
   */
  bool Get() const noexcept {
    return Matched();
  }
};

/** A flag class that simply counts the number of times it's matched
 */
class CounterFlag : public Flag {
private:
  const int startcount;
  int count;

public:
  CounterFlag(Group &group,
              const std::string &name,
              const std::string &help,
              Matcher &&matcher,
              const int startcount = 0) : Flag(group, name, help, std::move(matcher)), startcount(startcount),
                                          count(startcount) {}

  virtual ~CounterFlag() {}

  virtual FlagBase *Match(const std::string &arg) override {
    auto me = FlagBase::Match(arg);
    if (me) {
      ++count;
    }
    return me;
  }

  virtual FlagBase *Match(const char arg) override {
    auto me = FlagBase::Match(arg);
    if (me) {
      ++count;
    }
    return me;
  }

  /** Get the count
   */
  int &Get() noexcept {
    return count;
  }

  virtual void Reset() noexcept override {
    FlagBase::Reset();
    count = startcount;
  }
};

/** A default Reader class for argument classes
 *
 * Simply uses a std::istringstream to read into the destination type, and
 * raises a ParseError if there are any characters left.
 */
template<typename T>
struct ValueReader {
  bool operator()(const std::string &name,
                  const std::string &value,
                  T &destination) {
    std::istringstream ss(value);
    ss >> destination;

    if (ss.rdbuf()->in_avail() > 0) {
#ifdef ARGS_NOEXCEPT
      return false;
#else
      std::ostringstream problem;
      problem << "Argument '" << name << "' received invalid value type '" << value << "'";
      throw ParseError(problem.str());
#endif
    }
    return true;
  }
};

/** std::string specialization for ValueReader
 *
 * By default, stream extraction into a string splits on white spaces, and
 * it is more efficient to ust copy a string into the destination.
 */
template<>
struct ValueReader<std::string> {
  bool operator()(const std::string &name,
                  const std::string &value,
                  std::string &destination) {
    destination.assign(value);
    return true;
  }
};

/** An argument-accepting flag class
 *
 * \tparam T the type to extract the argument as
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 */
template<
    typename T,
    typename Reader = ValueReader<T>>
class ValueFlag : public ValueFlagBase {
private:
  T value;
  Reader reader;

public:

  ValueFlag(Group &group,
            const std::string &name,
            const std::string &help,
            Matcher &&matcher,
            const T &defaultValue = T(),
            const bool extraError = false) : ValueFlagBase(name, help,
                                                           std::move(matcher),
                                                           extraError),
                                             value(defaultValue) {
    group.Add(*this);
  }

  virtual ~ValueFlag() {}

  virtual void ParseValue(const std::string &value) override {
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, this->value))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, this->value);
#endif
  }

  /** Get the value
   */
  T &Get() noexcept {
    return value;
  }
};

/** An argument-accepting flag class that pushes the found values into a list
 *
 * \tparam T the type to extract the argument as
 * \tparam List the list type that houses the values
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 */
template<
    typename T,
    template<typename...> class List = std::vector,
    typename Reader = ValueReader<T>>
class ValueFlagList : public ValueFlagBase {
private:
  List<T> values;
  Reader reader;

public:

  ValueFlagList(Group &group,
                const std::string &name,
                const std::string &help,
                Matcher &&matcher,
                const List<T> &defaultValues = List<T>()) : ValueFlagBase(name, help, std::move(matcher)),
                                                            values(defaultValues) {
    group.Add(*this);
  }

  virtual ~ValueFlagList() {}

  virtual void ParseValue(const std::string &value) override {
    T v;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, v))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, v);
#endif
    values.insert(std::end(values), v);
  }

  /** Get the values
   */
  List<T> &Get() noexcept {
    return values;
  }

  virtual std::string Name() const override {
    return name + std::string("...");
  }

  virtual void Reset() noexcept override {
    ValueFlagBase::Reset();
    values.clear();
  }
};

/** A mapping value flag class
 *
 * \tparam K the type to extract the argument as
 * \tparam T the type to store the result as
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 * \tparam Map The Map type.  Should operate like std::map or std::unordered_map
 */
template<
    typename K,
    typename T,
    typename Reader = ValueReader<K>,
    template<typename...> class Map = std::unordered_map>
class MapFlag : public ValueFlagBase {
private:
  const Map<K, T> map;
  T value;
  Reader reader;

public:

  MapFlag(Group &group,
          const std::string &name,
          const std::string &help,
          Matcher &&matcher,
          const Map<K, T> &map,
          const T &defaultValue = T(),
          const bool extraError = false) : ValueFlagBase(name, help,
                                                         std::move(matcher),
                                                         extraError), map(map),
                                           value(defaultValue) {
    group.Add(*this);
  }

  virtual ~MapFlag() {}

  virtual void ParseValue(const std::string &value) override {
    K key;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, key))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, key);
#endif
    auto it = map.find(key);
    if (it == std::end(map)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Map;
#else
      std::ostringstream problem;
      problem << "Could not find key '" << key << "' in map for arg '" << name << "'";
      throw MapError(problem.str());
#endif
    } else {
      this->value = it->second;
    }
  }

  /** Get the value
   */
  T &Get() noexcept {
    return value;
  }
};

/** A mapping value flag list class
 *
 * \tparam K the type to extract the argument as
 * \tparam T the type to store the result as
 * \tparam List the list type that houses the values
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 * \tparam Map The Map type.  Should operate like std::map or std::unordered_map
 */
template<
    typename K,
    typename T,
    template<typename...> class List = std::vector,
    typename Reader = ValueReader<K>,
    template<typename...> class Map = std::unordered_map>
class MapFlagList : public ValueFlagBase {
private:
  const Map<K, T> map;
  List<T> values;
  Reader reader;

public:

  MapFlagList(Group &group,
              const std::string &name,
              const std::string &help,
              Matcher &&matcher,
              const Map<K, T> &map,
              const List<T> &defaultValues = List<T>()) : ValueFlagBase(name, help,
                                                                        std::move(matcher)),
                                                          map(map), values(defaultValues) {
    group.Add(*this);
  }

  virtual ~MapFlagList() {}

  virtual void ParseValue(const std::string &value) override {
    K key;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, key))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, key);
#endif
    auto it = map.find(key);
    if (it == std::end(map)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Map;
#else
      std::ostringstream problem;
      problem << "Could not find key '" << key << "' in map for arg '" << name << "'";
      throw MapError(problem.str());
#endif
    } else {
      this->values.emplace_back(it->second);
    }
  }

  /** Get the value
   */
  List<T> &Get() noexcept {
    return values;
  }

  virtual std::string Name() const override {
    return name + std::string("...");
  }

  virtual void Reset() noexcept override {
    ValueFlagBase::Reset();
    values.clear();
  }
};

/** A positional argument class
 *
 * \tparam T the type to extract the argument as
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 */
template<
    typename T,
    typename Reader = ValueReader<T>>
class Positional : public PositionalBase {
private:
  T value;
  Reader reader;
public:
  Positional(Group &group,
             const std::string &name,
             const std::string &help,
             const T &defaultValue = T())
      : PositionalBase(name, help), value(defaultValue) {
    group.Add(*this);
  }

  virtual ~Positional() {}

  virtual void ParseValue(const std::string &value) override {
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, this->value))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, this->value);
#endif
    ready = false;
    matched = true;
  }

  /** Get the value
   */
  T &Get() noexcept {
    return value;
  }
};

/** A positional argument class that pushes the found values into a list
 *
 * \tparam T the type to extract the argument as
 * \tparam List the list type that houses the values
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 */
template<
    typename T,
    template<typename...> class List = std::vector,
    typename Reader = ValueReader<T>>
class PositionalList : public PositionalBase {
private:
  List<T> values;
  Reader reader;

public:
  PositionalList(Group &group,
                 const std::string &name,
                 const std::string &help,
                 const List<T> &defaultValues = List<T>()) : PositionalBase(name, help), values(defaultValues) {
    group.Add(*this);
  }

  virtual ~PositionalList() {}

  virtual void ParseValue(const std::string &value) override {
    T v;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, v))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, v);
#endif
    values.insert(std::end(values), v);
    matched = true;
  }

  virtual std::string Name() const override {
    return name + std::string("...");
  }

  /** Get the values
   */
  List<T> &Get() noexcept {
    return values;
  }

  virtual void Reset() noexcept override {
    PositionalBase::Reset();
    values.clear();
  }
};

/** A positional argument mapping class
 *
 * \tparam K the type to extract the argument as
 * \tparam T the type to store the result as
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 * \tparam Map The Map type.  Should operate like std::map or std::unordered_map
 */
template<
    typename K,
    typename T,
    typename Reader = ValueReader<K>,
    template<typename...> class Map = std::unordered_map>
class MapPositional : public PositionalBase {
private:
  const Map<K, T> map;
  T value;
  Reader reader;

public:

  MapPositional(Group &group,
                const std::string &name,
                const std::string &help,
                const Map<K, T> &map,
                const T &defaultValue = T()) : PositionalBase(name, help), map(map), value(defaultValue) {
    group.Add(*this);
  }

  virtual ~MapPositional() {}

  virtual void ParseValue(const std::string &value) override {
    K key;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, key))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, key);
#endif
    auto it = map.find(key);
    if (it == std::end(map)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Map;
#else
      std::ostringstream problem;
      problem << "Could not find key '" << key << "' in map for arg '" << name << "'";
      throw MapError(problem.str());
#endif
    } else {
      this->value = it->second;
      ready = false;
      matched = true;
    }
  }

  /** Get the value
   */
  T &Get() noexcept {
    return value;
  }
};

/** A positional argument mapping list class
 *
 * \tparam K the type to extract the argument as
 * \tparam T the type to store the result as
 * \tparam List the list type that houses the values
 * \tparam Reader The functor type used to read the argument, taking the name, value, and destination reference with operator(), and returning a bool (if ARGS_NOEXCEPT is defined)
 * \tparam Map The Map type.  Should operate like std::map or std::unordered_map
 */
template<
    typename K,
    typename T,
    template<typename...> class List = std::vector,
    typename Reader = ValueReader<K>,
    template<typename...> class Map = std::unordered_map>
class MapPositionalList : public PositionalBase {
private:
  const Map<K, T> map;
  List<T> values;
  Reader reader;

public:

  MapPositionalList(Group &group,
                    const std::string &name,
                    const std::string &help,
                    const Map<K, T> &map,
                    const List<T> &defaultValues = List<T>()) : PositionalBase(name, help), map(map),
                                                                values(defaultValues) {
    group.Add(*this);
  }

  virtual ~MapPositionalList() {}

  virtual void ParseValue(const std::string &value) override {
    K key;
#ifdef ARGS_NOEXCEPT
    if (!reader(name, value, key))
    {
        error = Error::Parse;
    }
#else
    reader(name, value, key);
#endif
    auto it = map.find(key);
    if (it == std::end(map)) {
#ifdef ARGS_NOEXCEPT
      error = Error::Map;
#else
      std::ostringstream problem;
      problem << "Could not find key '" << key << "' in map for arg '" << name << "'";
      throw MapError(problem.str());
#endif
    } else {
      this->values.emplace_back(it->second);
      matched = true;
    }
  }

  /** Get the value
   */
  List<T> &Get() noexcept {
    return values;
  }

  virtual std::string Name() const override {
    return name + std::string("...");
  }

  virtual void Reset() noexcept override {
    PositionalBase::Reset();
    values.clear();
  }
};
}

#endif