#define SPDLOG_WCHAR_FILENAMES
#include "stdafx.h"
#include "main.h"

#include "spdlog/spdlog.h"
#include "args/args.hxx"

using namespace gsl;
using namespace std;
using namespace stdfs;

/*!
 *  Compare common-log-files. (current limitation: all files and path are ASCII).
 *
 *  Try using "set" operations.
 *  First, create hash for each line, appending with line-number and offset (for retrieving line content later)
 *  Thus each file has its own "set" of hashes.
 *  Then, the "diff" is the result of "set" operations. unique[left] = set[left]-set[right], and vice-versa.
 *
 * @return none
 */
int main(int argc, const char *argv[]) {
	const auto my_parameter = myParameter{argc, argv};

	if (!my_parameter.valid) {
		const auto myArgSpan = span<const char *, dynamic_extent>{argv, argc};
		cerr << "\nInvalid parameter, type \"" << myArgSpan[0] << " -h\" for help" << endl;
		return 1;
	}

	setup_logger(my_parameter.outputFilePath.wstring());

	if (my_parameter.howToCompare == CompareType::file) {
		compare_logFile2({my_parameter.path_left_, my_parameter.path_right});
		// W:/tools_baichun/log_cmp_easy/d1/t.log W:/tools_baichun/log_cmp_easy/d2/t.log
	} else if (my_parameter.howToCompare == CompareType::dir) {
		compare_dir({my_parameter.path_left_, my_parameter.path_right},
		            my_parameter.fileType);
	} else if (my_parameter.howToCompare == CompareType::dir_with_list) {
		// todo: compare using file-list
		cerr << "todo: compare using file-list " << my_parameter.listFilePath << endl;
	}

	return 0;
}


void compare_logFile2(const array<path, 2> &log_files) noexcept {
	/* for each file, read-in lines, and compute line-hash, using multi-thread with async() */
	auto asyn_Left = async(launch::async, compute_line_hash2, log_files.at(_left__));
	auto asynRight = async(launch::async, compute_line_hash2, log_files.at(_right_));

	const auto hash_result_matrix = array<pair<LineHashes, MAP_Hash_to_LineInfo2>, 2> //left, right
		{asyn_Left.get(), asynRight.get()};

	/* use set-MATH-operation to pick out diff lines(hashes) */
	const auto uniq_hashes = array<LineHashes, 2>{
		/*uniq_hashes[_left__] =*/set_diff(hash_result_matrix[_left__].first, hash_result_matrix[_right_].first),
		/*uniq_hashes[_right_] =*/set_diff(hash_result_matrix[_right_].first, hash_result_matrix[_left__].first)};

	/* process/sort/print the result */
	const auto myLogger = spdlog::get(LoggerName);
	for (const auto i : {_left__, _right_}) // print left unique lines, then right.
	{
		/* get lines content from hash */
		MAP_Hash_to_LineInfo2 myMap = hash_result_matrix[i].second;
		const auto unique_lines = getLinesFromHash2(uniq_hashes[i], log_files[i], myMap);


		/* filter with regex */
		const auto reg_span = gsl::make_span(Raw_filter_list);
		const auto filtered_lines = regexFilterLines2(unique_lines, reg_span);

		/* print the result */
		const auto my_mark = string{prefixMarks[i]};    // "+" or "-"
		const auto prefix3 = string(3, prefixMarks[i]); // +++ or ---

		auto diff_content_subdir = current_path() / (defaultSubDir + to_wstring(i));
		create_directory(diff_content_subdir);

		if (filtered_lines.size() != 0) {
			/* put diff lines in diff_content_file*/
			const auto diff_content_file_path = diff_content_subdir / log_files[i].stem();
			cout << diff_content_file_path.string() << endl; //show its path
			auto diff_content_file = ofstream{diff_content_file_path.string()};

			myLogger->info("{} {} unique lines in {}", prefix3, filtered_lines.size(), log_files[i].string());
			for (const auto &l : filtered_lines) {
				myLogger->debug("{} #{}, {}", my_mark, l.line_number, l.line_content);
				diff_content_file << my_mark << " #" << l.line_number << ", " << l.line_content << endl;
			}
		} else {
			myLogger->info("{} No diff: {}", my_mark, log_files[i].string());
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
}

void compare_dir(const array<path, 2> &dirs, const string &file_type_to_compare) noexcept {
	const auto myLogger = spdlog::get(LoggerName);

	/* do nothing if two same dir given. */
	if (dirs[_left__] == dirs[_right_]) {
		myLogger->info("Two same directories ({}) are given, comparison aborted.", dirs[_left__].string());
		return;
	}

	/* list all the log files, case insensitive */
	const auto files = array<LogFileList, 2>{getFilesFromDir(dirs[_left__], file_type_to_compare),
	                                         getFilesFromDir(dirs[_right_], file_type_to_compare)};

	/* find out unique/common files*/
	const auto uniqueFiles = array<LogFileList, 2>{
		files[_left__] - files[_right_],
		files[_right_] - files[_left__]}; // LogFileList operator-(const LogFileList &setA, const LogFileList &setB)

	const auto filesToCompare =
		array<LogFileList, 2>{files[_left__] - uniqueFiles[_left__], files[_right_] - uniqueFiles[_right_]};

	/* print the dir-list result */
	myLogger->info("Comparing two directories: \n{} {}\n{} {}", prefixMarks[_left__], dirs[_left__].string(),
	               prefixMarks[_right_], dirs[_right_].string());
	for (const auto i : {_left__, _right_}) {
		myLogger->info("\n{} unique files in {}", uniqueFiles[i].size(), dirs[i].string());
		for (const auto &j : uniqueFiles[i]) {
			myLogger->debug("\t{}", j);
		}
	}

	/* compare each file by compare_logFile() */
	myLogger->info("\nComparing {}/{} common files in {} and {}", filesToCompare[_left__].size(),
	               filesToCompare[_right_].size(), dirs[_left__].string(), dirs[_right_].string());

	for (auto i = 0UL; i != filesToCompare[_left__].size(); i++) {
		myLogger->info("\n({}/{})\t{}", i + 1, /* counting from 1, not 0 */
		               filesToCompare[_left__].size(), filesToCompare[_left__].at(i));

		compare_logFile2({dirs[_left__] / filesToCompare[_left__].at(i), // get full-name from dir/file
		                 dirs[_right_] / filesToCompare[_right_].at(i)});
	};
}

void setup_logger(const wstring &diff_record_file /* = "diff_summary.txt"*/) noexcept {
	// Create a logger with multiple sinks
	const auto mySink_console = make_shared<spdlog::sinks::stdout_sink_st>();
	auto mySink_full = make_shared<spdlog::sinks::simple_file_sink_st>(diff_record_file);
	auto mySink_brief = make_shared<spdlog::sinks::simple_file_sink_st>(getBase(diff_record_file) + L"_brief.txt");

	mySink_full->set_level(spdlog::level::debug);
	mySink_brief->set_level(spdlog::level::info);
	const auto mySinks = vector<spdlog::sink_ptr>{mySink_console, mySink_full, mySink_brief};

	auto myLogger = make_shared<spdlog::logger>(LoggerName, begin(mySinks), end(mySinks));
	spdlog::register_logger(myLogger);
	myLogger->set_pattern("%v");
}

pair<LineHashes, MAP_HashAndLine> doHashLines(const path &file) noexcept {
	auto inFile = ifstream{file.string(), ios_base::binary}; // must open as binary for correct position
	auto str_hash = hash<string>{};                          // the hash "function"
	auto hashes = LineHashes{};                              // store the hash result
	auto my_map = MAP_HashAndLine{}; // store the {hash, LineInfo} map; l_hash mapping_to (lineNumber, position)

	/* the initial position and lineNumber */
	auto lineNumber = LineNr{1}; // lineNumber is shown as 1, 2, 3 .... (starting from 1)
	auto position = inFile.tellg();

	/* read lines */
	for (string line; getline(inFile, line);) {
		/* compute hash and store in map and set */
		const auto l_hash = str_hash(line);
		hashes.push_back(l_hash);
		my_map.emplace(l_hash, PAIR_LineInfo{lineNumber, position});

		/* renew lineNumber and position for next iteration */
		++lineNumber;
		position = inFile.tellg();
	}

	sort(hashes.begin(), hashes.end()); // hash must be sorted, for the "set" math operation later.
	return make_pair(hashes, my_map);
}

pair<LineHashes, std::map<HashValue, LineInfo>>
compute_line_hash2(const path &log_file_for_hash) noexcept {
	auto inFile = ifstream{log_file_for_hash.string(), ios_base::binary};   // must open as binary for correct position
	auto hash_function = hash<string>{};
	auto hash_result = LineHashes{};
	auto my_map = MAP_Hash_to_LineInfo2{}; // store the {hash, LineInfo} map; l_hash mapping_to (lineNumber, position)

	/* the initial position and lineNumber */
	int lineNumber = 1; // lineNumber is shown as 1, 2, 3 .... (starting from 1)
	auto position = inFile.tellg();

	/* read lines */
	for (string l; getline(inFile, l);) {
		/* compute hash and store*/
		const auto line_hash = hash_function(l);
		hash_result.push_back(line_hash);
		my_map.emplace(line_hash, LineInfo{lineNumber, position});
		/* renew lineNumber and position for next iteration */
		++lineNumber;
		position = inFile.tellg();
	}

	//TODO : evaluate std::set Vs. sorted-vector, https://stackoverflow.com/a/15638063/3353857, safe-bet: if no freq. insertion, sorted-vector would be faster.
	sort(hash_result.begin(), hash_result.end()); // hash must be sorted, for the "set" math operation later.

	return {hash_result, my_map};
}

DiffResultLines getLinesFromHash(const LineHashes &hashes, const path &file, const MAP_HashAndLine &myMap) noexcept {
	DiffResultLines diffLines{};
	diffLines.reserve(hashes.size()); // reserve spaces, cause we know the size in advance.

	ifstream inFile(file.string(), ios_base::binary); // must open as binary for correct position

	string line{};
	for (const auto u_hash : hashes) {
		const LineNr lineNumber = myMap.at(u_hash).first;
		const Position position = myMap.at(u_hash).second;
		getline(inFile.seekg(position), line); // get line content via position
		diffLines.push_back(make_pair(lineNumber, move(line)));
	}
	return diffLines;
}

DiffResultLines2 getLinesFromHash2(const LineHashes &hashes,
                                   const path &file,
                                   const MAP_Hash_to_LineInfo2 &myMap) noexcept {
	DiffResultLines2 diffLines;
	diffLines.reserve(hashes.size()); // reserve spaces, because we know the size in advance.

	ifstream inFile(file.string(), ios_base::binary); // must open as binary for correct position

	string line{};
	for (const auto u_hash : hashes) {
		const auto lineNumber = myMap.at(u_hash).lineNr;
		const auto position = myMap.at(u_hash).position;
		getline(inFile.seekg(position), line); // get line content via position
		diffLines.push_back(DiffResult2{lineNumber, move(line)});
	}
	return diffLines;
}

template<typename SPAN>
DiffResultLines regexFilterLines(const DiffResultLines &lines, SPAN rawRegexList) noexcept {
	/* get myRegexObj from RAW-list, concatenate lists into one-line with "|" */
	//string regex_whole_line{rawRegexList[0]};
	//for_each(begin(rawRegexList) + 1, end(rawRegexList),
	//         [&regex_whole_line](const string s) { regex_whole_line.append("|").append(s); });

	string whole_line = rawRegexList[0];
	whole_line = std::accumulate(next(begin(rawRegexList)), end(rawRegexList), whole_line,
	                             [](string w_line, string s) {
	                               return w_line.append("|").append(s);
	                             });
	const regex myRegexObj{whole_line};

	/* go through diffLiens, filter out regex-matched lines */
	DiffResultLines filtered_result{};
	for (const auto &l : lines) {
		const bool matchRegex_shouldOmit = regex_search(l.second, myRegexObj);
		if (!matchRegex_shouldOmit) {
			filtered_result.push_back(l);
		}
	}
	sort(filtered_result.begin(), filtered_result.end());

	return filtered_result;
}

template<typename SPAN>
DiffResultLines2 regexFilterLines2(const DiffResultLines2 &lines, SPAN rawRegexList) noexcept {
	/* construct myRegexObj from RAW-list, concatenate lists into one-line with "|" */

	string whole_line = rawRegexList[0];
	whole_line = std::accumulate(next(begin(rawRegexList)), end(rawRegexList), whole_line,
	                             [](string w_line, string s) {
	                               return w_line.append("|").append(s);
	                             });
	const regex myRegexObj{whole_line};

	/* go through diffLiens, filter out regex-matched lines */
	DiffResultLines2 filtered_results{};
	for (const auto &l : lines) {
		const bool regex_matched = regex_search(l.line_content, myRegexObj);
		if (regex_matched) {
			//do nothing, because this line matched the regex
		} else {
			filtered_results.push_back(l); //add to result
		}
	}
	sort(filtered_results.begin(), filtered_results.end(),
	          [](const auto &i, const auto &j) {
	            return i.lineNr < j.lineNr;
	          });

	return filtered_results;
}

LogFileList getFilesFromDir(const path &folder, const string &fileExt) noexcept {
	vector<string> fileList{};
	for (const auto &f : directory_iterator{folder}) {
		if (is_regular_file(f)) {
			if (fileExt.empty() || f.path().extension().string() == ('.' + fileExt)) {
				fileList.push_back(f.path().filename().string());
			}
		}
	}
	sort(fileList.begin(), fileList.end(), iCompString); // case insensitive sort
	return fileList;
}

template<typename OrderedContainer>
OrderedContainer set_diff(const OrderedContainer &setA, const OrderedContainer &setB) noexcept {
	OrderedContainer result;
	set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), inserter(result, result.end()));
	return result;
}

LineHashes operator-(const LineHashes &setA, const LineHashes &setB) noexcept {

	LineHashes result;

	set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), inserter(result, result.end()));

	return result;
}

LogFileList operator-(const LogFileList &listA, const LogFileList &listB) noexcept {

	LogFileList result;

	set_difference(listA.begin(), listA.end(), listB.begin(), listB.end(), back_inserter(result), iCompString);

	return result;
}

myParameter::myParameter(const int argc, const char *const *argv) {
	args::ArgumentParser parser("Compare common-log-files.", RawStr_epilog_CallingExample);
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::ValueFlag<string> output_file(parser, "OUTPUT", "OUTPUT:file to store the diff summary", {'O', "output"});
	args::ValueFlag<string> list_file(
		parser, "LIST",
		"LIST:a file of all the file names (which exist in both dir1 and dir2) to be compared, one name perl line)",
		{'L', "file_list"});

	args::ValueFlag<string> file_ext(
		parser, "EXT", "EXT: which file extension to compare (\"log\" for *.log). If not specified, compare all files",
		{'T', "file_type"});

	args::Positional<string> f1(parser, "f1", "the 1st(left)  file/dir to compare");
	args::Positional<string> f2(parser, "f2", "the 1st(left)  file/dir to compare");

	/* parse parameters */
	try {
		parser.ParseCLI(argc, argv);
	} catch (args::Help) {
		cout << "===========" << endl;
		cout << parser;
		return;
	} catch (args::ParseError e) {
		cerr << "===========" << endl;
		cerr << e.what() << endl;
		cerr << parser;
		return;
	}

	/* get parameter values */
	path_left_.assign(removeLastSlash(f1.Get()));
	path_right.assign(removeLastSlash(f2.Get()));
	listFilePath.assign(list_file.Get());
	outputFilePath.assign(output_file.Get()); // set default first.
	fileType = file_ext.Get();

	/* validate parameters: both f1, f2 should be valid */
	for (auto p : {path_left_, path_right}) {
		if (p.empty()) {
			cerr << "\n! Missing parameters. Both f1 and f2 are needed\n" << endl;
			return;
		}

		if (!exists(p)) {
			cerr << "\n! Can't open " << p << ", \nplease check your parameter.\n" << endl;
			return;
		}
	}

	/* validate parameters: listFilePath(if given) should be valid */
	if (!listFilePath.empty()) {
		if (!exists(listFilePath)) // if given, it should exist.
		{
			cerr << "\n!! -L FILE_LIST: " << listFilePath << " doesn't exist, \nplease check your parameter.\n" << endl;
			return;
		}

		if (is_directory(listFilePath)) // it should not be an directory
		{
			cerr << "\n! -L FILE_LIST: " << listFilePath << " can't be a *directory*, \nplease check your parameter.\n"
			     << endl;
			return;
		}
	}

	/* validate parameters: outputFilePath(if anything wrong, set to default) */
	if (outputFilePath.empty()) // if empty, set to default
	{
		outputFilePath.assign(defaultOutputFile); // set default first.
	} else // if valid given, update to new value
	{
		ofstream f{output_file.Get()};
		if (f.good()) // test valid
		{
			outputFilePath.assign(output_file.Get());
		} else {
			cerr << "\n! Invalid -L OUTPUT:" << output_file.Get() << ", now use default " << defaultOutputFile
			     << " instead\n"
			     << endl;
			outputFilePath.assign(defaultOutputFile); // set default first.
		}
	}

	/* decide how to compare */
	if (is_regular_file(path_left_) && is_regular_file(path_right)) // two files given
	{
		howToCompare = CompareType::file;
		valid = true;
	} else if (is_directory(path_left_) && is_directory(path_right)) // two dir given
	{
		listFilePath.empty() ? howToCompare = CompareType::dir : howToCompare = CompareType::dir_with_list;
		valid = true;
	} else {
		cerr << "\n! Can't compare file Vs. dir, please check your parameter.\n" << endl;
		valid = false;
	}
	return;
}


#if 0
void compare_logFile(const array<path, 2> &files) noexcept {
	/* for each file, read-in lines, and compute line-hash, using multi-thread with ansync() */
	auto asyn_Left = async(launch::async, doHashLines, files.at(_left__));
	auto asynRight = async(launch::async, doHashLines, files.at(_right_));
	const auto hash_result_extended = array<pair<LineHashes, MAP_HashAndLine>, 2>{asyn_Left.get(), asynRight.get()};

	/* use set-MATH-operation to pick out diff lines(hashes) */
	const auto uniq_hashes = array<LineHashes, 2>{
		// LineHashes operator-(const LineHashes &setA, const LineHashes &setB)
		/*uniq_hashes[_left__] =*/hash_result_extended[_left__].first - hash_result_extended[_right_].first,
		/*uniq_hashes[_right_] =*/hash_result_extended[_right_].first - hash_result_extended[_left__].first};

	/* process/sort/print the result */
	const auto myLogger = spdlog::get(LoggerName);
	for (const auto i : {_left__, _right_}) // print left unique lines, then right.
	{
		/* get lines content from hash */
		const auto unique_lines = getLinesFromHash(uniq_hashes[i], files[i], hash_result_extended[i].second);

		const auto reg_span = gsl::make_span(Raw_filter_list);

		/* filter with regex */
		const auto filtered_lines = regexFilterLines(unique_lines, reg_span);

		/* print the result */
		const auto my_mark = string{prefixMarks[i]};    // "+" or "-"
		const auto prefix3 = string(3, prefixMarks[i]); // +++ or ---

		const auto diff_content_subdir = current_path() / (defaultSubDir + to_wstring(i));
		create_directory(diff_content_subdir);

		if (filtered_lines.empty()) {
			/* put diff lines in diff_content_file*/
			const auto diff_content_file_path = diff_content_subdir / files[i].stem();
			cout << diff_content_file_path.string() << endl; //show its path
			auto diff_content_file = ofstream{diff_content_file_path.string()};

			myLogger->info("{} {} unique lines in {}", prefix3, filtered_lines.size(), files[i].string());
			for (const auto &my_line : filtered_lines) {
				myLogger->debug("{} #{}, {}", my_mark, my_line.first, my_line.second); //lineNumber, lineContent
				diff_content_file << my_mark << " #" << my_line.first << ", " << my_line.second << endl;
			}
		} else {
			myLogger->info("{} No diff: {}", my_mark, files[i].string());
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
}

#endif