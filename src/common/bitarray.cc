#include "bitarray.hh"
#include <sstream>

bitarray_t::bitarray_t() 
{
  clear();  
}

void bitarray_t::clear()
{
  for (size_t i=0; i<BITARRAY_CHAR_COUNT; i++)
    data[i]=0;
}

bool bitarray_t::get(size_t _n) const
{
  if (_n>=BITARRAY_MAX_WIDTH) 
    {
      std::cerr <<"Bitarray index is too large. Exit ..."<<std::endl;
      exit(1);
    }
  return (data[_n/8] & (1 << (_n%8) ));	  
}

void bitarray_t::set(size_t _n)
{
  if (_n>=BITARRAY_MAX_WIDTH) 
    {
      std::cerr <<"Bitarray index is too large. Exit ..."<<std::endl;
      exit(1);
    }
  data[_n/8] |=  (1 << (_n%8) );
}  

void bitarray_t::unset(size_t _n)
{
  if (_n>=BITARRAY_MAX_WIDTH) 
    {
      std::cerr <<"Bitarray index is too large. Exit ..."<<std::endl;
      exit(1);
    }
  data[_n/8] &=  ~(1 << (_n%8) );
}  
  
void bitarray_t::setTo(size_t _n, bool _val)
{
  if (_n>=BITARRAY_MAX_WIDTH) 
    {
      std::cerr <<"Bitarray index is too large. Exit ..."<<std::endl;
      exit(1);
    }
  if (_val)
    data[_n/8] |=  (1 << (_n%8) );
  else 
    data[_n/8] &= ~(1 << (_n%8) );
}  

std::string bitarray_t::toString(size_t showFirst_n)
{
  std::ostringstream oss;

  for (size_t i=0; i<((showFirst_n==0)?BITARRAY_CHAR_COUNT:1+(showFirst_n/8)); i++)
    for (size_t j=0; j<(showFirst_n==0?8:(i!=(showFirst_n/8)?8:showFirst_n%8)); j++)
      {
	//std::cout <<"(" <<i <<"," <<j <<")" <<std::endl;
	char x=1;
	x=x<<j;	
	oss <<(data[i]&x?"1":"0");
      }
  return oss.str();
}


void bitarray_t::DBGprint(size_t showFirst_n)
{      
  std::cout <<toString(showFirst_n)<<std::endl;
}

void test_bitarray()
{
  bitarray_t ba;

  ba.DBGprint();

  std::cout <<"ba(3)="<<ba.get(3)<<std::endl;
  std::cout <<"setting ba(3)"<<std::endl;
  ba.set(3);
  std::cout <<"ba(3)="<<ba.get(3)<<std::endl;

  std::cout <<"ba(8)="<<ba.get(8)<<std::endl;
  std::cout <<"setting ba(8)"<<std::endl;
  ba.set(8);
  std::cout <<"ba(8)="<<ba.get(8)<<std::endl;

  std::cout <<"ba(9)="<<ba.get(9)<<std::endl;
  std::cout <<"setting ba(9)"<<std::endl;
  ba.set(9);
  std::cout <<"ba(9)="<<ba.get(9)<<std::endl;

  ba.DBGprint();

  std::cout <<"unsetting ba(3)"<<std::endl;
  ba.unset(3);
  std::cout <<"ba(3)="<<ba.get(3)<<std::endl;

  std::cout <<"unsetting ba(8)"<<std::endl;
  ba.unset(8);
  std::cout <<"ba(8)="<<ba.get(8)<<std::endl;

  ba.DBGprint();

//   std::cout <<"ba(103)="<<ba.get(103)<<std::endl;
//   std::cout <<"setting ba(103)"<<std::endl;
//   ba.set(103);
//   std::cout <<"ba(103)="<<ba.get(103)<<std::endl;

}
