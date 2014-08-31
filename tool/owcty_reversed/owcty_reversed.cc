#include <getopt.h>
#include <sstream>
#include <iostream>
#include <queue>
#include <stack>
#include "divine.h"

using namespace std;
using namespace divine;

typedef struct net_state_ref_t
{
  state_ref_t state_ref;
  int network_id; 
};

typedef struct appendix_t 
{
  short int remains;
  unsigned short int visited;
  unsigned short int inset;
  bool is_accepting;
  array_t<net_state_ref_t> *predecessors;
};

const int TAG_SEND_STATE = DIVINE_TAG_USER +0;
const int TAG_BACK_PROPAGATE = DIVINE_TAG_USER +1;
const int TAG_BACK_MARK = DIVINE_TAG_USER +2;
const int TAG_CE_COMPLETE = DIVINE_TAG_USER +4;
const int TAG_CE_TRY = DIVINE_TAG_USER +5;;
const int TAG_CE_WAY = DIVINE_TAG_USER +6;
const int TAG_CE_NOWAY = DIVINE_TAG_USER +7;

const char * input_file_ext = 0;

distributed_t distributed;
distr_reporter_t reporter(&distributed);

explicit_system_t *sys=0;

explicit_storage_t st;
timeinfo_t timer;
vminfo_t vm;
logger_t logger;
bool simple = false;


//////char BUFFER[2048];
message_t message;
message_t received_message;

appendix_t appendix;
queue<state_ref_t> waiting_queue;
queue<state_ref_t> backtracking_queue;
queue<state_ref_t> marking_queue;

stack<state_t> ce_stack;
path_t ce;
bool ce_complete = false;
bool ce_token = false;
bool ce_trying = false;

size_int_t succs_calls=0, preds_calls=0;
size_t trans=0,transcross=0;
size_t eliminated=0;
size_t visited = 0;
size_t marked=0,oldmarked=0;
unsigned short int iteration=1;


unsigned log_wrapper1()
{
  return waiting_queue.size();
}


struct info_t {
  size_t allstored;
  size_t allstates;
  size_t alltrans;
  size_t allcross;
  long int allmem;
  size_t alleliminated;
  size_t allmarked;
  size_t allmarking_queues;
  virtual void update();
  virtual ~info_t() {}
};

void info_t::update()
{
  if (distributed.network_id == 0)
    {
      allstored = st.get_states_stored();
      allstates = visited;
      alltrans = trans;
      allcross = transcross;
      allmem = vm.getvmsize();
      alleliminated = eliminated;
      allmarked = marked;
      allmarking_queues = marking_queue.size();
    }
  else
    {
      allstored += st.get_states_stored();
      allstates += visited;
      alltrans += trans;
      allcross += transcross;
      allmem += vm.getvmsize();
      alleliminated += eliminated;
      allmarked += marked;
      allmarking_queues += marking_queue.size();
    }
}

updateable_info_t<info_t> info;


void process_message(char *buf, int size, int src, int tag, bool urgent) 
{
  state_t state_received;
  state_ref_t ref;
  net_state_ref_t net_ref_received;
  
  switch (tag) {
  case TAG_SEND_STATE:
    received_message.load_data((byte_t*)buf,size);
    received_message.read_state(state_received);
    received_message.read_state_ref(net_ref_received.state_ref);
    net_ref_received.network_id = src;
			 
    if (!st.is_stored(state_received,ref))  //very first iteration
      {
	st.insert(state_received,ref);
 	appendix.inset = iteration;
	appendix.visited = 0;
	if (!simple)
	  {
	    appendix.predecessors = new array_t<net_state_ref_t>(1,1);
	    if (sys->is_accepting(state_received))
	      {
		appendix.is_accepting = true;
	      }
	    else
	      {
		appendix.is_accepting = false;
	      }
	  }
      }
    else // a later iteration
      {
	st.get_app_by_ref(ref,appendix);
      }		      

    if (appendix.inset != iteration)
      {
 	distributed.network.send_message((char *)(&(net_ref_received.state_ref)),
				    sizeof(state_ref_t),
				    src,
				    TAG_BACK_PROPAGATE);
      }
    else
      {      
	if (appendix.visited == iteration)   // already explored in this iteration
	  {
	    if (!simple)
	      {
		if (appendix.remains != 0)
		  {
		    appendix.predecessors->push_back(net_ref_received);
		  }
		else
		  {
		    distributed.network.send_message((char *)(&(net_ref_received.state_ref)),
						sizeof(state_ref_t),
						src,
						TAG_BACK_PROPAGATE);
		  }
	      }
	  }
	else // new in this iteration
	  {
	    visited ++;
	    appendix.visited = iteration;
	    if (!simple)
	      {
		appendix.predecessors->clear();
		appendix.predecessors->push_back(net_ref_received);
		appendix.remains = -1; // this will be redefined
		if (appendix.is_accepting && !simple)
		  {
		    marking_queue.push(ref);
		  }
	      }
	    st.set_app_by_ref(ref,appendix);
	    waiting_queue.push(ref);
	    distributed.set_busy();
	  }
      }
    delete_state(state_received);
    break;
  case TAG_BACK_PROPAGATE:
    memcpy(&ref,buf,sizeof(state_ref_t));
    st.get_app_by_ref(ref,appendix);
    if (appendix.remains != 0)
      {
	appendix.remains --;
	st.set_app_by_ref(ref,appendix);
	if (appendix.remains == 0)
	  {
	    backtracking_queue.push(ref);
	    distributed.set_busy();
	  }
      }
    break;    
  case TAG_BACK_MARK:
    memcpy(&ref,buf,sizeof(state_ref_t));
    st.get_app_by_ref(ref,appendix);
    if (appendix.inset == iteration && appendix.remains >0)
      {
	appendix.inset = iteration + 1;
	marked ++;
	st.set_app_by_ref(ref,appendix);
	marking_queue.push(ref);
	distributed.set_busy();
      }
    break;

  case TAG_CE_TRY:
    state_received = new_state(buf, size);
    st.is_stored(state_received,ref);
    delete_state(state_received);
    st.get_app_by_ref(ref,appendix);
    if (appendix.inset <iteration)
      {
 	distributed.network.send_message(NULL,0,0,TAG_CE_NOWAY);
	distributed.network.flush_buffer(0);
	break;
      }
    if (appendix.inset == iteration)
      {
 	distributed.network.send_message(NULL,0,0,TAG_CE_WAY);
	distributed.network.flush_buffer(0);
	appendix.inset = iteration+1;
	st.set_app_by_ref(ref,appendix);
	break;
      }
    else //appendix.inset==iteration+1;
      {
	distributed.network.send_message(NULL,0,0,TAG_CE_COMPLETE);
	distributed.network.flush_buffer(0);
	break;
      }
    break;

  case TAG_CE_NOWAY:
    ce_token = true;
    break;

  case TAG_CE_WAY:
    ce_trying = false;
    ce_token = true;
    break;

  case TAG_CE_COMPLETE:
    ce_trying = false;
    ce_token = true;
    ce_complete = true;

    if (distributed.network_id == 0)
      {
	for (int i=1; i<distributed.cluster_size; i++)
	  {
	    distributed.network.send_message(NULL,0,i,TAG_CE_COMPLETE);
	  }
	distributed.network.flush_all_buffers();
      }
    break;
  }
}


void version()
{
  cout <<"OWCTY reversed ";
  cout <<"version 1.0 build 7  (2006/09/20 17:00)"<<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  version();
  cout <<"-----------------------------------------------------------------"<<endl;

  cout <<"Usage: [mpirun -np N] owcty_reversed [options] input_file"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v,--version\t\tshow version"<<endl;
  cout <<" -h,--help\t\tshow this help"<<endl;
  cout <<" -H x,--htsize x\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -V,--verbose\t\tprint some statistics"<<endl;
  cout <<" -q,--quiet\t\tquite mode"<<endl;
  cout <<" -c, --statelist \t show counterexample states"<<endl;
  cout <<" -t,--trail\t\tproduce trail file"<<endl;
  cout <<" -f, --fast\t\tuse faster but less accurate algorithm for abstraction" << endl;
  cout <<" -r,--report\t\tproduce report file"<<endl;
  cout <<" -R,--remove\t\tremoves transitions while backpropagating (very slow, saves memory)"
       <<endl;
  cout <<" -s,--simple\t\tperform simple reachability only"<<endl;
  cout <<" -L,--log\t\tproduce logfiles (log period is 1 second)"<<endl;
  cout <<" -X w\t\t\tsets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
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
  bool remove_trans=false;
  string set_base_name;
  bool quiet = false;
  bool trail = false;
  bool show_ce = false;

	bool fastApproximation = false;


  static struct option longopts[] = {
    { "help",       no_argument, 0, 'h'},
    { "quiet",      no_argument, 0, 'q'},
    { "trail",      no_argument, 0, 't'},
    { "report",     no_argument, 0, 'r'},
    { "fast",		no_argument, 0, 'f'},
    { "verbose",    no_argument, 0, 'V'},
    { "log",        no_argument, 0, 'L'},
    { "remove",     no_argument, 0, 'R'},
    { "simple",     no_argument, 0, 's'},
    { "statelist",  no_argument, 0, 'c'},
    { "htsize",     required_argument, 0, 'H' },
    { "basename",   required_argument, 0, 'X' },
    { "version",    no_argument, 0, 'v'},
    { NULL, 0, NULL, 0 }
  };

  ostringstream oss,oss1;
  oss1<<"owcty_reversed";
  while ((c = getopt_long(argc, argv, "csfRhqtrvLX:H:VS", longopts, NULL)) != -1)
    {
      oss1 <<" -"<<(char)c;
      switch (c) {
      case 'h': usage();return 0; break;
      case 'v': version();return 0; break;
      case 'H': htsize=atoi(optarg); break;
      case 'R': remove_trans = true; break;
      case 'V':
      case 'S': print_statistics = true; break;
      case 's': simple = true; break;
      case 'q': quiet = true; break;
      case 'f': fastApproximation = true; break;
      case 'c': show_ce = true; break;
      case 'L': perform_logging = true; break;
      case 'X': base_name_is_set = true; set_base_name = optarg; break;
      case 'r': produce_report=true; break;
      case 't': trail=true; break;
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
    if (distributed.network_id==NETWORK_ID_MANAGER)
      cout << "Reading Affine System ..." << endl;
    input_file_ext = ".bio";
    sys = new affine_explicit_system_t(gerr);
   }
  
  ce.set_system(sys);
  
  /* opening and parsing file ... */
  string file_name;
  int file_opening;
  try
    {
      if ((file_opening=sys->read(argv[optind],fastApproximation))&&(distributed.network_id==NETWORK_ID_MANAGER))
	{
	  if (file_opening==system_t::ERR_FILE_NOT_OPEN)
	    gerr << distributed.network_id << ": " << "Cannot open file ...";
	  else 
	    gerr << distributed.network_id << ": " << "Syntax error ...";
	  if (file_opening)
	    gerr << thr();
	}
    }
  catch (ERR_throw_t & err)
    { 
      distributed.finalize();
      return err.id; 
    }

  file_name = argv[optind];
  int position = file_name.find(input_file_ext,0);
  file_name.erase(position,4);
  
  if (sys->get_with_property() && sys->get_property_type()!=BUCHI)
    {
      if (distributed.network_id==NETWORK_ID_MANAGER) 
	{
	  cerr<<"Cannot work with other than standard Buchi accepting condition."<<endl;
	}
      distributed.finalize();
      return 1;
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
      logger.register_unsigned(log_wrapper1,"queue");
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
  
  if (print_statistics && distributed.network_id==0)
    {
      cout <<"initialization time:  "<<timer.gettime()<<" s"<<endl;
    }
  
  state_t state;
  state_ref_t ref;
  succ_container_t succs_cont(*sys);
  succ_container_t::iterator i;
 

  // ========================================================
  // ========================================================
  // ========================================================


  do
    {
      
      if (distributed.network_id == 0 && print_statistics)
	{
	  cout <<endl;
	  cout <<"======================================"<<endl;
	  cout <<" Iteration: "<<iteration<<endl;
	  cout <<"======================================"<<endl;
	}

      visited = 0;
      eliminated = 0;

      state = sys->get_initial_state();
      if (distributed.partition_function(state)==distributed.network_id)
	{
	  if (!st.is_stored(state,ref))  //very first iteration
	    {
	      st.insert(state,ref);
	      appendix.inset = iteration;
	      appendix.visited = 0;
	      if (!simple)
		{
		  appendix.predecessors = new array_t<net_state_ref_t>(1,1);
		  if (sys->is_accepting(state))
		    {
		      appendix.is_accepting = true;
		    }
		  else
		    {
		      appendix.is_accepting = false;
		    }
		}
	    }
	  else
	    {
	      st.get_app_by_ref(ref,appendix);
	    }

	  if (appendix.inset == iteration) // can be processed in this iteration 
	                                    // probably useless for the initial state
	    {
	      visited ++;
	      if (!simple)
		{
		  appendix.predecessors->clear();
		  if (appendix.is_accepting)
		    {
		      marking_queue.push(ref);
		    }
		  appendix.remains = -1; // this will be redefined
		}
	      appendix.visited = iteration;
	      st.set_app_by_ref(ref,appendix);
	      waiting_queue.push(ref);
	      distributed.set_busy();
	    }
	}
      delete_state(state);

      while (!distributed.synchronized(info))  
	{		
	  distributed.process_messages();
	  
	  while(!backtracking_queue.empty())
	    {
	      eliminated ++;
	      ref = backtracking_queue.front();
	      backtracking_queue.pop();
	      st.get_app_by_ref(ref,appendix);
	      
	      preds_calls++;
	      array_t<net_state_ref_t>::iterator l_end = appendix.predecessors->end();
	      for (array_t<net_state_ref_t>::iterator i = appendix.predecessors->begin();
		   i != l_end; i++)
		{
		  net_state_ref_t p = *i;
		  if (p.network_id == distributed.network_id)
		    {
		      static_cast<appendix_t*>(st.app_by_ref(p.state_ref))->remains--;
		      if (static_cast<appendix_t*>(st.app_by_ref(p.state_ref))->remains==0)
				{
				  backtracking_queue.push(p.state_ref);
				}
		    }	 
		  else
		    {
		      distributed.network.send_message((char *)(&(p.state_ref)),sizeof(state_ref_t),
						  p.network_id,
						  TAG_BACK_PROPAGATE);
		    }
		}
	      if (remove_trans)
		{
		  delete appendix.predecessors;
		  appendix.predecessors = 0;
		  st.set_app_by_ref(ref,appendix);
		}
	    }
	  
	  if (!waiting_queue.empty())
	    { 
	      ref = waiting_queue.front();
	      waiting_queue.pop();
	      state = st.reconstruct(ref);
	      sys->get_succs(state,succs_cont);
	      succs_calls++;
	      
	      if (!simple)
			{
			  (static_cast<appendix_t*>(st.app_by_ref(ref)))->remains = succs_cont.size();
			}	      
			
	      if (succs_cont.size() == 0)
			{
			  if (!simple)
				{
				  backtracking_queue.push(ref);
				}
			}
	      else
			{	      
			  for (i=succs_cont.begin(); i!=succs_cont.end(); i++)
				{		
				  state_t r = *i;
				  if (iteration == 1)
					{
					  trans++;
					}
					
				  if (distributed.partition_function(r)!=distributed.network_id)
					{
					  if (iteration == 1)
						{
						  transcross++;
						}
	                  message.rewind();
	                  message.append_state(r);
	                  message.append_state_ref(ref);
				                  
					  distributed.network.send_message(message,
									  distributed.partition_function(r),
									  TAG_SEND_STATE);
					}
				  else
					{
					  state_ref_t r_ref;
					  net_state_ref_t net_ref;
					  net_ref.network_id = distributed.network_id;
					  net_ref.state_ref = ref;
					  
		 			  if (!st.is_stored(r,r_ref))  //first visit in first iteration
						{
						  st.insert(r,r_ref);
						  appendix.inset = iteration;
						  appendix.visited = 0;
						  if (!simple)
							{
							  appendix.predecessors = new array_t<net_state_ref_t>(1,1);
							  if (sys->is_accepting(r))
								{
								  appendix.is_accepting = true;
								}
							  else
								{
								  appendix.is_accepting = false;
								}
							}
						}
		 			  else // a later iteration
						{
						  st.get_app_by_ref(r_ref,appendix);
						  if (appendix.inset != iteration)
							{
							  appendix.remains = 0;
							}
						}		      
					  
					  if (appendix.inset == iteration && appendix.visited != iteration)
		 			    {
						  visited ++;
						  if (!simple)
							{
							  appendix.predecessors->clear();
							  appendix.predecessors->push_back(net_ref);
							  if (appendix.is_accepting)
								{
								  marking_queue.push(r_ref);
								}
							}
						  appendix.remains = -1; // this will be redefined
						  appendix.visited = iteration;
						  st.set_app_by_ref(r_ref,appendix);
						  waiting_queue.push(r_ref);
						}
					  else
						{
						  if (!simple)
							{
							  if (appendix.remains != 0) // => appendix.inset == iteration
								{
								  appendix.predecessors->push_back(net_ref);
								  st.set_app_by_ref(r_ref,appendix);
								}				
							  else
								{
								  st.set_app_by_ref(r_ref,appendix);
								  st.get_app_by_ref(ref,appendix);
								  appendix.remains --;
								  if (appendix.remains == 0)
									{
									  backtracking_queue.push(ref);
									}
								  st.set_app_by_ref(ref,appendix);
								}
							}
						}
					}
				  delete_state(r);
				}
			}
	      delete_state(state);	   	    
	    }
	  else
	    {
	      if (backtracking_queue.empty())
		{
		  distributed.set_idle();
		}
	    }
	}

      if (distributed.network_id == 0 && print_statistics)
	{
	  cout <<"                                                                          \r";
	  cout <<"states generated:  "<<info.data.allstates<<endl;
	  cout <<"states eliminated: "<<info.data.alleliminated<<endl;
	  cout <<"states remaining:  "<<info.data.allstates-info.data.alleliminated<<endl;
	  cout <<"markinq_queues:    "<<info.data.allmarking_queues<<endl;
	  cout <<"time:              "<<timer.gettime()<<" s"<<endl;
	}

      marked = 0;

//        cout <<"Local queue: "<<marking_queue.size()<<endl;

      while (!distributed.synchronized(info))
	{
	  while(!marking_queue.empty())
	    {
	      ref = marking_queue.front();
	      marking_queue.pop();
	      st.get_app_by_ref(ref,appendix);

	      if (appendix.remains == 0)
			{
			  continue;
			}
	      
	      preds_calls++;
	      array_t<net_state_ref_t>::iterator l_end = appendix.predecessors->end();
	      for (array_t<net_state_ref_t>::iterator i = appendix.predecessors->begin();
		   i != l_end; i++)
		{
		  net_state_ref_t p = *i;
		  
		  if (p.network_id == distributed.network_id)
		    {
		      st.get_app_by_ref(p.state_ref,appendix);

		      if (appendix.inset == iteration && appendix.remains >0)
				{
	//  			  cout <<" YES"<<endl;
				  delete_state(state);
				  appendix.inset = iteration+1;
				  marked ++;
				  marking_queue.push(p.state_ref);		      
				}
		      else
				{
	//  			  cout <<" NO"<<endl;
				}
		      st.set_app_by_ref(p.state_ref,appendix);
		    }	 
		  else
		    {
//  		      cout <<" - predecessor is remote"<<endl;
		      distributed.network.send_message((char *)(&(p.state_ref)),sizeof(state_ref_t),
						  p.network_id,
						  TAG_BACK_MARK);		  
		    }
		}
	    }
	  distributed.set_idle();
	  distributed.process_messages();
	}
      
      if (distributed.network_id == 0 && print_statistics)
	{
	  cout <<"-------------------"<<endl;
	  cout <<"states marked:     "<< info.data.allmarked<<endl;
	  cout <<"time:              "<<timer.gettime()<<" s"<<endl;
	}

      iteration ++;

     } while (info.data.allmarked != (info.data.allstates-info.data.alleliminated) 
	     && info.data.allmarked!=0 && !simple);  
      
  // ========================================================
  // ========================================================
  // ========================================================

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
	  if (print_statistics)
	    {
	      cout <<endl;
	    }
	  cout <<"======================================="<<endl;
	  if (simple)
	    {
	      cout <<" Cannot detect cycles in simple mode!"<<endl;
	    }
	  else
	    {
	      if (info.data.allmarked !=0) 
		{
		  cout <<"        --- Accepting cycle ---"<<endl;
		}     
	      else
		{
		  cout <<"      --- No accepting cycle ---"<<endl;
		}
	    }
	  cout <<"======================================="<<endl;	  
	}
    }


  /* counterexample generation */
  if (info.data.allmarked !=0 && (trail || show_ce))
    {
      state = sys->get_initial_state();
      if (distributed.partition_function(state) == distributed.network_id)
	{
	  st.is_stored(state,ref);
	  st.get_app_by_ref(ref,appendix);
	  appendix.inset = iteration+1;
	  st.set_app_by_ref(ref,appendix);
	}

      if (distributed.network_id == 0)
	{
	  ce_stack.push(state);
	  while (!ce_complete)
	    {
	      state = ce_stack.top();
	      sys->get_succs(state,succs_cont);
	      succ_container_t::iterator i = succs_cont.begin();
	      
	      ce_trying = true;
	      while (ce_trying)
		{
		  distributed.network.send_message((*i).ptr,(*i).size,
					      distributed.partition_function(*i),
					      TAG_CE_TRY);
		  distributed.network.flush_buffer(distributed.partition_function(*i));
		  ce_token = false;
		  while (!ce_token)
		    {
		      distributed.process_messages();
		    }		  
		  if (!ce_trying) // succeeded
		    {
		      ce_stack.push(*i);
		    }
		  else
		    {
		      delete_state(*i);
		    }
		  i++;
		}
	      while (i!=succs_cont.end())
		{
		  delete_state (*i);
		  i++;
		}
	    }

	  state = ce_stack.top();
	  ce_stack.pop();
	  state_t state2;
	  while (!ce_stack.empty())
	    {
	      state2=ce_stack.top();
	      ce.push_front(state2);
	      if (memcmp(state.ptr,(ce_stack.top()).ptr,state.size)==0)
			{
	//		  cout << "================" << endl;
			  ce.mark_cycle_start_front();
			}
	      ce_stack.pop();
	      delete_state(state2);
	    }	  
	  delete_state(state);
	}
      else
	{
	  delete_state(state);
	  while (!ce_complete)
	    {
	      distributed.process_messages();
	    }
	}
    }

  if (!quiet && distributed.network_id==0)
    {
      cout <<"states:            "<<info.data.allstored<<endl;
      cout <<"transitions:       "<<info.data.alltrans<<endl;
      cout <<"iterations:        "<<iteration-1<<endl;
      state = sys->get_initial_state();
      cout <<"size of a state:   "<<state.size<<endl;
      cout <<"size of appendix:  "<<sizeof(appendix)<<endl;
      delete_state(state);
      if (htsize != 0)
	cout <<"hashtable size     "<<htsize<<endl;
      cout <<"cross transitions: "<<info.data.allcross<<endl;
      cout <<"all memory         "<<info.data.allmem/1024.0<<" MB"<<endl;
      cout <<"time:              "<<timer.gettime()<<" s"<<endl;
      cout <<"-------------------"<<endl;
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

  if (trail && info.data.allmarked!=0)
    {
      ofstream ce_out;
      string pom_fn;
      if (base_name_is_set)
	{
	  pom_fn = set_base_name + ".trail";
	}
      else
	{
	  pom_fn = file_name+".trail";
	}
      ce_out.open(pom_fn.c_str());
      if (sys->can_system_transitions())
	ce.write_trans(ce_out);
      else
	ce.write_states(ce_out);	
      ce_out.close();
    }

  if (show_ce && info.data.allmarked!=0)
    {
      ofstream ce_out;
      string pom_fn;
      if (base_name_is_set)
	{
	  pom_fn = set_base_name + ".ce_states";
	}
      else
	{
	  pom_fn = file_name+".ce_states";
	}
      ce_out.open(pom_fn.c_str());
      ce.write_states(ce_out);
      ce_out.close();
    }

  if (produce_report)
    {
      ofstream rep_out;
      string pom_fn;
      if (base_name_is_set)
	{
	  pom_fn = set_base_name + ".report";
	}
      else
	{
	  pom_fn = file_name+".report";
	}
      rep_out.open(pom_fn.c_str());
      reporter.stop_timer();
      string problem=simple?"SSGen":"LTL MC";
      reporter.set_obligatory_keys(oss1.str(), argv[optind], problem, st.get_states_stored(), succs_calls);
      reporter.set_info("States", st.get_states_stored());
      reporter.set_info("Trans", trans);
      reporter.set_info("CrossTrans", transcross);      
      reporter.set_info("PredsCalls", preds_calls);
      if  ((distributed.network_id==0) && (!simple))
	{
	  if (info.data.allmarked !=0)
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

      reporter.collect_and_print(REPORTER_OUTPUT_LONG, rep_out);
      if (distributed.network_id==0)
	rep_out.close();
    }
  
  distributed.finalize();
  delete sys;
  return 0;
}


