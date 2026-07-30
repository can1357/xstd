// Correctness of the hardware hash paths.
//
// A published vector proves the algorithm; it does not prove the accelerated
// path agrees with the portable one everywhere the portable one is used. So the
// bulk of this is differential: the same bytes through `Traits<false>` and
// `Traits<true>`, across every length that moves a block boundary or a padding
// edge, and fed both in one call and in ragged streaming chunks.
//
// A mismatch here means the two implementations of one algorithm disagree,
// which is a far louder signal than a single vector failing.
//
#include <xstd/crc.hpp>
#include <xstd/sha1.hpp>
#include <xstd/sha256.hpp>
#include <xstd/sha512.hpp>

#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace {
	size_t failures = 0;

	void complain( const std::string& what, const std::string& expected, const std::string& actual ) {
		fprintf( stderr, "FAIL %s\n  expected %s\n  actual   %s\n", what.c_str(), expected.c_str(), actual.c_str() );
		++failures;
	}

	// A published vector, which pins the algorithm itself.
	//
	template<typename Hash>
	void expect_vector( const char* name, std::string_view input, const std::string& expected ) {
		Hash hasher;
		hasher.update( ( const uint8_t* ) input.data(), input.size() );
		std::string actual = hasher.to_string();
		for ( char& c : actual )
			c = char( tolower( c ) );
		if ( actual != expected )
			complain( std::string( name ) + " vector", expected, actual );
	}

	// The accelerated path against the portable one, over the same bytes.
	//
	template<typename Portable, typename Accelerated>
	void expect_agreement( const char* name, const std::vector<uint8_t>& data, std::mt19937_64& source ) {
		Portable oracle;
		oracle.update( data.data(), data.size() );

		Accelerated whole;
		whole.update( data.data(), data.size() );
		if ( whole.to_string() != oracle.to_string() )
			complain( std::string( name ) + " one-shot at " + std::to_string( data.size() ) + " bytes", oracle.to_string(), whole.to_string() );

		// Ragged chunks exercise the leftover buffer, which is the only part of
		// the block plumbing an accelerated compress can still get wrong.
		//
		Accelerated streamed;
		for ( size_t offset = 0; offset < data.size(); ) {
			size_t step = 1 + source() % 97;
			step        = std::min( step, data.size() - offset );
			streamed.update( data.data() + offset, step );
			offset += step;
		}
		if ( streamed.to_string() != oracle.to_string() )
			complain( std::string( name ) + " streamed at " + std::to_string( data.size() ) + " bytes", oracle.to_string(), streamed.to_string() );
	}
	// An independent reflected CRC-32, written the slow obvious way.
	//
	// Deliberately not the library's own software path: an oracle that shares
	// code with the thing under test cannot catch a fault they have in common.
	//
	template<uint32_t Rpoly>
	uint32_t reference_crc32( const uint8_t* data, size_t length ) {
		uint32_t crc = ~uint32_t( 0 );
		while ( length-- ) {
			crc ^= *data++;
			for ( int bit = 0; bit != 8; bit++ )
				crc = ( crc >> 1 ) ^ ( Rpoly & ( 0 - ( crc & 1 ) ) );
		}
		return ~crc;
	}

	template<typename Crc, uint32_t Rpoly>
	void expect_crc( const char* name, const std::vector<uint8_t>& data ) {
		uint32_t expected = reference_crc32<Rpoly>( data.data(), data.size() );
		uint32_t actual   = uint32_t( Crc{}.update( data.data(), data.size() ) );
		if ( actual != expected )
			complain( std::string( name ) + " at " + std::to_string( data.size() ) + " bytes",
						 std::to_string( expected ), std::to_string( actual ) );
	}
}

// The compile-time path must stay on the portable implementation, and must
// still agree with it. If an accelerated branch ever leaked into constant
// evaluation this would not compile.
//
static_assert( xstd::basic_sha<xstd::sha1_traits<true>>{}.update( "abc"[ 0 ] ).digest() ==
					xstd::basic_sha<xstd::sha1_traits<false>>{}.update( "abc"[ 0 ] ).digest(),
				  "constant evaluated SHA-1 disagrees with the portable path" );

// Constant evaluation has no hardware to fall back on, so this also pins the
// software CRC tables against a value the instructions produce at runtime.
//
static_assert( uint32_t( xstd::crc32{}.update( "123456789"[ 0 ] ) ) == 0x83DCEFB7, "constant evaluated CRC-32 is wrong" );

int main() {
	expect_vector<xstd::sha1>( "sha1/empty", "", "da39a3ee5e6b4b0d3255bfef95601890afd80709" );
	expect_vector<xstd::sha1>( "sha1/abc", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d" );
	expect_vector<xstd::sha1>( "sha1/448bit", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1" );

	expect_vector<xstd::sha256>( "sha256/empty", "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" );
	expect_vector<xstd::sha256>( "sha256/abc", "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" );
	expect_vector<xstd::sha256>( "sha256/448bit", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" );

	expect_vector<xstd::sha512>( "sha512/empty", "", "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e" );
	expect_vector<xstd::sha512>( "sha512/abc", "abc", "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f" );
	expect_vector<xstd::sha512>( "sha512/896bit", "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909" );

	// Padding boundaries, against digests from an independent implementation.
	//
	// The differential check above cannot see a fault here: padding lives in the
	// shared finalizer, so both paths would be wrong together. These lengths sit
	// on and around the points where the length field stops fitting -- 56 for a
	// 64-byte block, 112 for SHA-512's 128-byte one -- which is exactly where a
	// hash gets its padding wrong.
	//
	struct boundary {
		size_t length;
		const char* sha1;
		const char* sha256;
		const char* sha512;
	};
	static constexpr boundary boundaries[] = {
		{  55, "c1c8bbdc22796e28c0e15163d20899b65621d65a", "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318", "b0220c772cbf6c1822e2cb38a437d0e1d58772417a4bbb21c961364f8b6143e05aa6316dca8d1d7b19e16448419076395f6086cb55101fbd6d5497b148e1745f" },
		{  56, "c2db330f6083854c99d4b5bfb6e8f29f201be699", "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a", "962b64aae357d2a4fee3ded8b539bdc9d325081822b0bfc55583133aab44f18bafe11d72a7ae16c79ce2ba620ae2242d5144809161945f1367f41b3972e26e04" },
		{  57, "f08f24908d682555111be7ff6f004e78283d989a", "f13b2d724659eb3bf47f2dd6af1accc87b81f09f59f2b75e5c0bed6589dfe8c6", "d3115798e872fc1ca6b276368e8ea0926daec6ab1f8f08297e4348ff5f5fe4c6e5205413271babafd4929b070754bc5800e5db44790666ec4e2f6ac52a17e163" },
		{  63, "03f09f5b158a7a8cdad920bddc29b81c18a551f5", "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34", "c1b0f5c6d3b03dfe4a2602e67242f54e344090b66e01100a469b129f583f016c7e27dddeaa438393dcc7ec54b0b57c9ba7af007f9b56db5f6fb677d972a31362" },
		{  64, "0098ba824b5c16427bd7a1122a5a442a25ec644d", "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb", "01d35c10c6c38c2dcf48f7eebb3235fb5ad74a65ec4cd016e2354c637a8fb49b695ef3c1d6f7ae4cd74d78cc9c9bcac9d4f23a73019998a7f73038a5c9b2dbde" },
		{  65, "11655326c708d70319be2610e8a57d9a5b959d3b", "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0", "b83086cd8494e55708ad7ecd82dfb4bca1bda61ecbb7caf0c68967902e709345e5d8305eb7ac0d588afc6cbb75161aa9c8c7e0ea986bd833dafe5e1ccd37345a" },
		{ 111, "ac877859d427d9192054eea8feb3b8a403ef83a5", "6374f73208854473827f6f6a3f43b1f53eaa3b82c21c1a6d69a2110b2a79baad", "fa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef86818196921760b4beff48404df811b953828274461673c68d04e297b0eb7b2b4d60fc6b566a2" },
		{ 112, "689993727ba37386bb032495e9dbdfb4dd1ba744", "f54353008a2553262ecdc4a34749563ba0950e8b0fc8652780b0a614b99683c1", "c01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32bd05f0f1ba33e568b88fd2d970929b719ecbb152f58f130a407c8830604b70ca" },
		{ 113, "3bcfff44cf3237b9b63c661a530077f794872efc", "ba02731ae695aae5cd49b49d84330b63995733eb22102aca755f0179b1e0e20f", "55ddd8ac210a6e18ba1ee055af84c966e0dbff091c43580ae1be703bdb85da31acf6948cf5bd90c55a20e5450f22fb89bd8d0085e39f85a86cc46abbca75e24d" },
		{ 119, "ee971065aaa017e0632a8ca6c77bb3bf8b1dfc56", "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb", "130396a75cb483f2eee8c56d8a668bb3d2641f5243212c0bee2bd33da096ad9eb8179fe18f9eaacf76e09fae9de4c3f14ba13341e345be05bf76c182cc3468cb" },
		{ 120, "f34c1488385346a55709ba056ddd08280dd4c6d6", "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c", "f241de612b01aa2fa3cf01531d2a8e5e17fc761dfd48a704a834a47f57d6eade7804ecc39be42fdef16ec6adeaf7c01c2fd0c4cc97d3860907cfa4a3b36d0c05" },
		{ 127, "89d95fa32ed44a7c610b7ee38517ddf57e0bb975", "c57e9278af78fa3cab38667bef4ce29d783787a2f731d4e12200270f0c32320a", "828613968b501dc00a97e08c73b118aa8876c26b8aac93df128502ab360f91bab50a51e088769a5c1eff4782ace147dce3642554199876374291f5d921629502" },
		{ 128, "ad5b3fdbcb526778c2839d2f151ea753995e26a0", "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e", "b73d1929aa615934e61a871596b3f3b33359f42b8175602e89f7e06e5f658a243667807ed300314b95cacdd579f3e33abdfbe351909519a846d465c59582f321" },
		{ 129, "d96debf1bdcbc896e6c134ea76e8141f40d78536", "c12cb024a2e5551cca0e08fce8f1c5e314555cc3fef6329ee994a3db752166ae", "4f681e0bd53cda4b5a2041cc8a06f2eabde44fb16c951fbd5b87702f07aeab611565b19c47fde30587177ebb852e3971bbd8d3fd30da18d71037dfbd98420429" },
		{ 239, "fdc30857cf7b957f47ebd8288d5e5d7426f44394", "064b3d122abe25c36265f79fc794b0adf28a6c5e4fe8ed3661f2287e8cecadcc", "52c853cb8d907f3d4d6b889beb027985d7c273486d75f8baf26f80d24e90c74c6c3de3e22131582380a7d14d43f2941a31385439cd6ddc469f628015e50bf286" },
		{ 240, "c056f00fb97abb4c09424e3473562ff19d6e80dc", "9b3043905ca79556acb1d2d12f9a4c0bed8ced2bd1a82e7cdf26743069be53e3", "4c296d90c61052a62ffb1dd196f1b7b09373b1f93e71836baebf89690546b7595684dbe9467a8e484fa0d1094272b4344a7c24f5fee8daedeb0bf549c985ab5f" },
	};
	for ( const boundary& row : boundaries ) {
		std::string filler( row.length, 'a' );
		expect_vector<xstd::sha1>( ( "sha1/pad" + std::to_string( row.length ) ).c_str(), filler, row.sha1 );
		expect_vector<xstd::sha256>( ( "sha256/pad" + std::to_string( row.length ) ).c_str(), filler, row.sha256 );
		expect_vector<xstd::sha512>( ( "sha512/pad" + std::to_string( row.length ) ).c_str(), filler, row.sha512 );
	}

	// Lengths that straddle every boundary the padding rules care about: the
	// 64- and 128-byte blocks, the 56- and 112-byte points where the length
	// field no longer fits, and several blocks past both.
	//
	std::mt19937_64 source{ 0x243f6a8885a308d3ull };
	std::vector<size_t> lengths;
	for ( size_t n = 0; n <= 300; n++ )
		lengths.push_back( n );
	for ( size_t n : { 511, 512, 513, 1023, 1024, 1025, 4096, 8191 } )
		lengths.push_back( n );

	for ( size_t length : lengths ) {
		std::vector<uint8_t> data( length );
		for ( uint8_t& byte : data )
			byte = uint8_t( source() );

		expect_agreement<xstd::basic_sha<xstd::sha1_traits<false>>, xstd::basic_sha<xstd::sha1_traits<true>>>( "sha1", data, source );
		expect_agreement<xstd::basic_sha<xstd::sha256_traits<false>>, xstd::basic_sha<xstd::sha256_traits<true>>>( "sha256", data, source );
		expect_agreement<xstd::basic_sha<xstd::sha512_traits<false>>, xstd::basic_sha<xstd::sha512_traits<true>>>( "sha512", data, source );

		// The hardware CRC path walks 64 bytes at a time and then peels 4/2/1,
		// so every length modulo the unroll needs to land on the same answer.
		//
		expect_crc<xstd::crc32, 0xEDB88320>( "crc32", data );
		expect_crc<xstd::crc32c, 0x82F63B78>( "crc32c", data );
	}

	// Unaligned starts. The bulk CRC path reads eight bytes at a time from
	// whatever pointer it is handed, so every offset into a qword has to give
	// the same answer as a byte-at-a-time reference.
	//
	{
		std::vector<uint8_t> block( 1024 );
		for ( uint8_t& byte : block )
			byte = uint8_t( source() );

		for ( size_t offset = 0; offset != 8; offset++ ) {
			for ( size_t length : { size_t( 0 ), size_t( 1 ), size_t( 7 ), size_t( 63 ), size_t( 64 ), size_t( 65 ), size_t( 127 ), size_t( 512 ) } ) {
				std::vector<uint8_t> slice( block.begin() + offset, block.begin() + offset + length );
				std::string where = "@" + std::to_string( offset );
				expect_crc<xstd::crc32, 0xEDB88320>( ( "crc32 " + where ).c_str(), slice );
				expect_crc<xstd::crc32c, 0x82F63B78>( ( "crc32c " + where ).c_str(), slice );

				// The same bytes read in place, at a genuinely unaligned address.
				//
				uint32_t direct    = uint32_t( xstd::crc32{}.update( block.data() + offset, length ) );
				uint32_t reference = reference_crc32<0xEDB88320>( block.data() + offset, length );
				if ( direct != reference )
					complain( "crc32 in place " + where + " length " + std::to_string( length ), std::to_string( reference ), std::to_string( direct ) );
			}
		}
	}

	if ( failures ) {
		fprintf( stderr, "%zu hash check(s) failed\n", failures );
		return 1;
	}
	printf( "sha1, sha256, sha512, crc32 and crc32c agree with their references over %zu lengths\n", lengths.size() );
	return 0;
}
