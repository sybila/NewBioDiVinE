#include "common/distr_reporter.hh"
#include <cmath>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>

using namespace std;
using namespace divine;

#define TAG_REPORTER_DATA DIVINE_TAG_USER+137 // zmenit; jinam....
#define TAG_REPORTER_WORKSTATIONS DIVINE_TAG_USER+138 // zmenit; jinam....

const size_int_t divine::distr_reporter_t::BASIC_ITEMS = 6;

void distr_reporter_t::print_specific_value(ostream & out, const string label,
                                            const size_int_t i)
{
 int comp = distributed->cluster_size;
 double sum, min, max;
 sum = min = max = results[0][i];
 
 for (int j=1; j<comp; j++) {
   sum += results[j][i];
   if (results[j][i]<min) min = results[j][i];
   if (results[j][i]>max) max = results[j][i];
 }

 if (specific_info_flag[label] == REPORTER_MAX)
   { _set_pr(out, max);  out << max;}
 else if (specific_info_flag[label] == REPORTER_MIN)
   { _set_pr(out, min);out << min; }
 else if (specific_info_flag[label] == REPORTER_AVG)
   { _set_pr(out, (double)sum/comp);out << (double) sum/comp; }
 else if (specific_info_flag[label] == REPORTER_SUM)
   { _set_pr(out, sum);out << sum; }
 else { _set_pr(out, results[0][i]); out<< results[0][i]; }
}

void distr_reporter_t::_set_pr(ostream& out, double a) {
  if (a == floor(a)) {
    out<<setprecision(0);
  } else {
    out<<setprecision(6);
  }
}

void distr_reporter_t::collect_and_print(size_t type_of_output, ostream& out)
{
 distr_reporter_t::collect();
 distr_reporter_t::print(type_of_output, out);
}


void distr_reporter_t::collect()
{
  // assumption: all workstations are synchronized and do not send any messages
  // but you never know, so let us check it first

  int id = distributed->network_id;

  int cnt1=0;
  int cnt2=0;

  distributed->network.barrier();
  distributed->network.get_all_received_msgs_cnt (cnt1);
  distributed->process_messages();
  distributed->network.get_all_received_msgs_cnt (cnt2);
  if ( cnt1 != cnt2 )
    {
      cout <<id<<": There were unreceived messages! Probably wrong usage of distributed_t! "<<endl;
    }
  
  
  double* buf;
  // if you want to sent more system info, do not forget to change this...

  int items = BASIC_ITEMS + specific_info.size();
  int items_size = sizeof(double) * items;
  buf = (double*) malloc(items_size);
  
  distributed->set_busy();

  buf[0] = time;
  
  vm_info.scan();
  buf[1] = vm_info.vmsize/1024.0;

  int sent, sent_user;
  
  distributed->network.get_all_sent_msgs_cnt(sent);
  distributed->network.get_user_sent_msgs_cnt(sent_user);
  
  buf[2] = sent;
  buf[3] = sent_user;

  buf[4] = states_stored;
  buf[5] = succs_calls;

  int pom=0;
  for(map<string,double>::iterator i = specific_info.begin(); i!=specific_info.end(); i++, pom++) {
    buf[BASIC_ITEMS+pom] = (*i).second;
  }

  string workstations=distributed->processor_name;
  
  if (id != 0) {
    //send to master
    distributed->network.send_message((char*) buf, items_size, 0, TAG_REPORTER_DATA);
    distributed->network.send_message((char*) workstations.c_str(), workstations.length(), 0, TAG_REPORTER_WORKSTATIONS);
    distributed->network.flush_buffer(0); 
  } else {
    // get all messages
    // root: receive and process:
    results.resize(distributed->cluster_size);

    results[0].resize(items);
    for(int i = 0; i<items; i++) results[0][i] = buf[i];
    int got=0;
    while (got<2*(distributed->cluster_size-1)) {
      int src, tag;      
      distributed->network.receive_message((char*) buf, items_size, src, tag);
      switch (tag)
	{
	case TAG_REPORTER_DATA:
	  {
	    results[src].resize(items);
	    for(int i=0; i<items; i++) results[src][i] = buf[i];
	    break;
	  }
	case TAG_REPORTER_WORKSTATIONS:
	  {
	    string tmp=(char*)buf;
	    workstations= workstations + ' ' + tmp;
	    break;
	  }
	default:
	  { gerr << "Reporter receive error" << thr(); break; }
	}
      got++;
    }
  }
    
  distributed->set_idle(); //message sent/messages received
    
  while (!distributed->synchronized())
    distributed->process_messages(); //waiting for synchronization
    
}


void distr_reporter_t::print(size_t type_of_output, ostream& out) {
  if (distributed-> is_manager()) {
    size_int_t items = BASIC_ITEMS + specific_info.size();
    vector<string> item_labels;
    item_labels.resize(items);
    item_labels[0] = "Time";
    item_labels[1] = "VMSize"; //vm_info.vmsize is in kB, we want MB, thus "/1024.0"
    item_labels[2] = "MsgSent";
    item_labels[3] = "MsgSentUser";
    item_labels[4] = "StatesStored";
    item_labels[5] = "SuccsCalls";
    specific_long_name[item_labels[0]] = "Time";
    specific_long_name[item_labels[1]] = "Virtual Mem. Size";
    specific_long_name[item_labels[2]] = "Messages Sent";
    specific_long_name[item_labels[3]] = "User Messages Sent";
    specific_long_name[item_labels[4]] = "States Stored";
    specific_long_name[item_labels[5]] = "Successor func. calls";
    size_int_t pom = 0;
    for (map<string,double>::iterator i = specific_info.begin();
         i!=specific_info.end(); i++, pom++)
      item_labels[BASIC_ITEMS+pom] = (*i).first;
    
    size_int_t comp = distributed->cluster_size;

    out << "File:"<<file_name<<endl;
    out << "Algorithm:"<<alg_name<<endl;
    out << "Problem:"<<problem<<endl;
    out << "Nodes:"<<comp<<endl;
    out << setiosflags (ios_base::fixed);
        
    for(size_int_t i=0; i<items; i++) {
      double sum, min, max;
      sum = min = max = results[0][i];
      
      for (size_int_t j=1; j<comp; j++) {
	sum += results[j][i];
	if (results[j][i]<min) min = results[j][i];
	if (results[j][i]>max) max = results[j][i];
      }

      if (type_of_output == REPORTER_OUTPUT_SHORT) {
        string long_name = specific_long_name[item_labels[i]];
        if (long_name.size())
         {
          out << long_name << ": ";
          if (i<BASIC_ITEMS) { _set_pr(out, max); out << max << endl; }
          else
           {
            print_specific_value(out, item_labels[i], i);
            out << endl;
           }
         }
      }
      else if (type_of_output == REPORTER_OUTPUT_NORMAL) {
	// seq-like output; print only max values of system things
	// and master info of others
	if ((i != 2)&&(i !=3)&&(i<BASIC_ITEMS)) { // in short output we do not want messege counts
	  _set_pr(out, max);
	  out << item_labels[i] <<":"<<max<<endl;
	}
	if (i >=BASIC_ITEMS) { // for others according to flag
	  out << item_labels[i] <<":";
          print_specific_value(out,item_labels[i], i);
          out << endl;
	}
      } else {
	_set_pr(out, sum);
	out << item_labels[i] <<":sum:"<<sum<<endl;
	_set_pr(out, (double)sum/comp);
	out << item_labels[i] <<":avg:"<<(double)sum/comp<<endl;
	_set_pr(out, min);
	out << item_labels[i] <<":min:"<<min<<endl;
	_set_pr(out, max);
	out << item_labels[i] <<":max:"<<max<<endl;

	if (type_of_output == REPORTER_OUTPUT_LONG) {
	  out << item_labels[i] <<":values";
	  for (int j=0; j<distributed->cluster_size; j++) {
	    _set_pr(out, results[j][i]);
	    out << ":"<< results[j][i];
	  }
	  out <<endl;
	}
      }
    }
    
    string workstations=distributed->processor_name;
    set_global_info("Workstations", workstations);
    time_t act_time = ::time(0);
    char* act_time_str = new char[26];
    act_time_str = ctime(&act_time);
    act_time_str[24]='\0';
    string tmp_str = act_time_str;
    set_global_info("ReporterCreated", tmp_str);

    for(map<string,global_info_value_t>::iterator i = global_info.begin(); 
	i!=global_info.end(); i++) 
      {
	global_info_value_t gv = (*i).second;
	out << (*i).first <<":";
	if (gv.val_is_num)
	  {
	    _set_pr(out, gv.num_val); out << gv.num_val;
	  }
	else
	  {
	    out<<gv.str_val;
	  }
        out<<endl;
      }   
  }
}

void distr_reporter_t::collect_and_print(size_t type_of_output) {
  ofstream out;
  string rep_file = file_name+".report";
  out.open(rep_file.c_str());
  collect_and_print(type_of_output, out);
  out.close();
}











