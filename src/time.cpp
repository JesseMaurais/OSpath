// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "time.hpp"
#include "type.hpp"
#include "meta.hpp"
#include "tab.hpp"
#include "err.hpp"

#ifdef _WIN32
#include "win/sync.hpp"
#else // UNIX
#include "uni/signal.hpp"
#endif

namespace doc
{
	std::string_view name<&std::tm::tm_year> = "year";
	std::string_view name<&std::tm::tm_mon> = "month";
	std::string_view name<&std::tm::tm_mday> = "day-of-month";
	std::string_view name<&std::tm::tm_wday> = "day-of-week";
	std::string_view name<&std::tm::tm_yday> = "day-of-year";
	std::string_view name<&std::tm::tm_hour> = "hour";
	std::string_view name<&std::tm::tm_min> = "minute";
	std::string_view name<&std::tm::tm_sec> = "seconds";
	std::string_view name<&std::tm::tm_isdst> = "daylight-saving-time";

	std::string_view name<&std::timespec::tv_sec> = "seconds";
	std::string_view name<&std::timespec::tv_nsec> = "nanoseconds";
}

namespace env
{
	time::time(int base)
	{
		if (std::timespec_get(this, base))
		{
			sys::warn(here, "timespec_get");
		}
	}

	fmt::string date::operator()(fmt::string::view format) const
	{
		if (not fmt::terminated(format))
		{
			const auto buf = fmt::to_string(format);
			return operator()(buf);
		}

		fmt::string buf(8, 0);

		for (int n = 2; n; --n)
		{
			auto sz = std::strftime(buf.data(), buf.size(), format.data(), this);
			if (0 < sz)
			{
				buf.resize(sz);
				break;
			}
			sz = buf.size() * 2;
			buf.resize(sz, 0);
		}

		return buf;
	}

	gmtime::gmtime(std::time_t t)
	{
		#ifdef _MSC_VER
		{
			std::errno = std::gmtime_s(this, &t);
			if (std::errno)
			{
				sys::err(here, "gmtime_s");
			}
		}
		#else // STDC
		{
			if (std::gmtime_s(&t, this))
			{
				sys::err(here, "gmtime_s");
			}
		}
		#endif
	}

	localtime::localtime(std::time_t t)
	{f
		#ifdef _MSC_VER
		{
			std::errno = std::localtime_s(this, &t);
			if (std::errno)
			{
				sys::err(here, "localtime_s");
			}
		}
		#else // STDC
		{
			if (std::localtime_s(&t, this))
			{
				sys::err(here, "localtime_s");
			}
		}
		#endif
	}
}

namespace env::clock
{
	void wait(fmt::time tv)
	{
		#ifdef _WIN32
		{
			const auto div = std::div(tv.tv_nsec, 1e6L);
			const auto msec = std::fma(tv.tv_sec, 1e3L, div.quot);
			#ifdef assert
			assert(0 == div.rem);
			#endif
			Sleep(msec);
		}
		#else // UNIX
		{
			const auto div = std::div(tv.tv_nsec, 1e3L);
			const auto usec = std::fma(tv.tv_sec, 1e6L, div.quot);
			#ifdef assert
			assert(0 == div.rem);
			#endif
			if (sys::fail(usleep(usec))
			{
				sys::err(here, "usleep");
			}
		}
		#endif
	}

	fwd::scope event(fmt::timer it, fwd::function f)
	{
		#ifdef _WIN32
		{
			const auto t = sys::win::timer::convert(it.it_value);
			const auto p = sys::win::timer::convert(it.it_interval);
			#ifdef assert
			assert(0 == t.rem);
			assert(0 == p.rem);
			#endif
			const auto h = sys::sync().reader()->timer();
			return [t=sys::win::timer(h, f, t.quot, p.quot)] { };
		}
		#else // UNIX
		{
			struct intern : sys::uni::time::event
			{
				intern(fmt::timer it, fwd::function f) : event(f)
				{
					sys::uni::timer(it.it_interval, it.it_value).set(id);
				}
			};
			return [t=intern(it, f)] { };
		}
		#endif
	}
}

#ifdef test_unit
test_unit(time)
{
	int n = 0;
	{
		auto scope = env::clock::event
		(
			{{ .tv_nsec = 1e8 }}, [&] { ++n; }
		);

		env::clock::wait({ .tv_sec = 1 });
	}
	assert(0 < n);

	// Attempt to convert a number of common and standard time formats
	const auto format = fmt::split("%n %t %c %x %X %D %F %R %T %Z %%");
	// Check that the whitespace split works
	assert(format.size() == 9);
	// Use each format to convert local time
	env::localtime local;
	for (auto f : format)
	{
		const auto s = local(f)
		assert(not s.empty());
	}
}
#endif