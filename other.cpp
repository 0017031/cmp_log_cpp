#include "stdafx.h"
// #include "main.h"

namespace stdfs = std::experimental::filesystem;

using namespace gsl;
using namespace std;
using namespace stdfs;

string getBase(const string &s) noexcept
{
	// path p(s);
	return path{s}.stem().string();
}

bool iCompString(string s0, string s1) noexcept
{ // case insensitive compare
	transform(s0.begin(), s0.end(), s0.begin(), ::tolower);
	transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	return s0 < s1;
}

string &removeLastSlash(string &s) noexcept
{
	if (s.empty())
		return s;

	const auto lastChar{ s.back() };
	// if ('\\' == lastChar || '/' == lastChar)
	if (lastChar == '\\' || lastChar == '/')	
	{
		s.pop_back();
	}
	return s;
}

