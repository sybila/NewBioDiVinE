/*!\file
 * This file contains a definition of \ref distr_reporter_t - class for
 * reporting and measuring during distributed computations
 *
 * \author Radek Pelanek
 */

#ifndef _DIVINE_DISTR_REPORTER_HH_
#define _DIVINE_DISTR_REPORTER_HH_

#ifndef DOXYGEN_PROCESSING
#include <iostream>

#include "distributed/distributed.hh"
#include "common/reporter.hh"

namespace divine {
#endif //DOXYGEN_PROCESSING

  //! Short output, compatible with sequential one.
  const size_t REPORTER_OUTPUT_SHORT = 1;
  //! Normal output, with avg/max/min... etc, but without the list of values
  const size_t REPORTER_OUTPUT_NORMAL = 2;
  //! Long detailed output with values on all workstations
  const size_t REPORTER_OUTPUT_LONG = 3;

  //! These are constants for set_info
  const size_t REPORTER_MASTER = 0;
  const size_t REPORTER_MIN = 1;
  const size_t REPORTER_AVG = 2;
  const size_t REPORTER_SUM = 3;
  const size_t REPORTER_MAX = 4;

  //!Class distr_reporter_t extends the class reporter_t with support for
  //! distributed computation.
  
  class distr_reporter_t : public reporter_t {
  protected:
    static const size_int_t BASIC_ITEMS;

    typedef struct {
      double num_val;
      std::string str_val;
      bool val_is_num;
    } global_info_value_t;
      

    //!Pointer to the instance of distributed_t
    distributed_t* distributed;
    //!Specific information of the report - adjustable by set_info()
    std::map<std::string, std::size_t> specific_info_flag; 
    std::map<std::string, global_info_value_t> global_info; 
    std::map<std::string, std::string> global_long_name; 
    std::vector<std::vector<double> > results;
    
    void print_specific_value(std::ostream & out, const std::string label,
                              const divine::size_int_t i);
    void _set_pr(std::ostream& out, double a);
    
  public:
    //!A constructor
    distr_reporter_t(distributed_t* d) { distributed = d; }

    //!Collects informations from workstations
    /*!Master prints it into the given ostream \a out.
     * The \a type_of_output specifies the verbosity of the output.
     */
    void collect_and_print(std::size_t type_of_output, std::ostream& out);
    
    void print(std::size_t type_of_output, std::ostream& out);

    void collect();
    
    //! Same as collect_and_print() but prints the report into the standard file
    void collect_and_print(std::size_t type_of_output = REPORTER_OUTPUT_NORMAL);
    
    //!Sets specific information to report.
    /*!The last argumet specifies
     * what should be reported in the case of REPORTER_OUTPUT_SHORT (only one
     * value is reported in this case).
     */
    void set_info(std::string s, double a, size_t flag = REPORTER_MASTER)
    { specific_info[s] = a; specific_info_flag[s] = flag; if (s=="InitTime") init_time=a; }
    void set_info(std::string s, double a, const std::string& long_name, size_t flag = REPORTER_MASTER)
    { distr_reporter_t::set_info(s,a,flag); specific_long_name[s]=long_name; }

    //!Sets global information to report.
    void set_global_info(std::string s, double a)
      {
	global_info_value_t gv;
	gv.num_val=a;
	gv.str_val="";
	gv.val_is_num=true;
	if (distributed->is_manager())
	  {
	    global_info[s] = gv;
	  }
	else
	  {
	    gerr << "Cannot set global info to report on non-master nodes."<<thr();
	  }
      }
    
    void set_global_info(std::string s, double a, const std::string & long_name)
    { set_global_info(s,a); global_long_name[s] = long_name; }

    //!Sets global information to report.
    void set_global_info(std::string s, std::string a)
      {
	global_info_value_t gv;
	gv.num_val=0;
	gv.str_val=a;
	gv.val_is_num=false;
	if (distributed->is_manager())
	  {
	    global_info[s] = gv;
	  }
	else
	  {
	    gerr << "Cannot set global info to report on non-master nodes."<<thr();
	  }
      }

    void set_global_info(std::string s, std::string a, const std::string & long_name)
    { set_global_info(s,a); global_long_name[s] = long_name; }

  }
;

#ifndef DOXYGEN_PROCESSING
} //end of namespace
#endif //DOXYGEN_PROCESSING

#endif 





