#pragma once
#include "stdafx.h"
#include <boost/tokenizer.hpp>

constexpr size_t line_width_estimated{ 50 };
/**
 * \brief read file into mmap buffer, and count hash line by line
 * \tparam Container file_conent, could be string, vector<byte>, or gsl::span<byte>
 * \tparam JOBS number of hash-computing jobs
 */
template <typename Container = gsl::span<byte>, int JOBS = 2> //using Container = gsl::span<byte>; //constexpr int JOBS = 2;
class logfile_mmap {

public:
    struct line_hash_result
    {
        size_t hash_value;
        size_t line_number;
        gsl::span<typename Container::value_type> line_content;
    };

    struct block_hash_result
    {
        size_t line_count_offset = 0;
        std::vector<line_hash_result> line_hashes; //number_of_lines = line_hashes.size();
    };

private:
    const std::string file_name;
    boost::interprocess::file_mapping my_mapping1;
    boost::interprocess::mapped_region my_mapped_region1;
    Container content;
    std::array<Container, JOBS> split_content;

public:
    block_hash_result file_hash_result;
    std::array<block_hash_result, JOBS> split_hash_result;
    //block_hash_result split_hash_result[2];
    explicit logfile_mmap(const std::string &isf_file) :
        file_name(isf_file),
        my_mapping1(isf_file.c_str(), boost::interprocess::read_only),
        my_mapped_region1(my_mapping1, boost::interprocess::read_only),
        content(reinterpret_cast<byte *>(my_mapped_region1.get_address()), my_mapped_region1.get_size())
    {
        _do_split();

        const auto start = std::chrono::steady_clock::now();
        {
            auto asyn_hash_p0 = std::async(std::launch::async, _scan_and_hash_buffer, split_content[0]);
            auto asyn_hash_p1 = std::async(std::launch::async, _scan_and_hash_buffer, split_content[1]);
            split_hash_result[0] = asyn_hash_p0.get();
            split_hash_result[1] = asyn_hash_p1.get();
            split_hash_result[1].line_count_offset = 0;
            split_hash_result[1].line_count_offset = split_hash_result[0].line_hashes.size();
        }
        const auto end = std::chrono::steady_clock::now();
        const auto diff_ms = std::chrono::duration <double, std::milli>{ end - start };
        std::cout << "hashing " << split_hash_result[0].line_hashes.size() + split_hash_result[1].line_hashes.size() << " lines, ";
        std::cout << "with 2 threads, ";
        std::cout << "in " << diff_ms.count() << " ms" << std::endl;

        file_hash_result = _scan_and_hash_buffer(content);//(split_content[0]); //(content)

        for (auto &h : split_hash_result[0].line_hashes)
        {
            auto split_l_hash = h.hash_value;
            auto split_l_number = h.line_number + split_hash_result[0].line_count_offset;;
            auto full_l_hash = file_hash_result.line_hashes[split_l_number].hash_value;

            if (split_l_hash != full_l_hash)
            {
                std::cout << "#" << split_l_number << ", hash not equal\t";
                std::cout << split_l_hash << '\t' << full_l_hash << std::endl;
            }
        }

        for (auto &h : split_hash_result[1].line_hashes)
        {
            auto split_l_hash = h.hash_value;
            auto split_l_number = h.line_number + split_hash_result[1].line_count_offset;
            auto full_l_hash = file_hash_result.line_hashes[split_l_number].hash_value;

            if (split_l_hash != full_l_hash)
            {
                std::cout << "#" << split_l_number << ", hash not equal\t";
                std::cout << split_l_hash << '\t' << full_l_hash << std::endl;
            }
        }


    }
private:
    /**
     * \brief split content in two, at the line boundary
     */
    void _do_split()
    {
        const auto temp_middle_itr = content.begin() + content.size() / 2;
        const auto line_boundary_itr = std::next(std::find(temp_middle_itr, content.end(), '\n'));
        const auto boundary_offset = std::distance(content.begin(), line_boundary_itr);
        split_content[0] = content.first(boundary_offset);
        split_content[1] = content.last(content.size() - boundary_offset);
    }

    static block_hash_result _scan_and_hash_buffer(Container buffer)
    {
        block_hash_result hash_result;
        //hash_result.line_count_offset = line_offset;
        hash_result.line_hashes.reserve(buffer.size() / line_width_estimated);

        auto tmp_line_str = std::string{};
        tmp_line_str.reserve(1024);
        size_t line_number = 0; //starting from 0

        for (auto line_begin_itr = buffer.begin(); line_begin_itr != buffer.end();)
        {
            const auto line_end_itr = std::find(line_begin_itr, buffer.end(), '\n');
            hash_result.line_hashes.push_back(line_hash_result{
                    std::hash<std::string>{}(std::string(line_begin_itr, line_end_itr)),// hash_value
                    line_number++,                                                      // line_number
                    Container(&line_begin_itr[0], line_end_itr - line_begin_itr) }      // line_content
            );

            if (line_end_itr != buffer.end()) {
                line_begin_itr = std::next(line_end_itr);
            }
            else
            {
                line_begin_itr = buffer.end();
            }
        }
        return hash_result;
    }
};

#if 0
//logfile_mmap() = delete;
//~logfile_mmap() = default;
//logfile_mmap(const logfile_mmap &) = delete;
//logfile_mmap &operator=(const logfile_mmap &) = delete;
//logfile_mmap(logfile_mmap &&) = default;
//logfile_mmap &operator=(logfile_mmap &&) = default;


inline void tt()
{
    using namespace std;

    const auto start = chrono::steady_clock::now();
    {
        cout << "system_clock" << endl;
        cout << "chrono::system_clock::period::num = " << chrono::system_clock::period::num << endl;
        cout << "chrono::system_clock::period::den = " << chrono::system_clock::period::den << endl;
        cout << "steady = " << boolalpha << chrono::system_clock::is_steady << endl << endl;

        cout << "high_resolution_clock" << endl;
        cout << "chrono::high_resolution_clock::period::num = " << chrono::high_resolution_clock::period::num << endl;
        cout << "chrono::high_resolution_clock::period::den = " << chrono::high_resolution_clock::period::den << endl;
        cout << "steady = " << boolalpha << chrono::high_resolution_clock::is_steady << endl << endl;

        cout << "steady_clock" << endl;
        cout << "chrono::steady_clock::period::num = " << chrono::steady_clock::period::num << endl;
        cout << "chrono::steady_clock::period::den = " << chrono::steady_clock::period::den << endl;
        cout << "steady = " << boolalpha << chrono::steady_clock::is_steady << endl << endl;
    }
    const auto end = chrono::steady_clock::now();

    // Store the time difference between start and end
    const auto diff = end - start;
    cout << "time diff = " << diff.count() << endl;
    cout << chrono::duration <double, milli>(diff).count() << " ms" << endl;
    cout << chrono::duration <double, nano>(diff).count() << " ns" << endl;
}

{
    const auto start = std::chrono::steady_clock::now();
    file_hash_result = _scan_and_hash_buffer(content);//(split_content[0]); //(content)
    const auto end = std::chrono::steady_clock::now();
    const auto diff_ms = std::chrono::duration <double, std::milli>{ end - start };
    std::cout << "hashing " << file_hash_result.line_hashes.size() << " lines ";
    std::cout << "in " << diff_ms.count() << " ms" << std::endl;
}

#endif