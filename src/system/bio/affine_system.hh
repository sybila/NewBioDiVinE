 /* Bio extensions for DiVinE
  * Copyright (C) 2007-2008 Jiri Barnat, David Safranek, Sven Drazan, Faculty of Informatics Masaryk
  *     University Brno
  *
  * DiVinE - Distributed Verification Environment
  * Copyright (C) 2002-2007  Pavel Simecek, Jiri Barnat, Pavel Moravec,
  *     Radek Pelanek, Jakub Chaloupka, Faculty of Informatics Masaryk
  *     University Brno
  *
  *
  * DiVinE (both programs and libraries included in the distribution) is free
  * software; you can redistribute it and/or modify it under the terms of the
  * GNU General Public License as published by the Free Software Foundation;
  * either version 2 of the License, or (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  * or see http://www.gnu.org/licenses/gpl.txt
  */


/*!\file
 * The main contribution of this file is the class ma_system_t that represents multiaffine ode biological systems
 */
#ifndef DIVINE_AFFINE_SYSTEM_HH
#define DIVINE_AFFINE_SYSTEM_HH

#ifndef DOXYGEN_PROCESSING
#include <float.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <list>
#include <queue>
#include "system/system.hh"
#include "system/explicit_system.hh" //because of succ_container_t
#include "system/bio/affine_property.hh"
#include "common/array.hh"
#include "common/bitarray.hh"
#include "common/error.hh"

//#include "./parser/Parser.h"
#include "./data_model/Model.h"

#ifdef count
 #undef count
#endif
#ifdef max
 #undef max
#endif
#ifdef min
 #undef min
#endif
#ifdef PACKED
 #undef PACKED
#endif

//The main DiVinE namespace - we do not want Doxygen to see it
namespace divine {
#endif //DOXYGEN_PROCESSING

typedef double real_t;
const real_t RT_PRECISION = DBL_EPSILON;

typedef struct {
  size_t from;
  size_t to;
  real_t from_real;
  real_t to_real;
} interval_t;

typedef struct {
  real_t k;
  bitarray_t used_vars;
} summember_t;

typedef struct {
  size_t n_treshs;
  real_t *treshs;
  size_t n_sums;
  summember_t *sums;
  std::string name;
} vars_t;

typedef struct { 
  real_t value; 
  size_t var; 
  size_t pos;
} thr_t;


//!Class for entire multiaffine ODE system representation
/*!This class implements the abstract interface system_t
 *
 * It supports only very basic functionality of system_t interface
 * (processes, transition and expressions are not supported).
 * The calls of non-implemented methods cause error messsages.
 */
class affine_system_t: virtual public system_t
{
public:
  int inited;
  size_t dim;
  vars_t * vars;
  
	Model<real_t> model;

  //!A constructor.
  /*!\param estack =
   * the <b>error vector</b>, that will be used by created instance of system_t
   */
  affine_system_t(error_vector_t & evect = gerr);
  //!A destructor.
  virtual ~affine_system_t();

//nebude potrebne
 virtual  bool is_comment(std::string);

//nebude potrebne
virtual  void parse(std::string);

//nebude potrebne POUZIVA SA AJ V read_state() - mozno bude treba prepisat
virtual  real_t parseNumber(std::string);

//nebude potrebne
virtual  std::string getNextPart(std::string& _str);

//mozno uzitocne, ale bude treba prepisat ROVNAKE AKO dep() IBAZE VRACIA size_t
virtual size_t non_dependent( size_t rhs_var, size_t lhs_var );

//potrebne jedine ak nechame aj refineThreshs()
virtual real_t compute_inner_thrs( size_t _varx, size_t *_where, size_t _vary );

//potrebne jedine ak nechame aj refineThreshs()
virtual size_t recur_comp( size_t index, size_t* vec, size_t vec_size, size_t* idx );


virtual void update_initials();

//mozno uzitocne, ale bude treba prepisat
virtual size_t refineTreshs( size_t iterations );

virtual  real_t value(size_t * _where, size_t _direction);

virtual  signed int signum(size_t * _where, size_t _direction);

//mozno uzitocne, ale bude treba prepisat ROVNAKE AKO non_dependent() IBAZE VRACIA bool
virtual bool dep( size_t _var, size_t _next_dim );

virtual  void recursive(bool *result,
			size_t _var,
			bool _up_direction,
			size_t *_state,
			size_t _next_dim,
			real_t *all_values_array, bool dbg = false//FIXME SVEN
			);

//zrejme nebude treba
virtual  void check_sanity();

virtual  void error(const char *method, std::string err) const;

virtual  void error(const char *method, const char *err) const;

virtual  void error(const char *method, const int err_type) const;


//mozno bude treba - vola sa v recursive() cez write_state() ale iba v (dbg)
  //! Returns a size of state vector (in chars).
virtual  size_t get_state_size();

  //======================================================

//mozno bude treba  - vola sa v recursive() ale iba v (dbg)
  //! Returns a string representing a state.
virtual  std::string write_state(divine::state_t _state);

//nepouziva sa
//! Returns a state represented by a string
virtual  divine::state_t read_state(std::string _str);

  //! Sets the dimension of the system.
virtual  void set_dim(size_t _dim);

  //! Gets the dimension of the system.
virtual  size_t get_dim();

  //! Define a name of a variable with given index;
virtual  void set_varname(size_t _var, std::string _name);

  //! Returns a name of the  variable with given index;
virtual  std::string get_varname(size_t _var);

  //! Gets variable index from its name, if the name is invalid, returns the dimension (invalid id)
virtual  size_t get_varid(std::string _name);

  //! Returns the value of the given treshold
virtual  real_t get_tresh(size_t _var, size_t _tresh_id);

  //! Returns the position of the given treshold value
virtual size_t get_tresh_pos( size_t _var, real_t _thr_value);

  //! Returns the number of tresholds in the given dimension (variable).
virtual  size_t get_treshs(size_t _var);

//nadalej neuzitocne - nutne prerobit v Model.h
  //! Prints all initial conditions to stderr.
virtual void print_initials();

//nadalej neuzitocne - nutne prerobit v Model.h
  //! Prints all tresholds to stderr.
virtual void print_treshs();

  //!Sets one tresholds
virtual size_t add_tresh(size_t _var, real_t _tres);

  //! Sets the number of summants in the equation of the given variable.
virtual  void set_sums(size_t _var, size_t _n_sums);

  //! Returns the number of summants in the equation of the given variable.
virtual  size_t get_sums(size_t _var);

  //!Sets one of the summants in the equation of the given variable.
virtual  void add_sum(size_t _var, real_t _k, bitarray_t _usedvars);

  //======================================================

  /*! @name Obligatory part of abstact interface
     These methods have to be implemented in each implementation of system_t*/
  //!Warning - this method is still not implemented - TODO
  virtual slong_int_t read(std::istream & ins = std::cin);

  virtual slong_int_t read(const char * const filename);

  virtual slong_int_t read(const char * const filename, bool fast);
  
  //Neuplne implementovane
  virtual slong_int_t from_string(const std::string str);

  virtual bool write(const char * const filename);
// staci upravit telo - napr v triede model implementovat write a tu ho iba zavolat model.write()
  virtual void write(std::ostream & outs = std::cout);

  virtual std::string to_string();
  /*@}*/

  /*! @name Methods working with property process
    These methods are not implemented and can_property_process() returns false
   @{*/
   virtual process_t * get_property_process();
   virtual const process_t * get_property_process() const;
   virtual size_int_t get_property_gid() const;
   virtual void set_property_gid(const size_int_t gid);
   virtual void set_property_process(divine::affine_property_t *);
  /*@}*/


//    ///// AFFINE SYSTEM CANNOT WORK WITH PROPERTY PROCESS: /////
//    /*! @name Methods working with property process
//      These methods are not implemented and can_property_process() returns false
//     @{*/
//    //!Not imlemented in GENESIM \sys - throws error message
//    virtual process_t * get_property_process();
//    //!Not imlemented in GENESIM \sys - throws error message
//    virtual const process_t * get_property_process() const;
//    //!Not imlemented in GENESIM \sys - throws error message
//    virtual size_int_t get_property_gid() const;
//    //!Not imlemented in GENESIM \sys - throws error message
//    virtual void set_property_gid(const size_int_t gid);
//    /*@}*/

  
  ///// AFFINE SYSTEM CANNOT WORK WITH PROCESSES: /////
  /*!@name Methods working with processes
    These methods are not implemented and can_processes() returns false.
   @{*/
  //!Not imlemented in AFFINE \sys - throws error message
  virtual size_int_t get_process_count() const;
  //!Not imlemented in AFFINE \sys - throws error message
  virtual process_t * get_process(const size_int_t gid);
  //!Not imlemented in AFFINE \sys - throws error message
  virtual const process_t * get_process(const size_int_t id) const;
  //!Not implemented in AFFINE \sys - throws error message
  virtual property_type_t get_property_type();
  /*@}*/


  ///// AFFINE SYSTEM CAN WORK WITH TRANSITIONS: /////
  /*!@name Methods working with transitions
   @{*/
  //!Not imlemented in AFFINE \sys - throws error message
  virtual size_int_t get_trans_count() const { return dim; };
  //!Not imlemented in AFFINE \sys - throws error message
  virtual transition_t * get_transition(size_int_t gid);
  //!Not imlemented in AFFINE \sys - throws error message
  virtual const transition_t * get_transition(size_int_t gid) const;
  /*@}*/
  
  ///// AFFINE SYSTEM CANNOT BE MODIFIED: /////
  /*!@name Methods modifying a system
     These methods are not implemented and can_be_modified() returns false.
   @{*/
  //!Not imlemented in AFFINE \sys - throws error message
  virtual void add_process(process_t * const process);
  //!Not imlemented in AFFINE \sys - throws error message
  virtual void remove_process(const size_int_t process_id);
  /*@}*/

//ZMAZAT
  //! Returns the ratio of sizes of transitions that justifies their presence.
  real_t get_significance_treshold();

//ZMAZAT    
  //! Sets the ratio of sizes of transitions that justifies their presence.
  void set_significance_treshold(real_t _significance);

//ZMAZAT
  //! Sets the number of treshold refinment iterations.
  void set_iterations(size_t _it);

//ZMAZAT
  //! Sets the zero precision, if 0 is set, turns of the feature
  void set_zero_range(real_t);

//ZMAZAT
  //! Sets randomization type (0 ... off, 1 ... h^p, 2 ... h^r)
  void set_randomize(size_t);

//ZMAZAT
  //! Sets randomization parameter (multiplies the probability), (0,1]
  void set_rndparameter(real_t);

//ZMAZAT
  //! Sets selfloops (with non-transiency detection) (0 ... off, 1 ... on)
  void set_selfloops(size_t);

//ZMAZAT
  //! Gets the zero precision, if 0 is returned, the feature is turned off
  real_t get_zero_range();

//ZMAZAT
  //! Computes recommended zero precision using the RT_PRECISION constant
  real_t compute_zero_range();

//ZMAZAT
  //! Performs static partitioning of space of the given variable
  void static_partitioning(std::string _varname, real_t _stFrom, real_t _stTo, real_t _stStep);

//ZMAZAT
  //! Sets and returns recommended zero precision using the RT_PRECISION constant
  real_t compute_treshold_precision();

 protected:
 
 	bool useFastApproximation = false;
	
  real_t precision_thr;
  real_t zero_range;
  divine::affine_property_t *property_process;

  std::list<interval_t*> initials;

  bool has_system_keyword;
  real_t *array_of_values;
  bool most_signifficant_only;
  real_t significance;
  size_t refinement_iterations;

  // randomization switch (works only with most_significant_only ON)
  size_t randomize;  // 0 ... randomization switched off 
                   // 1 ... randomized due to h^p (hibi paper)
                   // 2 ... randomized due to h^r (hibi paper, not implemented yet!!!!)
  real_t maxprob;  // this constant is used to globally lower down the probability of rand. trans.

  bool selfloops;  // true implies generating of selfloops at any place where we are not sure of transiency
  
 };

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#endif //DOXYGEN_PROCESSING

#endif


