#pragma once
#include "type_helpers.hpp"
#include "time.hpp"
#include "event.hpp"

namespace xstd
{
	namespace impl
	{
		// Basic traits every timer has.
		//
		struct timer_base
		{
			event_base signal_event = {};
			duration interval;
			bool canceled = false;

			timer_base( duration interval ) : interval( interval ) {}

			// Signal helper.
			//
			void signal() { signal_event.notify(); }
		};

		// Implementation based on the functor passed.
		//
		template<typename F>
		struct timer_store : timer_base
		{
			F functor;
			timer_store( F&& fn, duration interval ) : timer_base( interval ), functor( std::forward<F>( fn ) ) {}

			void execute()
			{
				// Reset signal event, check for cancellation.
				//
				timer_base::signal_event.reset();
				if ( timer_base::canceled ) [[unlikely]]
				{
					delete this;
					return;
				}

					// Execute the function.
					//
				functor();

				// Schedule again.
				//
				schedule( false );
			}
			__forceinline void schedule( bool first_time )
			{
				if ( first_time )
					xstd::chore( [ & ] { execute(); } );
				else
					xstd::chore( [ & ] { execute(); }, timer_base::signal_event.handle(), timer_base::interval );
			}
		};
	};

	// Unique timer class.
	//
	struct timer
	{
		impl::timer_base* base = nullptr;

		// Null timer.
		//
		constexpr timer() {}
		constexpr timer( std::nullopt_t ) {}

		// Construct by pointer.
		//
		explicit constexpr timer( impl::timer_base* base ) : base( base ) {}

		// Constructed by the functor and the interval.
		//
		template<typename F, Duration D>
		timer( F&& fn, D interval, bool deferred_start = false )
		{
			auto* store = new impl::timer_store<F>( std::forward<F>( fn ), std::chrono::duration_cast< duration >( interval ) );
			store->schedule( !deferred_start );
			base = store;
		}

		// Move by swap.
		//
		constexpr timer( timer&& other ) noexcept : base( std::exchange( other.base, nullptr ) ) {}
		constexpr timer& operator=( timer&& other ) noexcept
		{
			std::swap( base, other.base );
			return *this;
		}

		// No copy allowed.
		//
		timer( const timer& ) = delete;
		timer& operator=( const timer& ) = delete;

		// Validity check.
		//
		constexpr bool valid() const { return base != nullptr; }
		constexpr explicit operator bool() const { return valid(); }

		// Signals the timer to trigger early.
		//
		void signal() const { base->signal(); }

		// Getter/setters of the interval.
		//
		duration interval() const { return base->interval; }
		void set_interval( duration interval )
		{ 
			base->interval = interval; 
			signal();
		}

		// Cancels the timer.
		//
		void cancel()
		{
			auto* timer = release();
			timer->canceled = true;
			timer->signal();
		}

		// Releases the timer so that the destructor will not cancel it.
		//
		impl::timer_base* release() 
		{ 
			return std::exchange( base, nullptr ); 
		}

		// Destructor cancels the timer.
		//
		~timer() 
		{ 
			if ( valid() ) 
				cancel(); 
		}
	};
};