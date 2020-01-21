#include "stdafx.h"
#undef TWOBLUECUBES_SINGLE_INCLUDE_CATCH_HPP_INCLUDED
#define CATCH_CONFIG_IMPL_ONLY
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "2.read_file_by_getline.h"
TEST_CASE("2.read_file_by_getline", "[getline]") {
    const auto filename = std::string{
        R"(C:\Users\baic\Box Sync\work\Diff_Tool_validation_100029-281\AccuRev\old\CommonTools\CommonLogFile\CommonEventLibrary\Test\TestData\ContainsHugeEvents.log)" };
        //R"(C:\Users\baic\Box Sync\work\Diff_Tool_validation_100029-281\AccuRev\old\CommonTools\CommonLogFile\CommonEventLibrary\Test\TestData\t0.log)" };
    const logfile_hashed myLog{ filename };

    REQUIRE(myLog.file_hash_result.line_count_offset == 0);
    REQUIRE(myLog.file_hash_result.line_hashes.size() == 16);
}


#if 0
#include "1.mapping_whole_file.h"

TEST_CASE("1.mmap.check-size", "[load]") {
    const auto filename = std::string{
        R"(C:\Users\baic\Box Sync\work\Diff_Tool_validation_100029-281\AccuRev\old\CommonTools\CommonLogFile\CommonEventLibrary\Test\TestData\ContainsHugeEvents.log)" };
    //R"(C:\Users\baic\Box Sync\work\Diff_Tool_validation_100029-281\AccuRev\old\CommonTools\CommonLogFile\CommonEventLibrary\Test\TestData\t0.log)" };

    const logfile_mmap<> myLog{ filename };

    auto total_size = myLog.split_hash_result[0].line_hashes.size() + myLog.split_hash_result[1].line_hashes.size();

    REQUIRE(total_size == 1009830);
    //const auto last_char_offset = myLog.split_content[0].size() - 1;
    //REQUIRE(myLog.split_content[0](last_char_offset) == '\n');
}
#endif