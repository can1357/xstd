#pragma once
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd
{
	// Declares a light-weight std::function replacement.
	// - Note: will not copy the lambda objects, lifetime is left to the user to be managed.
	//
	template<typename F>
	struct function_view;
	template<typename Ret, typename... Args>
	struct function_view<Ret( Args... )>
	{
		// Hold object pointer, invocation wrapper and whether it is const qualified or not.
		//
		void* obj = nullptr;
		Ret( *fn )( void*, Args... ) = nullptr;

		// Null construction.
		//
		constexpr function_view() {}
		constexpr function_view( std::nullptr_t ) {}

		// Construct from any functor.
		//
		template<typename F> requires ( Invocable<F, Ret, Args...> && !Same<std::decay_t<F>, function_view> )
		constexpr function_view( F& functor )
		{
			using traits = function_traits<F>;

			// Lambda?
			//
			if constexpr( traits::is_lambda )
			{
				// Stateless lambda?
				//
				if constexpr ( traits::is_stateless )
				{
					fn = [ ] ( void*, Args... args ) -> Ret
					{
						return F{}( std::move( args )... );
					};
				}
				else
				{
					obj = ( void* ) &functor;
					fn = [ ] ( void* obj, Args... args ) -> Ret
					{
						return ( *( F* ) obj )( std::move( args )... );
					};
				}
			}
			// Function pointer?
			//
			else
			{
				obj = ( void* ) functor;
				fn = [ ] ( void* obj, Args... args ) -> Ret
				{
					return ( ( F ) obj )( std::move( args )... );
				};
			}
		}
		
		// Unsafe for storage.
		template<typename F> requires ( Invocable<F, Ret, Args...> && !Same<std::decay_t<F>, function_view> )
		constexpr function_view( F&& functor ) : function_view( functor ) {}

		// Default copy/move.
		//
		constexpr function_view( function_view&& ) noexcept = default;
		constexpr function_view( const function_view& ) = default;
		constexpr function_view& operator=( function_view&& ) noexcept = default;
		constexpr function_view& operator=( const function_view& ) = default;

		// Validity check via cast to bool.
		//
		constexpr explicit operator bool() const { return obj; }

		// Redirect to functor.
		//
		constexpr Ret operator()( Args... args ) const
		{
			return fn( obj, std::forward<Args>( args )... );
		}
	};
};