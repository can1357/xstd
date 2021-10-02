#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include "coro.hpp"
#include "event.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_SCHEDULER: If set, chore will pass OS a callback to help with the scheduling.
//
#ifdef XSTD_CHORE_SCHEDULER
	extern "C" void __cdecl XSTD_CHORE_SCHEDULER( void( __cdecl* callback )( void* ), void* cb_arg, int64_t priority, size_t delay_100ns, xstd::event_handle event_handle );
#endif

namespace xstd
{
	namespace impl
	{
		// Lambda.
		//
		template<typename F>
		struct function_store
		{
			static void __cdecl run( void* adr )
			{
				// Stateless:
				//
				if constexpr ( DefaultConstructable<F> && sizeof( F ) == sizeof( std::monostate ) )
				{
					F{}();
				}
				// State fits a pointer:
				//
				else if constexpr ( sizeof( F ) <= sizeof( void* ) )
				{
					auto& fn = *( F* ) &adr; 
					fn(); 
					std::destroy_at( &fn );
				}
				// Large state:
				//
				else
				{
					auto* fn = ( F* ) adr;
					( *fn )(); 
					delete fn;
				}
			}

			template<typename... Tx>
			static std::pair<void( __cdecl* )( void* ), void*> apply( Tx&&... args )
			{
				// Stateless:
				//
				void* ctx;
				if constexpr ( DefaultConstructable<F> && sizeof( F ) == sizeof( std::monostate ) )
				{
					ctx = nullptr;
				}
				// State fits a pointer:
				//
				else if constexpr ( sizeof( F ) <= sizeof( void* ) )
				{
					new ( &ctx ) F( std::forward<Tx>( args )... );
				}
				// Large state:
				//
				else
				{
					ctx = new F( std::forward<Tx>( args )... );
				}
				return { &run, ctx };
			}
		};

		// Function pointer.
		//
		template<typename T> requires ( Void<T> || TriviallyDestructable<T> )
		struct function_store<T(__cdecl*)()>
		{
			static std::pair<void( __cdecl* )( void* ), void*> apply( T( __cdecl* ptr )( ) )
			{
				return { ( void( __cdecl* )( void* ) ) ptr, nullptr };
			}
		};

		// Coroutine handle.
		//
		template<typename P>
		struct function_store<coroutine_handle<P>>
		{
			static void __cdecl run( void* adr )
			{
				coroutine_handle<P>::from_address( adr ).resume();
			}
			static std::pair<void( __cdecl* )( void* ), void*> apply( coroutine_handle<P> hnd )
			{
				return { &run, hnd.address() };
			}
		};

		// Flattens the function.
		//
		template<typename F>
		inline std::pair<void( __cdecl* )( void* ), void*> flatten( F&& fn )
		{
			return function_store<std::decay_t<F>>::apply( std::forward<F>( fn ) );
		}
	};

	// Registers a chore.
	//
	template<typename T>
	inline void chore( T&& fn, duration delay, int64_t priority = -1 )
	{
		int64_t tick_count = delay / 100ns;

#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, priority, tick_count < 1 ? 1ull : size_t( tick_count ), nullptr );
#else
		std::thread( [ fn = std::forward<T>( fn ), delay ]()
		{
			std::this_thread::sleep_for( delay );
			fn();
		} ).detach();
#endif
	}
	template<typename T>
	inline void chore( T&& fn, timestamp due_time, int64_t priority = -1 )
	{
		return chore( std::forward<T>( fn ), due_time - time::now(), priority );
	}
	template<typename T>
	inline void chore( T&& fn, event_handle evt, int64_t priority = -1 )
	{
		assume( evt != nullptr );

#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, priority, 0, evt );
#else
		std::thread( [ fn = std::forward<T>( fn ), evt ]()
		{
			event_primitive::wait_from_handle( evt );
			fn();
		} ).detach();
#endif
	}
	template<typename T>
	inline void chore( T&& fn, [[maybe_unused]] int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg] = impl::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, priority, 0, nullptr );
#else
		std::thread( std::forward<T>( fn ) ).detach();
#endif
	}
};