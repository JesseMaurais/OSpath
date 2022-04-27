#ifndef time_hpp
#define time_hpp "Time & Date Format"

#include "fwd.hpp"
#include "fmt.hpp"
#include "tmp.hpp"
#include "ptr.hpp"
#include <ctime>
#include <cmath>
#include <tuple>

namespace fmt
{
	struct date : fwd::zero<std::tm>
	{
		static constexpr std::tuple table
		(
			&std::tm::tm_year,
			&std::tm::tm_mon,
			&std::tm::tm_mday,
			&std::tm::tm_wday,
			&std::tm::tm_yday,
			&std::tm::tm_hour,
			&std::tm::tm_min,
			&std::tm::tm_sec,
			&std::tm::tm_isdst
		);
	};

	struct time : fwd::zero<std::timespec>
	{
		static constexpr std::tuple table
		(
			&std::timespec::tv_sec,
			&std::timespec::tv_nsec
		);
	};

	struct timer
	{
		time it_interval;
		time it_value;

		static constexpr std::tuple table
		(
			&timer::it_interval,
			&timer::it_value
		);
	};
}

namespace env
{
	struct time : fmt::time
	{
		time(int = TIME_UTC);
		/// Convert the current time
		using time::time;
	};

	struct date : fmt::date
	{
		fmt::string operator()(fmt::string::view) const;
		/// Convert to string using format given like strftime
		using date::date;
	};

	struct gmtime : date
	{
		gmtime(std::time_t = std::time(nullptr));
		/// Convert to global from time point
		using date::date;
	};

	struct localtime : date
	{
		localtime(std::time_t = std::time(nullptr));
		/// Convert to local from time point
		using date::date;
	};
}

namespace env::clock
{
	void wait(fmt::time);
	/// Suspend execution for amount of time

	fwd::scope event(fmt::timer, fwd::function);
	/// Invoke function periodically on a timer
}

#endif // file
