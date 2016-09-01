#include "pchheader.h"
#include "main.h"

using namespace std;

/*!
 *  Compare common-log-files.
 *
 *  Try using "set" operations.
 *  First, create hash for each line, appending with line-number and offset (for retrieving line content later)
 *  Thus each file has its own "set" of hashes.
 *  Then, the "diff" is the resut of "set" operations. unique[left] = set[left]-set[right], and vice-versa.
 *
 * @return none
 */
int main(const int argc, const char **argv)
{

  myParameter p{argc, argv};
  if (!p.valid)
  {
    cerr << "\nInvalid parameter, type \"" << argv[0] << " -h\" for help" << endl;
    return 1;
  }

  setup_logger(p.outputFilePath.string());

  if (CompareType::file == p.howToCompare)
  {
    //W:/tools_baichun/log_cmp_easy/d1/t.log W:/tools_baichun/log_cmp_easy/d2/t.log
    hash_and_compare_log(p.path_left_.string(), p.path_right.string());

  }
  else if (CompareType::dir == p.howToCompare)
  {
    //todo: compare dir
    cerr << "todo: compare dir" << endl;
    compare_dir(p.path_left_, p.path_right);

  }
  else if (CompareType::dir_with_list == p.howToCompare)
  {
    //todo: compare using file-list
    cerr << "todo: compare using file-list " << p.listFilePath << endl;
  }

  return 0;
}

void setup_logger(string diff_record_file /* = "diff_summary.txt"*/)
{
  // Create a logger with multiple sinks
  auto mySink_console = make_shared<spdlog::sinks::stdout_sink_st>();
  auto mySink_full = make_shared<spdlog::sinks::simple_file_sink_st>(diff_record_file);
  auto mySink_brief = make_shared<spdlog::sinks::simple_file_sink_st>(getBase(diff_record_file) + "_brief.txt");

  mySink_full->set_level(spdlog::level::debug);
  mySink_brief->set_level(spdlog::level::info);
  vector<spdlog::sink_ptr> mySinks{mySink_console, mySink_full, mySink_brief};

  auto myLogger = make_shared<spdlog::logger>(LoggerName, begin(mySinks), end(mySinks));
  spdlog::register_logger(myLogger);
  myLogger->set_pattern("%v");
}

void hash_and_compare_log(const string file_left_, const string file_right)
{
  string files[2] = {file_left_, file_right};

  /* for each file, read-in lines, and compute line-hash, using multi-thread with std::ansync() */
  auto asyn_Left = async(launch::async, doHashLines, file_left_);
  auto asynRight = async(launch::async, doHashLines, file_right);
  pair<LineHashes, MAP_HashAndLine> myPairOf_Hashes_And_Map[2]{asyn_Left.get(), asynRight.get()};

  /* use set-MATH-operation to pick out diff lines(hashes) */
  //LineHashes operator-(const LineHashes &setA, const LineHashes &setB)
  LineHashes uniqHashes[2];
  uniqHashes[_left__] = (myPairOf_Hashes_And_Map[_left__].first - myPairOf_Hashes_And_Map[_right_].first);
  uniqHashes[_right_] = (myPairOf_Hashes_And_Map[_right_].first - myPairOf_Hashes_And_Map[_left__].first);

  /* process/sort/print the result */
  auto myLogger = spdlog::get(LoggerName);
  for (auto i: {_left__, _right_})
  { // print left unique lines, then right.
    /* get lines content from hash */
    DiffResultLines diff = getLinesFromHash(uniqHashes[i], files[i], myPairOf_Hashes_And_Map[i].second);

    /* filter with regex */
    DiffResultLines filteredLines = regexFilterLines(diff, Raw_filter_list);

    /* print the result */
    myLogger->info("{}{}{} {} unique lines in {}", prefixMarks[i], prefixMarks[i], prefixMarks[i],
                   filteredLines.size(), files[i]);
    for (auto l : filteredLines)
    {
      myLogger->debug("{} #{}, {}", prefixMarks[i], l.first, l.second);
    }
  }
  /*
  --- 2 unique lines in W:/tools_baichun/log_cmp_easy/d1/t.log
  - #61, 000000.000000|SortLog|StdProperty|sort_log.pl|K:/PGSV/sortl_log/Release/1.16/sort_log.pl|
  - #16218, 000102.393514|Predator|StdProperty|Number of System Errors Reported1:           |0|
  +++ 2 unique lines in W:/tools_baichun/log_cmp_easy/d2/t.log
  + #61, 000000.000000|SortLog|StdProperty|sort_log.pl|K:/PGSV/sortl_log/Release/1.16/sort_logaaa.pl|
  + #16224, 000102.393514|Predator|StdProperty|Number of Test Warnings Reported2:           |0|
  */

  return;
}

pair<LineHashes, MAP_HashAndLine> doHashLines(const string &fileName)
{
  ifstream inFile{fileName, ios_base::binary}; //must open as binary, then position is correct.
  hash<string> str_hash;  //the hash "function"
  LineHashes hashes{};       //store the hash
  MAP_HashAndLine map{};  //store the {hash, LineInfo} map; l_hash ~mapped-to~ (lineNumber, position)

  /* the initial position and lineNumber */
  LineNr lineNumber{1}; //lineNumber is shown as 1, 2, 3 .... (starting from 1)
  Position position = inFile.tellg();

  /* read lines */
  string line{};
  while (getline(inFile, line))
  {
    /* compute hash and store in map and set */
    HashValue l_hash = str_hash(line);
    hashes.push_back(l_hash);
    map[l_hash] = PAIR_LineInfo{lineNumber, position}; // l_hash ~mapped-to~ (lineNumber, position)

    /* renew lineNumber and position for next iteration */
    ++lineNumber;
    position = inFile.tellg();
  }

  std::sort(hashes.begin(), hashes.end()); //hash must be sorted, for the "set" math operation later.

  return std::make_pair(hashes, map);
}

LineHashes operator-(const LineHashes &setA, const LineHashes &setB)
{

  LineHashes result;

  set_difference(
      setA.begin(), setA.end(),
      setB.begin(), setB.end(),
      std::inserter(result, result.end()));

  return result;
}

LineHashes getUniqueElements(const LineHashes &setA, const LineHashes &setB)
{
  return setA - setB;
}

DiffResultLines getLinesFromHash(const LineHashes &hashes, const std::string &fileName, const MAP_HashAndLine &myMap)
{
  DiffResultLines diffLines{};

  ifstream inFile(fileName.c_str(), ios_base::binary); //only when open as binary, then position is correct.
  auto diffLineCount = hashes.size();
  diffLines.reserve(diffLineCount); //reserve spaces, cause we know the size in advance.

  string line{};
  for (auto u_hash : hashes)
  {
    LineNr lineNumber = myMap.at(u_hash).first;
    Position position = myMap.at(u_hash).second;
    getline(inFile.seekg(position), line);                      //get line content via position
    diffLines.push_back(pair<LineNr, string>{lineNumber, line});   //prefix with a mark "+", or "-"
  }

  return diffLines;
}

DiffResultLines regexFilterLines(const DiffResultLines &lines, const RegexRawLines &rawRegexList)
{
  /* get myRegexObj from RAW-list */
  string regex_whole_line{rawRegexList[0]}; //concatenate lists into one-line with "|"
  for_each(begin(rawRegexList) + 1, end(rawRegexList),
           [&regex_whole_line](const string s) {
             regex_whole_line.append("|").append(s);
           });
  regex myRegexObj{regex_whole_line};

  /* go through diffLiens, filter out regex-matched lines */
  DiffResultLines filtered_result;
  bool matchRegex_shouldOmit;
  for (auto l : lines)
  {
    matchRegex_shouldOmit = regex_search(l.second, myRegexObj);
    if (!matchRegex_shouldOmit)
    {
      filtered_result.push_back(l);
    }
  }
  auto diffLineCount = filtered_result.size();
  sort(filtered_result.begin(), filtered_result.begin() + diffLineCount);

  return filtered_result;
}

void compare_dir(const stdFs::path dir_left_, const stdFs::path dir_right)
{
  auto myLogger = spdlog::get(LoggerName);
  if (dir_left_ == dir_right)
  {
    myLogger->info("Two same directories ({}) are given, comparison aborted.", dir_left_.string());
    return;
  }

//# list all the log files, case insensitive
  vector<string> files[2]{getFilesFromDir(dir_left_), getFilesFromDir(dir_right)};

  cout << dir_left_ << endl;
  for (auto f: files[_left__])
  {
    cout << f << endl;
  }
  cout << endl << dir_right << endl;
  for (auto f: files[_right_])
  {
    cout << f << endl;
  }


//# find out common files

//# compare each file by compare_log_file()
}

myParameter::myParameter(const int argc, const char *const *argv)
    : valid{false},
      howToCompare{CompareType::unknown}
{
  args::ArgumentParser parser("Compare common-log-files.", RawStr_epilog_CallingExample);
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<std::string>
      output_file(parser,
                  "OUTPUT",
                  "OUTPUT:file to store the diff summary",
                  {'O', "output"});
  args::ValueFlag<std::string>
      list_file(parser,
                "FILE_LIST",
                "FILE_LIST:a file of all the file names (which exist in both dir1 and dir2) to be compared, one name perl line)",
                {'L', "file_list"});
  args::Positional<std::string> f1(parser, "f1", "the 1st(left)  file/dir to compare");
  args::Positional<std::string> f2(parser, "f2", "the 1st(left)  file/dir to compare");

  /* parse parameters */
  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (args::Help)
  {
    std::cout << "===========" << std::endl;
    std::cout << parser;
    return;
  }
  catch (args::ParseError e)
  {
    std::cerr << "===========" << std::endl;
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return;
  }

  /* get parameter values */
  path_left_.assign(f1.Get());
  path_right.assign(f2.Get());
  listFilePath.assign(list_file.Get());
  outputFilePath.assign(output_file.Get()); //set default first.

  /* validate parameters: both f1, f2 should be valid */
  for (auto p : {path_left_, path_right})
  {
    if (p.empty())
    {
      std::cerr << "\n! Missing parameters. Both f1 and f2 are needed\n" << std::endl;
      return;
    }
    else if (!stdFs::exists(p))
    {
      std::cerr << "\n! Can't open " << p << ", \nplease check your parameter.\n" << std::endl;
      return;
    }
  }

  /* validate parameters: listFilePath(if given) should be valid */
  if (!listFilePath.empty())
  {
    if (!stdFs::exists(listFilePath)) //if given, it should exist.
    {
      std::cerr << "\n!! -L FILE_LIST: " << listFilePath
                << " doesn't exist, \nplease check your parameter.\n" << std::endl;
      return;
    }
    else if (stdFs::is_directory(listFilePath)) // it should not be an directory
    {
      std::cerr << "\n! -L FILE_LIST: " << listFilePath
                << " can't be a *directory*, \nplease check your parameter.\n" << std::endl;
      return;
    }
  }

  /* validate parameters: outputFilePath(if anything wrong, set to default) */
  if (outputFilePath.empty()) //if empty, set to default
  {
    outputFilePath.assign(defaultOutputFile); //set default first.
  }
  else //if valid given, update to new value
  {
    ofstream f{output_file.Get()};
    if (f.good()) //test valid
    {
      outputFilePath.assign(output_file.Get());
    }
    else
    {
      std::cerr << "\n! Invalid -L OUTPUT:" << output_file.Get()
                << ", now use default " << defaultOutputFile << " instead\n" << std::endl;
      outputFilePath.assign(defaultOutputFile); //set default first.
    }
  }

  /* decide how to compare */
  if (stdFs::is_regular_file(path_left_) && stdFs::is_regular_file(path_right)) //two files given
  {
    howToCompare = CompareType::file;
    valid = true;
  }
  else if (stdFs::is_directory(path_left_) && stdFs::is_directory(path_right)) //two dir given
  {
    listFilePath.empty() ?
        howToCompare = CompareType::dir :
        howToCompare = CompareType::dir_with_list;
    valid = true;
  }
  else
  {
    std::cerr << "\n! Can't compare file Vs. dir, please check your parameter.\n" << std::endl;
    valid = false;
  }
  return;
}

std::vector<std::string> getFilesFromDir(const stdFs::path folder)
{
  std::vector<std::string> files;

  for (const auto f : stdFs::directory_iterator{folder})
  {
    if (stdFs::is_regular_file(f))
    {
      string s{f.path().filename().string()};
      files.push_back(s);
    }
  }
  std::sort(files.begin(), files.end(),
            [](string s0, string s1) { //case insensitive sort
              std::transform(s0.begin(), s0.end(), s0.begin(), ::tolower);
              std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
              return s0 < s1;
            });
  return files;
}

std::string getBase(const std::string &s)
{
  stdFs::path p;
  p.assign(s);
  return p.stem().string();
}