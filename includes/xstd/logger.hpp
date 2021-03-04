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
// XSTD_CON_ENFORCE_MODE_WINDOWS: If set, will enforce the command-line to output UTF8 on Windows platform.
// XSTD_CON_NO_COLORS: If set, disables colors.
// XSTD_CON_NO_WARNINGS: If set, disables warnings.
// XSTD_CON_NO_LOGS: If set, disables logs.
// XSTD_CON_ERROR_REDIRECT: If set, redirects errors to the set name. [Prototype: extern "C" void __cdecl [[noreturn]] ( const std::string& )]
// XSTD_CON_ERROR_NOMSG: If set, changes the redirect prototype to extern "C" void __cdecl [[noreturn]] ();
// XSTD_CON_ERR_DST: If set, changes the error/warning logging destination from stderr to the given FILE*.
// XSTD_CON_MSG_DST: If set, changes the generic logging destination from stdout to the given FILE*.
// XSTD_CON_IFLUSH: If set, instantaneously flushes the file after every message.
// XSTD_CON_SCOPED: If set, enables padding/scope handling.
//

#ifndef XSTD_CON_THREAD_LOCAL
	#define XSTD_CON_THREAD_LOCAL 1
#endif
#ifndef XSTD_CON_ENFORCE_MODE_WINDOWS
	#define XSTD_CON_ENFORCE_MODE_WINDOWS 1
#endif
#ifndef XSTD_CON_NO_COLORS
	#define XSTD_CON_NO_COLORS 0
#endif
#ifndef XSTD_CON_NO_LOGS
	#define XSTD_CON_NO_LOGS 0
#endif
#ifndef XSTD_CON_NO_WARNINGS
	#define XSTD_CON_NO_WARNINGS XSTD_CON_NO_LOGS
#endif
#ifndef XSTD_CON_ERROR_NOMSG
	#define XSTD_CON_ERROR_NOMSG 0
#endif
#ifndef XSTD_CON_ERR_DST
	#define XSTD_CON_ERR_DST stderr
#endif
#ifndef XSTD_CON_MSG_DST
	#define XSTD_CON_MSG_DST stdout
#endif
#ifndef XSTD_CON_SCOPED
	#define XSTD_CON_SCOPED 0
#endif
#ifdef XSTD_CON_ERROR_REDIRECT
	#if XSTD_CON_ERROR_NOMSG
		extern "C" void __cdecl XSTD_CON_ERROR_REDIRECT [[noreturn]] ();
	#else
		extern "C" void __cdecl XSTD_CON_ERROR_REDIRECT [[noreturn]] ( const char* );
	#endif
#endif

#if ( WINDOWS_TARGET && XSTD_CON_ENFORCE_MODE_WINDOWS )
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
#if XSTD_CON_SCOPED
		// Whether prints are muted or not.
		//
		bool mute = false;

		// Current padding level.
		//
		int padding = -1;

		// Padding leftover from previous print.
		//
		int padding_carry = 0;
#endif

		// Constructor initializes logger.
		//
		logger_state_t()
		{
#if ( WINDOWS_TARGET && XSTD_CON_ENFORCE_MODE_WINDOWS )
			constexpr uint32_t _CP_UTF8 = 65001;
			SetConsoleOutputCP( _CP_UTF8 );
			constexpr uint32_t _ENABLE_VIRTUAL_TERMINAL_PROCESSING = 4;
			constexpr uint32_t _STD_OUTPUT_HANDLE = ( uint32_t ) -11;
			unsigned long mode;
			GetConsoleMode( GetStdHandle( _STD_OUTPUT_HANDLE ), &mode );
			SetConsoleMode( GetStdHandle( _STD_OUTPUT_HANDLE ), mode | _ENABLE_VIRTUAL_TERMINAL_PROCESSING );
#endif
		}

		// Lock of the stream.
		//
#if XSTD_CON_THREAD_LOCAL
		std::recursive_timed_mutex mtx;
		void lock() { mtx.lock(); }
		void unlock() { mtx.unlock(); }
		bool try_lock() { return mtx.try_lock(); }
		bool try_lock( duration max_wait ) { return mtx.try_lock_for( 100ms ); }
#else
		void lock() {}
		void unlock() {}
		bool try_lock() { return true; }
		bool try_lock( duration max_wait ) { return true; }
#endif
	};
	inline logger_state_t logger_state = {};

#if XSTD_CON_SCOPED
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
#else
	struct scope_padding {};
	struct scope_verbosity {};
#endif

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

		FORCE_INLINE static int handle_scope( FILE* dst, const char* cstr )
		{
			// If we should pad this output:
			//
			int out_cnt = 0;
#if XSTD_CON_SCOPED
			if ( logger_state.mute )
				return -1;
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
							if ( cstr[ 0 ] == ' ' ) putchar( log_padding_c );
						}
						else
						{
							out_cnt += fprintf( dst, XSTD_CSTR( "%*c%c" ), log_padding_step - 1, ' ', log_padding_c );
						}
					}
				}

				// Set or clear the carry for next.
				//
				if ( cstr[ strlen( cstr ) - 1 ] == '\n' )
					logger_state.padding_carry = 0;
				else
					logger_state.padding_carry = logger_state.padding;
			}
#endif
			return out_cnt;
		}

		template<bool has_args>
		static int log_w( FILE* dst, console_color color, const char* fmt_str, ... )
		{
			// If it's a message:
			//
			std::unique_lock lock( logger_state, std::defer_lock_t{} );
			int out_cnt = 0;
			if ( dst == XSTD_CON_MSG_DST )
			{
				// Handle scope details:
				//
				lock.lock();
				out_cnt = handle_scope( dst, fmt_str );
				if ( out_cnt < 0 )
					return 0;
			}

			// Set to requested color and redirect to printf.
			//
			fputs( translate_color( color ), dst );

			// If string literal with no parameters, use puts instead.
			//
			if constexpr ( has_args )
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
			
			// Flush the file if requested so.
			//
#if XSTD_CON_IFLUSH
			fflush( dst );
#endif
			return out_cnt;
		}
	};

#if !XSTD_CON_NO_LOGS
	// Generic logger.
	//
	template<typename... Tx>
	FORCE_INLINE static int flog( FILE* dst, console_color color, const char* fmt_str, Tx&&... ps )
	{
		auto buf = fmt::create_string_buffer_for<Tx...>();
		return impl::log_w<sizeof...( Tx ) != 0>( dst, color, fmt_str, fmt::fix_parameter<Tx>( buf, std::forward<Tx>( ps ) )... );
	}
	template<console_color color = CON_DEF, typename... Tx>
	FORCE_INLINE static int flog( FILE* dst, const char* fmt_str, Tx&&... ps )
	{
		return flog( dst, color, fmt_str, std::forward<Tx>( ps )... );
	}
	template<typename... Tx>
	FORCE_INLINE static int log( console_color color, const char* fmt_str, Tx&&... ps )
	{
		return flog( XSTD_CON_MSG_DST, color, fmt_str, std::forward<Tx>( ps ) ... );
	}
	template<console_color color = CON_DEF, typename... Tx>
	FORCE_INLINE static int log( const char* fmt_str, Tx&&... ps )
	{
		return flog( XSTD_CON_MSG_DST, color, fmt_str, std::forward<Tx>( ps )... );
	}

	// Logs the object given as is instead of using any other formatting specifier.
	//
	template<console_color color = CON_DEF, typename... Tx> requires ( sizeof...( Tx ) != 0 )
	FORCE_INLINE static int finspect( FILE* dst, Tx&&... objects )
	{
		std::string result = fmt::as_string( std::forward<Tx>( objects )... ) + '\n';
		return flog( dst, color, result.c_str() );
	}
	template<console_color color = CON_DEF, typename... Tx>
	FORCE_INLINE static int inspect( Tx&&... objects )
	{
		return finspect<color>( XSTD_CON_MSG_DST, std::forward<Tx>( objects )... );
	}
#else
	template<typename... Tx> FORCE_INLINE static int flog( Tx&&... ps ) { return 0; }
	template<console_color, typename... Tx> FORCE_INLINE static int flog( Tx&&... ps ) { return 0; }
	template<typename... Tx> FORCE_INLINE static int log( Tx&&... ps ) { return 0; }
	template<console_color, typename... Tx> FORCE_INLINE static int log( Tx&&... ps ) { return 0; }

	template<typename... Tx> FORCE_INLINE static int finspect( Tx&&... ps ) { return 0; }
	template<console_color, typename... Tx> FORCE_INLINE static int finspect( Tx&&... ps ) { return 0; }
	template<typename... Tx> FORCE_INLINE static int inspect( Tx&&... ps ) { return 0; }
	template<console_color, typename... Tx> FORCE_INLINE static int inspect( Tx&&... ps ) { return 0; }
#endif

	// Prints a warning message.
	//
	template<typename... Tx>
	static void warning( const char* fmt_str, Tx&&... ps )
	{
#if !XSTD_CON_NO_WARNINGS
		// Forward to f-log with a prefix and a color.
		//
		std::lock_guard _g{ logger_state };
		flog<CON_YLW>( XSTD_CON_ERR_DST, XSTD_CSTR( "\n[!] Warning: " ) );
		flog<CON_YLW>( XSTD_CON_ERR_DST, fmt_str, std::forward<Tx>( ps )... );
		flog<CON_DEF>( XSTD_CON_ERR_DST, XSTD_CSTR( "\n" ) );
#endif
	}

	// Prints an error message and breaks the execution.
	//
	template<typename... Tx>
	static void error [[noreturn]] ( const char* fmt_str, Tx&&... ps )
	{
		// If there is an active hook, call into it, else add formatting and print.
		//
#ifdef XSTD_CON_ERROR_REDIRECT
	#if XSTD_CON_ERROR_NOMSG
		XSTD_CON_ERROR_REDIRECT();
	#else
		if constexpr ( sizeof...( Tx ) != 0 )
		{
			std::string buffer = fmt::str( fmt_str, std::forward<Tx>( ps )... );
			XSTD_CON_ERROR_REDIRECT( buffer.c_str() );
		}
		else
		{
			XSTD_CON_ERROR_REDIRECT( fmt_str );
		}
	#endif
		// If no error messages are requested:
		//
#elif XSTD_CON_ERROR_NOMSG
		// Throw an exception if we can.
		//
	#if !XSTD_NO_EXCEPTIONS
		throw std::exception{};
	#endif
		// If we should print the error to console:
		//
#else
		// Format error message.
		//
		std::string buffer;
		const char* error;
		if constexpr ( sizeof...( Tx ) != 0 )
		{
			buffer = fmt::str(
				fmt_str,
				std::forward<Tx>( ps )...
			);
			error = buffer.c_str();
		}
		else
		{
			error = fmt_str;
		}

		// Forward to f-log with a prefix and a color.
		//
		bool locked = logger_state.try_lock( 2s );
		flog<CON_RED>( XSTD_CON_ERR_DST, XSTD_CSTR( "\n[x] Error: " ) );
		flog<CON_RED>( XSTD_CON_ERR_DST, error );
		flog<CON_DEF>( XSTD_CON_ERR_DST, XSTD_CSTR( "\n" ) );

		// Flush the file if requested so.
		//
	#if XSTD_CON_IFLUSH
		fflush( XSTD_CON_ERR_DST );
	#endif

		// Unlock if previously locked.
		//
		if ( locked ) logger_state.unlock();

		// If exceptions are allowed, throw one.
		//
		if ( locked ) logger_state.unlock();
	#if !XSTD_NO_EXCEPTIONS
		throw std::runtime_error( error );
	#endif
#endif
		// Break the program.
		//
#if DEBUG_BUILD
		debugbreak();
		unreachable();
#else
		exit( -1 );
#endif
	}
};
