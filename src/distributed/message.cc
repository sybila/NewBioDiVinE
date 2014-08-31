#include "distributed/message.hh"
#include <fstream>
using namespace divine;
using namespace std;

void message_t::set_data
  (byte_t * const new_data, const size_int_t new_allocated_size,
   const size_int_t new_written_size)
   //defaultly new_written_size=new_allocated_size
  {
    delete [] data;
    data = new_data;
    first = 0;
    end = new_written_size;
    size = new_allocated_size;
  }

void message_t::set_data
  (byte_t * const new_data, const size_int_t new_allocated_size)
{ set_data(new_data, new_allocated_size, new_allocated_size); }

void message_t::potentially_realloc(const size_int_t increment)
{
 end += increment;
 if (end>size) //`end' indexes more then one byte behind the end of
               //allocated space
  {
   size_int_t new_alloc_size= (end/alloc_step + (end%alloc_step!=0))*alloc_step;
   byte_t * new_allocation = new byte_t[new_alloc_size];
   memcpy(new_allocation,data,size);
   delete [] data;
   data = new_allocation;
   size = new_alloc_size;
  }
}

message_t::message_t(const size_int_t number_of_preallocated_bytes,
                     const size_int_t reallocation_step)
{
 alloc_step = reallocation_step;
// size = number_of_preallocated_bytes;
 size = 4;
 data = new byte_t[size];
}

void message_t::append_data(const byte_t * const data_to_copy,
                       const size_int_t size_to_copy)
{
 size_int_t old_end = end;
 potentially_realloc(size_to_copy);
 memcpy(data+old_end,data_to_copy,size_to_copy);
}

void message_t::append_state(const state_t state)
{
 size_t len = state.size;
 char *sdata = state.ptr;

 size_int_t old_end = end;
 potentially_realloc(len + sizeof(size_t));
 memcpy(data+old_end, &len, sizeof(size_t));
 memcpy(data+old_end+sizeof(size_t), sdata, len);
}

void message_t::append_state_ref(const state_ref_t state_ref)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(state_ref_t));
 memcpy(data+old_end, &state_ref, sizeof(state_ref_t));
}

void message_t::append_bool(const bool flag)
{
 append_byte(byte_t(flag));
}

void message_t::append_byte(const byte_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(byte_t));
 memcpy(data+old_end, &number, sizeof(byte_t));
}

void message_t::append_sbyte(const sbyte_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(sbyte_t));
 memcpy(data+old_end, &number, sizeof(sbyte_t));
}

void message_t::append_sshort_int(const sshort_int_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(sshort_int_t));
 memcpy(data+old_end, &number, sizeof(sshort_int_t));
}

void message_t::append_ushort_int(const ushort_int_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(ushort_int_t));
 memcpy(data+old_end, &number, sizeof(ushort_int_t));
}

void message_t::append_slong_int(const slong_int_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(slong_int_t));
 memcpy(data+old_end, &number, sizeof(slong_int_t));
}

void message_t::append_ulong_int(const ulong_int_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(ulong_int_t));
 memcpy(data+old_end, &number, sizeof(ulong_int_t));
}

void message_t::append_size_int(const size_int_t number)
{
 size_int_t old_end = end;
 potentially_realloc(sizeof(size_int_t));
 memcpy(data+old_end, &number, sizeof(size_int_t));
}


void message_t::read_data(char * const data_to_copy, const std::size_t size_to_copy)
{
  memcpy(data_to_copy,data+first,size_to_copy);
  first += size_to_copy;
}


void message_t::read_state(state_t & state)
{
 size_t len = *((size_t *)(data + first));
 char *sdata = (char *) (data + first + sizeof(size_t));

 state = new_state(sdata, len);
 first += len + sizeof(size_t);
}

void message_t::read_state_ref(state_ref_t & ref)
{
 ref = *((state_ref_t *)(data + first));
 first += sizeof(state_ref_t);
}

void message_t::read_bool(bool & flag)
{
 byte_t byte;
 read_byte(byte);
 flag = bool(byte);
}

void message_t::read_byte(byte_t & number)
{
 number = *((byte_t *)(data + first));
 first += sizeof(byte_t);
}

void message_t::read_sbyte(sbyte_t & number)
{
 number = *((sbyte_t *)(data + first));
 first += sizeof(sbyte_t);
}

void message_t::read_sshort_int(sshort_int_t & number)
{
 number = *((sshort_int_t *)(data + first));
 first += sizeof(sshort_int_t);
}

void message_t::read_ushort_int(ushort_int_t & number)
{
 number = *((ushort_int_t *)(data + first));
 first += sizeof(ushort_int_t);
}

void message_t::read_slong_int(slong_int_t & number)
{
 number = *((slong_int_t *)(data + first));
 first += sizeof(slong_int_t);
}

void message_t::read_ulong_int(ulong_int_t & number)
{
 number = *((ulong_int_t *)(data + first));
 first += sizeof(ulong_int_t);
}

void message_t::read_size_int(size_int_t & number)
{
 number = *((size_int_t *)(data + first));
 first += sizeof(size_int_t);
}


