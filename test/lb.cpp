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

      for (int i = 0; i < LB_SIZE; i++) {
        buf[i] = 0;
      }
      
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

    bool eol() const {
      return ((((readInd + (WindowCols / 2))) % WindowCols) == 0);
    }

    bool inStartMargin() const {
      return (readInd % NumImageCols) < (WindowCols / 2);
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

    std::pair<int, int> windowStart() {
      
    }

    bool windowValid() const {
      int nValid = numValidEntries();
      return !inEndMargin() && !inStartMargin() && (nValid >= ((WindowRows - 1)*NumImageCols + WindowCols));
    }

    bool inEndMargin() const {
      int rc = WindowCols / 2;
      //cout << "rc = " << rc << endl;
      int endMargin = NumImageCols - (WindowCols / 2);
      //cout << "End margin = " << endMargin << endl;
      //cout << "Read ind   = " << readInd << endl;
      return (readInd > 0) && (((readInd % NumImageCols) == 0) ||
                               (readInd % NumImageCols) >= NumImageCols - (WindowCols / 2));
    }

    bool sol() const {
      return ((readInd % WindowCols) == 0);
    }
    
    void pop() {
      readInd = (readInd + 1) % LB_SIZE;

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

  // Or maybe the way to do it is to have 3 RAMs, one of which is small
  // and use the buffers exactly the same way, but converting from RAM
  // addresses? So far, this turned out to be the best way. It was easiest because
  // it involved

  // Now I can print out a plausible looking window that shifts slowly, but the
  // register window does not match the computed window in the test case because
  // there is a delay on the filling of the window. The window is not getting filled
  // until popping begins, but the window is supposed to be ready by the time
  // the first call to pop happens. Maybe we should actually shift the window
  // during writes and read the next values of each row register from the write
  // pointer?

  // Now there is a new issue: windowValid becomes invalid if the read pointer
  // gets incremented during the pops used to shift the register window in to
  // its correct place. The pop reduces the distance between read and write addresses
  // which reduces the number of elements in the buffer.

  // I want to be able to stop loading elements (or start shifting the read
  // pointer) during 
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

    RAMAddr writeAddr;
    RAMAddr readAddr;
    
    int writeInd;
    int readInd;

    PixelLoc readTopLeft;
    PixelLoc writeTopLeft;

    bool empty;

  public:

    ImageBuffer3x3() {
      writeInd = 0;
      readInd = 0;

      writeAddr = {0, 0, 3, {NumImageRows}};
      readAddr = {0, 0, 3, {NumImageRows}};
      
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

    void shiftWindowWrite() {
      e00 = e01;
      e01 = e02;
      e02 = readBuf(writeInd);

      e10 = e11;
      e11 = e12;
      e12 = readBuf(abs((writeInd + NumImageCols) % LB_SIZE));


      e20 = e21;
      e21 = e22;
      e22 = readBuf(abs((writeInd + 2*NumImageCols) % LB_SIZE));

      // cout << "--------- Register window" << endl;
      // printRegisterWindow();
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
      writeAddr = increment(writeAddr);

      shiftWindow();
    }

    void readShift() {
      pop();
      shiftWindow();
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
      return {readTopLeft.row + WINDOW_ROW_MARGIN - 2, readTopLeft.col + WINDOW_COL_MARGIN - 2};
    }

    bool windowFull() const {
      int nValid = numValidEntries();      
      return (nValid >= ((WindowRows - 1)*NumImageCols + WindowCols));
    }

    bool windowAlmostFull() const {
      int nValid = numValidEntries();      
      return ((nValid + 2) >= ((WindowRows - 1)*NumImageCols + WindowCols));
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
      //windowFull();
    }

    // Problem: Window is initially invalid, and I want it to become valid
    // before the first pop, but I dont want it to start shifting continuously until
    // I get in to the main loop. Maybe have a delay between filling and starting
    // to shift the window?
    void pop() {
      //shiftWindow();
      
      readInd = (readInd + 1) % LB_SIZE;
      readAddr = increment(readAddr);

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

      cout << "Calling read" << endl;

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

      return readBuf((readInd + NumImageCols*(rowOffset + (WindowRows / 2)) + (colOffset + (WindowCols / 2))) % LB_SIZE);

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

  template<typename ElemType, int NumKernelRows, int NumKernelCols, int NumImageRows, int NumImageCols>
  void bulkConv(const Mem2D<ElemType, NumImageRows, NumImageCols>& input,
                const Mem2D<ElemType, NumKernelRows, NumKernelCols>& kernel,
                Mem2D<ElemType, NumImageRows - 2*(NumKernelRows / 2), NumImageCols - 2*(NumKernelCols / 2) >& output) {

    for (int i = 1; i < NROWS - 1; i++) {
      for (int j = 1; j < NCOLS - 1; j++) {

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

    cout << "Register window on first valid" << endl;
    lb.printRegisterWindow();

    while (true) {

      cout << "Entering loop" << endl;

      if (lb.windowValid()) {
        cout << "Window valid" << endl;
        
        int res = 0;
        for (int row = 0; row < NumKernelRows; row++) {
          for (int col = 0; col < NumKernelCols; col++) {
            //cout << "Calling read" << endl;
            res += kernel(row, col)*lb.read(row - (NumKernelRows / 2), col - (NumKernelCols / 2));
          }
        }

        cout << "Writing to lbOutput" << endl;
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
  
  TEST_CASE("After 23 data loaded, 9 data read linebuffer is at EOL") {
    LineBuffer<int, 3, 3, 10> lb;
    for (int i = 0; i < 23; i++) {
      lb.write(i);
    }

    for (int i = 0; i < 9; i++) {
      cout << lb.read(0, 0) << endl;
      REQUIRE(!lb.inEndMargin());
      lb.pop();
    }
    
    REQUIRE(lb.inEndMargin());
  }

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
