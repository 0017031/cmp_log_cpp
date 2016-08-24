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

  if (p.valid) {
    setup_logger(p.outputFile_name);

    if (CompareType::file == p.howToCompare) {
      //W:/tools_baichun/log_cmp_easy/d1/t.log W:/tools_baichun/log_cmp_easy/d2/t.log
      hash_and_compare_log(p.f1_name, p.f2_name);

    } else if (CompareType::dir == p.howToCompare) {
      //todo: compare dir
      cerr << "todo: compare dir" << endl;

    } else if (CompareType::dir_with_list == p.howToCompare) {
      //todo: compare using file-list
      cerr << "todo: compare using file-list " << p.listFile_name << endl;
    }

    return 0;
  } else {
    return 1;
  }
}

void setup_logger(string diff_record_file /* = "diff_summary.txt"*/)
{
  // Create a logger with multiple sinks
  auto mySink_console = make_shared<spdlog::sinks::stdout_sink_st>();
  auto mySink_full = make_shared<spdlog::sinks::simple_file_sink_st>(diff_record_file);
  auto mySink_brief = make_shared<spdlog::sinks::simple_file_sink_st>(getBaseName(diff_record_file) + "_brief.txt");

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
  const int _left__ = 0;
  const int _right_ = 1;

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
  for (auto i: {_left__, _right_}) { // print left unique lines, then right.
    /* get lines content from hash */
    DiffResultLines diff = getLinesFromHash(uniqHashes[i], files[i], myPairOf_Hashes_And_Map[i].second);

    /* filter with regex */
    DiffResultLines filteredLines = regexFilterLines(diff, Raw_filter_list);

    /* print the result */
    myLogger->info("{}{}{} {} unique lines in {}", prefixMarks[i], prefixMarks[i], prefixMarks[i],
                   filteredLines.size(), files[i]);
    for (auto l : filteredLines) {
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
  ifstream inFile{fileName.c_str(), ios_base::binary}; //must open as binary, then position is correct.
  hash<string> str_hash;  //the hash "function"
  LineHashes hashes{};       //store the hash
  MAP_HashAndLine map{};  //store the {hash, LineInfo} map; l_hash ~mapped-to~ (lineNumber, position)

  /* the initial position and lineNumber */
  LineNr lineNumber{1}; //lineNumber is shown as 1, 2, 3 .... (starting from 1)
  Position position = inFile.tellg();

  /* read lines */
  string line{};
  while (getline(inFile, line)) {
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
  for (auto u_hash : hashes) {
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
  for (auto l : lines) {
    matchRegex_shouldOmit = regex_search(l.second, myRegexObj);
    if (!matchRegex_shouldOmit) {
      filtered_result.push_back(l);
    }
  }
  auto diffLineCount = filtered_result.size();
  sort(filtered_result.begin(), filtered_result.begin() + diffLineCount);

  return filtered_result;
}


