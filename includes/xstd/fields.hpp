#pragma once
#include <xstd/type_helpers.hpp>
#include <tuple>
#include <string_view>

namespace xstd
{
	// Field with manually set name.
	//
	template<auto Ref, char... Name>
	struct field;
	template<MemberReference R, R F, char... Name>
	struct field<F, Name...>
	{
		static constexpr bool is_function = false;

		static constexpr const char name[] = { Name..., '\x0' };
		static constexpr auto value = F;

		template<typename T> static constexpr auto&& get( T&& c ) { return c.*F; }
	};
	template<MemberFunction R, R F, char... Name>
	struct field<F, Name...>
	{
		static constexpr bool is_function = true;

		static constexpr const char name[] = { Name..., '\x0' };
		static constexpr auto value = F;
		
		template<typename T, typename... Tx> static constexpr auto&& invoke( T&& c, Tx&&... args ) { return c.*F( std::forward<Tx>( args )... ); }
	};

	// Automatically named field, only available when using clang.
	//
	namespace impl
	{
		template<auto Ref>
		struct field_name
		{
			static constexpr auto value = [ ]
			{
#ifdef __INTELLISENSE__
				return std::string_view{};
#else
				std::string_view name = const_tag<Ref>::to_string();
				size_t i = name.find( ':' );
				if ( i != std::string::npos )
					name.remove_prefix( i + 2 );
				return name;
#endif
			}();

			static consteval char at( size_t n ) { return n >= value.size() ? 0 : value[ n ]; }
		};

		template<size_t L, size_t P, auto Ref, char Last, char... Name>
		struct auto_field_gen
		{
			static constexpr auto dummy()
			{
				if constexpr ( sizeof...( Name ) > L || Last == '\x0' )
					return field<Ref, Name...>{};
				else
				{
					using T = typename auto_field_gen<L, P, Ref, field_name<Ref>::at( sizeof...( Name ) + 1 - P ), Name..., Last>::type;
					return T{};
				}
		
			}
			using type = decltype( dummy() );
		};
	};
	template<auto Ref, size_t L, char... Prefix>
	using basic_auto_field = typename impl::auto_field_gen<L, sizeof...( Prefix ), Ref, impl::field_name<Ref>::at( 0 ), Prefix...>::type;
	
	template<auto Ref>
	using auto_field = basic_auto_field<Ref, 32>;
	template<auto... Refs>
	using auto_field_list = std::tuple<auto_field<Refs>...>;

	// Concept for checking if the structure exports the field list.
	//
	template<typename T>
	concept FieldMappable = requires { typename T::field_list; };

	/*
	* Example of an auto field mapped type.
		struct x {
			int y;
			using field_list = xstd::auto_field_list<&x::y>;
		};
	*/
};