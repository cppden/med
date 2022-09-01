#include "ut.hpp"


//GTPC-like header for testing
namespace gtpc {

//Bits: 7   6   5   4   3   2   1   0
//     [   Msg Prio  ] [   Spare     ]
// The relative priority of the GTP-C message may take any value between 0 and 15,
// where 0 corresponds to the highest priority and 15 the lowest priority.
struct message_priority : med::value<med::init<0, uint8_t>>
{
	static constexpr char const* name()         { return "Message-Priority"; }

	enum : value_type
	{
		MASK = 0b11110000
	};

	uint8_t get() const                         { return (get_encoded() & MASK) >> 4; }
	void set(uint8_t v)                         { set_encoded((v << 4) & MASK); }
};

//Bits: 7   6   5   4   3   2   1   0
//     [ Version ] [P] [T] [MP][Spare]
struct version_flags : med::value<uint8_t>
{
	enum : value_type
	{
		MP = 1u << 2, //Message Priority flag
		T  = 1u << 3, //TEID flag
		P  = 1u << 4, //Piggybacking flag
		MASK_VERSION = 0xE0u, //1110 0000
	};

	uint8_t version() const             { return (get() & MASK_VERSION) >> 5; }

	bool piggyback() const              { return get() & P; }
	void piggyback(bool v)              { set(v ? (get() | P) : (get() & ~P)); }

	static constexpr char const* name() { return "Version-Flags"; }

	struct setter
	{
		template <class HDR>
		void operator()(version_flags& flags, HDR const& hdr) const
		{
			auto bits = flags.get();
			if (hdr.template as<message_priority>().is_default())
			{
				bits &= ~MP;
			}
			else
			{
				bits |= MP;
			}
			flags.set(bits);
		}
	};

	struct has_message_priority
	{
		template <class HDR>
		bool operator()(HDR const& hdr) const
		{
			return hdr.template as<version_flags>().get() & version_flags::MP;
		}
	};
};

struct sequence_number : med::value<med::bits<24>>
{
	static constexpr char const* name()         { return "Sequence-Number"; }
};

/*Oct\Bit  8   7   6   5   4   3   2   1
-----------------------------------------
  1       [ Version ] [P] [T] [MP][Spare]
  2       [ Sequence Number (1st octet) ]
  3       [ Sequence Number (2nd octet) ]
  4       [ Sequence Number (3rd octet) ]
  5       [ Msg Priority] [  Spare      ]
----------------------------------------- */
struct header : med::sequence<
	M< version_flags, version_flags::setter >,
	M< sequence_number >,
	O< message_priority, version_flags::has_message_priority >
>
{
	version_flags const& flags() const          { return get<version_flags>(); }
	version_flags& flags()                      { return ref<version_flags>(); }

	sequence_number::value_type sn() const      { return get<sequence_number>().get(); }
	void sn(sequence_number::value_type v)      { ref<sequence_number>().set(v); }

	static constexpr char const* name()         { return "Header"; }

	explicit header(uint8_t ver = 2)
	{
		flags().set(ver << 5);
	}
};

} //end: namespace gtpc

TEST(opt_defs, unset)
{
	uint8_t buffer[1024];

	gtpc::header header;
	header.sn(0x010203);

	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, header);
	ASSERT_STREQ("40 01 02 03 00 ", as_string(ctx.buffer()));
	check_decode(header, ctx.buffer());
}

TEST(opt_defs, set)
{
	uint8_t buffer[1024] = {};

	gtpc::header header;
	header.sn(0x010203);
	header.ref<gtpc::message_priority>().set(7);

	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, header);
	ASSERT_STREQ("44 01 02 03 70 ", as_string(ctx.buffer()));
	check_decode(header, ctx.buffer());
}
