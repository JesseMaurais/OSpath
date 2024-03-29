#ifndef sync_hpp
#define sync_hpp "Synchronize Threads"

#include "err.hpp"
#include "ptr.hpp"

#ifdef _WIN32
#include "win/sync.hpp"
#else
#include "uni/pthread.hpp"
#endif

namespace sys
{

#ifdef _WIN32

	using thread = win::start;
	using join = win::join;

	struct mutex : private win::critical_section
	{
		auto key()
		{
			enter();
			return fwd::make_shared<mutex>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				leave();
			});
		}
	};

	struct rwlock : private win::srwlock
	{
		auto reader()
		{
			lock();
			return fwd::make_shared<rwlock>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				unlock();
			});
		}

		auto writer()
		{
			xlock();
			return fwd::make_shared<rwlock>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				xunlock();
			});
		}
	};

#else // POSIX

	using thread = uni::start;
	using join = uni::join;

	struct mutex : private uni::mutex
	{
		auto key()
		{
			lock();
			return fwd::make_shared<mutex>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				unlock();
			});
		}
	};

	struct rwlock : private uni::rwlock
	{
		auto reader()
		{
			rdlock();
			return fwd::make_shared<rwlock>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				unlock();
			});
		}

		auto writer()
		{
			wrlock();
			return fwd::make_shared<rwlock>(this, [this](auto that)
			{
				#ifdef assert
				assert(this == that);
				#endif
				unlock();
			});
		}
	};

#endif

	template <class object> class exclusive_ptr : fwd::no_copy
	// Allow one writer but many readers
	{
		mutable rwlock lock;
		object* that;

	public:

		exclusive_ptr(object* ptr) : that(ptr)
		{
			#ifdef assert
			assert(nullptr != ptr);
			#endif
		}

		auto reader() const
		{
			return fwd::make_shared<object>(that, [this, key=lock.reader()](auto that)
			{
				#ifdef assert
				assert(this->that == that);
				#endif
			});
		}

		auto writer()
		{
			return fwd::make_shared<object>(that, [this, key=lock.writer()](auto that)
			{
				#ifdef assert
				assert(this->that == that);
				#endif
			});
		}

		auto get() const
		{
			return that;
		}

		auto get()
		{
			return that;
		}
	};

	template <class object> struct exclusive final : private object
	// Self exclusion object
	{
		exclusive_ptr<object> that;

		exclusive() : that(this)
		{ }

		auto reader() const
		{
			return that.reader();
		}

		auto writer()
		{
			return that.writer();
		}

		auto get() const
		{
			return that.get();
		}

		auto get()
		{
			return that.get();
		}
	};
}

#endif
