#include <exception>
#include <cassert>
#include <bitset>
#include <sstream>

#include "system/bio/affine_explicit_system.hh"
#include "system/bio/affine_system.hh"
#include "system/state.hh"
#include "common/bit_string.hh"
#include "common/error.hh"
#include "common/deb.hh"

#ifndef DOXYGEN_PROCESSING
using namespace divine;
#endif
using std::ostream;
using std::vector;
using std::cout;

// for given _state_ synchronizes the potential successor state _s_ (_sz_ is size without prop) 
//  with the property process and adds the newly created synchronized state to succs 
//  (duplicate of _s_);
//!!! _s_ is deleted when the procedure is finished
 void affine_explicit_system_t::sync_with_prop(state_t& state, state_t& s, succ_container_t& succs)
  {
    //first we get the current position of the property process
    size_t *_pps=static_cast<size_t*>(static_cast<void*>(state.ptr));
    size_t pp_pos = _pps[get_dim()];
  
    //now we go through all transitions of the property process
    affine_property_t* propproc = 
      dynamic_cast<affine_property_t *>(get_property_process());
    for (size_t ppt=0; ppt < propproc->get_trans_count();ppt++)
      {
        affine_property_transition_t *trans = dynamic_cast<affine_property_transition_t *>
        			( (dynamic_cast<affine_property_t*>(get_property_process()))-> get_transition(ppt) );
      
        //if the transition does not go from the current state we skip it
        size_t tmp_from_id = trans->get_from_id();
        if ( tmp_from_id == pp_pos)  
	  	{
	  	  //we of course consider only enabled transitions
	  	  if (trans->enabled(state))
	  	    {
	  	      //ok, then set properly the value of property process
	  	      size_t target_of_pp=trans->get_to_id();
			  size_t *_ppss=static_cast<size_t*>(static_cast<void*>(s.ptr));
			  size_t backup_value = _ppss[get_dim()];
			  _ppss[get_dim()] = target_of_pp;

			  // we create a new successor state
			  state_t s1=duplicate_state(s);
	  	      //and insert it into the succs container
	  	      succs.push_back(s1);

			  // roll back the original _s_ (this is for safety, might be removed)
			  _ppss[get_dim()] = backup_value;
	  	    }
	  	}		  
      }
    delete_state(s); //all generated states were cloned (from this one)
}

////METHODS of explicit_system:
affine_explicit_system_t::affine_explicit_system_t(error_vector_t & evect):
   system_t(evect), explicit_system_t(evect), affine_system_t(evect)
{
 //empty constructor
}

affine_explicit_system_t::~affine_explicit_system_t()
{
 //empty destructor
}

bool affine_explicit_system_t::is_erroneous(state_t _state)
{
  return false;
}

bool affine_explicit_system_t::is_accepting(state_t _state, size_int_t _acc_group, size_int_t _pair_member)
{
  if (get_with_property())
    {
      size_t aux=*(static_cast<size_t*>(static_cast<void*>(sizeof(size_t)*get_dim()+_state.ptr)));
      return static_cast<affine_property_t*>(get_property_process())->get_accepting(aux);
    }
  return false;
}

property_type_t affine_explicit_system_t::get_property_type()
{
  if (get_with_property())
    return BUCHI;
  else
    return NONE;
}

int affine_explicit_system_t::get_enabled_trans(const state_t state,
              enabled_trans_container_t & enb_trans)
{
 terr << "affine_explicit_system_t::get_enabled_trans not implemented" << thr();
 return 0; //unreachable
}

int affine_explicit_system_t::get_enabled_trans_count(const state_t state,
                                                     size_int_t & count)
{
 terr << "affine_explicit_system_t::get_enabled_trans_count not implemented"
      << thr();
 return 0; //unreachable
}

int affine_explicit_system_t::get_enabled_ith_trans(const state_t state,
                                                   const size_int_t i,
                                                   enabled_trans_t & enb_trans)
{
 terr << "affine_explicit_system_t::get_enabled_ith_trans not implemented"
      << thr();
 return 0; //unreachable
}

bool affine_explicit_system_t::get_enabled_trans_succ
   (const state_t state, const enabled_trans_t & enabled,
    state_t & new_state)
{
 terr << "affine_explicit_system_t::get_enabled_trans_succ not implemented"
      << thr();
 return false;
}

bool affine_explicit_system_t::get_enabled_trans_succs
   (const state_t state, succ_container_t & succs,
    const enabled_trans_container_t & enabled_trans)
{
 terr << "affine_explicit_system_t::get_enabled_trans_succs not implemented"
      << thr();
 return false; //unreachable
}


enabled_trans_t *  affine_explicit_system_t::new_enabled_trans() const
{
 terr << "affine_explicit_system_t::new_enabled_trans not implemented" << thr();
 return 0; //unreachable
}

bool affine_explicit_system_t::eval_expr(const expression_t * const expr,
                        const state_t state,
                        data_t & data) const
{
 terr << "affine_explicit_system_t::eval_expr not implemented" << thr();
 return false; //unreachable
}


void affine_explicit_system_t::print_state(state_t state,
                                          std::ostream & outs)
{
  DBG_print_state(state,outs,0);
}


size_int_t affine_explicit_system_t::get_preallocation_count() const
{
 return 10000;
}

void affine_explicit_system_t::DBG_print_state(state_t _state,
                                          std::ostream & outs,  const ulong_int_t format )
{
  if (inited==0) error ("DBG_print_state",1);
  std::ostringstream retval;
  retval << write_state(_state);
  outs << retval.str();
}

state_t affine_explicit_system_t::get_initial_state()
{
  // A pre-initial state is a state that if encountered in get_succs() function,
  // will make get_succs function generate all initial states according to the
  // INIT definitions in the bio model file.

  //CONDITIONS on pre-initial state:
  // _s[0]=get_treshs(0); 
  // _s[get_dim()] = intial location of property process, if property process is present

  return read_state("[pre-initial]");
}

//recursive function to generate initial states from one convex region
void affine_explicit_system_t::recursive_init(divine::state_t _state, size_t _var, succ_container_t & _succ, interval_t *_ins)
{
  const bool dbg=false;
  if (_var==get_dim()) //recursion ends here
    {
      divine::state_t ss = duplicate_state(_state);
      _succ.push_back(ss);
      if (dbg) std::cerr<<" "<<_succ.size();
      if (dbg) print_state(ss,cerr);
      if (dbg) std::cerr<<std::endl;
      return;
    }

  for (size_t tres_id = _ins[_var].from; tres_id < _ins[_var].to; tres_id++)
    {
      size_t *_s=static_cast<size_t*>(static_cast<void*>(_state.ptr));
      _s[_var]=tres_id;
      recursive_init(_state, _var+1, _succ, _ins);
    }  
}


void affine_explicit_system_t::print_aov()
{
  for (size_t k=0; k<4*get_dim(); k++)
    {
      cerr<<array_of_values[k]
	  <<(k==4*get_dim()-1?"":", ");
    }
  cerr<<endl;
}

int affine_explicit_system_t::get_succs(divine::state_t _state, succ_container_t & succs)
{
  const bool dbg=false;
  const bool avg=true;   // computing most_significant by average (true) or sum (false)
  succs.clear();

  if (inited==0) error ("get_succs",1);

  size_t *_s=static_cast<size_t*>(static_cast<void*>(_state.ptr));
  if (_s[0]==get_treshs(0)) //this is the pre-initial state, we have to generate all initial states
    {
      state_t ss = duplicate_state(_state);      
      // we have to remember initials in order to repeatedly generate successors
      // of the preinitial state!!
      std::list<interval_t*> auxlist=initials;
      if (auxlist.empty())
		{
		  std::cout<<"No initial conditions were given, use INIT: to specify some."<<std::endl;
		  exit (1);
		}
      // initial states are specified as a union of convex regions
      while (!auxlist.empty()) // here we iterate over convex regions
		{
		  //and we generate all states belonging to the currently processed convex region
		  recursive_init(ss,0,succs,auxlist.front());
		  auxlist.pop_front();
		}
      delete_state(ss);
      return 0;
    }

  // initialize array of value if needed
  if ( most_signifficant_only || selfloops )
    {
      if ( array_of_values == 0 )
		array_of_values = new real_t[4*get_dim()];
    
      else
		{
		  	  for ( size_t i = 0; i<4*get_dim(); i++ )
		  	    array_of_values[i] = 0;
		}
    }

  if (most_signifficant_only)
    {
      // =======================================================================
      // Array of values is used to keep sizes of vectors representing possible
      // transitions from a multidimensional rectangle. Given one dimension,
      // there are two faces of the rectangle that are vertical (orthogonal) to
      // the selected dimension, the lower face and the upper face. Futhermore,
      // the normal vector originating on one of these two faces may either aim
      // inwards the rectangle or outwards it.
      //
      // The logical structure of the array is as follows:
      //
      //   suppose dimension x \in [0,#dimensions-1]
      //
      //   array_of_values[x+0] = upper face outwards (positive direction)
      //   array_of_values[x+1] = upper face inwards  (negative direction)
      //   array_of_values[x+2] = lower face inwards  (positive direction)
      //   array_of_values[x+3] = lower face outwards (negative direction)
      //
      // =======================================================================

      // memset(array_of_values,0,4*sizeof(real_t)*get_dim());
      get_succs1(_state, succs, true);

      if (dbg) cerr<<"AFTER get_succs1()"<<endl;
      if (dbg) print_aov();

      // whereMax keeps info about which transition is the most significant one
      // if whereMax == 4*get_dim() then no transition were chosen yet
      size_t whereMax=4*get_dim();
      real_t max=0;     // maximal outgoing transition magnitude

      // if we weight the overall edge by number of vertices or not 
      size_t divide_by=(avg?get_dim():1);
      if (dbg) std::cerr << "Computing most significant by " << (avg?"average":"sum") 
			 << " ..." << endl;
      for ( size_t i = 0; i < 4*get_dim(); i+=4 )
		{	  
		  if (whereMax==4*get_dim() || 
			  max < ( array_of_values[i]/divide_by)	      
			  )
			{
			  whereMax=i;
			  max = array_of_values[i]/divide_by;
			}
		  if (whereMax==4*get_dim() || 
			  max < ( array_of_values[i+3]/divide_by)
			  )
			{
			  whereMax=i+3;
			  max = array_of_values[i+3]/divide_by;
			}
		}

      // maximal relativized transition magnitude
      real_t relmax=0;  

      // computed only when randomize == 2
      if ( randomize == 2 ) 
		{
		  size_t* _aux=static_cast<size_t*>(static_cast<void*>(_state.ptr));
		  size_t whereMax=4*get_dim();

		  for ( size_t i = 0; i < 4*get_dim(); i+=4 )
			{	  
			  size_t _var = (i-(i%4)) / 4; 

			  if (whereMax==4*get_dim() || 
			  	//relmax < ( array_of_values[i]/(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]) )
			  	relmax < ( array_of_values[i]/(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var])) )	      	      
			  )
				{
				  whereMax=i;
				  //relmax = array_of_values[i]/(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]);
				  relmax = array_of_values[i]/(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var]));
				}
			  if (whereMax==4*get_dim() || 
			  	//relmax < ( array_of_values[i+3]/(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]) )
			  	relmax < ( array_of_values[i+3]/(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var])))
			  )
				{
				  whereMax=i+3;
				  //relmax = array_of_values[i+3]/(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]);
				  relmax = array_of_values[i+3]/(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var]));
				}
			}
		}

      if (dbg) cerr << "max set to " << max << " at "<< whereMax << endl;
      
      // if randomization is turned on we throw the coin to decide whether to employ 
      // the randomization step (this leads to nondeterministic behaviour!)
      if ( randomize == 1 ) {  // most significant with randomized slow transitions
	
		real_t probability = 1;
		size_t int_prob = 1;
		bool chosen = false;

		for ( size_t i = 0; i < 4*get_dim(); i+=4 )
		  {
			if ( max*significance > array_of_values[i]/divide_by &&
			 (array_of_values[i]/divide_by) > 0 )
			  {
				probability = (array_of_values[i]/divide_by)/max;
				probability *= maxprob;
				probability *= 1023;	      
				int_prob = floor(probability);
				if ( int_prob == 0 )
				  int_prob++;

				// we throw the coin with threshold given by probability
				// and the seed given by the direction
				// if not chosen, the transition is nulled
				if ( /*not*/ ! coin(_state, int_prob, i) )
				  chosen = true;
			 	else
				  array_of_values[i]=0;
			  }
			if ( max*significance > array_of_values[i+3]/divide_by && 
			 (array_of_values[i+3]/divide_by) > 0 )
			  {
				probability = (array_of_values[i+3]/divide_by)/max;
				probability *= maxprob;
				probability *= 1023;
		        int_prob = floor(probability);
		        if ( int_prob == 0 )
		          int_prob++;

				if ( /*not*/ ! coin(_state, int_prob, i+3) )
			 	  chosen = true;
				else
				  array_of_values[i+3]=0;
			  }

			// the inside-pointing directions are not used
			array_of_values[i+1]=0;
			array_of_values[i+2]=0;
		  }
	
      }
      else if ( randomize == 2 ) {  // most significant with relativized randomized 
	                            // slow transitions	
		real_t probability = 1;
		size_t int_prob = 1;
		bool chosen = false;

		size_t* _aux=static_cast<size_t*>(static_cast<void*>(_state.ptr));

		for ( size_t i = 0; i < 4*get_dim(); i+=4 )
		  {
			size_t _var = (i-(i%4)) / 4; 
			// if the transition is not a significant one
			if ( max*significance > array_of_values[i]/divide_by &&
			 (array_of_values[i]/divide_by) > 0 )
			  {
				//probability = array_of_values[i]/(relmax*(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]));
				probability = array_of_values[i] / 
						(relmax*(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var])));
				probability *= maxprob;
				probability *= 1023;	      
				int_prob = floor(probability);
				if ( int_prob == 0 )
				  int_prob++;

				// we throw the coin with threshold given by probability
				// and the seed given by the direction
				// if not chosen, the transition is nulled
				if ( /*not*/ ! coin(_state, int_prob, i) )
				  chosen = true;
			 	else
				  array_of_values[i]=0;
			  }
			if ( max*significance > array_of_values[i+3]/divide_by && 
			 (array_of_values[i+3]/divide_by) > 0 )
			  {
				//probability = array_of_values[i+3]/(relmax*(vars[_var].treshs[_aux[_var]+1]-vars[_var].treshs[_aux[_var]]));
				probability = array_of_values[i+3] / 
						(relmax*(model.getThresholdForVarByIndex(_var, _aux[_var]+1) - model.getThresholdForVarByIndex(_var, _aux[_var])));
				probability *= maxprob;
				probability *= 1023;
		        int_prob = floor(probability);
		        if ( int_prob == 0 )
		          int_prob++;

				if ( /*not*/ ! coin(_state, int_prob, i+3) )
			 	  chosen = true;
				else
				  array_of_values[i+3]=0;
			  }


			// the inside-pointing directions are not used
			array_of_values[i+1]=0;
			array_of_values[i+2]=0;
		  }

      }
      else // direct most significant
		{
		  for ( size_t i = 0; i < 4*get_dim(); i+=4 )
			{
			  if ( max*significance > array_of_values[i]/divide_by )
				{
				  array_of_values[i]=0;
				}
			  if ( max*significance > array_of_values[i+3]/divide_by )
				{
				  array_of_values[i+3]=0;
				}
			  array_of_values[i+1]=0;
			  array_of_values[i+2]=0;
			}
		}		 	
		
      // clean all succs besides selfloop
      bool selfloop_present = false;
      for (succ_container_t::iterator i=succs.begin(); i!=succs.end(); i++)
		{
		  state_t _s = *i;
		  // DBG_print_state(_s, cerr << endl << "REMOVED STATE ... ", 0);
		  if ( _s == _state )
			selfloop_present = true;
		  else
			delete_state(_s);
		  
		}
      succs.clear();
      if ( selfloop_present ) {   // get back the selfloop when needed
		state_t _s = duplicate_state(_state);
        if (get_with_property())
          sync_with_prop(_state, _s, succs);
        else
          succs.push_back(_s);
      }

      for ( size_t i = 0; i < 4*get_dim(); i+=4 )
		{
		  
		  //UP direction
		  if ( array_of_values[i] >0 )
			{	      
			  state_t _s = duplicate_state(_state);
			  size_t* _aux=static_cast<size_t*>(static_cast<void*>(_s.ptr));
			  size_t _var= (i-(i%4)) / 4; 
			  if (_aux[_var] < get_treshs(_var)-2)
				_aux[_var]++;

			  if (get_with_property())
		    	sync_with_prop(_state, _s, succs);
			  else
		    	succs.push_back(_s);

			  // cerr << "up direction tendency = " <<  array_of_values[i] << endl;
			  // DBG_print_state(_s, cerr << endl << "ADDED STATE ... ", 0);
			}		
		  
		  //DOWN direction
		  if ( array_of_values[i+3] >0 )
			{
			  state_t _s = duplicate_state(_state);
			  size_t* _aux=static_cast<size_t*>(static_cast<void*>(_s.ptr));
			  size_t _var= (i-(i%4)) / 4; 
			  if (_aux[_var] > 0)
				_aux[_var]--;

			  if (get_with_property())
		    	sync_with_prop(_state, _s, succs);
			  else
		    	succs.push_back(_s);

			  // cerr << "down direction tendency = " << array_of_values[i+3] << endl;
			  // DBG_print_state(_s, cerr << endl << "ADDED STATE ... ", 0);
			}	
		}          
  
      int result = 0;
      if (succs.size()==0 && get_with_property()==false)
		result |= SUCC_DEADLOCK;
      
      // cerr << "SUCCS GENERATION FINISHED" << endl;      
      // cerr << "succs: " << succs.size() << " " << succs[0].size << endl;
      
      return result;
    }
  else
    {
      return get_succs1(_state, succs, selfloops);
    }
  
}

// this method generates successors of _state to succs
// when with_most_significant=true, recursive is called to traverse all vertices of _state and computes sums of values
// when with_most_significant=false, recursive is called with  "short-circuit execution"
// note: the positive feature is also needed for neglecting selfloops
int affine_explicit_system_t::get_succs1(divine::state_t _state, succ_container_t & succs, bool with_most_significant)
{
  const bool dbg=false;

  size_t *_s=static_cast<size_t*>(static_cast<void*>(_state.ptr));

  if (dbg) std::cerr << "Generating succs for " << write_state(_state) << endl;

  bool enabled=false;
  for (size_t m_d = 0; m_d < get_dim(); m_d++)
    {
      if (dbg) std::cerr<<"Checking in dimension:"<<m_d<<std::endl;
      if (_s[m_d]>0) // has a successor below
        {
          if (dbg) std::cerr<<"  - below"<<std::endl;
		  if (dbg) {cerr<<"   ";print_aov(); }
          recursive(&enabled,m_d,false,_s,0,(with_most_significant ? array_of_values : NULL));
		  if (dbg) {cerr<<"   ";print_aov(); }
          if (enabled)
            {
              _s[m_d]--;
		      state_t ss = duplicate_state(_state);
              _s[m_d]++;
			  if (get_with_property())
				sync_with_prop(_state, ss, succs);
			  else
				succs.push_back(ss);
            }
          enabled=false;
        }

      if (_s[m_d] < (/*vars[m_d].n_treshs*/get_treshs(m_d) - 2)) // has a successor above
        {
          _s[m_d]++;
          if (dbg) std::cerr<<"  - above"<<std::endl;

          //SVEN Debug
          /*
          if (_s[0] == 0 && _s[1] == 2 && _s[2] == 0 && _s[3] == 3 && m_d == 1) {
            cerr << "Recursive XXX: " << with_most_significant << endl;
            recursive(&enabled,m_d,true,_s,0,(with_most_significant ? array_of_values : NULL),true);
          }
          else
          */
          //SVEN Debug
          recursive(&enabled,m_d,true,_s,0,(with_most_significant ? array_of_values : NULL));
          
          if (enabled)
            {
			  if (get_with_property())
				{
				  state_t ss = duplicate_state(_state);
				  sync_with_prop(_state, ss, succs);
				}
			  else	      
			succs.push_back(duplicate_state(_state));
            }
          _s[m_d]--;
          enabled=false;
        }
    }

    //Sven DEBUG
    /*size_t* _tmp=static_cast<size_t*>(static_cast<void*>(_state.ptr));
    if (_tmp[0] == 0 && _tmp[1] == 2 && _tmp[2] == 0 && _tmp[3] == 2) {
      cerr << "Debug 0.2.0.2" << endl;
      for ( size_t i = 0; i < 4*get_dim(); i+=4 ) {
        cerr << array_of_values[i+0] << ", "
              << array_of_values[i+1] << ", "
              << array_of_values[i+2] << ", "
              << array_of_values[i+3] << endl;
      }
      cerr << "Debug 0.2.0.2" << endl;
    }*/
    //else cerr << "State: " << _tmp[0] << "." << _tmp[1] << "." << _tmp[2] << "." << _tmp[3] << endl;
    //Sven DEBUG

  // solve selfloop
  if ( selfloops )  // array_of_values allocated
    {
        if ( dbg ) std::cerr << "Solving self-loop..." << endl;

        // decide whether selfloop can be omitted
        bool _selfloop_omit = false;
        size_t k = 0;
        while ( /*not*/ ! _selfloop_omit && k < 4*get_dim() )
	  	{
	  	  if ( (array_of_values[k]+array_of_values[k+2]==0 && array_of_values[k+3]*array_of_values[k+1]>0) 
	  	       || (array_of_values[k]*array_of_values[k+2]>0 && array_of_values[k+3]+array_of_values[k+1]==0) )
	  	    _selfloop_omit = true;

	  	  k+=4;		  
	  	}
      
        if ( /*not*/ ! _selfloop_omit )
	  	{
	  	  // add selfloop
		      if (get_with_property())
		        {
		          state_t _ss = duplicate_state(_state);
		          sync_with_prop(_state, _ss, succs);
		        }
		        else
		        {
		          state_t _ss = duplicate_state(_state);
		          succs.push_back(_ss);
		        }
		  
	  	  if ( dbg ) {
	  	    std::cerr << " selfloop added to ";
	  	    DBG_print_state(_state, cerr, 0);
	  	    std::cerr << endl;
	  	  }
	  	}
    }  
  
  if (dbg) std::cerr << "getsuccs1: Number of successors: " << succs.size() << endl;

  int result = 0;
  if (succs.size()==0 && get_with_property()==false)
    result |= SUCC_DEADLOCK;

  // cerr << "SUCCS GENERATION FINISHED" << endl;
  
  // cerr << "succs: " << succs.size() << " " << succs[0].size << endl;

  return result;
}

int affine_explicit_system_t::get_ith_succ(state_t state, const int i, state_t & succ)
{
  std::cerr<<"NOT IMPLEMENTED"<<std::endl;
  return -1;
}

int affine_explicit_system_t::get_succs(state_t state, succ_container_t & succs,
               enabled_trans_container_t & etc)
{
  std::cerr<<"NOT IMPLEMENTED"<<std::endl;
  return -1;
}

size_t affine_explicit_system_t::random(divine::state_t s,divine::size_int_t seed) const
{
  return hashf.hash_state(s,seed);
}

bool affine_explicit_system_t::coin(divine::state_t s, size_t treshold, divine::size_int_t seed) const
{
  if (treshold > 1023 || treshold ==0)
    error("affine_explicit_treshold::coin","invalid treshold specified (treshold must be in (0,1023] )");
  if ( (random(s,seed)&1023) > treshold) return true;
  return false;
}
