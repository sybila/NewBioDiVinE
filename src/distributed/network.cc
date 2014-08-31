#include <iostream>
#include <ios>
#include <iomanip>
#include <string.h>
#include "distributed/network.hh"
#include "distributed/distributed.hh"
  
  using namespace divine;
  
#if !defined(ORIG_FLUSH)
/* The maximum flush rate is system dependent; 9000 seems fine for DAS-3.
 * A reasonably good value of the max flush rate actually appears to be the
 * average message rate that is achieved when the system has NOT yet
 * reached the distributed termination detection phase.  Should be
 * easy to implement.
 */
static int max_flush_rate = 9000;
#endif
 
comm_matrix_t::comm_matrix_t(int rows, int cols, error_vector_t& arg0):errvec(arg0)
{
  int i, j;

  if (rows < 1 || cols < 1)
    {
      errvec << "Comm Matrix: Create: Invalid dimensions" << thr();
    }

  elems = new int*[rows];
  rowcount = rows;
  colcount = cols;

  for (i=0; i < rows; i++)
    {
      elems[i] = new int[cols];
      for (j=0; j < cols; j++)
	{
	  // all elements are initialized to 0
	  elems[i][j] = 0;
	}
    }
}

comm_matrix_t::~comm_matrix_t(void)
{
  int i;

  for (i=0; i < rowcount; i++)
    {  
      delete elems[i];
    }

  delete []elems;
}

int& comm_matrix_t::operator()(int row, int col)
{ 
  if (row < 0 || row >= rowcount || col < 0 || col >= colcount)
    {
      errvec << "Comm Matrix: Operator(): Constant out of range!" << thr();
    }
  return elems[row][col];
}
 
namespace divine {
pcomm_matrix_t operator-(const pcomm_matrix_t& m)
{
  int i, j;
  pcomm_matrix_t ret(new comm_matrix_t(m->rowcount, m->colcount));

  for (i=0; i < m->rowcount; i++)
    for (j=0; j < m->colcount; j++)
      {
	(*ret)(i, j) = -(*m)(i,j);
      }

  return ret;
}

pcomm_matrix_t operator-(const pcomm_matrix_t& m1, const pcomm_matrix_t& m2)
{
  if (m1->rowcount != m2->rowcount || m1->colcount != m2->colcount)
    {
      m1->errvec << "Comm Matrix: Operator-: Different argument dimensions!" << thr();
    }

//  return (m1 + (-m2));
  pcomm_matrix_t neg_m2 = -m2;
  return (m1 + neg_m2);
}

pcomm_matrix_t operator+(const pcomm_matrix_t& m1, const pcomm_matrix_t& m2)
{
  if (m1->rowcount != m2->rowcount || m1->colcount != m2->colcount)
    {
      m1->errvec << "Comm Matrix: Operator+: Different argument dimensions!" << thr();
    }
  
  int i, j;
  pcomm_matrix_t ret(new comm_matrix_t(m1->rowcount, m1->colcount));

  for (i=0; i < m1->rowcount; i++)
    for (j=0; j < m1->colcount; j++)
      {
	(*ret)(i, j) = (*m1)(i,j) + (*m2)(i,j);
      }
 
  return ret;

}

pcomm_matrix_t operator*(int a, const pcomm_matrix_t& m)
{
  int i, j;
  pcomm_matrix_t ret(new comm_matrix_t(m->rowcount, m->colcount));
 
  for (i=0; i < m->rowcount; i++)
    for (j=0; j < m->colcount; j++)
      {
	(*ret)(i, j) = a * (*m)(i,j);
      }

  return ret;
}

pcomm_matrix_t operator*(const pcomm_matrix_t& m, int a)
{
  return a*m;
}
}

network_t::network_t(error_vector_t& arg0):errvec(arg0)
{
  fnetwork_initialized = false;
  finitialized = false;

  size_of_int = static_cast<int>(sizeof(int));

  fbuf_msgs_cnt_limit = 100;
  fbuf_size_limit = NETBUFFERSIZE;
  fbuf_time_limit_sec = 0;
  fbuf_time_limit_msec = 300; // default timeout = 300 miliseconds
  ftimed_flushing_enabled = true;

  send_buffer = NULL;
  recv_buffer = NULL;
  urgent_recv_buffer = NULL;
  tmp_sbuf = NULL;
  tmp_rbuf = NULL;
  tmp_buf_cl_size = NULL;
}

network_t::~network_t()
{
  int i;
  net_send_buffer_t *sbuf, *tmpsbuf;

  // Destroy buffers if not done already
  if (send_buffer != NULL)
    {
      for (i=0; i < fcluster_size; i++)
	{
	  sbuf = (*send_buffer)[i].first;
	  while (sbuf) {
	    delete []sbuf->ptr;
	    tmpsbuf = sbuf;
	    sbuf = sbuf->next;
	    delete tmpsbuf;
	  }
	}
      
      delete send_buffer;
    }

  if (recv_buffer != NULL)
    {
      for (i=0; i < fcluster_size; i++)
	{
	  delete [](*recv_buffer)[i].ptr;
	}
      
      delete recv_buffer;
    }

  if (urgent_recv_buffer != NULL)
    {
      for (i=0; i < fcluster_size; i++)
	{
	  delete [](*urgent_recv_buffer)[i].ptr;
	}
      
      delete urgent_recv_buffer;
    }

  if (tmp_rbuf != NULL)
    {
      delete []tmp_rbuf;
    }

  if (tmp_sbuf != NULL)
    {
      delete []tmp_sbuf;
    }

  if (tmp_buf_cl_size != NULL)
    {
      delete []tmp_buf_cl_size;
    }

  fnetwork_initialized = false;
  finitialized = false;
}

bool network_t::initialize_network(int &argc, char **&argv)
{
  int name_length;

  // Thow error if network has been already initialized
  if (fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_ALREADY_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // MPI Initialization
  flast_mpi_rc = MPI_Init (&argc, &argv);
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_INITIALIZATION_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  
  // Get number of computers
  flast_mpi_rc = MPI_Comm_size (MPI_COMM_WORLD, &fcluster_size);
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_INITIALIZATION_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  
  // Get this computer's unique identification number
  flast_mpi_rc = MPI_Comm_rank (MPI_COMM_WORLD, &fid);
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_INITIALIZATION_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  
  // Get this computer's name
  flast_mpi_rc = MPI_Get_processor_name (fprocessor_name, &name_length);
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_INITIALIZATION_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_PRESYNC)
  if (getenv("USE_PRESYNC") != NULL)
    {
      /* Do a naive (!) alltoall to get everyone talk to everyone else.
       * Otherwise new TCP connections over ethernet may be very hard to
       * establish while many nodes are already blasting to others.
       */
      char c;
      double start = MPI_Wtime();
      MPI_Request *recvreqs = new MPI_Request[fcluster_size];
      MPI_Request *sendreqs = new MPI_Request[fcluster_size];
      MPI_Status  *stats    = new MPI_Status [fcluster_size];
      char        *result   = new char[fcluster_size];
  
      MPI_Barrier(MPI_COMM_WORLD);
      /* startup receives */
      for (int i = 0; i < fcluster_size; i++) {
          MPI_Irecv(&result[i], 1, MPI_CHAR, i, 1, MPI_COMM_WORLD, &recvreqs[i]);
      }
      for (int i = 0; i < fcluster_size; i++) {
          MPI_Isend(&c, 1, MPI_CHAR, (fcluster_size + i) % fcluster_size,
		    1, MPI_COMM_WORLD, &sendreqs[i]);

          MPI_Barrier(MPI_COMM_WORLD);
          if (fid == 0) {
	    cout << " " << i;
	  }
      }
      MPI_Waitall(fcluster_size, recvreqs, stats);
      MPI_Waitall(fcluster_size, sendreqs, stats);
      MPI_Barrier(MPI_COMM_WORLD);
      delete [] recvreqs;
      delete [] sendreqs;
      delete [] stats;
      delete [] result;
      if (fid == 0) {
  	cout << ": synchronized " << fcluster_size << " cpus in "
  	     << (MPI_Wtime() - start) << " sec" << endl;
      }
    }
#endif

#if !defined(ORIG_FLUSH)
    char *max_flush_env = getenv("MAX_FLUSH_RATE");
    if (max_flush_env != NULL)
      {
	/* modify flush rate as specified */
	max_flush_rate = atoi(max_flush_env);
      }
#endif

  // Network initialization successful
  fnetwork_initialized = true;

  return true;
}

bool network_t::initialize_buffers()
{
  int i;
  
  // Throw error if network is not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_STATS_MPI_CALLS)
  stats_init(&stats_Test);
  stats_init(&stats_Isend);
  stats_init(&stats_Isend_urg);
  stats_init(&stats_Probe);
  stats_init(&stats_Iprobe0);
  stats_init(&stats_Iprobe1);
  stats_init(&stats_Recv);
  stats_init(&stats_Barrier);
  stats_init(&stats_Gather);
  stats_init(&stats_Allgather);
#endif

#if defined(OPT_STATS_MPI_RATE)
  stats_init(&stats_Sent_bytes_local);
  stats_init(&stats_Recv_bytes);
#if defined(OPT_GRID_CONFIG)
  stats_init(&stats_Sent_bytes_remote);
#endif

  stats_time_begin = MPI_Wtime();
  stats_slice_begin = stats_time_begin;
  stats_slice_end   = stats_slice_begin + STATS_INTERVAL;
#endif /* OPT_STATS_MPI_RATE */

#if defined(OPT_GRID_CONFIG)
  /* env GRID_CFG=00001111 specifies an 8-cpu grid with two 4-cpu clusters */
  grid_config = getenv("GRID_CFG");
  if (grid_config == NULL)
    {
      if (fid == 0)
	{
	  cerr << "warning: missing GRID_CFG environment var" << endl;
	}
    }
  else
    {
      if (strlen(grid_config) != fcluster_size)
        {
	  if (fid == 0)
	    {
	       cerr << "warning: GRID_CFG is not " << fcluster_size
		    << " chars long" << endl;
	    }
	  grid_config = NULL;
        }
#define local_dest(dst) ((grid_config == NULL) || (grid_config[dst] == grid_config[fid]))
#define first_node_this_cluster() (fid == 0 || !local_dest(fid - 1))
    }
#endif /* OPT_GRID_CONFIG */

  send_buffer = new vector<net_send_buffers_t>(fcluster_size);
  recv_buffer = new vector<net_recv_buffer_t>(fcluster_size);
  urgent_recv_buffer = new vector<net_urgent_recv_buffer_t>(fcluster_size);
  
  // Initialize auxiliary buffers
  tmp_sbuf = new char[2 * fbuf_size_limit];
  tmp_rbuf = new char[2 * fbuf_size_limit];
  tmp_buf_cl_size = new int[fcluster_size];

  for (i=0; i < fcluster_size; i++)
    {
      /*
      (*send_buffer)[i].ptr = new char[2 * fbuf_size_limit];
      (*send_buffer)[i].size = 0;
      (*send_buffer)[i].msgs_cnt = 0;
      */
      (*send_buffer)[i].first = NULL; // actual memory will be allocated when needed
      (*send_buffer)[i].current = NULL;
      (*send_buffer)[i].last = NULL;
      (*send_buffer)[i].total_allocated_size = 0;
      (*send_buffer)[i].total_pending_size = 0;
      (*send_buffer)[i].all_msgs_cnt = 0;
      (*send_buffer)[i].all_urgent_msgs_cnt = 0;
      (*send_buffer)[i].flushes_cnt = 0;
      /*
      if (ftimed_flushing_enabled)
	(*send_buffer)[i].flush_timeout.settimeout(fbuf_time_limit_sec, fbuf_time_limit_msec);
      */
      (*recv_buffer)[i].ptr = new char[2 * fbuf_size_limit];
      (*recv_buffer)[i].position = 0;
      (*recv_buffer)[i].size = 0;
      (*recv_buffer)[i].all_msgs_cnt = 0;
      (*urgent_recv_buffer)[i].ptr = new char[2 * fbuf_size_limit];
      (*urgent_recv_buffer)[i].size = 0;
      (*urgent_recv_buffer)[i].all_urgent_msgs_cnt = 0;
    }

  while (!recv_buffers_queue.empty())
    {
      recv_buffers_queue.pop();
    }

  while (!urgent_recv_buffers_queue.empty())
    {
      /* was: recv_buffers_queue.pop(); */
      urgent_recv_buffers_queue.pop();
    }

  fstat_sent_messages_cnt = 0;
  fstat_sent_messages_size = 0;
  fstat_sent_urgent_messages_cnt = 0;
  fstat_sent_urgent_messages_size = 0;
 
  fstat_received_messages_cnt = 0;
  fstat_received_messages_size = 0;
  fstat_received_urgent_messages_cnt = 0;
  fstat_received_urgent_messages_size = 0;

  fstat_all_barriers_cnt = 0;

  // Initialization complete and successful
  finitialized = true;

  return true;
}

bool network_t::finalize()
{
  int i;
  net_send_buffer_t *sbuf, *tmpsbuf;

  // Throw error if network is not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // MPI Finalization
  MPI_Barrier(MPI_COMM_WORLD); // prevent premature calls to MPI_Finalize (see MPI-1.2 standards)
  flast_mpi_rc = MPI_Finalize();

  fnetwork_initialized = false;

#if defined(OPT_STATS_MPI_CALLS)
  cout << fid << ": Isend    : n " << stats_num(&stats_Isend) <<
           " avg " << stats_mean(&stats_Isend) * 1e6 <<
	   " min " << stats_min(&stats_Isend) * 1e6  <<
	   " max " << stats_max(&stats_Isend) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Isend) * 1e6 << endl;
  cout << fid << ": IsendUrg : n " << stats_num(&stats_Isend_urg) <<
           " avg " << stats_mean(&stats_Isend_urg) * 1e6 <<
	   " min " << stats_min(&stats_Isend_urg) * 1e6  <<
	   " max " << stats_max(&stats_Isend_urg) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Isend_urg) * 1e6 << endl;
  cout << fid << ": Test     : n " << stats_num(&stats_Test) <<
           " avg " << stats_mean(&stats_Test) * 1e6 <<
	   " min " << stats_min(&stats_Test) * 1e6  <<
	   " max " << stats_max(&stats_Test) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Test) * 1e6 << endl;
  cout << fid << ": Probe    : n " << stats_num(&stats_Probe) <<
           " avg " << stats_mean(&stats_Probe) * 1e6 <<
	   " min " << stats_min(&stats_Probe) * 1e6  <<
	   " max " << stats_max(&stats_Probe) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Probe) * 1e6 << endl;
  cout << fid << ": Iprobe0  : n " << stats_num(&stats_Iprobe0) <<
           " avg " << stats_mean(&stats_Iprobe0) * 1e6 <<
	   " min " << stats_min(&stats_Iprobe0) * 1e6  <<
	   " max " << stats_max(&stats_Iprobe0) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Iprobe0) * 1e6 << endl;
  cout << fid << ": Iprobe1  : n " << stats_num(&stats_Iprobe1) <<
           " avg " << stats_mean(&stats_Iprobe1) * 1e6 <<
	   " min " << stats_min(&stats_Iprobe1) * 1e6  <<
	   " max " << stats_max(&stats_Iprobe1) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Iprobe1) * 1e6 << endl;
  cout << fid << ": Recv     : n " << stats_num(&stats_Recv) <<
           " avg " << stats_mean(&stats_Recv) * 1e6 <<
	   " min " << stats_min(&stats_Recv) * 1e6  <<
	   " max " << stats_max(&stats_Recv) * 1e6  <<
 	   " var " << stats_sqrt_var(&stats_Recv) * 1e6 << endl;
  cout << fid << ": Barrier  : n " << stats_num(&stats_Barrier) <<
           " avg " << stats_mean(&stats_Barrier) * 1e6     <<
	   " min " << stats_min(&stats_Barrier)  * 1e6     <<
	   " max " << stats_max(&stats_Barrier) *1e6       <<
 	   " var " << stats_sqrt_var(&stats_Barrier) * 1e6 << endl;
  cout << fid << ": Gather   : n " << stats_num(&stats_Gather) <<
           " avg " << stats_mean(&stats_Gather) * 1e6     <<
	   " min " << stats_min(&stats_Gather) * 1e6      <<
	   " max " << stats_max(&stats_Gather) * 1e6      <<
 	   " var " << stats_sqrt_var(&stats_Gather) * 1e6 << endl;
  cout << fid << ": Allgather: n " << stats_num(&stats_Allgather) <<
           " avg " << stats_mean(&stats_Allgather) * 1e6     <<
	   " min " << stats_min(&stats_Allgather) * 1e6      <<
	   " max " << stats_max(&stats_Allgather) * 1e6      <<
 	   " var " << stats_sqrt_var(&stats_Allgather) * 1e6 << endl;
#endif /* OPT_STATS_MPI_CALLS */

  if (finitialized) {
    // Destroy send and receive buffers
    for (i=0; i < fcluster_size; i++)
      {
	sbuf = (*send_buffer)[i].first;
	while (sbuf) {
	  delete []sbuf->ptr;
	  tmpsbuf = sbuf;
	  sbuf = sbuf->next;
	  delete tmpsbuf;
	}
	delete [](*recv_buffer)[i].ptr;
	delete [](*urgent_recv_buffer)[i].ptr;
      }
    
    delete send_buffer;
    send_buffer = NULL;
    delete recv_buffer;
    recv_buffer = NULL;
    delete []tmp_sbuf;
    tmp_sbuf = NULL;
    delete []tmp_rbuf;
    tmp_rbuf = NULL;
    delete []tmp_buf_cl_size;
    tmp_buf_cl_size = NULL;
    
    delete urgent_recv_buffer;
    urgent_recv_buffer = NULL;
    
    finitialized = false;
  }

  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_FINALIZATION_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  

  return true;
}

bool network_t::get_id(int &id)
{
  // Throw error if not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  id = fid;

  return true;
}

bool network_t::get_cluster_size(int &size)
{
  // Throw error if not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  size = fcluster_size;

  return true;
}

bool network_t::get_processor_name(char *proc_name, int& length)
{
  // Throw error if not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  length = strlen(fprocessor_name);
  if (proc_name != NULL)
    strcpy(proc_name, fprocessor_name);

  return true;
}

bool network_t::get_buf_msgs_cnt_limit(int& limit)
{
  limit = fbuf_msgs_cnt_limit;

  return true;
}

bool network_t::get_buf_size_limit(int& limit)
{
  limit = fbuf_size_limit;

  return true;
}

bool network_t::get_buf_time_limit(int& limit_sec, int& limit_msec)
{
  limit_sec = fbuf_time_limit_sec;
  limit_msec = fbuf_time_limit_msec;

  return true;
}

bool network_t::set_buf_msgs_cnt_limit(int limit)
{
  // Throw error if already initialized
  if (finitialized)
    {
      flast_net_rc = NET_ERR_ALREADY_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  fbuf_msgs_cnt_limit = limit;

  return true;
}
 
bool network_t::set_buf_size_limit(int limit)
{
  // Throw error if already initialized
  if (finitialized)
    {
      flast_net_rc = NET_ERR_ALREADY_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  fbuf_size_limit = limit;

  return true;
}

bool network_t::set_buf_time_limit(int limit_sec, int limit_msec)
{
  // Throw error if already initialized
  if (finitialized)
    {
      flast_net_rc = NET_ERR_ALREADY_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  fbuf_time_limit_sec = limit_sec;
  fbuf_time_limit_msec = limit_msec;

  if (fbuf_time_limit_sec == 0 && fbuf_time_limit_msec == 0)
    ftimed_flushing_enabled = false;
  else
    ftimed_flushing_enabled = true;

  return true;
}

bool network_t::get_all_sent_msgs_cnt(int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = fstat_sent_messages_cnt + fstat_sent_urgent_messages_cnt;

  return true;
}

bool network_t::get_all_received_msgs_cnt(int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = fstat_received_messages_cnt + fstat_received_urgent_messages_cnt;

  return true;
}

bool network_t::get_user_sent_msgs_cnt(int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = fstat_sent_messages_cnt;

  return true;
}

bool network_t::get_user_received_msgs_cnt(int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = fstat_received_messages_cnt;

  return true;
}

   
// get number of messages sent to a given workstation
bool network_t::get_sent_msgs_cnt_sent_to(int to,int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if the workstation number is invalid
  if (to < 0 || to >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_WORKSTATION_NUMBER;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = (*send_buffer)[to].all_msgs_cnt;
  return true;
}


int network_t::get_sent_msgs_cnt_sent_to(int to)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return -1;
    }

  // Throw error if the workstation number is invalid
  if (to < 0 || to >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_WORKSTATION_NUMBER;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return -1;
    }

  return ((*send_buffer)[to].all_msgs_cnt);
}


// get number of messages received from given workstation
bool network_t::get_recv_msgs_cnt_recv_from(int from,int& cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if the workstation number is invalid
  if (from < 0 || from >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_WORKSTATION_NUMBER;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = (*recv_buffer)[from].all_msgs_cnt;

  return true;
}


int network_t::get_recv_msgs_cnt_recv_from(int from)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return -1;
    }

  // Throw error if the workstation number is invalid
  if (from < 0 || from >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_WORKSTATION_NUMBER;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return -1;
    }

  return ((*recv_buffer)[from].all_msgs_cnt);
}


bool network_t::get_all_barriers_cnt(int &cnt)
{
  // Throw error if not initialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = fstat_all_barriers_cnt;

  return true;
}

bool network_t::barrier(void)
{
  // Throw error if network is not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_STATS_MPI_CALLS)
  double start = MPI_Wtime();
#endif
  flast_mpi_rc = MPI_Barrier(MPI_COMM_WORLD);
#if defined(OPT_STATS_MPI_CALLS)
  stats_update(&stats_Barrier, MPI_Wtime() - start);
#endif

  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_BARRIER_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  fstat_all_barriers_cnt += 1;

  return true;
}


bool network_t::gather(char *sbuf, int ssize, char *rbuf, int rsize, int root)
{
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_STATS_MPI_CALLS)
  double start = MPI_Wtime();
#endif
  flast_mpi_rc = MPI_Gather(sbuf, ssize, MPI_CHAR, rbuf, rsize, MPI_CHAR, root, MPI_COMM_WORLD);
#if defined(OPT_STATS_MPI_CALLS)
  stats_update(&stats_Gather, MPI_Wtime() - start);
#endif

  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_GATHER_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  return true;
}


bool network_t::all_gather(char *sbuf, int ssize, char *rbuf, int rsize)
{
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_STATS_MPI_CALLS)
  double start = MPI_Wtime();
#endif
  flast_mpi_rc = MPI_Allgather(sbuf, ssize, MPI_CHAR, rbuf, rsize, MPI_CHAR, MPI_COMM_WORLD);
#if defined(OPT_STATS_MPI_CALLS)
  stats_update(&stats_Allgather, MPI_Wtime() - start);
#endif

  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_ALLGATHER_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  return true;
}

bool network_t::get_comm_matrix(net_comm_matrix_type_t cm_type, pcomm_matrix_t& ret, int target)
{
  int i, ssize;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  switch (cm_type) {
  case ncmt_snm:
    {
      for (i=0; i < fcluster_size; i++)
	{
	  tmp_buf_cl_size[i] = (*send_buffer)[i].all_msgs_cnt;
	}
      break;
    }
  case ncmt_sum:
    {
      for (i=0; i < fcluster_size; i++) 
	{
	  tmp_buf_cl_size[i] = (*send_buffer)[i].all_urgent_msgs_cnt;
	}
      break;
    }
  case ncmt_rnm:
    {
      for (i=0; i < fcluster_size; i++)
	{
	  tmp_buf_cl_size[i] = (*recv_buffer)[i].all_msgs_cnt;
	}
      break;
    }
  case ncmt_rum:
    {
      for (i=0; i < fcluster_size; i++)
	{
	  tmp_buf_cl_size[i] = (*urgent_recv_buffer)[i].all_urgent_msgs_cnt;
	}
      break;
    }
  }

  ssize = fcluster_size * size_of_int;

  if (fid == target)
    {
      int j;

      int *tmp_buf_cl_sizexcl_size = new int[fcluster_size * fcluster_size];

      if (!gather(static_cast<char *>(static_cast<void *>(tmp_buf_cl_size)), 
		  ssize, 
		  static_cast<char *>(static_cast<void *>(tmp_buf_cl_sizexcl_size)), 
		  ssize, 
		  target))
	{
	  return false;
	}
      
      pcomm_matrix_t tmp_ret(new comm_matrix_t(fcluster_size, fcluster_size));
      
      for (i = 0; i < fcluster_size; i++)
  	for (j = 0; j < fcluster_size; j++)
  	  {
  	    (*tmp_ret)(i, j) = tmp_buf_cl_sizexcl_size[i * fcluster_size + j];
   	  }
      
      delete []tmp_buf_cl_sizexcl_size;

      ret = tmp_ret;
    }
  else
    {
      if (!gather(static_cast<char *>(static_cast<void *>(tmp_buf_cl_size)), 
		  ssize, 
		  NULL, 
		  0, 
		  target))
	{
	  return false;
	}
    }

  return true;
}

bool network_t::get_comm_matrix_snm(pcomm_matrix_t& ret, int target)
{
  return get_comm_matrix(ncmt_snm, ret, target);
}

bool network_t::get_comm_matrix_rnm(pcomm_matrix_t& ret, int target)
{
  return get_comm_matrix(ncmt_rnm, ret, target);
} 

bool network_t::get_comm_matrix_sum(pcomm_matrix_t& ret, int target)
{
  return get_comm_matrix(ncmt_sum, ret, target);
}

bool network_t::get_comm_matrix_rum(pcomm_matrix_t& ret, int target)
{
  return get_comm_matrix(ncmt_rum, ret, target);
}


bool network_t::abort(void)
{
  // Throw error if not intialized
  if (!fnetwork_initialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  flast_mpi_rc = MPI_Abort(MPI_COMM_WORLD, 0);

  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_ABORT_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  return true;
}

#if !defined(ORIG_FLUSH)
/* Note: the new LIMIT_SENDS code requires more checking.  The option was
 * enabled during the DAS-3 cluster/grid experiments, but it still needs to
 * be evaluated on a larger scale, when and how much it actually helps, etc.
 * TODO.
 */
#define LIMIT_SENDS
#endif
#if defined(LIMIT_SENDS)
static int nqueued_max;
#endif

bool network_t::send_message_ex(char* buf, int size, int dest, int tag, bool urgent)
{
  net_send_buffer_t *sb, *tmpsb;
  int flag;
#if defined(LIMIT_SENDS)
#define MAX_SENDS_QUEUED 10
#define MAX_WAIT 100
  int nqueued;
  int retried = 0;
#endif

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
   
  // Throw error if msg size exceeds permitted limit
  if (size + 2 * size_of_int > fbuf_size_limit)
    {
      flast_net_rc = NET_ERR_INVALID_MSG_SIZE;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if destination does not exist
  if (dest < 0 || dest >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_DESTINATION;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(LIMIT_SENDS)
  again:
   nqueued = 0;
#endif
  net_send_buffers_t& sbuffs = (*send_buffer)[dest];
  sb = sbuffs.current;
  if (!sb || sb->pending) {
    flag = 0;
    if (sb) { // sb->pending == true
      sb = NULL;
      tmpsb = sbuffs.first;
      do {
	flag = 0;
	if (tmpsb->pending) {
#if defined(OPT_STATS_MPI_CALLS)
	  double start = MPI_Wtime();
#endif
	  if (MPI_Test(&tmpsb->req, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
	    errvec << "MPI_Test failed!" << thr();
	    return false;
	  }
#if defined(OPT_STATS_MPI_CALLS)
	  stats_update(&stats_Test, MPI_Wtime() - start);
#endif
	} else {
	  flag = 1;
	}
	if (flag) {
	  sb = tmpsb;
	}
	tmpsb = tmpsb->next;
#if defined(LIMIT_SENDS)
	nqueued++;
	if (nqueued > nqueued_max) {
	    nqueued_max = nqueued;
	}
#endif
      } while (!sb && tmpsb);
    }
    if (flag) {
      if (sb->msgs_cnt != 0) {
	errvec << "There is available buffer with non-zero msgs_cnt!" << thr();
	return false;
      }
      if (sb->pending && sb->size == 0) {
	errvec << "There was pending buffer with zero size!" << thr();
	return false;
      }
      if (sb->pending)
	sbuffs.total_pending_size -= sb->size;
      if (sbuffs.total_pending_size < 0) {
	errvec << "Problem with calculating total_pending_size, it went negative!" << thr();
	return false;
      }
      sb->size = 0;
      sb->pending = false;
      sbuffs.current = sb;
#if defined(LIMIT_SENDS)
      if (nqueued >= MAX_SENDS_QUEUED / 2)
	{
	  /* try to do additional cleanup further down the queue to
	   * reduce memory footprint for send buffers.
	   */
	  net_send_buffer_t *sbp, *sbpprev, *sbpnext;
	  int thisflag;
	
	  sbpprev = sb;
	  for (sbp = sb->next; sbp != NULL; sbp = sbpnext)
	    {
	      sbpnext = sbp->next;

	      thisflag = 0;
	      if (sbp->pending) {
#if defined(OPT_STATS_MPI_CALLS)
		 double start = MPI_Wtime();
#endif
		 if (MPI_Test(&sbp->req, &thisflag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
		     errvec << "MPI_Test failed!" << thr();
		     return false;
		 }
#if defined(OPT_STATS_MPI_CALLS)
		 stats_update(&stats_Test, MPI_Wtime() - start);
#endif
	      } else {
		 thisflag = 1;
	      }
	      if (thisflag)
		{
		  /* this buffer can be freed */
		  if (sbp->pending)
		    sbuffs.total_pending_size -= sbp->size;
		  if (sbuffs.total_pending_size < 0)
		    {
		      errvec << "Problem with calculating total_pending_size, it went negative!" << thr();
		      return false;
		    }
		  sbpprev->next = sbpnext;
		  if (sbpnext == NULL)
		    {
		      /* last one */
		      sbuffs.last = sbpprev;
		    }
		  delete []sbp->ptr;
		  delete sbp;
		}
	      else
		{
		  /* buffer cannot be freed yet */
		  sbpprev = sbp;
		}
	    }
	}
#endif /* LIMIT_SENDS */
    } else {
#if defined(LIMIT_SENDS)
      if (nqueued >= MAX_SENDS_QUEUED) {
        /* flow control attempt: wait a bit for previous sends to complete */
	if (++retried < MAX_WAIT) {
	    /* TODO: we might want to poll here to achieve more progress
	     * while holding back the send, but unclear if the tools can
	     * handle upcalls while sending.
	     */
	    goto again;
	}
	/* else we go on as usual; at least we held back a bit */
      }
#endif
      sb = new net_send_buffer_t;
    
      sb->ptr = new char[2 * fbuf_size_limit];
      sb->size = 0;
      sb->msgs_cnt = 0;
      sb->pending = false;
      sb->next = NULL;
      if (ftimed_flushing_enabled)
	sb->flush_timeout.settimeout(fbuf_time_limit_sec, fbuf_time_limit_msec);

      sbuffs.current = sb;
      if (sbuffs.last) {
	sbuffs.last->next = sb;
	sbuffs.last = sb;
      } else {
	sbuffs.first = sb;
	sbuffs.last = sb;
      }
      sbuffs.total_allocated_size += 2 * fbuf_size_limit;
    }
  } else {
    if (urgent) {
      // musi se vybrat jiny buffer nez current
      sb = NULL;
      tmpsb = sbuffs.first;
      do {
	flag = 0;
	if (tmpsb != sbuffs.current) {
	  if (tmpsb->pending) {
#if defined(OPT_STATS_MPI_CALLS)
	    double start = MPI_Wtime();
#endif
	    if (MPI_Test(&tmpsb->req, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
	      errvec << "MPI_Test failed!" << thr();
	      return false;
	    }
#if defined(OPT_STATS_MPI_CALLS)
	    stats_update(&stats_Test, MPI_Wtime() - start);
#endif
	  } else {
	    flag = 1;
	  }
	}
	if (flag) {
	  sb = tmpsb;
	}
	tmpsb = tmpsb->next;
      } while (!sb && tmpsb);
    
      if (flag) {
	if (sb->msgs_cnt != 0) {
	  errvec << "There is available buffer with non-zero msgs_cnt!" << thr();
	  return false;
	}
	if (sb->pending && sb->size == 0) {
	  errvec << "There was pending buffer with zero size!" << thr();
	  return false;
	}
	if (sb->pending)
	  sbuffs.total_pending_size -= sb->size;
	if (sbuffs.total_pending_size < 0) {
	  errvec << "Problem with calculating total_pending_size, it went negative!" << thr();
	  return false;
	}
	sb->size = 0;
	sb->pending = false;
      } else {
	sb = new net_send_buffer_t;
	
	sb->ptr = new char[2 * fbuf_size_limit];
	sb->size = 0;
	sb->msgs_cnt = 0;
	sb->pending = false;
	sb->next = NULL;
	
	// pouze pro odeslani urgentni zpravy, nenastavuje se jako current
	if (sbuffs.last) {
	  sbuffs.last->next = sb;
	  sbuffs.last = sb;
	} else {
	  sbuffs.first = sb;
	  sbuffs.last = sb;
	}
	
	sbuffs.total_allocated_size += 2 * fbuf_size_limit;
      }
    }
  }

  // Copy message to the right place
  memcpy(sb->ptr + sb->size, &size, size_of_int);
  memcpy(sb->ptr + sb->size + size_of_int, &tag, size_of_int);
  memcpy(sb->ptr + sb->size + 2 * size_of_int, buf, size);

  if (urgent) {
    if (sb->msgs_cnt != 0 || sb->size != 0) {
      errvec << "Nonempty sendbuf before sending urgent message!" << thr();
      return false;
    }
    // msgs_cnt is not increased because the message is sent immediately
    sb->size += size + 2 * size_of_int; // size is increased foor the sake of calculating
                                        // total_pending_size

#if defined(OPT_STATS_MPI_CALLS)
    double start = MPI_Wtime();
#endif
    // Urgent message is sent immediately
    flast_mpi_rc = MPI_Isend(sb->ptr, sb->size, 
			      MPI_CHAR, dest, NET_TAG_URGENT, MPI_COMM_WORLD, &sb->req);
#if defined(OPT_STATS_MPI_CALLS)
    stats_update(&stats_Isend_urg, MPI_Wtime() - start);
#endif
#if defined(OPT_STATS_MPI_RATE)
#if defined(OPT_GRID_CONFIG)
    if (local_dest(dest))
      {
        stats_update(&stats_Sent_bytes_local, (double) sb->size);
      }
    else
      {
        stats_update(&stats_Sent_bytes_remote, (double) sb->size);
      }
#else
    stats_update(&stats_Sent_bytes_local, (double) sb->size);
#endif
#endif /* OPT_STATS_MPI_RATE */
    
    if (flast_mpi_rc != MPI_SUCCESS)
      {
	flast_net_rc = NET_ERR_SEND_MSG_FAILED;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
    }

    sb->pending = true;
    sbuffs.total_pending_size += sb->size;
    sbuffs.all_urgent_msgs_cnt++;
    fstat_sent_urgent_messages_cnt++;
    fstat_sent_urgent_messages_size += size + 2 * size_of_int;
  } else {
    // Normal message is saved to buffer
    // If this is the first message in the buffer -> reset timeout
    if (ftimed_flushing_enabled && sb->msgs_cnt == 0)
      {
	sb->flush_timeout.reset();
      }
    sb->msgs_cnt++;
    sb->size += size + 2 * size_of_int;
    
    // If one of limits is exceeded -> flush buffer
#if defined(ORIG_FLUSH)
    int test_flush_timeout = ftimed_flushing_enabled;
#else
    /* Don't perform the flush timeout check right here, as this
     * costs a syscall per msg fragment added to the buffer.
     */
    int test_flush_timeout = 0;
#endif
    if (test_flush_timeout) {
      if (sb->msgs_cnt >= fbuf_msgs_cnt_limit || 
	  sb->size >= fbuf_size_limit || 
	  sb->flush_timeout.testtimeout())
	{
	  if (!flush_buffer(dest))
	    return false;
	}
    } else {
      if (sb->msgs_cnt >= fbuf_msgs_cnt_limit || 
	  sb->size >= fbuf_size_limit)
	{
	  if (!flush_buffer(dest))
	    return false;
	}
    }
    
    sbuffs.all_msgs_cnt++;
    fstat_sent_messages_cnt++;
    fstat_sent_messages_size += size + 2 * size_of_int;
  }

  return true;
}

bool network_t::send_message(char* buf, int size, int dest, int tag)
{
  return (send_message_ex(buf, size, dest, tag, false));
}

bool network_t::send_message(const message_t & message, int dest, int tag)
{
  return send_message((char*)(message.get_data()),
                      message.get_written_size(), dest, tag);
}

bool network_t::send_urgent_message(char* buf, int size, int dest, int tag)
{  
  return (send_message_ex(buf, size, dest, tag, true));
}

bool network_t::send_urgent_message(const message_t & message, int dest, int tag)
{
  return send_urgent_message((char*)(message.get_data()),
                             message.get_written_size(), dest, tag);
}

bool network_t::flush_buffer(int dest)
{
  net_send_buffer_t *sb;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if destination does not exist
  if (dest < 0 || dest >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_DESTINATION;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  net_send_buffers_t& sbuffs = (*send_buffer)[dest];
  sb = sbuffs.current;

  // Send message if any
  if (sb) {
    if (sb->msgs_cnt > 0)
      {
	if (sb->pending) {
	  errvec << "Pending state set incorrectly!" << thr();
	  return false;
	}
	
#if defined(OPT_STATS_MPI_CALLS)
        double start = MPI_Wtime();
#endif
	flast_mpi_rc = MPI_Isend(sb->ptr, sb->size, MPI_CHAR, dest, 
				  NET_TAG_NORMAL, MPI_COMM_WORLD, &sb->req);
#if defined(OPT_STATS_MPI_CALLS)
        stats_update(&stats_Isend, MPI_Wtime() - start);
#endif
#if defined(OPT_STATS_MPI_RATE)
#if defined(OPT_GRID_CONFIG)
        if (local_dest(dest))
          {
            stats_update(&stats_Sent_bytes_local, (double) sb->size);
          }
        else
          {
            stats_update(&stats_Sent_bytes_remote, (double) sb->size);
          }
#else
        stats_update(&stats_Sent_bytes_local, (double) sb->size);
#endif
#endif /* OPT_STATS_MPI_RATE */
	
	if (flast_mpi_rc != MPI_SUCCESS)
	  {
	    flast_net_rc = NET_ERR_SEND_MSG_FAILED;
	    errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	    return false;
	  }
	
	sb->pending = true;
	sbuffs.total_pending_size += sb->size;
	sbuffs.flushes_cnt++;
      }
    
    sb->msgs_cnt = 0;
    // sb->size = 0; // to calculate total_pending_size correctly, size must be kept until
                     // the message is delivered (its pending state returns to false)
  }

  return true;
}

bool network_t::get_buffer_flushes_cnt(int dest, int& cnt)
{
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if destination does not exist
  if (dest < 0 || dest >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_DESTINATION;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  net_send_buffers_t& sbuffs = (*send_buffer)[dest];

  cnt = sbuffs.flushes_cnt;

  return true;
}

bool network_t::get_all_buffers_flushes_cnt(int& cnt)
{
  int i, cnt_one;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  cnt = 0;

  for (i=0; i < fcluster_size; i++)
    {
      if (!get_buffer_flushes_cnt(i, cnt_one))
	return false;

      cnt += cnt_one;
    }

  return true;
}

#if defined(OPT_STATS_MPI_PENDING)

bool network_t::get_total_pending_stats(int dest, int& size, int& curpkts, int& maxpkts, bool perform_test) {
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if destination does not exist
  if (dest < 0 || dest >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_DESTINATION;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  net_send_buffers_t& sbuffs = (*send_buffer)[dest];

  if (!perform_test) {
    size = sbuffs.total_pending_size;
    curpkts = 0;
    maxpkts = 0;
  } else {
    net_send_buffer_t *sb;
    int flag;
    int pkts = 0;
    int allpkts = 0;

    sb = sbuffs.first;
    while (sb) {
      allpkts++;
      if (sb->pending) {
	flag = 0;
#if defined(OPT_STATS_MPI_CALLS)
	double start = MPI_Wtime();
#endif
	if (MPI_Test(&sb->req, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
	  errvec << "MPI_Test failed!" << thr();
	  return false;
	}
#if defined(OPT_STATS_MPI_CALLS)
	stats_update(&stats_Test, MPI_Wtime() - start);
#endif
	if (flag) {
	  if (sb->msgs_cnt > 0) {
	    errvec << "There was pending buffer with non-zero msgs_cnt!" << thr();
	    return false;
	  }
	  if (sb->size == 0) {
	    errvec << "There was pending buffer with zero size!" << thr();
	    return false;
	  }
	  sbuffs.total_pending_size -= sb->size;
	  if (sbuffs.total_pending_size < 0) {
	    errvec << "Problem with calculating total_pending_size, it went negative!" << thr();
	    return false;
	  }
	  sb->size = 0;
	  sb->pending = false;
	} else {
	  pkts++;
	}
      }
      sb = sb->next;
    }

    size = sbuffs.total_pending_size;
    curpkts = pkts;
    maxpkts = allpkts;
  }

  return true;
}

bool network_t::get_all_total_pending_stats(int& size, int& curpkts, int& maxpkts, bool perform_test) {
  int i, size_one;
  int curpkts_one, maxpkts_one;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  size = 0;
  curpkts = 0;
  maxpkts = 0;

  for (i=0; i < fcluster_size; i++)
    {
      if (!get_total_pending_stats(i, size_one, curpkts_one, maxpkts_one, perform_test))
	return false;

      size += size_one;
      curpkts += curpkts_one;
      maxpkts += maxpkts_one;
    }

  return true;
}

#endif /* OPT_STATS_MPI_PENDING */

bool network_t::get_total_pending_size(int dest, int& size, bool perform_test) {
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  // Throw error if destination does not exist
  if (dest < 0 || dest >= fcluster_size)
    {
      flast_net_rc = NET_ERR_INVALID_DESTINATION;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  net_send_buffers_t& sbuffs = (*send_buffer)[dest];

  if (!perform_test) {
    size = sbuffs.total_pending_size;
  } else {
    net_send_buffer_t *sb;
    int flag;

    sb = sbuffs.first;
    while (sb) {
      if (sb->pending) {
	flag = 0;
#if defined(OPT_STATS_MPI_CALLS)
	double start = MPI_Wtime();
#endif
	if (MPI_Test(&sb->req, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
	  errvec << "MPI_Test failed!" << thr();
	  return false;
	}
#if defined(OPT_STATS_MPI_CALLS)
	stats_update(&stats_Test, MPI_Wtime() - start);
#endif
	if (flag) {
	  if (sb->msgs_cnt > 0) {
	    errvec << "There was pending buffer with non-zero msgs_cnt!" << thr();
	    return false;
	  }
	  if (sb->size == 0) {
	    errvec << "There was pending buffer with zero size!" << thr();
	    return false;
	  }
	  sbuffs.total_pending_size -= sb->size;
	  if (sbuffs.total_pending_size < 0) {
	    errvec << "Problem with calculating total_pending_size, it went negative!" << thr();
	    return false;
	  }
	  sb->size = 0;
	  sb->pending = false;
	}
      }
      sb = sb->next;
    }

    size = sbuffs.total_pending_size;
  }

  return true;
}

bool network_t::get_all_total_pending_size(int& size, bool perform_test) {
  int i, size_one;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  size = 0;

  for (i=0; i < fcluster_size; i++)
    {
      if (!get_total_pending_size(i, size_one, perform_test))
	return false;

      size += size_one;
    }

  return true;
}



bool network_t::flush_all_buffers()
{
  int i;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  for (i=0; i < fcluster_size; i++)
    {
      if (!flush_buffer(i))
	return false;
    }

  return true;
}


#if !defined(ORIG_FLUSH)

bool network_t::flush_some_buffers()
{
  int largest = -1; /* currently only flush largest pending buffer */
  int largest_size = -1;
  int size;
  int i;
  net_send_buffer_t *sb;

#if defined(OPT_STATS_MPI_RATE)
  /* Apply rate control when flushing buffers. */
  double now = MPI_Wtime();
  int max_pkts = max_flush_rate;

  if (stats_slice_begin <= now && now <= stats_slice_end)
    {
       /* scale pkts allowed to time passed in this slice */
       max_pkts = (int) (((double) max_pkts) *
	 ((now - stats_slice_begin) / STATS_INTERVAL));
    }

  int sent = stats_num (&stats_Sent_bytes_local);
#if defined(OPT_GRID_CONFIG)
  sent += stats_num (&stats_Sent_bytes_remote);
#endif
  if (sent > max_pkts)
    {
      return true;
    }
#else /* ! OPT_STATS_MPI_RATE */
  /* simpler (untuned) version with just static flush rate reduction */
  static int flush_skipped;

  if (--flush_skipped < 0)
    {
      flush_skipped = 100;
    }
  else
    {
      return true;
    }
#endif /* OPT_STATS_MPI_RATE */

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  for (i=0; i < fcluster_size; i++)
    {
      net_send_buffers_t& sbuffs = (*send_buffer)[i];
      sb = sbuffs.current;

      if (sb != NULL && sb->msgs_cnt > 0 && sb->size >= largest_size)
	{
	  largest = i;
	  largest_size = sb->size;
	}
    }

  if (largest >= 0)
    {
      if (!flush_buffer(largest))
	return false;
    }
  return true;
}

#endif /* ! ORIG_FLUSH */

bool network_t::flush_all_buffers_timed_out_only()
{
  net_send_buffer_t *sb;
  int i;

  if (!ftimed_flushing_enabled)
    return true;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if !defined(ORIG_TIMEOUT)
  // added to avoid many gettimeofday() calls
  timeval current;
  gettimeofday(&current,0);
#endif

  for (i=0; i < fcluster_size; i++)
    {
      sb = (*send_buffer)[i].current;
#if defined(ORIG_TIMEOUT)
      if (sb && sb->msgs_cnt > 0 && sb->flush_timeout.testtimeout())
#else
      if (sb && sb->msgs_cnt > 0 && sb->flush_timeout.testtimeout(current))
#endif
	if (!flush_buffer(i))
	  return false;
    }

  return true;
}


bool network_t::message_probe(int src, int tag, bool &flag, int &size, 
			      MPI_Status& status, bool blocking)
{
  int mpi_flag;
#if defined(OPT_STATS_MPI_RATE) || defined(OPT_STATS_MPI_CALLS)
  double start = MPI_Wtime();
#endif

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  if (blocking) {
    flast_mpi_rc = MPI_Probe(src, tag, 
			     MPI_COMM_WORLD, &status);
#if defined(OPT_STATS_MPI_CALLS)
    stats_update(&stats_Probe, MPI_Wtime() - start);
#endif
    mpi_flag = 1; // If blocking probe does not finish with error, there is always a message
  } else {
    flast_mpi_rc = MPI_Iprobe(src, tag, 
			      MPI_COMM_WORLD, &mpi_flag, &status);
#if defined(OPT_STATS_MPI_CALLS)
    if (mpi_flag != 0)
      {
	stats_update(&stats_Iprobe1, MPI_Wtime() - start);
      }
    else
      {
	stats_update(&stats_Iprobe0, MPI_Wtime() - start);
      }
#endif
  }
  
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_MSG_PROBE_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  
  flag = (mpi_flag != 0);

  if (flag)
    {
      flast_mpi_rc = MPI_Get_count(&status, MPI_CHAR, &size);
      
      if (flast_mpi_rc != MPI_SUCCESS)
	{
	  flast_net_rc = NET_ERR_GET_MSG_SIZE_FAILED;
	  errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	  return false;
	}
    }

#if defined(OPT_STATS_MPI_RATE)
  if (start >= stats_slice_end)
    {
#if defined(OPT_STATS_MPI_RATE_PRINT)
      double interval = (start - stats_slice_begin);
      static int cnt;

      int printit;
#if 0 /* defined(OPT_GRID_CONFIG) */
      printit = first_node_this_cluster();
#else
      printit = 1;
#endif
      if (printit)
	{
#if defined(OPT_STATS_MPI_PENDING)
	  int pending, curpkts, maxpkts;

	  get_all_total_pending_stats(pending, curpkts, maxpkts, true);
#endif
	  /* only print first cluster of each site and all of cluster 0 */
	  cout <<
		"t "   << (unsigned) (start - stats_time_begin) <<
		" "    << cnt++ <<
		" i "  << setw(3) << fid <<
		" #R " << setw(4) << (unsigned) stats_num (&stats_Recv_bytes) <<
		" #S " << setw(5) << (unsigned) stats_num (&stats_Sent_bytes_local)  <<
#if defined(OPT_GRID_CONFIG)
		" #Sr "<< setw(4) << (unsigned) stats_num (&stats_Sent_bytes_remote) <<
#endif
           	" a "  << setw(4) << (unsigned) stats_mean(&stats_Sent_bytes_local) <<
#if 0 /* defined(OPT_GRID_CONFIG) */
           	" ar " << setw(4) << (unsigned) stats_mean(&stats_Sent_bytes_remote) <<
	   	" Mr " << setw(4) << (unsigned) stats_max (&stats_Sent_bytes_remote) <<
#endif
		std::fixed << std::setprecision(1) <<
		" R " << ((stats_num (&stats_Recv_bytes) *
           		     stats_mean(&stats_Recv_bytes)) /
			     (1024 * 1024) / interval) <<
		" T " << ((stats_num (&stats_Sent_bytes_local) *
           		     stats_mean(&stats_Sent_bytes_local)) /
			     (1024 * 1024) / interval) <<
#if defined(OPT_GRID_CONFIG)
		" Tr " << ((stats_num (&stats_Sent_bytes_remote) *
           		     stats_mean(&stats_Sent_bytes_remote)) /
			     (1024 * 1024) / interval) <<
#endif
		"";

	  cout << " P" << " " << (vm.getvmsize() / 1024.0);

#if defined(OPT_STATS_MPI_PENDING)
	  cout << " " << setw(6) << pending
		<< " " << setw(2) << nqueued_max
		<< " " << setw(3) << curpkts
		<< " " << setw(3) << maxpkts;
#endif

#if defined(OPT_STATS_TAGS)
	  for (int i = 0; i <= DIVINE_TAG_SYNC_COMPLETION; i++)
	    {
	      if (tag_count[i] != 0)
		{
		   cout << " S" << i << " " << tag_count[i];
		   tag_count[i] = 0;
		}
	    }
	  for (int i = 0; i < (TAGS_MAX - DIVINE_TAG_USER); i++)
	    {
	      if (tag_count[DIVINE_TAG_USER + i] != 0)
		{
		  cout <<" U" << i << " " << tag_count[DIVINE_TAG_USER + i];
		  tag_count[DIVINE_TAG_USER + i] = 0;
		}
	    }
#endif

	  cout << endl;
	}
#endif /* OPT_STATS_MPI_RATE_PRINT */

        /* Start new slice, with new msg counts */
        stats_init(&stats_Sent_bytes_local);
	stats_init(&stats_Recv_bytes);
#if defined(OPT_GRID_CONFIG)
	stats_init(&stats_Sent_bytes_remote);
#endif

	stats_slice_begin = start;
	stats_slice_end = start + STATS_INTERVAL;
     }
#endif /* OPT_STATS_MPI_RATE */

  return true;
}

bool network_t::is_new_message_ex(int& size, int& src, int& tag, bool& flag, 
				  bool urgent, bool from_source, bool blocking)
{
  MPI_Status status;
  int msg_size = 0, msg_tag, msg_src;
  char *ptr; // used only to make the compilation possible
  
  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  flag = false;

  if (from_source) {
    if (src < 0 || src >= fcluster_size) 
      {
	flast_net_rc = NET_ERR_INVALID_SOURCE;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }

    if (urgent) {
      net_urgent_recv_buffer_t& urb = (*urgent_recv_buffer)[src];
      
      if (urb.size == 0)
	{
	  if (!message_probe(src, NET_TAG_URGENT, flag, msg_size, status, blocking))
	    return false;
	  
	  if (flag)
	    {
	      if (msg_size == 0) {
		flast_net_rc = NET_ERR_MSG_PROBE_FAILED;
		errvec << net_err_msgs[flast_net_rc].message 
		       << thr(net_err_msgs[flast_net_rc].type);
		return false;
	      }
	      
	      if (receive_urgent_message_ex(ptr, msg_size, src, msg_tag, 
					    false, true, true))
		{
		  size = msg_size;
		  tag = msg_tag;
		} else {
		  return false;
		}
	    }
	}
      else
	{
	  flag = true;
	  memcpy(&msg_size, urb.ptr, size_of_int);
	  memcpy(&msg_tag, urb.ptr + size_of_int, size_of_int);
	  size = msg_size;
	  tag = msg_tag;
	}
    } else {
      net_recv_buffer_t& rb = (*recv_buffer)[src];
      
      if (rb.size == 0)
	{
	  if (!message_probe(src, NET_TAG_NORMAL, flag, msg_size, status, blocking))
	    return false;
	  
	  if (flag)
	    {
	      if (msg_size == 0) {
		flast_net_rc = NET_ERR_MSG_PROBE_FAILED;
		errvec << net_err_msgs[flast_net_rc].message 
		       << thr(net_err_msgs[flast_net_rc].type);
		return false;
	      }
	      
	      if (receive_message_ex(ptr, msg_size, src, msg_tag, false, true, true))
		{
		  size = msg_size;
		  tag = msg_tag;
		} else {
		  return false;
		}
	    }
	  
	}
      else
	{
	  memcpy(&msg_size, rb.ptr + rb.position, size_of_int);
	  memcpy(&msg_tag, rb.ptr + rb.position + size_of_int, size_of_int);
	  
	  flag = true;
	  
	  size = msg_size;
	  tag = msg_tag;
	}
    }
    
  } else {
    if (urgent) {
      if (urgent_recv_buffers_queue.empty())
	{
	  if (!message_probe(MPI_ANY_SOURCE, NET_TAG_URGENT, flag, msg_size, status, blocking))
	    return false;
	  
	  if (flag)
	    {
	      if (msg_size == 0) {
		flast_net_rc = NET_ERR_MSG_PROBE_FAILED;
		errvec << net_err_msgs[flast_net_rc].message 
		       << thr(net_err_msgs[flast_net_rc].type);
		return false;
	      }

	      if (receive_urgent_message_ex(ptr, msg_size, status.MPI_SOURCE, msg_tag, 
					    false, true, true))
		{
		  size = msg_size;
		  tag = msg_tag;
		  src = status.MPI_SOURCE;
		} else {
		  return false;
		}
	    }
	}
      else
	{
	  msg_src = urgent_recv_buffers_queue.front();
	  net_urgent_recv_buffer_t& urb = (*urgent_recv_buffer)[msg_src];
	  
	  flag = true;
	  memcpy(&msg_size, urb.ptr, size_of_int);
	  memcpy(&msg_tag, urb.ptr + size_of_int, size_of_int);
	  src = msg_src;
	  size = msg_size;
	  tag = msg_tag;
	}
    } else {
      if (recv_buffers_queue.empty())
	{
	  if (!message_probe(MPI_ANY_SOURCE, NET_TAG_NORMAL, flag, msg_size, status, blocking))
	    return false;
	  
	  if (flag) {
	    if (msg_size == 0) {
	      flast_net_rc = NET_ERR_MSG_PROBE_FAILED;
	      errvec << net_err_msgs[flast_net_rc].message 
		     << thr(net_err_msgs[flast_net_rc].type);
	      return false;
	    }

	    if (receive_message_ex(ptr, msg_size, status.MPI_SOURCE, msg_tag, false, true, true))
	      {
		size = msg_size;
		src = status.MPI_SOURCE;
		tag = msg_tag;
	      } else {
		return false;
	      }
	  }
	}
      else
	{
	  msg_src = recv_buffers_queue.front();
	  
	  net_recv_buffer_t& rb = (*recv_buffer)[msg_src];
	  
	  memcpy(&msg_size, rb.ptr + rb.position, size_of_int);
	  memcpy(&msg_tag, rb.ptr + rb.position + size_of_int, size_of_int);
	  
	  flag = true;
	  
	  src = msg_src;
	  size = msg_size;
	  tag = msg_tag;
	}
    }
  }

  return true;
}

bool network_t::is_new_message(int& size, int& src, int& tag, bool& flag)
{
  return (is_new_message_ex(size, src, tag, flag, false, false, false));
}

bool network_t::is_new_message_from_source(int& size, int src, int& tag, bool& flag)
{
  return (is_new_message_ex(size, src, tag, flag, false, true, false));
}

bool network_t::is_new_urgent_message(int& size, int& src, int& tag, bool& flag)
{
  return (is_new_message_ex(size, src, tag, flag, true, false, false));
}

bool network_t::is_new_urgent_message_from_source(int& size, int src, int& tag, bool& flag)
{
  return (is_new_message_ex(size, src, tag, flag, true, true, false));
}

bool network_t::receive_message_from_network(char *buf, int size, int src, int tag, 
					     MPI_Status& status)
{
  int received_msg_size;

#if defined(OPT_STATS_MPI_CALLS)
  double start = MPI_Wtime();
#endif
  flast_mpi_rc = MPI_Recv(buf, size, MPI_CHAR, src, tag, MPI_COMM_WORLD, &status);
#if defined(OPT_STATS_MPI_CALLS)
  stats_update(&stats_Recv, MPI_Wtime() - start);
#endif
      
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  flast_mpi_rc = MPI_Get_count(&status, MPI_CHAR, &received_msg_size);
  
  if (flast_mpi_rc != MPI_SUCCESS)
    {
      flast_net_rc = NET_ERR_GET_MSG_SIZE_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }
  
  if (received_msg_size != size)
    {
      flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

#if defined(OPT_STATS_MPI_RATE)
  stats_update(&stats_Recv_bytes, (double) received_msg_size);
#endif
  
  return true;
}


bool network_t::receive_message_ex(char *&buf, int &size, int& src, int& tag,
				   bool non_exc, bool from_source, bool called_internally)
{
  int msg_size, msg_tag, msg_src, complete_msg_size = 0, buffer_front, i;
  bool flag;
  MPI_Status status;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  if (from_source) {
    // check whether the source is in the allowed interval
    if (src < 0 || src >= fcluster_size)
      {
	flast_net_rc = NET_ERR_INVALID_SOURCE;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }

    msg_src = src;
  } else {
    if (recv_buffers_queue.empty())
      {
	// All buffers are empty -> receive message from network
	if (called_internally) {
	  msg_size = size;

	  complete_msg_size = msg_size;
	  
	  if (!receive_message_from_network(tmp_rbuf, msg_size, MPI_ANY_SOURCE, 
					    NET_TAG_NORMAL, status))
	    return false;
	  
	  msg_src = status.MPI_SOURCE;
	} else {
	  if (!is_new_message_ex(size, src, tag, flag, false, false, true))
	    return false;

	  msg_src = src;
	}
      } else {
	// There is a non-empty buffer -> use the topmost one
	msg_src = recv_buffers_queue.front();
      }
  }

  net_recv_buffer_t& rb = (*recv_buffer)[msg_src];
 
  if (rb.size == 0) {
    if (from_source) {
      // Selected buffer is empty -> receive message from network
      if (called_internally) {
	msg_size = size;

	complete_msg_size = msg_size;
	
	if (!receive_message_from_network(rb.ptr, msg_size, msg_src, NET_TAG_NORMAL, status))
	  return false;
      } else {
	if (!is_new_message_ex(size, msg_src, tag, flag, false, true, true))
	  return false;

	complete_msg_size = rb.size;
      }
    } else {
      if (!called_internally) {
	// The buffer should have been already filled -> throw error

	flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }
    }

    if (called_internally) {
      rb.position = 0;
      rb.size = complete_msg_size;
      
      recv_buffers_queue.push(msg_src);
    }

    if (complete_msg_size == 0)
      {
	flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }
  }

  memcpy(&msg_size, rb.ptr + rb.position, size_of_int);
  memcpy(&msg_tag, rb.ptr + rb.position + size_of_int, size_of_int);
  
  
  if (!called_internally)
    {
      // If exclusive memory is not required for the buffer, 
      // return pointer to position in internal buffer
      if (non_exc)
	buf = rb.ptr + rb.position + 2 * size_of_int;
      else
	memcpy(buf, rb.ptr + rb.position + 2 * size_of_int, msg_size);
      
      rb.position += msg_size + 2 * size_of_int;
      
      if (rb.position >= rb.size)
	{
	  if (from_source) {
	    buffer_front = recv_buffers_queue.front();
	    recv_buffers_queue.pop();
	    if (buffer_front != src) {
	      // the right item is deeper in the queue
	      recv_buffers_queue.push(buffer_front);
	      for (i=0; i < static_cast<int>(recv_buffers_queue.size()) - 1; i++)
		{
		  buffer_front = recv_buffers_queue.front();
		  recv_buffers_queue.pop();
		  if (buffer_front != src)
		    recv_buffers_queue.push(buffer_front);
		}
	    }
	  } else {
	    recv_buffers_queue.pop();	    
	  }
	  rb.size = 0;
	}
      
      rb.all_msgs_cnt++;
      fstat_received_messages_cnt++;
      fstat_received_messages_size += msg_size + 2 * size_of_int;
    }


  src = msg_src;
  tag = msg_tag;
  size = msg_size;

  return true;
}


bool network_t::receive_message(char *buf, int& size, int& src, int& tag)
{
  return (receive_message_ex(buf, size, src, tag, false, false, false));
}

bool network_t::receive_message(message_t & message, int& src, int& tag)
{
  bool flag;
  int size;
  char * buf;
  is_new_message_ex(size, src, tag, flag, false, false, true);
  if (!flag) errvec << "Unexpected error: network_t::receive_message()"
                    << thr();
  if (message.get_allocated_size()<static_cast<unsigned int>(size))//if message is not sufficiently large
    buf = new char[size]; //allocate sufficiently large piece of memory
  else buf = (char *)(message.get_data());
  
  bool result = receive_message(buf, size, src, tag);

  if (message.get_allocated_size()<static_cast<unsigned int>(size))//if message is not sufficiently large
    message.set_data((byte_t *)(buf), size);//set received data to message
  else
    message.set_written_size(size);//else data has been written directly
                                   //to message and thus only change a size of
                                   //written data
  
  return result;
}

bool network_t::receive_message_non_exc(char *&buf, int& size, int& src, int& tag)
{
  return (receive_message_ex(buf, size, src, tag, true, false, false));
}

bool network_t::receive_message_from_source(char *buf, int& size, int src, int& tag)
{
  return (receive_message_ex(buf, size, src, tag, false, true, false));
}

bool network_t::receive_urgent_message_ex(char *&buf, int &size, int& src, int& tag, 
					  bool non_exc, bool from_source, bool called_internally)
{
  int msg_size, msg_tag, msg_src, complete_msg_size = 0, buffer_front, i;
  bool flag;
  MPI_Status status;

  // Throw error if not intialized
  if (!finitialized)
    {
      flast_net_rc = NET_ERR_NOT_INITIALIZED;
      errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
      return false;
    }

  if (from_source) {
    // check whether the source is in the allowed interval
    if (src < 0 || src >= fcluster_size)
      {
	flast_net_rc = NET_ERR_INVALID_SOURCE;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }

    msg_src = src;
  } else {
    if (urgent_recv_buffers_queue.empty())
      {
	// All buffers are empty -> receive message from network
	if (called_internally) {
	  msg_size = size;
	  
	  complete_msg_size = msg_size;
 
	  if (!receive_message_from_network(tmp_rbuf, msg_size, MPI_ANY_SOURCE, 
					    NET_TAG_URGENT, status))
	    return false;
	  
	  msg_src = status.MPI_SOURCE;
	} else {
	  if (!is_new_message_ex(size, src, tag, flag, true, false, true))
	    return false;

	  msg_src = src;
	}
      } else {
	// There is a non-empty buffer -> use the topmost one
	msg_src = urgent_recv_buffers_queue.front();
      }
  }

  net_urgent_recv_buffer_t& urb = (*urgent_recv_buffer)[msg_src];

  if (urb.size == 0) {
    if (from_source) {
      // Selected buffer is empty -> receive message from network
      if (called_internally) {
	msg_size = size;

	complete_msg_size = msg_size;
	
	if (!receive_message_from_network(urb.ptr, msg_size, msg_src, NET_TAG_URGENT, status))
	  return false;
      } else {
	if (!is_new_message_ex(size, msg_src, tag, flag, true, true, true))
	  return false;

	complete_msg_size = urb.size;
      }
    } else {
      if (!called_internally) {
	// The buffer should have been already filled

	flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }
    }
    
    if (called_internally) {
      urb.size = complete_msg_size;
	
      urgent_recv_buffers_queue.push(msg_src);
    }

    if (complete_msg_size == 0)
      {
	flast_net_rc = NET_ERR_RECEIVE_MSG_FAILED;
	errvec << net_err_msgs[flast_net_rc].message << thr(net_err_msgs[flast_net_rc].type);
	return false;
      }
  }

  memcpy(&msg_size, urb.ptr, size_of_int);
  memcpy(&msg_tag, urb.ptr + size_of_int, size_of_int);

  if (!called_internally)
    {
      if (non_exc)
	buf = urb.ptr + 2 * size_of_int;
      else
	memcpy(buf, urb.ptr + 2 * size_of_int, msg_size);

      // urb is for one message only, when the message is processed, the buffer is empty
      urb.size = 0; 
                  
      if (from_source) {
	buffer_front = urgent_recv_buffers_queue.front();
	urgent_recv_buffers_queue.pop();
	if (buffer_front != src) {
	  // the right item is deeper in the queue
	  urgent_recv_buffers_queue.push(buffer_front);
	  for (i=0; i < static_cast<int>(urgent_recv_buffers_queue.size()) - 1; i++)
	    {
	      buffer_front = urgent_recv_buffers_queue.front();
	      urgent_recv_buffers_queue.pop();
	      if (buffer_front != src)
		urgent_recv_buffers_queue.push(buffer_front);
	    }
	}
      } else {
	urgent_recv_buffers_queue.pop();
      }
      
      urb.all_urgent_msgs_cnt++;
      fstat_received_urgent_messages_cnt++;
      fstat_received_urgent_messages_size += msg_size + 2 * size_of_int;
    }
  
  src = msg_src;
  size = msg_size;
  tag = msg_tag;
  
  return true;
}

bool network_t::receive_urgent_message(char *buf, int& size, int& src, int& tag)
{
  return (receive_urgent_message_ex(buf, size, src, tag, false, false, false));
}

bool network_t::receive_urgent_message_non_exc(char *&buf, int& size, int& src, int& tag)
{
  return (receive_urgent_message_ex(buf, size, src, tag, true, false, false));
}

bool network_t::receive_urgent_message_from_source(char *buf, int& size, int src, int& tag)
{
  return (receive_urgent_message_ex(buf, size, src, tag, false, true, false));
}






















