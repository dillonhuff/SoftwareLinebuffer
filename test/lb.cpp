#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include "lb.h"

#include <iostream>

using namespace std;

namespace swlb {

  TEST_CASE("First window of an imagebuffer will be at (1, 1)") {
    ImageBuffer<int, 3, 3, 10, 10> lb;
    REQUIRE(lb.nextReadCenter() == PixelLoc(1, 1));
  }

  TEST_CASE("On initialization imagebuffer window is not valid") {
    ImageBuffer<int, 3, 3, 10, 10> lb;
    REQUIRE(!lb.windowValid());
  }

  TEST_CASE("After loading enough rows imagebuffer window is valid") {
    ImageBuffer<int, 3, 3, 10, 10> lb;
    for (int i = 0; i < 10*2 + 3; i++) {
      REQUIRE(!lb.windowValid());
      lb.write(i);
    }

    REQUIRE(lb.windowValid());
  }

  TEST_CASE("After loading enough rows and popping the imagebuffer center has shifted") {
    ImageBuffer<int, 3, 3, 10, 10> lb;
    for (int i = 0; i < 10*2 + 3; i++) {
      REQUIRE(!lb.windowValid());
      lb.write(i);
    }

    REQUIRE(lb.windowFull());    
    REQUIRE(lb.nextReadInBounds());    
    lb.pop();

    REQUIRE(lb.nextReadCenter() == PixelLoc(1, 2));

    lb.write(23);

    REQUIRE(lb.windowFull());    
    REQUIRE(lb.nextReadInBounds());    

    REQUIRE(lb.windowValid());
  }

  TEST_CASE("Loading values in sequence into imagebuffer") {

    const int NROWS = 8;    
    const int NCOLS = 10;

    Mem2D<int, NROWS, NCOLS> mem;
    int val = 1;
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 10; j++) {
        mem.set(i, j, val);
        val++;
      }
    }

    ImageBuffer<int, 3, 3, NROWS, NCOLS> lb;

    int rowInd = 0;
    int colInd = 0;

    // Startup: Fill the linebuffer
    while (!lb.windowValid()) {
      lb.write(mem(rowInd, colInd));
      colInd = (colInd + 1) % NCOLS;
      if (colInd == 0) {
        rowInd++;
      }
    }

    int offset = 0;
    
    auto win = lb.getWindow();
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        REQUIRE(win(i, j) == mem(i, j));
      }
    }

    while ((rowInd < NROWS) && (colInd < NCOLS)) {

      if (lb.windowValid()) {
        cout << "valid ";
      } else {
        cout << "INVALID ";
      }
      cout << "window" << endl;
      lb.printWindow();

      lb.pop();

      lb.write(mem(rowInd, colInd));

      colInd = (colInd + 1) % NCOLS;
      if (colInd == 0) {
        rowInd++;
      }

      offset++;

      if (lb.windowValid()) {
        auto win = lb.getWindow();
        int offRows = offset / NCOLS;
        int offCols = offset % NCOLS;
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            REQUIRE(win(i, j) == mem(offRows + i, offCols + j));
          }
        }
      }
      
    }

    lb.printWindow();
  }
  
  TEST_CASE("Circular buffer") {
    CircularFIFO<int, 100> cb;

    cb.write(10);
    cb.write(13);

    REQUIRE(cb.read() == 10);

    cb.pop();

    REQUIRE(cb.read() == 13);
  }

  const int KERNEL_WIDTH = 3;
    
  const int NROWS = 8;
  const int NCOLS = 10;
  
  const int OUT_ROWS = NROWS - 2;
  const int OUT_COLS = NCOLS - 2;

  template<typename ElemType, int NumRows, int NumCols>
  void fill(CircularFIFO<ElemType, NumRows*NumCols>& buf,
            const Mem2D<ElemType, NumRows, NumCols>& mem) {
    for (int i = 0; i < NumRows; i++) {
      for (int j = 0; j < NumCols; j++) {
        buf.write(mem(i, j));
      }
    }
  }

  Mem2D<int, 3, 3> exampleKernel() {
    Mem2D<int, 3, 3> kernel;
    for (int i = 0; i < KERNEL_WIDTH; i++) {
      for (int j = 0; j < KERNEL_WIDTH; j++) {
        kernel.set(i, j, i + j);
      }
    }

    return kernel;
  }

  Mem2D<int, NROWS, NCOLS> exampleInput() {
    Mem2D<int, NROWS, NCOLS> input;
    //vector<int> input;
    int val = 1;
    for (int i = 0; i < NROWS; i++) {
      for (int j = 0; j < NCOLS; j++) {
        input.set(i, j, val);
        val++;
      }
    }

    return input;
  }

  TEST_CASE("Using linebuffer for convolution") {

    Mem2D<int, NROWS, NCOLS> input = exampleInput();
    Mem2D<int, 3, 3> kernel = exampleKernel();

    Mem2D<int, OUT_ROWS, OUT_COLS> correctOutput;
    bulkConv<int, KERNEL_WIDTH, KERNEL_WIDTH, NROWS, NCOLS>(input, kernel, correctOutput);

    cout << "Correct output" << endl;
    correctOutput.print();

    CircularFIFO<int, NROWS*NCOLS> inputBuf;
    fill(inputBuf, input);

    CircularFIFO<int, OUT_ROWS*OUT_COLS> lbOutput;
    lineBufferConv<int, 3, 3, NROWS, NCOLS>(inputBuf, kernel, lbOutput);

    Mem2D<int, OUT_ROWS, OUT_COLS> lineBufOutput;
    cout << "LB output" << endl;
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        cout << lbOutput.read() << " ";
        lineBufOutput.set(i, j, lbOutput.read());
        lbOutput.pop();

      }
      cout << endl;
    }

    REQUIRE(lineBufOutput.size() == correctOutput.size());
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        REQUIRE(lineBufOutput(i, j) == correctOutput(i, j));
      }
    }
    
  }

  TEST_CASE("Using ImageBuffer3x3 for convolution") {

    Mem2D<int, NROWS, NCOLS> input = exampleInput();
    Mem2D<int, 3, 3> kernel = exampleKernel();

    Mem2D<int, OUT_ROWS, OUT_COLS> correctOutput;
    bulkConv<int, KERNEL_WIDTH, KERNEL_WIDTH, NROWS, NCOLS>(input, kernel, correctOutput);

    cout << "Correct output" << endl;
    correctOutput.print();

    CircularFIFO<int, NROWS*NCOLS> inputBuf;
    fill(inputBuf, input);

    CircularFIFO<int, OUT_ROWS*OUT_COLS> lbOutput;
    lineBufferConv3x3<int, NROWS, NCOLS>(inputBuf, kernel, lbOutput);

    Mem2D<int, OUT_ROWS, OUT_COLS> lineBufOutput;
    cout << "3x3 LB output" << endl;
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        cout << lbOutput.read() << " ";
        lineBufOutput.set(i, j, lbOutput.read());
        lbOutput.pop();

      }
      cout << endl;
    }

    REQUIRE(lineBufOutput.size() == correctOutput.size());
    for (int i = 0; i < OUT_ROWS; i++) {
      for (int j = 0; j < OUT_COLS; j++) {
        REQUIRE(lineBufOutput(i, j) == correctOutput(i, j));
      }
    }
    
  }
  
}
