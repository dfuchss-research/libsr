#ifndef __paleo_test_macros_h__
#define __paleo_test_macros_h__

#include <stdio.h>
#include <string.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////
//    These macros are to be used for convenience in paleo test files.  They  //
//       assume you have a global "context" struct that contains a "result"   //
//            struct which inherits from Paleo results by including the       //
//                      PALEO_RESULT_UNION macro at its top.                  //
////////////////////////////////////////////////////////////////////////////////

// Convenience macro to set up the context's result as having failed.
#define SET_FAIL(msg, ...) { \
  const int room = strlen(msg) + 100; \
  context.result->fmsg = realloc( \
      context.result->fmsg, room * sizeof(char)); \
  if (room <= snprintf(context.result->fmsg, room, msg, ##__VA_ARGS__)) { \
    fprintf(stderr, "Wrote too many bytes."); \
    assert(0); \
  } \
  context.result->possible = 0; \
}

// Convenience macro that does the same as SET_FAIL and then returns the result.
#define SET_FAIL_RTN(msg, ...) SET_FAIL(msg, ##__VA_ARGS__) return;

#endif  // __paleo_test_macros_h__
