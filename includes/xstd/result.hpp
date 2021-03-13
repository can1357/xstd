#pragma once
#include <optional>
#include "type_helpers.hpp"
#include "assert.hpp"

#undef assert // If cassert hijacks the name, undefine.

namespace xstd
{
	struct default_result {};

	// Status traits.
	//
	template<typename T>
	struct status_traits : T {};

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
		static constexpr char success_value[] = "";
		static constexpr char failure_value[] = "?";

		// Declare basic traits.
		//
		static inline bool is_success( const std::string& v ) { return v.empty(); }
	};

	// Declares a light-weight object wrapping a result type with a status code.
	//
	template <typename Value, typename Status>
	struct basic_result
	{
		using value_type =     Value;
		using status_type =    Status;
		using traits =         status_traits<Status>;

		// Status code and the value itself.
		//
		std::optional<Value> result = std::nullopt;
		Status status = Status{ traits::failure_value };

		// Invalid value construction.
		//
		constexpr basic_result() {}
		constexpr basic_result( std::nullopt_t ) {}

		// Default value construction via tag.
		//
		constexpr basic_result( default_result ) requires DefaultConstructable<Value> : result( Value{} ), status( Status{ traits::success_value } ) {}

		// Consturction with value/state combination.
		//
		template<typename T> requires ( Constructable<Value, T&&> && ( !Constructable<Status, T&&> || Same<std::decay_t<T>, Value> ) )
		constexpr basic_result( T&& value ) : result( std::forward<T>( value ) ), status( Status{ traits::success_value } ) {}
		template<typename S> requires ( Constructable<Status, S&&> && ( !Constructable<Value, S&&> || Same<std::decay_t<S>, Status> ) )
		constexpr basic_result( S&& status ) : status( std::forward<S>( status ) ) 
		{
			if ( traits::is_success( this->status ) )
				if constexpr ( DefaultConstructable<Value> )
					result.emplace();
		}
		template<typename T, typename S>  requires ( Constructable<Value, T&&> && Constructable<Status, S&&> )
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
		constexpr Value& emplace( T&& value, S&& _status )
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
				error( XSTD_ESTR( "Accessing failed result with: %s" ), message() );
		}

		// String conversion.
		//
		std::string to_string() const
		{
			if ( fail() ) 
				return fmt::str( "(Fail='%s')", message() );
			if constexpr ( StringConvertible<Value> ) 
				return fmt::str( "(Result='%s')", fmt::as_string( result ) );
			else                                    
				return "(Success)";
		}

		// For accessing the value, replicate the std::optional interface.
		//
		constexpr const Value& value() const &
		{
			assert();
			return result.value();
		}
		constexpr Value& value() &
		{
			assert();
			return result.value();
		}
		constexpr Value&& value() &&
		{
			assert();
			return std::move( result ).value();
		}
		constexpr Value value_or( const Value& o ) const
		{
			return success() ? result.value() : o;
		}

		// Accessors.
		//
		constexpr decltype(auto) operator->()
		{ 
			if constexpr ( PointerLike<Value> )
				return value();
			else
				return &value(); 
		}
		constexpr Value& operator*() { return value(); }
		constexpr const Value& operator*() const { return value(); }
		constexpr decltype( auto ) operator->() const
		{
			if constexpr ( PointerLike<Value> )
				return value();
			else
				return &value();
		}
	};

	template<typename T = std::monostate, typename S = bool>
	using result = basic_result<T, S>;

	template<typename T = std::monostate>
	using string_result = basic_result<T, std::string>;
};