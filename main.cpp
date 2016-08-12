
#include "main.h"

const string Raw_filter_list[] = {
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


const  string LoggerName("myLogger1");

//class join;
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
int main(int argc, char **argv)
{
    //todo: input parameter, and validation of the file/dir
    if (1 == argc)
    {
        cerr << "usage: " << argv[0] << " [-h] [-O OUTPUT] [-L FILE_LIST] f1 f2" << endl;
        return 1;
    }

    InputParser input(argc, argv);
    if (input.cmdOptionExists("-h"))
    {
        cout << << endl;
    }

    setup_logger();

    hash_compare_log_file("W:/tools_baichun/log_cmp_easy/d1/t.log", "W:/tools_baichun/log_cmp_easy/d2/t.log");
    return 0;
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
    hash<string> str_hash;
    LineNr lineNumber;
    Position position;
    string line;

    auto myLogger = spdlog::get(LoggerName);
    //myLogger->info("huahua1-info");
    //myLogger->debug("huahua2-debug");
    //myLogger->error("tst can't open {}", file_left);

    //try using wc to count lines, then use known-sized array. sort, then set-diff. Which way is faster?

    const int _left_ = 0, _right_ = 1;
    cHashSet mySet[2]; // a set of hash-values
    cMAP_HashAndLine myMap[2]; // hash-value ==map-to=> (lineNr, position-in-file)
    string fileToCompare[] = {file_left, file_right};
    char marks[] = {'-', '+'};
    size_t diffLineCount[2];
    vector<string> diffLines[2], filtered_result_lines[2];

	//todo: do hash with 2 thread, using std::async()
	//e.g. auto res1= async(f, some_vec);
	//     auto res2 =async(f, some_vec);
    //     cout << res1.get() << ' ' << res2.get() << endl;
	
    /* for each file, read-in lines, and compute line-hash */
    for (auto i: {_left_, _right_})
    {
        string f = fileToCompare[i];
        /* open file */
        ifstream inFile(f.c_str(), ios::binary); //must open as binary, then position is correct.
        if (inFile)
        {
            /* read in lines */
            lineNumber = 0;
            position = inFile.tellg();
            while (getline(inFile, line))
            {
                /* compute hash and store in map/set */
                cPAIR_LineInfo myLineInfo(++lineNumber, position);
                HashValue l_hash = str_hash(line); //try to detect collision of hashes when adding them to map
                myMap[i][l_hash] = myLineInfo; // l_hash ~mapped-to~ (lineNumber, position)
                mySet[i].insert(l_hash);
                position = inFile.tellg();
            }
            inFile.close();
        } else
        {
            myLogger->error("can't open {}", f);
        }
    }

    /* use set-operation to pick out diff lines(hashes) */
    cHashSet diffResults[2];
    set_difference(mySet[_left_].begin(), mySet[_left_].end(),
                   mySet[_right_].begin(), mySet[_right_].end(),
                   inserter(diffResults[_left_], diffResults[_left_].end()));
    set_difference(mySet[_right_].begin(), mySet[_right_].end(),
                   mySet[_left_].begin(), mySet[_left_].end(),
                   inserter(diffResults[_right_], diffResults[_right_].end()));

    /* process/sort/print the result, use line-number/position to retrieve line content */
    for (auto i: {_left_, _right_})
    {
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

string getFileName(const string &s)
{
    string sep = "\\/";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(found + 1);
    }
    return s;
}

string getBaseName(const string &s)
{
    string sep = ".";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(0, found);
    }
    return s;
}

string stripCRLF(const string &s)
{
    string sep = "\r\n";

    size_t found = s.find_last_of(sep);
    if (found != string::npos)
    {
        return s.substr(0, found);
    }
    return s;
}
