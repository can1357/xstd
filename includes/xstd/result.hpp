// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once
#include <optional>
#include "type_helpers.hpp"
#include "assert.hpp"

#undef assert // If cassert hijacks the name, undefine.

namespace xstd
{
	struct no_value_t {};
	struct default_result {};

	// Status traits.
	//
	template<typename T>
	struct status_traits;

	template<>
	struct status_traits<bool>
	{
		// Generic success and failure values.
		//
		static constexpr bool success_value = true;
		static constexpr bool failure_value = false;

		// Declare basic traits.
		//
		constexpr static inline bool is_success( bool v ) { return v; }
	};

	template<>
	struct status_traits<std::string>
	{
		// Generic success and failure values.
		//
		static constexpr const char* success_value = XSTD_CSTR( "" );
		static constexpr const char* failure_value = XSTD_ESTR( "?" );

		// Declare basic traits.
		//
		static inline bool is_success( const std::string& v ) { return v.empty(); }
	};


	// Declares a light-weight object wrapping a result type with a status code.
	//
	template <typename Val, typename Status>
	struct basic_result
	{
		using value_type =     Val;
		using status_type =    Status;
		using traits =         status_traits<Status>;

		// Status code and the value itself.
		//
		Status status = Status{ traits::failure_value };
		std::optional<Val> result = std::nullopt;

		// Invalid value construction.
		//
		constexpr basic_result() {}
		constexpr basic_result( std::nullopt_t ) {}

		// Default value construction via tag.
		//
		constexpr basic_result( default_result ) requires DefaultConstructable<Val> : result( Val{} ), status( Status{ traits::success_value } ) {}

		// Consturction with value/state combination.
		//
		template<typename T> requires ( Constructable<Val, T> )
		constexpr basic_result( T&& value ) : result( std::forward<T>( value ) ), status( Status{ traits::success_value } ) {}
		template<typename S> requires ( Constructable<Status, S> && !Constructable<Val, S> )
		constexpr basic_result( S&& status ) : status( std::forward<S>( status ) ) 
		{
			if ( traits::is_success( this->status ) )
				if constexpr ( DefaultConstructable<Val> )
					result.emplace();
		}
		template<typename T, typename S>  requires ( Constructable<Val, T> && Constructable<Status, S> )
		constexpr basic_result( T&& value, S&& status ) : result( std::forward<T>( value ) ), status( std::forward<S>( status ) ) {}

		// Default copy and move.
		//
		constexpr basic_result( basic_result&& ) noexcept = default;
		constexpr basic_result( const basic_result& ) = default;
		constexpr basic_result& operator=( basic_result&& ) noexcept = default;
		constexpr basic_result& operator=( const basic_result& ) = default;

		// Setters.
		//
		template<typename S>
		constexpr void raise( S&& _status )
		{
			status = std::forward<S>( _status );
		}
		template<typename T, typename S>
		constexpr Val& emplace( T&& value, S&& _status )
		{
			auto& v = result.emplace( std::forward<T>( value ) );
			status = std::forward<S>( _status );
			return v;
		}
		
		// Observers to check for the state.
		//
		constexpr bool success() const { return traits::is_success( status ); }
		constexpr bool fail() const { return !traits::is_success( status ); }
		constexpr explicit operator bool() const { return success(); }
		auto message() const
		{
			if constexpr ( StringConvertible<Status> )
				return fmt::as_string( status );
			else
				return success() ? XSTD_ESTR( "Success" ) : XSTD_ESTR( "Unknown error" );
		}
		constexpr void assert() const
		{
			if ( !success() ) [[unlikely]]
				xstd::error( XSTD_ESTR( "Accessing failed result with: %s" ), message() );
		}

		// String conversion.
		//
		std::string to_string() const
		{
			if ( fail() ) 
				return fmt::str( XSTD_CSTR( "(Fail='%s')" ), message() );

			if constexpr ( StringConvertible<Val> ) return fmt::str( XSTD_CSTR( "(Result=%s)" ), xstd::fmt::as_string( result ) );
			else                                    return XSTD_CSTR( "(Success)" );
		}

		// For accessing the value, replicate the std::optional interface.
		//
		constexpr const Val& value() const &
		{
			assert();
			return result.value();
		}
		constexpr Val& value() &
		{
			assert();
			return result.value();
		}
		constexpr Val&& value() &&
		{
			assert();
			return std::move( result ).value();
		}

		// Accessors.
		//
		constexpr decltype(auto) operator->()
		{ 
			if constexpr ( PointerLike<Val> )
				return value();
			else
				return &value(); 
		}
		constexpr Val& operator*() { return value(); }
		constexpr const Val& operator*() const { return value(); }
		constexpr decltype( auto ) operator->() const
		{
			if constexpr ( PointerLike<Val> )
				return value();
			else
				return &value();
		}
	};

	template<typename T = no_value_t, typename S = bool>
	using result = basic_result<T, S>;

	template<typename T = no_value_t>
	using string_result = basic_result<T, std::string>;
};