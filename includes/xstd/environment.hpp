// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
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
		if ( def.empty() ) xstd::error( XSTD_ESTR( "Environment variable %s is not defined!" ), name );
		else               return def;
	}
};