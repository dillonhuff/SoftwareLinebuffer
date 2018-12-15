#pragma once

#include <iostream>
#include <cassert>

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

  template<typename ElemType, int NumRows, int NumCols>
  class Mem2D {

    ElemType elems[NumRows*NumCols];

  public:

    Mem2D() {
      for (int i = 0; i < NumRows; i++) {
        for (int j = 0; j < NumRows; j++) {
          set(i, j, 0);
        }
      }
    }

    void print() {
      for (int i = 0; i < NumRows; i++) {
        for (int j = 0; j < NumCols; j++) {
          cout << (*this)(i, j) << " ";
        }
        cout << endl;
      }
    }
    
    int size() const {
      return NumRows*NumCols;
    }

    ElemType operator()(const int r, const int c) const {
      return elems[r*NumCols + c];
    }

    void set(const int r, const int c, const ElemType tp) {
      elems[r*NumCols + c] = tp;
    }
  };

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

      for (int i = 0; i < size; i++) {
        buf[i] = 0;
      }

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

  class PixelLoc {
  public:
    int row;
    int col;

    PixelLoc() : row(0), col(0) {}
    PixelLoc(const int r, const int c) : row(r), col(c) {}
  };

  bool operator==(const PixelLoc a, const PixelLoc b) {
    return (a.row == b.row) && (a.col == b.col);
  }

  std::ostream& operator<<(std::ostream& out, const PixelLoc b) {
    out << "(" << b.row << ", " << b.col << ")";
    return out;
  }

  class RAMAddr {
  public:
    int ramNumber;
    int indexInRAM;
    int numRAMs;
    int ramWidth;
  };

  RAMAddr increment(const RAMAddr addr) {
    RAMAddr inc;
    inc.numRAMs = addr.numRAMs;
    inc.ramWidth = addr.ramWidth;
    
    int nextInd = addr.indexInRAM + 1;
    if (nextInd == addr.ramWidth) {
      nextInd = 0;
      inc.ramNumber = (addr.ramNumber + 1) % addr.numRAMs;
    }

    inc.indexInRAM = nextInd;

    return inc;
  }

  template<typename ElemType, int NumImageRows, int NumImageCols>
  class ImageBuffer3x3 {
  public:

    const static int WindowRows = 3;
    const static int WindowCols = 3;
    
    const static int WINDOW_COL_MARGIN = (WindowCols / 2);
    const static int WINDOW_ROW_MARGIN = (WindowRows / 2);

    const static int OUTPUT_LEFT_BOUND = WINDOW_COL_MARGIN;
    const static int OUTPUT_RIGHT_BOUND = (NumImageCols - WINDOW_COL_MARGIN) - 1;

    const static int OUTPUT_TOP_BOUND = WINDOW_ROW_MARGIN;
    const static int OUTPUT_BOTTOM_BOUND = (NumImageRows - WINDOW_ROW_MARGIN) - 1;

    const static int LB_SIZE = (WindowRows - 1)*NumImageCols + WINDOW_COL_MARGIN + WindowCols;

    ElemType line0[NumImageCols];
    ElemType line1[NumImageCols];
    ElemType line2[WINDOW_COL_MARGIN + WindowCols];

    ElemType e00, e01, e02;
    ElemType e10, e11, e12;
    ElemType e20, e21, e22;

    int writeInd;
    int readInd;

    PixelLoc readTopLeft;
    PixelLoc writeTopLeft;

    bool empty;

  public:

    ImageBuffer3x3() {
      writeInd = 0;
      readInd = 0;

      readTopLeft = {0, 0};
      writeTopLeft = {0, 0};
      
      empty = true;
    }

    void printRegisterWindow() {
      cout << e00 << " " << e01 << " " << e02 << endl;
      cout << e10 << " " << e11 << " " << e12 << endl;
      cout << e20 << " " << e21 << " " << e22 << endl;
    }

    void shiftWindow() {
      e00 = e01;
      e01 = e02;
      e02 = readBuf(readInd);

      e10 = e11;
      e11 = e12;
      e12 = readBuf((readInd + NumImageCols) % LB_SIZE);


      e20 = e21;
      e21 = e22;
      e22 = readBuf((readInd + 2*NumImageCols) % LB_SIZE);

      cout << "--------- Register window" << endl;
      printRegisterWindow();
    }
    
    ElemType readBuf(const int i) {
      int ramNo = i / NumImageCols;
      int ramAddr = i % NumImageCols;

      if (ramNo == 0) {
        return line0[ramAddr];
      } else if (ramNo == 1) {
        return line1[ramAddr];
      } else if (ramNo == 2) {
        return line2[ramAddr];
      }

      assert(false);
    }

    void writeBuf(const int i, const ElemType t) {
      int ramNo = i / NumImageCols;
      int ramAddr = i % NumImageCols;

      if (ramNo == 0) {
        line0[ramAddr] = t;
      } else if (ramNo == 1) {
        line1[ramAddr] = t;
      } else if (ramNo == 2) {
        line2[ramAddr] = t;
      }
    }

    bool full() const {
      return !empty && (writeInd == readInd);
    }

    void write(ElemType t) {
      assert(!full());

      empty = false;
      writeBuf(writeInd, t);

      int nextRow = writeTopLeft.row;
      int nextCol = writeTopLeft.col + 1;
      if (nextCol == NumImageCols) {
        nextCol = 0;
        nextRow = nextRow + 1;
      }

      writeTopLeft = {nextRow, nextCol};
      
      writeInd = modInc(writeInd, LB_SIZE);

      shiftWindow();
    }

    void readShift() {
      pop();
      shiftWindow();
    }

    int numValidEntries() const {
      if (empty) {
        cout << "Empty" << endl;
        return 0;
      }

      if (readInd < writeInd) {
        return writeInd - readInd;
      }

      if (readInd == writeInd) {
        return LB_SIZE;
      }

      // readInd > writeInd
      return (LB_SIZE - readInd) + writeInd;
    }

    PixelLoc nextReadCenter() const {
      return {readTopLeft.row + WINDOW_ROW_MARGIN, readTopLeft.col + WINDOW_COL_MARGIN - 2};
    }

    bool windowFull() const {
      int nValid = numValidEntries();      
      return (nValid >= ((WindowRows - 1)*NumImageCols + WindowCols));
    }

    bool windowAlmostFull() const {
      int nValid = numValidEntries();      
      bool almostFull= ((nValid + 2) >= ((WindowRows - 1)*NumImageCols + WindowCols));

      cout << "nValid      = " << nValid << endl;
      cout << "almost full = " << almostFull << endl;
      return almostFull;
    }
    
    bool nextReadInBounds() const {
      PixelLoc center = nextReadCenter();

      bool inBounds =
        (OUTPUT_LEFT_BOUND <= center.col) &&
        (center.col <= OUTPUT_RIGHT_BOUND) &&
        (OUTPUT_TOP_BOUND <= center.row) &&
        (center.row <= OUTPUT_BOTTOM_BOUND);

      return inBounds;
    }

    bool windowValid() const {

      return nextReadInBounds() &&
        windowAlmostFull();
    }

    // Problem: Window is initially invalid, and I want it to become valid
    // before the first pop, but I dont want it to start shifting continuously until
    // I get in to the main loop. Maybe have a delay between filling and starting
    // to shift the window?
    void pop() {
      
      readInd = (readInd + 1) % LB_SIZE;
      int nextRow = readTopLeft.row;
      int nextCol = readTopLeft.col + 1;
      if (nextCol == NumImageCols) {
        nextCol = 0;
        nextRow = nextRow + 1;
      }
      readTopLeft = {nextRow, nextCol};

      if (readInd == writeInd) {
        empty = true;
      }
    }

    ElemType read(const int rowOffset, const int colOffset) {
      assert(rowOffset <= (WindowRows / 2));
      assert(colOffset <= (WindowCols / 2));

      //cout << "Calling read" << endl;

      if ((rowOffset == -1) && (colOffset == -1)) {
        return e00;
      }
      if ((rowOffset == -1) && (colOffset == 0)) {
        return e01;
      }
      if ((rowOffset == -1) && (colOffset == 1)) {
        return e02;
      }
      
      if ((rowOffset == 0) && (colOffset == -1)) {
        return e10;
      }
      if ((rowOffset == 0) && (colOffset == 0)) {
        return e11;
      }
      if ((rowOffset == 0) && (colOffset == 1)) {
        return e12;
      }

      if ((rowOffset == 1) && (colOffset == -1)) {
        return e20;
      }
      if ((rowOffset == 1) && (colOffset == 0)) {
        return e21;
      }
      if ((rowOffset == 1) && (colOffset == 1)) {
        return e22;
      }

      assert(false);

    }

    void printBuffer() {
      for (int i = 0; i < LB_SIZE; i++) {
        cout << readBuf(i) << " ";
      }
    }

    Mem2D<ElemType, WindowRows, WindowCols>
    getWindow() const {
      Mem2D<ElemType, WindowRows, WindowCols> window;      
      for (int rowOffset = 0; rowOffset < WindowRows; rowOffset++) {
        for (int colOffset = 0; colOffset < WindowCols; colOffset++) {

          int rawInd = (readInd + NumImageCols*rowOffset + colOffset);
          int ind = rawInd % LB_SIZE;
          window.set(rowOffset, colOffset, readBuf(ind));
        }

      }

      return window;
    }

    void printWindow() {
      for (int rowOffset = 0; rowOffset < WindowRows; rowOffset++) {
        for (int colOffset = 0; colOffset < WindowCols; colOffset++) {
          int rawInd = (readInd + NumImageCols*rowOffset + colOffset);
          int ind = rawInd % LB_SIZE;
          cout << readBuf(ind) << " ";
        }

        cout << endl;
      }
    }
    
  };
  
  template<typename ElemType, int WindowRows, int WindowCols, int NumImageRows, int NumImageCols>
  class ImageBuffer {

    const static int WINDOW_COL_MARGIN = (WindowCols / 2);
    const static int WINDOW_ROW_MARGIN = (WindowRows / 2);

    const static int OUTPUT_LEFT_BOUND = WINDOW_COL_MARGIN;
    const static int OUTPUT_RIGHT_BOUND = (NumImageCols - WINDOW_COL_MARGIN) - 1;

    const static int OUTPUT_TOP_BOUND = WINDOW_ROW_MARGIN;
    const static int OUTPUT_BOTTOM_BOUND = (NumImageRows - WINDOW_ROW_MARGIN) - 1;

    const static int LB_SIZE = (WindowRows - 1)*NumImageCols + WINDOW_COL_MARGIN + WindowCols;
    
    ElemType buf[LB_SIZE];
    
    int writeInd;
    int readInd;

    PixelLoc readTopLeft;
    PixelLoc writeTopLeft;

    bool empty;

  public:

    ImageBuffer() {
      writeInd = 0;
      readInd = 0;

      readTopLeft = {0, 0};
      writeTopLeft = {0, 0};
      
      empty = true;
    }

    bool full() const {
      return !empty && (writeInd == readInd);
    }

    void write(ElemType t) {
      assert(!full());

      empty = false;
      buf[writeInd] = t;

      int nextRow = writeTopLeft.row;
      int nextCol = writeTopLeft.col + 1;
      if (nextCol == NumImageCols) {
        nextCol = 0;
        nextRow = nextRow + 1;
      }

      writeTopLeft = {nextRow, nextCol};
      
      writeInd = modInc(writeInd, LB_SIZE);
    }

    int numValidEntries() const {
      if (empty) {
        return 0;
      }

      if (readInd < writeInd) {
        return writeInd - readInd;
      }

      if (readInd == writeInd) {
        return LB_SIZE;
      }

      // readInd > writeInd
      return (LB_SIZE - readInd) + writeInd;
    }

    PixelLoc nextReadCenter() const {
      return {readTopLeft.row + WINDOW_ROW_MARGIN, readTopLeft.col + WINDOW_COL_MARGIN};
    }

    bool windowFull() const {
      int nValid = numValidEntries();      
      return (nValid >= ((WindowRows - 1)*NumImageCols + WindowCols));
    }

    bool nextReadInBounds() const {
      PixelLoc center = nextReadCenter();

      bool inBounds =
        (OUTPUT_LEFT_BOUND <= center.col) &&
        (center.col <= OUTPUT_RIGHT_BOUND) &&
        (OUTPUT_TOP_BOUND <= center.row) &&
        (center.row <= OUTPUT_BOTTOM_BOUND);

      return inBounds;
    }

    bool windowValid() const {

      return nextReadInBounds() &&
        windowFull();
    }

    void pop() {
      readInd = (readInd + 1) % LB_SIZE;

      int nextRow = readTopLeft.row;
      int nextCol = readTopLeft.col + 1;
      if (nextCol == NumImageCols) {
        nextCol = 0;
        nextRow = nextRow + 1;
      }
      readTopLeft = {nextRow, nextCol};

      if (readInd == readInd) {
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

    Mem2D<ElemType, WindowRows, WindowCols>
    getWindow() const {
      Mem2D<ElemType, WindowRows, WindowCols> window;      
      for (int rowOffset = 0; rowOffset < WindowRows; rowOffset++) {
        for (int colOffset = 0; colOffset < WindowCols; colOffset++) {

          int rawInd = (readInd + NumImageCols*rowOffset + colOffset);
          int ind = rawInd % LB_SIZE;
          window.set(rowOffset, colOffset, buf[ind]);

        }

      }

      return window;
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

  template<typename ElemType, int NumKernelRows, int NumKernelCols, int NumImageRows, int NumImageCols>
  void bulkConv(const Mem2D<ElemType, NumImageRows, NumImageCols>& input,
                const Mem2D<ElemType, NumKernelRows, NumKernelCols>& kernel,
                Mem2D<ElemType, NumImageRows - 2*(NumKernelRows / 2), NumImageCols - 2*(NumKernelCols / 2) >& output) {

    for (int i = 1; i < NumImageRows - 1; i++) {
      for (int j = 1; j < NumImageCols - 1; j++) {

        int res = 0;
        for (int r = 0; r < NumKernelRows; r++) {
          for (int c = 0; c < NumKernelCols; c++) {
            res += kernel(r, c) * input(i + r - (NumKernelRows / 2), j + c - (NumKernelCols / 2));
          }
        }

        output.set(i - 1, j - 1, res);

      }
    }
  }

  template<typename ElemType, int NumImageRows, int NumImageCols>
  void lineBufferConv3x3(CircularFIFO<ElemType, NumImageRows*NumImageCols>& input,
                         const Mem2D<ElemType, 3, 3>& kernel,
                         CircularFIFO<ElemType, (NumImageRows - 2*((3)/2))*(NumImageCols - 2*((3)/2)) >& lbOutput) {

    const int NumKernelRows = 3;
    const int NumKernelCols = 3;
    
    ImageBuffer3x3<int, NumImageRows, NumImageCols> lb;
    
    //while (!lb.windowValid()) {
    while (!lb.windowFull()) {
      lb.write(input.read());
      input.pop();
    }

    // Need to have a warmup period where the register window gets shifted
    // into position?

    lb.readShift();
    lb.readShift();

    assert(lb.windowValid());

    cout << "Next read center = " << lb.nextReadCenter() << endl;
    
    assert(lb.nextReadCenter() == PixelLoc(1, 1));

    cout << "Register window on first valid" << endl;
    lb.printRegisterWindow();

    while (true) {

      if (lb.windowValid()) {
        //cout << "Window valid" << endl;
        
        int res = 0;
        for (int row = 0; row < NumKernelRows; row++) {
          for (int col = 0; col < NumKernelCols; col++) {
            //cout << "Calling read" << endl;
            res += kernel(row, col)*lb.read(row - (NumKernelRows / 2), col - (NumKernelCols / 2));
          }
        }

        cout << "Writing to " << res << " to lbOutput" << endl;
        lbOutput.write(res);
      }

      if (input.isEmpty()) {
        break;
      }

      lb.pop();
      lb.write(input.read());

      input.pop();
    }
  }
  
  template<typename ElemType, int NumKernelRows, int NumKernelCols, int NumImageRows, int NumImageCols>
  void lineBufferConv(CircularFIFO<ElemType, NumImageRows*NumImageCols>& input,
                      const Mem2D<ElemType, NumKernelRows, NumKernelCols>& kernel,
                      CircularFIFO<ElemType, (NumImageRows - 2*((NumKernelRows)/2))*(NumImageCols - 2*((NumKernelCols)/2)) >& lbOutput) {

    ImageBuffer<int, NumKernelRows, NumKernelCols, NumImageRows, NumImageCols> lb;
    
    while (!lb.windowValid()) {
      lb.write(input.read());
      input.pop();
    }

    while (true) {

      if (lb.windowValid()) {
        int res = 0;
        for (int row = 0; row < NumKernelRows; row++) {
          for (int col = 0; col < NumKernelCols; col++) {
            res += kernel(row, col)*lb.read(row - (NumKernelRows / 2), col - (NumKernelCols / 2));
          }
        }

        lbOutput.write(res);
      }

      if (input.isEmpty()) {
        break;
      }

      lb.pop();
      lb.write(input.read());

      input.pop();
    }
  }

  
}
