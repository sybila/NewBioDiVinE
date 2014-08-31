#include <cstdlib>
#include "common/inttostr.hh"

using namespace std;

//deprecated:
//extern "C" {
//extern int asprintf (char **__restrict __ptr,
//                     __const char *__restrict __fmt, ...)
//                    /*__THROW __attribute__ ((__format__ (__printf__, 2, 3)))*/;
//}

void divine::dispose_string(char * const str)
{ delete [] str; }

