#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include "coro.hpp"
#include "event.hpp"

// [[Configuration]]
// XSTD_CHORE_SCHEDULER: If set, chore will pass OS the data to help with the scheduling.
//
#ifdef XSTD_CHORE_SCHEDULER
	extern "C" void __cdecl XSTD_CHORE_SCHEDULER( void( __cdecl* cb )( void* ), void* arg, int64_t delay_ns, xstd::event_handle evt );
#else
	#include "thread_pool.hpp"
	namespace xstd {
		inline thread_pool<> g_default_threadpool = {};
	};
	#define XSTD_CHORE_SCHEDULER xstd::g_default_threadpool
#endif

namespace xstd {
	namespace impl {
		// Lambda.
		//
		template<typename F>
		struct function_store {
			static void __cdecl run( void* adr ) {
				// Stateless:
				//
				if constexpr ( Empty<F> && DefaultConstructible<F> ) {
					F{}( );
				}
				// State fits a pointer:
				//
				else if constexpr ( sizeof( F ) <= sizeof( void* ) ) {
					auto& fn = *(F*) &adr;
					fn();
					std::destroy_at( &fn );
				}
				// Large state:
				//
				else {
					auto* fn = (F*) adr;
					( *fn )( );
					delete fn;
				}
			}

			template<typename... Tx>
			static std::pair<void( __cdecl* )( void* ), void*> apply( Tx&&... args ) {
				// Stateless:
				//
				void* ctx;
				if constexpr ( Empty<F> && DefaultConstructible<F> ) {
					ctx = nullptr;
				}
				// State fits a pointer:
				//
				else if constexpr ( sizeof( F ) <= sizeof( void* ) ) {
					new ( &ctx ) F( std::forward<Tx>( args )... );
				}
				// Large state:
				//
				else {
					ctx = new F( std::forward<Tx>( args )... );
				}
				return { &run, ctx };
			}
		};

		// Function pointer.
		//
		template<typename T> requires ( Void<T> || TriviallyDestructable<T> )
		struct function_store<T( __cdecl* )( )> {
			static std::pair<void( __cdecl* )( void* ), void*> apply( T( __cdecl* ptr )( ) ) {
				return { ( void( __cdecl* )( void* ) ) ptr, nullptr };
			}
		};

		// Coroutine handle.
		//
		template<>
		struct function_store<coroutine_handle<>>
		{
#if XSTD_CORO_KNOWN_STRUCT
			static std::pair<void( __cdecl* )( void* ), void*> apply( coroutine_handle<> hnd ) {
				auto& coro = coroutine_struct<>::from_handle( hnd );
				return { coro.fn_resume, &coro };
			}
#else
			static void __cdecl run( void* adr ) {
				coroutine_handle<>::from_address( adr ).resume();
			}
			static std::pair<void( __cdecl* )( void* ), void*> apply( coroutine_handle<> hnd ) {
				return { &run, hnd.address() };
			}
#endif
		};
		template<NonVoid P> struct function_store<coroutine_handle<P>> : function_store<coroutine_handle<>>{};

		// Flattens the function.
		//
		template<typename F>
		inline std::pair<void( __cdecl* )( void* ), void*> flatten( F&& fn ) {
			return function_store<std::decay_t<F>>::apply( std::forward<F>( fn ) );
		}
	};

	// Immediate chores.
	//
	template<typename T>
	inline void chore( T&& fn ) {
		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, 0, nullptr );
	}

	// Delayed chores.
	//
	template<typename T>
	inline void chore( T&& fn, duration delay ) {
		int64_t delay_ns = int64_t( delay / 1ns );
		if ( delay_ns < 1 ) delay_ns = 1;

		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, delay_ns, nullptr );
	}
	template<typename T>
	inline void chore( T&& fn, timestamp due_time ) {
		return chore( std::forward<T>( fn ), due_time - time::now() );
	}

	// Event triggered chores.
	//
	template<typename T>
	inline void chore( T&& fn, event_handle evt ) {
		assume( evt != nullptr );

		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, 0, evt );
	}

	// Event triggered chores with timeout.
	//
	template<typename T>
	inline void chore( T&& fn, event_handle evt, duration delay ) {
		int64_t delay_ns = int64_t( delay / 1ns );
		if ( delay_ns < 1 ) delay_ns = 1;

		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, delay_ns, evt );
	}
	template<typename T>
	inline void chore( T&& fn, event_handle evt, timestamp due_time ) {
		return chore( std::forward<T>( fn ), evt, due_time - time::now() );
	}
};