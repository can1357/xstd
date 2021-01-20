#pragma once
#include <utility>
#include "type_helpers.hpp"

namespace xstd
{
	// Implements a simple RAII style scope-deferred task.
	//
	template<typename T> requires Invocable<T, void>
	struct finally
	{
		T functor;
		bool set = false;
		
		constexpr finally( T&& fn ) : functor( std::forward<T>( fn ) ), set( true ) {}
		constexpr finally( finally&& o ) noexcept : functor( o.functor ), set( std::exchange( o.set, false ) ) {}
		constexpr finally( const finally& ) = delete;
		constexpr void cancel() { set = false; }
		constexpr void apply() { if ( set ) functor(); set = false; }
		constexpr ~finally() { apply(); }
	};

	// Commonly used guard for the counter increment/decrement.
	//
	template<typename T> requires ( PostIncrementable<T> && PostDecrementable<T> )
	struct counter_guard
	{
		T& ref;
		counter_guard( T& ref ) : ref( ref ) { ++ref; }
		counter_guard( const counter_guard& ) = delete;
		~counter_guard() { --ref; }
	};
};