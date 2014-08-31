#ifndef DIVINE_MESSAGE_HH
#define DIVINE_MESSAGE_HH
/*!\file network.hh
 * Network support unit header*/

#ifndef DOXYGEN_PROCESSING
#include "common/types.hh"
#include "system/state.hh"
#include "storage/explicit_storage.hh"

namespace divine { // We don't want Doxygen to see namespace 'dve'
#endif // DOXYGEN_PROCESSING

//!Class representing a data to send or receive using network_t
/*!This class is good for sending or receiving of messages
 * consisting of several items (of possibly various types).
 * It supports a transmission of basic integer types, states, state
 * references and general sequences of bytes.
 *
 * \warning This class implicitly allocates 4096 B. It is not good
 * idea to have 1000000 instances of it or to create and destroy
 * its instances many times. It is presumed, that in a program
 * there will be only few "global" instances shared by all sending
 * and receiving procedures.
 */
class message_t
{
 private:
  byte_t * data; //!<Pointers to the byte sequence
  size_int_t size;  //!<Length of the byte sequence
  size_int_t alloc_step; //!<Length of one reallocation step in bytes
  size_int_t first; //!<Index of the first unread byte
  size_int_t end;   //!<Index of the first free byte in byte sequence
  void potentially_realloc(const size_int_t increment);
 public:
  //!A constructor
  /*!\param number_of_preallocated_bytes = a count of bytes to allocate
   *        for a byte sequence representing a message
   * \param reallocation_step = how much more bytes to allocate in case of
   *                            reallocation
   */
  message_t(const size_int_t number_of_preallocated_bytes=1024,
            const size_int_t reallocation_step=1024);
  
  //!A destructor
  /*!It only deallocates a byte sequence stored in a message*/
  ~message_t() { delete [] data; }
  
  //!Returns a stored byte sequence representing a message
  byte_t * get_data() { return data; }
  //!Returns a stored byte sequence representing a message
  const byte_t * get_data() const { return data; }
  //!Sets a byte sequence stored in a message
  /*!\param new_data = byte sequence to store in a message
   * \param new_allocated_size = size in bytes of the memory referenced by
   *        \a new_data
   * \param new_written_size = the number of bytes, which are already written
   *        in the message (valid bytes)
   * \warning The byte sequence set by this method will be deallocated in a
   *          destructor! If you do not like this behavior, use
   *          a method load_data().
   */
  void set_data(byte_t * const new_data, const size_int_t new_allocated_size,
                const size_int_t new_written_size);
  //!Sets a byte sequence stored in a message
  /*!The same as
   * <code>message_t::set_data(new_data,new_allocated_size,new_allocated_size)
   * </code>
   * \warning The byte sequence set by this method will be deallocated in a
   *          destructor! If you do not like this behavior, use
   *          a method load_data().
   */
  void set_data(byte_t * const new_data, const size_int_t new_allocated_size);
  
  //!Replaces a data in a message by given data (using memory copying)
  /*!Writes a given byte sequence to the begin of the message. The reading
   * head in rewound to the begin of the message, writting head is moved
   * to the end of new message content.
   * 
   * Implemented simply as <code>this->rewind();
   * this->append_data(new_data, new_allocated_size);</code>
   */
  void load_data(byte_t * const new_data, const size_int_t new_allocated_size)
  { rewind(); append_data(new_data, new_allocated_size); }

  //!Returns a size of byte sequence representing a message
  size_int_t get_allocated_size() const { return size; }
  //!Returns a size of part of the message, where something has been written
  //! using append_* methods
  size_int_t get_written_size() const { return end; }
  //!Sets a size of written part of message
  /*!It can be useful if you need to write a part of message directly to
   * the memory referenced by get_data().
   */
  void set_written_size(const size_int_t new_size) { end = new_size; }
 
  //!Writes `size_to_copy' bytes from `data_to_copy' to the message
  /*!\param data_to_copy = pointer to the sequence of bytes
   * \param size_to_copy = number of bytes to copy from the byte sequence given
   *                       in \a data_to_copy
   */
  void append_data(const byte_t * const data_to_copy,
                   const size_int_t size_to_copy);
  //!Writes a complete representation of state to the message
  /*!\param state = state to copy to the message
   * 
   * Writes `state.size' and the byte sequence referenced by `state.ptr'
   * to the message
   */
  void append_state(const state_t state);
  //!Writes a state reference `state_ref' to the message
  void append_state_ref(const state_ref_t state_ref);
  //!Writes a flag to the message
  /*!\warning
   * Flag is stored to the single byte (not to sizeof(bool) bytes!)*/
  void append_bool(const bool flag);
  //!Writes a number of type byte_t to the message
  void append_byte(const byte_t number);
  //!Writes a number of type sbyte_t to the message
  void append_sbyte(const sbyte_t number);
  //!Writes a number of type sshort_int_t to the message
  void append_sshort_int(const sshort_int_t number);
  //!Writes a number of type ushort_int_t to the message
  void append_ushort_int(const ushort_int_t number);
  //!Writes a number of type slong_int_t to the message
  void append_slong_int(const slong_int_t number);
  //!Writes a number of type ulong_int_t to the message
  void append_ulong_int(const ulong_int_t number);
  //!Writes a number of type size_int_t to the message
  void append_size_int(const size_int_t number);
  
  //!Copies `size_to_copy' bytes from the message to `data_to_copy'
  /*!\param data_to_copy = the pointer to the byte sequence at least
   *                       \a data_to_copy bytes long
   * \param size_to_copy = count of bytes to copy from the message
   */
  void read_data(char * const data_to_copy, const std::size_t size_to_copy);
  //!Copies a state stored in the message to `state'
  /*!\param state = the state which will be rewritten by the copy of
   *                state stored in the message
   * 
   * \warning This method does not deallocate the byte sequence in state.ptr.
   * This method allocated a new memory space and the user is reponsible
   * for a deallocation of written state.ptr.
   */ 
  void read_state(state_t & state);
  //!Copies a reference to state stored in the message to `ref'
  void read_state_ref(state_ref_t & ref);
  //!Copies a flag from the message to `flag'
  void read_bool(bool & flag);
  //!Copies a number of type byte_t to `number'
  void read_byte(byte_t & number);
  //!Copies a number of type sbyte_t to `number'
  void read_sbyte(sbyte_t & number);
  //!Copies a number of type sshort_int_t to `number'
  void read_sshort_int(sshort_int_t & number);
  //!Copies a number of type ushort_int_t to `number'
  void read_ushort_int(ushort_int_t & number);
  //!Copies a number of type slong_int_t to `number'
  void read_slong_int(slong_int_t & number);
  //!Copies a number of type ulong_int_t to `number'
  void read_ulong_int(ulong_int_t & number);
  //!Copies a number of type size_int_t to `number'
  void read_size_int(size_int_t & number);

  //!Moves a reading head to the beginning of the message
  /*!Then all read_* methods will read from the beginning of the message*/
  void rewind_read() { first = 0; }
  //!Moves a writting head to the beginning of the message
  /*!Then get_written_size() will return 0 and
   * append_* methods will write from the beginning of the message.
   * Use this method if you do not care of the stored message and you want to
   * write a new message to the same instance of this class.
   */
  void rewind_append() { end = 0; }
  //!Moves both reading and writting heads to the beginning of the message
  /*!Then get_written_size() will return 0,
   * append_* methods will write from the beginning of the message and
   * read_* methods will read from the beginning of the message
   * Use this method if you do not care of the stored message and you want to
   * write a new message to the same instance of this class.
   */
  void rewind() { first = 0; end = 0; }
};



#ifndef DOXYGEN_PROCESSING  
}
#endif // DOXYGEN_PROCESSING 

#endif




