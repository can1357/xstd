#pragma once
#include <optional>
#include "type_helpers.hpp"
#include "assert.hpp"

#undef assert // If cassert hijacks the name, undefine.

// Pack all types in this namespace to improve storage.
#if HAS_MS_EXTENSIONS
	#pragma pack(push, 1)
#endif

namespace xstd
{
	struct default_result {};

	namespace impl
	{
		template<typename T> concept HasInlineStTraits = requires( const T& x ) { T{ T::success_value }; T{ T::failure_value }; ( bool ) T::is_success( x ); };
		template<typename T> concept HasInlineMfTraits = requires( const T& x ) { T{ T::success_value }; T{ T::failure_value }; ( bool ) x.is_success(); };
		template<typename T> concept HasInlineAutoTraits = !HasInlineStTraits<T> && !HasInlineMfTraits<T> && requires( const T& x ) { T{ T::success_value }; T{ T::failure_value }; x.operator bool(); };
		template<typename T> concept HasExternTraits = requires { T::status_traits::is_success( std::declval<const T&>() ); T{ T::status_traits::success_value }; T{ T::status_traits::failure_value }; };

		template<typename T>
		struct default_traits {};

		// Fixed success/failure value + static function for success check.
		//
		template<HasInlineStTraits T>
		struct default_traits<T> 
		{
			static constexpr auto success_value = T::success_value;
			static constexpr auto failure_value = T::failure_value;
			constexpr static inline bool is_success( const T& v ) { return T::is_success( v ); }
		};

		// Fixed success/failure value + member function for success check.
		//
		template<HasInlineMfTraits T>
		struct default_traits<T>
		{
			static constexpr auto success_value = T::success_value;
			static constexpr auto failure_value = T::failure_value;
			constexpr static inline bool is_success( const T& v ) { return v.is_success(); }
		};

		// Fixed success/failure value + decay to bool for success check.
		//
		template<HasInlineAutoTraits T>
		struct default_traits<T>
		{
			static constexpr auto success_value = T::success_value;
			static constexpr auto failure_value = T::failure_value;
			constexpr static inline bool is_success( const T& v ) { return v.operator bool(); }
		};

		// Redirection to external class.
		//
		template<HasExternTraits T>
		struct default_traits<T> : T::status_traits {};

		// Optimization for monostate.
		//
		struct TRIVIAL_ABI optional_monostate
		{
			inline static std::monostate _value = {};
			__forceinline constexpr optional_monostate() noexcept {};
			__forceinline constexpr optional_monostate( std::nullopt_t ) noexcept {};
			__forceinline constexpr optional_monostate( std::monostate ) noexcept {};
			__forceinline constexpr optional_monostate( optional_monostate&& ) noexcept {};
			__forceinline constexpr optional_monostate( const optional_monostate& ) noexcept {};
			__forceinline constexpr optional_monostate& operator=( optional_monostate&& ) noexcept { return *this; };
			__forceinline constexpr optional_monostate& operator=( const optional_monostate& ) noexcept { return *this; };
			__forceinline constexpr std::monostate& emplace() { return _value; }
			__forceinline constexpr std::monostate& emplace( std::monostate ) { return _value; }
			__forceinline constexpr std::monostate& value() const noexcept { return _value; }
			__forceinline constexpr operator std::optional<std::monostate>() const noexcept { return std::optional{ std::monostate{} }; }
		};
	};

	// Status traits.
	//
	template<typename T>
	struct status_traits : impl::default_traits<T> {};

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
	struct TRIVIAL_ABI basic_result
	{
		using value_type =     Value;
		using status_type =    Status;
		using traits =         status_traits<Status>;
		using store_type =     std::conditional_t<Same<Value, std::monostate>, impl::optional_monostate, std::optional<Value>>;
		// Status code and the value itself.
		//
		Status status = Status{ traits::failure_value };
		store_type result = std::nullopt;

		// Invalid value construction.
		//
		constexpr basic_result() {}
		constexpr basic_result( std::nullopt_t ) {}

		// Default value construction via tag.
		//
		constexpr basic_result( default_result ) requires DefaultConstructable<Value> : status( Status{ traits::success_value } ), result( Value{} ) {}

		// Consturction with value/state combination.
		//
		template<typename T> requires ( Constructable<Value, T&&> && ( !Constructable<Status, T&&> || Same<std::decay_t<T>, Value> ) )
		constexpr basic_result( T&& value ) : status( Status{ traits::success_value } ), result( std::forward<T>( value ) ) {}
		template<typename S> requires ( Constructable<Status, S&&> && ( !Constructable<Value, S&&> || Same<std::decay_t<S>, Status> ) )
		constexpr basic_result( S&& status ) : status( std::forward<S>( status ) ) 
		{
			if constexpr ( DefaultConstructable<Value> )
				if ( traits::is_success( this->status ) )
					result.emplace();
		}
		template<typename T, typename S>  requires ( Constructable<Value, T&&> && Constructable<Status, S&&> )
		constexpr basic_result( T&& value, S&& status ) : status( std::forward<S>( status ) ), result( std::forward<T>( value ) ) {}

		// Result conversion.
		//
		template<typename T> requires( !Same<T, Value> && ( Constructable<Value, const T&> || Same<Value, std::monostate> ) )
		constexpr basic_result( const basic_result<T, Status>& other )
		{
			if constexpr ( Same<Value, std::monostate> )
			{
				status = other.status;
				if ( traits::is_success( status ) )
					result.emplace();
			}
			else
			{
				status = other.status;
				if ( traits::is_success( status ) )
					result.emplace( other.value() );
			}
		}

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
			if constexpr ( StringConvertible<Value> && !Same<std::monostate, Value> ) 
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
		constexpr decltype( auto ) operator->() const
		{
			if constexpr ( PointerLike<Value> )
				return value();
			else
				return &value();
		}
		constexpr Value& operator*() & { return value(); }
		constexpr Value&& operator*() && { return std::move( *this ).value(); }
		constexpr const Value& operator*() const & { return value(); }
	};

	template<typename T = std::monostate, typename S = bool>
	using result = basic_result<T, S>;

	template<typename T = std::monostate>
	using string_result = basic_result<T, std::string>;
};

#if HAS_MS_EXTENSIONS
	#pragma pack(pop)
#endif