#include "stdafx.h"
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

//unsigned int Factorial( unsigned int number ) {
//    return number <= 1 ? number : Factorial(number-1)*number;
//}
//
//TEST_CASE( "Factorials are computed", "[factorial]" ) {
//    REQUIRE( Factorial(1) == 1 );
//    REQUIRE( Factorial(2) == 2 );
//    REQUIRE( Factorial(3) == 6 );
//    REQUIRE( Factorial(10) == 3628800 );
//}

#include "1.mapping_whole_file.h"

TEST_CASE("1.load.mmap", "[load]") {
    const auto filename = std::string{
            R"(C:\Users\baic\Box Sync\work\Diff_Tool_validation_100029-281\AccuRev\old\CommonTools\CommonLogFile\CommonEventLibrary\Test\TestData\ContainsHugeEvents.log)" };
    const logfile_mmap mm{ filename };

    REQUIRE(mm.isf_span.size() == 59221277 );
}
//int main()
//{
//    return 0;
//}