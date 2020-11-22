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
#include <iostream>
#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <mutex>
#include <thread>
#include <functional>
#include <cstdarg>
#include "formatting.hpp"
#include "time.hpp"
#include "intrinsics.hpp"

// [Configuration]
// XSTD_CON_THREAD_LOCAL: If set, will use a mutex to protect the writes into console.
// XSTD_CON_ENFORCE_UTF8_WINDOWS: If set, will enforce the command-line to output UTF8 on Windows platform.
// XSTD_CON_NO_COLORS: If set, disables colors.
// XSTD_CON_NO_WARNINGS: If set, disables warnings.
// XSTD_CON_NO_LOGS: If set, disables logs.
// XSTD_CON_ERROR_REDIRECT: If set, redirects errors to the set name. [Prototype: extern "C" void __cdecl [[noreturn]] ( const std::string& )]
// XSTD_CON_ERROR_NOMSG: If set, changes the redirect prototype to extern "C" void __cdecl [[noreturn]] ();

#ifndef XSTD_CON_THREAD_LOCAL
	#define XSTD_CON_THREAD_LOCAL 1
#endif
#ifndef XSTD_CON_ENFORCE_UTF8_WINDOWS
	#define XSTD_CON_ENFORCE_UTF8_WINDOWS 1
#endif
#ifndef XSTD_CON_NO_COLORS
	#define XSTD_CON_NO_COLORS 0
#endif
#ifndef XSTD_CON_NO_WARNINGS
	#define XSTD_CON_NO_WARNINGS 0
#endif
#ifndef XSTD_CON_NO_LOGS
	#define XSTD_CON_NO_LOGS 0
#endif
#ifndef XSTD_CON_ERROR_NOMSG
	#define XSTD_CON_ERROR_NOMSG 0
#endif
#ifdef XSTD_CON_ERROR_REDIRECT
	#if XSTD_CON_ERROR_NOMSG
		extern "C" void __cdecl XSTD_CON_ERROR_REDIRECT [[noreturn]] ();
	#else
		extern "C" void __cdecl XSTD_CON_ERROR_REDIRECT [[noreturn]] ( const std::string& );
	#endif
#endif

#if ( WINDOWS_TARGET && XSTD_CON_ENFORCE_UTF8_WINDOWS )
extern "C" 
{
	__declspec( dllimport ) int __stdcall SetConsoleOutputCP( unsigned int code_page_id );
	__declspec( dllimport ) void* __stdcall GetStdHandle( unsigned long std_handle );
	__declspec( dllimport ) int __stdcall GetConsoleMode( void* handle, unsigned long* mode );
	__declspec( dllimport ) int __stdcall SetConsoleMode( void* handle, unsigned long mode );
};
#endif

// Console colors.
//
enum console_color
{
	CON_BRG = 15,
	CON_YLW = 14,
	CON_PRP = 13,
	CON_RED = 12,
	CON_CYN = 11,
	CON_GRN = 10,
	CON_BLU = 9,
	CON_DEF = 7,
};

namespace xstd
{
	// Padding customization for logger.
	//
	static constexpr char log_padding_c = '|';
	static constexpr int log_padding_step = 2;

	// Describes the state of the logging engine.
	//
	struct logger_state_t
	{
		// Whether prints are muted or not.
		//
		bool mute = false;

		// Current padding level.
		//
		int padding = -1;

		// Padding leftover from previous print.
		//
		int padding_carry = 0;

		// Constructor initializes logger.
		//
		logger_state_t()
		{
			constexpr uint32_t STD_OUTPUT_HANDLE = ( uint32_t ) -11;

#if ( WINDOWS_TARGET && XSTD_CON_ENFORCE_UTF8_WINDOWS )
			constexpr uint32_t CP_UTF8 = 65001;
			SetConsoleOutputCP( CP_UTF8 );
#endif
#if ( WINDOWS_TARGET && !XSTD_CON_NO_COLORS )
			constexpr uint32_t ENABLE_VIRTUAL_TERMINAL_PROCESSING = 4;
			unsigned long mode;
			GetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), &mode );
			SetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
#endif
		}

#if XSTD_CON_THREAD_LOCAL
		// Lock of the stream.
		//
		std::recursive_mutex mtx;

		void lock() { mtx.lock(); }
		void unlock() { mtx.unlock(); }
		bool try_lock() { return mtx.try_lock(); }
		bool try_lock( timeunit_t max_wait )
		{
			bool locked = false;
			auto t0 = time::now();
			while ( !( locked = try_lock() ) )
				if ( ( time::now() - t0 ) > max_wait )
					break;
			return locked;
		}
#else
		void lock() {}
		void unlock() {}
		bool try_lock() { return true; }
		bool try_lock( timeunit_t max_wait ) { return true; }
#endif
	};
	inline logger_state_t logger_state = {};

	// RAII hack for incrementing the padding until routine ends.
	// Can be used with the argument u=0 to act as a lock guard.
	// - Will wait for the critical section ownership and hold it
	//   until the scope ends.
	//
	struct scope_padding
	{
		int active;
		int prev;

		scope_padding( unsigned u ) : active( 1 )
		{
			logger_state.lock();
			prev = logger_state.padding;
			logger_state.padding += u;
		}

		void end()
		{
			if ( active-- <= 0 ) return;
			logger_state.padding = prev;
			logger_state.unlock();
		}
		~scope_padding() { end(); }
	};

	// RAII hack for changing verbosity of logs within the scope.
	// - Will wait for the critical section ownership and hold it
	//   until the scope ends.
	//
	struct scope_verbosity
	{
		int active;
		bool prev;

		scope_verbosity( bool verbose_output ) : active( 1 )
		{
			logger_state.lock();
			prev = logger_state.mute;
			logger_state.mute |= !verbose_output;
		}

		void end()
		{
			if ( active-- <= 0 ) return;
			logger_state.mute = prev;
			logger_state.unlock();
		}
		~scope_verbosity() { end(); }
	};

	// Main function used when logging.
	//
	namespace impl
	{
		static constexpr const char* translate_color( console_color color )
		{
#if !XSTD_CON_NO_COLORS
			switch ( color )
			{
				case CON_BRG: return XSTD_CSTR( ANSI_ESCAPE( "1;37m" ) );
				case CON_YLW: return XSTD_CSTR( ANSI_ESCAPE( "1;33m" ) );
				case CON_PRP: return XSTD_CSTR( ANSI_ESCAPE( "1;35m" ) );
				case CON_RED: return XSTD_CSTR( ANSI_ESCAPE( "1;31m" ) );
				case CON_CYN: return XSTD_CSTR( ANSI_ESCAPE( "1;36m" ) );
				case CON_GRN: return XSTD_CSTR( ANSI_ESCAPE( "1;32m" ) );
				case CON_BLU: return XSTD_CSTR( ANSI_ESCAPE( "1;34m" ) );
				case CON_DEF:
				default:      return XSTD_CSTR( ANSI_ESCAPE( "0m" ) );
			}
#else
			return "";
#endif
		}

		template<bool has_args>
		static int log_w( FILE* dst, console_color color, const char* fmt_str, ... )
		{
			// Hold the lock for the critical section guarding ::log.
			//
			std::lock_guard g( logger_state );

			// Do not execute if logs are disabled.
			//
			if ( logger_state.mute ) return 0;

			// If we should pad this output:
			//
			int out_cnt = 0;
			if ( logger_state.padding > 0 )
			{
				// If it was not carried from previous:
				//
				if ( int pad_by = logger_state.padding - logger_state.padding_carry )
				{
					for ( int i = 0; i < pad_by; i++ )
					{
						if ( ( i + 1 ) == pad_by )
						{
							out_cnt += fprintf( dst, XSTD_CSTR( "%*c" ), log_padding_step - 1, ' ' );
							if ( fmt_str[ 0 ] == ' ' ) putchar( log_padding_c );
						}
						else
						{
							out_cnt += fprintf( dst, XSTD_CSTR( "%*c%c" ), log_padding_step - 1, ' ', log_padding_c );
						}
					}
				}

				// Set or clear the carry for next.
				//
				if ( fmt_str[ strlen( fmt_str ) - 1 ] == '\n' )
					logger_state.padding_carry = 0;
				else
					logger_state.padding_carry = logger_state.padding;
			}

			// Set to requested color and redirect to printf.
			//
			if ( color != CON_DEF )
				fputs( translate_color( color ), dst );

			// If string literal with no parameters, use puts instead.
			//
			if ( has_args )
			{
				va_list args;
				va_start( args, fmt_str );
				out_cnt += vfprintf( dst, fmt_str, args );
				va_end( args );
			}
			else
			{
				out_cnt += fputs( fmt_str, dst );
			}

			// Reset to defualt color.
			//
			if ( color != CON_DEF )
				fputs( translate_color( CON_DEF ), dst );
			return out_cnt;
		}
	};

	template<typename... Tx>
	static int log( console_color color, const char* fmt_str, Tx&&... ps )
	{
#if !XSTD_CON_NO_LOGS
		auto buf = fmt::create_string_buffer_for<Tx...>();
		return impl::log_w<sizeof...( Tx ) != 0>( stdout, color, fmt_str, fmt::fix_parameter<Tx>( buf, std::forward<Tx>( ps ) )... );
#else
		return 0;
#endif
	}
	template<console_color color = CON_DEF, typename... Tx>
	static int log( const char* fmt_str, Tx&&... ps )
	{
#if !XSTD_CON_NO_LOGS
		auto buf = fmt::create_string_buffer_for<Tx...>();
		return impl::log_w<sizeof...( Tx ) != 0>( stdout, color, fmt_str, fmt::fix_parameter<Tx>( buf, std::forward<Tx>( ps ) )... );
#else
		return 0;
#endif
	}

	// Prints a warning message.
	//
	template<typename... params>
	static void warning( const char* fmt_str, params&&... ps )
	{
#if !XSTD_CON_NO_WARNINGS
		// Format warning message.
		//
		std::string message = XSTD_STR( "\n" ) + impl::translate_color( CON_YLW ) + XSTD_STR( "[!] Warning: " ) + fmt::str(
			fmt_str,
			std::forward<params>( ps )...
		) + '\n';

		// Try acquiring the lock and print the warning, if properly locked skiped the first newline.
		//
		bool locked = logger_state.try_lock( 10s );
		fputs( message.c_str() + locked, stderr );

		// Unlock if previously locked.
		//
		if ( locked ) logger_state.unlock();
#endif
	}

	// Prints an error message and breaks the execution.
	//
	template<typename... params>
	static void error [[noreturn]] ( const char* fmt_str, params&&... ps )
	{
		// If there is an active hook, call into it, else add formatting and print.
		//
#ifdef XSTD_CON_ERROR_REDIRECT
	#if XSTD_CON_ERROR_NOMSG
		XSTD_CON_ERROR_REDIRECT();
	#else
		XSTD_CON_ERROR_REDIRECT( fmt::str( fmt_str, std::forward<params>( ps )... ) );
	#endif
#else
		// Format error message.
		//
		std::string message = fmt::str(
			fmt_str,
			std::forward<params>( ps )...
		);
		message = XSTD_STR( "\n" ) + impl::translate_color( CON_RED ) + XSTD_STR( "[*] Error:" ) + std::move( message ) + '\n';

		// Try acquiring the lock and print the error, if properly locked skiped the first newline.
		//
		bool locked = logger_state.try_lock( 100ms );
		fputs( message.c_str() + locked, stderr );

		// Break the program, leave the logger locked since we'll break anyways.
		//
		unreachable();
		__debugbreak();
#endif
	}
};
