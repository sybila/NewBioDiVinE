#include <system/state.hh>
#include <fstream>
#include <limits.h>
#include <string.h>

using namespace divine;
using namespace std;

// Types for Huffman
typedef struct {
  unsigned int code;
  unsigned int bits;
} huff_sym_t;

#include "storage/huffman.hh"
#include "common/types.hh"

typedef struct {
  unsigned char *ptr;
  unsigned int  bitpos;
  unsigned int  bytepos;
} huff_bitstream_t;

huff_bitstream_t stream;

unsigned int HuffReadbits(unsigned int bits);	// Reads given number of bits from the stream
void HuffWritebits(unsigned int x, unsigned int bits); // Writes given bits to the stream

// Init the stream
void huffman_init(int state_size)
{
	stream.ptr=new unsigned char[2*state_size];
	memset(stream.ptr,0,2*state_size);
	stream.bitpos=0;
	stream.bytepos=0;
}

void huffman_clear()
{
  delete [] stream.ptr;
}

char* huffman_compress(state_t q, int &size, int oversize)
{
	
	unsigned int symbol;

	//std::cout <<"Uchar max:"<<UCHAR_MAX<<" "<<(UCHAR_MAX/2)*(UCHAR_MAX)<<endl;
	if (q.size>=(UCHAR_MAX/2)*(UCHAR_MAX))
	  {
	    gerr<<"HUFFMAN_COMPRESSION: cannot compress more then "
		<<(UCHAR_MAX/2)*(UCHAR_MAX)<<" bytes at once."<<thr();
	  }


	stream.bytepos=0;
	stream.bitpos=0;
	
	// Encode input stream
	for (size_int_t i=0; i<q.size; i++)
	{
		symbol=(unsigned char)q.ptr[i];
		HuffWritebits(sym[symbol].code, sym[symbol].bits);
	}
	
	// Calculate size of compressed state and fill unused bits in last byte with zeros
	if (stream.bitpos>0) {
		stream.ptr[stream.bytepos]=
		  (stream.ptr[stream.bytepos]>>(8-stream.bitpos))
					     <<(8-stream.bitpos);
		stream.bytepos++;
	}

	// Extract the result vector from the stream	
	size=stream.bytepos + 
	  ((q.size>=(UCHAR_MAX/2)) ? 2*sizeof(unsigned char) : sizeof(unsigned char));
	char *result=new char[size + oversize];
	if (q.size>=(UCHAR_MAX/2))
	  {
	    *(unsigned char*)(result) = 1+(UCHAR_MAX/2)+((unsigned char)(q.size/(UCHAR_MAX+1)));
	    *(unsigned char*)(result+1) = (unsigned char)(q.size%(UCHAR_MAX+1));
	  }
	else
	  {
	    *(unsigned char*)(result) = (unsigned char)q.size;
	  }
	
	memcpy(result + 
	       ((q.size>=(UCHAR_MAX/2)) ? 2*sizeof(unsigned char) : sizeof(unsigned char)), 
	       stream.ptr,
	       stream.bytepos);

	return result;
}


bool huffman_decompress(state_t &q, char* r, int size, int oversize)
{
	unsigned int code, bits, new_bits;
	int delta_bits;	
	int space_for_size = sizeof(unsigned char);
	unsigned char * temp_ptr = stream.ptr;

	size_t q_size=(size_t)(*((unsigned char*)r));
	if (q_size > (UCHAR_MAX/2))
	  {
	    q_size = (q_size & (UCHAR_MAX/2))*(UCHAR_MAX+1) +
	      (*((unsigned char*)r+1));
	    space_for_size += sizeof(unsigned char);
	  }
	//cout <<"q_size="<<q_size<<endl;
	realloc_state(q,q_size+oversize);
	q.size=q_size;

	stream.bytepos=0;
	stream.bitpos=0;
	
	// copy the coded vector to stream
	// omitting initial space for storing the original size	
	// memcpy(stream.ptr, r + space_for_size, size - space_for_size ); 
	stream.ptr = (unsigned char*)r + space_for_size;

	code=0;
	bits=0;
	// For each byte of the vector
	for (size_int_t i=0; i<q.size; i++)
	{  
		// Find coded symbol in the table
		for (short int k=0; k<256; k++)
		{
			delta_bits=sym[k].bits-bits;
			if (delta_bits>0)
			{
				new_bits=HuffReadbits(delta_bits);
				code=code|( new_bits<<(32-bits-delta_bits) );
				bits=sym[k].bits;
			}
			if (((code>>(32-sym[k].bits))<<(32-sym[k].bits)) 
					== (sym[k].code << (32-sym[k].bits)) )
			{
				q.ptr[i]=(char) k;
				code<<=sym[k].bits;
				bits-=sym[k].bits;
				break;
			}
		}
	}
	stream.ptr = temp_ptr;
	return true;
}


unsigned int HuffReadbits(unsigned int bits)
{
  unsigned int x, bit;
  unsigned char *buf;

	/* Get current stream state */
	buf       = &stream.ptr[stream.bytepos];
	bit       = stream.bitpos;

	/* Extract bits */
	x = 0;
	for(unsigned int count = 0; count < bits; count++ )
	{
		x = (x<<1) + (*buf & (1<<(7-bit)) ? 1 : 0);
		bit = (bit+1) & 7;
		if( !bit )
		{
			stream.bytepos++;
			buf++;
		}
	}
	
	/* Store new stream state */
	stream.bitpos   = bit;
	
	return x;
}

void HuffWritebits(unsigned int x, unsigned int bits)
{
	unsigned int  bit;
	unsigned char *buf;
	unsigned int  mask;
		
	/* Get current stream state */
	buf       = &stream.ptr[stream.bytepos];
	bit       = stream.bitpos;
	
	/* Append bits */
	mask = 1 << (bits-1);
	for(unsigned int count = 0; count < bits; count++)
	{
	  *buf = (*buf & (0xff^( 1<<(7-bit) ))) +	((x & mask ? 1 : 0)<<(7-bit));
		x <<= 1;
		bit = (bit+1) & 7;
		if(!bit)
		{
		  stream.bytepos++;
			buf++;
		}
	}
	
	/* Store new stream state */
	stream.bitpos   = bit;
}


