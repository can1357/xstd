#pragma once
#include <tuple>
#include "formatting.hpp"
#include "hashable.hpp"

namespace xstd
{
	// Defines a globally unique identifier.
	//
	struct guid
	{
		uint64_t low;
		uint64_t high;

		// Default constructor produces null.
		//
		_CONSTEVAL guid() : low( 0 ), high( 0 ) {}

		// Constructed by any hashable type as a constant expression.
		//
		template<typename T> requires ( !Same<std::decay_t<T>, guid> )
		_CONSTEVAL guid( T&& obj ) : low( make_hash( 0x49c54a9166f5c01cull, obj ).as64() ), high( make_hash( 0x7b0b6b0f8933b6a5ull, obj ).as64() ) {}

		// Default copy/move.
		//
		constexpr guid( guid&& ) noexcept = default;
		constexpr guid( const guid& ) = default;
		constexpr guid& operator=( guid&& ) noexcept = default;
		constexpr guid& operator=( const guid& ) = default;

		// String conversion.
		//
		std::string to_string() const
		{
			return xstd::fmt::str(
				"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				( low >> 0 ) & 0xFFFFFFFF, ( low >> 32 ) & 0xFFFF,
				( low >> 48 ) & 0xFFFF,    ( high >> 8 * 0 ) & 0xFF,
				( high >> 8 * 1 ) & 0xFF,  ( high >> 8 * 2 ) & 0xFF,
				( high >> 8 * 3 ) & 0xFF,  ( high >> 8 * 4 ) & 0xFF,
				( high >> 8 * 5 ) & 0xFF,  ( high >> 8 * 6 ) & 0xFF,
				( high >> 8 * 7 ) & 0xFF
			);
		}
		std::wstring to_wstring() const
		{
			return xstd::fmt::wstr(
				L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				( low >> 0 ) & 0xFFFFFFFF, ( low >> 32 ) & 0xFFFF,
				( low >> 48 ) & 0xFFFF,    ( high >> 8 * 0 ) & 0xFF,
				( high >> 8 * 1 ) & 0xFF,  ( high >> 8 * 2 ) & 0xFF,
				( high >> 8 * 3 ) & 0xFF,  ( high >> 8 * 4 ) & 0xFF,
				( high >> 8 * 5 ) & 0xFF,  ( high >> 8 * 6 ) & 0xFF,
				( high >> 8 * 7 ) & 0xFF
			);
		}

		// Make tiable for serialization.
		//
		constexpr auto tie() { return std::tie( low, high ); }

		// Basic comparison operators.
		//
		constexpr bool operator==( const guid& o ) const { return low == o.low && high == o.high; }
		constexpr bool operator!=( const guid& o ) const { return low != o.low || high != o.high; }
		constexpr bool operator<( const guid& o ) const { return  o.high > high || ( o.high == high && o.low > low ); }
	};
};