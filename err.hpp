#ifndef err_hpp
#define err_hpp

#ifdef assert
# undef assert
# warning You should not include assert. 
#endif

#ifdef NDEBUG
# define assert(x)
# define verify(x) (x)
#else
# define verify(x) assert(x)
# define assert(x) if (not(x)) sys::warn(here, #x);
#endif

#include <sstream>
#include "str.hpp"

namespace fmt
{
	struct where
	{
		char const *file; int const line; char const *func;
	};

	std::ostream& operator<<(std::ostream& os, where const& pos);

	template <typename Arg, typename... Args>
	auto err(Arg arg, Args... args)
	{
		std::stringstream ss;
		ss << arg;
		if constexpr (0 < sizeof...(args))
		{
			((ss << " " << args), ...);
		}
		return ss.str();
	}
}

#define here ::fmt::where { __FILE__, __LINE__, __func__ }

namespace sys
{
	extern bool debug;

	namespace impl
	{
		void warn(fmt::string_view);
		void err(fmt::string_view);
	}

	template <typename... Args>
	inline void warn(Args... args)
	{
		if (debug)
		{
			impl::warn(fmt::err(args...));
		}
	}

	template <typename... Args>
	inline void err(Args... args)
	{
		if (debug)
		{
			impl::err(fmt::err(args...));
		}
	}
}

#endif // file
