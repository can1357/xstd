// Compile host-side xstd APIs through NVCC's host and device parsing passes.
// These headers do not promise device-callable APIs; no GPU or runtime is needed.
//
#include <xstd/fnv.hpp>
#include <xstd/function_view.hpp>
#include <xstd/finally.hpp>
#include <xstd/random.hpp>
#include <xstd/result.hpp>
#include <xstd/file.hpp>
#include <xstd/serialization.hpp>
#include <xstd/hashable.hpp>
#include <xstd/type_helpers.hpp>

namespace {
	uint32_t add_one( uint32_t value ) {
		return value + 1;
	}
}

static_assert( xstd::Same<xstd::type_tag<uint32_t>::type, uint32_t> );
static_assert( xstd::Integral<uint32_t> );
static_assert( xstd::bit_cast<uint32_t>( 1.0f ) == 0x3f800000u );

// Instantiate both callable storage paths and the common utility templates.
//
uint64_t cuda_header_check( uint32_t seed ) {
	xstd::function_view<uint32_t( uint32_t )> free_view{ &add_one };
	uint32_t offset = 1;
	auto stateful = [ &offset ]( uint32_t value ) { return value + offset++; };
	xstd::function_view<uint32_t( uint32_t )> stateful_view{ stateful };
	xstd::finally guard{ [ &offset ] { ++offset; } };

	xstd::pcg rng{ seed };
	xstd::io_result<uint32_t> result{ stateful_view( free_view( rng() ) ) };
	auto bytes = xstd::serialize( result.value(), true );
	xstd::serialization context{ bytes, true };
	auto restored = xstd::deserialize<uint32_t>( context );
	guard.apply();

	xstd::fnv64 hash;
	hash.update( restored ).update( offset );
	return hash.as64() ^ xstd::make_hash<xstd::fnv64>( restored, offset ).as64();
}

// Instantiate file I/O without executing it as part of the compile-only check.
//
xstd::io_result<std::vector<uint32_t>> cuda_file_check( const std::filesystem::path& path ) {
	return xstd::file::read_raw<uint32_t>( path, 1 );
}
