#ifndef ASSERT_H
#define ASSERT_H

#define assert(expr) do { if (expr) {} } while(0)
#define static_assert _Static_assert

#endif /* ASSERT_H */
