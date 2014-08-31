/*!\file
 * This file contains the funcions which can convert some integer types to
 * the string representation (e. g. convert 2345 to "2345").
 *
 * It also contains the function dispose_string that only calls free() on a
 * given argument.
 *
 * These function are only used in error.hh (\eunit) for converting
 * numbers on the error input to strings.
 */
#ifndef _INTTOSTR_HH_
#define _INTTOSTR_HH_

#include <sstream>
#include <cstring>

#ifndef DOXYGEN_PROCESSING
namespace divine //We want Doxygen not to see namespace `dve'
{
#endif //DOXYGEN_PROCESSING

 template<typename T> 
 static inline char * create_string_from(T i)
 {
   std::ostringstream o;
   o << i;
   size_t size = 1 + o.str().size();   // +1 for terminating 0
   char *auxstr = new char[size]; 
   memset(auxstr, 0, size);   
   strncpy(auxstr, o.str().c_str(), size);
   return auxstr;
 }
 
 void dispose_string(char * const str);

#ifndef DOXYGEN_PROCESSING
}; //end of namespace `dve'
#endif

#endif
