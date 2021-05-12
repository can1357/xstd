#pragma once
#include <type_traits>
#include <optional>
#include <stdint.h>
#include <array>
#include <tuple>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include <utility>
#include <memory>
#include <variant>
#include <string_view>
#include <initializer_list>
#include "intrinsics.hpp"

namespace xstd
{
	// Type tag.
	//
	template<typename T>
	struct type_tag 
	{ 
		using type = T; 

		// Returns a 64-bit hash that can be used to identify the type.
		//
		template<typename __id__ = T>
		static _CONSTEVAL size_t _hasher()
		{
			const char* sig = FUNCTION_NAME;
			size_t tmp = 0xdb88df354763d75f;
			while ( *sig )
			{
				tmp ^= *sig++;
				tmp *= 0x00000100000001B3;
			}
			return tmp;
		}
		static _CONSTEVAL size_t hash()
		{
			return std::integral_constant<size_t, _hasher()>{};
		}

		// Returns the name of the type.
		//
		template<typename __id__ = T>
		static _CONSTEVAL std::string_view to_string()
		{
			std::string_view sig = FUNCTION_NAME;
			auto [begin, delta, end] = std::tuple{
#if MS_COMPILER
				std::string_view{ "<" },                      0,  ">"
#else
				std::string_view{ "__id__" },                 +3, "];"
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
	};

	// Constant tag.
	//
	template<auto v>
	struct const_tag
	{
		using value_type = decltype( v );
		static constexpr value_type value = v;
		constexpr operator value_type() const noexcept { return value; }

		template<auto __id__ = v>
		static constexpr std::string_view to_string()
		{
			std::string_view sig = FUNCTION_NAME;
			auto [begin, delta, end] = std::tuple{
#if MS_COMPILER
				std::string_view{ "<" },                      0,  ">"
#else
				std::string_view{ "__id__" },                 +3, "];"
#endif
			};

			// Find the beginning of the name.
			//
			size_t f = sig.size();
			while( sig.substr( --f, begin.size() ).compare( begin ) != 0 )
				if( f == 0 ) return "";
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

		template<typename T, typename = void> struct is_small_vector { static constexpr bool value = false; };
		template<typename T, size_t N> struct is_small_vector<small_vector<T, N>, void> { static constexpr bool value = true; };
	};
	template <template<typename...> typename Tmp, typename T>
	static constexpr bool is_specialization_v = impl::is_specialization_v<Tmp, std::remove_cvref_t<T>>;
	template <typename T>
	static constexpr bool is_std_array_v = impl::is_std_array<std::remove_cvref_t<T>>::value;
	template <typename T>
	static constexpr bool is_small_vector_v = impl::is_small_vector<std::remove_cvref_t<T>>::value;

	// Check whether data is stored contiguously in the iterable.
	//
	template<typename T>
	static constexpr bool is_contiguous_iterable_v = 
	(
		is_std_array_v<T> ||
		is_small_vector_v<T> ||
		is_specialization_v<std::vector, T> ||
		is_specialization_v<std::basic_string, T> ||
		is_specialization_v<std::basic_string_view, T> ||
		is_specialization_v<std::initializer_list, T> ||
		std::is_array_v<T&>
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
	concept Constructable = requires( Args&&... a ) { T( std::forward<Args>( a )... ); };
	template<typename T, typename X>
	concept Assignable = requires( T r, X && v ) { r = std::forward<X>( v ); };
	template<typename T>
	concept Complete = requires( T x ) { sizeof( T ) != 0; };

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

	// Functor traits.
	//
	template<typename T, typename Ret, typename... Args>
	concept Invocable = requires( T&& x ) { Convertible<decltype( x( std::declval<Args>()... ) ), Ret>; };
	template<typename T, typename... Args>
	concept InvocableWith = requires( T&& x ) { x( std::declval<Args>()... ); };

	// Container traits.
	//
	template<typename T>
	concept Iterable = requires( T&& v ) { std::begin( v ); std::end( v ); };
	template<typename T>
	concept ReverseIterable = requires( T&& v ) { std::rbegin( v ); std::rend( v ); };

	template<Iterable T>
	using iterator_type_t = decltype( std::begin( std::declval<T>() ) );
	template<Iterable T>
	using iterator_reference_type_t = decltype( *std::declval<iterator_type_t<T>>() );
	template<Iterable T>
	using iterator_value_type_t = typename std::remove_cvref_t<iterator_reference_type_t<T>>;

	template<typename V, typename T>
	concept TypedIterable = Iterable<T> && Same<std::decay_t<iterator_value_type_t<T>>, std::decay_t<V>>;

	template<typename T>
	concept DefaultRandomAccessible = requires( const T& v ) { v[ 0 ]; std::size( v ); };
	template<typename T>
	concept CustomRandomAccessible = requires( const T& v ) { v[ 0 ]; v.size(); };
	template<typename T>
	concept RandomAccessible = DefaultRandomAccessible<T> || CustomRandomAccessible<T>;

	// Iterator traits.
	//
	template<typename T> 
	concept ForwardIterable = std::is_base_of_v<std::forward_iterator_tag, T>;
	template<typename T> 
	concept BidirectionalIterable = std::is_base_of_v<std::bidirectional_iterator_tag, T>;
	template<typename T> 
	concept RandomIterable = std::is_base_of_v<std::random_access_iterator_tag, T>;

	// String traits.
	//
	template<typename T>
	concept CppStringView = is_specialization_v<std::basic_string_view, std::decay_t<T>>;
	template<typename T>
	concept CppString = is_specialization_v<std::basic_string, std::decay_t<T>>;
	template<typename T>
	concept CString = std::is_pointer_v<std::decay_t<T>> && ( 
		std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, char> ||
        std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, wchar_t> ||
        std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, char8_t> ||
        std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, char16_t> ||
        std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, char32_t> );
	template<typename T>
	concept String = CppString<T> || CString<T> || CppStringView<T>;

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
	__forceinline static constexpr const T& make_static() noexcept { return impl::static_allocator<T, params...>::value; }

	// Default constructs the type and returns a reference to the static allocator. 
	// This useful for many cases, like:
	//  1) Function with return value of (const T&) that returns an external reference or if not applicable, a default value.
	//  2) Using non-constexpr types in constexpr structures by deferring the construction.
	//
	template<typename T> __forceinline static constexpr const T& make_default() noexcept { return make_static<T>(); }

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
	template<typename T> __forceinline static constexpr make_const_t<T> make_const( T&& x ) noexcept { return ( make_const_t<T> ) x; }

	// Converts from a const qualified ref/ptr to a non-const-qualified ref/ptr.
	// - Make sure the use is documented, this is very hacky behaviour!
	//
	template<typename T> using make_mutable_t = typename impl::make_mutable<impl::decay_ptr<T>>::type;
	template<typename T> __forceinline static constexpr make_mutable_t<T> make_mutable( T&& x ) noexcept { return ( make_mutable_t<T> ) x; }

	// Converts from any ref/ptr to a const/mutable one based on the condition given.
	//
	template<bool C, typename T> using make_const_if_t = std::conditional_t<C, make_const_t<T>, T>;
	template<bool C, typename T> using make_mutable_if_t = std::conditional_t<C, make_mutable_t<T>, T>;
	template<bool C, typename T> __forceinline static constexpr make_const_if_t<C, T> make_const_if( T&& value ) noexcept { return ( make_const_if_t<C, T> ) value; }
	template<bool C, typename T> __forceinline static constexpr make_mutable_if_t<C, T> make_mutable_if( T&& value ) noexcept { return ( make_mutable_if_t<C, T> ) value; }

	// Carries constant qualifiers of first type into second.
	//
	template<typename B, typename T> using carry_const_t = std::conditional_t<is_const_underlying_v<B>, make_const_t<T>, make_mutable_t<T>>;
	template<typename B, typename T> __forceinline static constexpr carry_const_t<B, T> carry_const( [[maybe_unused]] B&& base, T&& value ) noexcept { return ( carry_const_t<B, T> ) value; }
	
	// Creates an std::atomic version of the given pointer to value.
	//
	template<typename T> __forceinline static auto& make_atomic( T& value )
	{
		if constexpr ( std::is_const_v<T> )
			return *( const std::atomic<std::remove_cv_t<T>>* ) &value;
		else
			return *( std::atomic<std::remove_volatile_t<T>>* ) &value;
	}
	template<typename T> __forceinline static auto* make_atomic_ptr( T* value )
	{
		if constexpr ( std::is_const_v<T> )
			return ( const std::atomic<std::remove_cv_t<T>>* ) value;
		else
			return ( std::atomic<std::remove_volatile_t<T>>* ) value;
	}
	template<typename T> __forceinline static volatile T& make_volatile( T& value ) { return value; }
	template<typename T> __forceinline static volatile T* make_volatile_ptr( T* value ) { return value; }
	
	// Creates a copy of the given value.
	//
	template<typename T> __forceinline static constexpr T make_copy( const T& x ) { return x; }
	
	// Makes a null pointer to type.
	//
	template<typename T> __forceinline static constexpr T* make_null() noexcept { return ( T* ) nullptr; }
	
	// Extracts types from a parameter pack.
	//
	namespace impl
	{
		template<size_t N, typename... Tx>
		struct extract { using type = void; };
		template<size_t N, typename T, typename... Tx>
		struct extract<N, T, Tx...> { using type = typename extract<N - 1, Tx...>::type; };
		template<typename T, typename... Tx>
		struct extract<0, T, Tx...> { using type = T; };
	};
	template<size_t N, typename... Tx>
	using extract_t = typename impl::extract<N, Tx...>::type;

	template<typename... Tx> using first_of_t = extract_t<0, Tx...>;
	template<typename... Tx> using last_of_t =  extract_t<sizeof...(Tx)-1, Tx...>;

	// Replace types within parameter packs.
	//
	template<typename T, typename O> using swap_type_t = O;
	template<auto V, typename O>     using swap_to_type = O;
	template<auto V, auto O>         static constexpr auto swap_value = O;
	template<auto V, auto O>         static constexpr auto swap_to_value = O;

	// Bitcasting.
	//
	template<TriviallyCopyable To, TriviallyCopyable From> requires( sizeof( To ) == sizeof( From ) )
	__forceinline static constexpr To bit_cast( const From& src ) noexcept
	{
		if ( !std::is_constant_evaluated() )
			return *( const To* ) &src;
#if HAS_BIT_CAST
		else
			return __builtin_bit_cast( To, src );
#endif
		unreachable();
	}
	template<typename T>
	concept Bitcastable = requires( T x ) { bit_cast<std::array<char, sizeof( T )>, T >( x ); };
	template<typename T>
	__forceinline static auto& bytes( T& src ) noexcept
	{
		return carry_const( src, ( std::array<char, sizeof( T )>& ) src );
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

		template<typename T, typename = void> struct is_member_function { static constexpr bool value = false; };
		template<typename C, typename R, typename... A> struct is_member_function<member_function_t<C, R, A...>, void> { static constexpr bool value = true; };

		template<typename T, typename = void> struct is_static_function { static constexpr bool value = false; };
		template<typename R, typename... A> struct is_static_function<static_function_t<R, A...>, void> { static constexpr bool value = true; };
	};
	template<typename T> concept MemberReference = impl::is_member_reference<T>::value;
	template<typename T> concept MemberFunction = impl::is_member_function<T>::value;
	template<typename T> concept StaticFunction = impl::is_static_function<T>::value;

	// Returns the offset/size of given member reference.
	//
	template<typename V, typename C> 
	__forceinline static int64_t make_offset( member_reference_t<C, V> ref ) noexcept { return ( int64_t ) ( uint64_t ) &( make_null<C>()->*ref ); }
	template<typename V, typename C>
	__forceinline static constexpr size_t member_size( member_reference_t<C, V> ) noexcept { return sizeof( V ); }

	// Simple void pointer implementation with arithmetic and free casts, comes useful
	// when you can't infer the type of an argument pointer or if you want to const initialize
	// an architecture specific pointer.
	//
#if __clang__
	struct [[clang::trivial_abi]] any_ptr
#else
	struct any_ptr
#endif
	{
		uint64_t address;
		
		// Constructed by any kind of pointer or a number.
		//
		inline constexpr any_ptr() : address( 0 ) {}
		inline constexpr any_ptr( std::nullptr_t ) : address( 0 ) {}
		inline constexpr any_ptr( uint64_t address ) : address( address ) {}
		inline constexpr any_ptr( const void* address ) : address( bit_cast<uint64_t>( address ) ) {}
		inline constexpr any_ptr( const volatile void* address ) : address( bit_cast<uint64_t>( address ) ) {}
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
	__forceinline static auto ptr_at( any_ptr base, int64_t off ) noexcept 
	{ 
		if constexpr( std::is_void_v<T> )
			return any_ptr( base + off );
		else
			return carry_const( base, ( T* ) ( base + off ) ); 
	}
	template<typename T>
	__forceinline static auto& ref_at( any_ptr base, int64_t off ) noexcept
	{ 
		return *ptr_at<T>(base, off); 
	}

	// Byte distance between two pointers.
	//
	__forceinline static constexpr int64_t distance( any_ptr src, any_ptr dst ) noexcept { return dst - src; }

	// Wrapper around assume aligned builtin.
	//
    template<size_t N, typename T> requires ( Pointer<T> || Same<T, any_ptr> )
	__forceinline static constexpr T assume_aligned( T ptr )
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
		FLATTEN __forceinline static constexpr auto make_tuple_series( T&& f, std::integer_sequence<Ti, I...> )
		{
			if constexpr ( std::is_void_v<decltype( f( const_tag<(Ti)0>{} ) )> )
				( ( f( const_tag<I>{} ) ), ... );
			else
				return Tr{ f( const_tag<I>{} )... };
		}
		template<typename Ti, typename T, Ti... I>
		FLATTEN __forceinline static constexpr auto make_constant_series( T&& f, std::integer_sequence<Ti, I...> )
		{
			if constexpr ( std::is_void_v<decltype( f( const_tag<(Ti)0>{} ) )> )
				( ( f( const_tag<I>{} ) ), ... );
			else
				return std::array{ f( const_tag<I>{} )... };
		}
	};
	template<auto N, template<typename...> typename Tr = std::tuple, typename T>
	FLATTEN __forceinline static constexpr auto make_tuple_series( T&& f )
	{
		if constexpr ( N != 0 )
			return impl::make_tuple_series<decltype( N ), Tr>( std::forward<T>( f ), std::make_integer_sequence<decltype( N ), N>{} );
		else
			return Tr<>{};
	}
	template<auto N, typename T>
	FLATTEN __forceinline static constexpr auto make_constant_series( T&& f )
	{
		return impl::make_constant_series<decltype( N )>( std::forward<T>( f ), std::make_integer_sequence<decltype( N ), N>{} );
	}

	// Implement helper for visit on numeric range.
	//
	template<auto First, auto Last, typename T, typename K = decltype( First ), typename R = decltype( std::declval<T>()( const_tag<K(First)>{} ) )>
	FLATTEN __forceinline static constexpr auto visit_range( K key, T&& f ) -> std::conditional_t<std::is_void_v<R>, bool, std::optional<R>>
	{
		if constexpr( First <= Last )
		{
			if ( key == First )
			{
				if constexpr ( std::is_void_v<R> )
				{
					f( const_tag<First>{} );
					return true;
				}
				else
				{
					return std::optional{ f( const_tag<First>{} ) };
				}
			}
			else
			{
				return visit_range<K( size_t( First ) + 1 ), Last, T, K>( key, std::forward<T>( f ) );
			}
		}
		else
		{
			if constexpr ( std::is_void_v<R> )
				return false;
			else
				return std::nullopt;
		}
	}

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
	
	// Helper for resolving the minimum integer type that can store the given value.
	//
	namespace impl
	{
		template<auto V>
		static constexpr auto integral_compress()
		{
			if constexpr ( V < 0 )
			{
				using T = decltype( integral_compressu<-V>() );
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
	using integral_compress_t = decltype( impl::integral_compress<V>() );

	// Flattens the lambda function to a function pointer, a pointer sized argument and a manual destroyer in case it is needed.
	//
	template<typename R = void>
	using flat_function_t = R( __cdecl* )( void* arg );
	template<typename T, typename StorePtr = std::nullptr_t, typename Ret = decltype( std::declval<T&&>()( ) )> requires Invocable<T, void>
	__forceinline std::tuple<flat_function_t<Ret>, void*, flat_function_t<>> flatten( T&& fn, StorePtr store = nullptr )
	{
		using F = std::decay_t<T>;

		void* argument = nullptr;
		flat_function_t<Ret> functor;
		flat_function_t<> discard;

		// If no storage required:
		//
		if constexpr ( std::is_default_constructible_v<F> )
		{
			functor = [ ] ( void* ) -> Ret { return F{}(); };
			discard = [ ] ( void* argument ) {};
		}
		// If function fits the argument inline:
		//
		else if constexpr ( sizeof( void* ) >= sizeof( F ) )
		{
			new ( &argument ) F( std::forward<T>( fn ) );
			functor = [ ] ( void* argument ) -> Ret
			{
				F* pfn = ( F* ) &argument;
				
				if constexpr ( std::is_void_v<Ret> )
				{
					( *pfn )( );
					std::destroy_at( pfn );
				}
				else
				{
					auto&& res = ( *pfn )( );
					std::destroy_at( pfn );
					return res;
				}
			};
			discard = [ ] ( void* argument )
			{
				delete ( F* ) &argument;
			};
		}
		// If a preallocated store is given:
		//
		else if constexpr ( !std::is_same_v<StorePtr, std::nullptr_t> )
		{
			using Store = std::remove_pointer_t<StorePtr>;
			static_assert( sizeof( Store ) >= sizeof( F ) );
			
			argument = new ( store ) F( std::forward<T>( fn ) );
			functor = [ ] ( void* argument ) -> Ret
			{
				F* pfn = ( F* ) argument;

				if constexpr ( std::is_void_v<Ret> )
				{
					( *pfn )( );
					std::destroy_at( pfn );
				}
				else
				{
					auto&& res = ( *pfn )( );
					std::destroy_at( pfn );
					return res;
				}
			};
			discard = [ ] ( void* argument )
			{
				std::destroy_at( ( F* ) argument );
			};
		}
		// If otherwise, allocate in heap.
		//
		else
		{
			argument = new F( std::forward<T>( fn ) );
			functor = [ ] ( void* argument ) -> Ret
			{
				F* pfn = ( F* ) argument;

				if constexpr ( std::is_void_v<Ret> )
				{
					( *pfn )( );
					delete pfn;
				}
				else
				{
					auto&& res = ( *pfn )( );
					delete pfn;
					return res;
				}
			};
			discard = [ ] ( void* argument )
			{
				delete ( F* ) argument;
			};
		}

		return std::tuple{ functor, argument, discard };
	}

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

	// Cold call allows you to call out whilist making sure the target does not get inlined.
	//
	template<typename F, typename... Tx>
	NO_INLINE COLD static decltype( auto ) cold_call( F&& fn, Tx&&... args )
	{
		return fn( std::forward<Tx>( args )... );
	}
};

// Expose literals.
//
namespace xstd::literals
{
	constexpr inline size_t operator ""_kb( size_t n ) { return n * 1024ull; }
	constexpr inline size_t operator ""_mb( size_t n ) { return n * 1024ull * 1024ull; }
	constexpr inline size_t operator ""_gb( size_t n ) { return n * 1024ull * 1024ull * 1024ull; }
	constexpr inline size_t operator ""_tb( size_t n ) { return n * 1024ull * 1024ull * 1024ull * 1024ull; }
};
using namespace std::literals;
using namespace xstd::literals;