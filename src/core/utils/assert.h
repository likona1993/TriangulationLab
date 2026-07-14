#pragma once
#include <cassert>

// Проверка инварианта, которая компилируется в no-op в релизных сборках
// (NDEBUG), в отличие от смысловых проверок ошибок во время выполнения.
#ifndef NDEBUG
#define GEO_ASSERT_INVARIANT(cond, msg) assert((cond) && msg)
#else
#define GEO_ASSERT_INVARIANT(cond, msg) ((void)0)
#endif
