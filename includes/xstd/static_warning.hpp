#pragma once
#include "intrinsics.hpp"
#define ____SW_SX2(x, y) x##y
#define ____SW_SX1(x, y) ____SW_SX2(x, y)

#if MS_COMPILER
	#define static_warning(condition, message)                          \
       constexpr auto ____SW_SX1(____sw_, __LINE__) = []{               \
       	__pragma( warning( push ) )                                     \
       	__pragma( warning( 1:4996 ) )                                   \
       	struct [[deprecated( message )]] _ {};                          \
       	if constexpr ( !( condition ) )                                 \
              _{};                                                      \
       	__pragma( warning( pop ) )                                      \
            return 0;                                                   \
       }
#else
	#define static_warning(condition, message)                          \
       constexpr auto ____SW_SX1(____sw_, __LINE__) = []{               \
       	_Pragma("GCC diagnostic push")                                  \
       	_Pragma("-Wdeprecated")                                         \
           struct {                                           	        \
               [[deprecated( message )]] void __() const {}             \
           } _;                                                         \
           if constexpr ( !( condition ) )                              \
              _.__();                                                   \
       	_Pragma("GCC diagnostic pop")                                   \
            return 0;                                                   \
       }
#endif