#ifndef ASSERT_H
#define ASSERT_H

#ifdef COMPILE_WITH_ASSERT

#define DEBUG_BREAK() asm { int 3 }

#define CORE_ASSERT(expr, msg) \
	do { \
		if (!(expr)) { \
			DEBUG_BREAK(); \
			Log::Get().Error("Assertion failed: {0}", msg); \
		} \
	} while (0)

#else

#define CORE_ASSERT(expr)

#endif

#endif // ASSERT_H