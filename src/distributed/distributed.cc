#include <iostream>
#include <stdexcept>
#include <string.h>
#include "distributed/distributed.hh"

using namespace divine;

distributed_t::distributed_t(error_vector_t& arg0):errvec(arg0)
{
  mode = PRE_INIT;
  cluster_size = 0;
  network_id = -1;
  process_user_message = NULL;
  process_message_functor = 0;

  fsync_state = -1;
  finfo_exchange = false;
  fproc_msgs_buf_exclusive_mem = false;
  fstatistics = NULL;
  tmp_buf_cl_size = NULL;
  sync_sr_buf = NULL;
}
  
void distributed_t::network_initialize(int &argc,char **&argv)
{
  int pn_len;
  char *pn;
  
  if (mode == PRE_INIT) 
    {
      network.initialize_network(argc, argv);
      network.get_cluster_size(cluster_size);
      network.get_id(network_id);
      network.get_processor_name(NULL, pn_len);
      pn = new char[pn_len + 1];
      network.get_processor_name(pn, pn_len);
      processor_name = pn;
      delete[] pn;

      mode = PRE_INIT_NET_OK;
    }
  else
    {
      errvec << "Cannot initialize network now." << thr(DISTRIBUTED_ERR_TYPE);
    }
} 
  
void distributed_t::initialize()
{
  int i;

  if (mode == PRE_INIT_NET_OK)
    {
      // vsechny mozne inicializace
      network.initialize_buffers();

      fstat_all_sync_barriers_cnt = 0;
      fstatistics = new vector<distr_stat_t>(cluster_size);

      for (i=0; i<cluster_size; i++)
		{
		  (*fstatistics)[i].all_sent_sync_msgs_cnt = 0;
		  (*fstatistics)[i].all_received_sync_msgs_cnt = 0;
		}

      tmp_buf_cl_size = new int[cluster_size];
      sync_sr_buf = NULL;

      mode = NORMAL;
    }
  else
    {
      errvec << "Network not initialized." << thr(DISTRIBUTED_ERR_TYPE);
    }
}

void distributed_t::finalize()
{
  switch (mode)
    {
    case PRE_INIT_NET_OK:
      {
		network.finalize();

		if (fstatistics != NULL)
		  {
			delete fstatistics;
			fstatistics = NULL;
		  }

		if (tmp_buf_cl_size != NULL)
		  {
			delete[] tmp_buf_cl_size;
			tmp_buf_cl_size = NULL;
		  }

		if (sync_sr_buf != NULL)
		  {
			delete[] sync_sr_buf;
			sync_sr_buf = NULL;
		  }
	
		mode = PRE_INIT;
		break;
      }
    case NORMAL:
      {
		network.finalize();

		mode = PRE_INIT;	
		break;
      }
    case PRE_INIT:
      {
		errvec << "Nothing to finalize." << thr(DISTRIBUTED_ERR_TYPE);
		  break;
      }
    }
}

  
  
distributed_t::~distributed_t(void)
{
  if (fstatistics != NULL)
    {
      delete fstatistics;
      fstatistics = NULL;
    }

  if (tmp_buf_cl_size != NULL)
    {
      delete[] tmp_buf_cl_size;
      tmp_buf_cl_size = NULL;
    }

  if (sync_sr_buf != NULL)
    {
      delete[] sync_sr_buf;
      sync_sr_buf = NULL;
    }
}

void distributed_t::get_comm_matrix(distr_comm_matrix_type_t cm_type, pcomm_matrix_t& ret, int target)
{
  int i, ssize;

  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  switch (cm_type) {
  case dcmt_ssm:
    {
      for (i=0; i < cluster_size; i++)
	{
	  tmp_buf_cl_size[i] = (*fstatistics)[i].all_sent_sync_msgs_cnt;
	}
      break;
    }
  case dcmt_rsm:
    {
      for (i=0; i < cluster_size; i++) 
	{
	  tmp_buf_cl_size[i] = (*fstatistics)[i].all_received_sync_msgs_cnt;
	}
      break;
    }
  }

  ssize = cluster_size * sizeof(int);

  if (network_id == target)
    {
      int j;

      int *tmp_buf_cl_sizexcl_size = new int[cluster_size * cluster_size];

      network.gather(static_cast<char *>(static_cast<void *>(tmp_buf_cl_size)), 
		     ssize, 
		     static_cast<char *>(static_cast<void *>(tmp_buf_cl_sizexcl_size)), 
		     ssize, 
		     target);
      
      pcomm_matrix_t tmp_ret(new comm_matrix_t(cluster_size, cluster_size));
      
      for (i = 0; i < cluster_size; i++)
  	for (j = 0; j < cluster_size; j++)
  	  {
  	    (*tmp_ret)(i, j) = tmp_buf_cl_sizexcl_size[i * cluster_size + j];
   	  }
      
      delete[] tmp_buf_cl_sizexcl_size;

      ret = tmp_ret;
    }
  else
    {
      network.gather(static_cast<char *>(static_cast<void *>(tmp_buf_cl_size)), 
		     ssize, 
		     NULL, 
		     0, 
		     target);
    }
}

void distributed_t::get_comm_matrix_ssm(pcomm_matrix_t& ret, int target)
{
  get_comm_matrix(dcmt_ssm, ret, target);
}

void distributed_t::get_comm_matrix_rsm(pcomm_matrix_t& ret, int target)
{
  get_comm_matrix(dcmt_rsm, ret, target);
}

int distributed_t::get_all_sent_sync_msgs_cnt(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  int counter = 0;

  for (int i = 0; i < cluster_size; i++)
    {
      counter += (*fstatistics)[i].all_sent_sync_msgs_cnt;
    }

  return counter;
}

int distributed_t::get_all_received_sync_msgs_cnt(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  int counter = 0;

  for (int i = 0; i < cluster_size; i++)
    {
      counter += (*fstatistics)[i].all_received_sync_msgs_cnt;
    }

  return counter;
}


int distributed_t::get_all_sync_barriers_cnt(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  return fstat_all_sync_barriers_cnt;
}

void distributed_t::set_busy(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  fbusy = true;
}
  
void distributed_t::set_idle(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }
#if defined(ORIG_FLUSH)
  network.flush_all_buffers();
#else
  network.flush_some_buffers();
  force_poll();
#endif
  fbusy = false;
}
 
void distributed_t::set_proc_msgs_buf_exclusive_mem(bool value)
{
  fproc_msgs_buf_exclusive_mem = value;
}

void distributed_t::get_proc_msgs_buf_excluseve_mem(bool &value)
{
  value = fproc_msgs_buf_exclusive_mem;
}

void distributed_t::set_hash_function(hash_functions_t hf)
{
  hasher.set_hash_function(hf);
}

int distributed_t::partition_function(state_t &state)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }
  
  if (cluster_size == 1)
    return 0;
  else {
    size_t len = state.size;
    unsigned char *data = reinterpret_cast<unsigned char*>(state.ptr);

    return (hasher.get_hash(data, len, NETWORK_HASH_SEED) % cluster_size);
  }
}

int distributed_t::get_state_net_id(state_t &state)
{
  return partition_function(state);
}

#if !defined(ORIG_POLL)
#define POLL_SKIP_MIN  0
#define POLL_SKIP_MAX  20 /* system dependent, but usually fine? */
#define MAX_RECV_COUNT (POLL_SKIP_MAX * 20) /* also needs investigation */
static int poll_skip_rate = POLL_SKIP_MAX;

static int all_skipped = 0;
static int sync_poll_skipped = 0;
static int flush_skipped = 0;
static int distr_sync_skipped = 0;

void distributed_t::force_poll(void)
{
  all_skipped = 0;
  sync_poll_skipped = 0;
}
#endif /* ! ORIG_POLL */

void distributed_t::process_messages(void)
{
  int size, src, tag;
  bool flag, some_new_msgs = false;
  char *buf;
  int sent_msgs, recved_msgs;
  int send_to;
  int total_pending_size;
  bool pending_safe = true;
  //bool orig_fbusy = fbusy;
  
#if !defined(ORIG_POLL)
  if (--all_skipped < 0)
    {
	all_skipped = poll_skip_rate;
    }
  else
    {
	return;
    }
#endif

  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  do {
#if defined(ORIG_POLL)
    network.flush_all_buffers_timed_out_only();
#else
    if (--flush_skipped <= 0)
      {
	network.flush_all_buffers_timed_out_only();
	flush_skipped = cluster_size * 10;

	/* Also check if we should adapt the overall polling rate */
	int received = network.stats_num (&network.stats_Recv_bytes);
	int sent = network.stats_num (&network.stats_Sent_bytes_local);
#if defined(OPT_GRID_CONFIG)
	sent += network.stats_num (&network.stats_Sent_bytes_remote);
#endif
	if (sent > (received + received / 16))
	  {
	    /* We're sending more than we're receiving, while in DiVinE
	     * it should typically be balanced. This could thus be a sign
	     * of not polling enough, so increase effective polling rate.
	     */
	    if (poll_skip_rate > POLL_SKIP_MIN)
	      {
		poll_skip_rate--;
	      }
	  }
	else
	  {
	    if (poll_skip_rate < POLL_SKIP_MAX)
	      {
		poll_skip_rate++;
	      }
	  }
      }
#endif
  
    do {
#if defined(ORIG_POLL)
      network.is_new_urgent_message(size, src, tag, flag);
#else
      if (--sync_poll_skipped < 0)
	{
          network.is_new_urgent_message(size, src, tag, flag);
	  sync_poll_skipped = 10; /* 100 too high for some tools? */
	}
      else
	{
	  flag = false;
	}
#endif
    
      if (flag)
	{
	  some_new_msgs = true;
	
#if defined(OPT_STATS_TAGS)
	  network.tag_count[tag & TAGS_MASK]++;
#endif
	  if (tag >= DIVINE_TAG_USER)
	    {
	      if (fproc_msgs_buf_exclusive_mem)
		{
		  buf = new char[size];
		  network.receive_urgent_message(buf, size, src, tag);
		}
	      else
		{
		  network.receive_urgent_message_non_exc(buf, size, src, tag);
		}
	      
              if (process_user_message != NULL)
                {
          	process_user_message(buf, size, src, tag, true);
                }
              else if (process_message_functor)
                {
                  process_message_functor->process_message(buf, size, src, tag, true);
                }
              else
                {
          	errvec << "Process user message is not defined" << thr(DISTRIBUTED_ERR_TYPE);
                }
	    }
          else
	    {
	      switch (tag)
		{
		case DIVINE_TAG_SYNC_ONE:
		  {
		    if (fsync_state >= 0)
		      {
			network.receive_urgent_message(static_cast<char *>(static_cast<void *>
									   (&sync_collector)), 
						       size, src, tag);
			
			(*fstatistics)[src].all_received_sync_msgs_cnt += 1;
		      
			if (network_id == NETWORK_ID_MANAGER)
			  {
			    // + 1 takes the precious receive into account
			    if (!fbusy && 
				sync_collector.received_msgs + 1 == sync_collector.sent_msgs)
			      {
				sync_one_result = sync_collector;
				
				fsync_state = DIVINE_TAG_SYNC_TWO;
				
				network.get_all_sent_msgs_cnt(sent_msgs);
				network.get_all_received_msgs_cnt(recved_msgs);
				
				// + 1 takes the next msgs into account
				sync_collector.sent_msgs = sent_msgs + 1; 
				sync_collector.received_msgs = recved_msgs;
				
				if (!finfo_exchange)
				  {
				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(static_cast<char *>
								(static_cast<void *>
								 (&sync_collector)), 
								sizeof(sync_collector), 
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
				else
				  {
				    char *sbuf;

				    sbuf = sync_sr_buf;

				    pinfo->update();

				    memcpy(sbuf, &sync_collector, sizeof(sync_collector));
				    memcpy(sbuf + sizeof(sync_collector), ptr_info_data, 
					   info_data_size);

				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(sbuf, 
								sizeof(sync_collector) + 
								info_data_size,
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
			      }
			    else
			      {
				fsync_state = DIVINE_TAG_SYNC_READY;
			      }
			  }
			else
			  {
			    if (!fbusy)
			      {
				network.get_all_sent_msgs_cnt(sent_msgs);
				network.get_all_received_msgs_cnt(recved_msgs);

				// + 1 takes the next msgs into account
				sync_collector.sent_msgs += sent_msgs + 1; 
				sync_collector.received_msgs += recved_msgs;

				send_to = (network_id + 1)%cluster_size;
				  
				network.send_urgent_message(static_cast<char *>
							    (static_cast<void *>
							     (&sync_collector)), 
							    sizeof(sync_collector), 
							    send_to, 
							    DIVINE_TAG_SYNC_ONE);

				(*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
			      }
			    else
			      {
				// the following causes the manager to refuse sync.
				sync_collector.sent_msgs = 0; 
				sync_collector.received_msgs = 10;

				send_to = NETWORK_ID_MANAGER;
				  
				network.send_urgent_message(static_cast<char *>
							    (static_cast<void *>
							     (&sync_collector)), 
							    sizeof(sync_collector), 
							    send_to, 
							    DIVINE_TAG_SYNC_ONE);

				(*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
			      }
			  }
		      }
		    else
		      {
			if (network_id == NETWORK_ID_MANAGER)
			  {
			    errvec << "Received DIVINE_TAG_SYNC_ONE when shouldn't have!"
				   << thr(DISTRIBUTED_ERR_TYPE);
			  }
			else
			  {
			    // the following causes the manager to refuse sync.
			    sync_collector.sent_msgs = 0; 
			    sync_collector.received_msgs = 10;
			  
			    send_to = NETWORK_ID_MANAGER;
			  
			    network.send_urgent_message(static_cast<char *>
							(static_cast<void *>
							 (&sync_collector)), 
							sizeof(sync_collector), 
							send_to, 
							DIVINE_TAG_SYNC_ONE);
			  
			    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
			  }
		      }
		    break;
		  }
		case DIVINE_TAG_SYNC_TWO:
		  {
		    if (fsync_state >= 0)
		      {
			if (!finfo_exchange)
			  {
			    network.receive_urgent_message(static_cast<char *>(static_cast<void *>
									       (&sync_collector)), 
							   size, src, tag);

			    (*fstatistics)[src].all_received_sync_msgs_cnt += 1;
			  }
			else
			  {
			    char *rbuf;
 			    
			    rbuf = sync_sr_buf;
			    
			    network.receive_urgent_message(rbuf, size, src, tag);
			  
			    (*fstatistics)[src].all_received_sync_msgs_cnt += 1;

			    memcpy(&sync_collector, rbuf, sizeof(sync_collector));
			    memcpy(ptr_info_data, rbuf + sizeof(sync_collector), 
				   info_data_size);
			  }

			if (network_id == NETWORK_ID_MANAGER)
			  {
			    // + 1 takes the previous receive into account
			    if (!fbusy && 
				sync_collector.received_msgs + 1 == sync_collector.sent_msgs &&
				sync_collector.sent_msgs == 
				sync_one_result.sent_msgs + cluster_size)
			      {
				
				fsync_state = DIVINE_TAG_SYNC_COMPLETION;

				if (!finfo_exchange)
				  {
				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(static_cast<char *>
								(static_cast<void *>
								 (&sync_collector)), 
								sizeof(sync_collector), 
								send_to, 
								DIVINE_TAG_SYNC_COMPLETION);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
				else
				  {
				    char *sbuf;

				    sbuf = sync_sr_buf;
 
				    memcpy(sbuf, &sync_collector, sizeof(sync_collector));
				    memcpy(sbuf + sizeof(sync_collector), ptr_info_data, 
					   info_data_size);
				  
				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(sbuf, 
								sizeof(sync_collector) + 
								info_data_size,
								send_to, 
								DIVINE_TAG_SYNC_COMPLETION);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
			      }
			    else
			      {
				fsync_state = DIVINE_TAG_SYNC_READY;
			      }
			  }
			else
			  {
			    if (!fbusy)
			      {
				network.get_all_sent_msgs_cnt(sent_msgs);
				network.get_all_received_msgs_cnt(recved_msgs);

				// + 1 takes the next msgs into account
				sync_collector.sent_msgs += sent_msgs + 1; 
				sync_collector.received_msgs += recved_msgs;

				if (!finfo_exchange)
				  {
				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(static_cast<char *>
								(static_cast<void *>
								 (&sync_collector)), 
								sizeof(sync_collector), 
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
				else
				  {
				    char *sbuf;

				    sbuf = sync_sr_buf;

				    pinfo->update();

				    memcpy(sbuf, &sync_collector, sizeof(sync_collector));
				    memcpy(sbuf + sizeof(sync_collector), ptr_info_data, 
					   info_data_size);

				    send_to = (network_id + 1)%cluster_size;

				    network.send_urgent_message(sbuf, 
								sizeof(sync_collector) + 
								info_data_size,
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
			      }
			    else
			      {
				// the following causes the manager to refuse sync.
				sync_collector.sent_msgs = 0; 
				sync_collector.received_msgs = 10;

				if (!finfo_exchange)
				  {
				    send_to = NETWORK_ID_MANAGER;

				    network.send_urgent_message(static_cast<char *>
								(static_cast<void *>
								 (&sync_collector)), 
								sizeof(sync_collector), 
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
				else
				  {
				    char *sbuf;

				    sbuf = sync_sr_buf;

				    memcpy(sbuf, &sync_collector, sizeof(sync_collector));
				    // this message causes the sync round to stop, so
				    // i probably could send trash instead of info, but
				    // for now i send the current info content.
				    memcpy(sbuf + sizeof(sync_collector), ptr_info_data, 
					   info_data_size);

				    send_to = NETWORK_ID_MANAGER;

				    network.send_urgent_message(sbuf, 
								sizeof(sync_collector) + 
								info_data_size,
								send_to, 
								DIVINE_TAG_SYNC_TWO);

				    (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
				  }
			      }
			  }
		      }
		    else
		      {
			errvec << "Received DIVINE_TAG_SYNC_TWO when shouldn't have!"
			       << thr(DISTRIBUTED_ERR_TYPE);

		      }
		    break;
		  }
		case DIVINE_TAG_SYNC_COMPLETION:
		  {
		    if (fsync_state >= 0)
		      {
			if (!finfo_exchange)
			  {
			    network.receive_urgent_message(static_cast<char *>(static_cast<void *>
									       (&sync_collector)), 
							   size, src, tag);

			    (*fstatistics)[src].all_received_sync_msgs_cnt += 1;
			  }
			else
			  {
			    char *rbuf;
 			    
			    rbuf = sync_sr_buf;
			    
			    network.receive_urgent_message(rbuf, size, src, tag);

			    (*fstatistics)[src].all_received_sync_msgs_cnt += 1;
			  
			    memcpy(&sync_collector, rbuf, sizeof(sync_collector));
			    memcpy(ptr_info_data, rbuf + sizeof(sync_collector), 
				   info_data_size);
			  }

			if (network_id == NETWORK_ID_MANAGER)
			  {
			    fsynchronized = true;
			  }
			else
			  {
			    fsynchronized = true;
			  
			    if (!finfo_exchange)
			      {
				send_to = (network_id + 1)%cluster_size;

				network.send_urgent_message(static_cast<char *>
							    (static_cast<void *>
							     (&sync_collector)), 
							    sizeof(sync_collector), 
							    send_to, 
							    DIVINE_TAG_SYNC_COMPLETION);

				(*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
			      }
			    else
			      {
				char *sbuf;
				
				sbuf = sync_sr_buf;
				
				memcpy(sbuf, &sync_collector, sizeof(sync_collector));
				memcpy(sbuf + sizeof(sync_collector), ptr_info_data, 
				       info_data_size);
				
				send_to = (network_id + 1)%cluster_size;
				  
				network.send_urgent_message(sbuf, 
							    sizeof(sync_collector) + 
							    info_data_size,
							    send_to, 
							    DIVINE_TAG_SYNC_COMPLETION);

				(*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
			      }
			  }
		      }
		    else
		      {
			errvec << "Received DIVINE_TAG_SYNC_COMPLETION when shouldn't have!"
			       << thr(DISTRIBUTED_ERR_TYPE);
		      }
		    break;
		  }
		default:
		  {
		    errvec << "Unprocessed urgent internal message!" << thr(DISTRIBUTED_ERR_TYPE);
		  }
		}
	    }
	}
    } while (false); 
    // replacing false with flag can cause infinite loop 
    // if other workstations respond to synchronization
    // messages too quickly


#if !defined(ORIG_POLL) && defined(MAX_RECV_COUNT)
    int recvcnt = 0;
#endif

    do {
      network.is_new_message(size, src, tag, flag);

      if (flag)
	{
	  some_new_msgs = true;

#if defined(OPT_STATS_TAGS)
	  network.tag_count[tag & TAGS_MASK]++;
#endif
	  if (tag >= DIVINE_TAG_USER)
           {
             if (fproc_msgs_buf_exclusive_mem)
              {
                buf = new char[size];
                network.receive_message(buf, size, src, tag);
              }
             else
	      {
               network.receive_message_non_exc(buf, size, src, tag);
              }
              
             if (process_user_message != NULL)
              {
                process_user_message(buf, size, src, tag, false);
              }
             else if (process_message_functor)
              {
                process_message_functor->process_message(buf, size, src, tag, false);
              }
             else
              {
                errvec << "Process user message is not defined" << thr(DISTRIBUTED_ERR_TYPE);
              }
           }
         else
           {
             // special internal message processing, currently none -> throw error
             errvec << "Unprocessed internal message" << thr(DISTRIBUTED_ERR_TYPE);
           }
       }
#if !defined(ORIG_POLL) && defined(MAX_RECV_COUNT)
       if (++recvcnt > MAX_RECV_COUNT) {
	 force_poll();
	 break;
       }
#endif
    } while (flag);

    /* the check is switched off
    if (pending_safe) {
      network.get_all_total_pending_size(total_pending_size, false);
      if (total_pending_size > 32*1024) {
	cout << network_id << ": "
	     << "Pending size estimate exceeded limit, total_pending_size = " 
	     << total_pending_size << endl;
	network.get_all_total_pending_size(total_pending_size, true);
	if (total_pending_size > 32*1024) {
	  pending_safe = false;
	  fbusy = true; // avoid synchronization
	  cout << network_id << ": "
	       << "Pending safe set to false, total_pending_size = " 
	       << total_pending_size << endl;
	}
      }
    } else {
      network.get_all_total_pending_size(total_pending_size, true);
      cout << network_id << ": "
	   << "total_pending_size = " << total_pending_size << endl;
    }
    */
  } while (!pending_safe && total_pending_size == 0);

  /*
  fbusy = orig_fbusy; // restore original value
  if (!pending_safe) {
    cout << network_id << ": "
         << "Pending safe state restored" << endl;	    
  }
  */

  if (network_id == NETWORK_ID_MANAGER && !fbusy && !some_new_msgs && 
#if !defined(ORIG_POLL)
      fsync_state == DIVINE_TAG_SYNC_READY && --distr_sync_skipped < 0)
#else
      fsync_state == DIVINE_TAG_SYNC_READY)
#endif
    {
#if !defined(ORIG_POLL)
      /* Reduce number of sync requests significantly.
       * Note there is also an interaction with the optimized flushing
       * strategy: as that now applies rate control (currently using
       * message counters taken from the lowest communication layer),
       * we could totally use up the per-slice packet quantum with
       * sync requests here, if we're not careful.
       */
      distr_sync_skipped = 100;
#endif
      fsync_state = DIVINE_TAG_SYNC_ONE;

      network.get_all_sent_msgs_cnt(sent_msgs);
      network.get_all_received_msgs_cnt(recved_msgs);

      sync_collector.sent_msgs = sent_msgs + 1; // + 1 takes the next msgs into account
      sync_collector.received_msgs = recved_msgs;

      send_to = (network_id + 1)%cluster_size;

      network.send_urgent_message(static_cast<char *>
				  (static_cast<void *> (&sync_collector)), 
				  sizeof(sync_collector), 
				  send_to, DIVINE_TAG_SYNC_ONE);

      (*fstatistics)[send_to].all_sent_sync_msgs_cnt += 1;
    }
}


bool distributed_t::synchronized(void)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  if (fsync_state < 0)
    {
      fsynchronized = false;
      fsync_state = DIVINE_TAG_SYNC_READY;
    }
  
  if (fsynchronized)
    {
      network.barrier();

      fstat_all_sync_barriers_cnt += 1;

      fsync_state = -1;
      finfo_exchange = false;

      if (sync_sr_buf != NULL)
		{
		  delete[] sync_sr_buf;
		  sync_sr_buf = NULL;
		}
    }
  
  return fsynchronized;
}

bool distributed_t::synchronized(abstract_info_t &info)
{
  if (mode != NORMAL)
    {
      errvec << "Something is not properly initialized" << thr(DISTRIBUTED_ERR_TYPE);
    }

  if (fsync_state < 0)
    {
      info.get_data_ptr(ptr_info_data);
      info.get_data_size(info_data_size);
      pinfo = &info;

      if (sync_sr_buf != NULL)
		{
		  errvec << "Non-NULL sync_sr_buf at the beginning of sync process!" 
			 << thr(DISTRIBUTED_ERR_TYPE);
		}

      sync_sr_buf = new char[sizeof(sync_collector) + info_data_size];

      finfo_exchange = true;
    }

  return synchronized();
}











