/*!\file
 * This file contains declaration of logger_t class. */
#ifndef _DIVINE_LOGGER_HH_
#define _DIVINE_LOGGER_HH_

#ifndef DOXYGEN_PROCESSING
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/times.h>

#include "common/sysinfo.hh"
#include "distributed/distributed.hh"
#include "storage/explicit_storage.hh"

void on_SIGALRM_signal(int);

namespace divine {
#endif//DOXYGEN_PROCESSING

  /*! Class logger_t is used to periodically log various values into
   *  logfiles. There are as many logfiles produced as there are
   *  workstations participating the computation. The logfiles may be
   *  further processed by plotlog script that uses gnuplot to create
   *  corresponding (e)ps graphs. The class can be used only in combination
   *  with class distributed_t. 
   *  
   *  NOTE: Usage of the logger_t class slowdowns the execution of the
   *  algorithm. */
  class logger_t
  {
  private:
    bool initialized;
    vminfo_t vm;
    int format;
    clock_t firsttick;
    tms tmsstruct;
    distributed_t *divine_ptr;
    size_int_t net_id;
    std::string host;
    explicit_storage_t *storage_ptr;
    std::ofstream logger_file;
    std::string logger_file_name;
    unsigned long int log_counter;

    typedef unsigned (*unsignedfunc_t)();
    unsignedfunc_t ufunc[10];
    std::string ufunc_name[10];
    int ufunc_count;

    typedef int (*intfunc_t)();
    intfunc_t ifunc[10];
    std::string ifunc_name[10];
    int ifunc_count;

    typedef double (*doublefunc_t)();
    doublefunc_t dfunc[10];
    std::string dfunc_name[10];
    int dfunc_count;
    
    public:
    class log_ulong_int_t {
      public:
      virtual ulong_int_t log() const = 0;
      virtual ~log_ulong_int_t() {} 
    };
    private:
    const log_ulong_int_t * ufunctor[10];
    std::string ufunctor_name[10];
    int ufunctor_count;
    
    public:
    class log_slong_int_t {
      public:
      virtual slong_int_t log() const = 0;
      virtual ~log_slong_int_t() {}
    };
    private:
    const log_slong_int_t * sfunctor[10];
    std::string sfunctor_name[10];
    int sfunctor_count;
    
    public:
    class log_double_t {
      public:
      virtual double log() const = 0;
      virtual ~log_double_t() {}
    };
    private:
    const log_double_t * dfunctor[10];
    std::string dfunctor_name[10];
    int dfunctor_count;
    
  public:
    unsigned int signal_period;
    
    /*! Constructor does nothing. To initialize the instance of the class
     *  use the init member function. */
    logger_t();

    /*! Destructor prints footer to the logfile and closes the logfile. */
    ~logger_t();

    /*! This member function is called to initialize an instance of logger_t
     *  class. It accepts two obligatory parameters: a pointer to instance
     *  of distributed_t class and a base name of logfiles. (The base name is
     *  extended with .00, .01, .02, etc, by logger_t to distinguish
     *  individual logfiles.) The third parameter that gives the format of
     *  logfiles is optional (default value is 0, which corresponds to the
     *  format compatibile with plotlog script).
     *
     *  The method opens the logfile and print header into it according the
     *  chosen format. The function should be called after the set_storage
     *  and all the register member functions were called.
     *
     *  \a divine_ptr_in can be zero value (network-related values will be then
     *  zero too).*/
    void init(distributed_t * divine_ptr_in, std::string basename,
              int format = 0);

    /*! The same as
     *  init(distributed_t * divine_ptr_in, std::string basename, int format),
     *  but divine_ptr_in = 0 implicitly.
     */
    void init(string tmpstr_in, int format_in = 0);

    /*! If logging of the number of states kept in the storage is requested,
     *  this member function should be used to set the pointer to instance
     *  of explicit_storage_t class.
     * 
     *  Should be called before init member function. */
    void set_storage(explicit_storage_t *storage_ptr);

    /*! Forces the logger to log current values now.  If signal/alarm
     *  mechanism is involved, then this function is typically called just
     *  before the destructor to log final values. If no signal/alarm
     *  mechanism is used, then this function is supposed to be called
     *  periodically. */      
    void log_now();

    /*! Starts POSIX signal/alarm mechanism to call log_now function
     *  periodically. The period is given in seconds. (One second is the
     *  shortest possible period.)
     *
     *  Should be called after the init member function. */
    void use_SIGALRM(unsigned int period=1);

    /*! Stops POSIX signal/alarm mechanism. (Starts ingoring incomming
     *  signals.) */
    void stop_SIGALRM(); 

    /*! This function can be used to register function that will be called
     *  by the log_now function to obtained unsigned int value to be logged
     *  into logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file.
     *
     *  This can be used to log the number of states stored in the queue of
     *  states waiting for exploration, for example. */
    void register_unsigned(unsignedfunc_t func_ptr,
                           std::string description = "");
    
    /*! This function can be used to register function that will be called
     *  by the log_now function to obtained int value to be logged into
     *  logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file. */
    void register_int(intfunc_t func_ptr, std::string description = ""); 

    /*! This function can be used to register function that will be called
     *  by the log_now function to obtained double value to be logged into
     *  logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file. */
    void register_double(doublefunc_t func_ptr, std::string description = "");
    
    /*! This function can be used to register functor that will be called
     *  by the log_now function to obtained unsigned int value to be logged
     *  into logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file.
     *
     *  This can be used to log the number of states stored in the queue of
     *  states waiting for exploration, for example. */
    void register_ulong_int(const log_ulong_int_t & functor,
                            std::string description = "");

    /*! This function can be used to register functor that will be called
     *  by the log_now function to obtained signed int value to be logged
     *  into logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file.
     *
     *  This can be used to log the number of states stored in the queue of
     *  states waiting for exploration, for example. */
    void register_slong_int(const log_slong_int_t & functor,
                            std::string description = "");

    /*! This function can be used to register functor that will be called
     *  by the log_now function to obtained double f. p. value to be logged
     *  into logfile. The second optional parameter is a short (below 8
     *  characters) description of the value that could be (depending on
     *  format) printed in the header of the file.
     *
     *  This can be used to log the number of states stored in the queue of
     *  states waiting for exploration, for example. */
    void register_double(const log_double_t & functor,
                            std::string description = "");

  };

#ifndef DOXYGEN_PROCESSING
}
#endif

#endif
