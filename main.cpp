#include <future>
#include "main.h"
const string Raw_filter_list[]{
    "#",
    //R"rawstring(MAOV\|StdProperty\|Verifier Configuration File)rawstring",
    //R"rawstring(MAOV\|StdProperty\|Verification Date & Time)rawstring",
    //R"rawstring(MAOV\|StdVersion\|)rawstring",
    //R"rawstring(MODEL\|StdProperty\|Simulation Date & Time)rawstring",
    //R"rawstring(MODEL\|StdVersion\|CEL)rawstring",
    //R"rawstring(MODEL\|StdVersion\|ModelExecutable)rawstring",
    //R"rawstring(MODEL\|StdVersion\|ModelLibrary)rawstring",
    //R"rawstring(SortLog\|StdProperty\|Filtered Events File)rawstring",
    //R"rawstring(SortLog\|StdProperty\|sort_log.pl)rawstring",
};

//constexpr string FullHelpMessage = {
//    R"rawstring(Compare common-log-files
//
//positional arguments:
//  f1                    the 1st(left)  file/dir to compare
//  f2                    the 2nd(right) file/dir to compare
//
//optional arguments:
//  -h                    show this help message and exit
//  -O OUTPUT
//                        file to store the diff summary
//  -L FILE_LIST
//                        a file which lists all the file names (which exist in both dir1 and dir2) to be compared (one name perl line)
//example:
//    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
//    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
//    c_hash_cmp_log.exe file1 file2 -L fileList.txt -O diff_summary.txt
//)rawstring"
//};

const string RawStr_epilog_CallingExample{
    R"rawstring(example:
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2 -L fileList.txt -O diff_summary.txt
    )rawstring"
};

const string LoggerName{"myLogger1"};

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

/*! @breif compute hashes line by line, store the result in a set, and a map
 *
 * @param fileName
 * @return  pair<HashSet, MAP_HashAndLine>
 */
pair<HashSet, MAP_HashAndLine> hashLines(const string &fileName)
{
    HashSet set;
    MAP_HashAndLine map;
    ifstream inFile(fileName.c_str(), ios_base::binary); //must open as binary, then position is correct.

    hash<string> str_hash; //the hash "function"

    /* the initial position and lineNumber */
    LineNr lineNumber = 1; //lineNumber is shown as 1, 2, 3 .... (starting from 1)
    Position position = inFile.tellg();

    /* read in lines */
    string line;
    while (getline(inFile, line))
    {
        /* compute hash and store in map and set */
        HashValue l_hash = str_hash(line);
        auto ret = set.insert(l_hash);
        map[l_hash] = PAIR_LineInfo {lineNumber, position}; // l_hash ~mapped-to~ (lineNumber, position)

#ifdef REPORT_REPEATED_LINES
        if (ret.second) //ret.second: a bool that is true if the element was actually inserted
        {
            map[l_hash] = PAIR_LineInfo {lineNumber, position}; // l_hash ~mapped-to~ (lineNumber, position)

        } else /* report if found same line (there is nothing to do about hash-collision, unless to change the hash-function) */
        {
            auto myLogger = spdlog::get(LoggerName);
            //myLogger->error("found a identical line: {}, {}#, {}", fileName, lineNumber, line);
            myLogger->error("same line?: #{}, {}", lineNumber, line);
            try //try to find privious line
            {
                PAIR_LineInfo l_Info = map.at(l_hash);  //throw "std::out_of_range" if not found
                Position posOriginal = inFile.tellg();
                string previousLine;
                getline(inFile.seekg(l_Info.second), previousLine);
                myLogger->error("prev line : #{}, {}\n", l_Info.first, previousLine);
                inFile.seekg(posOriginal);              //rember to restore file position!
            }
            catch (const std::out_of_range &oor)
            {
                myLogger->error("\tCan't find previous line");
            }
    }
#endif
        /* renew lineNumber and position for next iteration */
        ++lineNumber;
        position = inFile.tellg();
    }

    return pair<HashSet, MAP_HashAndLine> {set, map};
}

/*! @brief compare logfiles by computing the hash of each lines,
 *  then use set-math-operation to find out unique lines
 *
 * @param file_left_ [in]
 * @param file_right [in]
 */
void hash_and_compare_log(const string file_left_, const string file_right)
{
    //try using wc to count lines, then use known-sized array. sort, then set-diff. Which way is faster?
    const int _left__ = 0, _right_ = 1;

    /* for each file, read-in lines, and compute line-hash */
    auto asyn_Left = async(launch::async, hashLines, file_left_);
    auto asynRight = async(launch::async, hashLines, file_right);
    pair<HashSet, MAP_HashAndLine> ResultSetAndMap[2]{asyn_Left.get(), asynRight.get()};

#if 1 /* use set-MATH-operation to pick out diff lines(hashes) */
    HashSet uniqElements[2]; //HashSet operator-(const HashSet &setA, const HashSet &setB)
    uniqElements[_left__] = ResultSetAndMap[_left__].first - ResultSetAndMap[_right_].first;
    uniqElements[_right_] = ResultSetAndMap[_right_].first - ResultSetAndMap[_left__].first;

#else //async() no good here, because ResultSetAndMap[] in use.
    auto a1 = async(launch::async, getUniqueElements, ResultSetAndMap[_left__].first, ResultSetAndMap[_right_].first);
    auto a2 = async(launch::async, getUniqueElements, ResultSetAndMap[_right_].first, ResultSetAndMap[_left__].first);
    HashSet uniqElements[2]{a1.get(), a2.get()};
#endif

    //todo: refactor below, extract shorter functions.
    /* process/sort/print the result, use line-number/position to retrieve line content */
    auto myLogger = spdlog::get(LoggerName);
    string fileToCompare[] = {file_left_, file_right};
    char marks[] = {'-', '+'};
    size_t diffLineCount[2];
    vector<string> diffLines[2], filtered_result_lines[2];
    MAP_HashAndLine myMap[2]{ResultSetAndMap[_left__].second, ResultSetAndMap[_right_].second};
    for (auto i: {_left__, _right_})
    {
        hash<string> str_hash;
        LineNr lineNumber;
        Position position;
        string line;

        string f = fileToCompare[i];
        ifstream inFile(f.c_str(), ios::binary); //only when open as binary, then position is correct.
        if (inFile)
        {
            diffLineCount[i] = uniqElements[i].size();
            diffLines[i].reserve(diffLineCount[i]);

            for (auto u_hash : uniqElements[i]) //use u as an index/key in the map
            {
                lineNumber = myMap[i][u_hash].first;
                position = myMap[i][u_hash].second;
                getline(inFile.seekg(position), line); //get line content via position
                diffLines[i].push_back(marks[i] + to_string(lineNumber) + ", " + line);
            }
            inFile.close();

            /* regex on diff-lines */
            string raw_regex_line; //concatenate lists into one-line with "|"
            raw_regex_line.append(Raw_filter_list[0]);
            for_each(begin(Raw_filter_list) + 1, end(Raw_filter_list),
                [&raw_regex_line](const string s)
                {
                    raw_regex_line.append("|").append(s);
                });
            regex myRegexObj_FilterLines(raw_regex_line);

            /* go through diffLiens, filter out regex-matched lines */
            bool isIgnored;
            for (auto l : diffLines[i])
            {
                isIgnored = regex_search(l, myRegexObj_FilterLines);
                if (!isIgnored)
                {
                    filtered_result_lines[i].push_back(l);
                }
            }
            diffLineCount[i] = filtered_result_lines[i].size();
            sort(filtered_result_lines[i].begin(), filtered_result_lines[i].begin() + diffLineCount[i]);

            /* print the result */
            //cout << marks[i] << marks[i] << marks[i] << " " << diffLineCount[i] << " unique lines in "
            //     << fileToCompare[i] << endl;
            myLogger->info("{}{}{} {} unique lines in {}",
                marks[i],
                marks[i],
                marks[i],
                diffLineCount[i],
                fileToCompare[i]);

            for (auto l : filtered_result_lines[i])
            {
                //cout << l;
                //myLogger->debug(stripCRLF(l));
                myLogger->debug(l);
            }
        } else
        {
            myLogger->error("can't open {}", f);
        }

    }

    return;
}

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
    args::ArgumentParser parser("Compare common-log-files.", RawStr_epilog_CallingExample);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> output_file(parser, "OUTPUT", "OUTPUT:file to store the diff summary", {'O', "output"});
    args::ValueFlag<string> cmp_file_list(parser,
        "FILE_LIST",
        "FILE_LIST:a file of all the file names (which exist in both dir1 and dir2) to be compared, one name perl line)",
        {'L', "file_list"});
    args::Positional<std::string> f1(parser, "f1", "the 1st(left)  file/dir to compare");
    args::Positional<std::string> f2(parser, "f2", "the 1st(left)  file/dir to compare");

    /* parse parameters */
    int ret = S_OK;
    try
    {
        parser.ParseCLI(argc, argv);
        if (!f1 || !f2) //I do need both f1 and f2
        {
            cerr << "\n! Missing parameters. Both f1 and f2 are needed\n" << endl;
            cout << "===========" << endl;
            std::cout << parser;
            ret = S_FALSE;
        }
    }
    catch (args::Help)
    {
        cout << "===========" << endl;
        std::cout << parser;
        ret = S_OK;
    }
    catch (args::ParseError e)
    {
        cerr << "===========" << endl;
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        ret = S_FALSE;
    }

    /* validate parameters */
    string f1_name, f2_name;
    if (S_OK == ret)
    {
        if (output_file) { std::cout << "output_file: " << args::get(output_file) << std::endl; }
        if (cmp_file_list) { std::cout << "cmp_file_list: " << args::get(cmp_file_list) << std::endl; }

        f1_name = f1.Get();
        f2_name = f2.Get();

        /* both f1 and f2 should be valid */
        if (!FileCanBeRead(f1_name))
        {
            cerr << "\n! Can't open " << f1_name << ", \nplease check your input.\n" << endl;
            cout << "===========" << endl;
            std::cout << parser;
            ret = S_FALSE;
        } else if (!FileCanBeRead(f2_name))
        {
            cerr << "\n! Can't open " << f2_name << ", \nplease check your input.\n" << endl;
            cout << "===========" << endl;
            std::cout << parser;
            ret = S_FALSE;
        }
    }

    if (S_OK == ret)
    {
        setup_logger();

        if (IsDir(f1_name) && IsDir(f2_name))
        {
            //todo: compare dir
            cout << "both are dir" << endl;
        } else if (!IsDir(f1_name) && !IsDir(f2_name))
        {
            //cout << "both are Files" <<endl;
            //W:/tools_baichun/log_cmp_easy/d1/t.log W:/tools_baichun/log_cmp_easy/d2/t.log
            hash_and_compare_log(f1_name, f2_name);
        } else
        {
            cerr << "\n! Can't compare file Vs. directory\n" << endl;
            cout << "===========" << endl;
            std::cout << parser;
            return 1;
        }
        //todo: compare using file-list
    }
    return ret;
}
