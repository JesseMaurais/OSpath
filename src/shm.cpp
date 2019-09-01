// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "shm.hpp"
#include "sys.hpp"
#include "err.hpp"
#include "pos.hpp"

namespace sys::file
{
	bool memory::open(char const* name, int fd, ssize_t sz, size_t off, int mode, int type, void* ptr)
	{
		#ifdef _WIN32
		{
			(void) type;

			DWORD flags = 0;
			DWORD prot = 0;

			if (mode & run)
			{
				flags |= FILE_MAP_EXECUTE;

				if (mode & write)
				{
					prot = PAGE_EXECUTE_READWRITE;
					flags |= FILE_MAP_WRITE;
				}
				else
				{
					prot = PAGE_EXECUTE_READ;
				}

				if (mode & read)
				{
					flags |= FILE_MAP_READ;
				}
			}
			else
			{
				if (mode & write)
				{
					prot = PAGE_READWRITE;
					flags |= FILE_MAP_WRITE;
				}
				else
				{
					prot = PAGE_READONLY;
				}

				if (mode & read)
				{
					flags |= FILE_MAP_READ;
				}
			}

			auto const lo = LOWORD(off);
			auto const hi = HIWORD(off);
			
			sys::win::handle const h = CreateFileMapping
			(
				sys::win::get(fd),
				nullptr,
				prot,
				hi,
				lo,
				name
			);

			if (sys::win::fail(h))
			{
				sys::win::err(here, "CreateFileMapping");
				return true;
			}

			ptr = MapViewOfFile(h, flags, hi, lo, sz < 0 ? 0 : sz);
			if (nullptr == ptr)
			{
				sys::win::err(here, "MapViewOfFile");
				return true;
			}
		}
		#else
		{
			int prot = 0;
			if (mode & run) prot |= PROT_EXEC;
			if (mode & read) prot |= PROT_READ;
			if (mode & write) prot |= PROT_WRITE;

			int flags = 0;
			if (type & share) flags |= MAP_SHARED;
			if (type & privy) flags |= MAP_PRIVATE;
			if (type & fixed) flags |= MAP_FIXED;

			if (sys::fail(fd))
			{
				int oflags = 0;
				if (mode & read)
				{
					if (mode & write)
					{
						oflags |= O_RDWR;
					}
					else
					{
						oflags |= O_RDONLY;
					}
				}
				else
				{
					if (mode & write)
					{
						oflags |= O_WRONLY;
					}
				}

				sys::mode const um;
				fd = shm_open(name, oflags, um);
				if (sys::fail(fd))
				{
					sys::err(here, "shm_open", name);
					return true;
				}

				if (sys::fail(ftruncate(fd, sz)))
				{
					sys::err(here, "ftruncate", name);
					return true;
				}
			}

			if (sz < 0)
			{
				class sys::stat st(fd);
				if (sys::fail(st))
				{
					sys::err(here, "stat");
					return true;
				}
				sz = st.st_size;
			}

			ptr = mmap(ptr, sz, prot, flags, fd, off);
			if (MAP_FAILED == ptr)
			{
				sys::err(here, "mmap");
				return true;
			}
		}
		#endif

		length = sz;
		address = ptr;
		if (nullptr != name)
		{
			path = name;
		}
		return false;
	}

	bool memory::close()
	{
		if (nullptr != address)
		{
			auto const ptr = address;
			address = nullptr;
			auto const name = move(path);
			path.clear();

			#ifdef _WIN32
			{
				if (not UnmapViewOfFile(ptr))
				{
					sys::win::err(here, "UnmapViewOfFile", name);
					return true;
				}
			}
			#else
			{
				if (sys::fail(munmap(ptr, length)))
				{
					sys::err(here, "munmap", name);
					return true;
				}

				if (not empty(path))
				{
					auto const s = name.c_str();
					if (sys::fail(shm_unlink(s)))
					{
						sys::err(here, "shm_unlink", name);
						return true;
					}
				}
			}
			#endif
		}
		return false;
	}
}