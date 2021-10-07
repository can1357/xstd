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
		constexpr function_view( F&& functor )
		{
			using Fn =     std::decay_t<F>;
			using Traits = function_traits<Fn>;

			// Lambda?
			//
			if constexpr( Traits::is_lambda )
			{
				// Stateless lambda?
				//
				if constexpr ( Traits::is_stateless )
				{
					fn = [ ] ( void*, Args... args ) -> Ret
					{
						return Fn{}( std::move( args )... );
					};
				}
				// Stateful lambda.
				//
				else
				{
					obj = ( void* ) &functor;
					fn = [ ] ( void* obj, Args... args ) -> Ret
					{
						return ( *( Fn* ) obj )( std::move( args )... );
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
					return ( ( Fn ) obj )( std::move( args )... );
				};
			}
		}

		// Default copy/move.
		//
		constexpr function_view( function_view&& ) noexcept = default;
		constexpr function_view( const function_view& ) = default;
		constexpr function_view& operator=( function_view&& ) noexcept = default;
		constexpr function_view& operator=( const function_view& ) = default;

		// Validity check via cast to bool.
		//
		constexpr explicit operator bool() const { return fn != nullptr; }

		// Redirect to functor.
		//
		constexpr Ret operator()( Args... args ) const
		{
			return fn( obj, std::forward<Args>( args )... );
		}
	};

	// Deduction guide.
	//
	template<typename F>
	function_view( F )->function_view<typename function_traits<F>::normal_form>;
};