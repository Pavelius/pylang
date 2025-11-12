#pragma once

#include "array.h"

template<class T>
struct vector : public array {
	typedef T data_type;
	constexpr vector() : array(sizeof(T)) {}
	vector(const slice<T>& source) : array(sizeof(T)) { reserve(source.size()); count = source.size(); memcpy(data, source.begin(), count * element_size); }
	~vector() { clear(); }
	constexpr T& operator[](int index) { return ((T*)data)[index]; }
	constexpr const T& operator[](int index) const { return ((T*)data)[index]; }
	constexpr explicit operator bool() const { return count != 0; }
	constexpr operator slice<T>() const { return slice<T>((T*)data, count); }
	T*					add() { auto p = (T*)array::add(); *p = T(); return p; }
	void				add(const T& v) { *((T*)array::add()) = v; }
	constexpr const T*	begin() const { return (T*)data; }
	constexpr T*		begin() { return (T*)data; }
	void				clear() { for(auto& e : *this) e.~T(); count = 0; }
	constexpr const T*	end() const { return (T*)data + count; }
	constexpr T*		end() { return (T*)data + count; }
	constexpr int		indexof(const T* e) const { if(e >= (T*)data && e < (T*)data + count) return e - (T*)data; return -1; }
	constexpr int		indexof(const T t) const { for(auto& e : *this) if(e == t) return &e - (T*)data; return -1; }
	constexpr bool		is(const T* t) const { return t >= data && t < data + count; }
	constexpr T*		ptr(size_t index) const { return (T*)data + index; }
	constexpr T*		ptrs(size_t index) const { return (index < count) ? (T*)data + index : 0; }
};
