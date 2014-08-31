// {{{ include & namespace

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include "divine.h"
#include "distr_map.hh"

using namespace divine;
using namespace std;

// }}}

// {{{ constants

const int debug = 0;

const int TAG_SEND_STATE = DIVINE_TAG_USER + 0;
const int TAG_SEND_CYCLE_DETECT = DIVINE_TAG_USER + 1; //to send to Master if someone detect an accepting state lying on a cycle
const int TAG_CYCLE_DETECT = DIVINE_TAG_USER + 2; //to send to all workstations from which accepting state will be rediscovered existing accepting cycle
const int TAG_YOU_OWN_ACC_CYCLE = DIVINE_TAG_USER + 3;
const int TAG_CYCLE_RECOVERING = DIVINE_TAG_USER + 4;
const int TAG_CYCLE_RECOVERED = DIVINE_TAG_USER + 5;
const int TAG_CYCLE_RECONSTRUCTION = DIVINE_TAG_USER + 6;
const int TAG_CYCLE_RECONSTRUCTED = DIVINE_TAG_USER + 7;
const int TAG_SEND_STATE_ON_CE_CYCLE = DIVINE_TAG_USER + 8;
const int TAG_PATH_RECOVERING = DIVINE_TAG_USER + 9;
const int TAG_PATH_RECOVERED = DIVINE_TAG_USER + 10;
const int TAG_PATH_RECONSTRUCTION = DIVINE_TAG_USER + 11;
const int TAG_PATH_RECONSTRUCTED = DIVINE_TAG_USER + 12;
const int TAG_SEND_STATE_ON_CE_PATH = DIVINE_TAG_USER + 13;
const int TAG_PRINT = DIVINE_TAG_USER + 14;

// }}}

// {{{ structures

struct appendix_t 
{ map_value_t act_map; //actual value of function map && "pointer" to predecessor during counterexample reconstruction ("global_state_ref_t")
  map_value_t old_map; // value of function map from the previous iteration && auxiliary value during counterexample reconstruction
  list<state_ref_t>::iterator shrinkA_ptr;
} appendix;

struct info_shrinkA_t
{ int size_of_all_shrinkA_sets;
  void update(void);
};

struct info_state_space_t {
  long int all_states;
  unsigned long all_succ_calls;
  unsigned long all_edges_relaxed;
  long int all_mem;
  void update();
};

// }}}

// {{{ declaration of variables

explicit_system_t * p_sys;
explicit_storage_t st;
distributed_t distributed;
distr_reporter_t reporter(&distributed);
logger_t logger;

updateable_info_t<info_state_space_t> info_state_space;
updateable_info_t<info_shrinkA_t> info_shrinkA;


size_int_t nnn; //number of collaborating workstations
string ids,file_name;

char * input_file_ext = 0;
string file_base_name;


bool trail=false,show_ce=false,report=false;
bool statistics=false;
bool quietmode=false;
bool logging = false;
bool base_name = false;
bool fastApproximation = false;
string set_base_name;

size_int_t htsize=0;

size_int_t iter_count = 0;
unsigned long succs_calls = 0, edges_relaxed = 0, trans = 0, cross_trans = 0;
bool acc_cycle_found = false, finish = false, end_of_iteration = false;
bool acc_cycle_is_mine = false; //if I am owner of accepting state lying on a cycle
bool token = false; //process I a state during ce-reconstruction?
message_t message;
message_t received_message;

size_int_t cycle_length, path_length;
state_t *cycle_state, *path_state;

state_ref_t state_ref, acc_cycle_ref; //reference to the accepting state 
                                      //lying on a cycle
state_t current_state, s0;

queue<state_ref_t> waiting;
list<state_ref_t> shrinkA;

timeinfo_t timer;
vminfo_t vm;

// }}}

// {{{ declaration of some functions

void RELAX(state_t state, map_value_t propag, map_value_t subgraph);
void RELAX_PATH(state_t state, map_value_t propag, size_int_t depth);
void RELAX_CYCLE(state_t state, map_value_t propag, size_int_t depth);
void process_ce_cycle_state(map_value_t _ref, size_int_t depth);
void process_ce_path_state(map_value_t _ref, size_int_t depth);

// }}}

// {{{ version & usage & auxiliary functions

void version()
{
  cout <<"version 1.1 build 5 (2006/09/20 17:00)"<<endl;
}

void usage()
{
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"Distributed Maximal Accepting Predecessors" << endl;
  version();
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"Usage: [mpirun -np N] distr_map [options] file.dve"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v, --version \t show distr_map version"<<endl;
  cout <<" -h, --help \t show this help"<<endl;
  cout <<" -r, --report \t produce report (file.distr_map.report)"<<endl;
  cout <<" -t, --trail \t produce trail file (file.distr_map.trail)"<<endl;
  cout <<" -f, --fast \t use faster but less accurate algorithm for abstraction" << endl;
  cout <<" -c, --statelist \t show counterexample states"<<endl;
  cout <<" -H x, --htsize x \t set the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -V, --verbose \t print some statistics"<<endl;
  cout <<" -q, --quiet \t quiet mode (do not print anything "
       <<"- overrides all except -h and -v)"<<endl;
  cout <<" -L, --log \t perform logging"<<endl;
  cout <<" -X w \t sets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
}

void vypis_state_ref(state_ref_t ref, std::ostream &outs)
{ 
  outs << '[' << nid << ',' << ref.hres << ',' << ref.id << ']';
}

// }}}

// {{{ broadcast_message, send_state, process_messages, etc.

void info_state_space_t::update()
{
  if (nid == 0)
    {
      all_states = st.get_states_stored();
      all_succ_calls = succs_calls;
      all_edges_relaxed = edges_relaxed;
      all_mem = vm.getvmsize();
    }
  else
    {
      all_states += st.get_states_stored();
      all_succ_calls += succs_calls;
      all_edges_relaxed += edges_relaxed;
      all_mem += vm.getvmsize();
    }
}

void info_shrinkA_t::update(void)
{ 
  if (nid == 0)
  { size_of_all_shrinkA_sets = shrinkA.size();
  }
  else
  { size_of_all_shrinkA_sets += shrinkA.size();
  }
}


void send_cycle_detect(state_ref_t *state_ref)
{ if (debug>1)
    cerr << nid << " -> 0: " << TAG_SEND_CYCLE_DETECT << endl;
  message.rewind();
  message.append_state_ref(*state_ref);
  distributed.network.send_urgent_message(message,
                                     0,
                                     TAG_SEND_CYCLE_DETECT);
}


void send_state(int dest, state_t *state, map_value_t *propag, 
		map_value_t *old_map)
{ if (debug>1)
    cerr << nid << " -> " << dest << ": " << TAG_SEND_STATE << endl;
  message.rewind();
  message.append_state(*state);
  message.append_size_int(propag->bfs_order);
  message.append_size_int(propag->nid);
  message.append_size_int(propag->id);
  message.append_size_int(old_map->bfs_order);
  message.append_size_int(old_map->nid);
  message.append_size_int(old_map->id);

  distributed.network.send_message(message,
                              dest,
			      TAG_SEND_STATE);  
}

void send_ce_recovering(int dest, state_t *state, map_value_t *propag, 
			size_int_t *depth, const int tag)
{ if (debug>1)
    cerr << nid << " -> " << dest << ": " << tag << endl;
  message.rewind();
  message.append_state(*state);
  message.append_size_int(propag->bfs_order);
  message.append_size_int(propag->nid);
  message.append_size_int(propag->id);
  message.append_size_int(*depth);
  
  distributed.network.send_message(message,
                              dest,
                              tag);  
}

void broadcast_cycle(void)
{
  message.rewind();
  message.append_state_ref(acc_cycle_ref);
  for (size_int_t i = 0; i < nnn; i++)
    {
      if (i != nid)
        {
          distributed.network.send_urgent_message(message,
					     i,
					     TAG_CYCLE_DETECT);
        }
    }
}

void process_message(char *buf, int size, int src, int tag, bool urgent)
{
  //  cout << nid << " proc_mess: " << tag-DIVINE_TAG_USER << ' ' << src << endl;
  state_t recv_state;
  switch(tag) 
    { 
    case TAG_SEND_STATE:
      {
	if (!acc_cycle_found)
	  { 
	    map_value_t propag, old_map;
            received_message.load_data((byte_t*)(buf),size);
            received_message.read_state(recv_state);
            received_message.read_size_int(propag.bfs_order);
            received_message.read_size_int(propag.nid);
            received_message.read_size_int(propag.id);
            received_message.read_size_int(old_map.bfs_order);
            received_message.read_size_int(old_map.nid);
            received_message.read_size_int(old_map.id);

	    RELAX(recv_state, propag, old_map);
	    delete_state(recv_state);
	    if (!(waiting.empty())) //I will something process, I will be busy
	      distributed.set_busy();
	  }
	break;
      }
    case TAG_CYCLE_DETECT:
      { 
	received_message.load_data((byte_t*)(buf),size);
	received_message.read_state_ref(acc_cycle_ref);
	finish=acc_cycle_found/*=end_of_iteration*/=true;
	while (!(waiting.empty()))
	  waiting.pop();
	if (debug>1)
	  cerr << nid << ": I can finish." << endl; 
	break;
      }
    case TAG_YOU_OWN_ACC_CYCLE:
      { 
	acc_cycle_is_mine=true;
	break;
      }
    case TAG_SEND_CYCLE_DETECT: //can receive only Master
      { 
	if (!acc_cycle_found) //only the first incoming acc.state will be taken
	  { 
	    received_message.load_data((byte_t*)(buf),size);
            received_message.read_state_ref(acc_cycle_ref);
	    distributed.network.send_urgent_message(0,
                                     0,
                                     src,
                                     TAG_YOU_OWN_ACC_CYCLE);
	    cout << "======================" << endl;
	    cout << "Accepting cycle found!" << endl;
	    cout << "======================" << endl;
	    finish=acc_cycle_found/*=end_of_iteration*/=true;
	    while (!(waiting.empty()))
	      waiting.pop();	    
	    broadcast_cycle();
	  }
	break;
      }
    case TAG_CYCLE_RECOVERING:
      {
	if (!end_of_iteration)
	  { 
	    map_value_t propag;
	    size_int_t depth;
            received_message.load_data((byte_t*)(buf),size);
            received_message.read_state(recv_state);
            received_message.read_size_int(propag.bfs_order);
            received_message.read_size_int(propag.nid);
            received_message.read_size_int(propag.id);
            received_message.read_size_int(depth);

	    RELAX_CYCLE(recv_state, propag, depth);
	    delete_state(recv_state);
	    if (!(waiting.empty())) //I will something process, I will be busy
		distributed.set_busy();
	  }
	break;
      }
    case TAG_CYCLE_RECOVERED:
      {
	if ((nid==0)&&(cycle_length==0))
	  { 
	    received_message.load_data((byte_t*)(buf),size);
            received_message.read_size_int(cycle_length);
	    cycle_length++;
	    cycle_state = new state_t[cycle_length];
	    //	    cout << "cycle_length: " << cycle_length << endl;
	    message.rewind();
	    message.append_size_int(cycle_length);
	    for (size_int_t i = 1; i < nnn; i++)
	      {
		distributed.network.send_urgent_message(message, 
						   i,
						   TAG_CYCLE_RECOVERED);
	      }
	  }
	end_of_iteration = true; //it stands for end_of_cycle_recovering now
	while (!(waiting.empty()))
	  waiting.pop();
	break;
      }
    case TAG_CYCLE_RECONSTRUCTION:
      { 
	map_value_t _ref;
	size_int_t depth;
	received_message.load_data((byte_t*)(buf),size);
	received_message.read_size_int(_ref.bfs_order);
	received_message.read_size_int(_ref.nid);
	received_message.read_size_int(_ref.id);
	received_message.read_size_int(depth);
	process_ce_cycle_state(_ref, depth);
	break;
      }
    case TAG_CYCLE_RECONSTRUCTED:
      {
	end_of_iteration = true;//it stands for end_of_cycle_reconstruction now
	break;
      }
    case TAG_PATH_RECOVERING:
      {
	if (!end_of_iteration)
	  { 
	    map_value_t propag;
	    size_int_t depth;
            received_message.load_data((byte_t*)(buf),size);
            received_message.read_state(recv_state);
            received_message.read_size_int(propag.bfs_order);
            received_message.read_size_int(propag.nid);
            received_message.read_size_int(propag.id);
            received_message.read_size_int(depth);

	    RELAX_PATH(recv_state, propag, depth);
	    delete_state(recv_state);
	    if (!(waiting.empty())) //I will something process, I will be busy
		distributed.set_busy();
	  }
	break;
      }
    case TAG_PATH_RECOVERED:
      {
	if ((nid==0)&&(path_length==0))
	  {
	    received_message.load_data((byte_t*)(buf),size);
	    received_message.read_size_int(path_length);
	    path_length++;
	    path_state = new state_t[path_length];
	    //	    cout << "path_length: " << path_length << endl;
	    message.rewind();
	    message.append_size_int(path_length);
	    for (size_int_t i = 1; i < nnn; i++)
	      {
		distributed.network.send_urgent_message(message, 
						   i,
						   TAG_PATH_RECOVERED);
	      }
	  }
	end_of_iteration = true; //it stands for end_of_path_recovering now
	while (!(waiting.empty()))
	  waiting.pop();
	break;
      }
    case TAG_PATH_RECONSTRUCTION:
      { 
	map_value_t _ref;
	size_int_t depth;
	received_message.load_data((byte_t*)(buf),size);
	received_message.read_size_int(_ref.bfs_order);
	received_message.read_size_int(_ref.nid);
	received_message.read_size_int(_ref.id);
	received_message.read_size_int(depth);
	process_ce_path_state(_ref, depth);
	break;
      }
    case TAG_PATH_RECONSTRUCTED:
      {
	end_of_iteration = true;//it stands for end_of_path_reconstruction now
	break;
      }
    case TAG_SEND_STATE_ON_CE_CYCLE: //can receive only Master
      { 
	size_int_t depth;
        received_message.load_data((byte_t*)(buf),size);
        received_message.read_size_int(depth);
	//	cout << "cycle " << depth << endl;
        received_message.read_state(cycle_state[depth]);
	break;
      }
    case TAG_SEND_STATE_ON_CE_PATH: //can receive only Master
      { 
	size_int_t depth;
        received_message.load_data((byte_t*)(buf),size);
        received_message.read_size_int(depth);
	//	cout << "path " << depth << endl;
        received_message.read_state(path_state[depth]);
	break;
      }
    }
}

// }}}

// {{{ void RELAX(state_t state, map_value_t propag, map_value_t subgraph)

void RELAX(state_t state, map_value_t propag, map_value_t subgraph)
{
  appendix_t app;
  if (!(st.is_stored(state, state_ref)))
  { st.insert(state, state_ref);
    app.old_map = NULL_MAP_VISIT;
    app.old_map.nid = divine::MAX_ULONG_INT; //to detect whether a state is handled for the first time in MAP() in order to count (cross) trans correctly
    if (p_sys->is_accepting(state))
      { 
	app.act_map=prirad_novy_map(&(state_ref));
 	if (cmp_map(propag, app.act_map))
	  {
	    shrinkA.push_front(state_ref);
	    app.shrinkA_ptr = shrinkA.begin();
	  } 
	else 
	  { 
	    app.act_map=propag; //due to execution of this line the next iteration 
	    app.shrinkA_ptr = list<state_ref_t>::iterator(0); //is necessary	    
 	  }
      }
    else 
      {
	app.act_map = propag;
	app.shrinkA_ptr = list<state_ref_t>::iterator(0);
      }
    waiting.push(state_ref);
    st.set_app_by_ref(state_ref, app);
  }
  else //state already visited
    {
      st.get_app_by_ref(state_ref, app);
      if ((propag == app.act_map) && (app.shrinkA_ptr != list<state_ref_t>::iterator(0)))
	{
	  //accepting cycle revealed
	  send_cycle_detect(&state_ref);
	}
      else
	{
	  if (app.act_map == subgraph) //first visitation of this vertex during
	    // this iteration
	    {
	      app.old_map = subgraph;
	      if (p_sys->is_accepting(state))
		{ 
		  app.act_map=prirad_novy_map(&(state_ref));
		  if (cmp_map(propag, app.act_map))
		    {
		      shrinkA.push_front(state_ref);
		      app.shrinkA_ptr = shrinkA.begin();
		    }
		  else
		    {
		      app.act_map=propag; //due to execution of this line the next iteration 
		      app.shrinkA_ptr = list<state_ref_t>::iterator(0); //is necessary
		    }
		}
	      else 
		{
		  app.act_map = propag;
		  app.shrinkA_ptr = list<state_ref_t>::iterator(0);
		}
	      waiting.push(state_ref);
	      st.set_app_by_ref(state_ref, app);
	    }
	  else
	    {
	      if (((app.old_map == subgraph)||((app.old_map.nid==divine::MAX_ULONG_INT) && (subgraph==NULL_MAP_VISIT))) && (cmp_map(app.act_map,propag)))
		{                           //this (after "||") is necessary due to counting of states+transitions
		  app.act_map = propag;
		  if (app.shrinkA_ptr != list<state_ref_t>::iterator(0))
		    {
		      shrinkA.erase(app.shrinkA_ptr);
		      app.shrinkA_ptr = list<state_ref_t>::iterator(0);
		    }
		  waiting.push(state_ref);
		  st.set_app_by_ref(state_ref, app);
		}
	    }
	}
    }
}

// }}}

// {{{ void RELAX_CYCLE(state_t state, map_value_t propag, size_int_t depth)

void RELAX_CYCLE(state_t state, map_value_t propag, size_int_t depth)
{
  appendix_t app;
  depth++;
  if (st.is_stored(state, state_ref))
    { 
      st.get_app_by_ref(state_ref, app);
      if ((state_ref==acc_cycle_ref)&&(acc_cycle_is_mine))
	{
	  app.old_map.bfs_order=depth;
	  app.act_map=propag;
	  end_of_iteration = true;
	  message.rewind();
	  message.append_size_int(depth);
	  distributed.network.send_urgent_message(message, 
					     0,
					     TAG_CYCLE_RECOVERED);
	}
      else      
	if (app.old_map.id!=EXTRACT_CYCLE_VISIT)
	  {
	    app.old_map.bfs_order=depth;
	    app.act_map=propag;
	    app.old_map.nid=nid;
	    app.old_map.id=EXTRACT_CYCLE_VISIT;
	    waiting.push(state_ref);
	  }
      st.set_app_by_ref(state_ref, app);
    }
}

// }}}

// {{{ void RELAX_PATH(state_t state, map_value_t propag, size_int_t depth)

void RELAX_PATH(state_t state, map_value_t propag, size_int_t depth)
{
  appendix_t app;
  depth++;
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, app);
  if ((state_ref==acc_cycle_ref)&&(acc_cycle_is_mine))
    {
      app.old_map.bfs_order=depth;
      app.act_map=propag;
      end_of_iteration = true;
      message.rewind();
      message.append_size_int(depth);
      distributed.network.send_urgent_message(message, 
					 0,
					 TAG_PATH_RECOVERED);
    }
  else      
    if (app.old_map.id!=EXTRACT_PATH_VISIT)
      {
	app.old_map.bfs_order=depth;
	app.act_map=propag;
	app.old_map.nid=nid;
	app.old_map.id=EXTRACT_PATH_VISIT;
	waiting.push(state_ref);
      }
  st.set_app_by_ref(state_ref, app);
}

// }}}

// {{{ void process_ce_cycle_state(map_value_t _ref, size_int_t depth)

void process_ce_cycle_state(map_value_t _ref, size_int_t depth)
{
  state_ref_t ref;
  ref.hres = _ref.bfs_order;
  ref.id = _ref.id;
  state_t current_state = st.reconstruct(ref);
  message.rewind();
  message.append_size_int(depth);
  message.append_state(current_state);

  distributed.network.send_urgent_message(message,
				     0,
				     TAG_SEND_STATE_ON_CE_CYCLE);

  delete_state(current_state);
  if (depth==0) //hence I am at the begining of the cycle
    {
      for (size_int_t i=0; i<nnn; i++)
	if (i!=nid)
	  distributed.network.send_urgent_message(0,
					     0,
					     i,
					     TAG_CYCLE_RECONSTRUCTED);
      end_of_iteration=true;
    }
  else  //process predecessor of the state
    {
      st.get_app_by_ref(ref,appendix);
      if (appendix.act_map.nid==nid) //predecessor is mine
	{
	  process_ce_cycle_state(appendix.act_map, depth-1);
	}
      else
	{
	  depth--;
	  message.rewind();
	  message.append_size_int(appendix.act_map.bfs_order);
	  message.append_size_int(appendix.act_map.nid);
	  message.append_size_int(appendix.act_map.id);
	  message.append_size_int(depth);
	  
	  distributed.network.send_urgent_message(message,
					     appendix.act_map.nid,
					     TAG_CYCLE_RECONSTRUCTION);
	}
    }
}

// }}}

// {{{ void process_ce_path_state(map_value_t _ref, size_int_t depth)

void process_ce_path_state(map_value_t _ref, size_int_t depth)
{
  state_ref_t ref;
  ref.hres = _ref.bfs_order;
  ref.id = _ref.id;
  state_t current_state = st.reconstruct(ref);
  message.rewind();
  message.append_size_int(depth);
  message.append_state(current_state);

  distributed.network.send_urgent_message(message,
				     0,
				     TAG_SEND_STATE_ON_CE_PATH);

  delete_state(current_state);
  if (depth==0) //hence I am at the begining of the path
    {
      for (size_int_t i=0; i<nnn; i++)
	if (i!=nid)
	  distributed.network.send_urgent_message(0,
					     0,
					     i,
					     TAG_PATH_RECONSTRUCTED);
      end_of_iteration=true;
    }
  else  //process predecessor of the state
    {
      st.get_app_by_ref(ref,appendix);
      if (appendix.act_map.nid==nid) //predecessor is mine
	{
	  process_ce_path_state(appendix.act_map, depth-1);
	}
      else
	{
	  depth--;
	  message.rewind();
	  message.append_size_int(appendix.act_map.bfs_order);
	  message.append_size_int(appendix.act_map.nid);
	  message.append_size_int(appendix.act_map.id);
	  message.append_size_int(depth);

	  distributed.network.send_urgent_message(message,
					     appendix.act_map.nid,
					     TAG_PATH_RECONSTRUCTION);
	}
    }
}

// }}}

// {{{ void cycle_recovering()

void cycle_recovering()
{
  succ_container_t succs(*p_sys);
  state_t succ_state;
  map_value_t propag;
  size_int_t state_nid;
  end_of_iteration = false; //it stands for end_of_cycle_recovering now
  
  if (acc_cycle_is_mine)
    {
      appendix.act_map=NULL_MAP;
      appendix.shrinkA_ptr=list<state_ref_t>::iterator(0);
      appendix.old_map.bfs_order=0;
      appendix.old_map.nid=nid;
      appendix.old_map.id=EXTRACT_CYCLE_VISIT;
      st.set_app_by_ref(acc_cycle_ref, appendix);
      distributed.set_busy();
      waiting.push(acc_cycle_ref);
    }
  else
    distributed.set_idle();
  
  while ((!end_of_iteration)&&(!distributed.synchronized()))
    { 	      
      if (!(waiting.empty()))
	distributed.set_busy();
      do
	{ 
	  while (!(waiting.empty()))
	    { 
	      state_ref = waiting.front();
	      waiting.pop();
	      if (!end_of_iteration)
		{
		  current_state = st.reconstruct(state_ref);
		  if (debug>1)
		    {
		      p_sys->print_state(current_state,cout/*, 7*/);
		    }
		  p_sys->get_succs(current_state, succs);
		  propag.bfs_order = state_ref.hres;
		  propag.id = state_ref.id;
		  propag.nid = nid;
		  st.get_app_by_ref(state_ref,appendix);
		  for (size_int_t i = 0; i<succs.size(); i++)
		    { 
		      succ_state = succs[i];
		      state_nid = distributed.get_state_net_id(succ_state);
		      if (state_nid == nid) //state is mine?
			{ 
			  RELAX_CYCLE(succ_state, propag, 
				      appendix.old_map.bfs_order);
			}
		      else //send the message to the owner of the state
			{ 
			  send_ce_recovering(state_nid, &succ_state, 
					     &(propag), 
					     &(appendix.old_map.bfs_order),
					     TAG_CYCLE_RECOVERING);
			}
		      delete_state(succ_state);
		    }
		  delete_state(current_state);
		  succs.clear();
		}
	    } //end of "while (!(waiting.empty()))"
	  
	  distributed.network.flush_all_buffers();
	  distributed.process_messages();
	}
      while (!(waiting.empty())&&(!(end_of_iteration)));
      
      //now I am idle and next iteration should start
      distributed.set_idle();
      distributed.network.flush_all_buffers();
      distributed.process_messages();
    } //end of "while (!(end_of_iteration))"
  distributed.set_idle();
  distributed.network.flush_all_buffers();
  while (!distributed.synchronized())
    distributed.process_messages();
  while (!waiting.empty())
    waiting.pop();
  end_of_iteration = false;
}

// }}}

// {{{ void path_recovering()

void path_recovering()
{
  succ_container_t succs(*p_sys);
  state_t succ_state;
  map_value_t propag;
  size_int_t state_nid;
  if ((size_int_t)distributed.get_state_net_id(s0) == nid)
    { 
      appendix.act_map=NULL_MAP;
      appendix.shrinkA_ptr=list<state_ref_t>::iterator(0);
      appendix.old_map.bfs_order=0;
      appendix.old_map.nid=nid;
      appendix.old_map.id=EXTRACT_PATH_VISIT;
      st.is_stored(s0, state_ref);
      st.set_app_by_ref(state_ref, appendix);
      distributed.set_busy();
      waiting.push(state_ref);
    } 
  else 
    distributed.set_idle(); 
  end_of_iteration = false; //it stands for end_of_path_recovering now
  
  while ((!end_of_iteration)&&(!distributed.synchronized()))
    { 
      if (!(waiting.empty()))
	distributed.set_busy();
      do
	{ 
	  while (!(waiting.empty()))
	    { 
	      state_ref = waiting.front();
	      waiting.pop();
	      if (!end_of_iteration)
		{
		  current_state = st.reconstruct(state_ref);
		  if (debug>1)
		    {
		      p_sys->print_state(current_state,cout/*, 0*/);
		      cout << endl;
		    }
		  p_sys->get_succs(current_state, succs);
		  propag.bfs_order = state_ref.hres;
		  propag.id = state_ref.id;
		  propag.nid = nid;
		  st.get_app_by_ref(state_ref,appendix);
		  for (size_int_t i = 0; i<succs.size(); i++)
		    { 
		      succ_state = succs[i];
		      state_nid = distributed.get_state_net_id(succ_state);
		      if (state_nid == nid) //state is mine?
			{ 
			  RELAX_PATH(succ_state, propag, 
				     appendix.old_map.bfs_order);
			}
		      else //send the message to the owner of the state
			{ 
			  send_ce_recovering(state_nid, &succ_state, 
					     &(propag), 
					     &(appendix.old_map.bfs_order),
					     TAG_PATH_RECOVERING);
			}
		      delete_state(succ_state);
		    }
		  delete_state(current_state);
		  succs.clear();
		}
	    } //end of "while (!(waiting.empty()))"
	  
	  distributed.network.flush_all_buffers();
	  distributed.process_messages();
	}
      while (!(waiting.empty())&&(!(end_of_iteration)));
      
      //now I am idle and next iteration should start    
      distributed.set_idle();
      distributed.network.flush_all_buffers();
      distributed.process_messages();
    } //end of "while (!(end_of_iteration))"
  distributed.set_idle();
  distributed.network.flush_all_buffers();
  while (!distributed.synchronized())
    distributed.process_messages();
  while (!waiting.empty())
    waiting.pop();
  end_of_iteration = false;
}

// }}}

// {{{ void MAP()

void MAP()
{
  succ_container_t succs(*p_sys);
  state_t succ_state;
  appendix_t app;
  size_int_t state_nid;
  bool first_visit;
  end_of_iteration = false;

  while ((!end_of_iteration)&&(!distributed.synchronized(info_shrinkA)))
    { 
      if (!(waiting.empty()))
	distributed.set_busy();
      
      do
	{ 
	  int aux_counter = 0;
	  while (!(waiting.empty()))
	    { 
	      state_ref = waiting.front();
	      waiting.pop();
	      current_state = st.reconstruct(state_ref);

	      if (debug>1)
		{
		  p_sys->print_state(current_state,cout/*, 7*/);
		}
	      st.get_app_by_ref(state_ref, app);
	      first_visit=(app.old_map.nid==divine::MAX_ULONG_INT);
	      if (first_visit)
		{
		  app.old_map.nid--;
		  st.set_app_by_ref(state_ref, app);
		}
	      if (debug)
		{ 
		  cout << nid << ": state: ";
		  vypis_state_ref(state_ref, cout);
		  cout << "  ";
		  vypis_map_value(app.act_map, cout);
		  cout << "  ";
		  vypis_map_value(app.old_map, cout);
		  cout << endl;
		}
	      p_sys->get_succs(current_state, succs);
	      succs_calls++;
	      edges_relaxed+=succs.size();
	      if (first_visit)
		trans+=succs.size();

	      for (size_int_t i = 0; ((i<succs.size())&&(!acc_cycle_found)); 
		   i++)
		{ 
		  succ_state = succs[i];
		  state_nid = distributed.get_state_net_id(succ_state);
		  if (state_nid == nid) //state is mine?
		    { 
		      RELAX(succ_state, app.act_map, app.old_map);
		    }
		  else //send the message to the owner of the state
		    { 
		      if (first_visit)
			cross_trans++;		      
		      send_state(state_nid, &succ_state, &(app.act_map), 
				 &(app.old_map));
		    }
		  delete_state(succ_state);
		  }
	      delete_state(current_state);	    
#if defined(ORIG_POLL)
	      if (aux_counter>100) { distributed.process_messages(); aux_counter = 0; }
	      else aux_counter++;
#else
	      /* process_messages() now does its own rate control, so reduce less */
	      if (aux_counter>10) { distributed.process_messages(); aux_counter = 0; }
	      else aux_counter++;
#endif
	    } //end of "while (!(waiting.empty()))"
	  
#if defined(ORIG_FLUSH)
	  distributed.network.flush_all_buffers();
#else
#if 0
	  distributed.network.flush_some_buffers();
#else
	  /* better performance without any flushing at this point? */
#endif
#endif
	  distributed.process_messages();
	}
      while (!(waiting.empty())&&(!(end_of_iteration)));
      
      //now I am idle and next iteration should start
#if defined(ORIG_FLUSH)
      distributed.network.flush_all_buffers();
#else
      distributed.network.flush_some_buffers();
#endif
      distributed.set_idle();
      distributed.process_messages();
    } //end of "while (!(end_of_iteration))"  
  
}

// }}}

// {{{ void DEL_ACC()

void DEL_ACC()
{ 
  appendix_t app;
  while (!(shrinkA.empty()))
  { state_ref = shrinkA.back();

    //updating last_map and new_map values
    st.get_app_by_ref(state_ref,app);
    app.old_map=app.act_map;
    app.act_map=NULL_MAP;
    app.shrinkA_ptr=list<state_ref_t>::iterator(0); //really necessary?
    st.set_app_by_ref(state_ref,app);
    
    //removing states from shrinkA to structure waiting
    waiting.push(state_ref);
    shrinkA.pop_back();
  }
}

// }}}

// {{{ int main(int argc, char **argv)

int main(int argc, char **argv)
{ 
  try 
    { 
      distributed.process_user_message = process_message;
      distributed.network_initialize(argc, argv);
      distributed.initialize();
      distributed.set_proc_msgs_buf_exclusive_mem(false);


      ostringstream oss,oss1;
      oss1<<"distr_map";
      int c;
      opterr=0;

      static struct option longopts[] = {
		{ "help",       no_argument, 0, 'h'},
		{ "quiet",      no_argument, 0, 'q'},
		{ "trail",      no_argument, 0, 't'},
		{ "report",     no_argument, 0, 'r'},
		{ "fast",		no_argument, 0, 'f'},
	    { "log",        no_argument, 0, 'L'},
		{ "htsize",     required_argument, 0, 'H' },
		{ "statelist",  no_argument, 0, 'c'},
		{ "verbose", 	no_argument, 0, 'V'},
		{ "version",    no_argument, 0, 'v'},
		{ 0, 0, 0, 0 }
      };

      while ((c = getopt_long(argc, argv, "LX:H:SVfqhtrvc", longopts, 0)) != -1)
		{
		  oss1 <<" -"<<(char)c;
		  switch (c) {
			  case 'h': 
			  	if (distributed.network_id == 0) 
				{ 
				  usage(); 
				}
				return 0;
				break;
			  case 'L': logging = true; break;
			  case 'X': set_base_name = optarg; base_name = true; break;
			  case 'H': htsize=atoi(optarg);break;
			  case 'V':
			  case 'S': statistics = true;break;
			  case 'q': quietmode = true;break;
			  case 't': trail=true;break;
			  case 'f': fastApproximation = true; break;
			  case 'r': report = true;break;
			  case 'c': show_ce = true; break;
			  case 'v': 
			  	if (distributed.network_id == 0) 
				{ 
				  version();
				}
				return 0;
				break;
			  case '?': cerr <<"unknown switch -"<<(char)optopt<<endl;
		  }
		}
      
      if (quietmode)
		{
		  statistics = false;
		}
      
      if (argc < optind+1)
		{	
		  if (distributed.network_id == 0)
			{
			  usage();
			}
		  return 0;
		}
      
      nid = distributed.network_id;
      nnn = distributed.cluster_size;

      oss << (nid<10?" ":"") <<nid <<": ";
      ids = oss.str();

      if (htsize != 0)
        {	  
          if (htsize < 33)
            {
              int z = htsize;
              htsize = 1;
              for (;z>0; z--)
                {
                  htsize = 2* htsize;
                }
            }
          st.set_ht_size(htsize);
        }
      appendix.act_map=NULL_MAP; 
      appendix.old_map=NULL_MAP; 
      appendix.shrinkA_ptr=list<state_ref_t>::iterator(0);
      st.set_appendix(appendix);

      /* decisions about the type of an input */
      char * filename = argv[optind];
      int filename_length = strlen(filename);

      if (filename_length>=4 && strcmp(filename+filename_length-4,".bio")==0)
		{
		  if (nid==0)
			cout << "Reading bio source..." << endl;
		  input_file_ext = ".bio";
		  p_sys = new affine_explicit_system_t(gerr);
		}

    
      //reading of input:
      int file_opening;
      try
		{
		  if ((file_opening=p_sys->read(argv[optind],fastApproximation))&&(nid==0))
			{
			  if (file_opening==system_t::ERR_FILE_NOT_OPEN)
				gerr << nid << ": " << "Cannot open file ...";
			  else 
			  	gerr << nid << ": " << "Syntax error ...";
			}
		  if (file_opening)
			gerr << thr();
		}
      catch (ERR_throw_t & err)
		{ 
		  distributed.finalize();
		  return err.id; 
		}

      file_base_name = argv[optind];
      size_t pos = file_base_name.find(input_file_ext, file_base_name.length() - 4);
      if (pos != string::npos)
		file_base_name.erase(pos, 4);

      if (p_sys->get_with_property() && p_sys->get_property_type()!=BUCHI)
		{
		  gerr << nid << ": "<< "Cannot work with other than standard Buchi accepting condition." <<thr();
		}

      st.init();

      file_name = argv[optind];
      int position = file_name.find(input_file_ext,0);
      file_name.erase(position,strlen(input_file_ext));
      
      if (report)
		{
		  reporter.set_info("InitTime", timer.gettime());
		  reporter.start_timer();
		}

      if (logging)
        {
          logger.set_storage(&st);
          if (base_name)
            {
              logger.init(&distributed,set_base_name);
            }
          else
            {
              logger.init(&distributed,file_name);
            }
          logger.use_SIGALRM(1);
        }

      // =============================
      // === Distributed MAP =========
    
      if (nid == 0 && statistics)
		{
		  cout <<ids<<"Computation init:  "<<timer.gettime()<<" s"<<endl;
		  //timer.reset(); //computation time includes initialization
		}

      s0 = p_sys->get_initial_state();

      if ((size_int_t)distributed.get_state_net_id(s0) == nid)
		{ 
		  st.insert(s0, state_ref);
		  if (p_sys->is_accepting(s0))
			{ 
			  appendix.act_map=prirad_novy_map(&(state_ref));
			  shrinkA.push_front(state_ref);
			  appendix.shrinkA_ptr=shrinkA.begin();
			}
		  else
			{ 
			  appendix.act_map=NULL_MAP;
			  appendix.shrinkA_ptr=list<state_ref_t>::iterator(0);
			}
		  appendix.old_map=NULL_MAP_VISIT;
		  appendix.old_map.nid = divine::MAX_ULONG_INT; //to detect whether a state is handled for the first time in MAP() in order to count (cross) trans correctly
		  st.set_app_by_ref(state_ref, appendix);
		  waiting.push(state_ref);

		  distributed.set_busy();
		}
      else
		distributed.set_idle();
      
      do
	 	{
		  iter_count++;
		  if (nid == 0)
			  cout << nid << ": iteration nr. " << iter_count 
			   << ": shrinkA.size: " 
			   << info_shrinkA.data.size_of_all_shrinkA_sets << endl;
		  MAP();

		  if (!(acc_cycle_found)) // probably next iteration is necessary
			{ 
			  DEL_ACC();
			  finish=(info_shrinkA.data.size_of_all_shrinkA_sets==0);
			}	  
		}
      while (!finish);
      
      if (logging)
        {
          logger.log_now();
          logger.stop_SIGALRM();
        }

      if ((acc_cycle_found) && (trail || show_ce))
		{ 	  
		  //first, recovering of cycle
		  cycle_recovering();

		  //reconstruction of the cycle
		  if (acc_cycle_is_mine)
			{ 	  
			  st.get_app_by_ref(acc_cycle_ref, appendix);
			  map_value_t _ref;
			  _ref.bfs_order = acc_cycle_ref.hres;
			  _ref.id = acc_cycle_ref.id;
			  _ref.nid = nid;
			  process_ce_cycle_state(_ref, appendix.old_map.bfs_order);
			}
			
		  while(!end_of_iteration)//it stands for end_of_cycle_reconstruct. now
			{ 
			  distributed.process_messages();
			}

		  //synchronization: NECESSARY to call
		  distributed.set_idle();
		  distributed.network.flush_all_buffers();
		  while (!distributed.synchronized())
			distributed.process_messages();
			
		  while (!waiting.empty())
			waiting.pop();
			
		  end_of_iteration = false;
		  
		  //now recovering path from initial state to the cycle
		  path_recovering();
		  
		  //reconstruction of the path
		  if (acc_cycle_is_mine)
			{ 	  
			  st.get_app_by_ref(acc_cycle_ref, appendix);
			  map_value_t _ref;
			  _ref.bfs_order = acc_cycle_ref.hres;
			  _ref.id = acc_cycle_ref.id;
			  _ref.nid = nid;
			  process_ce_path_state(_ref, appendix.old_map.bfs_order);
			}
			
		  while(!distributed.synchronized())
			{ 
			  distributed.process_messages();
			}
		  
		  //output
		  if (nid == 0)
			{
			  path_t ce(p_sys);
			  ofstream ce_out;
			  string pom_fn;
			  if ((show_ce) || (trail))
				{
				  for (size_int_t i=0; i<path_length-1; i++)
					ce.push_back(path_state[i]);
				  ce.push_back(cycle_state[0]);
				  ce.mark_cycle_start_back();
				  for (size_int_t i=1; i<cycle_length-1; i++)
					ce.push_back(cycle_state[i]);		  
				  if (trail)
					{
					  if (base_name)
						{
						  pom_fn = set_base_name+".trail";
						}
					  else
						{
						  pom_fn = file_name+".trail";
						}
					  ce_out.open(pom_fn.c_str());
					  if (p_sys->can_system_transitions())			
						ce.write_trans(ce_out);
					  else
						ce.write_states(ce_out);
					  ce_out.close();
					}

				  if (show_ce)
					{
					  if (base_name)
						{
						  pom_fn = set_base_name+".ce_states";
						}
					  else
						{
						  pom_fn = file_name+".ce_states";
						}
					  ce_out.open(pom_fn.c_str());
					  ce.write_states(ce_out);
					  ce_out.close();
					}
				  ce.erase();
				}

			  //deleting allocated memory
			  for (size_int_t i=0; i<cycle_length; i++)
				delete_state(cycle_state[i]);
				
			  delete[] cycle_state;
			  for (size_int_t i=0; i<path_length; i++)
				delete_state(path_state[i]);
				
			delete[] path_state;
			}
		  
		} //end of "if ((acc_cycle_found) && (trail || show_ce))"

      // ----------------------------------------
      // finalization, collecting statistics etc.
      // ----------------------------------------

      if (statistics)
		{
		  while (!(distributed.synchronized(info_state_space)))
			distributed.process_messages();
		}

      if (report)
		{	  
		  ofstream report_out;
		  string pom_fn;
		      if (base_name)
		        {
		          pom_fn = set_base_name+".report";
		        }
		      else
		        {
		          pom_fn = file_name+".report";
		        }
		  if (nid==0) report_out.open(pom_fn.c_str());
		  reporter.set_obligatory_keys(oss1.str(), argv[optind], "LTL MC", st.get_states_stored(), succs_calls);
		  reporter.set_info("States", st.get_states_stored());
		  reporter.set_info("Trans", trans);
		  reporter.set_info("CrossTrans", cross_trans);
		  if  (nid == 0)
			{
			  if (acc_cycle_found)
				{
				  reporter.set_global_info("IsValid","No");
				  if (trail || show_ce)
					reporter.set_global_info("CEGenerated", "Yes");
				  else
					reporter.set_global_info("CEGenerated", "No");
				}
			  else
				{
				  reporter.set_global_info("IsValid","Yes");
				  reporter.set_global_info("CEGenerated", "No");
				}
			}
		  reporter.stop_timer();
		  reporter.collect_and_print(REPORTER_OUTPUT_LONG,report_out);
		  if (nid==0) report_out.close();
		}     

      if (statistics && nid==0)
		{
		  cout <<ids<<"state size:\t\t"<<s0.size<<endl;
		  cout <<ids<<"appendix size:\t"<<sizeof(appendix)<<endl;
	      cout << ids << "states generated:\t" <<
		  		info_state_space.data.all_states << endl;
	      if (htsize != 0)
	        cout <<ids<<"hashtable size:\t"<<htsize<<endl;
	      else
	        cout <<ids<<"hashtable size:\t"<<65536<<endl;
		        
	      cout <<ids<<"get_succs called:\t"<<
  				info_state_space.data.all_succ_calls<<endl;
	      cout <<ids<<"trans. relaxed:\t"<<
	   	  		info_state_space.data.all_edges_relaxed<<endl;
	      cout <<ids<<"all memory used:\t"<<
		  		info_state_space.data.all_mem/1024.0<<" MB"<<endl;
	      cout <<ids<<"Computation done:\t"<<timer.gettime()<<" s"<<endl;
	      cout <<ids<<"--------------------"<<endl;
		}
      
      if (nid == 0)
		{ 
		  cout << ' ' << nid << ": Accepting cycle:\t" 
			   << ((acc_cycle_found)?("YES"):("NO")) << endl
			   << ' ' << nid << ": Number of iterations:\t" << iter_count 
			   << endl;
		}      
      distributed.finalize();
      delete p_sys;
    }
  catch (...)
    {
      cout <<endl<<flush;
      distributed.network.abort();
    }
    return 0;
}

// }}}
