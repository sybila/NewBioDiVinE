 /* DiVinE - Distributed Verification Environment
  * Copyright (C) 2002-2007  Pavel Simecek, Jiri Barnat, Pavel Moravec,
  *     Radek Pelanek, Jakub Chaloupka, David Safranek, Faculty of Informatics Masaryk
  *     University Brno
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


#include <getopt.h>
#include "../../src/common/bitarray.hh"
#include <iostream>
#include <queue>
#include <sstream>
#include <unistd.h>

#include "sevine.h"

#define ES_FMT_DIVIDE_PROCESSES_BY_BACKSLASH_N 16

using namespace std;
using namespace divine;

affine_explicit_system_t asys(gerr);
explicit_storage_t st;
unsigned int max_b=0;
bool draw_accepting = false;
bool affine=false;

struct appendix_t {
  int depth;
};

appendix_t app,app1;

void print_transition(const transition_t * const trans)
{
  trans->write(cout);
}

void print_state(state_t state)
{
  ostringstream mystream;
  asys.DBG_print_state(state,mystream,0);
  string str = mystream.str();
  size_t i=0;

  if (max_b!=0)
    {
      int count=0;
      for (unsigned int j=0;j<str.length();j++)
	{
	  i=str.find(',',j);
	  if (i==j)
	    {
	      count ++;
	      if (count%max_b == 0)
		str.replace(i,1,"\\n");
	    }
	  i=str.find("-PP",j);
	  if (i==j)
	    {
	      count ++;
	      if (count%max_b == 0)
		str.replace(i,1,"\\n");
	    }
	}
    }
  else
    {      
      i=0;  
      while ((i=str.find(';',i)) != string::npos)
	{
	  str.replace(i,2,"\\n");
	  i=i+3;
	}
    }
  cout << str;
}

void version()
{
  cout <<"predot affine ";
  cout <<"1.2 (2009/Feb/03 15:15)"<<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  version();
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<endl;
  cout <<"Predot transforms a .bio file to format readable by 'dot'."<<endl;
  cout <<endl;
  cout <<"Usage: predot [switches] file.bio"<<endl;
  cout <<"Switches:"<<endl;
  cout <<" -h    show this help" <<endl;
  cout <<" -v    show predot version"<<endl;
  cout <<" -f    faster but less accurate abstraction" <<endl;
  cout <<" -a    draw accepting states (Buchi acceptance only)" <<endl ;
  cout <<" -D n  show only 1st n levels of the graph" <<endl;
  cout <<" -l    landscape" <<endl;
  cout <<" -w n  split line in a state after n fields" <<endl;
  return;
}

int main(int argc, char **argv)
{
  bool landscape = false;
  bool useFastAproximation = false;
  char c;
  int d=0,max_d=0;

  static const option longopts[]={
    {"version",0,0,'v'},
    {NULL,0,0,0}
  };

  opterr = 0;
  while ((c = getopt_long(argc, argv, "aD:hlfvw:", longopts, NULL)) != -1) {
    switch (c) {
    case 'a': draw_accepting = true; break;
    case 'D': max_d = atoi(optarg); break;
    case 'h': usage(); return 0;break;
    case 'l': landscape = true; break;
    case 'v': version();return 0;break;
      //    case 'g': { genesim = true; use_succs_only = true; } break;
    case 'w': max_b = atoi(optarg); break;
    case 'f': useFastAproximation = true; break;
    case '?': cerr <<"skipping unknown switch -"<<(char)optopt<<endl;break;
    }
  }

  if (argc<optind+1)
    {
      usage();
      return 0;
    }
  char *filename = argv[optind];
  int filename_length = strlen(filename);

  bool recognized = false;
  int file_opening=0;


  if (filename_length>=4 && strcmp(filename+filename_length-4,".bio")==0)
    {
      file_opening = asys.read(argv[optind],useFastAproximation);
      recognized = true;
      affine=true;
    }    

  if (!recognized)
    {
      cerr << "File type not recognized. Supported extensions are .dve, and .probdve" << endl;
      return 1;
    }

  
  if (file_opening)
    {
      cerr <<"Filename "<<argv[optind]<<" does not exist."<<endl;
      return 1;
    }
 
  if (asys.get_with_property() && (asys.get_property_type()==BUCHI) && draw_accepting )
    {
    }
  else
    draw_accepting=false;

  
  
  st.set_appendix(app);
  st.init();
   
  
  state_ref_t ref;
  state_t state;
  int state_count = 0;

  state = asys.get_initial_state();
  st.insert(state,ref);
  //cout <<ref.page_id<<","<<ref.hres<<","<<ref.id<<endl<<flush;

  app.depth = d;
  st.set_app_by_ref(ref,app);

  queue<state_t> current,next,temp;
  next.push(state);

  cout <<"digraph G {";
  if (landscape)
    {
      cout <<"  orientation=\"landscape\" size=\"10.8,7.2\""<<endl;
    }
  else
    {
      cout <<"  size=\"7.2,10.8\""<<endl;
    }
  //cout <<"  size=\"7.2,10.8\""<<endl;

  while (!next.empty())
    {
      d++;

      // current -> temp
      // this seems to have emptying purpose only...
      while (!current.empty())
	{
	  state_t a;
	  a = current.front();
	  current.pop();
	  // temp.push(a);
	}

      // next -> current
      cout <<"{ rank = same; ";
      while(!next.empty())
	{
	  state = next.front();
	  state_count++;
	  cout<<"\"";
	  print_state(state);
	  cout<<"\"; ";
	  next.pop();
	  current.push(state);
	}
      cout <<"}"<<endl;

      // temp -> next
//        while (!temp.empty())
//  	{
//  	  state_t a;
//  	  a = temp.front();
//  	  temp.pop();
//  	  next.push(a);
//  	}

      app.depth = d;
      while (!current.empty())
	{
	  succ_container_t succs(asys);
	  
	  state = current.front();
	  current.pop();

	  
	  state_ref_t stref;
	  int succs_result;
	  succs_result = asys.get_succs(state,succs);
		
	  if (draw_accepting)
	    {		    
	      if (asys.is_accepting(state))
		{
		  cout <<"\"";
		  print_state(state);
		  cout <<"\" [peripheries=2,style=bold];"<<endl;
		}
	    }		

	  for (std::size_t info_index=0; info_index!=succs.size();
	       info_index++)
	    {		
	      cout <<"\"";
	      print_state(state);
	      cout <<"\"->\"";
	      print_state(succs[info_index]);
	      cout <<"\""<<endl;
	    }

	  for (succ_container_t::iterator i = succs.begin(); i != succs.end(); ++i)
	    {
	      state_t r=*i;
	      if ((st.is_stored(r)) || (d==max_d))
		{
		  delete_state(r);
		}
	      else
		{		  
		  next.push(r);
		  st.insert(r,ref);
		  st.set_app_by_ref(ref,app);
		}
	    }
	  delete_state(state);
	}
    }

  cout <<"}"<<endl;
  cerr << "Number of states: "<<state_count<<endl;
  cerr << "Storage: " << st.get_states_stored()<<endl;

  return 0;

}








