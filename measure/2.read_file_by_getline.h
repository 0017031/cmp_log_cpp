#pragma once
#include "stdafx.h"
#include <boost/tokenizer.hpp>
#include <fstream>

constexpr size_t line_width_estimated{ 50 };
/**
 * \brief read file by std::getline, and count hash line by line
 * \tparam Container line_content, could be string
 * \tparam JOBS number of hash-computing jobs
 */
 //template <typename Container = std::string, int JOBS = 2> 
using Container = std::string;
constexpr int JOBS = 2;
class logfile_hashed {

public:
    struct line_with_hash
    {
        size_t hash_value;
        size_t line_number;
        std::string line_content;
    };

    struct hash_result
    {
        size_t line_count_offset = 0;
        std::vector<line_with_hash> line_hashes; //Note: Number_of_lines is line_hashes.size();
    };

private:
    const std::string name;
    const std::experimental::filesystem::path log_path;
    Container content;
    std::array<Container, JOBS> split_content;

public:
    hash_result file_hash_result;
    std::array<hash_result, JOBS> split_hash_result;

    explicit logfile_hashed(const std::string &isf_file) :
        name(isf_file), log_path(isf_file)
    {
        const auto start = std::chrono::steady_clock::now();
        {
            file_hash_result = _readlines_and_hash();
        }
        const auto end = std::chrono::steady_clock::now();
        const auto diff_ms = std::chrono::duration <double, std::milli>{ end - start };
        std::cout << "hashing " << file_hash_result.line_hashes.size() << " lines, ";
        std::cout << "in " << diff_ms.count() << " ms" << std::endl;

    }
private:
    hash_result _readlines_and_hash() const
    {

        auto logfile_istream = std::ifstream{ log_path, std::ios::binary | std::ios::in };
        const auto file_size = std::experimental::filesystem::file_size(log_path);

        auto myResult = hash_result{};
        myResult.line_hashes.reserve(file_size / line_width_estimated);

        auto str = std::string{};
        str.reserve(1024);
        size_t line_count = 0;
        while (std::getline(logfile_istream, str)) {
            myResult.line_hashes.push_back(line_with_hash{
                std::hash<std::string>{}(str),  //hash value
                line_count++,                   //line-number, starting from 0
                std::move(str)                  //line-content
                });
        }

        return myResult;
    }
};

