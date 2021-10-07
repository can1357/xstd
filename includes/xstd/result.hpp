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
		const char* full_value = nullptr;
		bool alloc_flag = false;

		// Wrappers allowing union-like access whilist staying constexpr.
		//
		inline constexpr void set_value( const char* new_value, bool allocated )
		{
			full_value = new_value;
			alloc_flag = allocated;
		}
		inline constexpr const char* get_value() const noexcept
		{
			return full_value;
		}
		inline constexpr bool is_allocated() const noexcept
		{
			if ( !std::is_constant_evaluated() )
				return alloc_flag;
			else
				return false;
		}

		// Construction from string literal.
		//
		inline constexpr exception() {}
		inline constexpr exception( std::nullptr_t ) {}
		inline constexpr exception( const char* str ) { set_value( str, false ); }

		// Construction from string.
		//
		inline exception( const std::string& str ) { assign_string( str.data(), str.length() ); }
		COLD void assign_string( const char* data, size_t length = std::string::npos )
		{
			if ( length == std::string::npos )
				length = strlen( data );
			char* res = new char[ length + 1 ];
			std::copy( data, data + length, res );
			res[ length ] = 0;
			reset();
			set_value( res, true );
		}
		
		// Construction from formatted string.
		//
		template<typename... Tx> requires ( sizeof...( Tx ) > 0 )
		inline exception( const char* fmt, Tx&&... args ) { assign_fmt( fmt, std::forward<Tx>( args )... ); }
		template<typename... Tx>
		COLD void assign_fmt( const char* fmt, Tx&&... args )
		{
			std::string buffer{ xstd::fmt::str( fmt, std::forward<Tx>( args )... ) };
			assign_string( buffer.data(), buffer.length() );
		}

		// Copy and move.
		//
		inline constexpr exception( const exception& other ) { assign( other ); }
		inline constexpr exception( exception&& other ) noexcept { swap( other ); }
		inline constexpr exception& operator=( const exception& other ) { assign( other ); return *this; }
		inline constexpr exception& operator=( exception&& other ) noexcept { swap( other ); return *this; }
		inline constexpr void swap( exception& o )
		{
			std::swap( full_value, o.full_value );
			std::swap( alloc_flag, o.alloc_flag );
		}
		inline constexpr void assign( const exception& o )
		{
			if ( !std::is_constant_evaluated() )
			{
				if ( o.is_allocated() )
					return assign_string( o.c_str() );
				else if ( is_allocated() )
					reset();
			}
			full_value = o.full_value;
			alloc_flag = o.alloc_flag;
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
			alloc_flag = false;
		}
		inline constexpr ~exception() { reset(); }

		// Observers.
		//
		inline constexpr bool has_value() const { return full_value; }
		inline constexpr explicit operator bool() const { return has_value(); }
		inline constexpr const char* c_str() const { return full_value ? get_value() : ""; }
		inline constexpr const char* data() const { return c_str(); }
		inline constexpr std::string_view get() const { return c_str(); }
		inline constexpr size_t size() const { return get().size(); };
		inline constexpr size_t length() const { return size(); };
		inline constexpr bool empty() const { return size() == 0; };
		inline constexpr const char& operator[]( size_t n ) const { return c_str()[ n ]; };
		inline constexpr auto begin() const { return get().begin(); }
		inline constexpr auto end() const { return get().end(); }
		inline std::string to_string() const { return c_str(); }
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
		static constexpr const char* success_value = nullptr;
		static constexpr const char* failure_value = "?";

		// Declare basic traits.
		//
		constexpr static inline bool is_success( const exception& o ) { return !o.has_value(); }
	};
	struct no_status {};
	template<>
	struct status_traits<no_status>
	{
		// Generic success and failure values.
		//
		static constexpr no_status success_value{};
		static constexpr no_status failure_value{};

		// Declare basic traits.
		//
		constexpr static inline bool is_success( const no_status& ) { return true; }
	};

	// Declares a light-weight object wrapping a result type with a status code.
	//
	struct in_place_success_t {};
	struct in_place_failure_t {};
	template <typename Value, typename Status>
	struct TRIVIAL_ABI basic_result
	{
		using value_type =     std::conditional_t<Same<Value, void>, std::monostate, Value>;
		using status_type =    Status;
		using traits =         status_traits<Status>;
		using store_type =     std::conditional_t<Same<value_type, std::monostate>, impl::optional_monostate, std::optional<value_type>>;

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
		template<typename T> requires ( Constructable<value_type, T> && ( !Constructable<Status, T> || Same<std::decay_t<T>, value_type> ) && !Same<std::decay_t<T>, in_place_success_t> && !Same<std::decay_t<T>, in_place_failure_t> )
		constexpr basic_result( T&& value ) : status( Status{ traits::success_value } ), result( std::forward<T>( value ) ) {}
		template<typename S> requires ( Constructable<Status, S> && ( !Constructable<value_type, S> || Same<std::decay_t<S>, Status> ) && !Same<std::decay_t<S>, in_place_success_t> && !Same<std::decay_t<S>, in_place_failure_t> )
		constexpr basic_result( S&& status ) : status( std::forward<S>( status ) ) 
		{
			if constexpr ( DefaultConstructable<value_type> )
				if ( traits::is_success( this->status ) )
					result.emplace();
		}
		template<typename T, typename S> requires ( !Same<std::decay_t<T>, in_place_success_t> && !Same<std::decay_t<T>, in_place_failure_t> )
		constexpr basic_result( T&& value, S&& status ) : status( std::forward<S>( status ) ), result( std::forward<T>( value ) ) {}
	
		template<typename... Args>
		constexpr basic_result( in_place_success_t, Args&&... args ) : status( Status{ traits::success_value } ), result( std::forward<Args>( args )... ) {}
		template<typename... Args>
		constexpr basic_result( in_place_failure_t, Args&&... args )
		{
			if constexpr ( sizeof...( Args ) != 0 )
				status = Status{ std::forward<Args>( args )... };
		}

		// Result conversion.
		//
		template<typename T> requires( !Same<T, value_type> && ( Constructable<value_type, const T&> || Same<value_type, std::monostate> ) )
		constexpr basic_result( const basic_result<T, Status>& other )
		{
			if constexpr ( Same<value_type, std::monostate> )
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
			Status st{ std::forward<S>( _status ) };
			if ( traits::is_success( st ) ) [[unlikely]]
				status = Status{ traits::failure_value };
			else
				std::swap( st, status );
		}
		constexpr value_type& emplace() requires ( Void<Value> || DefaultConstructable<Value> )
		{
			auto& v = result.emplace();
			status = Status{ traits::success_value };
			return v;
		}
		template<typename T>
		constexpr value_type& emplace( T&& value )
		{
			auto& v = result.emplace( std::forward<T>( value ) );
			status = Status{ traits::success_value };
			return v;
		}
		template<typename T, typename S>
		constexpr value_type& emplace( T&& value, S&& _status )
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
			if constexpr ( StringConvertible<value_type> && !Same<std::monostate, value_type> ) 
				return fmt::str( "(Result='%s')", fmt::as_string( result ) );
			else                                    
				return "(Success)";
		}
	
		// For accessing the value, replicate the std::optional interface.
		//
		constexpr const value_type& value() const &
		{
			assert();
			return result.value();
		}
		constexpr value_type& value() &
		{
			assert();
			return result.value();
		}
		constexpr value_type&& value() &&
		{
			assert();
			return std::move( result ).value();
		}
		constexpr value_type value_or( const value_type& o ) const
		{
			return success() ? result.value() : o;
		}
		constexpr value_type value_or( value_type o ) &&
		{
			return success() ? std::move( result ).value() : std::move( o );
		}

		// Conversion to optional.
		//
		constexpr std::optional<value_type> to_optional() &
		{
			if ( success() ) return result;
			else             return std::nullopt;
		}
		constexpr std::optional<value_type> to_optional() const&
		{
			if ( success() ) return result;
			else             return std::nullopt;
		}
		constexpr std::optional<value_type> to_optional() &&
		{
			if ( success() ) return std::move( result );
			else             return std::nullopt;
		}

		// Accessors.
		//
		constexpr decltype( auto ) operator->()
		{
			if constexpr ( PointerLike<value_type> )
				return value();
			else
				return &value(); 
		}
		constexpr decltype( auto ) operator->() const
		{
			if constexpr ( PointerLike<value_type> )
				return value();
			else
				return &value();
		}
		constexpr value_type& operator*() & { return value(); }
		constexpr value_type&& operator*() && { return std::move( *this ).value(); }
		constexpr const value_type& operator*() const & { return value(); }

		// Implement comparison against the stored result.
		//
		template<typename Cmp> requires EqualComparable<const value_type&, const Cmp&>
		constexpr bool operator==( const Cmp& other ) const { return success() && value() == other; }
		template<typename Cmp> requires NotEqualComparable<const value_type&, const Cmp&>
		constexpr bool operator!=( const Cmp& other ) const { return !success() || value() != other; }
	};

	template<typename T = void>
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