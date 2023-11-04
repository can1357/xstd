#pragma once
#include <optional>
#include <variant>
#include "type_helpers.hpp"
#include "assert.hpp"

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
			template<typename... Tx> FORCE_INLINE constexpr optional_monostate( Tx&&... ) noexcept {};
			template<typename T> FORCE_INLINE constexpr optional_monostate& operator=( T&& ) noexcept { return *this; };
			FORCE_INLINE constexpr std::monostate& emplace() { return _value; }
			FORCE_INLINE constexpr std::monostate& emplace( std::monostate ) { return _value; }
			FORCE_INLINE constexpr std::monostate& value() const noexcept { return _value; }
			FORCE_INLINE constexpr operator std::optional<std::monostate>() const noexcept { return std::optional{ std::monostate{} }; }
		};
	};

	// Define an exception type.
	//
	struct TRIVIAL_ABI exception
	{
		union {
			const char* _str = nullptr;
			struct {
				uint64_t _allocated : 1;
				int64_t  _snptr     : 63;
			};
		};

		// Wrappers allowing union-like access whilist staying constexpr.
		//
		inline constexpr void set_value( const char* new_value, bool allocated ) {
			if ( !std::is_constant_evaluated() ) {
				_snptr =     (int64_t) (uint64_t) new_value;
				_allocated = allocated ? 1 : 0;
			} else {
				_str = new_value;
			}
		}
		inline constexpr const char* get_value() const noexcept {
			if ( !std::is_constant_evaluated() ) {
				return (const char*) uint64_t( _snptr );
			} else {
				return _str;
			}
		}
		inline constexpr bool is_allocated() const noexcept {
			if ( !std::is_constant_evaluated() )
				return _allocated;
			else
				return false;
		}

		// Construction from string literal.
		//
		inline constexpr exception() = default;
		inline constexpr exception( std::nullptr_t ) {}
		inline constexpr exception( std::nullopt_t ) {}
		inline constexpr exception( const char* str ) { set_value( str, false ); }

		// Construction from string.
		//
		inline exception( std::string_view str ) { assign_string( str.data(), str.length() ); }
		inline exception( const std::string& str ) { assign_string( str.data(), str.length() ); }
		NO_INLINE void assign_string( const char* data, size_t length ) {
			char* res;
			if ( _allocated ) {
				res = (char*) realloc( (char*) get_value(), length + 1 );
			} else {
				res = (char*) malloc( length + 1 );
			}
			memcpy( res, data, length );
			res[ length ] = 0;
			set_value( res, true );
		}
		
		// Construction from formatted string.
		//
		template<typename... Tx> requires ( sizeof...( Tx ) > 0 )
		inline exception( const char* fmt, Tx&&... args ) { assign_fmt( fmt, std::forward<Tx>( args )... ); }
		template<typename... Tx>
		NO_INLINE void assign_fmt( const char* fmt, Tx&&... args ) {
			char* base = nullptr;
			if ( _allocated ) {
				base = (char*) get_value();
			}
			xstd::fmt::into( [&]( size_t length ) { return ( base = (char*) realloc( base, length + 1 ) ); }, fmt, std::forward<Tx>( args )... );
			set_value( base, true );
		}

		// Copy and move.
		//
		inline constexpr exception( const exception& other ) { assign( other ); }
		inline constexpr exception( exception&& other ) noexcept { swap( other ); }
		inline constexpr exception& operator=( const exception& other ) { assign( other ); return *this; }
		inline constexpr exception& operator=( exception&& other ) noexcept { swap( other ); return *this; }
		inline constexpr void swap( exception& o ) {
			std::swap( _str, o._str );
		}
		inline constexpr void assign( const exception& o ) {
			if ( !std::is_constant_evaluated() ) {
				if ( o.is_allocated() )
					return assign_string( o.c_str(), strlen( o.c_str() ) );
				else if ( is_allocated() && _str != o._str )
					free( (void*) get_value() );
			}
			_str = o._str;
		}

		// Deallocate on deconstruction.
		//
		inline constexpr void reset() {
			if ( !std::is_constant_evaluated() ) {
				if ( is_allocated() )
					free( (void*) get_value() );
			}
			_str = nullptr;
		}
		inline constexpr ~exception() { reset(); }

		// Observers.
		//
		inline constexpr bool has_value() const { return _str != nullptr; }
		inline constexpr explicit operator bool() const { return has_value(); }
		inline constexpr const char* c_str() const { return _str ? get_value() : ""; }
		inline constexpr const char* data() const { return c_str(); }
		inline constexpr std::string_view get() const { return c_str(); }
		inline constexpr size_t size() const { return get().size(); };
		inline constexpr size_t length() const { return size(); };
		inline constexpr bool empty() const { return size() == 0; };
		inline constexpr const char& operator[]( size_t n ) const { return c_str()[ n ]; };
		inline constexpr auto begin() const { return get().begin(); }
		inline constexpr auto end() const { return get().end(); }
		inline std::string to_string() const { return c_str(); }

		// value_or implementation.
		//
		template<typename... Tx>
		inline constexpr exception value_or( Tx&&... args ) const& {
			if ( has_value() ) {
				return exception{ *this };
			} else {
				return exception{ std::forward<Tx>( args )... };
			}
		}
		template<typename... Tx>
		inline constexpr exception value_or( Tx&&... args ) & {
			if ( has_value() ) {
				return exception{ *this };
			} else {
				return exception{ std::forward<Tx>( args )... };
			}
		}
		template<typename... Tx>
		inline constexpr exception value_or( Tx&&... args ) && {
			if ( has_value() ) {
				return exception{ std::move( *this ) };
			} else {
				return exception{ std::forward<Tx>( args )... };
			}
		}
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
	namespace impl { struct no_status {}; };
	template<>
	struct status_traits<impl::no_status>
	{
		// Generic success and failure values.
		//
		static constexpr impl::no_status success_value{};
		static constexpr impl::no_status failure_value{};

		// Declare basic traits.
		//
		constexpr static inline bool is_success( const impl::no_status& ) { return true; }
	};

	// Declares a light-weight object wrapping a result type with a status code.
	//
	struct in_place_success_t {};
	struct in_place_failure_t {};
	template <typename Value, typename Status>
	struct TRIVIAL_ABI basic_result
	{
		// Traits.
		//
		using status_type =    std::conditional_t<Void<Status>, impl::no_status, Status>;
		using value_type =     std::conditional_t<Void<Value>,  std::monostate,  Value>;
		static constexpr bool has_status = !Same<status_type,   impl::no_status>;
		static constexpr bool has_value =  !Same<value_type,    std::monostate>;

		using traits =         status_traits<status_type>;
		using store_type =     std::conditional_t<has_value, std::optional<value_type>, impl::optional_monostate>;

		// Status code and the value itself.
		//
		status_type status;
		store_type result = std::nullopt;
	
		// Invalid value construction.
		//
		constexpr basic_result() : status{ traits::failure_value } {}
		constexpr basic_result( std::nullopt_t ) : basic_result() {}

		// Default copy and move.
		//
		constexpr basic_result( basic_result&& ) noexcept = default;
		constexpr basic_result( const basic_result& ) = default;
		constexpr basic_result& operator=( basic_result&& ) noexcept = default;
		constexpr basic_result& operator=( const basic_result& ) = default;

		// Consturction with value/state combination.
		//
		template<typename T> requires ( Constructible<value_type, T> && ( !Constructible<status_type, T> || Same<std::decay_t<T>, value_type> ) && !Same<std::decay_t<T>, in_place_success_t> && !Same<std::decay_t<T>, in_place_failure_t> )
		constexpr basic_result( T&& value ) : status{ traits::success_value }, result( std::forward<T>( value ) ) {}
		template<typename S> requires ( Constructible<status_type, S> && ( !Constructible<value_type, S> || Same<std::decay_t<S>, status_type> ) && !Same<std::decay_t<S>, in_place_success_t> && !Same<std::decay_t<S>, in_place_failure_t> )
		constexpr basic_result( S&& status ) : status( std::forward<S>( status ) ) 
		{
			if constexpr ( DefaultConstructible<value_type> )
				if ( traits::is_success( this->status ) )
					result.emplace();
		}
		template<typename T, typename S> requires ( !Same<std::decay_t<T>, in_place_success_t> && !Same<std::decay_t<T>, in_place_failure_t> )
		constexpr basic_result( T&& value, S&& status ) : status( std::forward<S>( status ) ), result( std::forward<T>( value ) ) {}
	
		template<typename... Args>
		constexpr basic_result( in_place_success_t, Args&&... args ) : status{ traits::success_value }, result( std::in_place_t{}, std::forward<Args>( args )... ) {}
		template<typename... Args>
		constexpr basic_result( in_place_failure_t, Args&&... args )
		{
			if constexpr ( sizeof...( Args ) != 0 )
				status = status_type{ std::forward<Args>( args )... };
			else
				status = status_type{ traits::failure_value };
		}

		// Setters.
		//
		template<typename S>
		constexpr void raise( S&& _status )
		{
			status_type st{ std::forward<S>( _status ) };
			if ( traits::is_success( st ) ) [[unlikely]]
				status = status_type{ traits::failure_value };
			else
				std::swap( st, status );
		}
		template<typename... Tx>
		constexpr value_type& emplace( Tx&&... value )
		{
			auto& v = result.emplace( std::forward<Tx>( value )... );
			status = status_type{ traits::success_value };
			return v;
		}
		
		// Observers to check for the state.
		//
		constexpr bool success() const { return traits::is_success( status ); }
		constexpr bool fail() const { return !traits::is_success( status ); }
		constexpr explicit operator bool() const { return success(); }
		NO_INLINE auto message() const
		{
			if constexpr ( StringConvertible<Status> )
				return fmt::as_string( status );
			else
				return success() ? XSTD_ESTR( "Success" ) : XSTD_ESTR( "Unknown error" );
		}
		COLD void assert_fail() const
		{
			std::string err = message();
			throw_fmt( message().c_str() );
		}
		FORCE_INLINE constexpr void assert() const
		{
			if ( !success() ) [[unlikely]] {
				assert_fail();
			}
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
		FORCE_INLINE constexpr const value_type& value() const &
		{
			assert();
			return result.value();
		}
		FORCE_INLINE constexpr value_type& value() &
		{
			assert();
			return result.value();
		}
		FORCE_INLINE constexpr value_type&& value() &&
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

		// Optional conversion.
		//
		constexpr std::optional<value_type> to_optional() const {
			if ( fail() ) 
				return std::nullopt;
			return result.value();
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
	inline constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>& res )
	{
		if ( res.success() ) return fn( res.value() );
		else                 return fn( res.status );
	}
	template<typename F, typename T, typename S>
	inline constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>&& res )
	{
		if ( res.success() ) return fn( std::move( res.value() ) );
		else                 return fn( std::move( res.status ) );
	}

	template<typename F, typename T, typename S>
	inline constexpr decltype( auto ) visit( F&& fn, const xstd::basic_result<T, S>& res )
	{
		if ( res.success() ) return fn( res.value() );
		else                 return fn( res.status );
	}

	template<typename F, typename T, typename S, typename... Ox>
	inline constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>& res, Ox&&... rest )
	{
		return visit( [ & ] <typename... Tx> ( Tx&&... vx ) -> decltype( auto )
		{
			if ( res.success() ) return fn( res.value(), std::forward<Tx>( vx )... );
			else                 return fn( res.status, std::forward<Tx>( vx )... );
		}, std::forward<Ox>( rest )... );
	}

	template<typename F, typename T, typename S, typename... Ox>
	inline constexpr decltype( auto ) visit( F&& fn, xstd::basic_result<T, S>&& res, Ox&&... rest )
	{
		return visit( [ & ] <typename... Tx> ( Tx&&... vx ) -> decltype( auto )
		{
			if ( res.success() ) return fn( std::move( res.value() ), std::forward<Tx>( vx )... );
			else                 return fn( std::move( res.status ), std::forward<Tx>( vx )... );
		}, std::forward<Ox>( rest )... );
	}

	template<typename F, typename T, typename S, typename... Ox>
	inline constexpr decltype( auto ) visit( F&& fn, const xstd::basic_result<T, S>& res, Ox&&... rest )
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