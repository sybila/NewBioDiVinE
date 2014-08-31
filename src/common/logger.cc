#include "common/logger.hh"
#include <sstream>
#include <signal.h>

using namespace divine;

static divine::logger_t *logger_ptr;

void on_SIGALRM_signal(int a)
{
  logger_ptr->log_now();  
  alarm(logger_ptr->signal_period);
}   


logger_t::logger_t()
{
  log_counter = 0;
  logger_ptr = this;

  storage_ptr = NULL;

  ufunc_count = 0;
  ifunc_count = 0;
  dfunc_count = 0;  
  
  ufunctor_count = 0;
  sfunctor_count = 0;
  dfunctor_count = 0;  
}

logger_t::~logger_t()
{  

  
  if (initialized)
    {
      logger_file.open(logger_file_name.c_str(),ofstream::app|ofstream::out);
      switch (format) {
      case 0:
	logger_file<<"# ==========================================="
		   <<"============================================"<<endl;
	break;
      case 1: 
	logger_file << "</data>" << endl;
	break;
      default:
	logger_file<<"# Closing log file (format "<<format<<")"<<endl;
	break;
      }  
      logger_file.close();
    }
}

void logger_t::init(string tmpstr_in, int format_in)
{
 init(0,tmpstr_in,format_in);
}

void logger_t::init(distributed_t * divine_ptr_in, string tmpstr_in,
		    int format_in)
{
  initialized = true;
  firsttick = times(&tmsstruct);
  divine_ptr = divine_ptr_in;
  format = format_in;
  
  if (divine_ptr)
    { net_id = divine_ptr->network_id; host = divine_ptr->processor_name; }
  else { net_id = 0; host = "NotDistributed"; }
  
  ostringstream idstr;
  idstr <<".";
  if (net_id <10)
    idstr<<"0";
  idstr<<net_id;

  logger_file_name = tmpstr_in + idstr.str();
  logger_file.open(logger_file_name.c_str(),ofstream::out);
  
  switch (format) {
  case 0:
    logger_file<<"# "<<host<<" (network id = "
	       <<net_id<<") "	       
	       <<"pid="<<getpid()
	       <<endl;
    logger_file<<"# ==========================================="
	       <<"============================================"<<endl;
    logger_file<<"#\tuser\tsys\tidle\tmem\t";
    if (storage_ptr)
      {
	logger_file <<"states\t";
      }
    logger_file<<"sent\trecv\tbarrier\tpackets\t";
    for (int i=0; i<ufunc_count; i++)
      {
	logger_file <<ufunc_name[i]<<"\t";
      }
    for (int i=0; i<ufunctor_count; i++)
      {
	logger_file <<ufunctor_name[i]<<"\t";
      }

    for (int i=0; i<ifunc_count; i++)
      {
	logger_file <<ifunc_name[i]<<"\t";
      }
    for (int i=0; i<sfunctor_count; i++)
      {
	logger_file <<sfunctor_name[i]<<"\t";
      }

    for (int i=0; i<dfunc_count; i++)
      {
	logger_file <<dfunc_name[i]<<"\t";
      }
    for (int i=0; i<dfunctor_count; i++)
      {
	logger_file <<dfunc_name[i]<<"\t";
      }
    logger_file <<"cmatrix";
    logger_file<<endl;    
    logger_file<<"# -------------------------------------------"
	       <<"--------------------------------------------"<<endl;
    break;
  case 1:
          logger_file << "<fileHeader entrySize=\"500\" format=\"format1\"/>\n\t <data machineID=\"" << net_id << "\" hostName=\"" << host << "\">" << endl;
    break;
  default:
    logger_file<<"# Starting log file (format "<<format<<")"<<endl;
    logger_file<<"# Hostname: "<<host<<" ("
	       <<net_id<<")"<<endl;
    break;
  }
  logger_file.close();
}

void logger_t::log_now()
{
  clock_t now=times(&tmsstruct);
  int tmp;

  logger_file.open(logger_file_name.c_str(),ofstream::app|ofstream::out);

  switch (format) {
  case 0:
   {
    vm.scan();
    tmp = (100*vm.vmsize/1024);
    logger_file<<(now-firsttick)/100.0<<"\t"
	       <<tmsstruct.tms_utime<<"\t"
	       <<tmsstruct.tms_stime<<"\t"
	       <<(now-firsttick-tmsstruct.tms_utime-tmsstruct.tms_stime)<<"\t"
	       <<tmp/100.0<<"\t";
    
    if (storage_ptr)
      {
	logger_file << storage_ptr->get_states_stored()<<"\t";
      }
      
      int sent_msg_cnt=0, recv_msg_cnt=0, barriers_cnt=0, buffer_flush_cnt=0;
      if (divine_ptr)
       {
        divine_ptr->network.get_all_sent_msgs_cnt(sent_msg_cnt);
      
        divine_ptr->network.get_all_received_msgs_cnt(recv_msg_cnt);
      
        divine_ptr->network.get_all_barriers_cnt(barriers_cnt);

//        logger_file<<divine_ptr->get_all_sync_barriers_cnt()<<"\t";
      
        divine_ptr->network.get_all_buffers_flushes_cnt(buffer_flush_cnt);
       }
      logger_file<<sent_msg_cnt<<"\t";
      logger_file<<recv_msg_cnt<<"\t";
      logger_file<<barriers_cnt<<"\t";
      logger_file<<buffer_flush_cnt<<"\t";

//      n->get_buffer_flushes_cnt(int dest, int& cnt);
//      n->get_user_sent_msgs_cnt(tmp);
//      n->get_user_received_msgs_cnt(tmp);

    for (int i=0; i<ufunc_count; i++)
      {
	logger_file <<ufunc[i]()<<"\t";
      }
    for (int i=0; i<ufunctor_count; i++)
      {
	logger_file <<ufunctor[i]->log()<<"\t";
      }

    for (int i=0; i<sfunctor_count; i++)
      {
	logger_file <<sfunctor[i]->log()<<"\t";
      }

    for (int i=0; i<dfunctor_count; i++)
      {
	logger_file <<dfunctor[i]->log()<<"\t";
      }
    
    if (divine_ptr)
     {
      for (int i=0; i<divine_ptr->cluster_size; i++)
        {
	  logger_file <<divine_ptr->network.get_sent_msgs_cnt_sent_to(i);
	  if (i<(divine_ptr->cluster_size-1))
	    {
	      logger_file <<",";
	    }
	  else
	    {
	      //logger_file <<"\t";
	    }
        }
      }
     else logger_file << "0";

    logger_file<<endl;
    break;
   }
  case 1:
   {
    int sent = 0, received = 0;
    if (divine_ptr)
     {
      divine_ptr->network.get_all_sent_msgs_cnt(sent);
      divine_ptr->network.get_all_received_msgs_cnt(received);
     }

    logger_file<<"<entry entryNumber=\""<<log_counter<<"\" "
	       <<"clock=\""<<now<<"\" " 
	       << "numberOfStates=\"" <<storage_ptr->get_states_stored()<< "\" statesSent=\"" << sent 
	       << " \"statesRecieved=\"" << received;
    if (ufunc_count)
      logger_file<< "\" queueSize=\"" << ufunc[0]();
    else if (ufunctor_count)
      logger_file<< "\" queueSize=\"" << ufunctor[0]->log();
    logger_file<< "\" totalbytesSent=\"0\" stateGenerationTime=\"55\" communicationTime=\"44\" idleTime=\"3333\""; //TODO
    logger_file<<"/>"<<endl;
    break;
   }
  default:
    break;
  }
  
  log_counter++;
  //logger_file<<flush;
  logger_file.close();
  
  //  <entry entryNumber="9" clock="1088183850" numberOfStates="27412" statesSent="25286" statesRecieved="57410" queueSize="21344" totalBytesSent="303444"
  //  stateGenerationTime="2" communicationTime="0" idleTime="0"/>
    
}


void logger_t::stop_SIGALRM()
{
  signal(SIGALRM,SIG_IGN);       //ignore signal
  //signal(SIGALRM,SIG_DFL);       //default handler
}

void logger_t::use_SIGALRM(unsigned int period_in)
{
  signal_period=period_in;
  signal(SIGALRM,on_SIGALRM_signal);
  alarm(signal_period);
}

void logger_t::register_unsigned(unsignedfunc_t uf_in,string name)
{
  if (ufunc_count<10)
    {
      ufunc[ufunc_count] = uf_in;
      ufunc_name[ufunc_count] = name;
      ufunc_count ++;
    }
  else 
    {
      cout <<"cannot register so many functions"<<endl;
    }
}

void logger_t::register_int(intfunc_t if_in,string name)
{
  if (ifunc_count<10)
    {
      ifunc[ifunc_count] = if_in;
      ifunc_name[ifunc_count] = name;
      ifunc_count ++;
    }
  else 
    {
      cout <<"cannot register so many functions"<<endl;
    }
}

void logger_t::register_double(doublefunc_t df_in,string name)
{
  if (dfunc_count<10)
    {
      dfunc[dfunc_count] = df_in;
      dfunc_name[dfunc_count] = name;
      dfunc_count ++;
    }
  else 
    {
      cout <<"cannot register so many functions"<<endl;
    }
}

void logger_t::register_ulong_int(const log_ulong_int_t & functor,
                                  std::string description)
{
 if (ufunctor_count<10)
  {
   ufunctor[ufunctor_count] = &functor;
   ufunctor_name[ufunctor_count] = description;
   ufunctor_count++;
  }
 else
  {
    gerr << "Cannot register so many functors" << thr();
  }
}

void logger_t::register_slong_int(const log_slong_int_t & functor,
                                  std::string description)
{
 if (sfunctor_count<10)
  {
   sfunctor[sfunctor_count] = &functor;
   sfunctor_name[sfunctor_count] = description;
   sfunctor_count++;
  }
 else
  {
    gerr << "Cannot register so many functors" << thr();
  }
}

void logger_t::register_double(const log_double_t & functor,
                               std::string description)
{
 if (dfunctor_count<10)
  {
   dfunctor[dfunctor_count] = &functor;
   dfunctor_name[dfunctor_count] = description;
   dfunctor_count++;
  }
 else
  {
    gerr << "cannot register so many functors" << thr();
  }
}

void logger_t::set_storage(explicit_storage_t *sp)
{
  storage_ptr = sp;
}









