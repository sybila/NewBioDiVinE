 /* GeNeSim (Genetic Networks Simulator) extensions for DiVinE
  * Copyright (C) 2007 David Safranek, Sven Drazan, Faculty of Informatics Masaryk
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
 * This file implements a property automaton for genesim system.
 */
#ifndef DIVINE_AFFINE_PROPERTY_HH
#define DIVINE_AFFINE_PROPERTY_HH

#ifndef DOXYGEN_PROCESSING
#include <assert.h>
#include <fstream>
#include <string>
#include <iostream>
#include <map>
#include <math.h>
#include <list>
#include <system/transition.hh>
#include <system/bio/affine_atomic_propositions.hh>

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

  //!Tranistion of a property process
  /*!Positive guards are list of affine APs that must be satisfied in order to
   * enable the transition. Simirarly, all negative guards must evaluate to
   * false to enable the transition.  */
  class affine_property_transition_t: public  divine::transition_t {
  public:
    
    //!Constructor
    affine_property_transition_t();

    //!Constructor
    affine_property_transition_t(const affine_property_transition_t &_from);

    //!Constructor
    affine_property_transition_t(std::string _from, std::string _to,
				  std::list<divine::affine_ap_t> *_positive_guards,
				  std::list<divine::affine_ap_t> *_negative_guards);

    //!Function to set the contents of the transition
    void set(std::string _from, std::string _to,
	     std::list<divine::affine_ap_t> *_positive_guards,
	     std::list<divine::affine_ap_t> *_negative_guards);

    void set_from_id(size_t _from)
    {
      from_id = _from;
    }

    void set_to_id(size_t _to)
    {
      to_id = _to;
    }

    size_t get_from_id()
    {
      return from_id;
    }

    size_t get_to_id()
    {
      return to_id;
    }

    std::string get_from_name()
    {
      return from_name;
    }

    std::string get_to_name()
    {
      return to_name;
    }

    //!Checks whether the transiotion is enabled over given state
    bool enabled(divine::state_t);
    

    //Obligatory virtual interface
    //!Returns a string representation
    virtual std::string to_string() const;

    virtual void write(std::ostream&) const;

    virtual int read(std::istream&, divine::size_int_t);

    virtual int from_string(std::string&, divine::size_int_t);

    void clear();

    //!Destructor
    virtual ~affine_property_transition_t();

  protected:
    std::string from_name;
    std::string to_name;
    size_t from_id;
    size_t to_id;
    std::list<divine::affine_ap_t> *positive_guards;
    std::list<divine::affine_ap_t> *negative_guards;    
  };

  //!Property automaton (Buchi accepting condition) for affine system
  class affine_property_t: public process_t {
  public:        
    //!Constructor
    affine_property_t();
    
    //!Add new states to property process. Re-inserting exisiting state is safe.
    void add_state(std::string _name);

    /*!Marks initial state. Assertion violated if the state to be marked as
     *initial hasn't been inserted before.
     */
    void set_initial(std::string _name);
    
    //!Returns internal ID the  an initial state
    size_t get_initial_state();

    /*!Marks accepting state. Assertion violated if the state to be marked as
     *accepting hasn't been inserted before.
     */
    void set_accepting(std::string _name);

    //!Returns whether the state with given internal ID is an accepting state
    bool get_accepting(size_t);

    //Obligatory inherited interface
    virtual std::string to_string() const;
    virtual void write(std::ostream&) const;
    virtual divine::transition_t* get_transition(divine::size_int_t _lid);
    virtual const divine::transition_t* get_transition(divine::size_int_t _lid) const;
    virtual divine::size_int_t get_trans_count() const;
    virtual void remove_transition(divine::size_int_t);      
    virtual int from_string(std::string&);
    virtual int read(std::istream&);
    
    /*!Inserts a new transition to affine preperty proces. Both states should
     * have been inserted before, otherwise assertion is violated.
     */
    virtual void add_transition(divine::transition_t * _trans);



    
        
    //!Destructor
    virtual ~affine_property_t();

  protected:
    std::map<std::string,size_t> state_names;
    array_t<bool> accepting;
    size_t initial;
    array_t<affine_property_transition_t *> transitions;
  };

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#endif //DOXYGEN_PROCESSING

#endif







