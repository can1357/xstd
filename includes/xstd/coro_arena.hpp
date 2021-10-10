#pragma once
#include "coro.hpp"
#include <span>
#include <optional>

namespace xstd::coro
{
	// Declare a coroutine arena for allocation.
	//
	template<int N = -1>
	struct arena;

	// Arena with size information erased.
	//
	template<>
	struct arena<-1>
	{
		char* begin;
		char* end;

		// Allocation.
		//
		inline constexpr char* allocate( size_t n ) noexcept
		{
			// Align the request length.
			//
			n = ( n + 7 ) & ~7;

			// Try to allocate from the arena.
			//
			auto* it = begin + n;
			if ( it <= end ) [[likely]] {
				 begin = it;
				 return it - n;
			}
			return nullptr;
		}

		// Construction by view.
		//
		constexpr arena( char* data, size_t length ) noexcept : begin( data ), end( data + length ) {}
		constexpr arena( std::span<char> range ) noexcept : arena( range.data(), range.size() ) {}
	};

	// Null arena with requirement of heap ellision.
	//
	template<>
	struct arena<0> : arena<>
	{
		constexpr arena() noexcept : arena<>( nullptr, 0 ) {}
	};

	// Arena backed by constant size buffer.
	//
	template<int N> requires( N > 0 )
	struct arena<N> : arena<>
	{
		char buffer[ N ];
		constexpr arena() noexcept : arena<>( buffer ) {}

		// No move, no copy.
		//
		arena( arena&& ) = delete;
		arena( const arena& ) = delete;
		arena& operator=( arena&& ) = delete;
		arena& operator=( const arena& ) = delete;
	};
	
	// In-place coroutine that forces allocation from the arena.
	//
	template<typename T>
	struct in_place : T
	{
		struct promise_type : coroutine_traits<T>::promise_type
		{
			inline constexpr void* operator new( [[maybe_unused]] size_t n ) noexcept { return nullptr; }
			inline constexpr void* operator new( size_t n, arena<>& arena ) noexcept { return arena.allocate( n ); }
			inline constexpr void operator delete( [[maybe_unused]] void* p ) noexcept {}
		};

		// Constructor for get_return_object.
		//
		inline constexpr in_place( T&& retval ) : T( std::move( retval ) ) {}
	};
	
	// In-place coroutine that tries to use the arena, in case it is not possible fails.
	//
	template<typename T>
	struct try_in_place
	{
		struct promise_type : coroutine_traits<T>::promise_type
		{
			inline void* operator new( [[maybe_unused]] size_t n ) noexcept { return nullptr; }
			inline constexpr void* operator new( size_t n, arena<>& arena ) noexcept { return arena.allocate( n ); }
			inline constexpr void operator delete( [[maybe_unused]] void* p ) noexcept {}
			static constexpr try_in_place get_return_object_on_allocation_failure() { return std::nullopt; }
		};

		// Storage of the coroutine type.
		//
		std::optional<T> result;

		// Constructors for get_return_object and get_return_object_on_allocation_failure.
		//
		inline constexpr try_in_place( std::nullopt_t ) {}
		inline constexpr try_in_place( T&& retval ) : result( std::move( retval ) ) {}

		// State check by cast to boolean.
		//
		inline constexpr explicit operator bool() const noexcept { return result.has_value(); }

		// Decay to value type.
		//
		inline constexpr operator T&() & { return result.value(); }
		inline constexpr operator T&&() && { return std::move( result ).value(); }
		inline constexpr operator const T&() const& { return result.value(); }
		inline constexpr T& operator*()& { return result.value(); }
		inline constexpr T&& operator*()&& { return std::move( result ).value(); }
		inline constexpr const T& operator*() const& { return result.value(); }
	};

	// In-place coroutine that optionally alocates from the arena.
	//
	template<typename T>
	struct in_place_if : T
	{
		struct promise_type : coroutine_traits<T>::promise_type
		{
			bool externally_allocated; // Left undefined.

			inline static promise_type& resolve( void* frame ) 
			{ 
				return coroutine_handle<promise_type>::from_address( frame ).promise(); 
			}
			inline void* operator new( size_t n, arena<>& arena ) noexcept
			{
				if ( void* in_place = arena.allocate( n ) )
				{
					resolve( in_place ).externally_allocated = false;
					return in_place;
				}
				else
				{
					void* external = ::operator new( n );
					resolve( external ).externally_allocated = true;
					return external;
				}
			}
			inline void operator delete( void* p ) noexcept
			{
				if ( resolve( p ).externally_allocated )
					::operator delete( p );
			}
		};

		// Constructor for get_return_object.
		//
		inline constexpr in_place_if( T&& retval ) : T( std::move( retval ) ) {}
	};
};