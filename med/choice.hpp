/**
@file
choice class as union

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <new>

#include "exception.hpp"
#include "error.hpp"
#include "debug.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "encode.hpp"
#include "decode.hpp"

namespace med {

template <class... IEs>
struct cases;

template <class IE, class... IEs>
struct cases<IE, IEs...>
{
	template <typename FIELD>
	using at = std::conditional_t<std::is_same<typename IE::case_type, FIELD>::value, IE, typename cases<IEs...>::template at<FIELD> >;
};

template<>
struct cases<>
{
	template <typename FIELD>
	using at = void; //No such field found
};


namespace sl {

template <class... IEs>
struct choice_for;

template <class IE, class... IEs>
struct choice_for<IE, IEs...>
{
	template <class ENCODER, class TO>
	static MED_RESULT encode(ENCODER&& encoder, TO const& to)
	{
		if (IE::tag_type::match(get_tag(to.header())))
		{
			using case_t = typename IE::case_type;
			CODEC_TRACE("CASE[%s]", name<case_t>());
			void const* store_p = &to.m_storage;

			return med::encode(encoder, to.header())
				MED_AND med::encode(encoder, *static_cast<case_t const*>(store_p));
		}
		else
		{
			return choice_for<IEs...>::encode(encoder, to);
		}
	}

	template <class DECODER, class TO, class UNEXP>
	static MED_RESULT decode(DECODER&& decoder, TO& to, UNEXP& unexp)
	{
		if (IE::tag_type::match( get_tag(to.header()) ))
		{
			using case_t = typename IE::case_type;
			CODEC_TRACE("->CASE[%s]", name<case_t>());
			case_t* case_p = new (&to.m_storage) case_t{};
			return med::decode(decoder, *case_p, unexp);
		}
		else
		{
			return choice_for<IEs...>::decode(decoder, to, unexp);
		}
	}

	template <class TO>
	static std::size_t calc_length(TO const& to)
	{
		if (IE::tag_type::match( get_tag(to.header()) ))
		{
			using case_t = typename IE::case_type;
			void const* store_p = &to.m_storage;
			return med::get_length(to.header())
				+ med::get_length(*static_cast<case_t const*>(store_p));
		}
		else
		{
			return choice_for<IEs...>::calc_length(to);
		}
	}

};

template <>
struct choice_for<>
{
	template <class FUNC, class TO, class UNEXP>
	static MED_RESULT decode(FUNC&& func, TO& to, UNEXP& unexp)
	{
		return unexp(func, to, to.header());
	}

	template <class FUNC, class TO>
	static MED_RESULT encode(FUNC&& func, TO const& to)
	{
		return func(error::INCORRECT_TAG, name<TO>(), get_tag(to.header()));
	}

	template <class TO>
	static std::size_t calc_length(TO const&)
	{
		return 0;
	}
};

}	//end: namespace sl

template <class HEADER, class ...CASES>
class choice : public IE<CONTAINER>
{
private:
	template <class T>
	struct selector
	{
		explicit selector(T* that) noexcept
			: m_this(that)
		{}

		template <class U> operator U&()
		{
			static_assert(!std::is_const<T>(), "CONST CHOICE RETURNS A POINTER, NOT REFERENCE!");
			return m_this->template ref<U>();
		}

		template <class U> operator U const* ()
		{
			return m_this->template get<U>();
		}

		T* m_this;
	};
	template <class T>
	static auto make_selector(T* that) { return selector<T>{that}; }

public:
	using header_type = HEADER;

	header_type const& header() const       { return m_header; }
	header_type const& get_header() const   { return m_header; }
	header_type& header()                   { return m_header; }

	bool is_set() const                     { return header().is_set(); }
	std::size_t calc_length() const         { return sl::choice_for<CASES...>::calc_length(*this); }

	auto select()                           { return make_selector(this); }
	auto select() const                     { return make_selector(this); }
	auto cselect() const                    { return make_selector(this); }

	template <class CASE>
	CASE& ref()
	{
		//TODO: how to prevent a copy when callee-side re-uses reference by mistake?
		static_assert(!std::is_const<CASE>(), "REFERENCE IS NOT FOR ACCESSING AS CONST");
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		void* store_p = &m_storage;
		return *(set_tag(header(), IE::tag_type::get_encoded())
			? new (store_p) CASE{}
			: static_cast<CASE*>(store_p));
	}

	template <class CASE>
	CASE& clear()
	{
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		set_tag(header(), IE::tag_type::get());
		return *(new (&m_storage) CASE{});
	}

	template <class CASE>
	CASE const* get() const
	{
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		void const* store_p = &m_storage;
		return IE::tag_type::match( get_tag(header()) ) ? static_cast<CASE const*>(store_p) : nullptr;
	}

	template <class ENCODER>
	MED_RESULT encode(ENCODER&& encoder) const
	{
		return sl::choice_for<CASES...>::encode(encoder, *this);
	}

	template <class DECODER, class UNEXP>
	MED_RESULT decode(DECODER&& decoder, UNEXP& unexp)
	{
		return med::decode(decoder, header(), unexp)
			MED_AND sl::choice_for<CASES...>::decode(decoder, *this, unexp);
	}

private:
	template <class... IEs>
	friend struct sl::choice_for;

	using storage_type = std::aligned_union_t<0, typename CASES::case_type...>;

	header_type  m_header;
	storage_type m_storage;
};

} //end: namespace med
