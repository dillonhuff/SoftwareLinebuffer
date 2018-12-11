#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <iostream>

using namespace std;

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
      writeInd = modInc(writeInd, size);
    }

    ElemType read() {
      return buf[readInd];
    }

    void pop() {
      readInd = modInc(readInd, size);
    }
  };

  template<typename ElemType, int WindowRows, int WindowCols, int NumImageCols>
  class LineBuffer {

    const static int LB_SIZE = (WindowRows - 1)*NumImageCols + WindowCols;
    
    ElemType buf[LB_SIZE];
    int writeInd;
    int readInd;
    bool empty;

  public:

    LineBuffer() {
      writeInd = 0;
      readInd = 0;
      empty = true;
    }

    bool full() const {
      cout << "writeInd = " << writeInd << endl;
      cout << "readInd  = " << readInd << endl;
      cout << "LB_SIZE  = " << LB_SIZE << endl;
      return !empty && (writeInd == readInd);
    }

    void write(ElemType t) {
      assert(!full());

      empty = false;
      buf[writeInd] = t;
      writeInd = modInc(writeInd, LB_SIZE);
    }

    void pop() {
      readInd = (readInd + 1) % LB_SIZE;
      if (readInd == writeInd) {
        empty = true;
      }
    }

    void printWindow() {
      for (int rowOffset = 0; rowOffset < WindowRows; rowOffset++) {
        for (int colOffset = 0; colOffset < WindowCols; colOffset++) {
          int ind = (readInd + NumImageCols*rowOffset + colOffset) % LB_SIZE;
          cout << buf[ind] << " ";
        }

        cout << endl;
      }
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

  TEST_CASE("3 x 3 linebuffer") {

    LineBuffer<int, 3, 3, 10> lb;

    int val = 1;
    while (!lb.full()) {
      lb.write(val);
      val++;
    }

    cout << "Linebuffer window" << endl;
    lb.printWindow();

    lb.pop();
    val++;
    lb.write(val);

    cout << "Linebuffer window" << endl;
    lb.printWindow();

  }

}
