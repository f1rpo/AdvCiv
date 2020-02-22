#pragma once

#ifndef TYPE_CHOICE_H
#define TYPE_CHOICE_H

/*	advc.fract: Metaprogramming code to replace C++11 std::conditional, std::common_type.
	If more such functionality is needed, boost/mpl could help. */

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
template<typename IntType1, typename IntType2> struct choose_int_type { typedef int type; };
// Prefer int over all other types; also covers enum types.
template<typename IntType2> struct choose_int_type<int, IntType2>
{
	typedef int type;
	BOOST_STATIC_ASSERT(sizeof(IntType2) <= 4);
};
template<typename IntType1> struct choose_int_type<IntType1, int>
{
	typedef int type;
	BOOST_STATIC_ASSERT(sizeof(IntType1) <= 4);
};
// Prefer all other types over unsigned char
template<typename IntType1> struct choose_int_type<IntType1, unsigned char>
{
	typedef IntType1 type;
};
template<typename IntType2> struct choose_int_type<unsigned char, IntType2>
{
	typedef IntType2 type;
};
// Handle the remaining combinations through full specialization
// Helper macro
#define _INT_TYPE_CHOICE template<> struct choose_int_type
// int,int and unsigned char,unsigned char to avoid ambiguity
_INT_TYPE_CHOICE<int,            int>            { typedef int            type; };
_INT_TYPE_CHOICE<unsigned char,  unsigned char>  { typedef unsigned char  type; };
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

#endif
