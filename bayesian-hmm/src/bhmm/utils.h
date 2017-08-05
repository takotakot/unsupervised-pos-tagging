#pragma once
#include <boost/python.hpp>
#include <unordered_map>
#include <vector>

namespace utils{
	template<class T>
	boost::python::list list_from_vector(std::vector<T> &vec);
	void split_word_by(const std::wstring &str, wchar_t delim, std::vector<std::wstring> &word_str_vec);
}