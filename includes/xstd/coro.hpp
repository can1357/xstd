#pragma once
#include <type_traits>
#include <functional>
#include <utility>
#include "intrinsics.hpp"

#if MS_COMPILER
	#include <experimental/coroutine>
#endif

// Define the core types.
//
namespace xstd
{
#if !MS_COMPILER
	// Builtins.
	//
	namespace builtin
	{
		inline bool done( void* address ) { return __builtin_coro_done( address ); }
		inline void resume( void* address ) { __builtin_coro_resume( address ); }
		inline void destroy( void* address ) { __builtin_coro_destroy( address ); }
		template<typename P> inline void* to_handle( P& ref ) { return __builtin_coro_promise( &ref, alignof( P ), true ); }
		template<typename P> inline P& to_promise( void* hnd ) { return *( P* ) __builtin_coro_promise( hnd, alignof( P ), false ); }
	};
#endif

	template<typename T>
	concept Coroutine = requires{ typename T::promise_type; };
	template<typename T>
	concept Awaitable = requires( T x ) { x.await_ready(); x.await_suspend(); x.await_resume(); };


	// Types that are exported as std.
	//
	namespace coro_export
	{
#if !MS_COMPILER
		// Coroutine traits.
		//
		template<Coroutine Ret, typename... Args>
		struct coroutine_traits { using promise_type = typename Ret::promise_type; };

		// Coroutine handle.
		//
		template<typename Promise = void>
		struct coroutine_handle;
		template<>
		struct coroutine_handle<void>
		{
			void* handle = nullptr;

			inline coroutine_handle() : handle( nullptr ) {};
			inline coroutine_handle( std::nullptr_t ) : handle( nullptr ) {};

			inline constexpr void* address() const noexcept { return handle; }
			inline constexpr explicit operator bool() const noexcept { return handle; }

			inline void resume() const { builtin::resume( handle ); }
			inline void destroy() const { builtin::destroy( handle ); }
			inline bool done() const { return builtin::done( handle ); }
			inline void operator()() const { resume(); }

			inline bool operator<( const coroutine_handle& o ) const noexcept { return handle < o.handle; }
			inline bool operator==( const coroutine_handle& o ) const noexcept { return handle == o.handle; }
			inline bool operator!=( const coroutine_handle& o ) const noexcept { return handle != o.handle; }

			inline static coroutine_handle<> from_address( void* addr ) noexcept { coroutine_handle<> tmp{}; tmp.handle = addr; return tmp; }
		};
		template<typename Promise>
		struct coroutine_handle : coroutine_handle<>
		{
			inline coroutine_handle() {};
			inline coroutine_handle( std::nullptr_t ) : coroutine_handle<>( nullptr ) {};

			inline Promise& promise() const { return builtin::to_promise<Promise>( handle ); }
			inline static coroutine_handle<Promise> from_promise( Promise& pr ) noexcept { return from_address( builtin::to_handle( pr ) ); }
			inline static coroutine_handle<Promise> from_address( void* addr ) noexcept { coroutine_handle<Promise> tmp{}; tmp.handle = addr; return tmp; }
		};

		// Dummy awaitables.
		//
		struct suspend_never
		{
			inline bool await_ready() const noexcept { return true; }
			inline void await_suspend( coroutine_handle<> ) const noexcept {}
			inline void await_resume() const noexcept {}
		};
		struct suspend_always
		{
			inline bool await_ready() const noexcept { return false; }
			inline void await_suspend( coroutine_handle<> ) const noexcept {}
			inline void await_resume() const noexcept {}
		};
#else
		template<Coroutine Ret, typename... Args> using coroutine_traits = std::experimental::coroutine_traits<Ret, Args...>;
		template<typename Promise>                using coroutine_handle = std::experimental::coroutine_handle<Promise>;
		using suspend_never =  std::experimental::suspend_never;
		using suspend_always = std::experimental::suspend_always;
#endif

		struct suspend_terminate
		{
			inline bool await_ready() { return false; }
			inline void await_suspend( coroutine_handle<> hnd ) { hnd.destroy(); }
			inline void await_resume() const { unreachable(); }
		};
	};
	using namespace coro_export;

	// Unique coroutine handle.
	//
	template<typename Pr>
	struct unique_coroutine
	{
		coroutine_handle<Pr> hnd{ nullptr };

		// Null handle.
		//
		inline constexpr unique_coroutine() {}
		inline constexpr unique_coroutine( std::nullptr_t ) {}

		// Constructed by handle.
		//
		inline unique_coroutine( coroutine_handle<Pr> hnd ) : hnd{ hnd } {}

		// Constructed by promise.
		//
		inline unique_coroutine( Pr& p ) : hnd{ coroutine_handle<Pr>::from_promise( p ) } {}

		// No copy allowed.
		//
		inline unique_coroutine( const unique_coroutine& ) = delete;
		inline unique_coroutine& operator=( const unique_coroutine& ) = delete;

		// Move by swap.
		//
		inline unique_coroutine( unique_coroutine&& o ) noexcept : hnd( o.release() ) {}
		inline unique_coroutine& operator=( unique_coroutine&& o ) noexcept { std::swap( hnd, o.hnd ); return *this; }

		// Redirections.
		//
		inline void resume() const { hnd.resume(); }
		inline bool done() const { return hnd.done(); }
		inline Pr& promise() const { return hnd.promise(); }
		inline void operator()() const { return resume(); }
		inline operator coroutine_handle<Pr>() const { return hnd; }

		// Getter and validity check.
		//
		inline coroutine_handle<Pr> get() const { return hnd; }
		inline explicit operator bool() const { return ( bool ) hnd; }

		// Reset and release.
		//
		inline coroutine_handle<Pr> release() { return std::exchange( hnd, nullptr ); }
		inline void reset() { if ( auto h = release() ) h.destroy(); }

		// Reset on destruction.
		//
		~unique_coroutine() { reset(); }
	};
};
#if !MS_COMPILER
	namespace std 
	{ 
		using namespace xstd::coro_export; 
		template<typename Pr>
		struct hash<coroutine_handle<Pr>>
		{
			inline size_t operator()( const coroutine_handle<Pr>& hnd ) const noexcept 
			{ 
				return hash<size_t>{}( ( size_t ) hnd.address() );
			}
		};
	};
	namespace std::experimental { using namespace xstd::coro_export; };
#endif

namespace xstd
{
	// Helper to get the promise from a coroutine handle.
	//
	template<typename Promise>
	FORCE_INLINE static Promise& get_promise( coroutine_handle<> hnd )
	{ 
		return coroutine_handle<Promise>::from_address( hnd.address() ).promise();
	}

	// Helper to get the current coroutine handle / promise.
	//
#if GNU_COMPILER
	// Clang:
	FORCE_INLINE static coroutine_handle<> this_coroutine( void* adr = __builtin_coro_frame() ) { return coroutine_handle<>::from_address( adr ); }
#else
	// MSVC:
	FORCE_INLINE static coroutine_handle<> this_coroutine( void* adr = _coro_frame_ptr() ) { return coroutine_handle<>::from_address( adr ); }
#endif

	template<typename C, typename Promise = typename coroutine_traits<C>::promise_type>
	FORCE_INLINE static Promise& this_promise( coroutine_handle<> h = this_coroutine() )
	{ 
		return get_promise<Promise>( h );
	}
};