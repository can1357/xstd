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
		bool const_invocable = true;

		// Null construction.
		//
		function_view() {}
		function_view( std::nullptr_t ) {}

		// Construct from any functor.
		//
		template<typename F> requires ( Invocable<F, Ret, Args...> && ( !Same<std::decay_t<F>, function_view> ) )
		function_view( F& functor )
		{
			obj = ( void* ) &functor;
			fn = [ ] ( void* obj, Args... args ) -> Ret
			{
				return ( *( F* ) obj )( std::forward<Args>( args )... );
			};
			const_invocable = Invocable<std::add_const_t<std::decay_t<F>>, Ret, Args...>;
		}
		
		// Unsafe for storage.
		template<typename F> requires ( Invocable<F, Ret, Args...> && ( !Same<std::decay_t<F>, function_view> ) )
		function_view( F&& functor ) : function_view( functor ) {}

		// Default copy/move.
		//
		function_view( function_view&& ) noexcept = default;
		function_view( const function_view& ) = default;
		function_view& operator=( function_view&& ) noexcept = default;
		function_view& operator=( const function_view& ) = default;

		// Validity check via cast to bool.
		//
		explicit operator bool() const { return obj; }

		// Redirect to functor.
		//
		Ret operator()( Args... args )
		{
			return fn( obj, std::forward<Args>( args )... );
		}
		Ret operator()( Args... args ) const
		{
			fassert( const_invocable );
			return fn( obj, std::forward<Args>( args )... );
		}
	};
};