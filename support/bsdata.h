#pragma once

#include "array.h"

#ifdef _MSC_VER
#define	BSDATATMPL
#else
#define	BSDATATMPL template<>
#endif

#define	lenghtof(C) (sizeof(C)/sizeof(C[0]))
#define	FO(T,R) ((size_t)&((T*)0)->R)

#define BSDATA(e) BSDATATMPL e bsdata<e>::elements[]
#define BSDATAD(e) BSDATATMPL array bsdata<e>::source(sizeof(e));
#define BSDATAE(e) BSDATATMPL array bsdata<e>::source(bsdata<e>::elements, sizeof(bsdata<e>::elements[0]), 0, lenghtof(bsdata<e>::elements));
#define BSDATAF(e) BSDATATMPL array bsdata<e>::source(bsdata<e>::elements, sizeof(bsdata<e>::elements[0]), lenghtof(bsdata<e>::elements), lenghtof(bsdata<e>::elements));
#define BSDATAC(e, c) BSDATATMPL e bsdata<e>::elements[c] = {}; BSDATAE(e)
#define NOBSDATA(e) template<> struct bsdata<e> : bsdata<int> {};
#define assert_enum(e, last) static_assert(sizeof(bsdata<e>::elements) / sizeof(bsdata<e>::elements[0]) == static_cast<int>(last) + 1, "Invalid count of " #e " elements"); BSDATAF(e)

template<typename T>
struct bsdata {
	static T			elements[];
	static array		source;
	static constexpr array*	source_ptr = &source;
	static T*			add() { return (T*)source.add(); }
	static T*			addz() { for(auto& e : bsdata<T>()) if(!e) return &e; return add(); }
	static constexpr bool have(const void* p) { return source.have(p); }
	static T*			find(const char* id) { return (T*)source.findv(id, 0); }
	static constexpr T&	get(int i) { return begin()[i]; }
	static constexpr T*	get(const void* p) { return have(p) ? (T*)p : 0; }
	static constexpr T*	begin() { return (T*)source.data; }
	static constexpr T*	end() { return (T*)source.data + source.getcount(); }
	static constexpr T*	ptr(short unsigned i) { return (i == 0xFFFF) ? 0 : (T*)source.ptr(i); }
};
template<> struct bsdata<int> { static constexpr array* source_ptr = 0; };
NOBSDATA(unsigned)
NOBSDATA(short)
NOBSDATA(unsigned short)
NOBSDATA(char)
NOBSDATA(unsigned char)
NOBSDATA(const char*)
NOBSDATA(bool)

template<typename T> short unsigned getbsi(const T* v) { return bsdata<T>::source.indexof(v); }
template<typename T> T* getbs(short unsigned v) { return (v==0xFFFF) ? 0 : (T*)bsdata<T>::source.ptr(v); }
