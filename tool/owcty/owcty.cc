#include <getopt.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <queue>
#include <stack>
#include <time.h>
#include "divine.h"

using namespace std;
using namespace divine;

typedef struct net_state_ref_t
{
  state_ref_t state_ref;
  int network_id; 
  void print();
};

void net_state_ref_t::print()
{
  cout <<"(";
  cout <<state_ref.hres<<",";
  cout <<state_ref.id<<",";
  cout <<network_id;
  cout <<")"<<endl;    
}

//returns true iff ref1<ref2 when comparing triple (hres, id, network_id) lexicographically
//!!!be aware: according to this operator, NET_STATE_REF_NULL (which is defined further) has to be the highest possible value of net_state_ref_t!!!
bool operator< (net_state_ref_t ref1, net_state_ref_t ref2)
{
  return ((ref1.state_ref.hres<ref2.state_ref.hres)||
	  ((ref1.state_ref.hres==ref2.state_ref.hres)&&(ref1.state_ref.id<ref2.state_ref.id))||
	  ((ref1.state_ref.hres==ref2.state_ref.hres)&&(ref1.state_ref.id==ref2.state_ref.id)&&(ref1.network_id<ref2.network_id)));
}

bool operator== (net_state_ref_t ref1, net_state_ref_t ref2)
{
  return ((ref1.state_ref.hres==ref2.state_ref.hres)&&
	  (ref1.state_ref.id==ref2.state_ref.id)&&
	  (ref1.network_id==ref2.network_id));
}

typedef struct appendix_t 
{
  size_t iteration; //for reduced state space generation: iteration=1 iff state is on the stack; for generating counterexample it serves as parent[r] (see article)
  size_t p; //two usages: during accepting cycle detection it holds number of state's predecessors; during counterexample generation it holds depth of a counterexample trail
  bool in_S; 
  net_state_ref_t parent;
  bit_string_t *ample_set; //in reduced state space generation: set to NULL iff state not yet processed
  net_state_ref_t max_pred_seed; //to detect global cycles, store in each state maximal predecessor which is a seed (seed=successor of fully expanded state)
};


const int TAG_SEND_STATE = DIVINE_TAG_USER +0;
const int TAG_SEND_RESET = DIVINE_TAG_USER +1;
const int TAG_SEND_REACH = DIVINE_TAG_USER +2;
const int TAG_SEND_ELIMI = DIVINE_TAG_USER +3;
const int TAG_SEND_POR_STATE = DIVINE_TAG_USER +4;
const int TAG_CANDIDATE_DETECT = DIVINE_TAG_USER +5;
const int TAG_CYCLE_RECOVERED=DIVINE_TAG_USER+6;
const int TAG_VISIT1_STATE = DIVINE_TAG_USER+7;
const int TAG_VISIT2_STATE = DIVINE_TAG_USER+8;
const int TAG_SEND_CYCLE_DETECT = DIVINE_TAG_USER+9;
const int TAG_SEND_STATE_ON_CE_CYCLE=DIVINE_TAG_USER+10;
const int TAG_CYCLE_RECONSTRUCTED=DIVINE_TAG_USER+11;
const int TAG_CYCLE_RECONSTRUCTION=DIVINE_TAG_USER+12;
const int TAG_PATH_RECOVERED=DIVINE_TAG_USER+13;
const int TAG_SEND_STATE_ON_CE_PATH=DIVINE_TAG_USER+14;
const int TAG_PATH_RECONSTRUCTED=DIVINE_TAG_USER+15;
const int TAG_PATH_RECONSTRUCTION=DIVINE_TAG_USER+16;
const int TAG_TRY_TO_FIND_CANDIDATE=DIVINE_TAG_USER+17;

const char * input_file_ext = 0;


//!!!be aware: according to the operator< (net_state_ref_t,net_state_ref_t), NET_STATE_REF_NULL has to be the highest possible value of net_state_ref_t!!!
state_ref_t REF_NULL;//by default, it is invalidated thus assigned to very high value
const net_state_ref_t NET_STATE_REF_NULL = { REF_NULL, static_cast<int>(divine::MAX_ULONG_INT)};

distributed_t distributed;
distr_reporter_t reporter(&distributed);
explicit_system_t * p_sys = 0;
explicit_storage_t st;
timeinfo_t timer;
vminfo_t vm;
logger_t logger;
bool simple = false;

message_t message;
message_t received_message;

appendix_t appendix;

queue<state_ref_t> waiting_queue; //used for generation of state space reduced by p.o.r. too
queue<state_ref_t> q_queue;
queue<state_ref_t> L_queue;

bool candidate_given=false;
bool acc_cycle_is_mine = false;
bool end_of_iteration = false;

state_t ce_state,ce_winning_state, succ_state;
state_ref_t ce_winning_state_ref;
bool ce_state_found = false;
bool ce_state_winner = false;
bool ce_collecting_firsttime = true;
net_state_ref_t ce_state_predecessor_ref;

state_ref_t acc_cycle_ref;
state_ref_t state_ref, succ_ref;
int who_has_candidates = 0;

size_int_t succs_calls = 0;

size_int_t cycle_length, path_length;
state_t *cycle_state, *path_state;

enabled_trans_container_t *all_enabled_trans; 


path_t ce;

size_t trans=0,transcross=0;
size_t Ssize=0,oldSsize=0;
size_t fullexpand=0; //how many states has been fully expanded when p.o.r. enabled
size_t expand_due_to_proviso=0; //how many states has to be fully expanded due to proviso checking (cycle condition) when p.o.r. enabled
size_t reexplored_states=0; //how many states has to be reexplored due to max_pred_seed
size_t iteration=0;

//declaration of some functions called in process_message()
void visit2(state_t state,state_ref_t predecessor, int depth,int nid);
void process_ce_cycle_state(state_ref_t _ref,int depth);
void visit1(state_t state, state_ref_t predecessor, size_t depth,int nid);
void process_ce_path_state(state_ref_t _ref, size_t depth);

unsigned log_wrapper1()
{
  return waiting_queue.size();
}

unsigned log_wrapper2()
{
  return q_queue.size();
}

unsigned log_wrapper3()
{
  return L_queue.size();
}

unsigned log_wrapper4()
{
  return L_queue.size()+q_queue.size()+waiting_queue.size();
}


struct info_t {
  size_t allstored;
  size_t allSsize;
  size_t alltrans;
  size_t allcross;
  size_t allfullexpand;
  size_t allreexplored_states;
  size_t allexpand_due_to_proviso;
  long int allmem;
  virtual void update();
  virtual ~info_t() {}
};

void info_t::update()
{
  if (distributed.network_id == 0)
    {
      allstored = st.get_states_stored();
      allSsize = Ssize;
      alltrans = trans;
      allcross = transcross;
      allfullexpand = fullexpand;
      allreexplored_states = reexplored_states;
      allexpand_due_to_proviso = expand_due_to_proviso;
      allmem = vm.getvmsize();
    }
  else
    {
      allstored += st.get_states_stored();
      allSsize += Ssize;
      alltrans += trans;
      allcross += transcross;
      allfullexpand += fullexpand;
      allreexplored_states += reexplored_states;
      allexpand_due_to_proviso += expand_due_to_proviso;
      allmem += vm.getvmsize();
    }
}

updateable_info_t<info_t> info;

void full_expand_state(state_t state);

void process_message(char *buf, int size, int src, int tag, bool urgent) 
{
  state_t state_received;
  state_ref_t ref;
  net_state_ref_t net_ref_received;
  
  switch (tag) {
  case TAG_SEND_STATE:
    received_message.load_data((byte_t*)buf,size);
    received_message.read_state(state_received);
    received_message.read_data((char *)&net_ref_received,sizeof(net_state_ref_t));
    if (!st.is_stored(state_received,ref))
      {
	st.insert(state_received,ref);
	appendix.in_S = p_sys->is_accepting(state_received);
	appendix.p = 0;
	appendix.iteration = 0;
	appendix.ample_set = 0; //i.e. NULL
	appendix.max_pred_seed = NET_STATE_REF_NULL;
	appendix.parent = net_ref_received;
	st.set_app_by_ref(ref,appendix);		
	waiting_queue.push(ref);
	if (appendix.in_S) //perform Reset together with first reachability
	  {
	    Ssize ++;
	    q_queue.push(ref);
	  }
	distributed.set_busy();
      }
    delete_state(state_received);
    break;
  case TAG_SEND_RESET:
    received_message.load_data((byte_t*)buf,size);
    received_message.read_state(state_received);
    st.is_stored(state_received,ref);
    st.get_app_by_ref(ref,appendix);
    if (appendix.iteration < iteration)
      {
	appendix.in_S = appendix.in_S && p_sys->is_accepting(state_received);
	appendix.iteration = iteration;
	appendix.p = 0;
	st.set_app_by_ref(ref,appendix);		     
	waiting_queue.push(ref);
	distributed.set_busy();
	if (appendix.in_S)
	  {
	    Ssize ++;
	    q_queue.push(ref);
	  }
      }
    delete_state(state_received);
    break;
  case TAG_SEND_REACH:
    received_message.load_data((byte_t*)buf,size);
    received_message.read_state(state_received);
    st.is_stored(state_received,ref);
    st.get_app_by_ref(ref,appendix);
    if (!appendix.in_S) 
      {
	appendix.in_S = true;
	Ssize ++;
	waiting_queue.push(ref);
	distributed.set_busy();
      }
    appendix.p ++;
    st.set_app_by_ref(ref,appendix);		     
    delete_state(state_received);
    break;
  case TAG_SEND_POR_STATE:
    {
      net_state_ref_t net_ref_received2;
      bool revisited=false;
      received_message.load_data((byte_t*)buf,size);
      received_message.read_state(state_received);
      received_message.read_data((char *)&net_ref_received,sizeof(net_state_ref_t)); //parent
      received_message.read_data((char *)&net_ref_received2,sizeof(net_state_ref_t)); //propagated max_pred_seed
      if (!st.is_stored(state_received, ref))
	{
	  revisited=true;
	  st.insert(state_received, ref);
	  appendix.in_S = p_sys->is_accepting(state_received);
	  appendix.p = 0;
	  appendix.parent = net_ref_received;
	  appendix.iteration = 0;
	  // received_message.read_data((char *)&(appendix.parent),sizeof(net_state_ref_t));
	  appendix.ample_set = 0; //i.e. NULL
	  if (net_ref_received2 == NET_STATE_REF_NULL) //thus, state_received is a seed
	    {
	      appendix.max_pred_seed.state_ref=ref;
	      appendix.max_pred_seed.network_id=distributed.network_id;
	    }
	  else //propagate max_pred_seed
	    appendix.max_pred_seed=net_ref_received2;

	  st.set_app_by_ref(ref,appendix);
	  if (appendix.in_S) //perform Reset together with first reachability
	    {
	      Ssize ++;
	      q_queue.push(ref);
	    }
	}
      else //the state may have lower max_pred_seed; in such situation, update the value and push the state to waiting_queue
	{
	  st.get_app_by_ref(ref, appendix);
	  if (appendix.max_pred_seed<net_ref_received2)
	    {
	      appendix.max_pred_seed=net_ref_received2;
	      revisited=true;
	      st.set_app_by_ref(ref, appendix);
	    }
	  else
	    if ((appendix.max_pred_seed==net_ref_received2)&&(!(appendix.max_pred_seed==NET_STATE_REF_NULL))
		&&(appendix.ample_set!=0)) //a global cycle may be closed => fully expand this state
	      {
		expand_due_to_proviso++;
		full_expand_state(state_received);
	      }
	}
      if (revisited)
	{
	  distributed.set_busy();
	  waiting_queue.push(ref);
	}
      delete_state(state_received);      
      break;
    }
  case TAG_SEND_ELIMI:
    received_message.load_data((byte_t*)buf,size);
    received_message.read_state(state_received);
    st.is_stored(state_received,ref);
    st.get_app_by_ref(ref,appendix);
    appendix.p --;
    st.set_app_by_ref(ref,appendix);
    if (appendix.p == 0)
      {
	L_queue.push(ref);
	distributed.set_busy();
      }
    delete_state(state_received);
    break;
  case TAG_CANDIDATE_DETECT:
    {
      received_message.load_data((byte_t*)(buf),size);
      received_message.read_state_ref(acc_cycle_ref);

      candidate_given=true;
      iteration++;
      while (!(waiting_queue.empty()))
	waiting_queue.pop();
      break;
    }

  case TAG_VISIT2_STATE:
    {
      if (!end_of_iteration)
       {
	 size_int_t nid;
	 size_int_t depth;
	 
	 received_message.load_data((byte_t*)(buf),size);
	 received_message.read_state(state_received);
         received_message.read_state_ref(ref);
         received_message.read_size_int(nid);
	 received_message.read_size_int(depth);
	   
	 visit2(state_received, ref, depth,nid);
	 if (!(waiting_queue.empty())) //I will have some process, I will be busy
           distributed.set_busy();
	}
      delete_state(state_received);
      break;
    }

  case TAG_CYCLE_RECOVERED:
    { 
      if (distributed.network_id==0)
       {
	 received_message.load_data((byte_t*)(buf),size);
	 received_message.read_size_int(cycle_length);

	 cycle_length++;
	 cycle_state = new state_t[cycle_length];

	 message.rewind();

	 for (int i = 1; i < distributed.cluster_size; i++)
	   {
	     distributed.network.send_urgent_message(message,
						     i,
						     TAG_CYCLE_RECOVERED);
	   }
        }
      candidate_given=true;
      end_of_iteration = true; //it stands for end_of_cycle_recovering now
      while (!(waiting_queue.empty()))
	waiting_queue.pop();
      break;
      }  //end tag_cycle

  case TAG_CYCLE_RECONSTRUCTED:
    {
      end_of_iteration = true;//it stands for end_of_cycle_reconstruction now
      break;
    }

  case TAG_CYCLE_RECONSTRUCTION:
    {
      size_int_t depth;
      
      received_message.load_data((byte_t*)(buf),size);
      received_message.read_state_ref(ref);
      received_message.read_size_int(depth);
	
      process_ce_cycle_state(ref, depth);
      break;
    }
   
  case TAG_SEND_STATE_ON_CE_CYCLE: //can receive only Master
    {
      size_int_t depth;

      received_message.load_data((byte_t*)(buf),size);
      received_message.read_size_int(depth);
      received_message.read_state(cycle_state[depth]);

      break;
    }
   
  case TAG_VISIT1_STATE:
    {
      if (!end_of_iteration)
       {
         size_int_t nid;
	 size_int_t depth;
	    
	 received_message.load_data((byte_t*)(buf),size);
	 received_message.read_state(state_received);
         received_message.read_state_ref(ref);
         received_message.read_size_int(nid);
	 received_message.read_size_int(depth);
	 
	 visit1(state_received, ref, depth,nid);
	 if (!(waiting_queue.empty())) //I'll have some process, I will be busy
	  distributed.set_busy();
       }
      delete_state(state_received);
      break;
    }
      
  case TAG_PATH_RECOVERED:
    {
      if (distributed.network_id==0)
       {
	 received_message.load_data((byte_t*)(buf),size);
	 received_message.read_size_int(path_length);
	 path_length++;

	 path_state = new state_t[path_length];
	 
	 message.rewind();

	 for (int i = 1; i < distributed.cluster_size; i++)
	   {
	     distributed.network.send_urgent_message(message,
						     i,
						     TAG_PATH_RECOVERED);
	   }
       }	 
      end_of_iteration = true; //it stands for end_of_path_recovering now
      while (!(waiting_queue.empty()))
	waiting_queue.pop();
      break;
    }
    
  case TAG_PATH_RECONSTRUCTION:
    {
      size_int_t depth;

      received_message.load_data((byte_t*)(buf),size);
      received_message.read_state_ref(ref);
      received_message.read_size_int(depth);

      process_ce_path_state(ref, depth);
      break;
    }
    
  case TAG_PATH_RECONSTRUCTED:
    {
      end_of_iteration = true;//it stands for end_of_path_reconstruction now
      break;
    }
  
  case TAG_SEND_STATE_ON_CE_PATH: //can receive only Master
    {
      size_int_t depth;

      received_message.load_data((byte_t*)(buf),size);
      received_message.read_size_int(depth);
      received_message.read_state(path_state[depth]);
      break;
    }
    
  case TAG_TRY_TO_FIND_CANDIDATE:
    {
      who_has_candidates=distributed.network_id;//I'm on to find a candidate
      break;
    }
  }
}


void version()
{
  cout <<"OWCTY ";
  cout <<"version 1.0 build 14  (2007/Nov/26 16:00)"<<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  version();
  cout <<"-----------------------------------------------------------------"<<endl;

  cout <<"Usage: [mpirun -np N] owcty [options] input_file"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v,--version\t\tshow version"<<endl;
  cout <<" -h,--help\t\tshow this help"<<endl;
  cout <<" -H x,--htsize x\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -V,--verbose\t\tprint some statistics"<<endl;
  cout <<" -q,--quiet\t\tquite mode"<<endl;
  cout <<" -c, --statelist\tshow counterexample states"<<endl;
  cout <<" -C x,--compress x\tset state compression method"<<endl;
  cout <<"\t\t\t(0 = no compression, 1 = Huffman static compression)"<<endl;
  cout <<" -t,--trail\t\tproduce trail file"<<endl;
  cout <<" -f, --fast\t\tuse faster but less accurate algorithm for abstraction" << endl;
  cout <<" -r,--report\t\tproduce report file"<<endl;
  cout <<" -s,--simple\t\tperform simple reachability only"<<endl;
  cout <<" -L,--log\t\tproduce logfiles (log period is 1 second)"<<endl;
  cout <<" -X w\t\t\tsets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
}

void generate_successors(state_t state, state_ref_t ref, enabled_trans_container_t *enb, succ_container_t *succs_cont)
{
    p_sys->get_succs(state,*succs_cont);
}

void full_expand_state(state_t state)
{ 
  succ_container_t succs_cont(*p_sys);
  fullexpand++;
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, appendix);
  for (std::size_t i=0; i<appendix.ample_set->get_bit_count(); i++)
    {
      appendix.ample_set->enable_bit(i); //to detect that the state is fully expanded, enable transitions of all processes  
    }
  //the following line uses the fact that NET_STATE_REF_NULL is the highest possible value of net_state_ref_t; thus, a fully expanded state won't be revisited any time
  appendix.max_pred_seed=NET_STATE_REF_NULL; 
  appendix.iteration=0; //state won't be on the stack
  st.set_app_by_ref(state_ref, appendix);
  
  bit_string_t empty_cluster_bit_string(distributed.cluster_size);
  empty_cluster_bit_string.clear();
  succs_calls++;
  succs_cont.clear();
  p_sys->get_succs(state, succs_cont);
  net_state_ref_t nref;
  nref.state_ref=state_ref; nref.network_id=distributed.network_id;
  for (size_int_t i = 0; (i<succs_cont.size()); i++)
    {
      trans++;
      succ_state = succs_cont[i];
      if (distributed.partition_function(succ_state)!=distributed.network_id)
	{
	  transcross++;
	  message.rewind();
	  message.append_state(succ_state);
	  message.append_data((byte_t *)&nref,sizeof(net_state_ref_t));
	  message.append_data((byte_t *)&NET_STATE_REF_NULL,sizeof(net_state_ref_t));
	  distributed.network.send_message(message,
					   distributed.partition_function(succ_state),
					   TAG_SEND_POR_STATE);
	}
      else
	{
	  if (!st.is_stored(succ_state,succ_ref))
	    {
	      st.insert(succ_state,succ_ref);
	      appendix.in_S = p_sys->is_accepting(succ_state);
	      appendix.p = 0;
	      appendix.iteration = 0;
	      appendix.parent.state_ref = state_ref;
	      appendix.parent.network_id = distributed.network_id;
	      appendix.ample_set = 0;
	      appendix.max_pred_seed.state_ref = succ_ref;
	      appendix.max_pred_seed.network_id = distributed.network_id;
	      st.set_app_by_ref(succ_ref,appendix);
	      waiting_queue.push(succ_ref);
	      if (appendix.in_S) //perform Reset together with first reachability
		{
		  Ssize ++;
		  q_queue.push(succ_ref);
		}
	    }
	}
      delete_state(succ_state);
    }
  succs_cont.clear();
}

// {{{ functions for recovering counterexample

void broadcast_candidate()
{
  message.rewind();
  message.append_state_ref(acc_cycle_ref);
  iteration++;

  for (int i = 0; i < distributed.cluster_size; i++)
    {  
      if (i != distributed.network_id)
      	{
	  message.rewind();
	  message.append_state_ref(acc_cycle_ref);
	  distributed.network.send_message(message,
						  i,
						  TAG_CANDIDATE_DETECT);
	}
    }
  distributed.network.flush_all_buffers();
}

void visit2(state_t state,state_ref_t predecessor, int depth,int nid)
{
  state_ref_t state_ref;
  appendix_t app;

  depth++;
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, app);
  
  //cycle recovered I have to send to master
  if (((state_ref.id==acc_cycle_ref.id)&&(state_ref.hres==acc_cycle_ref.hres))&&(acc_cycle_is_mine))
    {
     app.p=depth;
     app.parent.state_ref=predecessor;
     app.parent.network_id=nid;
     end_of_iteration = true;
     candidate_given=true;
     app.iteration=iteration;
     st.set_app_by_ref(state_ref, app);

     message.rewind();
     message.append_size_int(depth);
     distributed.network.send_urgent_message(message,
					     0,
					     TAG_CYCLE_RECOVERED);
  
     while(!waiting_queue.empty())
       waiting_queue.pop();			   
   }
  else
   {
     //1st visit of period of this candidate
     if ((app.iteration!=iteration))
      {   
        app.p=depth;
        app.parent.state_ref=predecessor;
        app.parent.network_id=nid;
	app.iteration=iteration;
	waiting_queue.push(state_ref);
      }
   }
  st.set_app_by_ref(state_ref, app);

 }

void send_ce_recovering(int dest, state_t *state,state_ref_t *predecessor,int *nid,
			size_t *depth, const int tag)
{
  message.rewind();
  message.append_state(*state);
  message.append_state_ref(*predecessor);
  message.append_size_int(*nid);
  message.append_size_int(*depth);

  distributed.network.send_message(message,
				   dest,
				   tag);
}

void cycle_find()
{
  state_ref_t state_ref;
  state_t current_state;
  succ_container_t succs(*p_sys);
  state_t succ_state;
  int state_nid;
  if (!waiting_queue.empty()) 
    distributed.set_busy();

  while (!end_of_iteration)
    {
      candidate_given=false;
      while (!candidate_given)
	{
	  if (who_has_candidates==distributed.network_id) //it's my turn to choose the candidate
	    {
	      if (!q_queue.empty()) //I've still some accepting state =>it'll be the candidate
		{
		  acc_cycle_ref=q_queue.front();
		  q_queue.pop();
		  waiting_queue.push(acc_cycle_ref);
		  appendix.p=0;
		  st.set_app_by_ref(acc_cycle_ref,appendix);
		  distributed.set_busy();
		  acc_cycle_is_mine=true;
		  candidate_given=true;
		  broadcast_candidate();
		  //		  cout << distributed.network_id << acc_cycle_ref << "kandidat" << endl;
		}
	      else //I've not accepting vertex so tell computer who_has_candidate+1 to choose it
		{
		  if ((distributed.network_id+1)<distributed.cluster_size)
		    {
		      who_has_candidates=distributed.network_id+1;
		      message.rewind();
		      distributed.network.send_urgent_message(message,
							      distributed.network_id+1,
							      TAG_TRY_TO_FIND_CANDIDATE);
		    }
		  else
		    {
		      //it can't happen because of this function runs if only accepting cycle is detected
		      cerr << "Error when generating counterexample." << endl;
		    }
		}
	    }
	  else //it's not my turn to choose a candidate => acc_cycle_is_mine=false;
	    acc_cycle_is_mine=false;
	  distributed.process_messages();
	}
      
      while (!distributed.synchronized())
	{
	  while (!(waiting_queue.empty()))
	    {
	      distributed.set_busy();
	      if (!end_of_iteration)
		{
		  state_ref = waiting_queue.front();
		  waiting_queue.pop();
		  if (!end_of_iteration)
		    {
		      current_state = st.reconstruct(state_ref);
		      generate_successors(current_state, state_ref, all_enabled_trans, &succs);
		      st.get_app_by_ref(state_ref,appendix);
		      
		      for (std::size_t i = 0; i<succs.size(); i++)
			{
			  succ_state = succs[i];
			  
			  state_nid = distributed.get_state_net_id(succ_state);
			  if (state_nid == distributed.network_id) //state is mine
			    visit2(succ_state, state_ref, appendix.p,distributed.network_id);
			  else //send the message to the owner of the state
			    {
			      send_ce_recovering(state_nid, &(succ_state),
						 &(state_ref),
						 &(distributed.network_id),
						 &(appendix.p),
						 TAG_VISIT2_STATE);
			    }
			  delete_state(succ_state);			  
			}
		      delete_state(current_state);
		    }
		}
	    } //end of "while (!(waiting_queue.empty()))"
	  distributed.network.flush_all_buffers();
	  distributed.process_messages();
	  distributed.set_idle();
	} //end of "while (!distributed.synchronized()
    }//end of while (!end_of_iteration)


  while (!waiting_queue.empty())
      waiting_queue.pop();
  end_of_iteration = false;
  // part of reconstruction of cycle
  if (acc_cycle_is_mine)
   {
     st.get_app_by_ref(acc_cycle_ref,appendix);
     if (appendix.p>0)
      {
	process_ce_cycle_state(acc_cycle_ref, appendix.p);
      }
   }
  while(!end_of_iteration)//it stands for end_of_cycle_reconstruction now
    distributed.process_messages(); //reconstruction of the cycle is performed here

  while(!distributed.synchronized())//now I have to sychronize with other computers
    distributed.process_messages();
}

void process_ce_cycle_state(state_ref_t _ref, int depth)
{
  state_t current_state = st.reconstruct(_ref);

  message.rewind();
  message.append_size_int(depth);
  message.append_state(current_state);
  distributed.network.send_urgent_message(message,
					  0,
					  TAG_SEND_STATE_ON_CE_CYCLE);
  delete_state(current_state);

  if (depth==0) //hence I am at the begining of the cycle
   {
     message.rewind();

     for (int i=0; i<distributed.cluster_size; i++)
	distributed.network.send_urgent_message(message,
						i,
						TAG_CYCLE_RECONSTRUCTED);
   }
  else  //process predecessor of the state
   {
     st.get_app_by_ref(_ref,appendix);

     if (appendix.parent.network_id==distributed.network_id) //predecessor is mine
      {
        process_ce_cycle_state(appendix.parent.state_ref,depth-1);
      }
     else
      {
        depth--;
	message.rewind();
	message.append_state_ref(appendix.parent.state_ref);
	message.append_size_int(depth);
	distributed.network.send_urgent_message(message,
						appendix.parent.network_id,
						TAG_CYCLE_RECONSTRUCTION);
      }
   }
}

void visit1(state_t state, state_ref_t predecessor, size_t depth, int nid)
{
  state_ref_t state_ref;
  appendix_t app;

  depth++;
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, app);

  //path recovered
  if (((state_ref.id==acc_cycle_ref.id))&&(state_ref.hres==acc_cycle_ref.hres))
   {
     app.p=depth;
     app.parent.state_ref=predecessor;
     app.parent.network_id=nid;
     end_of_iteration = true;

     message.rewind();
     message.append_size_int(depth);
     distributed.network.send_urgent_message(message,
					     0,
					     TAG_PATH_RECOVERED);

     while(!waiting_queue.empty())
       waiting_queue.pop();

   }

  if ((app.iteration!=MAX_ULONG_INT)&&(!(end_of_iteration)))
   {
     app.p=depth;
     app.parent.state_ref=predecessor;
     app.parent.network_id=nid;
     app.iteration=MAX_ULONG_INT;
     waiting_queue.push(state_ref);
   }

  st.set_app_by_ref(state_ref, app);
  return;
}

void path_find()
{
  state_ref_t state_ref;
  state_t current_state;
  succ_container_t succs(*p_sys);
  state_t succ_state;
  int state_nid;
  end_of_iteration = false;

  while (!distributed.synchronized())
    {
      if (!(waiting_queue.empty()))
	distributed.set_busy();
      
      do
	{
	  while (!(waiting_queue.empty()))
	    {
	      state_ref = waiting_queue.front();
	      waiting_queue.pop();
	      if (!end_of_iteration)
		{
		  current_state = st.reconstruct(state_ref);
		  generate_successors(current_state, state_ref, all_enabled_trans, &succs);
		  
		  st.get_app_by_ref(state_ref,appendix);
		  for (std::size_t i = 0; i<succs.size(); i++)
		    {
		      succ_state = succs[i];
		      state_nid = distributed.get_state_net_id(succ_state);
		      if (state_nid == distributed.network_id) //state is mine
			{
			  visit1(succ_state, state_ref, appendix.p,distributed.network_id);
			}
		      else //send the message to the owner of the state
			{
			  send_ce_recovering(state_nid, &succ_state,
					     &(state_ref),
					     &(distributed.network_id),
					     &(appendix.p),
					     TAG_VISIT1_STATE);
			}
		      delete_state(succ_state);
		    }
		  delete_state(current_state);
		  distributed.process_messages();
		}
	    } //end of "while (!(waiting_queue.empty()))"
	  distributed.network.flush_all_buffers();
	}
      while (!(waiting_queue.empty())&&(!(end_of_iteration)));
      
      //now I am idle and next iteration should start
      if (end_of_iteration)
	distributed.set_idle();
      else
	distributed.set_busy();
      distributed.process_messages();
    } //end of "while (!(end_of_iteration))"
  
  while (!waiting_queue.empty())
    waiting_queue.pop();
  while (!distributed.synchronized())
    distributed.process_messages();
  end_of_iteration = false;

  //reconstruction of the path
  if (acc_cycle_is_mine)
   {
     st.get_app_by_ref(acc_cycle_ref, appendix);
     if (appendix.p>0)
      {
	process_ce_path_state(acc_cycle_ref, appendix.p);
      }
   }

  while(!end_of_iteration)//it stands for end_of_cycle_reconstruct. now
    distributed.process_messages(); //reconstruction of the path is performed here

  while(!distributed.synchronized())//now I have to sychronize with other computers
    distributed.process_messages();
  return;
}

void process_ce_path_state(state_ref_t _ref, size_t depth)
{
  state_t current_state;
  current_state = st.reconstruct(_ref);

  message.rewind();
  message.append_size_int(depth);
  message.append_state(current_state);
  distributed.network.send_urgent_message(message,
					  0,
					  TAG_SEND_STATE_ON_CE_PATH);
  delete_state(current_state);
  message.rewind();

  if (depth==0) //hence I am at the begining of the path
   {
     for (int i=0; i<distributed.cluster_size; i++)
       distributed.network.send_urgent_message(message,
					       i,
					       TAG_PATH_RECONSTRUCTED);
     distributed.process_messages();			   				   
   }
  else  //process predecessor of the state
   {
     st.get_app_by_ref(_ref,appendix);

     if (appendix.parent.network_id==distributed.network_id) //predecessor is mine
       process_ce_path_state(appendix.parent.state_ref, depth-1);
     else
	{
	  depth--;

	  message.rewind();
	  message.append_state_ref(appendix.parent.state_ref);
	  message.append_size_int(depth);
	  distributed.network.send_urgent_message(message,
						  appendix.parent.network_id,
						  TAG_PATH_RECONSTRUCTION);
	}
   }
  return;
}


int main(int argc, char** argv) 
{

  /* parsing switches ... */
  distributed.network_initialize(argc, argv);  

  int c;
  size_t htsize=0;
  bool produce_report=false;
  bool print_statistics=false;
  bool perform_logging=false;
  bool base_name_is_set=false;
  string set_base_name;
  bool quiet = false;
  bool trail = false;
  bool show_ce = false;  
  int compression=0;
  
  bool fastApproximation = false;


  ostringstream oss,oss1;
  oss1<<"owcty";
  static struct option longopts[] = {
    { "help",       no_argument, 0, 'h'},
    { "quiet",      no_argument, 0, 'q'},
    { "trail",      no_argument, 0, 't'},
    { "report",     no_argument, 0, 'r'},
    { "fast",		no_argument, 0, 'f'},
    { "verbose",    no_argument, 0, 'V'},
    { "log",        no_argument, 0, 'L'},
    { "statelist",  no_argument, 0, 'c'},
    { "simple",     no_argument, 0, 's'},
    { "comp",       required_argument, 0, 'C' },
    { "htsize",     required_argument, 0, 'H' },
    { "basename",   required_argument, 0, 'X' },
    { "version",    no_argument, 0, 'v'},
    { NULL, 0, NULL, 0 }
  };

  while ((c = getopt_long(argc, argv, "cfshqtrvLC:X:H:VS", longopts, NULL)) != -1)
    {
      oss1 <<" -"<<(char)c;
      switch (c) {
      case 'h': usage();return 0; break;
      case 'v': version();return 0; break;
      case 'H': htsize=atoi(optarg); break;
      case 'V':
      case 'S': print_statistics = true; break;
      case 'c': show_ce = true; break;
      case 's': simple = true; break;
      case 'f': fastApproximation = true; break;
      case 'q': quiet = true; break;
      case 'L': perform_logging = true; break;
      case 'X': base_name_is_set = true; set_base_name = optarg; break;
      case 'r': produce_report=true; break;
      case 't': trail=true; break;
      case 'C': compression = atoi(optarg); break;
      case '?': cerr <<"unknown switch -"<<(char)optopt<<endl;
      }
    }  

  if (quiet || simple)
    {
      print_statistics = false;
    }

  if (argc < optind+1)
    {
      usage();
      return 0;
    }

  
  /* decisions about the type of an input */
  char * filename = argv[optind];
  int filename_length = strlen(filename);
  if (filename_length>=4 && strcmp(filename+filename_length-4,".bio")==0)
   {
     if (distributed.network_id==NETWORK_ID_MANAGER && print_statistics)
       {
		 cout << "Reading affine system..." << endl;
       }
     input_file_ext = ".bio";
     p_sys = new affine_explicit_system_t(gerr);
   }

  ce.set_system(p_sys);
  
  /* opening and parsing file ... */
  string file_name;
  int file_opening;
  
  clock_t start, finish;
  double durationInSec = 0.0;
  cout << "\tDURATION: " << durationInSec << "\n";

  try
    {
    	start = clock();
    
		if ((file_opening=p_sys->read(argv[optind],fastApproximation))&&(distributed.network_id==NETWORK_ID_MANAGER))
			{
			
		finish = clock();
		durationInSec = (double)(finish - start) / CLOCKS_PER_SEC;
		cout << "\tDURATION: " << durationInSec << "\n";
			
				if (file_opening==system_t::ERR_FILE_NOT_OPEN)
					gerr << distributed.network_id << ": " << "Cannot open file ...";
				else 
					gerr << distributed.network_id << ": " << "Syntax error ...";
			}
			
		finish = clock();
		durationInSec = (double)(finish - start) / CLOCKS_PER_SEC;
		cout << "\tDURATION: " << durationInSec << "\n";
			
		if (file_opening)
			gerr << thr();
    }
  catch (ERR_throw_t & err)
    { 
		distributed.finalize();
    	return err.id; 
    }
    

  file_name = argv[optind];
  int position = file_name.find(input_file_ext,0);
  file_name.erase(position,4);
  
  if (p_sys->get_with_property() && p_sys->get_property_type()!=BUCHI)
    {
      if (distributed.network_id==NETWORK_ID_MANAGER) 
		{
		  cerr<<"Cannot work with other than standard Buchi accepting condition."<<endl;
		}
      distributed.finalize();
      return 1;
    }
    
  if ((!p_sys->get_with_property())&&(distributed.network_id==NETWORK_ID_MANAGER)&&(!simple))
    {
      cout << "Warning: No property included in model. Switched to simple mode." << endl;
      simple=true;
    }

  /* initialization ... */
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
  
  st.set_appendix(appendix);
  if (compression >=0 && compression <=1)
    {
      if (compression == 0)
        st.set_compression_method(NO_COMPRESS);
      if (compression == 1)
        st.set_compression_method(HUFFMAN_COMPRESS);
    }
  else
    {
      gerr<<"Invalid compression method. Allowed values: \n"
	  <<"  -C0 for no compression,\n"
	  <<"  -C1 for Huffman's compression with static codebook."<<thr();
    }
  st.init();

  distributed.process_user_message = process_message;
  distributed.initialize();
  
  if (produce_report)
    {
      reporter.set_info("InitTime", timer.gettime());
      reporter.start_timer();
    }

  if (perform_logging)
    {
      logger.set_storage(&st);
      logger.register_unsigned(log_wrapper1,"w_queue");
      logger.register_unsigned(log_wrapper2,"q_queue");
      logger.register_unsigned(log_wrapper3,"L_queue");
      logger.register_unsigned(log_wrapper4,"all_qs");

      if (base_name_is_set)
		{
		  logger.init(&distributed,set_base_name);
		}
      else
		{
		  logger.init(&distributed,file_name);
		}
      logger.use_SIGALRM(1);
    }

  state_t state;
  state_ref_t ref;
  succ_container_t succs_cont(*p_sys);
  succ_container_t::iterator i;
 

  // ========================================================
  // ========================================================
  // ========================================================



  state = p_sys->get_initial_state();
  if (distributed.partition_function(state)==distributed.network_id)
    {
      st.insert(state,ref);
      appendix.in_S = p_sys->is_accepting(state);
      appendix.p = 0;
      appendix.iteration = 0;
      appendix.parent.state_ref.invalidate();
      appendix.parent.network_id = -1;
      appendix.ample_set = 0;
      appendix.max_pred_seed.state_ref = succ_ref;
      appendix.max_pred_seed.network_id = distributed.network_id;
      st.set_app_by_ref(ref,appendix);
      waiting_queue.push(ref);
      distributed.set_busy();
      if (appendix.in_S) //perform Reset together with first reachability
		{
		  Ssize ++;
		  q_queue.push(ref);
		}
    }
  delete_state(state);
  
  /* reachability */
  

  if (distributed.network_id == 0 && print_statistics)
    {
	  cout <<"======================================="<<endl;
	  cout <<"Reachability & Reset ..."<<flush;
    }


  while (!distributed.synchronized(info))
	{
	  if (!waiting_queue.empty())
		{
		  ref = waiting_queue.front();
		  waiting_queue.pop();
		  
		  state = st.reconstruct(ref);
		  succs_calls++;
		  succs_cont.clear();
		  p_sys->get_succs(state,succs_cont);

		  for (i=succs_cont.begin(); i!=succs_cont.end(); i++)
			{		
			  trans++;
			  state_t r = *i;	  
			  if (distributed.partition_function(r)!=distributed.network_id)
				{
				  net_state_ref_t nref;
				  nref.state_ref = ref;
				  nref.network_id = distributed.network_id;
				  transcross++;
				  message.rewind();
				  message.append_state(r);
				  message.append_data((byte_t *)&nref,sizeof(net_state_ref_t));
				  distributed.network.send_message(message,
								   distributed.partition_function(r),
								   TAG_SEND_STATE);
				}
			  else
				{
				  state_ref_t r_ref;
				  if (!st.is_stored(r,r_ref))
					{
					  st.insert(r,r_ref);
					  appendix.in_S = p_sys->is_accepting(r);
					  appendix.p = 0;
					  appendix.ample_set = 0;
					  appendix.iteration = 0;
					  appendix.parent.state_ref = ref;
					  appendix.parent.network_id = distributed.network_id;
					  st.set_app_by_ref(r_ref,appendix);		
					  if (appendix.in_S) //perform Reset together with first reachability
						{
						  Ssize ++;
						  q_queue.push(r_ref);
						}
					  waiting_queue.push(r_ref);
					}
				}
			  delete_state(r);
			}
		  delete_state(state);
		  succs_cont.clear();
		}
	  else
		{
#if defined(ORIG_FLUSH)
		  distributed.network.flush_all_buffers();
#else
		  distributed.network.flush_some_buffers();
#endif
		  distributed.set_idle();
		}
	  distributed.process_messages();
	}


  if (distributed.network_id == 0 && print_statistics)
    {
      cout <<" done. ("<<info.data.allstored<<" states)"<<endl;
      cout <<"  Number of states in S: "<<info.data.allSsize<<endl;
      cout <<"  all memory:  "<<info.data.allmem/1024.0<<" MB"<<endl;
    }

  iteration = 0;    
  
  /* while */

  while (!simple && info.data.allSsize != oldSsize && info.data.allSsize >0)
    {
      oldSsize = info.data.allSsize;
      iteration ++;
      if (distributed.network_id == 0 && print_statistics)
		{	 	  
		  cout <<"======================================="<<endl;
		  cout <<"Iteration #"<<iteration<<endl;
		  cout <<"  ------------------------------------- "<<endl;
		  cout <<"  Reachability ..."<<flush;
		}       

      
      /* reachability */
      if (!q_queue.empty())
		{
		  distributed.set_busy();
		}
      while (!distributed.synchronized(info))
		{
		  if (!waiting_queue.empty() || !q_queue.empty())
			{
			  if (!waiting_queue.empty())
				{
				  ref = waiting_queue.front();

				  waiting_queue.pop();	  
				}
			  else
				{
				  ref = q_queue.front();
				  q_queue.pop();
				  st.get_app_by_ref(ref,appendix);
				  if (appendix.p == 0)
					{
					  L_queue.push(ref);
					}
				}
			  
			  state = st.reconstruct(ref);
			  succs_calls++;
			  generate_successors(state, ref, all_enabled_trans, &succs_cont);
			  
			  for (i=succs_cont.begin(); i!=succs_cont.end(); i++)
				{		
				  state_t r = *i;	  
				  if (distributed.partition_function(r)!=distributed.network_id)
					{
					  message.rewind();
					  message.append_state(r);
					  distributed.network.send_message(message,
								  distributed.partition_function(r),
								  TAG_SEND_REACH);
					}
				  else
					{
					  state_ref_t r_ref;
					  st.is_stored(r,r_ref);
					  st.get_app_by_ref(r_ref,appendix);
					  if (!appendix.in_S) 
						{
						  appendix.in_S = true;
						  Ssize ++;
						  waiting_queue.push(r_ref);
						}
					  appendix.p ++;
					  st.set_app_by_ref(r_ref,appendix);		     
					}
				  delete_state(r);				      
				}
			  delete_state(state);
			}
		  else
			{
	#if defined(ORIG_FLUSH)
			  distributed.network.flush_all_buffers();
	#else
			  distributed.network.flush_some_buffers();
	#endif
			  distributed.set_idle();
			}
		  distributed.process_messages();
		}

      while (!L_queue.empty())
		{
		  ref = L_queue.front();
		  L_queue.pop();
		  st.get_app_by_ref(ref,appendix);
		  if (appendix.p==0)
			{
			  waiting_queue.push(ref);
			}	  
		}
      while (!waiting_queue.empty())
		{
		  L_queue.push(waiting_queue.front());
		  waiting_queue.pop();
		}

      while (!distributed.synchronized(info))
		{
		  distributed.process_messages();
		}

      if (distributed.network_id == 0 && print_statistics)
		{	 
		  cout <<"  done."<<endl;
		  cout <<"  Number of states in S: "<<info.data.allSsize<<endl;
		  cout <<"  all memory:  "<<info.data.allmem/1024.0<<" MB"<<endl;
		  cout <<"  ------------------------------------- "<<endl;
		  cout <<"  Elimination ..."<<flush;
		}       
      
      /* elimination */
      if (!L_queue.empty())
		{
		  distributed.set_busy();
		}
      
      while (!distributed.synchronized(info))
		{
		  if (!L_queue.empty())
			{
			  ref = L_queue.front();
			  L_queue.pop();
			  st.get_app_by_ref(ref,appendix);
			  if (appendix.p != 0) //must be here as we deal with L_queue in a different way
				{
				  continue;
				}
			  
			  appendix.in_S = false;
			  st.set_app_by_ref(ref,appendix);
			  Ssize --;
			  
			  state = st.reconstruct(ref);
			  succs_calls++;
			  generate_successors(state, ref, all_enabled_trans, &succs_cont);

			  for (i=succs_cont.begin(); i!=succs_cont.end(); i++)
				{		
				  state_t r = *i;	  
				  if (distributed.partition_function(r)!=distributed.network_id)
					{
					  message.rewind();
					  message.append_state(r);
					  distributed.network.send_message(message,
								  distributed.partition_function(r),
								  TAG_SEND_ELIMI);
					}
				  else
					{
					  state_ref_t r_ref;
					  st.is_stored(r,r_ref);
					  st.get_app_by_ref(r_ref,appendix);		      
					  appendix.p --;
					  st.set_app_by_ref(r_ref,appendix);
					  if (appendix.p == 0)
						{
						  L_queue.push(r_ref);
						}
					}
				  delete_state(r);
				}
			  delete_state(state);	      
			}
		  else
			{
	#if defined(ORIG_FLUSH)
			  distributed.network.flush_all_buffers();
	#else
			  distributed.network.flush_some_buffers();
	#endif
			  distributed.set_idle();
			}
		  distributed.process_messages();
		}
      
      if (distributed.network_id == 0 && print_statistics)
		{	 
		  cout <<" done."<<endl;
		  cout <<"  Number of states in S: "<<info.data.allSsize<<endl;
		  cout <<"  all memory:  "<<info.data.allmem/1024.0<<" MB"<<endl;
		}       

      if (info.data.allSsize>0) //otherwise Reset isn't necessary
		{
		  if (distributed.network_id == 0 && print_statistics)
			{	 
			  cout <<"  -------------------------------------"<<endl;
			  cout <<"  Reset ..."<<flush;
			}

		  /* reset */
		  Ssize = 0;
		  
		  state = p_sys->get_initial_state();
		  if (distributed.partition_function(state) == distributed.network_id)
			{
			  st.is_stored(state,ref);
			  st.get_app_by_ref(ref,appendix);
			  appendix.in_S = appendix.in_S && p_sys->is_accepting(state);
			  appendix.iteration = iteration;
			  appendix.p = 0;
			  st.set_app_by_ref(ref,appendix);
			  waiting_queue.push(ref);
			  if (appendix.in_S)
				{
				  Ssize ++;
				  q_queue.push(ref);			      
				}
			  distributed.set_busy();
			}
		  delete_state(state);
		  
		  while (!distributed.synchronized(info))
			{
			  if (!waiting_queue.empty())
				{
				  ref = waiting_queue.front();
				  waiting_queue.pop();
				  state = st.reconstruct(ref);
				  succs_calls++;
				  generate_successors(state, ref, all_enabled_trans, &succs_cont);
				  state_ref_t r_ref;
				  for (i=succs_cont.begin(); i!=succs_cont.end(); i++)
					{		
					  state_t r = *i;	  
					  if (distributed.partition_function(r)!=distributed.network_id)
						{
						  message.rewind();
						  message.append_state(r);
						  distributed.network.send_message(message,
										   distributed.partition_function(r),
										   TAG_SEND_RESET);
						}
					  else
						{
						  st.is_stored(r,r_ref);
						  st.get_app_by_ref(r_ref,appendix);
						  if (appendix.iteration < iteration)
							{
							  appendix.in_S = appendix.in_S && p_sys->is_accepting(r);
							  appendix.iteration = iteration;
							  appendix.p = 0;
							  st.set_app_by_ref(r_ref,appendix);		     
							  waiting_queue.push(r_ref);
							  if (appendix.in_S)
								{
								  Ssize ++;
								  q_queue.push(r_ref);			      
								}
							}
						}
					  delete_state(r);
					}
				  delete_state(state);
				}
			  else
				{
		#if defined(ORIG_FLUSH)
				  distributed.network.flush_all_buffers();
		#else
				  distributed.network.flush_some_buffers();
		#endif
				  distributed.set_idle();
				}
			  distributed.process_messages();
			}
		  
		  if (distributed.network_id == 0 && print_statistics)
			{	 	  
			  cout <<" done."<<endl;
			  //  	  cout <<"  Size of q_queue: "<<info.data.allinq<<endl;
			  cout <<"  Number of states in S: "<<info.data.allSsize<<endl;
			  cout <<"  all memory:  "<<info.data.allmem/1024.0<<" MB"<<endl;
			}
		} //end of "if (info.data.allSsize>0)"
    }

  if (perform_logging)
    {
      logger.stop_SIGALRM();
      logger.log_now();
    }


  /* printing statistics ... */
  if (!quiet)
    {
      if (distributed.network_id == 0 && !quiet)
		{
		  if (!simple)
			{
			  cout <<"======================================="<<endl;	  
	  	      if (info.data.allSsize !=0) 
				{
				  cout <<"       --- Accepting cycle ---"<<endl;
				}     
			  else
				{
				  cout <<"      --- No accepting cycle ---"<<endl;
				}
			  cout <<"======================================="<<endl;	  
			}
		}
    }

  
  /* counterexample generation */

  if (info.data.allSsize!=0 && (trail || show_ce))
    {
      if (distributed.network_id == 0 && print_statistics)
		{
		  cout <<"Generating counterexample ..."<<flush;
		}
      
      cycle_find(); //detect an accepting state lying in in_S and on a cycle

      //I own an initial state so I can start to find a path to a cycle
      state_t s0 = p_sys->get_initial_state();
      if (distributed.get_state_net_id(s0) == distributed.network_id)
		{
		  appendix.p=0;
		  appendix.in_S=false;
		  st.is_stored(s0,state_ref);
		  st.set_app_by_ref(state_ref, appendix);
		  distributed.set_busy();
		  waiting_queue.push(state_ref);
		}
      else
		distributed.set_idle();
      
      end_of_iteration = false; //it stands for end_of_path_recovering now
      path_find();
      delete_state(s0);
      //output
      if (distributed.network_id == 0)
		{
		  path_t ce(p_sys);
		  ofstream ce_out;
		  string pom_fn;
		  if ((show_ce)||(trail))
			{
			  if (path_state[0]==path_state[1])
				{
				  delete_state(path_state[1]);
				  path_length--;
				}
			  for (size_int_t i=0; i<path_length-1; i++)
				ce.push_back(path_state[i]);
			  ce.push_back(cycle_state[0]);
			  ce.mark_cycle_start_back();
			  for (size_int_t i=1; i<cycle_length-1; i++)
				ce.push_back(cycle_state[i]);
			  ofstream ce_out;
			  string pom_fn;
			  if (trail)
				{
				  if (base_name_is_set)
					{
					  pom_fn = set_base_name+".trail";
					}
				  else
					{
					  pom_fn = file_name+".trail";
					}
				  ce_out.open(pom_fn.c_str());
				  if (!p_sys->can_system_transitions())
					ce.write_states(ce_out);
				  else
					ce.write_trans(ce_out);
				  ce_out.close();
				}
			  
			  if (show_ce)
				{
				  if (base_name_is_set)
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
		  for (size_t i=0; i<cycle_length; i++)
			delete_state(cycle_state[i]);
		  delete[] cycle_state;
		  for (size_t i=0; i<path_length; i++)
			delete_state(path_state[i]);
		  delete[] path_state;
		}

      if (distributed.network_id == 0 && print_statistics)
		{
		  cout <<" done."<<endl;
		  cout <<"---------------------------------------"<<endl;
		}     
    }

  if (!quiet && distributed.network_id==0)
    {
      cout <<"States:\t\t\t"<<info.data.allstored<<endl;

      cout <<"transitions:\t\t"<<info.data.alltrans<<endl;
      cout <<"iterations:\t\t"<<iteration<<endl;
      state = p_sys->get_initial_state();
      cout <<"size of a state:\t"<<state.size<<endl;
      cout <<"size of appendix:\t"<<sizeof(appendix)<<endl;
      delete_state(state);
      
      if (htsize != 0)
		cout <<"hashtable size:\t\t"<<htsize<<endl;
		
      cout <<"cross transitions:\t"<<info.data.allcross<<endl;
      cout <<"all memory:\t\t"<<info.data.allmem/1024.0<<" MB"<<endl;
      cout <<"time:\t\t\t"<<timer.gettime()<<" s"<<endl;
      cout <<"------------------------"<<endl;
    }
    
  if (!quiet)
    {
      distributed.network.barrier();
      cout <<distributed.network_id<<": local states:      "<<st.get_states_stored()<<endl;
    }
  
  if (!quiet)
    {
      distributed.network.barrier();
      cout <<distributed.network_id<<": local memory:      "<<vm.getvmsize()/1024.0<<endl;
    }

  if (produce_report)
    {
      ofstream ce_out;
      string pom_fn;
      if (base_name_is_set)
		{
		  pom_fn = set_base_name + ".report";
		}
      else
		{
		  pom_fn = file_name+".report";
		}
      if (distributed.network_id == 0)
		{
		  ce_out.open(pom_fn.c_str());
		}
      reporter.stop_timer();
      string problem = simple?"SSGen":"LTL MC";
      reporter.set_obligatory_keys(oss1.str(), argv[optind], problem, st.get_states_stored(), succs_calls);
      reporter.set_info("States", st.get_states_stored());
      reporter.set_info("Trans", trans);
      reporter.set_info("CrossTrans", transcross);      

      if (distributed.network_id==0) //set global information to reporter
		{
		  if (!simple)
			{

			  if (info.data.allSsize!=0)
				{
				  reporter.set_global_info("IsValid", "No");
				  if (info.data.allSsize!=0 && (trail || show_ce))
					{
					  reporter.set_global_info("CEGenerated", "Yes");
					}
				  else
					reporter.set_global_info("CEGenerated", "No");
				}
			  else
				{
				  reporter.set_global_info("IsValid", "Yes");
				  reporter.set_global_info("CEGenerated", "No");
				}
			}
		}
      
      reporter.collect_and_print(REPORTER_OUTPUT_LONG, ce_out);
      if (distributed.network_id == 0)
		{
		  ce_out.close();
		}
    }
  
  delete p_sys;
  distributed.finalize();
  return 0;
}
