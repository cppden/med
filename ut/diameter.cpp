#include "ut.hpp"

namespace diameter {

//to concat message code and request/answer bit
constexpr std::size_t REQUEST = 0x80000000;

struct version : med::value<med::fixed<1, uint8_t>> {};
struct length : med::value<med::bytes<3>> {};

struct cmd_flags : med::value<uint8_t>
{
	enum : value_type
	{
		R = 0x80,
		P = 0x40,
		E = 0x20,
		T = 0x10,
	};

	bool request() const                { return get() & R; }
	void request(bool v)                { set(v ? (get() | R) : (get() & ~R)); }

	bool proxiable() const              { return get() & P; }
	void proxiable(bool v)              { set(v ? (get() | P) : (get() & ~P)); }

	bool error() const                  { return get() & E; }
	void error(bool v)                  { set(v ? (get() | E) : (get() & ~E)); }

	bool retx() const                   { return get() & T; }
	void retx(bool v)                   { set(v ? (get() | T) : (get() & ~T)); }

	static constexpr char const* name() { return "Cmd-Flags"; }
};

struct cmd_code : med::value<med::bytes<3>>
{
	static constexpr char const* name()     { return "Cmd-Code"; }
	//non-fixed tag matching for ANY message example
	static constexpr bool match(value_type) { return true; }
};

struct app_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "App-Id"; }
};

struct hop_by_hop_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "Hop-by-Hop-Id"; }
};

struct end_to_end_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "End-to-End-Id"; }
};

/*0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |    Version    |                 Message Length                |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | command flags |                  Command-Code                 |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         Application-ID                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      Hop-by-Hop Identifier                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      End-to-End Identifier                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  AVPs ...
 +-+-+-+-+-+-+-+-+-+-+-+-+- */
struct header : med::sequence<
	med::mandatory< version >,
	med::placeholder::_length<>,
	med::mandatory< cmd_flags >,
	med::mandatory< cmd_code >,
	med::mandatory< app_id >,
	med::mandatory< hop_by_hop_id >,
	med::mandatory< end_to_end_id >
>
{
	std::size_t get_tag() const                 { return get<cmd_code>().get() + (flags().request() ? REQUEST : 0); }
	void set_tag(std::size_t tag)               { ref<cmd_code>().set(tag & 0xFFFFFF); flags().request(tag & REQUEST); }

	cmd_flags const& flags() const              { return get<cmd_flags>(); }
	cmd_flags& flags()                          { return ref<cmd_flags>(); }

	app_id::value_type ap_id() const            { return get<app_id>().get(); }
	void ap_id(app_id::value_type id)           { ref<app_id>().set(id); }

	hop_by_hop_id::value_type hop_id() const    { return get<hop_by_hop_id>().get(); }
	void hop_id(hop_by_hop_id::value_type id)   { ref<hop_by_hop_id>().set(id); }

	end_to_end_id::value_type end_id() const    { return get<end_to_end_id>().get(); }
	void end_id(end_to_end_id::value_type id)   { ref<end_to_end_id>().set(id); }

	static constexpr char const* name()         { return "Header"; }
};

struct avp_code : med::value<uint32_t>
{
	static constexpr char const* name()     { return "AVP-Code"; }
	//non-fixed tag matching for any_avp
	static constexpr bool match(value_type) { return true; }
};

template <avp_code::value_type CODE>
struct avp_code_fixed : med::value<med::fixed<CODE, typename avp_code::value_type>> {};

struct avp_flags : med::value<uint8_t>
{
	enum : value_type
	{
		V = 0x80, //vendor specific/vendor-id is present
		M = 0x40, //AVP is mandatory
		P = 0x20, //protected
	};

	bool mandatory() const              { return get() & M; }
	void mandatory(bool v)              { set(v ? (get() | M) : (get() & ~M)); }

	bool protect() const                { return get() & P; }
	void protect(bool v)                { set(v ? (get() | P) : (get() & ~P)); }

	static constexpr char const* name() { return "AVP-Flags"; }
};

struct vendor : med::value<uint32_t>
{
	static constexpr char const* name()   { return "Vendor"; }

	struct has
	{
		template <class HDR>
		bool operator()(HDR const& hdr) const
		{
			return hdr.template as<avp_flags>().get() & avp_flags::V;
		}
	};
};

/*0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           AVP Code                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |V M P r r r r r|                  AVP Length                   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                        Vendor-ID (opt)                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |    Data ...
 +-+-+-+-+-+-+-+-+                                                 */
template <class BODY, avp_code::value_type CODE, uint8_t FLAGS = 0, vendor::value_type VENDOR = 0>
struct avp :
	med::sequence<
		M< avp_flags >,
		med::placeholder::_length<-4>, //include AVP Code
		O< vendor, vendor::has >,
		M< BODY >
	>
	, med::add_meta_info< med::mi<med::mik::TAG, avp_code_fixed<CODE>> >
{
	using length_type = med::value<med::bytes<3>>;
	using padding = med::padding<uint32_t, false>;

	auto const& flags() const                   { return this->template get<avp_flags>(); }
	auto& flags()                               { return this->template ref<avp_flags>(); }
	vendor const* get_vendor() const            { return this->template get<vendor>(); }

	BODY const& body() const                    { return this->template get<BODY>(); }
	BODY& body()                                { return this->template ref<BODY>(); }
	bool is_set() const                         { return body().is_set(); }

	template <typename... ARGS>
	auto set(ARGS&&... args)                    { return body().set(std::forward<ARGS>(args)...); }

	avp()
	{
		if constexpr (0 != VENDOR)
		{
			this->template ref<vendor>().set(VENDOR);
			this->template ref<avp_flags>().set(FLAGS | avp_flags::V);
		}
		else if constexpr (0 != FLAGS)
		{
			this->template ref<avp_flags>().set(FLAGS & ~avp_flags::V);
		}
	}
};

struct unsigned32 : med::value<uint32_t>
{
	static constexpr char const* name() { return "Unsigned32"; }
};

struct result_code : avp<unsigned32, 268, avp_flags::M>
{
	static constexpr char const* name() { return "Result-Code"; }
};

struct origin_host : avp<med::ascii_string<>, 264, avp_flags::M>
{
	static constexpr char const* name() { return "Origin-Host"; }
};

struct origin_realm : avp<med::ascii_string<>, 296, avp_flags::M>
{
	static constexpr char const* name() { return "Origin-Realm"; }
};

struct disconnect_cause : avp<unsigned32, 273, avp_flags::M>
{
	static constexpr char const* name() { return "Disconnect-Cause"; }
};

struct error_message : avp<med::ascii_string<>, 281>
{
	static constexpr char const* name() { return "Error-Message"; }
};

struct failed_avp : avp<med::octet_string<>, 279, avp_flags::M>
{
	static constexpr char const* name() { return "Failed-AVP"; }
};

struct any_avp :
	med::sequence<
		M< avp_code >,
		M< avp_flags >,
		med::placeholder::_length<>,
		O< vendor, vendor::has >,
		M< med::octet_string<> >
	>
	, med::add_meta_info< med::mi<med::mik::TAG, avp_code> >
{
	using length_type = med::value<med::bytes<3>>;
	using padding = med::padding<uint32_t, false>;

	auto& body() const                  { return get<med::octet_string<>>(); }
	bool is_set() const                 { return body().is_set(); }
};

/*
<DPR>  ::= < Diameter Header: 282, REQ >
		 { Origin-Host }
		 { Origin-Realm }
		 { Disconnect-Cause }
*/
struct DPR : med::set<
	M< origin_host >,
	M< origin_realm >,
	M< disconnect_cause >,
	O< any_avp, med::inf >
>
{
	static constexpr std::size_t code = 282;
	static constexpr char const* name() { return "Disconnect-Peer-Request"; }
};

/*
<DPA>  ::= < Diameter Header: 282 >
		 { Result-Code }
		 { Origin-Host }
		 { Origin-Realm }
		 [ Error-Message ]
	   * [ Failed-AVP ]
*/
struct DPA : med::set<
	M< result_code >,
	M< origin_host >,
	M< origin_realm >,
	O< error_message >,
	O< failed_avp, med::inf >,
	O< any_avp, med::inf >
>
{
	static constexpr std::size_t code = 282;
	static constexpr char const* name() { return "Disconnect-Peer-Answer"; }
};

//example how to decode ANY message extracting only few AVPs of interest
struct ANY : med::set<
	O< result_code >,
	O< origin_host >,
	O< origin_realm >,
	O< any_avp, med::inf >
>
{
	static constexpr auto name() { return "Diameter-Message"; }
};

template <class MSG>
using request = M<med::value<med::fixed<REQUEST | MSG::code, uint32_t>>, MSG>;
template <class MSG>
using answer = M<med::value<med::fixed<MSG::code, uint32_t>>, MSG>;

//couple of messages from base protocol for testing
struct base : med::choice< header
	, request<DPR>
	, answer<DPA>
	, M<cmd_code, ANY>
>
{
	using length_type = length;
};

uint8_t const dpr[] = {
	0x01, 0x00, 0x00, 0x4C, //VER(1), LEN(3)
	0x80, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
	0x00, 0x00, 0x00, 0x00, //APP-ID
	0x22, 0x22, 0x22, 0x22, //H2H-ID
	0x55, 0x55, 0x55, 0x55, //E2E-ID

	0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
	0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
	'O', 'r', 'i', 'g',
	'.', 'H', 'o', 's',
	't',   0,   0,   0,

	0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
	0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
	'o', 'r', 'i', 'g',
	'.', 'r', 'e', 'a',
	'l', 'm', '.', 'n',
	'e', 't',   0,   0,

	0x00, 0x00, 0x01, 0x11, //AVP = 273 Disconnect-Cause AVP
	0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
	0x00, 0x00, 0x00, 0x02, //cause = 2
};

} //end: namespace diameter

TEST(diameter, padding_encode)
{
	diameter::base base;
	diameter::DPR& msg = base.select();

	base.header().ap_id(0);
	base.header().hop_id(0x22222222);
	base.header().end_id(0x55555555);

	msg.ref<diameter::origin_host>().set("Orig.Host");
	msg.ref<diameter::origin_realm>().set("orig.realm.net");
	msg.ref<diameter::disconnect_cause>().set(2);

	uint8_t buffer[1024] = {};
	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, base);

	EXPECT_EQ(sizeof(diameter::dpr), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(diameter::dpr, buffer));
}

TEST(diameter, padding_decode)
{
	med::decoder_context<> ctx{ diameter::dpr };
	diameter::base base;
	decode(med::octet_decoder{ctx}, base);

	ASSERT_EQ(0, base.header().ap_id());
	ASSERT_EQ(0x22222222, base.header().hop_id());
	ASSERT_EQ(0x55555555, base.header().end_id());

	diameter::DPR const* msg = base.cselect();
	ASSERT_NE(nullptr, msg);

	EQ_STRING_M(diameter::origin_host, "Orig.Host");
	EQ_STRING_M(diameter::origin_realm, "orig.realm.net");
	ASSERT_EQ(2, msg->get<diameter::disconnect_cause>().body().get());
}

TEST(diameter, padding_bad)
{
	uint8_t const dpr[] = {
			0x01, 0x00, 0x00, 76-3, //VER(1), LEN(3)
			0x80, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
			0x00, 0x00, 0x00, 0x00, //APP-ID
			0x22, 0x22, 0x22, 0x22, //H2H-ID
			0x55, 0x55, 0x55, 0x55, //E2E-ID

			0x00, 0x00, 0x01, 0x11, //AVP = 273 Disconnect-Cause AVP
			0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
			0x00, 0x00, 0x00, 0x02, //cause = 2

			0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
			0x40, 0x00, 0x00, 22, //V.M.P(1), LEN(3) = 22 + padding
			'o', 'r', 'i', 'g',
			'.', 'r', 'e', 'a',
			'l', 'm', '.', 'n',
			'e', 't',   0,   0,

			0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
			0x40, 0x00, 0x00, 17, //V.M.P(1), LEN(3) = 17 + padding
			'O', 'r', 'i', 'g',
			'.', 'H', 'o', 's',
			't',   //0,   0,   0,
	};
	med::decoder_context<> ctx{ dpr };

	diameter::base base;
	EXPECT_THROW(decode(med::octet_decoder{ctx}, base), med::exception);
}

TEST(diameter, any_avp)
{
	uint8_t const dpa[] = {
		0x01, 0x00, 0x00, 76+12+20, //VER(1), LEN(3)
		0x00, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
		0x00, 0x00, 0x00, 0x00, //APP-ID
		0x22, 0x22, 0x22, 0x22, //H2H-ID
		0x55, 0x55, 0x55, 0x55, //E2E-ID

		0x00, 0x00, 0x01, 0x0C, //AVP = 268 Result Code
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0x00, 0x00, 0x0B, 0xBC, //result = 3004

		0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
		0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
		'O', 'r', 'i', 'g',
		'.', 'H', 'o', 's',
		't',   0,   0,   0,

		//NOTE: this AVP is not expected in DPA
		0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0xA5, 0x5A, 0xBC, 0xCB, //id

		0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
		0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
		'o', 'r', 'i', 'g',
		'.', 'r', 'e', 'a',
		'l', 'm', '.', 'n',
		'e', 't',   0,   0,

		0x00, 0x00, 0x01, 0x17, //AVP-CODE = 279 Failed-AVP (grouped)
		0x40, 0x00, 0x00, 20, //V.M.P(1), LEN(3)
		0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0x01, 0x00, 0x00, 0x23, //id = S6a
	};

	//encoding is done according message description thus need to reorder
	uint8_t const dpa_enc[] = {
		0x01, 0x00, 0x00, 76+12+20, //VER(1), LEN(3)
		0x00, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
		0x00, 0x00, 0x00, 0x00, //APP-ID
		0x22, 0x22, 0x22, 0x22, //H2H-ID
		0x55, 0x55, 0x55, 0x55, //E2E-ID

		0x00, 0x00, 0x01, 0x0C, //AVP = 268 Result Code
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0x00, 0x00, 0x0B, 0xBC, //result = 3004

		0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
		0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
		'O', 'r', 'i', 'g',
		'.', 'H', 'o', 's',
		't',   0,   0,   0,

		0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
		0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
		'o', 'r', 'i', 'g',
		'.', 'r', 'e', 'a',
		'l', 'm', '.', 'n',
		'e', 't',   0,   0,

		0x00, 0x00, 0x01, 0x17, //AVP-CODE = 279 Failed-AVP (grouped)
		0x40, 0x00, 0x00, 20, //V.M.P(1), LEN(3)
		0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0x01, 0x00, 0x00, 0x23, //id = S6a

		//NOTE: this AVP is not expected in DPA
		0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0xA5, 0x5A, 0xBC, 0xCB, //id
	};

	static_assert(sizeof(dpa) == sizeof(dpa_enc));

	med::decoder_context<> dctx{ dpa };
	diameter::base base;
	decode(med::octet_decoder{dctx}, base);

	ASSERT_EQ(0, base.header().ap_id());
	ASSERT_EQ(0x22222222, base.header().hop_id());
	ASSERT_EQ(0x55555555, base.header().end_id());

	diameter::DPA const* msg = base.cselect();
	ASSERT_NE(nullptr, msg);

	ASSERT_EQ(3004, msg->get<diameter::result_code>().body().get());
	EQ_STRING_M(diameter::origin_host, "Orig.Host");
	EQ_STRING_M(diameter::origin_realm, "orig.realm.net");

	EXPECT_EQ(1, msg->count<diameter::failed_avp>());
	EXPECT_EQ(1, msg->count<diameter::any_avp>());
	for (auto& any : msg->get<diameter::any_avp>())
	{
		EXPECT_EQ(0x102, any.get<diameter::avp_code>().get());
	}

	//encode
	uint8_t buffer[1024] = {};
	med::encoder_context<> ectx{ buffer };
	CODEC_TRACE("\n\nENCODE: %s\n\n", ectx.buffer().toString());
	encode(med::octet_encoder{ectx}, base);

	EXPECT_EQ(sizeof(dpa_enc), ectx.buffer().get_offset());
	ASSERT_TRUE(Matches(dpa_enc, buffer));
}

TEST(diameter, any_msg)
{
	uint8_t const dwa[] = {
		0x01, 0x00, 0x00, 0x4C, //VER(1), LEN(3)
		0x00, 0x00, 0x01, 0x18, //R.P.E.T(1), CMD(3) = 280
		0x00, 0x00, 0x00, 0x00, //APP-ID
		0x22, 0x22, 0x22, 0x22, //H2H-ID
		0x55, 0x55, 0x55, 0x55, //E2E-ID

		0x00, 0x00, 0x01, 0x0C, //AVP = 268 Result Code
		0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
		0x00, 0x00, 0x0B, 0xBC, //result = 3004

		0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
		0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
		'O', 'r', 'i', 'g',
		'.', 'H', 'o', 's',
		't',   0,   0,   0,

		0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
		0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
		'o', 'r', 'i', 'g',
		'.', 'r', 'e', 'a',
		'l', 'm', '.', 'n',
		'e', 't',   0,   0,
	};

	med::decoder_context<> dctx{ dwa };

	diameter::base base;
	decode(med::octet_decoder{dctx}, base);

	ASSERT_EQ(0, base.header().ap_id());
	ASSERT_EQ(0x22222222, base.header().hop_id());
	ASSERT_EQ(0x55555555, base.header().end_id());

	diameter::ANY const* msg = base.cselect();
	ASSERT_NE(nullptr, msg);

	auto* rc = msg->get<diameter::result_code>();
	ASSERT_NE(nullptr, rc);
	EXPECT_EQ(3004, rc->body().get());

	{
		auto* host = msg->get<diameter::origin_host>();
		ASSERT_NE(nullptr, host);
		EXPECT_EQ("Orig.Host"sv, as_sv(*host));
	}
	{
		auto* realm = msg->get<diameter::origin_realm>();
		ASSERT_NE(nullptr, realm);
		EXPECT_EQ("orig.realm.net"sv, as_sv(*realm));
	}

	//encode
	uint8_t buffer[1024] = {};
	med::encoder_context<> ectx{ buffer };
	CODEC_TRACE("\n\nENCODE: %s\n\n", ectx.buffer().toString());
	encode(med::octet_encoder{ectx}, base);

	EXPECT_EQ(sizeof(dwa), ectx.buffer().get_offset());
	ASSERT_TRUE(Matches(dwa, buffer));
}

