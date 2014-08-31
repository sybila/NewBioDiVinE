#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>
#include "common/deb.hh"
#include "system/bio/affine_system.hh"
#include <bitset>
#include <map>
#include <limits.h>
#include <algorithm>
#include "common/bit_string.hh"

#include "./parser/parse.cc"
#include "./scanner/lex.cc"
#include "./data_model/Model.h"


#ifndef DOXYGEN_PROCESSING
using namespace divine;
#endif //DOXYGEN_PROCESSING

const unsigned int_bits = CHAR_BIT * sizeof(unsigned);

// ==================================================================

//===================================================================
// Local auxiliary functions
//===================================================================

// Should remove leading and terminating white space
void  trim(std::string& _l)
{
  if (!_l.empty())
    {
      _l.erase(_l.find_last_not_of(" \t\n")+1);
      if (!_l.empty())
        {
          size_t p=_l.find_first_not_of(" \t\n");
          if (p!=std::string::npos)
            _l.erase(0,p);
        }
    }
}

// Splits a string according to a separator. The result is stored in a queue of strings.
std::queue<std::string>  split(std::string _separator, std::string _what)
{
  std::queue<std::string> retval;
  while (!_what.empty())
    {
      size_t pos = _what.find(_separator);
      std::string s;
      if (pos==std::string::npos)
        {
          s=_what;
          _what.clear();
        }
      else
        {
          s=_what.substr(0,pos-(_separator.size()-1));
          _what.erase(0,pos+_separator.size());
        }
      trim(s);
      retval.push(s);
      //std::cout <<"split:"<<s<<" _what="<<_what<<std::endl;
    }
  return retval;
}

//===================================================================
// affine_system_t members
//===================================================================

void affine_system_t::set_zero_range(real_t _zero)
{
  zero_range = _zero;
}

real_t affine_system_t::get_zero_range()
{
  return zero_range;
}


real_t affine_system_t::get_significance_treshold()
{
  return significance;
}

    
void affine_system_t::set_significance_treshold(real_t _significance)
{
  if (_significance<=0 || _significance >1)
    {
      error("affine_system_t::set_significance_treshold","invalid significance specified ");
    }
  significance=_significance;
  if (_significance == 1)
    {
      most_signifficant_only = false;
    }
  else
    {
      most_signifficant_only = true;
    }
}

void affine_system_t::set_randomize(size_t _rndtype)
{
  if ( _rndtype<0 || _rndtype >2 )
    {
      error("affine_system_t::set_randomize","invalid randomization specified ");
    }
  randomize = _rndtype;
}

void affine_system_t::set_rndparameter(real_t _rp)
{
  if ( _rp<=0 || _rp >1 )
    {
      error("affine_system_t::set_rndparameter","invalid randomization parameter specified ");
    }
  maxprob = _rp;
}

void affine_system_t::set_selfloops(size_t _sl)
{
  if ( _sl<0 || _sl >1 )
    {
      error("affine_system_t::set_selfloops","invalid selfloops setting ");
    }
  if ( _sl )
    selfloops = true;
  else
    selfloops = false;
}


void affine_system_t::static_partitioning(std::string _varname, real_t _stFrom, real_t _stTo, real_t _stStep)
{
  size_t id=get_varid(_varname);
  for (real_t val=_stFrom; val<=_stTo; val+=_stStep)
    {
      add_tresh(id,val);
    }
}

//Constructor
affine_system_t::affine_system_t(error_vector_t & evect):system_t(evect),inited(0), dim(0), vars(0)  
{
  property_process = 0;
  has_system_keyword = false;
  get_abilities().system_can_property_process = true;
  significance=1;
  most_signifficant_only = false;
  array_of_values = 0;
  zero_range = 0;
  precision_thr = 0;
  refinement_iterations = 0;

  randomize = 0;
  maxprob = 1;
  selfloops = true;
}

//Destructor
affine_system_t::~affine_system_t() 
{
  if (array_of_values)
    free(array_of_values);
}

void affine_system_t::set_iterations(size_t _it)
{
  refinement_iterations = _it;
}

size_t affine_system_t::get_state_size()
{
  size_t retval;

  if ( get_with_property() )
    retval = (get_dim() + 1)*sizeof(size_t);
  else
    retval = get_dim()*sizeof(size_t);
  return retval;
}

std::string affine_system_t::write_state(divine::state_t _state)
{
  if (inited==0) 
  	error ("affine_system_t::write_state",1);
  	
  std::ostringstream retval;
  retval<<"[";
  for (size_t i=0; i < get_dim(); i++)
    {
      size_t aux=*(static_cast<size_t*>(static_cast<void*>(sizeof(size_t)*i+_state.ptr)));
      if (i == 0 and aux == get_treshs(0))
		{
		  retval <<"pre-initial]";	       	   
		  return retval.str();	  
		}
      retval << model.getThresholdForVarByIndex(i, aux) << "(" << aux << ")" << (i == get_dim()-1?"":",");
    }
  if (get_with_property()) 
    {
      size_t aux=*(static_cast<size_t*>(static_cast<void*>(sizeof(size_t)*get_dim()+_state.ptr)));
      retval <<"-PP:"<<aux;
    }
  retval <<"]";	       	   
  return retval.str();
}

divine::state_t affine_system_t::read_state(std::string _stateStr)
{  
  if (!inited) error ("read_state",1);
  
  divine::state_t _s;
  size_t propPos=0;
  std::string _str=_stateStr;
  _s.size=get_state_size();
  _s.ptr=new char[sizeof(size_t)*_s.size];
  if (!_s.ptr)
    {
      error("read_state",3);
    }
  memset(_s.ptr,0,_s.size);

  if (std::string::npos!=_str.find("initial"))
    {
      //CONDITIONS on pre-initial state:
      // _s[0]=get_treshs(0); 
      // _s[get_dim()] = intial location of property process, if property process is present

      (static_cast<size_t*>(static_cast<void*>(_s.ptr)))[0]=get_treshs(0);
      if (get_with_property()) 
	{
	  affine_property_t *propproc = dynamic_cast<affine_property_t*>(get_property_process());
	  assert (propproc!=0);
	  size_t temp_pp_pos=propproc->get_initial_state();
	  (static_cast<size_t*>(static_cast<void*>(_s.ptr)))[get_dim()] = temp_pp_pos;
	}
      return _s;
    }
  
  trim(_str);
  _str.substr(1,_str.size()-2);
  std::queue<std::string> f;
  if (get_with_property())
    f=split("-PP:",_str);
  else
    f.push(_str);
  
  std::queue<std::string> f1=split(",",f.front());
  size_t index=0;
  while (!f1.empty())
    {
      std::string auxs=f1.front();
      auxs=auxs.substr(auxs.find("(")+1);
      auxs=auxs.substr(0,auxs.size()-1); // erase terminating ')'
      size_t val=trunc(parseNumber(auxs));
      (static_cast<size_t*>(static_cast<void*>(_s.ptr)))[index]=val;      
      f1.pop();
      index++;
    }
  f.pop();
  if (get_with_property()) 
    {
      propPos=trunc(parseNumber(f.front()));
      (static_cast<size_t*>(static_cast<void*>(_s.ptr)))[get_dim()]=propPos;
    }
  return _s;
}

size_t affine_system_t::non_dependent( size_t rhs_var, size_t lhs_var )
{
  size_t non_dep = 1;
  for ( size_t i = 0; i < vars[lhs_var].n_sums; i++ )
    {
      if ( vars[lhs_var].sums[i].used_vars.get(rhs_var) )
	non_dep = 0;
    }

  return non_dep;
}

// computes _varx position of sign-changing point (only _vary is moving all other vars are fixed)
// _where is the position in which to compute (_where[_vary] is expected to be set to 0),
// in other words we find _vary_value such that the following is satisfied: 
//                f_varx(_where, _vary_value) = 0
// note: returns 0 if f_varx does not depend on _vary
real_t affine_system_t::compute_inner_thrs( size_t _varx, size_t *_where, size_t _vary )
{
  real_t sum_const = 0;
  real_t sum_vary = 0;
  for ( size_t i = 0; i < vars[_varx].n_sums; i++ )
    {
      real_t aux = 1;
      for ( size_t k = 0; k < get_dim(); k++ )
	{
	  // skip the unknown variable
	  if ( k == _vary ) 
	    { 
	      k++;
	      if ( k >= get_dim() )
		break;
	    }	  

	  if ( _where[k] >= vars[k].n_treshs )
	    std::cerr << "bad position " << _where[k] << "  of " << k << endl;

	  if ( vars[_varx].sums[i].used_vars.get(k) )
	    aux *= vars[k].treshs[_where[k]];
	}
      if ( vars[_varx].sums[i].used_vars.get(_vary) )
	sum_vary += vars[_varx].sums[i].k * aux;
      else
	sum_const += vars[_varx].sums[i].k * aux;
    }

  real_t result = 0;
  if ( sum_vary != 0 )
    result = (- sum_const) / sum_vary; 

  if ( vars[_vary].treshs[0] < result && result < vars[_vary].treshs[vars[_vary].n_treshs-1] )
    return result;
  else
    return 0;
}


// recursive function that computes next position in the threshold mesh
size_t affine_system_t::recur_comp( size_t index, size_t* vec, size_t vec_size, size_t* idx )
{
  size_t retval = 1;
  
  if ( index < 0 || index >= vec_size ) return 0;
  
  if ( vec[index]+1 >= vars[idx[index]].n_treshs )
    { 
      size_t rtv;
      rtv = recur_comp( index-1, vec, vec_size, idx );
      if ( not rtv ) retval = 0;
    }
  else
    {
      vec[index]++;
      for ( size_t i = index+1; i < vec_size; i++ )
	vec[i] = 0;
    }
  
  return retval;
}

size_t affine_system_t::refineTreshs( size_t it )
{
  const bool dbg = false;

  std::queue<thr_t> thrs_set;

  // initialize the set of thresholds to the threshold grid given in the model
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      for ( size_t j = 0; j < vars[i].n_treshs; j++ )
	{
	  thr_t thr;
	  thr.value = vars[i].treshs[j];
	  thr.var = i;
	  thr.pos = j;

	  // we exclude outter thresholds (as sanity is assumed)
	  if ( 0 < thr.pos && thr.pos < vars[i].n_treshs-1 )
	    thrs_set.push(thr);
	}
    }

  if ( true )
    {
      std::cerr << " * Computing threshold refinement for total " 
		<< thrs_set.size() 
		<< " initial thresholds..." 
		<< endl;
    }

  size_t iterations = 0;
  std::queue<thr_t> thrs_added;

  // iterate the refinement procedure up to the precision
  while ( iterations < it )
    {
      if (dbg) std::cerr << " ** Iteration " << iterations << " **" << endl;

      // consider all thresholds in the queue
      while ( not thrs_set.empty() )
	{
	  thr_t thr;
	  thr = thrs_set.front();
	  thrs_set.pop();
	  
	  // choose the unknown variable
	  for ( size_t i = 0; i < get_dim(); i++ )
	    {
	      if ( i != thr.var && not non_dependent(i,thr.var) )
		{
		  if (dbg)
		    std::cerr << "Solving threshold " << vars[thr.var].name << "[" << thr.pos << "]" << " for " << vars[i].name << endl;
		  
		  if ( get_dim() > 2 )
		    {
		      // count redundant variables 
		      size_t step = 0;
		      for ( size_t k = 0; k < get_dim(); k++ )
			{
			  if ( k == i || k == thr.var || non_dependent(k,thr.var) )
			    step++;
			}

		      size_t* where = new size_t[ get_dim() - step ];
		      size_t* idx = new size_t[ get_dim() - step ];
		      size_t* where_actual = new size_t[ get_dim() ];
		      for ( size_t j = 0; j < get_dim() - step; j++ )			
			where[j] = 0;			 

		      // get only dependent variables 
		      step = 0;
		      for ( size_t k = 0; k < get_dim(); k++ )
			{
			  if ( k == i || k == thr.var || non_dependent(k,thr.var) )
			    step++;
			  else
			    idx[k-step] = k;
			}
		      
		      bool stop = false;
		      while ( not stop ) 
			{		   
			  // initialize complete current position array
			  for ( size_t k = 0; k < get_dim(); k++ )
			    where_actual[k] = 0;			    
			  for ( size_t k = 0; k < get_dim() - step; k++ )
			    where_actual[idx[k]] = where[k];
			  where_actual[thr.var] = thr.pos;

			  real_t added_thr_value;
			  added_thr_value = compute_inner_thrs( thr.var, where_actual, i );
			  if ( added_thr_value )
			    {
			      if (dbg)
					std::cerr << "adding inner threshold to " << vars[i].name << " at " << added_thr_value << endl;
			      size_t added;			      
			      added = add_tresh( i, added_thr_value );
			      if ( added ) {
				thr_t added_thr;
				added_thr.value = added_thr_value;
				added_thr.var = i;
				added_thr.pos = get_tresh_pos( i, added_thr_value );
				thrs_added.push( added_thr );
			      }
			    }

			  // consider another position in the threshold plane			  
			  size_t where_size = get_dim() - step;
			  size_t rtv = recur_comp( where_size-1, where, where_size, idx );

			  // check if it is not the maximal position 
			  bool not_max_pos = 1;
			  for ( size_t k = 0; k < get_dim() - step; k++ )
			    if ( where[k] != vars[idx[k]].n_treshs - 1 )
			      not_max_pos = 0;

			  if ( not rtv || not_max_pos )	  
			    stop = true;
			  
			  if (dbg)
			    std::cerr << "generating position: ";
			  for ( size_t k = 0; k < where_size; k++ )
			    if (dbg)
			      std::cerr << where[k] << ",";

			  if (dbg)
			    std::cerr << endl;
			}
		      
		      delete [] where;
		      delete [] where_actual;
		      delete [] idx;
		    }
		  else if ( get_dim() == 2 )
		    {		 
		      size_t* where_actual = new size_t[ get_dim() ];		  
		      where_actual[i] = 0;
		      where_actual[thr.var] = thr.pos;
		      
		      if (dbg)
			std::cerr << "taking " << where_actual[0] << "," << where_actual[1] << endl;
		      
		      real_t added_thr_value;
		      added_thr_value = compute_inner_thrs( thr.var, where_actual, i );
		      if ( added_thr_value )
			{
			  if (dbg) 
			    std::cerr << "adding inner threshold to " << vars[i].name << " at " << added_thr_value << endl;
			  size_t added;
			  added = add_tresh( i, added_thr_value );
			  if ( added ) {
			    thr_t added_thr;
			    added_thr.value = added_thr_value;
			    added_thr.var = i;
			    added_thr.pos = get_tresh_pos( i, added_thr_value );
			    thrs_added.push( added_thr );
			  }
			}
		      
		      delete [] where_actual;
		    }
		}	 
	    }
	}

      if (dbg)
	std::cerr <<" ** " << thrs_added.size() << " thresholds added to stack ** " << endl;

      while ( not thrs_added.empty() )
	{
	  thr_t t;
	  t = thrs_added.front();
	  thrs_set.push(t);
	  thrs_added.pop();
	}

      iterations++;
    }

  return 1;
}

// _where musi byt pole indexov, kde kazda premenna ma jeden index a toto poradie je rovnake ako poradie premennych v poli vars.
// Tento index ukazuje do pola tresholdov danej premennej na danom poradi a urcuje tak presnu suradnicu v jednej dimenzii (cize hodnotu tejto premennej).
real_t affine_system_t::value(size_t *_where, size_t _var)
{
  bool dbg = false;
  real_t sum = 0;
  
  if(dbg) std::cerr << "Computing value() for var with index " << _var << "\n";

	for(size_t s = 0; s < model.getEquationForVariable(_var).size(); s++) {
	
		//adding value of constant in actual summember 's' of equation for variable '_var'
		real_t underSum = model.getSumForVarByIndex(_var,s).GetConstant();
		if(dbg) std::cerr << "\tconstant is " << underSum << "\n";

		//adding values of variables in actual summember 's' of equation for variable '_var' in given point
		for(size_t v = 0; v < model.getSumForVarByIndex( _var, s ).GetVars().size(); v++) {		
		
			size_t actualVarIndex = model.getSumForVarByIndex(_var,s).GetVars().at(v) - 1;
			real_t thres = model.getThresholdForVarByIndex( actualVarIndex, _where[ actualVarIndex ] );
			
			if(dbg) std::cerr << "\tthres for var " << actualVarIndex << " is " << thres << "\n";
			
			underSum *= thres;
		}
	
		//adding average value of actual summember's parameter, if any exists
		if(model.getSumForVarByIndex(_var,s).hasParam()) {
			std::pair<real_t,real_t> param = model.getParamRange(model.getSumForVarByIndex(_var,s).GetParam() - 1);
			underSum *= (param.second + param.first) * 0.5;
		}
		
		//adding enumerated ramps for actual summember 's' of equation for variable '_var'
		for(size_t r = 0; r < model.getSumForVarByIndex(_var, s).GetRamps().size(); r++) {
			
			size_t rampVarIndex = model.getSumForVarByIndex(_var, s).GetRamps().at(r).dim -1;
			real_t thres = model.getThresholdForVarByIndex( rampVarIndex, _where[ rampVarIndex ] );
			
			underSum *= model.getSumForVarByIndex(_var, s).GetRamps().at(r).value(thres);
		}
		
		//adding enumerated step functions for actual summember 's' of equation for variable '_var'
		for(size_t r = 0; r < model.getSumForVarByIndex(_var, s).GetSteps().size(); r++) {
			
			size_t stepVarIndex = model.getSumForVarByIndex(_var, s).GetSteps().at(r).dim -1;
			real_t thres = model.getThresholdForVarByIndex( stepVarIndex, _where[ stepVarIndex ] );
			
			underSum *= model.getSumForVarByIndex(_var, s).GetSteps().at(r).value(thres);
		}
/*
		//adding enumerated hill functions for actual summember 's' of equation for variable '_var'
		for(size_t r = 0; r < model.getSumForVarByIndex(_var, s).GetHills().size(); r++) {
			
			size_t hillVarIndex = model.getSumForVarByIndex(_var, s).GetHills().at(r).dim -1;
			real_t thres = model.getThresholdForVarByIndex( hillVarIndex, _where[ hillVarIndex ] );
			
			underSum *= model.getSumForVarByIndex(_var, s).GetHills().at(r).value(thres);
		}
*/
		
		//adding enumerated summember 's' to sum
		sum += underSum;
	}

  if ( dbg ) std::cerr << "final value = " << sum << std::endl;

  // if smaller than the largest possible real_t numerical error
  if ( (sum > 0 && sum < zero_range) || (sum < 0 && sum > -zero_range ) )
    {
      if ( dbg ) std::cerr << "value too small, taking as zero...[precision = " << zero_range << "]" << endl;

      sum = 0;
    }

  return sum;
}

signed int affine_system_t::signum(size_t *_where, size_t _var)
{
  if (value(_where,_var)>0) return 1;
  if (value(_where,_var)<0) return -1;
  return 0;
}

bool affine_system_t::dep( size_t _var, size_t _next_dim )
{
  bool retval = false;
  
  for( size_t i = 0; i < model.getEquationForVariable(_var).size(); i++ ) {
		for( size_t v = 0; v < model.getSumForVarByIndex(_var, i).GetVars().size(); v++ ) {
			if( model.getSumForVarByIndex(_var, i).GetVars().at(v) - 1 == _next_dim ) {
				retval = true;
				break;
			}
		}
  }
/*
  for ( size_t i = 0; i < vars[_var].n_sums; i++ )
    if ( vars[_var].sums[i].used_vars.get(_next_dim) ) {
      retval = true;
      break;
    }
*/
  return retval;
}

void affine_system_t::recursive(

  bool *_result,           // true means the transition is enabled
  size_t _var,             // dimension of transition (i.e. direction)
  bool _up_direction,      // whether we want to go up (true) or down (false)
  size_t *_state,          // point from where we should compute (if _rem_dims = 0)
  size_t _next_dim,        // next dimension according to which the state will be split
  real_t *all_values_array,// if not null then indicates that I want compute all values
                           // NOTE that this turns off early termination (short-circuit enumaration)!
  bool dbg                 // for debuging purposes
)
{

  if (dbg) std::cerr << "Running recursive for var:" << _var 
		     << " next_dim:" << _next_dim 
		     << std::endl;

  if (_next_dim == get_dim())
    {
      // now we have to compute
      if (dbg)
        {
          divine::state_t s;
          s.ptr = static_cast<char *>(static_cast<void*>(_state));
          s.size = get_state_size();
          std::cout << write_state(s);
        }

      if (all_values_array==NULL)
		{
		  if (_up_direction && signum(_state,_var) > 0)
			{
			  if (dbg) std::cerr << endl << value(_state,_var) << endl;
			  *_result = true;
			}

		  if (!_up_direction && signum(_state,_var) < 0)
			{
			  if (dbg) std::cerr << endl << value(_state,_var) << endl;
			  *_result = true;
			}
		}
      else
		{
		  if ( dbg ) std::cerr << "affine_system::recursive: computing array_of_values" << endl;

		  if (_up_direction)
			{
			  if (signum(_state,_var) > 0) 
				{
				  all_values_array[4*_var + 0] += value(_state,_var);
				  // if most_signifficant, we only compute the array
				  if ( !most_signifficant_only && selfloops )
				  		*_result = true;
		       	}	
			  if (signum(_state,_var) < 0)
				  all_values_array[4*_var + 1] += (-1)*value(_state,_var);
			}
		  if (!_up_direction)
			{
			  if (signum(_state,_var) > 0)
					all_values_array[4*_var + 2] += value(_state,_var);
			  if (signum(_state,_var) < 0)
				{ 
				  all_values_array[4*_var + 3] += (-1)*value(_state,_var);	
				  // if most_signifficant, we only compute the array
			  	  if ( !most_signifficant_only && selfloops )
			            *_result=true;
				}
			}
		  if (dbg) {
		        cerr << "Point: ";
		        for ( size_t i = 0; i < get_dim(); i++ ) 
		        	cerr << _state[i] << ".";
		        cerr << ": ";
		        for ( size_t i = 0; i < 4*get_dim(); i++ ) 
		        	cerr << all_values_array[i] << ", ";
				cerr << endl;
	      }
		}

      if (dbg) std::cerr << " res = " << (*_result) << std::endl;
      return;
    }

  if (_next_dim == _var) // we skip this dimension
    {
      recursive (_result, _var, _up_direction, _state, _next_dim+1, all_values_array, dbg);
    }
  else // we have to split according this direction
    {
      //SVEN FIXME pridal jsem podminku "&& all_values_array == NULL", aby se zamezilo predcasnemu
      //ukonceni
      if (*_result && all_values_array == NULL) 
      		return;
      // we can exit only if there is no all_values_array to compute, otherwise we must go on 
      
      if (dbg) std::cerr << "Shift" << std::endl;
      
      while ((_next_dim < get_dim()) && (! dep(_var, _next_dim)))		//DEMON - miesto 'not' som dal '!'
			_next_dim++;
      if (_next_dim < get_dim() ) 
      {
			if (dbg) std::cerr << "dependency found" << std::endl;
		
			//SVEN FIXME tento radek tu nebyl
			recursive (_result, _var, _up_direction, _state, _next_dim+1,all_values_array, dbg);
			//SVEN FIXME 
			
			_state[_next_dim]++;
			recursive (_result, _var, _up_direction, _state, _next_dim+1,all_values_array, dbg);
			_state[_next_dim]--;
      }
      else // if shifted
		{
		  recursive (_result, _var, _up_direction, _state, _next_dim, all_values_array, dbg);
		}
    }
}

std::string affine_system_t::getNextPart(std::string& _str)
{
  trim(_str);
  size_t partpos,t;
  if (_str.size()==0) return ("");

  partpos=_str.find("VARS:",1);
  t=_str.find("EQ:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("TRES:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("INIT:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("SIGNIFICANCE:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("ZERORANGE:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("REFINETRESHS:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("RANDOMIZE:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("RNDPARAMETER:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("SELFLOOPS:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("STATICTRESHS:",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("process",1); if (partpos==std::string::npos || partpos>t) partpos=t;
  t=_str.find("system",1); if (partpos==std::string::npos || partpos>t) partpos=t;

  std::string retval=_str.substr(0,partpos);
  _str.erase(0,partpos);

//   std::cout<<"Parted  =="<<retval<<"=="<<_str<<"=="<<std::endl;
  return retval;
}

real_t affine_system_t::parseNumber(std::string _str)
{
  real_t retval=0;
  trim(_str);
  if(_str[0]=='(' && _str[_str.size()-1]==')')
    {
      _str.erase(0,1);
      _str.erase(_str.size()-1,_str.size());
    }
  trim(_str);

  if (std::string::npos!=_str.find_first_of("eE"))
    {
      std::string saux  = _str.substr(_str.find_first_of("eE")+1);
      int exponent = round(parseNumber(saux));  
      saux = _str.substr(0,_str.find_first_of("eE"));      
      return parseNumber(saux)*pow(10,exponent);
    }

  bool negative = false;
  if (_str[0]=='-') negative=true;
  _str.erase(0,_str.find_first_of(".0123456789"));
  if (_str[0]=='.') _str='0'+_str;
  if (_str.find(".")==std::string::npos) _str=_str+".";
  while (_str[0]!='.')
    {
      //std::cout <<"  _str[0]='"<<_str[0]<<"'"<<std::endl;
      real_t i=_str[0]-'0';
      retval=retval*10+i;
      _str.erase(0,1);
    }
  _str.erase(0,1);
  real_t frac=0;
  size_t order=1.0;
  while (!_str.empty())
    {
      //std::cout <<"  _str[0]='"<<_str[0]<<"'"<<std::endl;
      real_t i=_str[0]-'0';
      frac=frac*10+i;
      order*=10.0;
      _str.erase(0,1);
    }
  retval+= frac/order;
  if (negative) retval *= -1;
  //std::cout<<"parse_num: "<<retval<<std::endl;
  return retval;
}

// this method's retval characterizes the numerical error of the computation of nullclines in refineTreshs()
// we can therefore use it for computation of derivations in the following way: 
//                        f(x) \in <-retval,retval> => f(x) = 0
//
// moreover the following members are set:
//                          zero_range = retval (see above)
//                          precision_thr (resolution of the threshold grid)
//
// note: you can increase zero_range -> it will be automatically considered in precision_thr
real_t affine_system_t::compute_zero_range()
{
  const bool dbg=false;
  real_t real_t_precision = RT_PRECISION; // for double this is pow(10,-14)
  real_t retval = 0;

  size_t mx_sums = 0;
  size_t mx_degree = 0;
  real_t mx_k = 0;
  real_t mx_value = 0;
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      mx_sums = std::max(vars[i].n_sums,mx_sums);
      mx_value = std::max(vars[i].treshs[vars[i].n_treshs-1],mx_value);
            
      for ( size_t j = 0; j < vars[i].n_sums; j++ )
	{
	  size_t lc_value = 0;
	  for ( size_t k = 0; k < get_dim(); k++) 
	    {
	      lc_value += vars[i].sums[j].used_vars.get(k);
	    }
	  mx_degree = std::max(lc_value, mx_degree);
	  mx_k = std::max(vars[i].sums[j].k,mx_k);
	}
    }
  
  // set precision of the derviation computation
  retval = (real_t_precision*mx_sums*pow(mx_value,mx_degree-1)*(mx_value + mx_k));
  zero_range = retval;

  if (dbg) 
    {
      std::cerr << "Detected real_t precision: "<<real_t_precision<<endl;
      std::cerr << "PRECISION computed: derivation precision = " 
		<< zero_range 
		<< ", threshold precision = "
		<<precision_thr<<endl;
    }
  return retval;
}

// Sets and returns the precision of the threshold grid. To derive the precision
// the zero precision is used if non-zero, RT_PRECISION oterwise.
real_t affine_system_t::compute_treshold_precision()
{
  size_t mx_sums = 0;
  size_t mx_degree = 0;
  real_t mx_k = 0;
  real_t mx_value = 0;
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      mx_sums = std::max(vars[i].n_sums,mx_sums);
      mx_value = std::max(vars[i].treshs[vars[i].n_treshs-1],mx_value);
            
      for ( size_t j = 0; j < vars[i].n_sums; j++ )
	{
	  size_t lc_value = 0;
	  for ( size_t k = 0; k < get_dim(); k++) 
	    {
	      lc_value += vars[i].sums[j].used_vars.get(k);
	    }
	  mx_degree = std::max(lc_value, mx_degree);
	  mx_k = std::max(vars[i].sums[j].k,mx_k);
	}
    }
  if (get_zero_range()==0)
    return (precision_thr = (RT_PRECISION / (mx_k*mx_degree*mx_value)));
  else
    return (precision_thr = (get_zero_range()/(mx_k*mx_degree*mx_value)));
}

// this method prints all thresholds of the system (.bio syntax is used)
void affine_system_t::print_treshs()
{
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      std::cerr << "TRES:" << vars[i].name << ":";
      for ( size_t j = 0; j < vars[i].n_treshs; j++ )
	{
	  std::cerr << vars[i].treshs[j];
	  if ( j < vars[i].n_treshs-1 ) std::cerr << ",";
	}
      std::cerr << "... total: " << vars[i].n_treshs << endl;
    }
}

// this method prints all initial conditions of the system
void affine_system_t::print_initials()
{
  std::cerr << "Number of initial conditions: " << initials.size() << endl;
  // for each interval in the list
  std::list<interval_t*>::iterator i = initials.begin();
  while ( i != initials.end() )
    {
      std::cerr << "INIT:";
      for ( size_t j = 0; j < get_dim(); j++ )
	{	
	  std::cerr << (*i)[j].from_real << "(" << (*i)[j].from << ")" << ":" 
		    << (*i)[j].to_real << "(" << (*i)[j].to << ")";
	  if ( j < get_dim() - 1 ) std::cerr << ":";
	}

      i++;      

      std::cerr << endl;
    }
}

/* This procedure checks for the sanity of just parsed system. The sanity checks are
 * - whether all equations are defined
 * - whether there are at least 2 tresholds for each variable
 * - whether the system is closed (all directions on the edge lead inwards)
 * - whether there are initial conditions specified and there was a 'system async;' keyword;
 */
void affine_system_t::check_sanity()
{
  const bool dbg=true;
  
  if (dbg)
    std::cerr<<"Sanity checks:"<<std::endl;

  // check for definition of all equations
  if (dbg) std::cerr << "  * Whether all equations are defined ... ";
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      if ( vars[i].n_sums < 1  )
	{	  
	  std::cerr <<std::endl;
	  std::cerr << "ERROR: There is no equation for variable " 
		    << get_varname(i) << "." << std::endl;
	  std::cerr << "Exiting ... " << std::endl;
	  exit(1);
	}
    }
  if (dbg) std::cerr << "\t\t\tYES" << std::endl;

  // check for at least 2 thresholds in each dimension
  if (dbg) std::cerr << "  * Whether there are enough thresholds in every dimension ... ";
  for ( size_t i = 0; i < get_dim(); i++ )
    {
      if ( vars[i].n_treshs < 2 )
	{
	  std::cerr << "ERROR: Variable " << get_varname(i) 
		    << " has less than 2 thresholds!" << std::endl;
	  std::cerr << "Exiting ... " << std::endl;
	  exit(1);
	}
    }
  if (dbg) std::cerr << "\tYES" << std::endl;
     
  // check if the specified system makes a closed box
  if (dbg) std::cerr << "  * Whether the system is closed ... ";
  bool warned=false;
  if (get_dim()>8)
    {
      if (dbg) std::cerr << "\t\t\t\tSKIPPED (too many variables).";
    }
  else
    {
      size_t* point = new size_t [ get_dim() ];
      
      for ( size_t i = 0; i < pow(2,get_dim()); i++ )
	{
	  for ( size_t j = 0; j < get_dim(); j++ )
	    {
	      point[j] = (std::bitset<int_bits>(i)[j] ? vars[j].n_treshs - 1 : 0);
	    }
	  
	  for ( size_t j = 0; j < get_dim(); j++ )
	    {
	      
	      // top boundary exceeded
	      if ( (signum(point, j) >= 0) && (point[j] > 0) )
		{
		  if (dbg)
		    {
		      if (!warned) 
			{
			  warned = true;
			  if (dbg) std::cerr << "\t\t\t\tNO" << std::endl;
			}
		      std::cerr << "      WARNING: In variable " << get_varname(j) 
				<< " the box is exceeded at point [";
		      
		      for ( size_t k = 0; k < get_dim(); k++ )
			{
			  std::cerr << (point[k] ? vars[k].treshs[point[k]] : vars[k].treshs[0]);
			  if ( k < get_dim()-1 ) std::cerr << ",";
			}
		      
		      std::cerr << "] (value = " << value(point, j) << ")" << std::endl;
		    }
		  // std::cerr << "Exiting ... " << std::endl;
		  // exit(1);
		}
	      
	      // bottom boundary 
	      if ( (signum(point, j) <= 0) && (point[j] == 0) )
		{
		  if (dbg)
		    {
		      if (!warned) 
			{
			  warned = true;
			  std::cerr << "\t\t\t\tNO" << std::endl;
			}
		      std::cerr << "      WARNING: In variable " << get_varname(j) 
				<< " the box is underflown at point [";
		      
		      for ( size_t k = 0; k < get_dim(); k++ )
			{
		      std::cerr << (point[k] ? vars[k].treshs[point[k]] : vars[k].treshs[0] );
		      if ( k < get_dim()-1 ) std::cerr << ",";
			}
		      
		      std::cerr << "] (value = " << value(point, j) << ")" << std::endl;
		    }
		  // std::cerr << "Exiting ... " << std::endl;
		  // exit(1);
		}
	    }      
	}
      delete [] point;
      if (dbg) if (!warned) { std::cerr << "\t\t\t\tYES"; }
    }

  // const int int_bits = dim * sizeof(int);
  // for ( int i = 0; i < pow(2,dim); i++ ) {
  //  for ( size_t j = 0; j < dim; j++ ) {
  //    if ( std::bitset<dim>(i)[j] )
  //	cerr << "......";
  //  }
  // }

  //checking for initial conditions
  if (dbg) std::cerr << "  * Whether initial conditions were specified ...";
  if (initials.empty())
    {
      if (dbg) std::cerr << "\t\tNO"<<std::endl;
      if (dbg) std::cerr << std::endl;
      std::cerr<<"    == WARNING: ======================================"<<std::endl;
      std::cerr<<"    = No initial conditions were specified!!!        ="<<std::endl;
      std::cerr<<"    = Using lowest values of all variables instead.  ="<<std::endl;
      std::cerr<<"    =================================================="<<std::endl;
      if (dbg) std::cerr << std::endl;
      interval_t *inits= new interval_t[get_dim()];
      for (size_t i=0; i<get_dim(); i++)
	{
	  inits[i].from=0;
	  inits[i].from=1;
	}
      initials.push_front(inits);     
    }
  else
    {
      if (dbg) std::cerr << "\t\tYES"<<std::endl;
    }

  //checking whether there was a 'system async;' keyword
  if (dbg) std::cerr << "  * Whether has obligatory 'system async' keyword ...";
  if (!has_system_keyword)
    {
      if (dbg) std::cerr<<"\t\tNO"<<std::endl;
      std::cerr <<"ERROR: File contains no 'system async' keyword,"
		<<" which is obligatory." << std::endl;
      std::cerr <<"Exiting ..."<<std::endl;
      exit(1);      
    }
  else
    {
      if (dbg) std::cerr << "\t\tYES"<<std::endl;
    }

  if (dbg)
    std::cerr<<"All sanity checks passed."<<std::endl;
}

void affine_system_t::parse(std::string _token)
{
  const bool dbg=true;
  if (dbg) std::cerr<<"Parsing token: =="<<_token<<"=="<<std::endl;
  size_t pos;
  std::queue<std::string> fields;
  std::queue<std::string> fields1;
/*
  // ---- VARS ----
  if ((pos=_token.find("VARS:"))!=std::string::npos)
    {
      _token.erase(0,5);
      fields=split(",",_token);
      if (dbg) std::cerr<<" -- found "<<fields.size()<<" variables."<<std::endl;
      set_dim(fields.size());
      size_t var_id=0;
      while (!fields.empty())
        {
          set_varname(var_id,fields.front());
          if (dbg) std::cerr<<"   - var:"<<fields.front()<<std::endl;
          fields.pop();
          var_id++;
        }
      return;
    }
  // ---- EQ ----
  if ((pos=_token.find("EQ:"))!=std::string::npos)
    {
      _token.erase(0,3);
      trim(_token);
      _token.erase(0,1); //remove leading 'd'
      pos=_token.find_first_of('=');
      std::string var_name=_token.substr(0,pos);
      var_name.erase(var_name.size(),1);
      trim(var_name);
      _token.erase(0,pos+1);
      fields=split("+",_token);
      if (dbg) std::cerr<<" -- found "<<var_name<<"("<<get_varid(var_name)
                        <<") with "<<fields.size()<<" summants"<<std::endl;
      set_sums(get_varid(var_name),fields.size());
      std::queue<std::string> members;
      while (!fields.empty())
        {
          members=split("*",fields.front());
          fields.pop();
          //if (dbg) std::cerr<<"   - "<<members.size()<<" members"<<std::endl;
          summember_t sm;
          sm.k=parseNumber(members.front());
          members.pop();
          sm.used_vars.clear();
          while(!members.empty())
            {
              sm.used_vars.set(get_varid(members.front()));
              members.pop();
            }
          if (dbg) std::cerr<<"   - k = "<<(sm.k>0?" ":"")
                            <<sm.k<<"\t var bits = "<<sm.used_vars.toString(get_dim())<<std::endl;
          add_sum(get_varid(var_name),sm.k,sm.used_vars);
        }

      return;
    }
  // ---- TRESH ----
  if ((pos=_token.find("TRES:"))!=std::string::npos)
    {
      _token.erase(0,5);
      trim(_token);
      pos=_token.find_first_of(':');
      std::string var_name=_token.substr(0,pos);
      var_name.erase(var_name.size(),1);
      trim(var_name);
      _token.erase(0,pos+1);
      fields=split(",",_token);
      if (dbg) std::cerr<<" -- found "<<var_name<<"(" <<get_varid(var_name)
                        <<") with "<<fields.size()<<" tresholds"<<std::endl;

      while(!fields.empty())
        {
          real_t val=parseNumber(fields.front());
          add_tresh(get_varid(var_name),val);
          fields.pop();
        }
    }

  // ---- REFINETRESHS ----
  if ((pos=_token.find("REFINETRESHS:"))!=std::string::npos)
    {
      _token.erase(0,13);
      trim(_token);
      real_t signif=parseNumber(_token);
      size_t it=signif;
      set_iterations(it);
      if (dbg) 
	{
	  std::cerr<<" -- Refine tresholds";
	  if (it==0) 
	    std::cerr<<" turned off."<<endl;      
	  else 
	    std::cerr<<" will be computed in "<<it<<" iterations."<<std::endl;     
	}
    }

  // ---- STATICTRESHS ----
  if ((pos=_token.find("STATICTRESHS:"))!=std::string::npos)
    {
      _token.erase(0,13);
      trim(_token);

      pos=_token.find_first_of(':');
      std::string var_name=_token.substr(0,pos);
      var_name.erase(var_name.size(),1);
      trim(var_name);
      _token.erase(0,pos+1);


      fields=split(",",_token);
      if (fields.size()!=3)
	{
	  error("parse","Static tresholds partitioning requires 3 parameters (from, to, step).");
	}

      real_t stFrom,stTo,stStep;
      stFrom=parseNumber(fields.front());
      fields.pop();
      stTo=parseNumber(fields.front());
      fields.pop();
      stStep=parseNumber(fields.front());
      fields.pop();

      if (dbg)
	{
	  std::cerr <<"StaticTreshs:"<<var_name<<":"<<stFrom<<","<<stTo<<","<<stStep<<std::endl;
	}
      
      static_partitioning(var_name,stFrom,stTo,stStep);
    }
  

  // ---- SIGNIFICANCE ----
  if ((pos=_token.find("SIGNIFICANCE:"))!=std::string::npos)
    {
      _token.erase(0,13);
      trim(_token);
      real_t signif=parseNumber(_token);
      set_significance_treshold(signif);
      if (dbg) std::cerr<<" -- Significance treshold: "<<significance<<std::endl;     
    }

  // ---- ZERO RANGE ----
  if ((pos=_token.find("ZERORANGE:"))!=std::string::npos)
    {
      _token.erase(0,10);
      trim(_token);
      real_t signif=parseNumber(_token);
      if (signif>=0) 
	set_zero_range(signif);
      else
	set_zero_range(0);	
      if (dbg) std::cerr<<" -- Zero range: "<<significance<<std::endl;     
    }

  // ---- RANDOMIZE ----
  if ((pos=_token.find("RANDOMIZE:"))!=std::string::npos)
    {
      _token.erase(0,10);
      trim(_token);
      size_t rndtype=parseNumber(_token);
      set_randomize(rndtype);
      if (dbg) std::cerr<<" -- Randomization variant: "<<randomize<<std::endl;
    }

  // ---- RNDPARAMETER ----
  if ((pos=_token.find("RNDPARAMETER:"))!=std::string::npos)
    {
      _token.erase(0,13);
      trim(_token);
      real_t rndparam=parseNumber(_token);
      set_rndparameter(rndparam);
      if (dbg) std::cerr<<" -- Randomization parameter: "<<maxprob<<std::endl;
    }

  // ---- SELFLOOPS ----
  if ((pos=_token.find("SELFLOOPS:"))!=std::string::npos)
    {
      _token.erase(0,10);
      trim(_token);
      size_t slflps=parseNumber(_token);
      set_selfloops(slflps);
      if (dbg) std::cerr<<" -- Selfloop setting: "<<selfloops<<std::endl;
    }

  // ---- INIT ----
  if ((pos=_token.find("INIT:"))!=std::string::npos)
    {
      _token.erase(0,5);
      trim(_token);
      if ((pos=_token.find(";"))!=std::string::npos) //we assume terminating ';'
	  	  _token.erase(_token.size()-1,1); // erase terminating ';'      
      trim(_token);
      fields=split(",",_token);
      if (fields.size()!=get_dim())
		{
		  std::cerr<<"Invalid number of fields in INIT: "<<_token<<";"<<std::endl;
		  exit(1);
		}
      interval_t *inits= new interval_t[get_dim()];
      size_t _dim=0;
      while (!fields.empty())
		{	  
		  fields1=split(":",fields.front());
		  if (fields1.size()!=2)
			{	      
			  std::cerr<<"Invalid interval specification in INIT: "<<_token<<";"
				   <<" Interval:"<< fields.front()
				   <<" size="<<fields1.size()
				   <<" fields1.front="<<fields1.front()
				   <<std::endl;
			  exit(1);
			}

		  real_t tr_val;
		  size_t tresh;
		  std::string numstr;

		  numstr=fields1.front();
		  fields1.pop();
		  trim(numstr);  
		  tresh=get_treshs(_dim);
		  tr_val=parseNumber(numstr);
		  for (size_t i=0; i<get_treshs(_dim); i++)
			{
			    if (get_tresh(_dim,i)==tr_val)
					tresh=i;
			}
		  if (tresh==get_treshs(_dim))
			{
			  std::cerr <<"Treshold "<<tr_val
				<<" used to define initial values is not among specified"
				<<"tresholds for variable "<<get_varname(_dim)
				<<"."<<std::endl;
			  exit(1);
			}	 
		  inits[_dim].from=tresh;
		  inits[_dim].from_real = vars[_dim].treshs[tresh];

		  numstr=fields1.front();
		  fields1.pop();
		  trim(numstr);
		  tresh=get_treshs(_dim);
		  tr_val=parseNumber(numstr);
		  for (size_t i=0; i<get_treshs(_dim); i++)
			{
			  	if (get_tresh(_dim,i)==tr_val)
					tresh=i;
			}
		  if (tresh==get_treshs(_dim))
			{
			  std::cerr <<"Treshold "<<tr_val
				<<" used to define initial values is not among specified"
				<<"tresholds for variable "<<get_varname(_dim)
				<<"."<<std::endl;
			  exit(1);
			}
		  inits[_dim].to=tresh;
		  inits[_dim].to_real = vars[_dim].treshs[tresh];

		  if (dbg) std::cerr <<"Init interval var "<<get_varname(_dim)<<"("<<_dim<<") interval "
					 <<inits[_dim].from<<"-"<<inits[_dim].to<<std::endl;


		  _dim++;
		  fields.pop();
		}
      initials.push_front(inits);
      if (dbg) std::cerr <<"  Convex region completed. Convex regions="<<initials.size()<<std::endl;
      
    }
  
*/
  // ---- process ----
  	if ((pos=_token.find("process"))!=std::string::npos)
    {
	  	if (get_property_process()!=0)
		{
	  		std::cerr <<"Too many DVE processes defined."<<std::endl;
	  		std::cerr <<"Exiting ..."<<std::endl; 
		}

      affine_property_t *prop=new affine_property_t();

	  _token.erase(0,7);
	  trim(_token);
	  pos=_token.find_first_of('{');
	  _token.erase(0,pos+1);
	  _token.erase(_token.size()-1,1);
	  trim(_token);

      
      // First we split the string representing the property process 
      // into state-definition part and transition-definition part.
      pos=_token.find("trans");
      std::string procstates=_token.substr(0,pos);
      _token.erase(0,procstates.size()+5);

      // now we split state definition according to ';'
      fields=split(";",procstates);

      // now we iterate, and identify individual parts
      while(!fields.empty())
      {
		  int mode=0;
		  if (dbg) std::cerr <<fields.front()<<std::endl;
		  if (fields.front().find("state")==0)
      	  {
			  fields.front().erase(0,6);
			  mode=1;
	      }

		  if (fields.front().find("init")==0)
		  {
			  fields.front().erase(0,5);
			  mode=2;
		  }

	  	  if (fields.front().find("accept")==0)
	      {
			  fields.front().erase(0,7);
			  mode=3;
	      }

		  // now having one part, we split accordin to ',' to get individual states
		  fields1=split(",",fields.front());
		  while (!fields1.empty())
		  {
			  switch (mode) {
			  case 1:
				prop->add_state(fields1.front());
				break;
			  case 2:
				prop->set_initial(fields1.front());
				break;
			  case 3:
				prop->set_accepting(fields1.front());
				break;
			  }
			  fields1.pop();
		  }
		  fields.pop();
	  }      
      
      //Here we parse the transition-definition part of the string representing property process.
      
      _token.erase(_token.size()-1,1); // erase terminating ';'      

      fields=split(",",_token);
      while(!fields.empty())
	  {
		  std::list<affine_ap_t> *positiveAP;
		  std::list<affine_ap_t> *negativeAP;
		  affine_ap_t APtrue, AP1;
		  affine_property_transition_t *trans;

		  fields.front().erase(fields.front().size()-1,1); // erase terminating '}'
		  fields1=split("{",fields.front());
		  pos=fields1.front().find("->");
		  std::string sname1=fields1.front().substr(0,pos);
		  trim(sname1);
		  std::string sname2=fields1.front().substr(pos+2,fields1.front().size()-pos-2);
		  trim(sname2);
		  fields1.pop();

		  if (dbg) std::cerr <<"prop trans:"<<sname1<<"->"<<sname2<<std::endl;
		  if (fields1.empty()) // empty transition ===> true
		  {
			  positiveAP = new std::list<affine_ap_t>();
			  negativeAP = new std::list<affine_ap_t>();
			  positiveAP->push_back(APtrue);	      
			  if (dbg) std::cerr <<" + true"<<std::endl;
		  }
		  else
		  {
			  fields1.front().erase(fields1.front().size()-1,1); // erase terminating ';'
			  fields1.front().erase(0,6); // erase leading 'guard '
			  std::string aux=fields1.front();
			  fields1.pop();
			  fields1=split("&&",aux);
			  positiveAP = new std::list<affine_ap_t>();
			  negativeAP = new std::list<affine_ap_t>();
			  while (!fields1.empty())
		  	  {
				  //..... parsing Atomic Proposition ....
				  if ( fields1.front().find("!")!=std::string::npos)
				  {
					  std::cerr<<"Atomic proposition contains !, which is not allowed."<<std::endl;
					  exit(1);
				  }

				  if ( fields1.front().find("==")!=std::string::npos)
				  {
					  std::cerr<<"Atomic proposition contains ==, which is not allowed."<<std::endl;
					  exit(1);
				  }
				  
				  bool negated=false;
				  affine_ap_t AP;
				  std::string APstr=fields1.front();

				  //Remove possible ()
				  if (APstr.find("(")!=std::string::npos)
				  {
					  if (APstr.find("(")!=0 || APstr.find(")")!=APstr.size()-1)
					  {
	  					  error("parse", "cannot parse atomic proposition "+APstr);
					  }
					  APstr.erase(0,1);
					  APstr.erase(APstr.size()-1,1);
				  }
				  trim(APstr);

				  //Test for true statement
				  if (APstr.find("true")!=std::string::npos || 
					  APstr.find("True")!=std::string::npos ||
					  APstr.find("TRUE")!=std::string::npos
					  ) 
				  {
					  positiveAP->push_back(APtrue);	      
					  fields1.pop();
					  continue;
				  }
				  
				  if (APstr.find("not")==0)
				  {
					  APstr.erase(0,4);
					  negated=true;
				  }
				  std::string text_repr=APstr;
				  pos=APstr.find_first_of("!>=<");
				  aux=APstr.substr(0,pos);		
				  trim(aux);
				  size_t var = get_varid(aux);
				  APstr.erase(0,pos);
				  pos=APstr.find_first_not_of("!>=<");
				  aux=APstr.substr(pos,APstr.size()-pos);		
				  trim(aux);
				  APstr.erase(pos,APstr.size()-pos);
				  trim(APstr);

				  size_t tresh = get_treshs(var);
				  real_t tr_val = parseNumber(aux);
				  for (size_t i = 0; i < get_treshs(var); i++)
				  {
					  if (get_tresh(var,i) == tr_val)
						  tresh=i;
				  }

				  if (tresh == get_treshs(var))
				  {
					  std::cerr <<"Treshold " << tr_val
						  		<<" used in atomic proposition [" << fields1.front()
						  		<<"] is not among specified tresholds for variable " << get_varname(var)
						  		<<"." << std::endl;
					  exit(1);
				  }

				  if (APstr.size()==2 && std::string::npos!=APstr.find(">="))
				  {
					  AP.set(var,AFFINE_AP_GREATER_EQUAL,tresh,text_repr);
					  if (dbg) std::cerr<<(negated?" - ":" + ")<<var<<">="<<tresh
										<<"\t["<<fields1.front()<<"]"
										<< std::endl;
				  }
				  if (APstr.size()==2 && std::string::npos!=APstr.find("<="))
				  {
					  AP.set(var,AFFINE_AP_LESS_EQUAL,tresh,text_repr);
					  if (dbg) std::cerr<<(negated?" - ":" + ")<<var<<"<="<<tresh
										<<"\t["<<fields1.front()<<"]"
										<< std::endl;
				  }
				  if (APstr.size()==1 && std::string::npos!=APstr.find(">"))
				  {
					  AP.set(var,AFFINE_AP_GREATER_EQUAL,tresh,text_repr);
					  if (dbg) std::cerr<<(negated?" - ":" + ")<<var<<">="<<tresh
										<<"\t["<<fields1.front()<<"]"
										<< std::endl;
				  }
				  if (APstr.size()==1 && std::string::npos!=APstr.find("<"))
				  {
					  AP.set(var,AFFINE_AP_LESS_EQUAL,tresh,text_repr);
					  if (dbg) std::cerr<<(negated?" - ":" + ")<<var<<"<="<<tresh
										<<"\t["<<fields1.front()<<"]"
										<< std::endl;
				  }

				  if (negated)
					  negativeAP->push_back(AP);	      
				  else
					  positiveAP->push_back(AP);	      		    
				  
				  fields1.pop();
			  }	     	     
		  }
		  trans = new affine_property_transition_t(sname1,sname2,positiveAP,negativeAP);
		  prop->add_transition(trans);
		  fields.pop();
	  }
      set_property_process(prop);      
  }
  // ---- system  ----
  if ((pos=_token.find("system"))!=std::string::npos)
    {
      if ((pos=_token.find("async"))!=std::string::npos)
		{
		  has_system_keyword = true;
		}
    }
}

// method that recomputes intervals of initial conditions (must be called after refineTreshs)
void affine_system_t::update_initials()
{
	interval_t *inits= new interval_t[get_dim()];

	for(int i = 0; i < get_dim(); i++) {

		inits[i].from_real = model.getInitsValuesForVariable(i).first;
		inits[i].to_real = model.getInitsValuesForVariable(i).second;

		for(int t = 0; t < model.getThresholdsForVariable(i).size(); t++) {
			if(inits[i].from_real == model.getThresholdForVarByIndex(i,t))
				inits[i].from = t;

			if(inits[i].to_real == model.getThresholdForVarByIndex(i,t))
				inits[i].to = t;
		}
	}
	initials.push_front(inits);

/*
  const bool dbg = false;

  std::cerr << "Updating initial conditions ..." << endl;

  // for each initial condition in the list
  std::list<interval_t*>::iterator i = initials.begin();
  while ( i != initials.end() )
    {
      // for each dimension
      for ( size_t j = 0; j < get_dim(); j++ )
	{
	  real_t tr_val;
	  size_t tresh;
	  tresh=get_treshs(j);
	  tr_val=(*i)[j].from_real;
	  for (size_t k=0; k<get_treshs(j); k++)
	    {
	      if (get_tresh(j,k)==tr_val)
		tresh=k;
	    }
	  if (tresh==get_treshs(j))
	    {
	      std::cerr <<"Treshold "<<tr_val
			<<" used to define initial values is not among specified"
			<<"tresholds for variable "<<get_varname(j)
			<<"."<<std::endl;
	      exit(1);
	    }
	  (*i)[j].from=tresh;
      
	  tresh=get_treshs(j);
	  tr_val=(*i)[j].to_real;
	  for (size_t k=0; k<get_treshs(j); k++)
	    {
	      if (get_tresh(j,k)==tr_val)
		tresh=k;
	    }
	  if (tresh==get_treshs(j))
	    {
	      std::cerr <<"Treshold "<<tr_val
			<<" used to define initial values is not among specified"
			<<"tresholds for variable "<<get_varname(j)
			<<"."<<std::endl;
	      exit(1);
	    }

	  (*i)[j].to=tresh;
      
	  if (dbg) std::cerr <<"Init interval var "<<get_varname(j)
			 <<"("<<j<<") interval "
			 <<(*i)[j].from<<"-"<<(*i)[j].to<<std::endl;           
	}

      i++;
    }
*/
}


//===================================================================
//===================================================================
// Should return true if the string starts with '#' or "//"
bool affine_system_t::is_comment(std::string _l)
{
  trim(_l);
  if ( (_l[0]=='#') ||
       (_l[0]=='/' && _l[1]=='/')
       )
    return true;
  return false;
}

slong_int_t affine_system_t::read(const char * const fn, bool fast) {
	useFastApproximation = fast;
	return read(fn);
}


slong_int_t affine_system_t::read(const char * const fn)
{
	
  const bool dbg=false;
  if (dbg)
    std::cerr << "Reading file '" << fn << "'" << std::endl;
  
  std::string line;
  std::ifstream modelfile (fn);
  std::ifstream modelfile2 (fn);

  std::string modelline="";

  if (dbg) std::cerr << fn << " is_open: " 
		     << modelfile.is_open() 
		     << modelfile.good() 
		     << modelfile.fail() 
		     << std::endl;

  if (modelfile2.is_open())
  {
    if (dbg) std::cerr << "Parsing file ..." << std::endl;

	
	Parser parser(modelfile2);
    parser.parse();

    model = parser.returnStorage();

    model.RunAbstraction(useFastApproximation);

	inited = 1;
	update_initials();
  }
  else
    {
      error ("affine_system::read","Cannot open requested file.");
      return 1;
    }


  if (modelfile.is_open())
  {
    while (! modelfile.eof() )
    {
      getline (modelfile,line);
      if (is_comment(line)) continue;
      trim(line);
      modelline+= line;
    }
	
	size_t start, end;

	start = modelline.find("process");
	end = modelline.find("system",start);

	std::string part = modelline.substr(start, end - start);
	trim(part);
	parse(part);

	part = modelline.substr(end);
	trim(part);
	parse(part);

    array_of_values = static_cast<real_t*>(malloc(sizeof(real_t)*model.getDims()*4));  //4 = up/down * inside/outside
  }
  else
    {
      error ("affine_system::read","Cannot open requested file.");
      return 1;
    }

  return 0;
}

slong_int_t affine_system_t::read(std::istream & ins)
{
  gerr << "affine_system_t::read not yet implemented" << thr(); //TODO
  return 1; //unreachable;
}

//===================================================================
void affine_system_t::set_dim(size_t _dim)
{
  if (inited==0)
    {
      if (_dim>=BITARRAY_MAX_WIDTH)
        {
          error ("set_dim","Dimension is too high, recompile with larger bitarrays.");
        }
      dim = _dim;
      vars = new vars_t[dim];
      for (size_t i=0; i<dim; i++)
      {
        vars[i].n_sums=0; //zero means undefined equation
        vars[i].n_treshs=0; //zero means uninitiated
      }
      inited=1;
    }
  else
    error("set_dim","Dimension already set.");
}

//===================================================================
size_t affine_system_t::get_dim()
{
  if (inited!=0)
    {
      return model.getDims();
    }
  else
    error("get_dim",1);
  return 0;
}

//===================================================================
void affine_system_t::set_varname(size_t _var, std::string _name)
{
  if (inited==0) error ("set_varname",1);
  vars[_var].name = _name;
}

//===================================================================
std::string affine_system_t::get_varname(size_t _var)
{
  if (inited==0)
    error("get_varname",1);
  if (_var >= model.getDims())
    error ("get_varname",2);
  return model.getVariable(_var);
}

//===================================================================
size_t affine_system_t::get_varid(std::string _name)
{
  if (inited==0) error ("get_varid",1);
  size_t i = model.getVariableIndex(_name);

	if(i == UINT_MAX)
		error ("get_varid","Unknown variable name -- "+ _name + ".");

  return i;
}


//===================================================================
real_t affine_system_t::get_tresh(size_t _var, size_t _tresh_id)
{
  if (inited==0)
    error("get_tresh",1);
  if (_var >= model.getDims() || _tresh_id >= get_treshs(_var))
    error ("get_tresh",2);
  return model.getThresholdForVarByIndex(_var,_tresh_id);
}

//===================================================================
size_t affine_system_t::get_tresh_pos( size_t _var, real_t _thr_value)
{
  if (inited==0)
    error("get_tresh_pos",1);
  if ( _var >= get_dim() )
    error("get_tresh_pos",2);

  size_t retval = 0;
  for ( size_t i = 0; i < get_treshs(_var); i++ )
    if ( get_tresh(_var, i) == _thr_value )
      retval = i;

  return retval;
}

//===================================================================
 size_t affine_system_t::get_treshs(size_t _var)
 {
  if (inited==0)
    error("get_treshs",1);
  if (_var >= model.getDims())
    error ("get_treshs",2);
  return model.getThresholdsForVariable(_var).size();
 }


//===================================================================
void affine_system_t::set_sums(size_t _var, size_t _sums)
{
  if (inited==0)
    error("set_sums",1);
  if (_var>=dim || _sums==0 || vars[_var].n_sums != 0)
    error ("set_sums",2);
  vars[_var].n_sums = _sums;
  vars[_var].sums = new summember_t[_sums];
  memset(vars[_var].sums,0,_sums*(sizeof(summember_t)));
}

//===================================================================
size_t affine_system_t::get_sums(size_t _var)
{
  if (inited == 0)
    error("get_sums",1);
  if (_var >= get_dim())
    error ("get_sums",2);
  return model.getEquationForVariable(_var).size();
}


//===================================================================
void affine_system_t::add_sum(size_t _var, real_t _k, bitarray_t _usedvars)
{
  if (inited==0)
    error("add_sum",1);
  if (_var>=dim || vars[_var].n_sums == 0 || vars[_var].sums==0)
    error ("add_sum",2);

  size_t i=0;
  while (vars[_var].sums[i].k!=0 && i<vars[_var].n_sums) {i++;}
  if (i>=vars[_var].n_sums)
    {
      error("add_sum","More summants specified than declared.");
    }
  else
    {
      vars[_var].sums[i].k=_k;
      vars[_var].sums[i].used_vars=_usedvars;
    }
}

//===================================================================
// returns 0 iff the added threshold already exists
size_t affine_system_t::add_tresh(size_t _var, real_t _tres)
{
  const bool dbg=false;
  size_t pos=0;
  if (inited==0)
    error("add_tres",1);

  if (_var>=dim)
    error ("add_tres",2);

  if (vars[_var].n_treshs == 0)
    {
      vars[_var].n_treshs=1;
      vars[_var].treshs=new real_t[1];
      vars[_var].treshs[0]=_tres;
    }
  else
    {
      //insertsort _tres
      real_t *tmp=new real_t[vars[_var].n_treshs+1];
      while (pos<vars[_var].n_treshs &&
             vars[_var].treshs[pos]<_tres)
        {
          tmp[pos]=vars[_var].treshs[pos];
          pos++;
        }
      
      // now pos points to the position where _tres should be stored
      // NOTE pos may point to out of vars[_var].treshs

      // do not include iff threshold equal to some existing threshold
      if (pos<vars[_var].n_treshs && _tres==vars[_var].treshs[pos])
        {
          std::cerr<<"Warning: Treshold "<<_tres<<" for variable "<<vars[_var].name
                   <<" is specified multiple times. It was not added."<<std::endl;
          return 0;
        }

      // do not include iff threshold is too close to another (up to the num. precision)
      bool lt_cond = false;
      if ( (lt_cond=(pos==0&&(vars[_var].treshs[0] - _tres<precision_thr)))
	   || (pos==vars[_var].n_treshs && (_tres - vars[_var].treshs[pos-1] < precision_thr))
	   || ( (pos>0 && pos<vars[_var].n_treshs && 
		 ((_tres - vars[_var].treshs[pos-1] < precision_thr)
		  || (lt_cond=(vars[_var].treshs[pos] - _tres < precision_thr)))) )
	   )
	{
	  std::cerr<<"Warning: Treshold "<<_tres<<" for variable "<<vars[_var].name
                   <<" too close to threshold "<<(lt_cond?vars[_var].treshs[pos]:vars[_var].treshs[pos-1])<<" to be distinguished up to the precision "<<precision_thr<<". It was not added."<<std::endl;
          return 0;	  
	}

      //here we complete the insertsort by copying the rest of tresholds
      while (pos<vars[_var].n_treshs)
	{
	  tmp[pos]=_tres;
	  _tres=vars[_var].treshs[pos];
	  pos++;	  
	}
      tmp[pos]=_tres;

      delete[] vars[_var].treshs;
      vars[_var].n_treshs++;

      vars[_var].treshs=tmp;
    }

  if (dbg)
    {
      std::cerr<<"   + treshs for var "<<_var<<" = "<<vars[_var].treshs[0];
      pos=1;
      while (pos<vars[_var].n_treshs)
        {
          std::cerr<<", "<<vars[_var].treshs[pos];
          pos++;
        }
      std::cerr<<std::endl;
    }

  return 1;
}


//===================================================================
void affine_system_t::error(const char *method, const char *err) const
{
  std::cerr<<"ERROR in "<<method<<":"<<err<<std::endl;
  exit(1);
}


//===================================================================
void affine_system_t::error(const char *method, std::string err) const
{
  error(method,err.c_str());
}


//===================================================================
void affine_system_t::error(const char *method, const int err_type) const
{
  switch (err_type) {
  case 0: error(method,"Unspecified error."); break;
  case 1: error(method,"Use set_dim() to set the dimension first."); break;
  case 2: error(method,"Index too large, or setting invalid value."); break;
  case 3: error(method,"Memory allocation failed");
  default: error(method,"Unknown error."); break;
  }
}

// =============================================================================

slong_int_t affine_system_t::from_string(const std::string str)
{
 std::istringstream str_stream(str);
 return read(str_stream);
}

void affine_system_t::write(std::ostream & outs)
{
  if (inited == 0) return;
  outs << "VARS: ";
  for (size_t i=0; i<get_dim(); i++)
    {
      outs<<get_varname(i);
      if (i!=get_dim()-1)
		outs<<", ";
    }
  outs<<endl;
  outs<<endl;
  for (size_t i=0; i<get_dim(); i++)
    {
      outs<<"EQ:";
      outs<<get_varname(i);
      outs<<"=";
      for (size_t j=0; j<get_sums(i); j++)
		{
		/*
		  outs<<"("<<vars[i].sums[j].k<<")";		//TODO tento riadok treba prepisat
		  for (size_t k=0; k<get_dim(); k++)
			{
			  if (vars[i].sums[j].used_vars.get(k))		//TODO tento riadok treba prepisat
				{
				  outs<<"*";
				  outs<<get_varname(k);
				}
			}
		  if (j!=get_sums(i)-1)	    
			outs<<" + ";
		*/
		}
      outs<<endl;
    }
  outs<<endl;
  for (size_t i=0; i<get_dim(); i++)
    {
      outs<<"TRES:";
      outs<<get_varname(i);
      outs<<":";
      for (size_t j=0; j<get_treshs(i); j++)
		{
		  outs<<get_tresh(i,j);
		  if (j!=get_treshs(i)-1)	    
			outs<<", ";
		}
      outs<<endl;
    }
  outs<<endl;

  std::list<interval_t*> aux_inits=initials;
  
  while (aux_inits.size()>0)
    {
      interval_t *inits=aux_inits.front();
      aux_inits.pop_front();
      outs<<"INIT: ";
      for (size_t j=0; j<get_dim(); j++)
	{
	  outs<< inits[j].from_real;
	  outs<<":";
	  outs<< inits[j].to_real;
	  if (j!=get_dim()-1) 
	    outs<<",";
	  else
	    outs<<";";
	}
      outs<<endl;
    }
 
  outs<<endl;
  outs<<"SIGNIFICANCE: "
      <<(most_signifficant_only?get_significance_treshold():1)<<endl;
  outs<<"ZERORANGE: "<<zero_range<<endl;
  outs<<"REFINETRESHS: "<<(refinement_iterations)<<endl;
  outs<<endl;
  
  if (get_with_property())
    {     
      outs<<property_process->to_string();
      outs<<endl;
    }

  outs<<"system async";
  if (get_with_property())
    {
      outs<<" property LTL_property";
    }
  outs<<";"<<endl;  
}

bool affine_system_t::write(const char * const filename)
{
  std::ofstream f(filename);
  if (f)
    {
      write(f);
      f.close();
      return true;
    }
  else
    { return false; }
}

std::string affine_system_t::to_string()
{
  std::ostringstream ostr;
  write(ostr);
  return ostr.str();
}

   ///// Methods of the virtual interface of system_t
   ///// dependent on can_property_process():

process_t * affine_system_t::get_property_process()
{
  return property_process;
}

const process_t * affine_system_t::get_property_process() const
{
  return property_process;
}

size_int_t affine_system_t::get_property_gid() const
{
  terr << "affine_system_t::set_property_gid not implemented" << thr();
  property_process->get_gid();
  return 0;
}

void affine_system_t::set_property_gid(const size_int_t gid)
{
  terr << "affine_system_t::set_property_gid not implemented" << thr();
}

void affine_system_t::set_property_process(divine::affine_property_t *_pp)
 {
   set_with_property(true);
   property_process = _pp;
 }

property_type_t affine_system_t::get_property_type()
{
  return BUCHI;
}  


   ///// Methods of the virtual interface of system_t
   ///// dependent on can_processes()

size_int_t affine_system_t::get_process_count() const
{
  terr << "affine_system_t::get_process_count not implemented" << thr();
 return 0; //unreachable
}

process_t * affine_system_t::get_process(const size_int_t gid)
{
  terr << "affine_system_t::get_process not implemented" << thr();
  return 0; //unreachable
}

const process_t * affine_system_t::get_process(const size_int_t id) const
{
  terr << "affine_system_t::get_process not implemented" << thr();
  return 0; //unreachable
}

   ///// Methods of the virtual interface of system_t
   ///// dependent on can_transitions():

transition_t * affine_system_t::get_transition(size_int_t gid)
{
  terr << "affine_system_t::get_transition not implemented" << thr();
  return 0; //unreachable
}

const transition_t * affine_system_t::get_transition(size_int_t gid) const
{
  terr << "affine_system_t::get_transition not implemented" << thr();
  return 0; //unreachable
}

   ///// Methods of the virtual interface of system_t
   ///// dependent on can_be_modified() are here:

void affine_system_t::add_process(process_t * const process)
{
 terr << "affine_system_t::add_process not implemented" << thr();
}

void affine_system_t::remove_process(const size_int_t process_id)
{
  terr << "affine_system_t::remove_process not implemented" << thr();
}

///// END of the implementation of the virtual interface of affine_system_t

