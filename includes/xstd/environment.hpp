#pragma once
#include <stdlib.h>
#include <string>
#include <filesystem>
#include "assert.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	// Declares two getenv wrappers fully obeying the CRT security warnings
	// on Windows environment and a f suffixed version returning a filesystem path.
	// - Path version will throw on not found if no default given since we don't want to 
	//   pollute the current directory.
	//
	static inline std::string getenv( const char* name )
	{
#ifdef _CRT_INSECURE_DEPRECATE
		size_t cnt;
		char* buffer = nullptr;
		if ( !_dupenv_s( &buffer, &cnt, name ) && buffer )
		{
			std::string res = { buffer, cnt - 1 };
			free( buffer );
			return res;
		}
#else
		if ( auto* s = ::getenv( name ) )
			return s;
#endif
		return {};
	}
	static inline std::filesystem::path getenvf( const char* name, const std::filesystem::path& def = {} )
	{
#ifdef _CRT_INSECURE_DEPRECATE
		size_t cnt;
		char* buffer = nullptr;
		if ( !_dupenv_s( &buffer, &cnt, name ) && buffer )
		{
			std::string res = { buffer, cnt - 1 };
			free( buffer );
			return res;
		}
#else
		if ( auto* s = ::getenv( name ) )
			return s;
#endif
		if ( def.empty() ) error( XSTD_ESTR( "Environment variable %s is not defined!" ), name );
		else               return def;
	}
};