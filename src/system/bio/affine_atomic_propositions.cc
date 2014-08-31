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
 * The main contribution of this file is to define special atomic proposition for 
 * genesim explicit system
 */
#ifndef DIVINE_ATOMIC_PROPOSITIONS_CC
#define DIVINE_ATOMIC_PROPOSITIONS_CC

#ifndef DOXYGEN_PROCESSING
#include "system/bio/affine_atomic_propositions.hh"

#endif //DOXYGEN_PROCESSING

  //!Copy constructor
  divine::affine_ap_t::affine_ap_t(const affine_ap_t &_from)
  {
    is_empty = _from.is_empty;
    index = _from.index;
    op = _from.op;
    treshold = _from.treshold;
    text_representation = _from.text_representation;
  };

  //!Constructor
  divine::affine_ap_t::affine_ap_t()
  {
    is_empty = 1;
  };
  
  //!Constructor
  divine::affine_ap_t::affine_ap_t(int _index, const int _op, size_t _treshold, std::string _text )
  {
    set(_index,_op,_treshold,_text);
  };

  //!Function to set the atomic proposition
  void divine::affine_ap_t::set(int _index, const int _op, size_t _treshold, std::string _text)
  {
    is_empty = 0;
    index = _index;
    op = _op;
    treshold=_treshold;
    text_representation = _text;
  };
  
  //!Function to set the atomic proposition to true (tt)
  void divine::affine_ap_t::set(std::string )
  {
    is_empty=1;
  }
  
  //!Returns whether the AP is valid in the given state
  bool divine::affine_ap_t::valid(state_t _state)
  {
    if (is_empty) return true;
        
    size_t treshold_pos = treshold;
    size_t *_s=static_cast<size_t*>(static_cast<void*>(_state.ptr));
    size_t value = _s[index];
    switch (op)
      {
      // strict inequalities coincide with nonstrict because we do not care
      // of threshold hyper-planes
      case AFFINE_AP_LESS:
	return value<treshold_pos;
        break;
      case AFFINE_AP_LESS_EQUAL:
	return value<treshold_pos;
	break;
	// case AFFINE_AP_EQUAL:
	// return value==treshold_pos;
	// break;
      case AFFINE_AP_GREATER:
	return value>=treshold_pos;
	break;
      case AFFINE_AP_GREATER_EQUAL:
	return value>=treshold_pos;
	break;
	// case AFFINE_AP_NOT_EQUAL:
	// return value!=treshold_pos;
	// break;
      }
    assert(0); //unknown arithmetic expression
    return false;
  }
  
  //! Returns a string representation of the object
  std::string divine::affine_ap_t::to_string()
  {
    if (is_empty)
      {
	return "true";
      }
    return text_representation;
    
//     std::ostringstream result;
//     result<<"dom["<<index<<"]";
//     switch (op)
//       {
//       case AFFINE_AP_LESS:
// 	result<<"<";
// 	break;
//       case AFFINE_AP_LESS_EQUAL:
// 	result<<"<=";
// 	break;
//       case AFFINE_AP_EQUAL:
// 	result<<"==";
// 	break;
//       case AFFINE_AP_GREATER:
// 	result<<">";
// 	break;
//       case AFFINE_AP_GREATER_EQUAL:
// 	result<<">=";
// 	break;
//       case AFFINE_AP_NOT_EQUAL:
// 	result<<"!=";
// 	break;
//       }
//     result<<"trshld["<<treshold<<"]";
//     return result.str();
  }
  
  //!Destructor
  divine::affine_ap_t::~affine_ap_t()
  {
  };

#endif




