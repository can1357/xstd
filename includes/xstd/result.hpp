#pragma once
#include <optional>
#include <variant>
#include "type_helpers.hpp"
#include "assert.hpp"

#undef assert // If cassert hijacks the name, undefine.

// Pack all types in this namespace to improve storage.
#if HAS_MS_EXTENSIONS
	#pragma pack(push, 1)
#endif

namespace xstd
{
	namespace impl
	{
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

	// Define an exception type.
	//
	struct TRIVIAL_ABI exception
	{
		union
		{
			const char* full_value = nullptr;
			struct
			{
				int64_t pointer        : 63;
				uint64_t mismatch_bit  : 1;
			};
		};

		// Wrappers allowing union-like access whilist staying constexpr.
		//
		inline constexpr void set_value( const char* new_value, bool allocated )
		{
			full_value = new_value;
			if ( !std::is_constant_evaluated() )
			{
				if ( allocated )
				{
					mismatch_bit = !mismatch_bit;
					assume( ( const char* ) pointer != full_value );
				}
				else
				{
					assume( ( const char* ) pointer == full_value );
				}
			}
		}
		inline constexpr const char* get_value() const noexcept
		{
			if ( std::is_constant_evaluated() )
				return full_value;
			else
				return ( const char* ) ( uint64_t ) pointer;
		}
		inline constexpr bool is_allocated() const noexcept
		{
			if ( !std::is_constant_evaluated() )
				return ( const char* ) pointer != full_value;
			else
				return false;
		}

		// Construction from string view and string literal.
		//
		inline constexpr exception() {}
		inline constexpr exception( std::nullptr_t ) {}
		inline constexpr exception( const char* str ) { set_value( str, false ); }
		inline exception( std::string_view str ) { assign_string( str ); }

		// Copy and move.
		//
		inline constexpr exception( const exception& other ) { assign( other ); }
		inline constexpr exception( exception&& other ) noexcept { swap( other ); }
		inline constexpr exception& operator=( const exception& other ) { assign( other ); return *this; }
		inline constexpr exception& operator=( exception&& other ) noexcept { swap( other ); return *this; }
		inline constexpr void swap( exception& o )
		{
			std::swap( full_value, o.full_value );
		}
		inline constexpr void assign( const exception& o )
		{
			if ( !std::is_constant_evaluated() )
			{
				if ( o.is_allocated() )
					return assign_string( o.get() );
			}
			full_value = o.full_value;
		}
		COLD inline void assign_string( std::string_view str )
		{
			reset();
			size_t length = str.length();
			char* res = new char[ length + 1 ];
			std::copy( str.begin(), str.end(), res );
			res[ length ] = 0;
			set_value( res, true );
		}

		// Deallocate on deconstruction.
		//
		inline constexpr void reset()
		{
			if ( !std::is_constant_evaluated() )
			{
				if ( is_allocated() )
					delete[] get_value();
			}
			full_value = nullptr;
		}
		inline constexpr ~exception() { reset(); }

		// Observers.
		//
		inline constexpr bool empty() const { return !full_value; }
		inline constexpr explicit operator bool() const { return empty(); }

		// Getters.
		//
		inline std::string to_string() const { return c_str(); }
		inline constexpr const char* data() const { return c_str(); }
		inline constexpr const char* c_str() const { return full_value ? get_value() : ""; }
		inline constexpr std::string_view get() const { return c_str(); }
	};

	// Status traits.
	//
	template<typename T>
	struct status_traits : T::status_traits {};
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
	struct status_traits<exception>
	{
		// Generic success and failure values.
		//
		static constexpr exception success_value{ nullptr };
		static constexpr exception failure_value{ "?" };

		// Declare basic traits.
		//
		static inline bool is_success( const exception& o ) { return o.empty(); }
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
		template<typename T, typename S>
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
		template<typename T>
		constexpr Value& emplace( T&& value )
		{
			auto& v = result.emplace( std::forward<T>( value ) );
			status = Status{ traits::success_value };
			return v;
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
				xstd::cold_call( [ & ] { error( XSTD_ESTR( "Accessing failed result with: %s" ), message() ); } );
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

	template<typename T = std::monostate>
	using result = basic_result<T, exception>;
};

namespace std
{
	template<typename F, typename T, typename S>
	static constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>& res )
	{
		if ( res.success() ) return fn( res.value() );
		else                 return fn( res.status );
	}
	template<typename F, typename T, typename S>
	static constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>&& res )
	{
		if ( res.success() ) return fn( std::move( res.value() ) );
		else                 return fn( std::move( res.status ) );
	}

	template<typename F, typename T, typename S>
	static constexpr decltype( auto ) visit( F&& fn, const xstd::basic_result<T, S>& res )
	{
		if ( res.success() ) return fn( res.value() );
		else                 return fn( res.status );
	}

	template<typename F, typename T, typename S, typename... Ox>
	static constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>& res, Ox&&... rest )
	{
		return visit( [ & ] <typename... Tx> ( Tx&&... vx ) -> decltype( auto )
		{
			if ( res.success() ) return fn( res.value(), std::forward<Tx>( vx )... );
			else                 return fn( res.status, std::forward<Tx>( vx )... );
		}, std::forward<Ox>( rest )... );
	}

	template<typename F, typename T, typename S, typename... Ox>
	static constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>&& res, Ox&&... rest )
	{
		return visit( [ & ] <typename... Tx> ( Tx&&... vx ) -> decltype( auto )
		{
			if ( res.success() ) return fn( std::move( res.value() ), std::forward<Tx>( vx )... );
			else                 return fn( std::move( res.status ), std::forward<Tx>( vx )... );
		}, std::forward<Ox>( rest )... );
	}

	template<typename F, typename T, typename S, typename... Ox>
	static constexpr decltype( auto ) visit( F&& fn, const xstd::basic_result<T, S>& res, Ox&&... rest )
	{
		return visit( [ & ] <typename... Tx> ( Tx&&... vx ) -> decltype( auto )
		{
			if ( res.success() ) return fn( res.value(), std::forward<Tx>( vx )... );
			else                 return fn( res.status, std::forward<Tx>( vx )... );
		}, std::forward<Ox>( rest )... );
	}
};


#if HAS_MS_EXTENSIONS
	#pragma pack(pop)
#endif