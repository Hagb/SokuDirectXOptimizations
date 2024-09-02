#include <assert.h>

#define myassert(expr)                                                         \
  do {                                                                         \
    auto v = (expr);                                                           \
    assert(v);                                                                 \
  } while (0)