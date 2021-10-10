#pragma once
#include <type_traits>
#include <functional>
#include <utility>
#include "intrinsics.hpp"

#define HAS_STD_CORO MS_COMPILER

#if HAS_STD_CORO
	#include <coroutine>
#endif

// Define the core types.
//
#if !HAS_STD_CORO
	namespace std
	{
		template<typename Ret, typename... Args>
		struct coroutine_traits;
	};
#endif

namespace xstd
{
#if !HAS_STD_CORO
	namespace builtin
	{
#ifdef __INTELLISENSE__
		inline void* noop();
#else
		inline void* noop() { return __builtin_coro_noop(); }
#endif
		inline bool done( void* address ) { return __builtin_coro_done( address ); }
		inline void resume( void* address ) { __builtin_coro_resume( address ); }
		inline void destroy( void* address ) { __builtin_coro_destroy( address ); }
		template<typename P> inline void* to_handle( P& ref ) { return __builtin_coro_promise( &ref, alignof( P ), true ); }
		template<typename P> inline P& to_promise( void* hnd ) { return *( P* ) __builtin_coro_promise( hnd, alignof( P ), false ); }
	};
#endif
};

#if !HAS_STD_CORO
namespace std
{
	// Coroutine traits.
	//
	template<typename Ret, typename... Args>
	struct coroutine_traits;
	template<typename Ret, typename... Args> 
	requires requires { typename Ret::promise_type; }
	struct coroutine_traits<Ret, Args...>
	{
		using promise_type = typename Ret::promise_type;
	};

	// Coroutine handle.
	//
	template<typename Promise = void>
	struct coroutine_handle;
	template<>
	struct coroutine_handle<void>
	{
		void* handle = nullptr;

		inline constexpr coroutine_handle() noexcept : handle( nullptr ) {};
		inline constexpr coroutine_handle( std::nullptr_t ) noexcept : handle( nullptr ) {};

		inline constexpr void* address() const noexcept { return handle; }
		inline constexpr explicit operator bool() const noexcept { return handle; }

		inline void resume() const { xstd::builtin::resume( handle ); }
		inline void destroy() const noexcept { xstd::builtin::destroy( handle ); }
		inline bool done() const noexcept { return xstd::builtin::done( handle ); }
		inline void operator()() const { resume(); }
		
		inline constexpr bool operator==( coroutine_handle<> other ) const { return address() == other.address(); }
		inline constexpr bool operator!=( coroutine_handle<> other ) const { return address() != other.address(); }
		inline constexpr bool operator<( coroutine_handle<> other ) const { return address() < other.address(); }

		inline static coroutine_handle<> from_address( void* addr ) noexcept { coroutine_handle<> tmp{}; tmp.handle = addr; return tmp; }
	};
	template<typename Promise>
	struct coroutine_handle : coroutine_handle<>
	{
		inline constexpr coroutine_handle() noexcept {};
		inline constexpr coroutine_handle( std::nullptr_t ) noexcept : coroutine_handle<>( nullptr ) {};

		inline Promise& promise() const { return xstd::builtin::to_promise<Promise>( handle ); }
		inline static coroutine_handle<Promise> from_promise( Promise& pr ) noexcept { return from_address( xstd::builtin::to_handle( pr ) ); }
		inline static coroutine_handle<Promise> from_address( void* addr ) noexcept { coroutine_handle<Promise> tmp{}; tmp.handle = addr; return tmp; }
	};

	template<typename Pr>
	struct hash<coroutine_handle<Pr>>
	{
		inline size_t operator()( const coroutine_handle<Pr>& hnd ) const noexcept
		{
			return hash<size_t>{}( ( size_t ) hnd.address() );
		}
	};

	// No-op coroutine.
	//
	struct noop_coroutine_promise {};
	template <>
	struct coroutine_handle<noop_coroutine_promise>
	{
		void* handle;
		inline coroutine_handle() noexcept : handle( xstd::builtin::noop() ) {}

		inline constexpr operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address( handle ); }
		inline constexpr void* address() const noexcept { return handle; }
		inline constexpr explicit operator bool() const noexcept { return true; }

		inline constexpr void resume() const {}
		inline constexpr void destroy() const noexcept {}
		inline constexpr bool done() const noexcept { return false; }
		inline constexpr void operator()() const { resume(); }

		inline constexpr bool operator==( coroutine_handle<> other ) { return address() == other.address(); }
		inline constexpr bool operator!=( coroutine_handle<> other ) { return address() != other.address(); }
		inline constexpr bool operator<( coroutine_handle<> other ) { return address() < other.address(); }

		inline bool operator==( coroutine_handle<noop_coroutine_promise> ) { return true; }
		inline bool operator!=( coroutine_handle<noop_coroutine_promise> ) { return false; }
		inline bool operator<( coroutine_handle<noop_coroutine_promise> ) { return false; }

		inline noop_coroutine_promise& promise() const noexcept { return xstd::builtin::to_promise<noop_coroutine_promise>( handle ); }
	};
	using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;
	inline noop_coroutine_handle noop_coroutine() noexcept { return noop_coroutine_handle{}; }

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
};
#endif

namespace xstd
{
	// Import the select types.
	//
	template<typename Promise = void>
	using coroutine_handle = std::coroutine_handle<Promise>;
	template<typename Ret, typename... Args>
	using coroutine_traits = std::coroutine_traits<Ret, Args...>;
	using suspend_always =   std::suspend_always;
	using suspend_never =    std::suspend_never;

	using noop_coroutine_promise = std::noop_coroutine_promise;
	using noop_coroutine_handle =  std::noop_coroutine_handle;
	inline noop_coroutine_handle noop_coroutine() noexcept { return std::noop_coroutine(); }

	// Coroutine traits.
	//
	template<typename T>
	concept Coroutine = requires { typename coroutine_traits<T>::promise_type; };
	template<typename T>
	concept Awaitable = requires( T x ) { x.await_ready(); x.await_suspend(); x.await_resume(); };

	// Suspend type that terminates the coroutine.
	//
	struct suspend_terminate
	{
		inline bool await_ready() { return false; }
		inline void await_suspend( coroutine_handle<> hnd ) { hnd.destroy(); }
		inline void await_resume() const { unreachable(); }
	};

	// Unique coroutine that is destroyed on destruction by caller.
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
	
	// Helper to get the promise from a coroutine handle.
	//
	template<typename Promise>
	FORCE_INLINE static Promise& get_promise( coroutine_handle<> hnd )
	{ 
		return coroutine_handle<Promise>::from_address( hnd.address() ).promise();
	}

	// Helper to get the current coroutine handle / promise.
	//
#if __clang__
	// Clang:
	FORCE_INLINE static coroutine_handle<> this_coroutine( void* adr = __builtin_coro_frame() ) { return coroutine_handle<>::from_address( adr ); }
#else
	// Not implemented.
	FORCE_INLINE static coroutine_handle<> this_coroutine( void* adr = nullptr );
#endif

	template<typename C, typename Promise = typename coroutine_traits<C>::promise_type>
	FORCE_INLINE static Promise& this_promise( coroutine_handle<> h = this_coroutine() )
	{ 
		return get_promise<Promise>( h );
	}
};

// Clang requires coroutine_traits under std::experimental.
//
#if __clang__
namespace std::experimental 
{ 
	template<typename Promise = void>
	struct coroutine_handle : xstd::coroutine_handle<Promise> 
	{
		using xstd::coroutine_handle<Promise>::coroutine_handle;
		using xstd::coroutine_handle<Promise>::operator=;
	};

	template<typename Ret, typename... Args> 
		requires requires { typename xstd::coroutine_traits<Ret, Args...>::promise_type; }
	struct coroutine_traits
	{
		using promise_type = typename xstd::coroutine_traits<Ret, Args...>::promise_type;
	};
};
#endif