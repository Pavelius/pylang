#pragma once

#ifdef __GNUC__
using size_t = decltype(sizeof(0));
#endif

typedef int(*fncompare)(const void*, const void*);
typedef int(*fngetnum)(const void* object);

typedef bool(*fnallow)(const void* object, int index);
typedef void(*fncommand)(void* object);
typedef void(*fnevent)(); // Callback function of any command executing
typedef bool(*fnvisible)(const void* object);
typedef bool(*fncondition)();
typedef const char*(*fngetname)(const void* object);

extern "C" int atexit(void(*)()) noexcept;
extern "C" void exit(int exit_code) noexcept;
extern "C" void* bsearch(const void* key, const void* base, size_t num, size_t size, fncompare proc);
extern "C" void* memchr(const void* ptr, int value, long unsigned num);
extern "C" void* memcpy(void* destination, const void* source, size_t size) noexcept(true);
extern "C" int memcmp(const void* p1, const void* p2, size_t size) noexcept(true);
extern "C" void* memmove(void* destination, const void* source, size_t size) noexcept(true);
extern "C" void* memset(void* destination, int value, size_t size) noexcept(true);
extern "C" void	qsort(void* base, size_t num, size_t size, fncompare proc);

template<class T> struct slice {
	T* data;
	size_t count;
	typedef T data_type;
	constexpr slice() : data(0), count(0) {}
	template<size_t N> constexpr slice(T(&v)[N]) : data(v), count(N) {}
	constexpr slice(T* data, unsigned count) : data(data), count(count) {}
	constexpr slice(T* p1, const T* p2) : data(p1), count(p2 - p1) {}
	explicit operator bool() const { return count != 0; }
	constexpr T* begin() const { return data; }
	constexpr T* end() const { return data + count; }
	constexpr unsigned size() const { return count; }
};
