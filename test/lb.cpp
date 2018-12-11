#define CATCH_CONFIG_MAIN

#include "catch.hpp"

namespace swlb {

  template<typename T>
  static inline
  T modInc(T tp, T maxSize) {
    T fresh = tp + 1;
    if (fresh == maxSize) {
      return 0;
    } else {
      return fresh;
    }
  }

  template<typename ElemType, int size>
  class CircularFIFO {

    ElemType buf[size];
    int writeInd;
    int readInd;
    
  public:

    CircularFIFO() {
      writeInd = 0;
      readInd = 0;
    }

    void write(const ElemType tp) {
      buf[writeInd] = tp;
      writeInd++;
    }

    ElemType read() {
      return buf[readInd];
    }

    void pop() {
      readInd = modInc(readInd, size);
    }
  };

  TEST_CASE("Circular buffer") {
    CircularFIFO<int, 100> cb;

    cb.write(10);
    cb.write(13);

    REQUIRE(cb.read() == 10);

    cb.pop();

    REQUIRE(cb.read() == 13);
  }

}
