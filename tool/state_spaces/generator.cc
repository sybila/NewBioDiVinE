#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include "sevine.h"
#include <queue>

using namespace std;
using namespace divine;

// static vminfo_t vm;

// float get_consumed_memory()
// { return float(double(vm.getvmsize())/1024); }

int main(int argc, char** argv) {  
  char c;
  bool print_acc= false, print_trans=false, print_dot=false, print_help=false,
       print_graph=false, print_states=false, print_statistics = false,
       print_deadlock=false, print_leading_to_error=false;
  int compression=NO_COMPRESS;
       
  int verbose_level =0, states=0, transitions=0, counter=1;
  int hash_table = 65536;
  bool quiet_and_quick = false;
  size_t deadlock_count = 0;
  size_t error_count = 0;
  explicit_storage_t st;
  
  while ((c = getopt(argc, argv, "ahdqtv:sSH:C:gDE")) != -1) {
    switch (c) {
    case 'a': print_acc = true;break;
    case 't': print_trans = true;break;
    case 'd': print_dot = true;break;
    case 's': print_states = true;break;
    case 'S': print_statistics = true;break;
    case 'v': verbose_level = atoi(optarg); break;
    case 'q': quiet_and_quick = true;break;
    case 'h': print_help = true;break;
    case 'H': hash_table = atoi(optarg);break;
    case 'C': compression = (atoi(optarg)==1?HUFFMAN_COMPRESS:NO_COMPRESS); break;
    case 'g': print_graph = true;break;
    case 'D': print_deadlock = true;break;
    case 'E': print_leading_to_error = true;break;
    }
  }

  if (print_help) {
    cout << "Usage:\n"
"divine.generator [-ahdgqstSDE] [-v number] [-H size] input_file\n"
"\n"
"Generates the entire state space and prints it in various formats.\n"
"\n"
"Options:\n"
"-a print flags A/N telling whether a state is accepting or not after each state\n"
"-d   output in dot format\n"
"-g   print graph\n"
"-h   this help\n"
"-t   print all transitions\n"
"-v n verbosity level (0-15; different formats of state output)\n"
"-q   quiet and quick - do not print anything, keep only important statistics\n"
"-s   print all states\n"
"-D   print DEAD after each deadlock state\n"
"-E   print LEADING TO ERROR after each state leading to erroneous state\n"
"-H x hash table size to ( x<33 ? 2^x : x )\n"
"-S   print statistics\n"
;
  } else {

    if (argc < optind+1)
      { 
	cerr << "No input file given!" << endl;
	return 1;
      }

    try {
      //initialization
      explicit_system_t * p_System;
      char * filename = argv[optind];
      int filename_length = strlen(filename);
      if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
       {
        cout << "Reading bytecode source..." << endl;
        p_System = new affine_explicit_system_t(gerr);
       }
      else
       {
        cout << "Reading DVE source..." << endl;
        p_System = new dve_explicit_system_t(gerr);
       }

      int file_opening = p_System->read(filename);
      if (file_opening) return file_opening;
      succ_container_t succs(*p_System);
      
      //Advanced setting of hash table size:
      if (hash_table > 0)
       {
        if (hash_table < 33) //sizes 1..32 interpret as 2^1..2^32
         {
          int z = hash_table;
          hash_table = 1;
          for (;z>0; z--)
           {
            hash_table = 2* hash_table;
           }
         }
        st.set_ht_size(hash_table);
	st.set_compression_method(compression);
       }

      if (!quiet_and_quick && (print_graph || print_dot))
        st.set_appendix(counter);
        
      st.init();
    
      state_t init = p_System->get_initial_state();

      queue<state_ref_t> Wait;
      state_ref_t pom;
      bool is_erroneous;
      bool is_deadlock;
      bool is_leading_to_error;
      
      st.insert(init,pom);
      Wait.push(pom);
      delete_state(init);
      
      if (!quiet_and_quick) {
        if (print_graph || print_dot) st.set_app_by_ref(pom,counter++);
        if (print_dot) { cout <<"digraph G {"<<endl;}
      }

      while (! Wait.empty()) {
	state_ref_t act_ref = Wait.front(); Wait.pop();
	state_t q = st.reconstruct(act_ref);

        if (!quiet_and_quick)
         {
	  states++;
	  if (states %1000 ==0) cerr << "States\t"<<states<<"\tqueue\t"<<Wait.size()<<"      \r";
         }
        if (p_System->is_erroneous(q)) is_erroneous = true;
        else is_erroneous=false;
        is_deadlock=false;
        is_leading_to_error=false;
        
        if (!is_erroneous) {
          int return_value=p_System->get_succs(q, succs);
	  if (succs_deadlock(return_value))
            { deadlock_count++; is_deadlock=true; }
          if (succs_error(return_value)) is_leading_to_error=true;
          
          if (quiet_and_quick) {
            for (succ_container_t::iterator i=succs.begin();i!=succs.end();++i){
              state_t & r=*i;
              if (! st.is_stored(r)) {
                st.insert(r,pom);  //stiring a state to queue and "visited" set
                Wait.push(pom);
              }
	      delete_state(r); //deleting a copy of (potent.) inserted state
            }
          }
          else {
            if (print_states && ! print_dot) {
//              p_System->DBG_print_state(q,cout,verbose_level);
              p_System->print_state(q,cout);
              if (print_acc)  cout << (p_System->is_accepting(q)?'A':'N');
              if (print_leading_to_error && is_leading_to_error)
                cout << " LEADING TO ERROR";
              if (print_deadlock && is_deadlock) cout << " DEAD";
              cout <<endl;
            }
            if (print_dot && print_acc && p_System->is_accepting(q)){
              cout <<"\"";
//              p_System->DBG_print_state(q,cout,verbose_level);
              p_System->print_state(q,cout);
              cout << "\"[style=filled, color=gray];"<<endl;
            }
            if (print_graph) {
              int x;
              st.get_app_by_ref(act_ref, x);
              cout << x << " "<<succs.size();
            }
            for (succ_container_t::iterator i=succs.begin();i!=succs.end();++i){
              state_t & r=*i;
              if (! st.is_stored(r)) {
                st.insert(r,pom);
                Wait.push(pom);
                if (print_dot || print_graph) st.set_app_by_ref(pom,counter++);
              }
              if (print_dot) {
                cout << "\"";
//	        p_System->DBG_print_state(q,cout,verbose_level);
	        p_System->print_state(q,cout);
	        cout << "\" -> \""; 
//	        p_System->DBG_print_state(r,cout,verbose_level);	    
	        p_System->print_state(r,cout);	    
	        cout <<"\""<<endl;
	      } else if (print_trans) {
//	        p_System->DBG_print_state(q,cout,verbose_level);
	        p_System->print_state(q,cout);
	        cout << " -> "; 
//	        p_System->DBG_print_state(r,cout,verbose_level);	    
	        p_System->print_state(r,cout);	    
	        cout <<endl;
	      } else if (print_graph) {
	        st.is_stored(r,pom);
	        int x;
	        st.get_app_by_ref(pom,x);
	        cout << " "<<x;
	      }
	      delete_state(r); //deleting a copy of (potent.) inserted state
	    }
	    if (print_graph) cout<<endl;
          }
        
          transitions += succs.size();
        }
        else error_count++;

        delete_state(q);  //deleting a copy reconstructed state
      } //end of the main BFS loop
      
      if (!quiet_and_quick && print_dot) cout << "}"<<endl;
      if (!quiet_and_quick)
        cerr << "                                                          "
                "     "<<endl;
      if (print_statistics) {
        dve_explicit_system_t * p_dve_system;

	cout << "Statistics:\nStates\t\t\t\t"<<st.get_states_stored()<<"\nTransitions\t\t\t"<<transitions<<endl;
        cout << "Deadlock states:\t\t"<< deadlock_count << endl;
        cout << "Erroneous states:\t\t"<< error_count << endl;
	cout << "Max. memory for states set:\t"<< float(st.get_mem_max_used())/1024 << " MB" <<endl;
// 	cout << "Actually used virtual memory:\t"<< get_consumed_memory() << " MB" <<endl;
	if ((p_dve_system = dynamic_cast<dve_explicit_system_t*>(p_System)))
          cout << "Size of state:\t\t\t"<< p_dve_system->get_space_sum() <<endl;
	cout << "Size of hash table:\t\t"<< hash_table <<endl;
	cout << "Max. collision table:\t\t"<< st.get_max_coltable()<<endl;
	cout << "Average collision table:\t"<< (double) states/hash_table <<endl;
	if ((p_dve_system = dynamic_cast<dve_explicit_system_t*>(p_System)))
	  cout << "Storage efficiency:\t\t" << (double) (st.get_mem_max_used() - (p_dve_system->get_space_sum() *states))/st.get_mem_max_used()<<endl;
      } 

    } catch  (ERR_throw_t & err_type)
      { return err_type.id; }
  }
  
  return 0;
}
