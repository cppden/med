//#pragma once
#include <boost/preprocessor/facilities/overload.hpp>

#ifndef __MED_MACROS_ON_HPP_INCLUDED__
#define __MED_MACROS_ON_HPP_INCLUDED__
#undef __MED_MACROS_OFF_HPP_INCLUDED__


#define MED_BIT_MASK(bits) ((1u << (bits)) - 1)

/**
 * Define setter and getter for a bit
 */
#define MED_BIT_ACCESS_DEF(NAME, BIT, SETTER, GETTER) \
	bool NAME() const { return this->GETTER() & (1 << BIT); } \
	void NAME(bool v) { this->SETTER( v ? (this->GETTER() | (1 << BIT)) : (this->GETTER() & ~(1 << BIT)) ); } \
/* MED_BIT_ACCESS_DEF */
#define MED_BIT_ACCESS_DEF_1(NAME)                      MED_BIT_ACCESS_DEF(NAME, 0, set, get)
#define MED_BIT_ACCESS_DEF_2(NAME, BIT)                 MED_BIT_ACCESS_DEF(NAME, BIT, set, get)
#define MED_BIT_ACCESS_DEF_3(NAME, BIT, SETTER)         MED_BIT_ACCESS_DEF(NAME, BIT, SETTER, get)
#define MED_BIT_ACCESS_DEF_4(NAME, BIT, SETTER, GETTER) MED_BIT_ACCESS_DEF(NAME, BIT, SETTER, GETTER)

#define MED_BIT_ACCESSOR(...) BOOST_PP_OVERLOAD(MED_BIT_ACCESS_DEF_, __VA_ARGS__)(__VA_ARGS__)

/**
 * Define setter and getter for number of BITS starting from leaDEF_st significant at SHIFT
 */
#define MED_BITS_ACCESS_DEF(NAME, BITS, SHIFT, SETTER, GETTER) \
	value_type NAME() const { return (this->GETTER() >> SHIFT) & MED_BIT_MASK(BITS); } \
	void NAME(value_type v) { this->SETTER( (this->GETTER() & ~(MED_BIT_MASK(BITS) << SHIFT)) | ((v & MED_BIT_MASK(BITS)) << SHIFT) ); } \
/* MED_BITS_ACCESS_DEF */
#define MED_BITS_ACCESS_DEF_2(NAME, BITS)                 MED_BITS_ACCESS_DEF(NAME, BITS, 0, set, get)
#define MED_BITS_ACCESS_DEF_3(NAME, BITS, SHIFT)          MED_BITS_ACCESS_DEF(NAME, BITS, SHIFT, set, get)
#define MED_BITS_ACCESS_DEF_4(NAME, BITS, SHIFT, SETTER)          MED_BITS_ACCESS_DEF(NAME, BITS, SHIFT, SETTER, get)
#define MED_BITS_ACCESS_DEF_5(NAME, BITS, SHIFT, SETTER, GETTER) MED_BITS_ACCESS_DEF(NAME, BITS, SHIFT, SETTER, GETTER)
#define MED_BITS_ACCESSOR(...) BOOST_PP_OVERLOAD(MED_BITS_ACCESS_DEF_, __VA_ARGS__)(__VA_ARGS__)


/**
 * Define setter and getter for least significant BITS ORing with MASK in setter
 */
#define MED_BITS_ACCESSOR_MASK(NAME, BITS, MASK) \
	value_type NAME() const { return this->get() & MED_BIT_MASK(BITS); } \
	void NAME(value_type v) { this->set( (this->get() & ~MED_BIT_MASK(BITS)) | MASK | (v & MED_BIT_MASK(BITS)) ); } \
/* MED_BITS_ACCESSOR_MASK */


#endif //__MED_MACROS_ON_HPP_INCLUDED__

