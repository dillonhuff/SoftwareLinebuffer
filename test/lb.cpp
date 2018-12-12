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

    bool empty;
    
  public:

    CircularFIFO() {
      writeInd = 0;
      readInd = 0;
      empty = true;
    }

    bool full() {
      return !empty && (writeInd == readInd);
    }

    void write(const ElemType tp) {
      assert(!full());

      buf[writeInd] = tp;
      writeInd = modInc(writeInd, size);
      empty = false;
    }

    ElemType read() {
      return buf[readInd];
    }

    bool isEmpty() const {
      return empty;
    }

    void pop() {
      readInd = modInc(readInd, size);

      if (writeInd == readInd) {
        empty = true;
      }
    }
  };

  template<typename ElemType, int WindowRows, int WindowCols, int NumImageCols>
  class LineBuffer {

    const static int LB_SIZE = (WindowRows - 1)*NumImageCols + (WindowCols / 2) + WindowCols;
    
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
      // cout << "writeInd = " << writeInd << endl;
      // cout << "readInd  = " << readInd << endl;
      // cout << "LB_SIZE  = " << LB_SIZE << endl;
      return !empty && (writeInd == readInd);
    }

    void write(ElemType t) {
      assert(!full());

      empty = false;
      buf[writeInd] = t;

      writeInd = modInc(writeInd, LB_SIZE);
    }

    void pop() {
      // If we are at the end of a row, shift to the next row
      if ((((readInd + (WindowCols / 2))) % WindowCols) == 0) {
        readInd = (readInd + 2*(WindowCols / 2)) % LB_SIZE;
      } else {
        readInd = (readInd + 1) % LB_SIZE;
      }
      if (readInd == writeInd) {
        empty = true;
      }
    }

    ElemType read(const int rowOffset, const int colOffset) {
      assert(rowOffset <= (WindowRows / 2));
      assert(colOffset <= (WindowCols / 2));

      return buf[(readInd + NumImageCols*(rowOffset + (WindowRows / 2)) + (colOffset + (WindowCols / 2))) % LB_SIZE];
    }

    void printBuffer() {
      for (int i = 0; i < LB_SIZE; i++) {
        cout << buf[i] << " ";
      }
    }

    void printWindow() {
      for (int rowOffset = 0; rowOffset < WindowRows; rowOffset++) {
        for (int colOffset = 0; colOffset < WindowCols; colOffset++) {
          int rawInd = (readInd + NumImageCols*rowOffset + colOffset);
          int ind = rawInd % LB_SIZE;
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

    lb.write(val);
    val++;
    
    cout << "Linebuffer window" << endl;
    lb.printWindow();

    lb.pop();

    lb.write(val);
    val++;

    cout << "Linebuffer window" << endl;
    lb.printWindow();
    
  }


  const int KERNEL_WIDTH = 3;
    
  const int NROWS = 8;
  const int NCOLS = 10;
  
  const int OUT_ROWS = NROWS - 2;    
  const int OUT_COLS = NCOLS - 2;
  
  void lineBufferConv3x3(CircularFIFO<int, NROWS*NCOLS>& input,
                         const vector<int>& kernel,
                         CircularFIFO<int, OUT_ROWS*OUT_COLS>& lbOutput) {
    LineBuffer<int, 3, 3, NCOLS> lb;
    
    while (!lb.full()) {
      cout << "Writing " << input.read() << " to linebuffer" << endl;
      lb.write(input.read());
      input.pop();
    }

    lb.printWindow();
    lb.printBuffer();

    assert(lb.read(-1, -1) == 1);
    assert(lb.read(-1, 0) == 2);
    assert(lb.read(-1, 1) == 3);    

    assert(lb.read(0, -1) == 11);
    assert(lb.read(0, 0) == 12);
    assert(lb.read(0, 1) == 13);
    
    cout << "--------------" << endl;
    int numWrites = 0;
    while (!input.isEmpty()) {
      int top = kernel[0*KERNEL_WIDTH + 0]*lb.read(-1, -1) +
        kernel[0*KERNEL_WIDTH + 1]*lb.read(-1, 0) +
        kernel[0*KERNEL_WIDTH + 2]*lb.read(-1, 1);

      int mid = kernel[1*KERNEL_WIDTH + 0]*lb.read(0, -1) +
        kernel[1*KERNEL_WIDTH + 1]*lb.read(0, 0) +
        kernel[1*KERNEL_WIDTH + 2]*lb.read(0, 1);

      int low = kernel[2*KERNEL_WIDTH + 0]*lb.read(1, -1) +
        kernel[2*KERNEL_WIDTH + 1]*lb.read(1, 0) +
        kernel[2*KERNEL_WIDTH + 2]*lb.read(1, 1);

      cout << "top = " << top << endl;
      cout << "mid = " << mid << endl;
      cout << "low = " << low << endl;
      cout << "---------" << endl;

      int total = top + mid + low;
      cout << "Writing " << total << " to output" << endl;
      lbOutput.write(total);
      numWrites++;

      cout << "Write number = " << numWrites << endl;

      lb.pop();
      lb.write(input.read());

      lb.printWindow();
      cout << "--------------" << endl;
      
      input.pop();
    }
  }
  
  TEST_CASE("Using linebuffer for convolution") {

    vector<int> input;
    int val = 1;
    for (int i = 0; i < NROWS; i++) {
      for (int j = 0; j < NCOLS; j++) {
        input.push_back(val);
        val++;
      }
    }

    vector<int> kernel;
    for (int i = 0; i < KERNEL_WIDTH; i++) {
      for (int j = 0; j < KERNEL_WIDTH; j++) {
        kernel.push_back(i + j);
      }
    }

    vector<int> correctOutput;
    correctOutput.resize(OUT_ROWS*OUT_COLS);

    for (int i = 1; i < NROWS - 1; i++) {
      for (int j = 1; j < NCOLS - 1; j++) {
        int top = kernel[0*KERNEL_WIDTH + 0]*input[(i - 1)*NCOLS + (j - 1)] +
          kernel[0*KERNEL_WIDTH + 1]*input[(i - 1)*NCOLS + j] +
          kernel[0*KERNEL_WIDTH + 2]*input[(i - 1)*NCOLS + (j + 1)];

        int mid = kernel[1*KERNEL_WIDTH + 0]*input[(i)*NCOLS + (j - 1)] +
          kernel[1*KERNEL_WIDTH + 1]*input[(i)*NCOLS + (j)] +
          kernel[1*KERNEL_WIDTH + 2]*input[(i)*NCOLS + (j + 1)];

        int low = kernel[2*KERNEL_WIDTH + 0]*input[(i + 1)*NCOLS + (j - 1)] +
          kernel[2*KERNEL_WIDTH + 1]*input[(i + 1)*NCOLS + j] +
          kernel[2*KERNEL_WIDTH + 2]*input[(i + 1)*NCOLS + (j + 1)];

        cout << "top = " << top << endl;
        cout << "mid = " << mid << endl;
        cout << "low = " << low << endl;
        cout << "---------" << endl;
        correctOutput[(i - 1)*OUT_COLS + (j - 1)] =
          top + mid + low;
      }
    }

    cout << "Correct output" << endl;
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        cout << correctOutput[i*OUT_COLS + j] << " ";
      }
      cout << endl;
    }

    CircularFIFO<int, NROWS*NCOLS> inputBuf;
    for (int i = 0; i < NROWS; i++) {
      for (int j = 0; j < NCOLS; j++) {
        inputBuf.write(input[i*NCOLS + j]);
      }
    }

    CircularFIFO<int, OUT_ROWS*OUT_COLS> lbOutput;
    lineBufferConv3x3(inputBuf, kernel, lbOutput);

    vector<int> lineBufOutput;
    cout << "LB output" << endl;
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        cout << lbOutput.read() << " ";
        lineBufOutput.push_back(lbOutput.read());
        lbOutput.pop();

      }
      cout << endl;
    }

    REQUIRE(lineBufOutput.size() == correctOutput.size());
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        REQUIRE(lineBufOutput[i*OUT_COLS + j] == correctOutput[i*OUT_COLS + j]);
      }
    }
    
  }

}
