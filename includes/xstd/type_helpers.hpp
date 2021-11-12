#pragma once
#include <type_traits>
#include <optional>
#include <stdint.h>
#include <array>
#include <span>
#include <tuple>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include <utility>
#include <memory>
#include <variant>
#include <bit>
#include <string_view>
#include <ranges>
#include <initializer_list>
#include "intrinsics.hpp"

// [Configuration]
// XSTD_VECTOR_EXT: Enables compiler support for vector extensions.
//
#ifndef XSTD_VECTOR_EXT
	#define XSTD_VECTOR_EXT __has_builtin(__builtin_convertvector)
#endif

namespace xstd
{
	// Type/value namers.
	//
	namespace impl
	{
		template<typename T>
		struct type_namer
		{
			template<typename __id__ = T>
			static _CONSTEVAL std::string_view __id__()
			{
				auto [sig, begin, delta, end] = std::tuple{
#if GNU_COMPILER
					std::string_view{ __PRETTY_FUNCTION__ }, std::string_view{ "__id__" }, +3, "]"
#else
					std::string_view{ __FUNCSIG__ },         std::string_view{ "__id__" }, +1, ">"
#endif
				};

				// Find the beginning of the name.
				//
				size_t f = sig.size();
				while ( sig.substr( --f, begin.size() ).compare( begin ) != 0 )
					if ( f == 0 ) return "";
				f += begin.size() + delta;

				// Find the end of the string.
				//
				auto l = sig.find_first_of( end, f );
				if ( l == std::string::npos )
					return "";

				// Return the value.
				//
				return sig.substr( f, l - f );
			}

			static constexpr auto name = [ ] ()
			{
				constexpr std::string_view view = type_namer<T>::__id__<T>();
				std::array<char, view.length() + 1> data = {};
				std::copy( view.begin(), view.end(), data.data() );
				return data;
			}( );
			inline _CONSTEVAL operator std::string_view() const { return { &name[ 0 ], &name[ name.size() - 1 ] }; }
			inline _CONSTEVAL operator const char* ( ) const { return &name[ 0 ]; }
		};
		template<auto V>
		struct value_namer
		{
			template<auto __id__ = V>
			static _CONSTEVAL std::string_view __id__()
			{
				auto [sig, begin, delta, end] = std::tuple{
#if GNU_COMPILER
					std::string_view{ __PRETTY_FUNCTION__ }, std::string_view{ "__id__" }, +3, "]"
#else
					std::string_view{ __FUNCSIG__ },         std::string_view{ "__id__" }, +0, ">"
#endif
				};

				// Find the beginning of the name.
				//
				size_t f = sig.size();
				while ( sig.substr( --f, begin.size() ).compare( begin ) != 0 )
					if ( f == 0 ) return "";
				f += begin.size() + delta;

				// Find the end of the string.
				//
				auto l = sig.find_first_of( end, f );
				if ( l == std::string::npos )
					return "";

				// Return the value.
				//
				return sig.substr( f, l - f );
			}

			static constexpr auto name = [ ] ()
			{
				constexpr std::string_view view = value_namer<V>::__id__<V>();
				std::array<char, view.length() + 1> data = {};
				std::copy( view.begin(), view.end(), data.data() );
				return data;
			}( );
			inline _CONSTEVAL operator std::string_view() const { return { &name[ 0 ], &name[ name.size() - 1 ] }; }
			inline _CONSTEVAL operator const char* ( ) const { return &name[ 0 ]; }
		};

		static _CONSTEVAL size_t ctti_hash( const char* sig )
		{
			size_t tmp = 0xdb88df354763d75f;
			while ( *sig )
			{
				tmp ^= *sig++;
				tmp *= 0x100000001B3;
			}
			return tmp;
		}
	};

	// Type tag.
	//
	template<typename T>
	struct type_tag 
	{ 
		using type = T;

		// Returns a 64-bit hash that can be used to identify the type.
		//
		template<typename t = T> static _CONSTEVAL size_t hash() { return std::integral_constant<size_t, impl::ctti_hash( FUNCTION_NAME )>{}; }

		// Returns the name of the type.
		//
		template<auto C = 0> static _CONSTEVAL std::string_view to_string() { return impl::type_namer<T>{}; }
		template<auto C = 0> static _CONSTEVAL const char* c_str() { return impl::type_namer<T>{}; }
	};

	// Constant tag.
	//
	template<auto V>
	struct const_tag
	{
		using value_type = decltype( V );
		static constexpr value_type value = V;
		constexpr operator value_type() const noexcept { return value; }

		// Returns a 64-bit hash that can be used to identify the value.
		//
		template<auto v = V> static _CONSTEVAL size_t hash() { return std::integral_constant<size_t, impl::ctti_hash( FUNCTION_NAME )>{}; }

		// Returns the name of the value.
		//
		template<auto C = 0> static _CONSTEVAL std::string_view to_string() { return impl::value_namer<V>{}; }
		template<auto C = 0> static _CONSTEVAL const char* c_str() { return impl::value_namer<V>{}; }
	};

	// String literal.
	//
	template <size_t N>
	struct string_literal
	{
		char value[ N ]{};
		constexpr string_literal( const char( &str )[ N ] ) { std::copy_n( str, N, value ); }

		// Observers.
		//
		inline constexpr const char* c_str() const { return &value[ 0 ]; }
		inline constexpr const char* data() const { return c_str(); }
		inline constexpr std::string_view get() const { return { c_str(), c_str() + size() }; }
		inline constexpr size_t size() const { return N - 1; }
		inline constexpr size_t length() const { return size(); }
		inline constexpr bool empty() const { return size() == 0; }
		inline constexpr const char& operator[]( size_t n ) const { return c_str()[ n ]; }
		inline constexpr auto begin() const { return get().begin(); }
		inline constexpr auto end() const { return get().end(); }
		inline std::string to_string() const { return c_str(); }
	};
	template <size_t N>
	struct wstring_literal
	{
		wchar_t value[ N ]{};
		constexpr wstring_literal( const wchar_t( &str )[ N ] ) { std::copy_n( str, N, value ); }

		// Observers.
		//
		inline constexpr const wchar_t* c_str() const { return &value[ 0 ]; }
		inline constexpr const wchar_t* data() const { return c_str(); }
		inline constexpr std::wstring_view get() const { return { c_str(), c_str() + size() }; }
		inline constexpr size_t size() const { return N - 1; }
		inline constexpr size_t length() const { return size(); }
		inline constexpr bool empty() const { return size() == 0; }
		inline constexpr const wchar_t& operator[]( size_t n ) const { return c_str()[ n ]; }
		inline constexpr auto begin() const { return get().begin(); }
		inline constexpr auto end() const { return get().end(); }
		inline std::wstring to_string() const { return c_str(); }
	};

	// Check for specialization.
	//
	template<typename T, size_t N>
	struct small_vector;
	namespace impl
	{
		template <template<typename...> typename Tmp, typename>
		static constexpr bool is_specialization_v = false;
		template <template<typename...> typename Tmp, typename... Tx>
		static constexpr bool is_specialization_v<Tmp, Tmp<Tx...>> = true;

		template<typename T, typename = void> struct is_std_array { static constexpr bool value = false; };
		template<typename T, size_t N> struct is_std_array<std::array<T, N>, void> { static constexpr bool value = true; };

		template<typename T, typename = void> struct is_std_span { static constexpr bool value = false; };
		template<typename T, size_t N> struct is_std_span<std::span<T, N>, void> { static constexpr bool value = true; };

		template<typename T, typename = void> struct is_small_vector { static constexpr bool value = false; };
		template<typename T, size_t N> struct is_small_vector<small_vector<T, N>, void> { static constexpr bool value = true; };
	};
	template <template<typename...> typename Tmp, typename T>
	static constexpr bool is_specialization_v = impl::is_specialization_v<Tmp, std::remove_cvref_t<T>>;
	template <typename T>
	static constexpr bool is_std_array_v = impl::is_std_array<std::remove_cvref_t<T>>::value;
	template <typename T>
	static constexpr bool is_std_span_v = impl::is_std_span<std::remove_cvref_t<T>>::value;
	template <typename T>
	static constexpr bool is_small_vector_v = impl::is_small_vector<std::remove_cvref_t<T>>::value;

	// Check whether data is stored contiguously in the iterable.
	//
	template<typename T>
	static constexpr bool is_contiguous_iterable_v = 
	(
		is_std_array_v<T> ||
		is_std_span_v<T> ||
		is_small_vector_v<T> ||
		is_specialization_v<std::vector, T> ||
		is_specialization_v<std::basic_string, T> ||
		is_specialization_v<std::basic_string_view, T> ||
		is_specialization_v<std::initializer_list, T> ||
		std::is_array_v<std::remove_cvref_t<T>> ||
		std::is_array_v<T>
	);

	// Checks if the given lambda can be evaluated in compile time.
	//
	template<typename F, std::enable_if_t<(F{}(), true), int> = 0>
	static constexpr bool is_constexpr( F )   { return true; }
	static constexpr bool is_constexpr( ... ) { return false; }

	// Common concepts.
	//
	template<typename T>
	concept Integral = std::is_integral_v<T>;
	template<typename T>
	concept Arithmetic = std::is_arithmetic_v<T>;
	template<typename T>
	concept FloatingPoint = std::is_floating_point_v<T>;
	template<typename T>
	concept Void = std::is_void_v<T>;
	template<typename T>
	concept NonVoid = !std::is_void_v<T>;
	template<typename T>
	concept Trivial = std::is_trivial_v<T>;
	template<typename T>
	concept Enum = std::is_enum_v<T>;
	template<typename T>
	concept Tuple = is_specialization_v<std::tuple, T> || is_specialization_v<std::pair, T>;
	template<typename T>
	concept Span = is_std_span_v<T>;
	template<typename T>
	concept Pointer = std::is_pointer_v<T>;
	template<typename T>
	concept LvReference = std::is_lvalue_reference_v<T>;
	template<typename T>
	concept RvReference = std::is_rvalue_reference_v<T>;
	template<typename T>
	concept Reference = std::is_reference_v<T>;
	template<typename T>
	concept Const = std::is_const_v<T>;
	template<typename T>
	concept Mutable = !std::is_const_v<T>;
	template<typename T>
	concept Volatile = std::is_volatile_v<T>;
	template<typename T>
	concept Optional = is_specialization_v<std::optional, T>;
	template<typename T>
	concept Final = std::is_final_v<T>;
	template<typename T>
	concept Empty = std::is_empty_v<T>;
	template<typename T>
	concept Variant = is_specialization_v<std::variant, T>;
	template<typename T>
	concept Dereferencable = Pointer<std::decay_t<T>> || requires( T&& x ) { x.operator*(); };
	template<typename T>
	concept MemberPointable = Pointer<std::decay_t<T>> || requires( T&& x ) { x.operator->(); };
	template<typename T>
	concept PointerLike = MemberPointable<T> && Dereferencable<T>;

	template<typename T>
	concept TriviallyCopyable = std::is_trivially_copyable_v<T>;
	template<typename T>
	concept TriviallyMoveAssignable = std::is_trivially_move_assignable_v<T>;
	template<typename T>
	concept TriviallyMoveConstructable = std::is_trivially_move_constructible_v<T>;
	template<typename T>
	concept TriviallyCopyAssignable = std::is_trivially_copy_assignable_v<T>;
	template<typename T>
	concept TriviallyCopyConstructable = std::is_trivially_copy_constructible_v<T>;
	template<typename T>
	concept TriviallySwappable = TriviallyMoveConstructable<T> && TriviallyMoveConstructable<T>;
	template<typename From, typename To>
	concept TriviallyAssignable = std::is_trivially_assignable_v<From, To>;
	template<typename T>
	concept TriviallyConstructable = std::is_trivially_constructible_v<T>;
	template<typename T>
	concept TriviallyDefaultConstructable = std::is_trivially_default_constructible_v<T>;
	template<typename T>
	concept TriviallyDestructable = std::is_trivially_destructible_v<T>;
	
	template<typename T>
	concept MoveAssignable = std::is_move_assignable_v<T>;
	template<typename T>
	concept MoveConstructable = std::is_move_constructible_v<T>;
	template<typename T>
	concept CopyAssignable = std::is_copy_assignable_v<T>;
	template<typename T>
	concept CopyConstructable = std::is_copy_constructible_v<T>;
	template<typename T>
	concept Swappable = MoveConstructable<T> && MoveConstructable<T>;
	template<typename T>
	concept DefaultConstructable = std::is_default_constructible_v<T>;
	template<typename T>
	concept Destructable = std::is_destructible_v<T>;

	template<typename A, typename B>
	concept Same = std::is_same_v<A, B>;
	template <typename From, typename To>
	concept Convertible = std::is_convertible_v<From, To>;
	template <template<typename...> typename Tmp, typename T>
	concept Specialization = is_specialization_v<Tmp, T>;
	template<typename T, typename... Args>
	concept Constructable = requires( Args&&... a ) { T{ std::forward<Args>( a )... }; };
	template<typename From, typename To>
	concept Assignable = std::is_assignable_v<From, To>;
	template<typename T>
	concept Complete = requires( T x ) { sizeof( T ) != 0; };
	template <typename From, typename To>
	concept Castable = requires( From && x ) { ( To ) x; };

	// Comparison traits.
	//
	template<typename T, typename O> concept ThreeWayComparable = requires( T&& a, O&& b ) { a <=> b; };
	template<typename T, typename O> concept LessEqualComparable = requires( T&& a, O&& b ) { a <= b; };
	template<typename T, typename O> concept LessComparable = requires( T&& a, O&& b ) { a <  b; };
	template<typename T, typename O> concept EqualComparable = requires( T&& a, O&& b ) { a == b; };
	template<typename T, typename O> concept NotEqualComparable = requires( T&& a, O&& b ) { a != b; };
	template<typename T, typename O> concept GreatComparable = requires( T&& a, O&& b ) { a >  b; };
	template<typename T, typename O> concept GreatEqualComparable = requires( T&& a, O&& b ) { a >= b; };

	// Artihmetic traits.
	//
	template<typename T> concept PreIncrementable = requires( T&& x ) { ++x; };
	template<typename T> concept PostIncrementable = requires( T&& x ) { x++; };
	template<typename T> concept PreDecrementable = requires( T&& x ) { --x; };
	template<typename T> concept PostDecrementable = requires( T&& x ) { x--; };
	template<typename T, typename O> concept Subtractable = requires( T&& x, O&& y ) { x - y; };
	template<typename T, typename O> concept Addable = requires( T&& x, O&& y ) { x + y; };
	template<typename T, typename O> concept Multipliable = requires( T&& x, O&& y ) { x * y; };
	template<typename T, typename O> concept Divisible = requires( T && x, O && y ) { x / y; };
	template<typename T, typename O> concept Modable = requires( T&& x, O&& y ) { x % y; };
	template<typename T, typename O> concept LeftShiftable = requires( T&& x, O&& y ) { x << y; };
	template<typename T, typename O> concept RightShiftable = requires( T&& x, O&& y ) { x >> y; };
	template<typename T, typename O> concept Andable = requires( T&& x, O&& y ) { x & y; };
	template<typename T, typename O> concept Orable = requires( T&& x, O&& y ) { x | y; };
	template<typename T, typename O> concept Xorable = requires( T&& x, O&& y ) { x ^ y; };
	template<typename T> concept Signed =   std::is_signed_v<T>;
	template<typename T> concept Unsigned = std::is_unsigned_v<T>;

	template<Integral T1, Integral T2> requires ( Signed<T1> == Signed<T2> )
	using integral_max_t = std::conditional_t<( sizeof( T1 ) > sizeof( T2 ) ), T1, T2>;
	template<FloatingPoint T1, FloatingPoint T2>
	using floating_max_t = std::conditional_t<( sizeof( T1 ) > sizeof( T2 ) ), T1, T2>;
	template<Integral T1, Integral T2> requires ( Signed<T1> == Signed<T2> )
	using integral_min_t = std::conditional_t<( sizeof( T1 ) < sizeof( T2 ) ), T1, T2>;
	template<FloatingPoint T1, FloatingPoint T2>
	using floating_min_t = std::conditional_t<( sizeof( T1 ) < sizeof( T2 ) ), T1, T2>;
	
	// Checks if the type is a member function or not.
	//
	template<typename T>                                 struct is_member_function { static constexpr bool value = false; };
	template<typename C, typename R, typename... Tx>     struct is_member_function<R(C::*)(Tx...)>             { static constexpr bool value = true; };
	template<typename C, typename R, typename... Tx>     struct is_member_function<R(C::*)(Tx..., ...)>        { static constexpr bool value = true; };
	template<typename C, typename R, typename... Tx>     struct is_member_function<R(C::*)(Tx...) const>       { static constexpr bool value = true; };
	template<typename C, typename R, typename... Tx>     struct is_member_function<R(C::*)(Tx..., ...)  const> { static constexpr bool value = true; };
	template<typename T>
	static constexpr bool is_member_function_v = is_member_function<T>::value;

	// Function traits.
	//
	template<typename F>
	struct function_traits
	{
		static constexpr bool is_valid = false;
		static constexpr bool is_vararg = false;
		static constexpr bool is_lambda = false;
	};

	// Function pointers:
	//
	template<typename R, typename... Tx>
	struct function_traits<R(*)(Tx...)>
	{
		static constexpr bool is_valid = true;
		static constexpr bool is_vararg = false;
		static constexpr bool is_lambda = false;

		using return_type = R;
		using arguments =   std::tuple<Tx...>;
		using owner =       void;
		using normal_form = R( Tx... );
	};
	template<typename R, typename... Tx>
	struct function_traits<R(*)(Tx..., ...)> : function_traits<R(*)(Tx...)>
	{
		static constexpr bool is_vararg = true;
	};

	// Member functions:
	//
	template<typename C, typename R, typename... Tx>
	struct function_traits<R(C::*)(Tx...)>
	{
		static constexpr bool is_valid = true;
		static constexpr bool is_vararg = false;
		static constexpr bool is_lambda = false;

		using return_type = R;
		using arguments =   std::tuple<Tx...>;
		using owner =       C;
		using normal_form = R( Tx... );
	};
	template<typename C, typename R, typename... Tx>
	struct function_traits<R(C::*)(Tx..., ...)> : function_traits<R(C::*)(Tx...)>
	{
		static constexpr bool is_vararg = true;
	};
	template<typename C, typename R, typename... Tx>
	struct function_traits<R(C::*)(Tx...) const>
	{
		static constexpr bool is_valid = true;
		static constexpr bool is_vararg = false;
		static constexpr bool is_lambda = false;

		using return_type = R;
		using arguments =   std::tuple<Tx...>;
		using owner =       const C;
		using normal_form = R( Tx... );
	};
	template<typename C, typename R, typename... Tx>
	struct function_traits<R(C::*)(Tx..., ...) const> : function_traits<R(C::*)(Tx...) const>
	{
		static constexpr bool is_vararg = true;
	};

	// Lambdas or callables.
	//
	template<typename F> concept CallableObject = requires{ is_member_function_v<decltype(&F::operator())>; };
	template<CallableObject F> struct function_traits<F> : function_traits<decltype( &F::operator() )> 
	{ 
		static constexpr bool is_lambda = true;
	};
	template<typename F> concept Function =        function_traits<F>::is_valid;
	template<typename F> concept Lambda =          function_traits<F>::is_lambda;
	template<typename F> concept StatelessLambda = Empty<F> && Lambda<F>;

	// Callable traits.
	//
	template<typename T, typename... Args>
	concept InvocableWith = requires( T && x ) { x( std::declval<Args>()... ); };
	namespace impl
	{
		template<typename T, typename Ret, typename... Args>
		struct invoke_traits
		{
			static constexpr bool success = false;
		};
		template<typename T, typename Ret, typename... Args> requires InvocableWith<T, Args...>
		struct invoke_traits<T, Ret, Args...>
		{
			using R = decltype( std::declval<T&&>()( std::declval<Args&&>()... ) );
			static constexpr bool success = Void<Ret> ? Void<R> : Convertible<R, Ret>;
		};
	};
	template<typename T, typename Ret, typename... Args>
	concept Invocable = impl::invoke_traits<T, Ret, Args...>::success;

	// Iterator traits.
	//
	template<typename T>
	concept Iterator = requires( T && v ) { std::next( v ); *v; v == v; v != v; };
	template<typename T>
	using iterator_ref_t = decltype( *std::declval<T>() );
	template<typename T>
	using iterator_val_t = typename std::decay_t<iterator_ref_t<T>>;

	template<typename V, typename T>
	concept TypedIterator = Iterator<T> && Same<iterator_val_t<T>, std::decay_t<V>>;
	template<typename V, typename T>
	concept ConvertibleIterator = Iterator<T> && Convertible<iterator_ref_t<T>, std::decay_t<V>>;

	template<typename T> 
	concept ForwardIterable = std::is_base_of_v<std::forward_iterator_tag, T>;
	template<typename T> 
	concept BidirectionalIterable = std::is_base_of_v<std::bidirectional_iterator_tag, T>;
	template<typename T> 
	concept RandomIterable = std::is_base_of_v<std::random_access_iterator_tag, T>;

	// Container traits.
	//
	template<typename T>
	concept Iterable = requires( T&& v ) { std::begin( v ); std::end( v ); };
	template<typename T>
	concept ContiguousIterable = is_contiguous_iterable_v<T>;
	template<typename T>
	using iterator_t = decltype( std::begin( std::declval<T&>() ) );
	template<typename T>
	using const_iterator_t = decltype( std::begin( std::declval<const T&>() ) );
	template<typename T>
	using iterable_ref_t = iterator_ref_t<iterator_t<T>>;
	template<typename T>
	using iterable_const_ref_t = iterator_ref_t<const_iterator_t<T>>;
	template<typename T>
	using iterable_val_t = iterator_val_t<iterator_t<T>>;

	template<typename T>
	concept ReverseIterable = requires( T&& v ) { std::rbegin( v ); std::rend( v ); };
	template<typename V, typename T>
	concept TypedIterable = Iterable<T> && Same<std::decay_t<iterable_val_t<T>>, std::decay_t<V>>;
	template<typename V, typename T>
	concept IterableAs = Iterator<T> && Convertible<iterable_val_t<T>, std::decay_t<V>>;
	template<typename T>
	concept DefaultRandomAccessible = requires( const T& v ) { v[ 0 ]; std::size( v ); };
	template<typename T>
	concept CustomRandomAccessible = requires( const T& v ) { v[ 0 ]; v.size(); };
	template<typename T>
	concept RandomAccessible = DefaultRandomAccessible<T> || CustomRandomAccessible<T>;

	
	// Character traits.
	//
	template<typename T> concept Char =   Same<T, char> || Same<T, wchar_t> || Same<T, char8_t> || Same<T, char16_t> || Same<T, char32_t>;
	template<typename T> concept Char8 =  Char<T> && sizeof( T ) == 1;
	template<typename T> concept Char16 = Char<T> && sizeof( T ) == 2;
	template<typename T> concept Char32 = Char<T> && sizeof( T ) == 4;
	
	// String traits.
	//
	template<typename T> concept CppString =     is_specialization_v<std::basic_string, std::decay_t<T>>;
	template<typename T> concept CppStringView = is_specialization_v<std::basic_string_view, std::decay_t<T>>;
	template<typename T> concept CString =       Pointer<std::decay_t<T>> && Char<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>;
	template<typename T> concept String =        CppString<T> || CString<T> || CppStringView<T>;

	template<String T>
	using string_unit_t = typename std::remove_cvref_t<decltype( std::declval<T>()[ 0 ] )>;
	template<String T>
	using string_view_t = typename std::basic_string_view<string_unit_t<T>>;
	template<String T>
	using cppstring_t =   typename std::basic_string<string_unit_t<T>>;

	// Atomicity-related traits.
	//
	template<typename T>
	concept Lockable = requires( T& x ) { x.lock(); x.unlock(); };
	template<typename T>
	concept SharedLockable = requires( T& x ) { x.lock_shared(); x.unlock_shared(); };
	template<typename T>
	concept TryLockable = requires( T & x ) { x.try_lock(); };
	template<typename T>
	concept LockCheckable = requires( T & x ) { ( bool ) x.locked(); };
	template<typename T>
	concept SharedTryLockable = requires( T& x ) { x.try_lock_shared(); };
	template<typename T>
	concept TimeLockable = requires( T & x, std::chrono::milliseconds y ) { x.try_lock_for( y ); x.try_lock_until( y ); };
	template<typename T>
	concept SharedTimeLockable = requires( T & x, std::chrono::milliseconds y ) { x.try_lock_shared_for( y ); x.try_lock_shared_until( y ); };
	template<typename T>
	concept Atomic = is_specialization_v<std::atomic, T>;

	// Chrono traits.
	//
	template<typename T>
	concept Duration = is_specialization_v<std::chrono::duration, T>;
	template<typename T>
	concept Timestamp = is_specialization_v<std::chrono::time_point, T>;
	 
	// XSTD traits, Tiable types have a mutable functor called tie which returns a list of members tied.
	// => std::tuple<&...> T::tie()
	//
	template<typename T> concept Tiable = requires( T& x ) { x.tie(); };

	// Constructs a static constant given the type and parameters, returns a reference to it.
	//
	namespace impl
	{
		template<typename T, auto... params>
		struct static_allocator { inline static const T value = { params... }; };
	};
	template<typename T, auto... params>
	FORCE_INLINE inline constexpr const T& make_static() noexcept { return impl::static_allocator<T, params...>::value; }

	// Default constructs the type and returns a reference to the static allocator. 
	// This useful for many cases, like:
	//  1) Function with return value of (const T&) that returns an external reference or if not applicable, a default value.
	//  2) Using non-constexpr types in constexpr structures by deferring the construction.
	//
	template<typename T> FORCE_INLINE inline constexpr const T& make_default() noexcept { return make_static<T>(); }

	// Special type that decays to a constant reference to the default constructed value of the type.
	//
	struct static_default
	{
		template<typename T> requires ( !Reference<T> )
		constexpr operator const T&() const noexcept { return make_default<T>(); }
	};

	// Implementation of type-helpers for the functions below.
	//
	namespace impl
	{
		template<typename T>
		using decay_ptr = typename std::conditional_t<std::is_pointer_v<std::remove_cvref_t<T>>, std::remove_cvref_t<T>, T>;

		template<typename T, typename = void> struct make_const {};
		template<typename T> struct make_const<T&, void>    { using type = std::add_const_t<T>&;    };
		template<typename T> struct make_const<T*, void>    { using type = std::add_const_t<T>*;    };

		template<typename T, typename = void> struct make_mutable {};
		template<typename T> struct make_mutable<T&, void>  { using type = std::remove_const_t<T>&;  };
		template<typename T> struct make_mutable<T*, void>  { using type = std::remove_const_t<T>*;  };

		template<typename T, typename = void> struct is_const_underlying : std::false_type {};
		template<typename T> struct is_const_underlying<const T&, void>  : std::true_type  {};
		template<typename T> struct is_const_underlying<const T*, void>  : std::true_type  {};
	};

	// Checks the const qualifiers of the underlying object.
	//
	template<typename T> static constexpr bool is_const_underlying_v = impl::is_const_underlying<impl::decay_ptr<T>>::value;
	template<typename T> static constexpr bool is_mutable_underlying_v = !impl::is_const_underlying<impl::decay_ptr<T>>::value;

	// Converts from a non-const qualified ref/ptr to a const-qualified ref/ptr.
	//
	template<typename T> using make_const_t = typename impl::make_const<impl::decay_ptr<T>>::type;
	template<typename T> FORCE_INLINE inline constexpr make_const_t<T> make_const( T&& x ) noexcept { return ( make_const_t<T> ) x; }

	// Converts from a const qualified ref/ptr to a non-const-qualified ref/ptr.
	// - Make sure the use is documented, this is very hacky behaviour!
	//
	template<typename T> using make_mutable_t = typename impl::make_mutable<impl::decay_ptr<T>>::type;
	template<typename T> FORCE_INLINE inline constexpr make_mutable_t<T> make_mutable( T&& x ) noexcept { return ( make_mutable_t<T> ) x; }

	// Converts from any ref/ptr to a const/mutable one based on the condition given.
	//
	template<bool C, typename T> using make_const_if_t = std::conditional_t<C, make_const_t<T>, T>;
	template<bool C, typename T> using make_mutable_if_t = std::conditional_t<C, make_mutable_t<T>, T>;
	template<bool C, typename T> FORCE_INLINE inline constexpr make_const_if_t<C, T> make_const_if( T&& value ) noexcept { return ( make_const_if_t<C, T> ) value; }
	template<bool C, typename T> FORCE_INLINE inline constexpr make_mutable_if_t<C, T> make_mutable_if( T&& value ) noexcept { return ( make_mutable_if_t<C, T> ) value; }

	// Carries constant qualifiers of first type into second.
	//
	template<typename B, typename T> using carry_const_t = std::conditional_t<is_const_underlying_v<B>, make_const_t<T>, make_mutable_t<T>>;
	template<typename B, typename T> FORCE_INLINE inline constexpr carry_const_t<B, T> carry_const( [[maybe_unused]] B&& base, T&& value ) noexcept { return ( carry_const_t<B, T> ) value; }
	
	// Creates an std::atomic version of the given pointer to value.
	//
	template<typename T> FORCE_INLINE inline auto& make_atomic( T& value )
	{
		if constexpr ( std::is_const_v<T> )
			return *( const std::atomic<std::remove_cv_t<T>>* ) &value;
		else
			return *( std::atomic<std::remove_volatile_t<T>>* ) &value;
	}
	template<typename T> FORCE_INLINE inline auto* make_atomic_ptr( T* value )
	{
		if constexpr ( std::is_const_v<T> )
			return ( const std::atomic<std::remove_cv_t<T>>* ) value;
		else
			return ( std::atomic<std::remove_volatile_t<T>>* ) value;
	}
	template<typename T> FORCE_INLINE inline volatile T& make_volatile( T& value ) { return value; }
	template<typename T> FORCE_INLINE inline volatile T* make_volatile_ptr( T* value ) { return value; }
	
	// Creates a copy of the given value.
	//
	template<typename T> FORCE_INLINE inline constexpr T make_copy( const T& x ) { return x; }
	
	// Makes a null pointer to type.
	//
	template<typename T> FORCE_INLINE inline constexpr T* make_null() noexcept { return ( T* ) nullptr; }

	// Nontemporal memory helpers.
	//
	template<typename T>
	FORCE_INLINE inline T load_nontemporal( const void* p )
	{
#if GNU_COMPILER
		return __builtin_nontemporal_load( ( const T* ) p );
#else
		return *( const T* ) p;
#endif
	}
	template<typename T>
	FORCE_INLINE inline void store_nontemporal( void* p, T value )
	{
#if GNU_COMPILER
		__builtin_nontemporal_store( value, ( T* ) p );
#else
		*( T* ) p = value;
#endif
	}

	// Misaligned memory helpers.
	//
	template<typename T>
	FORCE_INLINE inline T load_misaligned( const void* p )
	{
#if GNU_COMPILER
		struct wrapper { T value; } __attribute__((packed));
		return ( ( const wrapper* ) p )->value;
#else
		using wrapper = std::array<char, sizeof( T )>;
		return bit_cast< T >( *( const wrapper* ) p );
#endif
	}

	template<typename T>
	FORCE_INLINE inline void store_misaligned( void* p, T r )
	{
#if GNU_COMPILER
		struct wrapper { T value; } __attribute__((packed));
		( ( wrapper* ) p )->value = r;
#else
		using wrapper = std::array<char, sizeof( T )>;
		*( wrapper* ) p = bit_cast< wrapper >( r );
#endif
	}

	// Extracts types from a parameter pack.
	//
	namespace impl
	{
		template<size_t N, typename... Tx>              struct textract              { using type = void; };
		template<size_t N, typename T, typename... Tx>  struct textract<N, T, Tx...> { using type = typename textract<N - 1, Tx...>::type; };
		template<typename T, typename... Tx>            struct textract<0, T, Tx...> { using type = T; };
		
		template<size_t N, auto... Vx>                  struct vextract              {};
		template<size_t N, auto V, auto... Vx>          struct vextract<N, V, Vx...> { static constexpr auto value = vextract<N - 1, Vx...>::value; };
		template<auto V, auto... Vx>                    struct vextract<0, V, Vx...> { static constexpr auto value = V; };
	};
	template<size_t N, typename... Tx> using extract_t =  typename impl::textract<N, Tx...>::type;
	template<typename... Tx>           using first_of_t = extract_t<0, Tx...>;
	template<typename... Tx>           using last_of_t =  extract_t<sizeof...(Tx)-1, Tx...>;
	template<size_t N, auto... Vx>     static constexpr auto extract_v =  impl::vextract<N, Vx...>::value;
	template<auto... Vx>               static constexpr auto first_of_v = extract_v<0, Vx...>;
	template<auto... Vx>               static constexpr auto last_of_v =  extract_v<sizeof...(Vx)-1, Vx...>;

	// Replace types within parameter packs.
	//
	template<typename T, typename O> using swap_type_t = O;
	template<auto V, typename O>     using swap_to_type = O;
	template<auto V, auto O>         static constexpr auto swap_value = O;
	template<typename T, auto O>     static constexpr auto swap_to_value = O;

	// Converts any type to their trivial equivalents.
	//
	template<size_t N>
	struct trivial_converter;
	template<>
	struct trivial_converter<8>
	{
		using integral_signed =   int64_t;
		using integral_unsigned = uint64_t;
		using floating_point =    double;
		using character =         void;
	};
	template<>
	struct trivial_converter<4>
	{
		using integral_signed =   int32_t;
		using integral_unsigned = uint32_t;
		using floating_point =    float;
		using character =         char32_t;
	};
	template<>
	struct trivial_converter<2>
	{
		using integral_signed =   int16_t;
		using integral_unsigned = uint16_t;
		using floating_point =    void;
		using character =         wchar_t;
	};
	template<>
	struct trivial_converter<1>
	{
		using integral_signed =   int8_t;
		using integral_unsigned = uint8_t;
		using floating_point =    void;
		using character =         char;
	};

	template<typename T> using convert_int_t =  typename trivial_converter<sizeof( T )>::integral_signed;
	template<typename T> using convert_uint_t = typename trivial_converter<sizeof( T )>::integral_unsigned;
	template<typename T> using convert_fp_t =   typename trivial_converter<sizeof( T )>::floating_point;
	template<typename T> using convert_char_t = typename trivial_converter<sizeof( T )>::character;
	
	// Declare the vector types.
	//
#if XSTD_VECTOR_EXT && defined(__clang__)
	template<typename T, size_t N>
	using native_vector = T  __attribute__((ext_vector_type(N), __aligned__(1)));
#elif XSTD_VECTOR_EXT
	template <typename T, size_t N>
	using native_vector [[gnu::aligned(1), gnu::vector_size(sizeof(T[N]))]] = T;
#else
	template <typename T, size_t N>
	using native_vector = std::array<T, std::bit_ceil(N)>;
#endif

	// Bitcasting.
	//
	template<TriviallyCopyable To, TriviallyCopyable From> requires( sizeof( To ) == sizeof( From ) )
	FORCE_INLINE inline constexpr To bit_cast( const From& src ) noexcept
	{
		if ( !std::is_constant_evaluated() )
			return *( const To* ) &src;
#if HAS_BIT_CAST && !defined(__INTELLISENSE__)
		else
			return __builtin_bit_cast( To, src );
#else
		return std::bit_cast< To >( src );
#endif
		unreachable();
	}
	template<typename T>
	concept Bitcastable = requires( T x ) { bit_cast<std::array<uint8_t, sizeof( T )>, T >( x ); };

	template<typename T> requires ( !Reference<T> && !Const<T> )
	FORCE_INLINE inline auto& as_bytes( T& src ) noexcept
	{
		return ( std::array<uint8_t, sizeof( T )>& ) src;
	}
	template<typename T> requires ( !Reference<T> && !Const<T> )
	FORCE_INLINE inline auto& as_bytes( const T& src ) noexcept
	{
		return ( const std::array<uint8_t, sizeof( T )>& ) src;
	}

	// Constant-lenght accelerated routines.
	//
	namespace impl
	{
		template<size_t N>
		FORCE_INLINE inline auto mismatch_vector( const uint8_t* a, const uint8_t* b )
		{
			if constexpr ( N == 0 )
			{
				return 0;
			}
			else if constexpr ( N == 8 || N == 4 || N == 2 || N == 1 )
			{
				using T = typename trivial_converter<N>::integral_unsigned;
				return *( const T* ) a ^ *( const T* ) b;
			}
			else if constexpr ( N == 16 || N == 32 )
			{
				using vec_t = native_vector<uint64_t, N / 8>;
				auto c =   load_misaligned<vec_t>( a );
				auto c2 =  load_misaligned<vec_t>( b );
				
				if constexpr ( Xorable<vec_t, vec_t> )
				{
					c ^= c2;
				}
				else
				{
					for ( size_t n = 0; n != ( N / 8 ); n++ )
						c[ n ] ^= c2[ n ];
				}
				return c;
			}
			else if constexpr ( std::bit_floor( N ) != N )
			{
				constexpr size_t U = std::bit_floor( N );
				auto c =  mismatch_vector<U>( a, b );
				auto c2 = mismatch_vector<U>( a + ( N - U ), b + ( N - U ) );

				if constexpr ( Orable<decltype( c ), decltype( c )> )
				{
					c |= c2;
				}
				else
				{
					for ( size_t n = 0; n != ( sizeof( c ) / sizeof( c[ 0 ] ) ); n++ )
						c[ n ] |= c2[ n ];
				}
				return c;
			}
			else
			{
				auto c =  mismatch_vector<N / 2>( a, b );
				auto c2 = mismatch_vector<N / 2>( a + ( N / 2 ), b + ( N / 2 ) );
				if constexpr ( Orable<decltype( c ), decltype( c )> )
				{
					c |= c2;
				}
				else
				{
					for ( size_t n = 0; n != ( sizeof( c ) / sizeof( c[ 0 ] ) ); n++ )
						c[ n ] |= c2[ n ];
				}
				return c;
			}
		}
	};
	template<auto N>
	FORCE_INLINE inline void trivial_copy_n( void* __restrict a, const void* __restrict b ) noexcept
	{
		if constexpr ( N > 0 )
		{
#if __has_builtin(__builtin_memcpy_inline)
			__builtin_memcpy_inline( a, b, N );
#else
			uint8_t* __restrict va = ( uint8_t* ) a;
			const uint8_t* __restrict vb = ( const uint8_t* ) b;
			for ( size_t n = 0; n != size_t( N ); n++ )
				va[ n ] = vb[ n ];
#endif
		}
	}
	template<typename T>
	FORCE_INLINE inline void trivial_copy( T& __restrict a, const T& __restrict b ) noexcept
	{
		trivial_copy_n<sizeof( T )>( &a, &b );
	}
	template<typename T>
	FORCE_INLINE inline void trivial_swap( T& __restrict a, T& __restrict b ) noexcept
	{
		auto& __restrict va = as_bytes( a );
		auto& __restrict vb = as_bytes( b );

		if constexpr ( sizeof( T ) <= 0x200 )
		{
			auto tmp = va;
			va = vb;
			vb = tmp;
		}
		else
		{
			std::swap( va, vb );
		}
	}
	template<typename T>
	FORCE_INLINE inline bool trivial_equals( const T& a, const T& b ) noexcept
	{
		auto result = impl::mismatch_vector<sizeof( T )>( ( const uint8_t* ) &a, ( const uint8_t* ) &b );
		if constexpr ( sizeof( result ) <= 8 )
		{
			return result == 0;
		}
		else
		{
			uint64_t acc = 0;
			for ( size_t n = 0; n != ( sizeof( result ) / sizeof( result[ 0 ] ) ); n++ )
				acc |= result[ n ];
			return acc == 0;
		}
	}

	template<Unsigned T, auto N>
	FORCE_INLINE inline constexpr T trivial_read_n( const uint8_t* src )
	{
		if constexpr ( N <= 0 )
		{
			return T( 0 );
		}
		else
		{
			constexpr size_t L = std::min<size_t>( ( size_t ) N, sizeof( T ) );

			T result = 0;
			if ( std::is_constant_evaluated() )
			{
				for ( size_t i = 0; i != L; i++ )
					result |= T( src[ i ] ) << ( 8 * i );
			}
			else
			{
				if constexpr ( L == sizeof( T ) )
				{
					result = *( const T* ) src;
				}
				else if constexpr ( L >= 4 )
				{
					result = T( *( const uint32_t* ) src );
					if constexpr ( L == 7 )
						result |= T( *( const uint32_t* ) &src[ 3 ] ) << 24;
					else if constexpr ( L == 6 )
						result |= T( *( const uint16_t* ) &src[ 4 ] ) << 32;
					else if constexpr ( L == 5 )
						result |= T( src[ 4 ] ) << 32;
				}
				else if constexpr ( L >= 2 )
				{
					result = T( *( const uint16_t* ) src );
					if constexpr ( L == 3 )
						result |= T( src[ 2 ] ) << 16;
				}
				else if constexpr ( L == 1 )
				{
					result = T( *( const uint16_t* ) src );
				}
			}
			return result;
		}
	}

	// Member reference helper.
	//
	template<typename C, typename M>
	using member_reference_t = M C::*;

	// Function pointer helpers.
	//
	template<typename R, typename... A>
	using static_function_t = R(*)(A...);
	template<typename C, typename R, typename... A>
	using member_function_t = R(C::*)(A...);

	// Concept versions of the traits above.
	//
	namespace impl
	{
		template<typename T, typename = void> struct is_member_reference { static constexpr bool value = false; };
		template<typename C, typename M> struct is_member_reference<member_reference_t<C, M>, void> { static constexpr bool value = true; };

		template<typename T, typename = void> struct is_static_function { static constexpr bool value = false; };
		template<typename R, typename... A> struct is_static_function<static_function_t<R, A...>, void> { static constexpr bool value = true; };
	};
	template<typename T> concept MemberReference = impl::is_member_reference<T>::value;
	template<typename T> concept MemberFunction = is_member_function_v<T>;
	template<typename T> concept StaticFunction = impl::is_static_function<T>::value;

	// Returns the offset/size of given member reference.
	//
	template<typename V, typename C> 
	FORCE_INLINE inline int64_t make_offset( member_reference_t<C, V> ref ) noexcept { return ( int64_t ) ( uint64_t ) &( make_null<C>()->*ref ); }
	template<typename V, typename C>
	FORCE_INLINE inline constexpr size_t member_size( member_reference_t<C, V> ) noexcept { return sizeof( V ); }

	// Simple void pointer implementation with arithmetic and free casts, comes useful
	// when you can't infer the type of an argument pointer or if you want to const initialize
	// an architecture specific pointer.
	//
	struct TRIVIAL_ABI any_ptr
	{
		uint64_t address;
		
		// Constructed by any kind of pointer or a number.
		//
		inline constexpr any_ptr() : address( 0 ) {}
		inline constexpr any_ptr( std::nullptr_t ) : address( 0 ) {}
		inline constexpr any_ptr( uint64_t address ) : address( address ) {}
		template<typename T>
		inline constexpr any_ptr( T* address ) : address( bit_cast<uint64_t>( address ) ) {}
		template<typename R, typename... A>
		inline constexpr any_ptr( static_function_t<R, A...> fn ) : address( bit_cast<uint64_t>( fn ) ) {}
		template<typename C, typename R, typename... A>
		inline constexpr any_ptr( member_function_t<C, R, A...> fn ) : address( bit_cast<uint64_t>( fn ) ) {}

		// Default copy and move.
		//
		constexpr any_ptr( any_ptr&& ) noexcept = default;
		constexpr any_ptr( const any_ptr& ) = default;
		constexpr any_ptr& operator=( any_ptr&& ) noexcept = default;
		constexpr any_ptr& operator=( const any_ptr& ) = default;

		// Can decay to any pointer or an integer.
		//
		template<typename T>
		inline constexpr operator T*() const { return bit_cast<T*>( address ); }
		template<typename R, typename... A>
		inline constexpr operator static_function_t<R, A...>() const { return bit_cast<static_function_t<R, A...>>( address ); }
		template<typename C, typename R, typename... A>
		inline constexpr operator member_function_t<C, R, A...>() const { return bit_cast<member_function_t<C, R, A...>>( address ); }
		inline constexpr operator uint64_t() const { return address; }
		
		inline constexpr any_ptr& operator++() { address++; return *this; }
		inline constexpr any_ptr operator++( int ) { auto s = *this; operator++(); return s; }
		inline constexpr any_ptr& operator--() { address--; return *this; }
		inline constexpr any_ptr operator--( int ) { auto s = *this; operator--(); return s; }

		template<Integral T> inline constexpr any_ptr operator+( T d ) const { return address + d; }
		template<Integral T> inline constexpr any_ptr operator-( T d ) const { return address - d; }
		template<Integral T> inline constexpr any_ptr& operator+=( T d ) { address += d; return *this; }
		template<Integral T> inline constexpr any_ptr& operator-=( T d ) { address -= d; return *this; }
	};

	// Gets the type at the given offset.
	//
	template<typename T = void>
	FORCE_INLINE inline auto ptr_at( any_ptr base, ptrdiff_t off = 0 ) noexcept
	{ 
		if constexpr( std::is_void_v<T> )
			return any_ptr( base + off );
		else
			return carry_const( base, ( T* ) ( base + off ) ); 
	}
	template<typename T>
	FORCE_INLINE inline auto& ref_at( any_ptr base, ptrdiff_t off = 0 ) noexcept
	{ 
		return *ptr_at<T>(base, off); 
	}

	// Byte distance between two pointers.
	//
	FORCE_INLINE inline constexpr int64_t distance( any_ptr src, any_ptr dst ) noexcept { return dst - src; }

	// Wrapper around assume aligned builtin.
	//
	template<size_t N, typename T> requires ( Pointer<T> || Same<T, any_ptr> )
	FORCE_INLINE inline constexpr T assume_aligned( T ptr )
	{
#if __has_builtin(__builtin_assume_aligned)
		if ( !std::is_constant_evaluated() )
			return T( __builtin_assume_aligned( ( const void* ) ptr, N ) );
#endif
		return ptr;
	}

	// Implement helpers for basic series creation.
	//
	namespace impl
	{
		template<typename Ti, template<typename...> typename Tr, typename T, Ti... I>
		FLATTEN FORCE_INLINE inline constexpr auto make_tuple_series( T&& f, [[maybe_unused]] std::integer_sequence<Ti, I...> seq )
		{
			if constexpr ( std::is_void_v<decltype( f( const_tag<( Ti ) 0>{} ) ) > )
			{
				auto eval = [ & ] <Ti X> ( const_tag<X> tag, auto&& self )
				{
					f( tag );
					if constexpr ( ( X + 1 ) != ( sizeof...( I ) ) )
						self( const_tag<X + 1>{}, self );
				};
				eval( const_tag<Ti( 0 )>{}, eval );
			}
			else
			{
				using Tp = Tr<decltype( f( const_tag<I>{} ) )...>;
#if MS_COMPILER
				// Needs to be default initializable on MSVC, fuck this.
				Tp arr = {};
				auto assign = [ & ] <Ti X> ( const_tag<X> tag, auto&& self )
				{
					std::get<X>( arr ) = f( tag );
					if constexpr ( ( X + 1 ) != ( sizeof...( I ) ) )
						self( const_tag<X + 1>{}, self );
				};
				assign( const_tag<Ti( 0 )>{}, assign );
				return arr;
#else
				return Tp{ f( const_tag<I>{} )... };
#endif
			}
		}
		template<typename Ti, typename T, Ti... I>
		FLATTEN FORCE_INLINE inline constexpr auto make_constant_series( T&& f, [[maybe_unused]] std::integer_sequence<Ti, I...> seq )
		{
			using R = decltype( f( const_tag<( Ti ) 0>{} ) );

			if constexpr ( std::is_void_v<R> )
			{
				auto eval = [ & ] <Ti X> ( const_tag<X> tag, auto && self )
				{
					f( tag );
					if constexpr ( ( X + 1 ) != ( sizeof...( I ) ) )
						self( const_tag<X + 1>{}, self );
				};
				eval( const_tag<Ti( 0 )>{}, eval );
			}
			else
			{
#if MS_COMPILER
				// Needs to be default initializable on MSVC, fuck this.
				std::array<R, sizeof...( I )> arr = {};
				auto assign = [ & ] <Ti X> ( const_tag<X> tag, auto&& self )
				{
					std::get<X>( arr ) = f( tag );
					if constexpr ( ( X + 1 ) != ( sizeof...( I ) ) )
						self( const_tag<X + 1>{}, self );
				};
				assign( const_tag<Ti( 0 )>{}, assign );
				return arr;
#else
				return std::array<R, sizeof...( I )>{ f( const_tag<I>{} )... };
#endif
			}
		}
	};
	template<auto N, template<typename...> typename Tr = std::tuple, typename T>
	FLATTEN FORCE_INLINE inline constexpr auto make_tuple_series( T&& f )
	{
		if constexpr ( N != 0 )
			return impl::make_tuple_series<decltype( N ), Tr>( std::forward<T>( f ), std::make_integer_sequence<decltype( N ), N>{} );
		else
			return Tr<>{};
	}
	template<auto N, typename T>
	FLATTEN FORCE_INLINE inline constexpr auto make_constant_series( T&& f )
	{
		return impl::make_constant_series<decltype( N )>( std::forward<T>( f ), std::make_integer_sequence<decltype( N ), N>{} );
	}

	// Visit strategies:
	//
	namespace impl
	{
#define __visit_8(i, x)		\
		case (i+0): x(i+0);	\
		case (i+1): x(i+1);	\
		case (i+2): x(i+2);	\
		case (i+3): x(i+3);	\
		case (i+4): x(i+4);	\
		case (i+5): x(i+5);	\
		case (i+6): x(i+6);	\
		case (i+7): x(i+7);	
#define __visit_64(i, x)        \
		__visit_8(i+(8*0), x)     \
		__visit_8(i+(8*1), x)     \
		__visit_8(i+(8*2), x)     \
		__visit_8(i+(8*3), x)     \
		__visit_8(i+(8*4), x)     \
		__visit_8(i+(8*5), x)     \
		__visit_8(i+(8*6), x)     \
		__visit_8(i+(8*7), x)
#define __visit_512(i, x)        \
		__visit_64(i+(64*0), x)    \
		__visit_64(i+(64*1), x)    \
		__visit_64(i+(64*2), x)    \
		__visit_64(i+(64*3), x)    \
		__visit_64(i+(64*4), x)    \
		__visit_64(i+(64*5), x)    \
		__visit_64(i+(64*6), x)    \
		__visit_64(i+(64*7), x)
#define __visitor(i) 						       \
		if constexpr ( ( i ) < Lim ) {	       \
			return fn( const_tag<size_t(i)>{} ); \
		}											       \
		unreachable();							       \

		template<size_t Lim, typename F> FORCE_INLINE inline constexpr decltype( auto ) numeric_visit_8( size_t n, F&& fn ) { switch ( n ) { __visit_8( 0, __visitor ); default: unreachable(); } }
		template<size_t Lim, typename F> FORCE_INLINE inline constexpr decltype( auto ) numeric_visit_64( size_t n, F&& fn ) { switch ( n ) { __visit_64( 0, __visitor ); default: unreachable(); } }
		template<size_t Lim, typename F> FORCE_INLINE inline constexpr decltype( auto ) numeric_visit_512( size_t n, F&& fn ) { switch ( n ) { __visit_512( 0, __visitor ); default: unreachable(); } }
#undef __visitor
#undef __visit_512
#undef __visit_64
#undef __visit_8
	};

	// Strict numeric visit.
	//
	template<size_t Count, typename F>
	FORCE_INLINE inline constexpr decltype( auto ) visit_index( size_t n, F&& fn )
	{
		if constexpr ( Count <= 8 )
			return impl::numeric_visit_8<Count>( n, std::forward<F>( fn ) );
		else if constexpr ( Count <= 64 )
			return impl::numeric_visit_64<Count>( n, std::forward<F>( fn ) );
		else if constexpr ( Count <= 512 )
			return impl::numeric_visit_512<Count>( n, std::forward<F>( fn ) );
		else
		{
			// Binary search.
			//
			constexpr size_t Midline = Count / 2;
			if ( n >= Midline )
				return visit_index<Count - Midline>( n - Midline, [ & ] <size_t N> ( const_tag<N> ) { fn( const_tag<N + Midline>{} ); } );
			else
				return visit_index<Midline>( n, std::forward<F>( fn ) );
		}
	}

	// Implement helper for visit on numeric range.
	//
	template<auto First, auto Last, typename T, typename K = decltype( First ), typename R = decltype( std::declval<T>()( const_tag<K(First)>{} ) )>
	FLATTEN FORCE_INLINE inline constexpr auto visit_range( K key, T&& f ) -> std::conditional_t<Void<R>, bool, std::optional<R>>
	{
		if ( First <= key && key <= Last )
		{
			return visit_index<size_t( Last ) - size_t( First ) + 1>( size_t( key ) - size_t( First ), [ & ] <size_t N> ( const_tag<N> )
			{
				using Tag = const_tag<K( N + size_t( First ) )>;
				if constexpr ( std::is_void_v<R> )
				{
					f( Tag{} );
					return true;
				}
				else
				{
					return std::optional{ f( Tag{} ) };
				}
			} );
		}
		else
		{
			if constexpr ( std::is_void_v<R> )
				return false;
			else
				return std::nullopt;
		}
	}

	// Implement helper for visiting/transforming every element of a tuple-like.
	//
	template<typename T, typename F>
	FLATTEN FORCE_INLINE inline constexpr decltype(auto) visit_tuple( F&& f, T&& tuple )
	{
		return make_tuple_series<std::tuple_size_v<std::decay_t<T>>>( [ & ] <auto I> ( const_tag<I> ) -> decltype( auto )
		{
			using Tv = decltype( std::get<I>( tuple ) );
			if constexpr( InvocableWith<F&&, Tv, size_t> )
				return f( std::get<I>( tuple ), I );
			else
				return f( std::get<I>( tuple ) );
		} );
	}
	
	// Helper for resolving the minimum integer type that can store the given value.
	//
	namespace impl
	{
		template<auto V>
		static constexpr auto integral_shrink()
		{
			if constexpr ( V < 0 )
			{
				using T = decltype( integral_shrink<-V>() );
				return ( std::make_signed_t<T> ) V;
			}
			else if constexpr ( V <= UINT8_MAX )
				return ( uint8_t ) V;
			else if constexpr ( V <= UINT16_MAX )
				return ( uint16_t ) V;
			else if constexpr ( V <= UINT32_MAX )
				return ( uint32_t ) V;
			else
				return ( size_t ) V;
		}
	};
	template<auto V>
	using integral_shrink_t = decltype( impl::integral_shrink<V>() );

	// Initializer list with rvalue iterator.
	//
	template<typename T>
	struct rvalue_wrap
	{
		mutable T value;

		template<typename... Tx> requires Constructable<T, Tx...>
		constexpr rvalue_wrap( Tx&&... args ) noexcept : value{ std::forward<Tx>( args )... } {}

		// Copy by move.
		//
		constexpr rvalue_wrap( const rvalue_wrap& o ) noexcept : value( o.get() ) {}
		constexpr rvalue_wrap& operator=( const rvalue_wrap& o ) noexcept { value = o.get(); return *this; }
		
		// Default move.
		//
		constexpr rvalue_wrap( rvalue_wrap&& ) noexcept = default;
		constexpr rvalue_wrap& operator=( rvalue_wrap&& ) noexcept = default;

		// Getter.
		//
		constexpr T&& get() const noexcept { return std::move( value ); }
		constexpr operator T&&() const noexcept { return std::move( value ); }
	};
	template<typename T> rvalue_wrap( T )->rvalue_wrap<T>;

	template<typename T>
	using rvalue_initializer_list = std::initializer_list<rvalue_wrap<T>>;

	// Tiable comperators.
	//
	struct tie_equal_to
	{
		template<typename T>
		bool operator()( const T& a, const T& b ) const noexcept
		{
			return make_mutable( a ).tie() == make_mutable( b ).tie();
		}
	};
	struct tie_less_than
	{
		template<typename T>
		bool operator()( const T& a, const T& b ) const noexcept
		{
			return make_mutable( a ).tie() < make_mutable( b ).tie();
		}
	};

	// A null type that takes no space, ignores assignments and always decays to the default value.
	//
	template<typename T>
	struct null_store
	{
		template<typename... Tx>
		constexpr null_store( [[maybe_unused]] Tx&&... args ) {}
		template<typename Ty>
		constexpr null_store& operator=( [[maybe_unused]] Ty&& arg ) { return *this; }
		constexpr operator const T&() const noexcept { return make_default<T>(); }
	};
	template<typename T, bool C>
	using conditional_store = std::conditional_t<C, T, null_store<T>>;

	// Emits an unrolled loop for a constant length.
	//
	template<size_t N, typename F>
	FORCE_INLINE static void unroll( F&& fn )
	{
		__hint_unroll()
		for ( size_t i = 0; i != N; i++ )
			fn();
	}

	// Emits an unrolled loop (upto N times) for a dynamic length.
	// -- Exhaust variant takes a reference to N and will not handle leftovers.
	// -- Scaled version iterates in the given unit size, returns the leftover length.
	//
	template<size_t N, typename F>
	FORCE_INLINE static bool unroll_exhaust_n( F&& fn, size_t& n )
	{
		for ( size_t i = n / N; i != 0; i-- )
			unroll<N>( fn );
		n %= N;
		return !n;
	}
	template<auto... N, typename F>
	FORCE_INLINE static void unroll_n( F&& fn, size_t n )
	{
		( unroll_exhaust_n<N>( fn, n ) || ... );
		for ( ; n; n-- )
			fn();
	}
	template<size_t S, auto... N, typename F>
	FORCE_INLINE static size_t unroll_scaled_n( F&& fn, size_t n )
	{
		unroll_n<(N/S)...>( fn, n / S );
		return n % S;
	}

	// Identity function that hints to the compiler to be maximally pessimistic about the operation.
	//
	template<typename T>
	FORCE_INLINE static T black_box( T value )
	{
#if GNU_COMPILER
		if constexpr ( Integral<T> || Pointer<T> || Enum<T> || FloatingPoint<T> )
			asm volatile( "" : "+r" ( value ) );
		else
			asm volatile( "" : "+m" ( value ) );
		return value;
#else
		volatile T* dummy = &value;
		return *dummy;
#endif
	}

	// Cold call allows you to call out whilist making sure the target does not get inlined.
	//
	template<typename F, typename... Tx>
	NO_INLINE COLD static decltype( auto ) cold_call( F&& fn, Tx&&... args )
	{
		return fn( std::forward<Tx>( args )... );
	}

	// Runs the given lambda only if it's the first time.
	//
	template<typename F> requires ( !Pointer<F> )
	FORCE_INLINE inline decltype( auto ) run_once( F&& fn )
	{
		// Traits of the result.
		//
		using R =  decltype( fn() );
		using Rx = std::conditional_t<Void<R>, std::monostate, R>;

		// Result and the state store.
		//
		static std::atomic<uint8_t> status = 1; // 1 = first time, 2 = in-progress, 0 = complete
		alignas( Rx ) static uint8_t result[ sizeof( Rx ) ] = {};
		
		if ( status.load( std::memory_order::relaxed ) != 0 ) [[unlikely]]
		{
			cold_call( [ & ]
			{
				uint8_t expected = 1;
				if ( status.compare_exchange_strong( expected, 2, std::memory_order::acquire ) )
				{
					if constexpr ( !Void<R> )
						std::construct_at<R>( ( R* ) &result[ 0 ], fn() );
					else
						fn();
					std::atomic_thread_fence( std::memory_order::acquire );
					status.store( 0, std::memory_order::relaxed );
				}
				else if ( expected != 0 )
				{
					do
						yield_cpu();
					while ( status.load( std::memory_order::relaxed ) != 0 );
				}
			} );
		}
		if constexpr( !Void<R> )
			return *( const R* ) &result[ 0 ];
	}

	// Creates a deleter from a given allocator type.
	//
	namespace impl
	{
		template<typename A>
		struct allocator_delete : A
		{
			using type = allocator_delete<A>;
			using A::A;
			template<typename Tv>
			void operator()( Tv* p ) const noexcept { A::deallocate( p ); }
		};
		template<Empty A>
		struct allocator_delete<A>
		{
			using type = allocator_delete<A>;
			template<typename Tv>
			void operator()( Tv* p ) const noexcept { A{}.deallocate( p ); }
		};
		template<typename T>
		struct allocator_delete<std::allocator<T>>
		{
			using type = std::default_delete<T>;
			template<typename Tv>
			void operator()( Tv* p ) const noexcept { std::default_delete<T>{}( p ); }
		};
	};
	template<typename T>
	using allocator_delete = typename impl::allocator_delete<T>::type;
};

// Expose literals.
//
namespace xstd::literals
{
	constexpr inline size_t operator ""_kb( unsigned long long n ) { return ( size_t ) ( n * 1024ull ); }
	constexpr inline size_t operator ""_mb( unsigned long long n ) { return ( size_t ) ( n * 1024ull * 1024ull ); }
	constexpr inline size_t operator ""_gb( unsigned long long n ) { return ( size_t ) ( n * 1024ull * 1024ull * 1024ull ); }
	constexpr inline size_t operator ""_tb( unsigned long long n ) { return ( size_t ) ( n * 1024ull * 1024ull * 1024ull * 1024ull ); }
};
using namespace std::literals;
using namespace xstd::literals;