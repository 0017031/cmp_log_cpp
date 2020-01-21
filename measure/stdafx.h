// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once


//#include <algorithm>
//#include <experimental/filesystem>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <future>

//#include <fstream>
//#include <string>
//#include <utility>



#include <gsl/gsl>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
// https://www.boost.org/doc/libs/1_67_0/doc/html/interprocess/sharedmemorybetweenprocesses.html#interprocess.sharedmemorybetweenprocesses.mapped_file
//#include <boost/tokenizer.hpp>
//#include <boost/endian/conversion.hpp>

#define CATCH_CONFIG_ALL_PARTS
#include "catch.hpp"

using byte = char;
#define UNREFERENCED_PARAMETER(x) (x)
