#pragma once

#ifndef TYPE_CHOICE_H
#define TYPE_CHOICE_H

/*	advc.fract: Metaprogramming code to replace C++11 std::conditional, std::common_type.
	If more such functionality is needed, boost/mpl could help. */

// Replacement for std::is_same (and boost::is_same)
template<typename T1, typename T2>
struct is_same_type
{
	static const bool value = false;
};
template<typename T>
struct is_same_type<T,T>
{
	static const bool value = true;
};

// Replacement for std::conditional
template<bool bCondition, typename T1, typename T2>
struct choose_type { typedef T1 type; };
template<typename T1, typename T2>
struct choose_type<false,T1,T2> { typedef T2 type; };

/*	Replacement for std::common_type. I need it only for integer types with 4 bytes or fewer.
	Chooses signed over unsigned.
	If both are signed or both unsigned, the larger type is chosen.
	Usage example: typename choose_int_type<short,uint>::type
	(will resolve to short) */
// Default: int; that also covers enum types.
template<typename IntType1, typename IntType2> struct choose_int_type
{
	BOOST_STATIC_ASSERT(sizeof(IntType1) <= 4);
	BOOST_STATIC_ASSERT(sizeof(IntType2) <= 4);
	typedef int type;
};
// Helper macro
#define _INT_TYPE_CHOICE template<> struct choose_int_type
// The first few are necessary to avoid ambiguity
_INT_TYPE_CHOICE<int,            int>            { typedef int            type; };
_INT_TYPE_CHOICE<int,            char>           { typedef int            type; };
_INT_TYPE_CHOICE<char,           int>            { typedef int            type; };
_INT_TYPE_CHOICE<unsigned char,  unsigned char>  { typedef unsigned char  type; };
_INT_TYPE_CHOICE<unsigned char,  char>           { typedef char           type; };
_INT_TYPE_CHOICE<char,           unsigned char>  { typedef char           type; };
_INT_TYPE_CHOICE<unsigned char,  short>          { typedef short          type; };
_INT_TYPE_CHOICE<short,          unsigned char>  { typedef short          type; };
_INT_TYPE_CHOICE<unsigned short, unsigned char>  { typedef unsigned short type; };
_INT_TYPE_CHOICE<unsigned char,  unsigned short> { typedef unsigned short type; };
_INT_TYPE_CHOICE<unsigned int,   unsigned char>  { typedef unsigned int   type; };
_INT_TYPE_CHOICE<unsigned char,  unsigned int>   { typedef unsigned int   type; };
_INT_TYPE_CHOICE<unsigned char,  int>            { typedef int            type; };
_INT_TYPE_CHOICE<int,            unsigned char>  { typedef int            type; };
_INT_TYPE_CHOICE<char,           char>           { typedef char           type; };
_INT_TYPE_CHOICE<char,           short>          { typedef short          type; };
_INT_TYPE_CHOICE<short,          char>           { typedef short          type; };
_INT_TYPE_CHOICE<char,           unsigned short> { typedef char           type; };
_INT_TYPE_CHOICE<unsigned short, char>           { typedef char           type; };
_INT_TYPE_CHOICE<char,           unsigned int>   { typedef char           type; };
_INT_TYPE_CHOICE<unsigned int,   char>           { typedef char           type; };
_INT_TYPE_CHOICE<short,          short>          { typedef short          type; };
_INT_TYPE_CHOICE<short,          unsigned short> { typedef short          type; };
_INT_TYPE_CHOICE<unsigned short, short>          { typedef short          type; };
_INT_TYPE_CHOICE<short,          unsigned int>   { typedef short          type; };
_INT_TYPE_CHOICE<unsigned int,   short>          { typedef short          type; };
_INT_TYPE_CHOICE<unsigned short, unsigned short> { typedef unsigned short type; };
_INT_TYPE_CHOICE<unsigned short, unsigned int>   { typedef unsigned int   type; };
_INT_TYPE_CHOICE<unsigned int,   unsigned short> { typedef unsigned int   type; };

// Bigger in terms of sizeof; tiebreaker: treat signed as bigger than unsigned.
template<typename IntType1, typename IntType2> struct choose_bigger_int_type
{
	BOOST_STATIC_ASSERT(sizeof(IntType1) <= 4);
	BOOST_STATIC_ASSERT(sizeof(IntType2) <= 4);
	typedef int type;
};
template<> struct choose_bigger_int_type<short,          short>          { typedef short type; };
template<> struct choose_bigger_int_type<short,unsigned  short>          { typedef short type; };
template<> struct choose_bigger_int_type<unsigned short, short>          { typedef short type; };
template<> struct choose_bigger_int_type<unsigned short, unsigned short> { typedef unsigned short type; };
template<> struct choose_bigger_int_type<short,          char>           { typedef short type; };
template<> struct choose_bigger_int_type<short,          unsigned char>  { typedef short type; };
template<> struct choose_bigger_int_type<char,           short>          { typedef short type; };
template<> struct choose_bigger_int_type<unsigned char,  short>          { typedef short type; };
template<> struct choose_bigger_int_type<unsigned short, char>           { typedef unsigned short type; };
template<> struct choose_bigger_int_type<unsigned short, unsigned char>  { typedef unsigned short type; };
template<> struct choose_bigger_int_type<char,           unsigned short> { typedef unsigned short type; };
template<> struct choose_bigger_int_type<unsigned char,  unsigned short> { typedef unsigned short type; };
template<> struct choose_bigger_int_type<char,           char>           { typedef char type; };
template<> struct choose_bigger_int_type<char,           unsigned char>  { typedef char type; };
template<> struct choose_bigger_int_type<unsigned char,  char>           { typedef char type; };
template<> struct choose_bigger_int_type<unsigned char,  unsigned char>  { typedef unsigned char type; };

#endif
