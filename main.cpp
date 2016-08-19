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

const string RawStr_example{
    R"rawstring(example:
    c_hash_cmp_log.exe dir1  dir2   -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2  -O diff_summary.txt
    c_hash_cmp_log.exe file1 file2 -L fileList.txt -O diff_summary.txt
    )rawstring"
};

const string LoggerName{"myLogger1"};

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
    args::ArgumentParser parser("Compare common-log-files.", RawStr_example);
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

    string f1_name, f2_name;
    if (S_OK == ret)
    {
        if (output_file) { std::cout << "output_file: " << args::get(output_file) << std::endl; }
        if (cmp_file_list) { std::cout << "cmp_file_list: " << args::get(cmp_file_list) << std::endl; }

        /* validate parameters */
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
            hash_compare_log_file(f1_name, f2_name);
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

void hash_compare_log_file(string file_left, string file_right)
{
    //try using wc to count lines, then use known-sized array. sort, then set-diff. Which way is faster?
    const int _left_ = 0, _right_ = 1;
    HashSet mySet[2];           // a set of hash-values
    MAP_HashAndLine myMap[2];   // hash-value ##MappingTo## (lineNr, position-in-file)

    //todo: do hash with 2 thread, using std::async()
    //e.g. auto res1= async(f, some_vec);
    //     auto res2 =async(f, some_vec);
    //     cout << res1.get() << ' ' << res2.get() << endl;

    /* for each file, read-in lines, and compute line-hash */
    cchash(file_left, mySet[_left_], myMap[_left_]);
    cchash(file_right, mySet[_right_], myMap[_right_]);

    /* use set-operation to pick out diff lines(hashes) */
    auto myLogger = spdlog::get(LoggerName);
    HashSet diffResults[2];
    set_difference(mySet[_left_].begin(), mySet[_left_].end(),
        mySet[_right_].begin(), mySet[_right_].end(),
        inserter(diffResults[_left_], diffResults[_left_].end()));
    set_difference(mySet[_right_].begin(), mySet[_right_].end(),
        mySet[_left_].begin(), mySet[_left_].end(),
        inserter(diffResults[_right_], diffResults[_right_].end()));

    /* process/sort/print the result, use line-number/position to retrieve line content */
    string fileToCompare[] = {file_left, file_right};
    char marks[] = {'-', '+'};
    size_t diffLineCount[2];
    vector<string> diffLines[2], filtered_result_lines[2];
    for (auto i: {_left_, _right_})
    {
        hash<string> str_hash;
        LineNr lineNumber;
        Position position;
        string line;

        string f = fileToCompare[i];
        ifstream inFile(f.c_str(), ios::binary); //only when open as binary, then position is correct.
        if (inFile)
        {
            diffLineCount[i] = diffResults[i].size();
            diffLines[i].reserve(diffLineCount[i]);

            for (auto u : diffResults[i]) //use u as an index/key in the map
            {
                lineNumber = myMap[i][u].first;
                position = myMap[i][u].second;
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
 * @brief compute hash for each line of the file, store the hash-set and hash-map.
 * @param fileName [in] name of the file
 * @param set [out] hash-set, a "set" of the hash values
 * @param map [out] a "map"; l_hash ~MappingTo~ (lineNumber, position)
 */
void cchash(const string &fileName,
    HashSet &set,
    MAP_HashAndLine &map)
{
    ifstream inFile(fileName.c_str(), ios_base::binary); //must open as binary, then position is correct.

    /* read in lines */
    LineNr lineNumber = 1; //lineNumber is shown as 1, 2, 3 ....
    Position position = inFile.tellg();
    string line;
    hash<string> str_hash;
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
}

