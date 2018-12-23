#ifndef fmt_hpp
#define fmt_hpp

#include <locale>
#include <codecvt>
#include <string>
#include <sstream>
#include <iterator>
#include <cassert>
#include "alg.hpp"
#include "fs.hpp"

namespace fmt
{
	// String converters for generic programming

	template <typename T>
	inline std::string to_string(T const &x)
	{
		return std::to_string(x);
	}

	template <>
	inline std::string to_string(std::string const &s)
	{
		return s;
	}

	template <>
	inline std::string to_string(std::string_view const &s)
	{
		return std::string(s.data(), s.size());
	}

	template <>
	inline std::string to_string(char const * const &s)
	{
		return to_string(std::string_view(s));
	}

	template <>
	inline std::string to_string(char * const &s)
	{
		return to_string(std::string_view(s));
	}

	template <>
	inline std::string to_string(char const &c)
	{
		return std::string(1, c);
	}

	template <>
	inline std::string to_string(std::wstring const &w)
	{
		using utf8 = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<utf8> convert;
		return convert.to_bytes(w);
	}

	template <>
	inline std::string to_string(std::wstring_view const &w)
	{
		using utf8 = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<utf8> convert;
		return convert.to_bytes(w.data(), w.data() + w.size());
	}

	template <>
	inline std::string to_string(wchar_t const * const &s)
	{
		return to_string(std::wstring_view(s));
	}

	template <>
	inline std::string to_string(wchar_t * const &s)
	{
		return to_string(std::wstring_view(s));
	}

	template <>
	inline std::string to_string(wchar_t const &c)
	{
		return to_string(std::wstring(1, c));
	}

	template <>
	inline std::string to_string(sys::file::path const &p)
	{
		return to_string(p.string());
	}

	// Custom terminal string view types

	struct string_view : std::string_view
	{
		string_view(std::string_view const& view)
		{
			assert('\0' == view[view.size()]);
		}
	};

	using string_span = std::vector<string_view>;
	using string = std::string;

	inline bool empty(string const& s)
	{
		return s.empty();
	}

	inline bool empty(string_view s)
	{
		return s.empty();
	}

	inline bool empty(string_span s)
	{
		return s.empty() or stl::all_of(s, empty);
	}

	// Basic string formatting tools

	inline void replace(string &buf, string_view s, string_view r)
	{
		using size_type = string_view::size_type;
		constexpr size_type end = string::npos;
		for (auto at = buf.find(s.data(), 0, s.size()); at != end; at = buf.find(s.data(), at + r.size(), s.size()))
		{
			buf.replace(at, s.size(), r.data(), r.size());
		}
	}

	inline string_span split(string_view s, string_view del)
	{
		string_span tok;
		using size_type = string_view::size_type;
		constexpr size_type end = string_view::npos;
		for (size_type at = 0, to = s.find_first_of(del); at != end; to = s.find_first_of(del, at))
		{
			if (to != at)
			{
				tok.emplace_back(s.substr(at, to - at));
			}
			at = s.find_first_not_of(del, to);
		}
		return tok;
	}

	template <typename Container>
	inline string join(Container const& tok, string const& del)
	{
		std::stringstream stream;
		auto it = std::ostream_iterator<std::string>(stream, del.c_str());
		std::copy(std::begin(tok), std::end(tok), it);
		return stream.str();
	}

	inline string to_upper(string_view s)
	{
		stl::transform(s, [](char c) { return std::toupper(c, std::locale()); });
		return s;
	}

	inline string to_lower(string_view s)
	{
		stl::transform(s, [](char c) { return std::tolower(c, std::locale()); });
		return s;
	}

	inline string::iterator trim_begin(string &s)
	{
		constexpr auto isblank = [](char c) { return std::isblank(c, std::locale()); };
		return s.erase(begin(s), stl::find_if(s, isblank));
	}

	inline string::iterator trim_end(string &s)
	{
		constexpr auto isblank = [](char c) { return std::isblank(c, std::locale()); };
		return s.erase(stl::find_if_not(s, isblank), end(s));
	}

	inline bool trim(string &s)
	{
		return trim_begin(s) != trim_end(s);
	}

	// String formatting with inline tags

	class format
	{
	public:

		format(std::string_view s, std::string_view begin_tag="{", std::string_view end_tag="}")
			: buffer(to_string(s))
			, begin(begin_tag)
			, end(end_tag)
			, index(0)
		{}

		template <typename T> format& operator % (T&& arg)
		{
			replace(buffer, next_tag(), to_string(arg));
			return *this;
		}

		operator std::string_view() const
		{
			return buffer;
		}

		operator std::string()
		{
			return buffer;
		}

	private:

		std::string buffer, begin, end;
		std::string::size_type index;

		std::string next_tag()
		{
			std::stringstream tag;
			tag << begin << ++index << end;
			return tag.str();
		}
	};

	inline string quote(string_view string)
	{
		return format("\"{1}\"") % string;
	}

	inline string key_value(string_view key, string_view value)
	{
		return format("{1}={2}") % key % value;
	}

	inline std::pair<std::string_view, std::string_view> key_value(std::string_view s)
	{
		auto const pair = split(s, "=");
		if (not pair.empty())
		{
			auto &key = pair.front();
			auto &value = pair.back();
			return std::pair(key, value);
		}
		return std::pair("", "");
	}
}

#endif // file