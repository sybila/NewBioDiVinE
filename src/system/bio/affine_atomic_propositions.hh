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
 * affine explicit system
 */
#ifndef DIVINE_ATOMIC_PROPOSITIONS_HH
#define DIVINE_ATOMIC_PROPOSITIONS_HH

#ifndef DOXYGEN_PROCESSING
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include "system/state.hh"
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

  const int AFFINE_AP_LESS          = 112;
  const int AFFINE_AP_LESS_EQUAL    = 113;
  const int AFFINE_AP_EQUAL         = 114;
  const int AFFINE_AP_GREATER       = 115;
  const int AFFINE_AP_GREATER_EQUAL = 116;
  const int AFFINE_AP_NOT_EQUAL     = 117;

  //!Class representing a single atomic proposition
  class affine_ap_t {
  public:
    //!Copy constructor
    affine_ap_t(const affine_ap_t &);

    //!Constructor
    affine_ap_t();

    //!Constructor
    affine_ap_t(int _index, const int _op, size_t _treshold, std::string text);

    //!Function to set the atomic proposition
    void set(int _index, const int _op, size_t _treshold, std::string text);

    //!Function to set the atomic proposition to true (tt)
    void set(std::string = std::string("true"));

    //!Returns whether the AP is valid in the given state
    bool valid(state_t _state);

    //! Returns a string representation of the object
    std::string to_string();
  
    //!Destructor
    ~affine_ap_t();
  protected:
    bool is_empty;
    int index;
    size_t treshold;
    int op;
    std::string text_representation;
  };

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#endif //DOXYGEN_PROCESSING

#endif
