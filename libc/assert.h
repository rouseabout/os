#ifndef ASSERT_H
#define ASSERT_H

#define assert(ignore) ((void) 0)

#if !defined(__cplusplus)
#define static_assert _Static_assert
#endif

#endif /* ASSERT_H */
