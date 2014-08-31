/*!\file
 * The main contribution of this file is the class affine_explicit_system_t
 */
#ifndef DIVINE_AFFINE_EXPLICIT_SYSTEM_HH
#define DIVINE_AFFINE_EXPLICIT_SYSTEM_HH

#ifndef DOXYGEN_PROCESSING
#include "system/explicit_system.hh"
#include "system/bio/affine_system.hh"
#include "system/state.hh"
#include "storage/explicit_storage.hh"
#include "common/types.hh"
#include "common/deb.hh"
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

namespace divine { //We want Doxygen not to see namespace `dve'
using std::cerr; using std::endl;
#endif //DOXYGEN_PROCESSING

class succ_container_t;

class affine_explicit_system_t : public explicit_system_t, public affine_system_t
{
private:
  //!Implements synchronization with property process, used by get_succs.
  //!Note that this deletes s from the memory.
  virtual void sync_with_prop(state_t& state, state_t& s, succ_container_t& succs);

  //! Internal function to enumerate initial states.
  void recursive_init(divine::state_t _state, size_t _var, succ_container_t & _succ, interval_t *_ins);

  //real_t *array_of_values;
  void print_aov();

  divine::hash_function_t hashf;
  size_t random(divine::state_t, divine::size_int_t seed=1) const;
  bool coin(divine::state_t, size_t treshold, divine::size_int_t seed=1) const;

 //PUBLIC PART:
 public:
 
 ///// VIRTUAL INTERFACE METHODS: /////
 
 //!A constructor
 /*!\param evect = \evector used for reporting of error messages*/
 affine_explicit_system_t(error_vector_t & evect);
 //!A destructor
 virtual ~affine_explicit_system_t();//!<A destructor.
 
 /*! @name Obligatory part of abstact interface
    These methods have to implemented in each implementation of
    explicit_system_t  */
 //!Implements explicit_system_t::is_erroneous() in AFFINE \sys, but
 //! see also implementation specific notes below
 /*!It constantly returns true - till now AFFINE does not
  * support any constrol of error states */
 virtual bool is_erroneous(state_t state);
 //!Implements explicit_system_t::is_accepting() in AFFINE \sys
 virtual bool is_accepting(state_t state, size_int_t acc_group=0, size_int_t pair_member=1);

 //!Implements explicit_system_t::violates_assertion() in AFFINE
 /*!Currently it only returns false, because assertions are not supported by
  * AFFINE */
 virtual bool violates_assertion(const state_t) const { return false; }

 //!Implements explicit_system_t::violated_assertion_count() in AFFINE
 /*!Currently it only returns 0, because assertions are not supported by
  * AFFINE */
 virtual size_int_t violated_assertion_count(const state_t) const
 { return 0; }
 
 //!Implements explicit_system_t::violated_assertion_string() in AFFINE
 /*!Currently it only returns empty string, because assertions are not
  * supported by AFFINE */
 virtual std::string violated_assertion_string(const state_t,
                                               const size_int_t) const
 { return std::string(""); }
                                                   
 
 //!Implements explicit_system_t::get_property_type()
 virtual property_type_t get_property_type();


 /*!This methods always returns 10000. No better estimation is implemented.*/
 virtual size_int_t get_preallocation_count() const;
 //!Implements explicit_system_t::print_state() in AFFINE \sys, but
 //! see also implementation specific notes below
 virtual void print_state(state_t state, std::ostream & outs = std::cout);
 //!Implements explicit_system_t::get_initial_state() in AFFINE \sys
 virtual state_t get_initial_state();
 //!Implements explicit_system_t::get_succs() in AFFINE \sys
 virtual int get_succs(state_t state, succ_container_t & succs);
 //!Implements auxiliary explicit_system_t::get_succs() in AFFINE \sys
 virtual int get_succs1(state_t _state, succ_container_t & succs, bool with_most_significant);
 //!Implements explicit_system_t::get_ith_succ() in AFFINE \sys
 virtual int get_ith_succ(state_t state, const int i, state_t & succ);
 /*@}*/
 
///// AFFINE EXPLICIT SYSTEM CANNOT WORK WITH SYSTEM TRANSITIONS /////
/*! @name Methods working with system transitions and enabled transitions
   These methods are not implemented and can_system_transitions() returns false
  @{*/

 //!Not imlemented in AFFINE \sys - throws error message
 virtual int get_succs(state_t state, succ_container_t & succs,
               enabled_trans_container_t & etc);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual int get_enabled_trans(const state_t state,
                       enabled_trans_container_t & enb_trans);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual int get_enabled_trans_count(const state_t state, size_int_t & count);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual int get_enabled_ith_trans(const state_t state,
                                 const size_int_t i,
                                 enabled_trans_t & enb_trans);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual bool get_enabled_trans_succ
   (const state_t state, const enabled_trans_t & enabled,
    state_t & new_state);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual bool get_enabled_trans_succs
   (const state_t state, succ_container_t & succs,
    const enabled_trans_container_t & enabled_trans);
 //!Not imlemented in AFFINE \sys - throws error message
 virtual enabled_trans_t * new_enabled_trans() const;
 /*@}*/
 
///// AFFINE EXPLICIT SYSTEM CANNOT EVALUATE EXPRESSIONS /////
 /*! @name Methods for expression evaluation
   These methods are not implemented and can_evaluate_expressions() returns
   false
  @{*/
 //!Not imlemented in AFFINE \sys - throws error message
 virtual bool eval_expr(const expression_t * const expr,
                        const state_t state,
                        data_t & data) const;


  virtual void DBG_print_state(state_t state, std::ostream & outs,
			       const ulong_int_t format);

 /*@}*/
 
}; //END of class affine_explicit_system_t

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#include "common/undeb.hh"

#endif //DOXYGEN_PROCESSING

#endif
