#pragma once
#include <atomic>
#include "type_helpers.hpp"

// Implements a wrapper around std::atomic/?mutex that makes it copyable. This is deleted for valid
// reasons in the base class, however when used as a class member, it forces the object to
// implement a custom constructor which in most cases not needed since object can be reasonably
// assumed to be not copied in the first place while it is still being operated.
//
namespace xstd
{
	template<typename T>
	struct relaxed_atomic : std::atomic<T>
	{
		// Inerit assignment and construction.
		//
		using base_type = std::atomic<T>;
		using base_type::base_type;
		using base_type::operator=;

		// Allow copy construction and assignment.
		//
		relaxed_atomic( const relaxed_atomic& o ) : base_type( o.load() ) {}
		relaxed_atomic& operator=( const relaxed_atomic& o ) { base_type::operator=( o.load() ); return *this; }
	};
	template<typename T>
	struct relaxed_mutex : T
	{
		// Inerit construction.
		//
		using base_type = T;
		using base_type::base_type;

		// Allow copy/move construction and assignment, safety is left to the owner.
		//
		relaxed_mutex( relaxed_mutex&& o ) noexcept {}
		relaxed_mutex( const relaxed_mutex& o ) {}
		relaxed_mutex& operator=( relaxed_mutex&& o ) noexcept { return *this; }
		relaxed_mutex& operator=( const relaxed_mutex& o ) { return *this; }
	};
	namespace impl
	{
		template<typename T>
		struct relaxed_type;
		template<Atomic T>   struct relaxed_type<T> { using type = relaxed_atomic<typename T::value_type>; };
		template<Lockable T> struct relaxed_type<T> { using type = relaxed_mutex<T>; };
	};
	template<typename T>
	using relaxed = typename impl::relaxed_type<T>::type;
};