#ifndef _DIVINE_NETWORK_HH_
#define _DIVINE_NETWORK_HH_
/*!\file network.hh
 * Network support unit header*/

#ifndef DOXYGEN_PROCESSING

#undef SEEK_SET
#undef SEEK_END
#undef SEEK_CUR

#include <vector>
#include <queue>
#include <memory>
#include <cmath>
#include <mpi.h>
#include "common/error.hh"
#include "common/sysinfo.hh"
#include "system/state.hh"
#include "distributed/message.hh"

using namespace std;

namespace divine { // We don't want Doxygen to see namespace 'dve'
#endif // DOXYGEN_PROCESSING
  //! MPI tag for normal messages <br>
  //! This tag is used internally by network_t and has nothing to do with tags passed <br>
  //! as parameters to network_t methods.  Programmers using network_t do not have direct <br>
  //! access to MPI, hence this constant is irrelevant to them.
  const int NET_TAG_NORMAL = 0;
  //! MPI tag for urgent messages <br>
  //! This tag is used internally by network_t and has nothing to do with tags passed <br>
  //! as parameters to network_t methods. Programmers using network_t do not have direct <br>
  //! access to MPI, hence this constant is irrelevant to them.
  const int NET_TAG_URGENT = 1;

  //! Identifier of exceptions raised by network_t
  const int NETWORK_ERR_TYPE = 1729; 
  //! Everything's ok
  const int NET_NO_ERROR = 0; 
  //! Trying to initialize, when network is already initialized
  const int NET_ERR_ALREADY_INITIALIZED = 1; 
  //! Trying to do something that requires initialized network, when network is not initialized
  const int NET_ERR_NOT_INITIALIZED = 2; 
  //! Initialization of network failed
  const int NET_ERR_INITIALIZATION_FAILED = 3;
  //! Finalization of network failed
  const int NET_ERR_FINALIZATION_FAILED = 4;
  //! Trying to send message whose size exceeds the buffer size
  const int NET_ERR_INVALID_MSG_SIZE = 5;
  //! Trying to send message to or aquire information about a non-existing destination
  const int NET_ERR_INVALID_DESTINATION = 6;
  //! Trying to get message from or aquire information about a non-existing source
  const int NET_ERR_INVALID_SOURCE = 7;
  //! Sending of message failed
  const int NET_ERR_SEND_MSG_FAILED = 8;
  //! Probe for messages failed
  const int NET_ERR_MSG_PROBE_FAILED = 9;
  //! Receiving of message failed
  const int NET_ERR_RECEIVE_MSG_FAILED = 10;
  //! Getting message size from network failed
  const int NET_ERR_GET_MSG_SIZE_FAILED = 11;
  //! Barrier function failed
  const int NET_ERR_BARRIER_FAILED = 12;
  //! Network abort function failed
  const int NET_ERR_ABORT_FAILED = 13;
  //! Function gather failed
  const int NET_ERR_GATHER_FAILED = 14; 
  //! Function allgather failed
  const int NET_ERR_ALLGATHER_FAILED = 15; 
  //! Trying aquire information about a non-existing workstation
  const int NET_ERR_INVALID_WORKSTATION_NUMBER = 16;
  //! Array of error descriptions
  const ERR_triplet_t net_err_msgs[17] = { 
    ERR_triplet_t("No error.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_NO_ERROR),
    ERR_triplet_t("Error: Network already initialized.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_ALREADY_INITIALIZED),
    ERR_triplet_t("Error: Network not initialized.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_NOT_INITIALIZED),
    ERR_triplet_t("Error: Network initialization failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_INITIALIZATION_FAILED),
    ERR_triplet_t("Error: Network finalization failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_FINALIZATION_FAILED),
    ERR_triplet_t("Error: Invalid message size.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_INVALID_MSG_SIZE),
    ERR_triplet_t("Error: Invalid destination.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_INVALID_DESTINATION),
    ERR_triplet_t("Error: Invalid source.", 
     		  NETWORK_ERR_TYPE, 
    		  NET_ERR_INVALID_SOURCE),
    ERR_triplet_t("Error: Send message failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_SEND_MSG_FAILED),
    ERR_triplet_t("Error: Message probe failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_MSG_PROBE_FAILED),
    ERR_triplet_t("Error: Receive message failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_RECEIVE_MSG_FAILED),
    ERR_triplet_t("Error: Get msg size failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_GET_MSG_SIZE_FAILED),
    ERR_triplet_t("Error: Barrier failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_BARRIER_FAILED),
    ERR_triplet_t("Error: Abort failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_ABORT_FAILED),
    ERR_triplet_t("Error: Gather failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_GATHER_FAILED),
    ERR_triplet_t("Error: AllGather failed.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_ALLGATHER_FAILED),
    ERR_triplet_t("Error: Invalid workstation number.", 
    		  NETWORK_ERR_TYPE, 
    		  NET_ERR_INVALID_WORKSTATION_NUMBER)
  };
  
  class comm_matrix_t;
 
  //!Communication matrix auto pointer
  /*!Points to communication matrix. Variables of this type are outputs of some
   * statistical methods of network_t. The only important thing you need to know about auto
   * pointers is that if auto pointer is destroyed (or rewritten), then the object it points
   * to is also destroyed (unless some other pointer points to the object).*/
  typedef auto_ptr<comm_matrix_t> pcomm_matrix_t;

  //!Communication matrix
  /*!Matrix returned by methods of network_t is usually of size \a cl_size * \a cl_size, where
   * \a cl_size if the number of computers in the cluster, which can be retrieved by
   * network_t::get_cluster_size() method. In i-th row
   * and j-th column of the matrix is an integer that stores information about data sent by 
   * workstation with id \a i to workstation with id \a j.*/
  class comm_matrix_t {
  private:
    int **elems;
    int rowcount, colcount;
  protected:
    error_vector_t& errvec;
  public:
    //! A contructor, creates matrix with \a rows rows and \a col columns
    /*! \param rows - number of rows of the matrix to be created
     *  \param col - number of columns of the matrix to be created
     *  \param arg0 - object used for error reporting
     *
     *  Use this constructor to create matrix with \a rows rows and \a col columns.
     *  If you don't want errors to be handled in a default way, pass your own
     *  error handling object of type error_vector_t.*/
    comm_matrix_t(int rows, int col, error_vector_t& arg0 = gerr);
    //! A destructor, frees space allocated for the matrix
    ~comm_matrix_t();
    //! Function that returns number of rows in the matrix
    /*! \return number of rows in the matrix
     *
     * Use this function to get the number of rows in the matrix.*/
    int getrowcount(void) { return rowcount; }
    //! Function that returns number of columns in the matrix
    /*! \return number of columns in the matrix
     *
     * Use this function to get the number of columns in the matrix.*/
    int getcolcount(void) { return colcount; }
    //! Operator that retrieves elements of the matrix
    /*! \param row - specifies the row of the element to be retrieved
     *  \param col - specifies the column of the element to be retrieved
     *
     *  \return value of the retrieved element
     *
     *  Use this operator to retrieve element at position (\a i, \a j) in the matrix by passing
     *  \a i as the first parameter and \a j as the second parameter.*/

    int& operator()(int row, int col);
    //! Operator that inverts all elements in the matrix ("puts minus" in front of every element)
    /*! \param m - pointer to matrix whose elements are to be inverted
     *  \return  pointer to matrix with inverted elements
     *
     *  Use this operator to get matrix with all elements inverted.*/
    friend pcomm_matrix_t operator-(const pcomm_matrix_t& m);
    //! Operator that subtracts matrix \a m2 from matrix \a m1 (element-wise)
    /*! \param m1 - pointer to matrix, minuend
     *  \param m2 - pointer to matrix, subtrahend
     *	\return pointer to difference matrix between \a m1 and \a m2
     *  
     *  Use this operator to subtract two matrices.*/
    friend pcomm_matrix_t operator-(const pcomm_matrix_t& m1, const pcomm_matrix_t& m2);
    //! Operator that adds matrix \a m1 to matrix \a m2
    /*! \param m1 - pointer to matrix, first addend
     *  \param m2 - pointer to matrix, second addend
     *  \return pointer to sum matrix of \a m1 and \a m2
     *
     *  Use this operator to add two matrices.*/
    friend pcomm_matrix_t operator+(const pcomm_matrix_t& m1, const pcomm_matrix_t& m2);
    //! Operator that multiplies matrix \a m by integer \a a
    /*! \param a - integer that is to be multiplied with all elements of the matrix passed as
     *             second parameter
     *  \param m - pointer to matrix whose elements are to be multiplied by the integer passed
     *             as first parameter
     *  \return pointer to matrix whose elements are elements of \a m multiplied by \a a
     *
     *  Use this operator to multiply all elements of matrix by an integer.*/
    friend pcomm_matrix_t operator*(int a, const pcomm_matrix_t& m);
    //! Operator that multiplies matrix \a m by integer \a a, analogous to operator*()
    friend pcomm_matrix_t operator*(const pcomm_matrix_t& m, int a);
  };
  
  //! Network communication support class
  /*! This class mainly provides methods for buffered transmission of messages. Additionally
   *  barrier and gather methods are included. Methods for detailed statistics are a matter of
   *  course.*/
  class network_t {
public:

/* Config options OPT_STATS and OPT_STATS_MPI_RATE are needed
 * for full functionality of some of the new optimizations:
 */
#define OPT_STATS
#define OPT_STATS_MPI_RATE

/* The remaining config options are not required (should probably be disabled
 * in the distribution by default) but are very useful for perf. analysis,
 * to see what is happening when on what nodes, etc.
 */
#if 0
#define OPT_STATS_MPI_RATE_PRINT
#define OPT_STATS_MPI_PENDING
#define OPT_STATS_TAGS
#define OPT_GRID_CONFIG
#endif
#if 0 /* enable for call-level MPI statistics */
#define OPT_STATS_MPI_CALLS
#endif

#if defined(OPT_STATS_TAGS)
#define TAGS_MAX  256 /* two power because of next mask */
#define TAGS_MASK (TAGS_MAX - 1)
    int tag_count[TAGS_MAX];
#endif /* OPT_STATS_TAGS */

#if defined(OPT_GRID_CONFIG)
    /* For printing per-cluster stats in a grid environment: */
    char *grid_config;
#endif

#if defined(OPT_STATS)
    struct statistics {
      unsigned long long num;
      double sum;
      double sum_sq;
      double min;
      double max;
    };

    void stats_init(statistics *stats) {
      stats->num = 0;
      stats->sum = 0.0;
      stats->sum_sq = 0.0;
      stats->min = 9e99;
      stats->max = 0.0;
    }

    void stats_update(statistics *stats, double val) {
      stats->num++;
      stats->sum += val;
      stats->sum_sq += val * val;
      if (val > stats->max)
        {
          stats->max = val;
        }
      if (val < stats->min)
        {
          stats->min = val;
        }
    }

    unsigned stats_num(statistics *stats) {
      return stats->num;
    }

    double stats_min(statistics *stats) {
      if (stats->num > 0)
        {
          return stats->min;
        }

      return 0.0;
    }

    double stats_max(statistics *stats) {
      return stats->max;
    }

    double stats_mean(statistics *stats) {
      if (stats->num > 0)
        {
          return stats->sum / stats->num;
        }

      return 0.0;
    }

    double stats_var(statistics *stats) {
      if (stats->num > 1)
        {
          double var;

          var = (stats->sum_sq - (stats->sum * stats->sum) / stats->num) / 
                (stats->num - 1);
          /* for VAR very close to 0, due to rounding, the difference may
           * in fact become slightly negative, so:
           */
          if (var >= 0.0)
            {
              return var;
            }
        }

      return 0.0;
    }

    double stats_sqrt_var(statistics *stats) {
      double var = stats_var(stats);

      if (var > 0.0)
        {
          return sqrt(var);
        }

      return 0.0;
    }

#endif /* OPT_STATS */
    
#if defined(OPT_STATS_MPI_RATE)
#define STATS_INTERVAL 3.0
    double stats_time_begin, stats_slice_begin, stats_slice_end;

    statistics stats_Sent_bytes_local;
    statistics stats_Recv_bytes;
#if defined(OPT_GRID_CONFIG)
    statistics stats_Sent_bytes_remote;
#endif
#endif

#if defined(OPT_STATS_MPI_RATE_PRINT)
    vminfo_t vm; /* to print vm increase at runtime */
#endif

#if defined(OPT_STATS_MPI_CALLS)
    statistics stats_Issend, stats_Issend_urg, stats_Test,
	   stats_Isend, stats_Isend_urg,
           stats_Probe, stats_Iprobe0, stats_Iprobe1,
	   stats_Recv, stats_Barrier, stats_Gather, stats_Allgather;
#endif

  private:
    // variables with prefix "all" in the following structures are used for statistics,
    // they store number of all messages sent to each user and number of all messages
    // received from each user
    // although net_send_buffer is not used to send urgent messages it is good place to store
    // the count of them
    struct net_send_buffer_t { char* ptr; int size; int msgs_cnt; timeinfo_t flush_timeout; 
      bool pending; MPI_Request req; net_send_buffer_t *next; };
    struct net_send_buffers_t { net_send_buffer_t *first; net_send_buffer_t *current;
      net_send_buffer_t *last; int total_allocated_size; int total_pending_size;
      int all_msgs_cnt; int all_urgent_msgs_cnt; int flushes_cnt; };
    struct net_recv_buffer_t { char* ptr; int position; int size; int all_msgs_cnt; };
    struct net_urgent_recv_buffer_t { char* ptr; int size; int all_urgent_msgs_cnt; };
    
    enum net_comm_matrix_type_t { ncmt_snm, ncmt_rnm, ncmt_sum, ncmt_rum };
    
    bool fnetwork_initialized; // is MPI (network) initialized
    bool finitialized; // is everything initialized?
    int flast_mpi_rc;    // last MPI return code
    int flast_net_rc;    // last non-MPI return code
    bool ftimed_flushing_enabled;

    vector<net_send_buffers_t> *send_buffer; // send buffers, one for every workstation
    vector<net_recv_buffer_t> *recv_buffer; // receive buffers, one for every workstation
    vector<net_urgent_recv_buffer_t> *urgent_recv_buffer; // urgent receive buffers, one frevrwrksn
    char *tmp_rbuf; // auxiliary receive buffer
    char *tmp_sbuf; // auxiliary send buffer
    int* tmp_buf_cl_size; // auxuliary buffer of size sizeof(int)*fcluster_size

    // queue of receive buffers (used to keep the order of received non-urgent data)
    queue<int> recv_buffers_queue; 
    // queue of urgent receive buffers (used to keep the order of received urgent data)
    queue<int> urgent_recv_buffers_queue;
    
    // buf params, initialized at class creation, can be changed before initialize_buffers() only
    int fbuf_msgs_cnt_limit;
    int fbuf_size_limit;
    int fbuf_time_limit_sec;
    int fbuf_time_limit_msec;

    // statistics
    int fstat_sent_messages_cnt; // only non-urgent
    int fstat_sent_messages_size; // only non-urgent
    int fstat_sent_urgent_messages_cnt; // only urgent
    int fstat_sent_urgent_messages_size; // only urgent

    int fstat_received_messages_cnt; // only non-urgent
    int fstat_received_messages_size; // only non-urgent
    int fstat_received_urgent_messages_cnt; // only urgent
    int fstat_received_urgent_messages_size; // only urgent

    int fstat_all_barriers_cnt; // count of all barriers

    int fid;   // computer unique identifier
    int fcluster_size; // number of computers in the cluster
    char fprocessor_name[MPI_MAX_PROCESSOR_NAME + 1]; // computer name

    int size_of_int; // holds the value sizeof(int)
 
    bool get_comm_matrix(net_comm_matrix_type_t cm_type, pcomm_matrix_t& ret, int target);
    bool send_message_ex(char* buf, int size, int dest, int tag, bool urgent);
    bool message_probe(int src, int tag, bool &flag, int &size, 
		       MPI_Status& status, bool blocking);
    bool is_new_message_ex(int& size, int& src, int& tag, bool& flag, 
			   bool urgent, bool from_source, bool blocking);
    bool receive_message_from_network(char *buf, int size, int src, int tag, 
				      MPI_Status& status);
    bool receive_message_ex(char *&buf, int &size, int& src, int& tag, 
			    bool non_exc, bool from_source, bool called_internally);
    bool receive_urgent_message_ex(char *&buf, int &size, int& src, int& tag, 
				   bool non_exc, bool from_source, bool called_internally);
  public:
    //! A constructor, does't initialize network!, <br>
    //! non-default error handling vector can be specified
    network_t(error_vector_t& arg0 = gerr);
    //! A destructor, frees allocated memory
    ~network_t(); 
  
    //! Initializes network, essentially establishes connections and gives all workstations <br>
    //! the command line parameters
    /*! \param argc - number of command line parameters, passing the first parameter of main()
     *                is recommended
     *  \param argv - command line parameters, passing the second parameter of main() is
     *                recommended  
     *  \return \b true if initialization was successful, \b false otherwise
     *  
     *  Use this function to initialize network. After successful initialization you can obtain
     *  workstation identification by calling get_id() function, number of computers in cluster
     *  by calling get_cluster_size() function and workstation name using get_processor_name()
     *  function. You can also set some properties of build-in buffering scheme using 
     *  set-functions.*/
    bool initialize_network(int &argc, char **&argv);
    //! Completes initialization, must be called after and only after initialize_network()
    /*! \return \b true if initialization was successful, \b false otherwise
     *
     *  Use this function to complete initialization process. It initializes buffers, thus
     *  enabling the send/receive functions, which use buffering.*/
    bool initialize_buffers(); 
    //! Finalizes network, frees all allocated buffers
    bool finalize();
    //! Finalizes network, essentially closes connections and frees buffers.
    /*! \return \b true if initialization was successful, \b false otherwise
     *
     *  Use this function ti finalize network, this works only if the network was previously
     *  initialized, otherwise error occurs. The current implementation does not allow to
     *  initialize the network more than once, so subsequent calls to initialize_network()
     *  will cause error.*/

    //! Stops the computation until all workstation call barrier()
    /*! \return \b true if barrier was successful, \b false otherwise
     *
     *  Use this function to "synchronize" all workstation. The function blocks until
     *  all workstations call it.*/
    bool barrier(void);
    //! Similar to barrier() but allows to collect some data on selected workstation
    /*! \param sbuf - pointer to buffer that contains the data to be collected
     *  \param ssize - size of the data sbuf points to (in sizeof(char))
     *  \param rbuf - pointer to buffer that receives the collected data (relevant only on
     *                selected workstation)
     *  \param rsize - size of the collected data corresponding to one workstation, therefore
     *                 \a rsize should be equal to ssize (relevant only on selected workstation)
     *  \param root - id of the selected workstation that receives the collected data
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to "synchronize" workstations (in the sense explained for barrier() 
     *  function) and collect some information at the same time. Each workstation
     *  must call gather with the same \a ssize, all workstations except for the selected one
     *  can pass \b NULL and \b 0 as rbuf and rsize parameters, respectively. Root parameter
     *  must be the same on all workstations. The selected workstation should pass the same
     *  value as \a ssize as \a rsize. The buffer to which \a rbuf points must be of size
     *  \a rsize * [number of computers in the cluster]. 
     *
     *  The collected data are in order imposed by workstation ids.*/
    bool gather(char *sbuf, int ssize, char *rbuf, int rsize, int root);
    //! Similar to gather(), but the collected information are received on every workstation.
    /*! Differs from gather() in that all workstations must have \a rbuf and \a rsize valid,
     *  not only one workstation.*/
    bool all_gather(char *sbuf, int ssize, char *rbuf, int rsize);
    //! Abort computation
    /*! \return \b true if the function succeeds, \b false otherwise
     *  Use this function to terminate all processes participating in the distributed computation*/
    bool abort(void);
    //! Blocking message send
    /*! \param buf - pointer to data that are to be sent
     *  \param size - size of the data to be sent (in sizeof(char))
     *  \param dest - id of the workstation that receives the data
     *  \param tag - additional information attached to the message, which is typically used to
     *               identify the type of the message
     *  \return \b true if the function succeeds, \b false otherwise
     *   
     *  Use this function to send data to a workstation, the sending process is blocking in the
     *  sense that as soon as the function finishes the data which \a buf points to has already
     *  been copied to "lower level" and the original memory can be safely rewritten.
     *
     *  The function is connected to a buffering mechnism, it means that the messages are not
     *  sent instantly, but they are accumulated in a special buffer. There is one such buffer
     *  for each workstation. The data from a buffer are sent to the destination workstation
     *  as soon as one of the limits (set by functions set_buf_msgs_cnt_limit(),
     *  set_buf_size_limit(), set_buf_time_limit())
     *  is exceeded or flush_buffer() function is called.*/
    bool send_message(char* buf, int size, int dest, int tag);

    //! Blocking message send
    /*! \param message - a message to send
     *  \param dest - id of the workstation that receives the data
     *  \param tag - additional information attached to the message, which is typically used to
     *               identify the type of the message
     *  \return \b true if the function succeeds, \b false otherwise
     *   
     *  Use this function to send data to a workstation, the sending process is blocking in the
     *  sense that as soon as the function finishes the data which \a buf points to has already
     *  been copied to "lower level" and the original memory can be safely rewritten.
     *
     *  The function is connected to a buffering mechnism, it means that the messages are not
     *  sent instantly, but they are accumulated in a special buffer. There is one such buffer
     *  for each workstation. The data from a buffer are sent to the destination workstation
     *  as soon as one of the limits (set by functions set_buf_msgs_cnt_limit(),
     *  set_buf_size_limit(), set_buf_time_limit())
     *  is exceeded or flush_buffer() function is called.*/
    bool send_message(const message_t & message, int dest, int tag);

    //! Blocking urgent message send
    /*! Similar to send_message() function but of course there are differences. The function
     *  is not connected to buffering mechanism, so the messages are sent immediately.
     *  Different functions are used to receive messages sent by this function.*/
    bool send_urgent_message(char* buf, int size, int dest, int tag);
    
    //! Blocking urgent message send
    /*! Similar to send_message() function but of course there are differences. The function
     *  is not connected to buffering mechanism, so the messages are sent immediately.
     *  Different functions are used to receive messages sent by this function.*/
    bool send_urgent_message(const message_t & message, int dest, int tag);
    //! One send buffer flush
    /*! \param dest - identifcation of the buffer (id of the workstation the buffer is
     *                designated for) to be flushed
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to send data in the buffer determined by the \a dest parameter.*/
    bool flush_buffer(int dest);
    
    //! All buffers flush
    /*! \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to flush all buffers (Technically, the function calls flush_buffer()
     *  function for all workstations in the cluster.*/
    bool flush_all_buffers();
#if !defined(ORIG_FLUSH)
    //! Some buffers flush
    /*! \return \b true if the function succeeds, \b false otherwise
     *
     * Use this function to flush "some" of the largest buffers (currently, the function
     * only calls flush_buffer() for the single workstation with the largest amount of
     * work buffered, since this already appears to give good performance in practice). */
    bool flush_some_buffers();
#endif
    //! Only timed out buffers flush
    /*! \return \b true if the fuction succeeds, \b false otherwise
     *
     *  Use this fuction to flush all buffers that have the time limit exceeded (the time
     *  limit can bee adjusted by the set_buf_time_limit() function).*/
    bool flush_all_buffers_timed_out_only();
    //! Non-blocking message probe
    /*! \param size - output parameter that receives the size of the message, if there is one
     *  \param src - output parameter that receives the source workstation of the message,
     *               if there is one
     *  \param tag - output parameter that receives the tag of the message, if there is one
     *  \param flag - output parameter that determines if there is a message waiting to be
     *                received
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to find out whether there is some unreceived message waiting.
     *  The function is non-blocking in the sense that it does not wait for messages
     *  to arrive, it only finds out whether there are some.
     *  
     *  This function only looks for messages sent by send_message() function.*/
    bool is_new_message(int& size, int& src, int& tag, bool& flag);
    //! Non-blocking message from specific source probe
    /*! Similar to is_new_message(), but the \a src parameter is input. It means that the
     *  function only looks for messages from speceified source workstation.*/
    bool is_new_message_from_source(int& size, int src, int& tag, bool& flag);
    //! Non-blocking urgent message probe
    /*! Similar to is_new_message(), but it looks only for messages sent by send_urgent_message()
     *  function.*/
    bool is_new_urgent_message(int& size, int& src, int& tag, bool& flag);
    //! Non-blocking urgent message probe from specific source
    /*! Similar to is_new_message(), but it looks only for messages sent by send_urgent_message()
     *  function and only for those of them that are from source workstation specified by the input
     *  parameter \a src.*/
    bool is_new_urgent_message_from_source(int& size, int src, int& tag, bool& flag);
    //! Blocking message receive
    /*! \param buf - pointer to memory used to store the received data
     *  \param size - output parameter that receives the size of the received message
     *  \param src - output parameter that gets id of the source workstation of the received
     *               message
     *  \param tag - output parameter that gets the tag of the received message
     *  \return \b true if the function suceeds, \b false otherwise
     *
     *  Use this function to receive a message from network. The receiving process is blocking
     *  in the sense that the function does not terminate until the message is completely
     *  received. Apart from the other things,
     *  It means that if there is no message to be received, the function will block
     *  the computation until one arrives.
     *
     *  This function only concerns messages sent by the send_message() function.*/
    bool receive_message(char *buf, int& size, int& src, int& tag);
    
    //! Blocking message receive
    /*! \param message - instance of message_t, where the data will be stored
     *  \param src - output parameter that gets id of the source workstation of the received
     *               message
     *  \param tag - output parameter that gets the tag of the received message
     *  \return \b true if the function suceeds, \b false otherwise
     *
     *  Use this function to receive a message from network. The receiving process is blocking
     *  in the sense that the function does not terminate until the message is completely
     *  received. Apart from the other things,
     *  It means that if there is no message to be received, the function will block
     *  the computation until one arrives.
     *
     *  This function only concerns messages sent by the send_message() function.*/
    bool receive_message(message_t & message, int& src, int& tag);
    //! Blocking message receive, which sets buf to point to internal buffer
    /*! Similar to receive_message(), but the \a buf parameter is output and need not be 
     *  initialized, because the function
     *  sets it to point to internal buffer containing the received data, which saves
     *  both memory and time. It is obvious that the memory \a buf points to must neither be
     *  freed nor rewritten.*/
    bool receive_message_non_exc(char *&buf, int& size, int& src, int& tag);
    //! Blocking message from source receive
    /*! Similar to receive_message(), but it receives only messages sent by send_urgent_message
     *  function.*/
    bool receive_message_from_source(char *buf, int& size, int src, int& tag);
    //! Blocking message receive
    /*! Similar to receive_message(), but it receives only messages sent by send_urgent_message
     *  function.*/
    bool receive_urgent_message(char *buf, int& size, int& src, int& tag);
    //! Blocking urgent message receive, which sets buf to point to internal buffer
    /*! Similar to receive_message_non_exc(), but it receives only messages sent by 
     *  send_urgent_message()
     *  function.*/
    bool receive_urgent_message_non_exc(char *&buf, int& size, int& src, int& tag);
    //! Blocking urgent message from source receive
    /*! Similar to receive_message_from source(), but it receives only messages sent by 
     *  send_urgent_message()
     *  function.*/
    bool receive_urgent_message_from_source(char *buf, int& size, int src, int& tag);

    // get communication matrix of sent non-urgent messages, requires synchronized call on all
    // the computers in the cluster (snm means "sent normal messages"). ret is valid
    // on computer with fid = target. Similarly for the next three functions.
    //! Get communication matrix of sent normal messages
    /*! \param ret - output parameter, pointer to communication matrix (of type comm_matrix_t)
     *               that contains at position (\a i, \a j) the count of normal messages (sent
     *               by function send_message()) sent by workstation with id \a i to
     *               workstation with id \a j
     *  \param target - only one workstation gets valid pointer ret, this workstation is
     *                  specified by \a target parameter, which must be the same on all
     *                  workstations
     *  \return \b true if the function suceeds, \b false otherwise
     *
     *  Use this function to get the counts of messages sent by each workstation to
     *  each workstation. The function must be called in a way explained for the barrier()
     *  function.*/
    bool get_comm_matrix_snm(pcomm_matrix_t& ret, int target);
    // rnm stands for "received normal messages"
    //! Get communication matrix of received normal messages
    /*! Similar to get_comm_matrix_snm(), but the position (\a i, \a j) of the matrix contains
     *  the count of normal messages (sent by send_message() function) received on workstation 
     *  with id \a i from workstation 
     *  with id \a j.*/
    bool get_comm_matrix_rnm(pcomm_matrix_t& ret, int target);
    // sum stands for "sent urgent messages"
    //! Get communication matrix of sent urgent messages
    /*! Similar to get_comm_matrix_snm(), but the position (\a i, \a j) of the matrix contains
     *  the count of urgent messages (sent by send_urgent_message() function) sent by 
     *  workstation with id \a i to workstation with id \a j.*/
    bool get_comm_matrix_sum(pcomm_matrix_t& ret, int target);
    // rum stands for "received urgent messages"
    //! Get communication matrix of received urgent messages
    /*! Similar to get_comm_matrix_snm(), but the position (\a i, \a j) of the matrix contains
     *  the count of urgent messages (sent by send_urgent_message() function) received on 
     *  workstation with id \a i from workstation with id
     *  \a j.*/
    bool get_comm_matrix_rum(pcomm_matrix_t& ret, int target);

    //! Get send buffer's message count limit
    /*! \param limit - the maximal count of messages send buffer can hold
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to get the send buffer's message count limit. If the number of messages
     *  in any send buffer (on each workstation there is one send buffer for every workstation) 
     *  is equal to this limit, the whole contents of the buffer is sent to the workstation
     *  the buffer is designated for.
     *
     *  \b 100 messages is default.
     *
     *  This limit can be set by set_buf_msgs_cnt_limit() function, but only before 
     *  initialize_buffers() function is called.*/
    bool get_buf_msgs_cnt_limit(int& limit);
    //! Get send buffer's size limit
    /*! \param limit - the maximal size of the send buffer
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to get the send buffer's size limit. If the size of
     *  any send buffer (on each workstation there is one send buffer for every workstation) 
     *  is about to exceed or equal this limit, 
     *  the whole contents of the buffer is sent to the workstation
     *  the buffer is designated for.
     *
     *  \b 8192 chars is default.
     *  
     *  This limit can be set by set_buf_size_limit() function, but only before
     *  initialize_buffers function is called.*/
    bool get_buf_size_limit(int& limit);
    //! Get send buffer's time limit
    /*! \param limit_sec - together with \a limit_msec the "maximal" amount of time send buffer 
     *                     can hold data without flush (in seconds)
     *  \param limit_msec - together with \a limit_sec the "maximal" amount of time send buffer 
     *                      can hold data without flush (in miliseconds)
     *  \return \b true if the function succeeds, \b false otherwise
     *
     *  Use this function to get the send buffer's time limit. If the time of
     *  any send buffer (on each workstation there is one send buffer for every workstation) 
     *  is about to exceed or equal this limit, 
     *  the whole contents of the buffer is sent to the workstation
     *  the buffer is designated for. The previous sentence is not completely true, because if
     *  nobody calls some function that checks the limit, the data can stay in the buffer
     *  for more that the limit. There is no "timer" that check the limit.
     *
     *  \b 300 miliseconds is default.
     * 
     *  This limit can be set by set_buf_time_limit() function, but only before 
     *  initialize buffers function is called. 
     *
     *  \b Setting the limit to 0 seconds and 0 miliseconds turns timed flushing off.*/
    bool get_buf_time_limit(int& limit_sec, int& limit_msec);

    //! Set send buffer's message count limit, see get_buf_msgs_cnt_limit() doc for more info
    bool set_buf_msgs_cnt_limit(int limit);
    //! Set send buffer's size limit, see get_buf_size_limit() doc for more info
    bool set_buf_size_limit(int limit);
    //! Set send buffer's time limit, see get_buf_time_limit() doc for more info
    bool set_buf_time_limit(int limit_sec, int limit_msec);

    //! Get all sent messages count (including urgent messages)
    /*! \param cnt - output parameter that receives the count of all messages sent by the
     *               calling workstation
     *  \return \b true if the function succeeds, \b false otherwise.*/
    bool get_all_sent_msgs_cnt(int& cnt);   
    //! Get all received messages count (including urgent messages)
    /*! \param cnt - output parameter that receives the count of all messages received on the
     *               calling workstation
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_all_received_msgs_cnt(int& cnt);

   
    //! Get number of messages sent to a given workstation
    /*! \param to - specifies the workstation we are interested in
     *  \param cnt - output parameter that receives the count of all messages sent by calling
     *               workstation to a workstation specified by \a to parameter.
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_sent_msgs_cnt_sent_to(int to,int& cnt);
    //! Get number of messages sent to a given workstation
    /*! \param to - specifies the workstation we are interested in
     *
     *  \return the count of all messages sent by calling workstation to a workstation 
     *          specified by \a to parameter*/
    int  get_sent_msgs_cnt_sent_to(int to);

    //! Get number of messages received from given workstation
    /*! \param from - specifies the workstation we are interested in
     *  \param cnt - output parameter that receives the count of all message received on the
     *               calling workstation from the workstation specified by \a from parameter.
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_recv_msgs_cnt_recv_from(int from,int& cnt);
    //! Get number of messages received from given workstation
    /*! \param from - specifies the workstation we are interested in
     *  \return the count of all message received on the calling workstation from 
     *          the workstation specified by \a from parameter*/
    int  get_recv_msgs_cnt_recv_from(int from);


    //! Get normal sent messages count (only normal messages, without urgent messages)
    /*! \param cnt - output parameter that receives the count of all normal messages sent by the
     *               calling workstation
     *  \return \b true if the function succeeds, \b false otherwise.*/
    bool get_user_sent_msgs_cnt(int& cnt);   
    //! Get normal received messages count (only normal messages, without urgent messages)
    /*! \param cnt - output parameter that receives the count of all normal messages received 
     *               on the calling workstation
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_user_received_msgs_cnt(int& cnt);

    //! Get count of all barrier() calls
    /*! \param cnt - output parameter that receives the count of all barrier() function calls
     *               on the calling workstation
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_all_barriers_cnt(int& cnt);

    //! Get number of flushes of send buffer for workstation \a dest
    /*! \param dest - identification of the send buffer
     *  \param cnt - output parameter that receives the count of all times the send buffer
     *               designated for workstation with id \a dest has been flushed
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_buffer_flushes_cnt(int dest, int& cnt);
    //! Get number of flushes of all send buffers
    /*! \param cnt - output parameter that receives the count of all send buffer flushes on the
     *               calling workstation
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_all_buffers_flushes_cnt(int& cnt);
    //! Get size of pending (blocked) send buffer for workstation \a dest
    /*! \param dest - identification of the send buffer
     *  \param size - output parameter that receives total size of memory occupied by pending 
     *                send buffer designated for workstation with id \a dest
     *  \param test - if test = true, tests are performed and exact value is retrieved,
     *                if test = false, upper estimate is returned
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_total_pending_size(int dest, int& size, bool test);
#if defined(OPT_STATS_MPI_PENDING)
    //! Get stats for pending (blocked) send buffer for workstation \a dest
    bool get_total_pending_stats(int dest, int& size, int& curpkts, int& maxpkts, bool test);
#endif
    //! Get total size of pending (blocked) send buffers for all workstations
    /*! \param size - output parameter that receives total size of memory occupied by all pending
     *                send buffers
     *  \param test - if test = true, tests are performed and exact value is retrieved,
     *                if test = false, upper estimate is returned
     *  \return \b true if the function succeeds, \b false otherwise*/
    bool get_all_total_pending_size(int& size, bool test);
#if defined(OPT_STATS_MPI_PENDING)
    //! Get total stats for pending (blocked) send buffers for all workstations
    bool get_all_total_pending_stats(int& size, int& curpkts, int& maxpkts, bool test);
#endif
    //! Retrieves the identifier of the calling workstation
    /*! \param id - output parameter that receives the workstation id
     *  \return \b true if the function succeeds, \b false otherwise
     *  
     *  Use this function to get the unique identifier of the calling workstation.
     *  The identifier is in the range [0..\a cluster_size - 1], where \a cluster_size
     *  is the number of computers in the cluster and
     *  can be obtained by calling the get_cluster_size() function.
     *  
     *  You must call initialize_network() function before this function.*/
    bool get_id(int &id);
    //! Retrieves the number of computers in the cluster
    /*! \param size - output parameter that receives the retrieved value
     *  \return \b true if the function succeeds, \b false otherwise
     *  
     *  Use this function to get the number of computers in the cluster. The variable
     *  that receives the actual size of the cluster is passed as the first parameter.
     *
     *  You must call initialize_network() function before this function.*/
    bool get_cluster_size(int &size);
    //! Retrieves the name of the calling workstation
    /*! \param proc_name - output parameter that receives the workstation name
     *  \param length - output parameter that receives the number of characters in the name
     *  \return \b true if the function succeeds, \b false otherwise
     *  
     *  Use this function to get the name of the calling workstation.
     *
     *  You must call initialize_network() function before this function.*/
    bool get_processor_name(char *proc_name, int& length);
    
  protected:
    error_vector_t& errvec;
    
  };
  
#ifndef DOXYGEN_PROCESSING  
}
#endif // DOXYGEN_PROCESSING 

#endif


























