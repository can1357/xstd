#pragma once
#include <tuple>
#include <variant>
#include <optional>
#include <functional>
#include "type_helpers.hpp"

namespace xstd
{
	// Lightweight version of std::async::deferred that does not store any 
	// type-erased functions nor does any heap allocation.
	//
	template<typename Ret, typename Fn, typename... Tx>
	struct deferred_result
	{
		// Has the functor and its arguments.
		//
		template<typename T>
		using wrap_t = std::conditional_t<
			Reference<T>,
			std::reference_wrapper<std::remove_reference_t<T>>,
			std::remove_const_t<T>
		>;

		struct future_value
		{
			wrap_t<Fn> functor;
			std::tuple<wrap_t<Tx>...> arguments;
		};
		
		// Has the final value.
		//
		using known_value = std::decay_t<Ret>;

		// Current value.
		//
		std::optional<future_value> future;
		std::optional<known_value> current;

		// Null constructor.
		//
		deferred_result() {}
		deferred_result( std::nullopt_t ) {}

		// Construct by functor and its arguments.
		//
		deferred_result( Fn functor, wrap_t<Tx>... arguments )
			: future( future_value{ .functor = std::move( functor ), .arguments = { std::move( arguments )... } } ) {}

		// Constructor by known result.
		//
		deferred_result( known_value v ) : current( std::move( v ) ) {}

		// Returns a reference to the final value stored.
		//
		known_value& get()
		{
			if ( !current.has_value() )
			{
				// Convert pending value to known value.
				//
				current = std::apply( future->functor, future->arguments );
			}

			// Return a reference to known value.
			//
			return *current;
		}
		const known_value& get() const
		{ 
			return make_mutable( this )->get(); 
		}

		// Simple wrappers to check state.
		//
		bool is_valid() const { return future || current; }
		bool is_known() const { return current; }
		bool is_pending() const { return future; }

		// Assigns a value, discarding the pending invocation if relevant.
		//
		known_value& operator=( known_value new_value )
		{ 
			current = std::move( new_value );
			return *current;
		}

		// Syntax sugars.
		//
		operator known_value&() { return get(); }
		known_value& operator*() { return get(); }
		known_value* operator->() { return &get(); }
		operator const known_value&() const { return get(); }
		const known_value& operator*() const { return get(); }
		const known_value* operator->() const { return &get(); }
	};

	// Declare deduction guide.
	//
	template<typename Fn, typename... Tx>
	deferred_result( Fn, Tx... ) -> deferred_result<decltype(std::declval<Fn>()(std::declval<Tx>()...)), Fn, Tx...>;

	// Type erased view of deferred result.
	//
	template<typename T>
	struct deferred_value
	{
		// View only holds a pointer to the deferred_value, erasing the type.
		//
		using getter_type = T& (*) ( void* );
		void* ctx;
		getter_type getter;

		// Construction by value, store pointer in context and use type-casting lambda as getter.
		//
		deferred_value( T& value )
		{
			ctx = &value;
			getter = [ ] ( void* p ) -> T& { return *( T* ) p; };
		}
		// -- Lifetime must be guaranteed by the caller.
		deferred_value( T&& value ) : deferred_value( ( T& ) value ) {}

		// Construction by deferred result.
		//
		template<typename... Tx>
		deferred_value( const deferred_result<T, Tx...>& result )
		{
			ctx = ( void* ) &result;
			getter = [ ] ( void* self ) -> T&
			{
				return ( ( deferred_result<T, Tx...>* ) self )->get();
			};
		}

		// Construction by type + functor, type erase context and store as is.
		//
		deferred_value( const void* ctx, getter_type getter )
			: ctx( ( void* ) ctx ), getter( getter ) {}

		// Simple wrapping ::get() and cast to reference.
		//
		T& get() const { return getter( ctx ); }
		operator T& () const { return get(); }
	};

	// Declare deduction guide.
	//
	template<typename Ret, typename Fn, typename... Tx>
	deferred_value( deferred_result<Ret, Fn, Tx...> )->deferred_value<Ret>;
};