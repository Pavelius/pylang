#pragma once

#define FG(V) (1<<V)
#define FGT(C, V) (C&FG(V))!=0

// Abstract flag set
template<unsigned N, typename T = unsigned char>
class flagable {
	static constexpr unsigned s = sizeof(T) * 8;
	T data[N];
public:
	constexpr explicit operator bool() const { for(auto e : data) if(e) return true; return false; }
	constexpr flagable() : data{0} {}
	constexpr void add(const flagable<N, T>& v) { for(unsigned i = 0; i < N; i++) data[i] |= v.data[i]; }
	constexpr void clear() { for(unsigned i = 0; i < N; i++) data[i] = 0; }
	constexpr bool is(short unsigned v) const { return (data[v / s] & (1 << (v % s))) != 0; }
	constexpr void remove(short unsigned v) { data[v / s] &= ~(1 << (v % s)); }
	constexpr void set(short unsigned v) { data[v / s] |= 1 << (v % s); }
	constexpr void set(short unsigned v, bool b) { b ? set(v) : remove(v); }
};
// Abstract flag set partial
template<typename T>
class flagable<1, T> {
	T data;
public:
	constexpr explicit operator bool() const { return data != 0; }
	constexpr flagable() : data(0) {}
	constexpr flagable(T data) : data(data) {}
	constexpr void add(const flagable<1, T>& v) { data |= v.data; }
	constexpr void clear() { data = 0; }
	constexpr bool is(short unsigned v) const { return (data & (1 << v)) != 0; }
	constexpr void remove(short unsigned v) { data &= ~(1 << v); }
	constexpr void set(short unsigned v) { data |= (1 << v); }
	constexpr void set(short unsigned v, bool b) { b ? set(v) : remove(v); }
};
typedef flagable<1, unsigned long long> flag64;
typedef flagable<1, unsigned> flag32;
typedef flagable<1, unsigned short> flag16;
typedef flagable<1, unsigned char> flag8;