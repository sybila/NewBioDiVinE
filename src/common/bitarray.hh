#pragma once

#include <stdlib.h>
#include <iostream>
#include <string>

const size_t BITARRAY_CHAR_COUNT=8;
const size_t BITARRAY_MAX_WIDTH=8*BITARRAY_CHAR_COUNT;

class  bitarray_t {
private:
  char data[BITARRAY_CHAR_COUNT];
public:
  bitarray_t();
  void clear();
  bool get(size_t _n) const;
  void set(size_t _n);
  void unset(size_t _n);
  void setTo(size_t _n, bool _val);
  std::string toString(size_t showFirst_n=0);
  void DBGprint(size_t showFirts_n=0);
};

void test_bitarray();
