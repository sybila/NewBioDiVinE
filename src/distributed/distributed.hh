#ifndef _DIVINE_DISTRIBUTED_HH_
#define _DIVINE_DISTRIBUTED_HH_
/*!\file distributed.hh
 * Distributed computing support unit header*/

#ifndef DOXYGEN_PROCESSING
#include <string>
#include "common/error.hh"
#include "system/state.hh"
#include "distributed/network.hh"
 
#define DISTRIBUTED_ERR_TYPE 12

using namespace std;

namespace divine {
#endif // DOXYGEN_PROCESSING


  //! Constant used internaly, of no relevance to user of distributed_t
  const int DIVINE_TAG_SYNC_READY = 0;
  //! Constant used internaly, of no relevance to user of distributed_t
  const int DIVINE_TAG_SYNC_ONE = 1;
  //! Constant used internaly, of no relevance to user of distributed_t
  const int DIVINE_TAG_SYNC_TWO = 2;
  //! Constant used internaly, of no relevance to user of distributed_t
  const int DIVINE_TAG_SYNC_COMPLETION = 3;
  //! All messages sent by user must have tags greater or equal to this constant.
  const int DIVINE_TAG_USER = 144;

  //! Id of the workstation that initiated the distributed computation
  const int NETWORK_ID_MANAGER = 0;

  //! Default hash seed for partition function.
  const int NETWORK_HASH_SEED = 0xbafbaf99;

  class abstract_info_t {
  public:
    virtual void update(void) {}
    virtual void get_data_ptr(char *&ptr) = 0;
    virtual void get_data_size(int &size) = 0;
    virtual ~abstract_info_t() {}
  };


  struct no_const_data_type_t {
    int x;
  };
  
  //! Class used to collect data during synchronization
  /*! Instances of this class are passed to distributed_t::synchronized(abstract_info_t &info)
   *  function.
   *
   *  The \a updateable_data_t type should be struct with simple types in it (no dynamic
   *  arrays, strings, etc.) - the struct holds the collected data.
   *  It must have a "void update(void)" function,
   *  which will be called on every workstation.
   *
   *  The word "updateable" means that each workstation can change the data being collected
   *  during the synchronization process (by calling the update() function).
   *  For more information see distributed_t::synchronized(abstract_info_t &info) function.*/
  template <class updateable_data_t, class constant_data_t = no_const_data_type_t>
  class updateable_info_t : public abstract_info_t {
  public:
    //! Structure that stores local constant part of info (usually needed
    //! for computation of the collected part)
    constant_data_t const_data;
    //! Structure that stores collected data
    updateable_data_t data;
    virtual void update(void);
    virtual void get_data_ptr(char *&ptr);
    virtual void get_data_size(int &size);
    virtual ~updateable_info_t() {}
  };

  //! Class used to collect data during synchronization
  /*! Instances of this class are passed to distributed_t::synchronized(abstract_info_t &info)
   *  function.
   *
   *  The \a updateable_data_t type should be struct with simple types in it (no dynamic
   *  arrays, strings, etc.) - the struct holds the collected data.
   *  It must have a "void update(void)" function,
   *  which will be called on every workstation.
   *
   *  The word "updateable" means that each workstation can change the data being collected
   *  during the synchronization process (by calling the update() function).
   *  For more information see distributed_t::synchronized(abstract_info_t &info) function.*/
  template <class updateable_data_t>
  class updateable_info_t<updateable_data_t, no_const_data_type_t>: public abstract_info_t {
  public:
    //! Structure that stores collected data
    updateable_data_t data;
    virtual void update(void);
    virtual void get_data_ptr(char *&ptr);
    virtual void get_data_size(int &size);
    virtual ~updateable_info_t() {}
  };

  //! Class used to collect data during synchronization
  /*! Instances of this class are passed to distributed_t::synchronized(abstract_info_t &info)
   *  function.
   *
   *  The \a static_data_t type should be struct with simple types in it (no dynamic
   *  arrays, strings, etc.) - the struct holds the collected data.
   *
   *  The word "static" means that each workstation cannot change the data being collected
   *  during the synchronization process.
   *  For more information see distributed_t::synchronized(abstract_info_t &info) function.*/
  template <class static_data_t>
  class static_info_t : public abstract_info_t {
  private:
    static_data_t data;
  public:
    virtual void get_data_ptr(char *&ptr); 
    virtual void get_data_size(int &size);
    virtual ~static_info_t() {}
  };

  struct distr_stat_t { 
    int all_sent_sync_msgs_cnt;
    int all_received_sync_msgs_cnt;
  };
  
  class message_processor_t
  {
   public:
    virtual void process_message(char *buf, int size, int src, int tag, bool urgent) = 0;
    virtual ~message_processor_t() {}
  };


  //! Main distributed support class.
  /*! The class provides support for common operations necessary for distributed computing
   *  such as partition function and termination detection (synchronization).*/
  class distributed_t {
  private:
    message_processor_t * process_message_functor;

    bool fbusy;
    int fsync_state;
    bool fsynchronized;
    bool fproc_msgs_buf_exclusive_mem;

    bool finfo_exchange;
    char *ptr_info_data;
    int info_data_size; 
    abstract_info_t *pinfo;

    int fstat_all_sync_barriers_cnt;
    vector<distr_stat_t> *fstatistics;

    int* tmp_buf_cl_size;
    char* sync_sr_buf;

    enum distr_comm_matrix_type_t { dcmt_ssm, dcmt_rsm };
    void get_comm_matrix(distr_comm_matrix_type_t cm_type, pcomm_matrix_t& ret, int target);
  protected:
    divine::error_vector_t& errvec;

    enum distr_mode_t {PRE_INIT,PRE_INIT_NET_OK,NORMAL};
    distr_mode_t mode;

    struct sync_data_t {int sent_msgs; int received_msgs; };
    sync_data_t sync_one_result, sync_collector;

    hash_function_t hasher;

  public:
    network_t network; //!< instance of network_t, you can use it to send/receive messages, etc.

    int cluster_size; //!< Total number of computers
    int network_id; //!< Unique computer identifier
    string processor_name; //!< Name of the computer
    
    //!Returns, whether this workstation is a manager of the distributed computation
    /*!The same as
     * \code
     * distributed.network_id == NETWORK_ID_MANAGER
     * \endcode*/
    bool is_manager() const
    { return (network_id == NETWORK_ID_MANAGER); }
    
    //! A constructor, does't initialize anything!, <br>
    //! non-default error handling vector can be specified
    distributed_t(divine::error_vector_t& arg0 = divine::gerr);
    ~distributed_t(void);
    //! Network initialization
    /*!\param argc = Number of command line parameters. <br>
     *    Passing the first argument of main is common.
     * \param argv = Array of command line parameters. <br> 
     *    Passing the second argument of main is common.
     *
     *  Initializes network, since then cluster_size, network_id and processor_name are valid.
     *  You can also set send buffer's limits using functions of network.
     *  But you still cannot send/receive messages, etc. To complete initialization call the
     *  initialize() function.
     *  
     */
    void network_initialize(int &argc,char **&argv);
    //! Completes initialization. Remember that you can initialize only once.
    void initialize();
    //! Finalizes network. Remember that you can finalize only once.
    void finalize();
    //! Sets a message processor
    void set_message_processor(message_processor_t * proc_message)
    { process_message_functor = proc_message; }
    //! Returns a message processor
    const message_processor_t * get_message_processor() const
    { return process_message_functor; }

    // ret is valid only at target machine
    // ssm stands for "sent sync messages"
    //! Get communication matrix of sent sync messages
    /*! \param ret - output parameter, pointer to communication matrix (of type comm_matrix_t)
     *               that contains at position (\a i, \a j) the count of synchronization
     *               messages sent by workstation with id \a i to
     *               workstation with id \a j
     *  \param target - only one workstation gets valid pointer ret, this workstation is
     *                  specified by \a target parameter, which must be the same on all
     *                  workstations
     *  \return \b true if the function suceeds, \b false otherwise
     *
     *  Synchronization process requires communication between workstations.
     *  Use this function to get the counts of synchronization messages sent by each workstation
     *  to each workstation. The function must be called in a way explained for the 
     *  network_t::barrier() function.*/
    void get_comm_matrix_ssm(pcomm_matrix_t& ret, int target);
    // ret is valid only at target machine
    // rsm stands for "received sync messages"
    //! Get communication matrix of received sync messages
    /*! Similar to get_comm_matrix_ssm(), but the position (\a i, \a j) of the matrix contains
     *  the count of synchronization messages received on 
     *  workstation 
     *  with id \a i from workstation 
     *  with id \a j.*/
    void get_comm_matrix_rsm(pcomm_matrix_t& ret, int target);

    //! Get all sent synchronization messages count
    /*! \return Number of synchronization messages sent by the calling workstation.*/
    int get_all_sent_sync_msgs_cnt(void);
    //! Get all received synchronization messages count
    /*! \return Number of synchronization messages received by the calling workstation.*/
    int get_all_received_sync_msgs_cnt(void);
    //! Get all synchronization barriers count
    /*! \return Number of synchronization barriers on the calling workstation.*/
    int get_all_sync_barriers_cnt(void);

    //! Avoids synchronization
    /*! This method must be called before process_messages() to be effective. */
    void set_busy(void);
    //! Allows synchronization
    /*! This method must be called before process_messages() to be effective. */
    void set_idle(void);
    // following functions set/get parameter which determines whether the buf
    // parameter in process_user_message function points directly to internal
    // buffer or to exclusive memory.
    // if set to true deletion of the memory is left to user
    // if set to false buf must not be deleted!!!
    //! Determines whether the parameter buf in the process_user_message() function points <br>
    //! directly to internal buffers
    /*! \param value - if \b true, then the \a buf parameter in process_user_message()
     *                 points to exclusive memory and the user is responsible for freeing it.
     *                 if \b false, then no special memory is allocated and \a buf points
     *                 directly to internal buffer and user must not free it.*/                 
    void set_proc_msgs_buf_exclusive_mem(bool value);
    //! Gets the value of a variable that determines whether the parameter \a buf
    //! in process_user_message() points directly to internal buffers.
    /*! \b true means \a buf points to newly allocated (exclusive) memory, \b false means it points
     *  to internal buffers.
     *
     *  \b false is default, it saves both time and memory.*/
    void get_proc_msgs_buf_excluseve_mem(bool &value);
    
    //! Sets hash function to be used for partitioning.
    void set_hash_function(hash_functions_t);

    //! Function to determine owner of a state.
    /*!\param state = a state, which association with a computer is to be retrieved
     * \return computer unique identifier of the state owner
     *
     * Partition function, returns the number of the computer which the state passed as
     * the argument belongs to.
     */
    int partition_function(divine::state_t &state);
    //! The same as partition_function(), left here for historical reasons
    int get_state_net_id(divine::state_t &state);
    
    //! Function that checks for arrived messages and participates in synchronization
    /*! This function checks for new messages. If some message arrived, it 
     *  receives it and calls 
     *  process_user_message() function, that's function user must write and give
     *  to distributed_t by assigning it's pointer to process_user_message().
     *  
     *  When called on manager worstation (the one with id NETWORK_ID_MANAGER),
     *  it checks whether the workstation is idle (that is no user message was received
     *  in this process_messages() call and user did not call set_busy()), if yes
     *  and user also called synchronized() function previously, then synchronization
     *  process is initiated.*/
    void process_messages(void);
#if !defined(ORIG_POLL)
    /* The new polling strategy of process_messages() tries to reduce the number of
     * unsuccesful polls, effectively skipping a good fraction of them.  Sometimes this
     * is counter productive, and the caller knows that polling actively is benificial.
     * In that case, call force_poll() before calling process_messages().
     */
    void force_poll(void);
#endif
    //! Pointer to user-defined function that processes user messages
    /*! \param buf - pointer to message data, it points either to newly allocated memory
     *               or to internal buffers (see set_proc_msgs_buf_exclusive_mem())
     *  \param size - size of the received message
     *  \param src - id of the sender workstation
     *  \param tag - tag of the message (Remember that all user messages must have tag greater 
     *               or equal to DIVINE_TAG_USER.               
     *  \param urgent - specifies whether the received message is urgent or not
     *
     *  To be able to use synchronization together with sending your own messages,
     *  you must write this function and assign it's pointer to process_user_message().*/
    void (*process_user_message) (char *buf, int size, int src, int tag, bool urgent);

    //! Function that together with process_messages does synchronization
    /*! \return \b true if workstations are synchronized.
     *    
     *  It works in the following way. By first call to this function 
     *  (on all workstations in the cluster) you are telling that
     *  you want to perform some distributed computation and at the end you wan to synchronize.
     *  During the computation you send messages using network_t::send_message()
     *  or network_t::send_urgent_message(). To receive message use the process_messages()
     *  function which calls process_user_message(), where you can do something with
     *  the received message.
     *  By subsequent calls to synchronized() you determine if all workstations finished
     *  their work. In distributed computation, work mosly implies sending messages, but
     *  if you want to do some bigger amount of work without sending messages
     *  and you don't want to synchronize, use the set_busy() function. Don't forget to
     *  call set_idle() function when your work is finished and remember to call them
     *  before process_messages().
     *  synchronized() and process_messages() should be called quite often to make the
     *  computation efficient.
     *
     *  \warning Synchronization is performed in process_messages() method.*/
    bool synchronized(void);
    //! Function that together with process_messages does synchronization and allows
    //! to collect some information during the synchronization process
    /*! Similar to synchronized() but it allows to collect some information. Let's
     *  take updateable info as an example (see updateable_info_t). You must create
     *  a structure (of type struct), attributes of the structure will be used for
     *  the collected data. The structure also contains update() function, which
     *  manipulates with the attributes. Create an instance of updateable_info_t and pass 
     *  the structure as its template parameter (the actual structure used for collected
     *  data is accessible via the updateable_info_t::data parameter).
     *  Pass the instance as the parameter
     *  of this function. After all workstations are synchronized, manager workstation (0)
     *  sends the contents of the structure to workstation 1, workstation 1 sends it 
     *  to workstation 2, etc. The last workstation then completes the round.
     *  On every workstation the update() function is called, which can manipulate
     *  with the attributes.*/
    bool synchronized(abstract_info_t &info);
  };

  template <class updateable_data_t>
  void updateable_info_t<updateable_data_t, no_const_data_type_t>::update()
  {
    data.update();
  }

  template <class updateable_data_t>
  void updateable_info_t<updateable_data_t, no_const_data_type_t>::get_data_ptr(char *&ptr)
  {
    ptr = static_cast<char *>(static_cast<void *>(&data));
  }

  template <class updateable_data_t>
  void updateable_info_t<updateable_data_t, no_const_data_type_t>::get_data_size(int &size)
  {
    size = sizeof(updateable_data_t);
  }
  
  template <class updateable_data_t, class constant_data_t>
  void updateable_info_t<updateable_data_t, constant_data_t>::update()
  {
    data.update(const_data);
  }

  template <class updateable_data_t, class constant_data_t>
  void updateable_info_t<updateable_data_t, constant_data_t>::get_data_ptr(char *&ptr)
  {
    ptr = static_cast<char *>(static_cast<void *>(&data));
  }

  template <class updateable_data_t, class constant_data_t>
  void updateable_info_t<updateable_data_t, constant_data_t>::get_data_size(int &size)
  {
    size = sizeof(updateable_data_t);
  }


  template <class static_data_t>
  void static_info_t<static_data_t>::get_data_ptr(char *&ptr)
  {
    ptr = static_cast<char *>(static_cast<void *>(&data));
  }

  template <class static_data_t>
  void static_info_t<static_data_t>::get_data_size(int &size)
  {
    size = sizeof(static_data_t);
  }

#ifndef DOXYGEN_PROCESSING  
}
#endif // DOXYGEN_PROCESSING 

#endif











