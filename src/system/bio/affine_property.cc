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
#ifndef DIVINE_AFFINE_PROPERTY_CC
#define DIVINE_AFFINE_PROPERTY_CC

#ifndef DOXYGEN_PROCESSING
#include "system/bio/affine_property.hh"
 
#endif //DOXYGEN_PROCESSING

//!Constructor
divine::affine_property_transition_t::affine_property_transition_t()
{
  positive_guards = negative_guards = 0;
  from_id = to_id = 0;
  from_name = to_name= "" ;
};

//!Constructor
divine::affine_property_transition_t::affine_property_transition_t(const divine::affine_property_transition_t &_from)
{
  assert(0);
};
  
//!Constructor
divine::affine_property_transition_t::affine_property_transition_t(std::string _from, std::string _to,
								     std::list<divine::affine_ap_t> *_positive_guards,
								     std::list<divine::affine_ap_t> *_negative_guards)
{
  positive_guards = negative_guards = 0;
  set(_from,_to,_positive_guards,_negative_guards);
};

//!Function to set the contents of the transition
void divine::affine_property_transition_t::set(std::string _from, std::string _to,
						std::list<divine::affine_ap_t> *_positive_guards,
						std::list<divine::affine_ap_t> *_negative_guards)
{
  assert(positive_guards == 0);
  assert(negative_guards == 0);
  from_name = _from;
  to_name   = _to;
  positive_guards = _positive_guards;
  negative_guards = _negative_guards;
};

//Obligatory virtual interface
//!Returns a string representation
std::string divine::affine_property_transition_t::to_string() const
{
  std::list<divine::affine_ap_t>::iterator i;
  bool has_guards=false;
  std::string result = from_name + "->" + to_name + " {";
  if (positive_guards != 0)
    {
      has_guards=true;
      result += "guard ";
      for (i=positive_guards->begin();i!=positive_guards->end(); i++)
	{
	  if (i!=positive_guards->begin())
	    {
	      result +=" && ";
	    }
	  result += "("+i->to_string()+")";
	}
    }
  
  if (negative_guards != 0)
    {
      if (!has_guards) 
		{
		  result += "guard ";
		  has_guards=true;
		}
      for (i=negative_guards->begin();i!=negative_guards->end(); i++)
		{
		  if (i!=negative_guards->begin())
			{
			  result +=" && ";
			}
		  result += "(not "+i->to_string()+")";
		}
    } 

  if (has_guards)
    {
      result += ";";
    }
  result +="}";
  return result;    
};

bool divine::affine_property_transition_t::enabled(divine::state_t _state)
{
  std::list<divine::affine_ap_t>::iterator i;
  
  for (i=positive_guards->begin(); i!=positive_guards->end(); i++)
    {
      if (!(i->valid(_state)))
		{
		  return false;
		}	  
    }

  for (i=negative_guards->begin(); i!=negative_guards->end(); i++)
    {
      if ((i->valid(_state)))
		{
		  return false;
		}	  
    }
  return true;
}
 


void divine::affine_property_transition_t::write(std::ostream&) const
{
  gerr<<"affine_property_t does not implement write"<<thr();
  return;
};

int divine::affine_property_transition_t::read(std::istream&, divine::size_int_t)
{
  gerr<<"affine_property_t does not implement read"<<thr();
  return 0;
};

int divine::affine_property_transition_t::from_string(std::string&, divine::size_int_t)
{
  gerr<<"affine_property_t does not implement from_string"<<thr();
  return 0;
};

void divine::affine_property_transition_t::clear()
{
  positive_guards->clear();
  negative_guards->clear();
  delete positive_guards;
  delete negative_guards;
}

//!Destructor
divine::affine_property_transition_t::~affine_property_transition_t()
{
  clear();
}

//!Constructor
divine::affine_property_t::affine_property_t() 
{
};

//!Add new states to property process. Re-inserting exisiting state is safe.
void divine::affine_property_t::add_state(std::string _name)
{
  std::map<std::string,size_t>::iterator pos;
  pos = state_names.find(_name);
  if (pos == state_names.end()) //new state
    {
      size_t new_id=state_names.size();
      state_names[_name]=new_id;
      accepting.push_back(false); //state is not accepting by default

//       std::cout<<"New property proces state: "<<_name<<" with _lid "<<new_id
// 	       <<" and accepting="<<accepting[new_id]<<endl;
//       for (size_t u=0; u<accepting.size(); u++)
// 			{
// 	  			cout<<"-- accepting["<<u<<"]="<<accepting[u]<<endl;
// 			}

    }
};

/*!Marks initial state. Assertion violated if the state to be marked as
 *initial hasn't been inserted before.
 */
void divine::affine_property_t::set_initial(std::string _name)
{
  std::map<std::string,size_t>::iterator pos;
  pos = state_names.find(_name);
  if (pos != state_names.end()) //existing state
    {
      initial = state_names[_name];
      return;
    }
  gerr<<"Non-exisiting state in property process was tried to be marked as initial"<<thr();
}

/*!Returns the local id of the initial state
 */
size_t divine::affine_property_t::get_initial_state()
{
  return initial;
}


/*!Marks accepting state. Assertion violated if the state to be marked as
 *accepting hasn't been inserted before.
 */
void divine::affine_property_t::set_accepting(std::string _name)
{
//    cerr<<" set ACCEPTANCE for state_name=\""<<_name<<"\" which is " ;

  std::map<std::string,size_t>::iterator pos;
  pos = state_names.find(_name);
  if (pos != state_names.end()) //existing state
    {
      size_t index = state_names[_name];
      assert(index>=0 && index<state_names.size());
      accepting[index]=true;
//        cerr<<"lid="<<index<<endl;
//        for (size_t u=0; u<accepting.size(); u++)
//  	{
//  	  cerr<<"-- accepting["<<u<<"]="<<accepting[u]<<endl;
//  	}

      return;
    }
  gerr<<"Non-exisiting state in property process was tried to be marked as accepting."<<thr();
}

/*! Returns whether the state of given internal identification, has been denoted
 *  as accepting state. */
bool divine::affine_property_t::get_accepting(size_t _lid)
{
//    cerr<<" get ACCEPTANCE for lid="<<_lid<<endl;
//    for (size_t u=0; u<accepting.size(); u++)
//      {
//        cerr<<"-- accepting["<<u<<"]="<<accepting[u]<<endl;
//      }

  if (_lid >= state_names.size()) {
    gerr << "affine_property_t::get_accepting(size_t _lid) called with invalid value of _lid" << thr();
  }

  return accepting[_lid];
}



//Obligatory inherited interface
std::string divine::affine_property_t::to_string() const
{
  std::ostringstream auxstr;
  write(auxstr);
  return auxstr.str();
}

void divine::affine_property_t::write(std::ostream& out) const
{
  out <<"process LTL_property {"<<endl;
  out <<"state ";

  std::map<std::string,size_t>::const_iterator it;
  for (it = state_names.begin();
       it != state_names.end();
       it ++
       )
    {
      if (it != state_names.begin())
	out <<", ";
      out<<(*it).first;
    }

  out <<";"<<endl;
  out <<"init ";
  for (it = state_names.begin();
       it != state_names.end();
       it ++
       )
    {
      if ((*it).second == initial)
	out<<(*it).first;
    }
  out <<";"<<endl;

  out <<"accept ";
  bool printed_some=false;
  for (it = state_names.begin();
       it != state_names.end();
       it ++
       )
    {
      size_t state_id = (*it).second;
      if (accepting[state_id])
	{
	  if (printed_some)
	      out <<", ";
	  else
	    printed_some = true;
	  out<<(*it).first;
	}
    }
  out <<";"<<endl;

  out <<"trans"<<endl;
  //array_t<affine_property_transition_t *> transitions;
  for (array_t<affine_property_transition_t *>::const_iterator itr = transitions.begin();
       itr != transitions.end();
       itr ++)
    {
      if (itr!=transitions.begin())
	{
	  out <<","<<endl;
	}
      out<<"   "<<(*itr)->to_string();
    }
  out <<";"<<endl;  
  out <<"}"<<endl;
}

divine::transition_t* divine::affine_property_t::get_transition(divine::size_int_t _lid)
{
  return transitions[_lid];
}

const divine::transition_t* divine::affine_property_t::get_transition(divine::size_int_t _lid) const
{
  return transitions[_lid];
}

divine::size_int_t divine::affine_property_t::get_trans_count() const
{
  return transitions.size();
}

void divine::affine_property_t::remove_transition(divine::size_int_t)
{
  gerr <<"Not implemented."<<thr();      
}

int divine::affine_property_t::from_string(std::string&)
{
  gerr <<"Not implemented."<<thr();      
  return 1;
}

int divine::affine_property_t::read(std::istream&)
{
  gerr <<"Not implemented."<<thr();      
  return 1;
}

/*!Inserts a new transition to affine property proces. Both states should
 * have been inserted before, otherwise assertion is violated.
 */
void divine::affine_property_t::add_transition(divine::transition_t * _trans)
{
  affine_property_transition_t *trans =
    dynamic_cast<affine_property_transition_t *>(_trans);
  
  std::map<std::string,size_t>::iterator pos;
  pos = state_names.find(trans->get_from_name());
  if (pos == state_names.end())
    {
      gerr<<"Tried to define a transition going from a non-exisiting state "
	  <<"in property process."<<thr();
      return;
    }
  trans->set_from_id(state_names[trans->get_from_name()]);
  
  pos = state_names.find(trans->get_to_name());
  if (pos == state_names.end())
    {
      gerr<<"Tried to define a transition going to a non-exisiting state "
	  <<"in property process."
	  <<thr();
      return;
    }
  trans->set_to_id(state_names[trans->get_to_name()]);
  transitions.push_back(trans);
}    

// Methods needed for synchronization within affine_explicit_system_t::get_succs()

//!Destructor
divine::affine_property_t::~affine_property_t() 
{
  state_names.clear();
  accepting.clear();
  
  for(array_t<affine_property_transition_t*>::iterator i = transitions.begin();
      i != transitions.end();
      i++)
    {
      delete (*i);
    }
  transitions.clear();
};

#endif







