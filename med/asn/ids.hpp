#pragma once

/**
@file
ASN.1 common constants and identifiers

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <cstdint>

#include "../traits.hpp"

namespace med::asn {

/* X.680
8.1: A tag is specified by giving a class and a number within the class.
8.2: The number is a non-negative integer, specified in decimal notation.
8.3 Restrictions on tags assigned by the user of ASN.1 are specified in 31.2
	NOTE: There is no formal difference between use of tags from the other three classes. Where
	application class tags are employed, a private or context-specific class tag could generally be
	applied instead, as a matter of user choice and style. The presence of the three classes is
	largely for historical reasons.
8.5 Some encoding rules require a canonical order for tags.
8.6 The canonical order for tags is based on the outermost tag of each type and is defined as follows:
	a) those elements or alternatives with universal class tags shall appear first, followed by those
	with application class tags, followed by those with context-specific tags, followed by those with
	private class tags;
	b) within each class of tags, the elements or alternatives shall appear in ascending order of their
	tag numbers.
*/
//NOTE: natural numbering to match the canonical order and X.690:Table 1 - Encoding of class of tag
enum class tag_class : uint8_t
{
	UNIVERSAL        = 0b00,
	APPLICATION      = 0b01,
	CONTEXT_SPECIFIC = 0b10,
	PRIVATE          = 0b11,
};

/*
3.8.77 tag: Additional information, separate from the abstract values of the type, which is
	associated with every ASN.1 type and which can be changed or augmented by a type prefix.
	NOTE – Tag information is used in some encoding rules to ensure that encodings are not ambiguous.
	Tag information differs from encoding instructions because tag information is associated with all
	ASN.1 types, even if they do not have a type prefix.
3.8.78 tagged types: A type defined by referencing a single existing type and a tag.
3.8.79 tagging: Assigning a new tag to a type, replacing or adding to the existing (possibly the default) tag.
3.8.87 type prefix: Part of the ASN.1 notation that can be used to assign an encoding instruction
	or a tag to a type.

9.1 An encoding instruction is assigned to a type using either a type prefix (31.3) or an encoding control section (54).
9.5 Multiple encoding instructions with the same or with different encoding references may be
	assigned to a type (using either or both of type prefixes and an encoding control section).
	Encoding instructions assigned with a given encoding reference are independent from those
	assigned with a different encoding reference, and from any use of a type prefix to perform tagging.
9.7 If an encoding instruction is assigned to the "Type" in a "TypeAssignment", it becomes
	associated with the type, and is applied wherever the "typereference" of the "TypeAssignment"
	is used. This includes use in other modules through the export and import statements.

31.1.2 A prefixed type is either a "TaggedType" or an "EncodingPrefixedType".
31.1.3 A prefixed type which is a tagged type is mainly of use where X680 requires the use of types
	with distinct tags (in seq, set, choice). The use of a "TagDefault" of AUTOMATIC TAGS in a module
	allows this to be accomplished without the explicit appearance of "TaggedType" in that module.
31.1.4 The assignment of an encoding instruction using an "EncodingPrefixedType" is only relevant to
	the encodings identified by the associated encoding reference and has no effect on the abstract
	values of the type.
31.1.5 NOTE – Specification would be simpler, however, historically, tagging was introduced 1st.
	Tagging also differs syntactically from assignment of encoding instructions: the specification
	that tagging is EXPLICIT or IMPLICIT occurs following the closing "]" of the tag.
*/
enum class tag_type : uint8_t
{
	EXPLICIT,
	IMPLICIT,
	AUTOMATIC,
};

//Table 1 – Universal class tag assignments
enum tag_value : uint8_t
{
	BOOLEAN = 1,
	INTEGER = 2,
	BIT_STRING = 3,
	OCTET_STRING = 4,
	NULL_TYPE = 5,
	OBJECT_IDENTIFIER = 6,
	OBJECT_DESCRIPTOR = 7,
	EXTERNAL = 8,
	REAL = 9,
	ENUMERATED = 10,
	EMBEDDED_PDV = 11,
	UTF8_STRING = 12,
	SEQUENCE = 16,
	SET = 17,
	//Character string types
	NUMERIC_STRING = 18,
	PRINTABLE_STRING = 19,
	T61_STRING = 20,
	VIDEOTEX_STRING = 21,
	IA5_STRING = 22,
	GRAPHIC_STRING = 25,
	VISIBLE_STRING = 26,
	GENERAL_STRING = 27,
	UNIVERSAL_STRING = 28,
	CHARACTER_STRING = 29,
	BMP_STRING = 30,
	//Time
	UTC_TIME = 23,
	GENERALIZED_TIME = 24,
	//Date
	DATE = 31,
	TIME_OF_DAY = 32,
	DATE_TIME = 33,
	DURATION = 34,
	//OIDs
	OID_IRI = 35,
	RELATIVE_OID_IRI = 36,
};


template <std::size_t TAG, tag_class CLASS = tag_class::UNIVERSAL, tag_type TYPE = tag_type::IMPLICIT>
struct traits : med::base_traits
{
	static constexpr auto asn_tag_type = TYPE;
	static constexpr auto asn_tag_class = CLASS;
	static constexpr auto asn_tag_value = TAG;
};


} //end: namespace med::asn
