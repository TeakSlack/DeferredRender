#ifndef ASSERT_H
#define ASSERT_H

#include "Util/Log.h"

#ifdef COMPILE_WITH_ASSERT

#define DEBUG_BREAK() __debugbreak()

#define CORE_ASSERT(expr, msg) \
	do { \
		if (!(expr)) { \
			DEBUG_BREAK(); \
			CORE_ERROR("Assertion failed: {0}", msg); \
		} \
	} while (0);

#else

#define CORE_ASSERT(expr, msg)

#endif

#endif // ASSERT_H