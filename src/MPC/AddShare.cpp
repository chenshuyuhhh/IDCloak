#include "AddShare.h"
#include "cmath"
#include <cassert>
#include <cstddef>
#include <vector>
#include <volePSI/Defines.h>

namespace volePSI {
extern std::vector<coproto::AsioSocket> psocks; // connection psocks
extern size_t nParties;                         // the party num
extern size_t myIdx;                            // the number of party
extern size_t lIdx;                             // the number of lead party
extern bool normal; // leader party(0), normal party(1)
                    // extern PRNG pPrng;

AddShare::AddShare() {}

AddShare::AddShare(int row, int col) {
  data = Mat2d::Zero(row, col);
  this->col = col;
  this->row = row;
  this->size = row * col;
}

AddShare::AddShare(Mat2d data) {
  this->data = data;
  this->size = data.size();
  this->col = data.cols();
  this->row = data.rows();
}

void AddShare::resize(int row, int col) {
  this->data.resize(row, col);
  this->row = row;
  this->col = col;
  this->size = row * col;
}

void AddShare::PrintSize() {
  std::cout << row << " " << col << " " << size << std::endl;
}

void AddShare::ShuffleOFF(std::vector<AddShare> &shuffle_net) {
  std::vector<BoolShare> bnet;
  oc::u64 pcol;
  oc::u64 prow = ceil(size / 2.0);
  switch (size) {
  case 5256:
    pcol = 23;
    break;
  case 85196:
    pcol = 31;
    break;
  case 1380625:
    pcol = 39;
    break;
  case 22369621:
    pcol = 48;
    break;
  default:
    pcol = ceil(size * (log(size) / log(2)) - 0.91 * size + 1) / prow + 1;
  }
#ifdef DEBUG_INFO
  std::cout << "col " << pcol << std::endl;
#endif
  for (size_t i = 0; i < pcol; i++) {
    bnet.push_back(BoolShare(pcol));
  }
}

void AddShare::ShuffleOFFTrick(std::vector<AddShare> &shuffle_net) {
  oc::u64 pcol;
  oc::u64 prow = ceil(size / 2.0);
  switch (row) {
  case 5256:
    pcol = 23;
    break;
  case 85196:
    pcol = 31;
    break;
  case 1380625:
    pcol = 39;
    break;
  case 22369621:
    pcol = 48;
    break;
  default:
    pcol = ceil(size * (log(size) / log(2)) - 0.91 * size + 1) / prow + 1;
  }
#ifdef DEBUG_INFO
  std::cout << "col " << pcol << std::endl;
#endif
  for (size_t i = 0; i < pcol; i++) {
    shuffle_net.push_back(AddShare(prow, 1));
  }
}

void AddShare::ShuffleOFFTrick(std::vector<Mat2d> &mask, std::vector<Mat2d> &shumask1,
                     std::vector<Mat2d> &shumask2) {
  for (int i = 0; i < nParties; i++) {
    mask[i] = Mat2d(row, col);
    mask[i].setZero();

    shumask1[i] = Mat2d(row, col);
    shumask1[i].setZero();

    shumask2[i] = Mat2d(row, col);
    shumask2[i].setZero();
  }
}

void fixedShuffle(std::vector<int> &indices, int size, int rand)
{
    std::mt19937 gen(rand); // Use rand() as the seed
    indices.resize(size);
    
    // Initialize indices
    for (int i = 0; i < size; ++i) {
        indices[i] = i;
    }
    
    // Shuffle the indices
    std::shuffle(indices.begin(), indices.end(), gen);
}

void fixedShuffle(Mat2d &mat, int rand)
{
    std::mt19937 gen(rand); // Use rand() as the seed
    std::vector<int> indices(mat.rows());
    
    // Initialize indices
    for (int i = 0; i < mat.rows(); ++i) {
        indices[i] = i;
    }
    
    // Shuffle the indices
    std::shuffle(indices.begin(), indices.end(), gen);
    
    // Create a temporary matrix to store the shuffled rows
    Mat2d tempMat = mat;
    
    // Reorder the rows based on the shuffled indices
    for (int i = 0; i < mat.rows(); ++i) {
        mat.row(i) = tempMat.row(indices[i]);
    }
}

void fixedShuffle(Mat2d &mat, std::vector<int> &indices)
{
    assert(indices.size() == mat.rows());
        
    // Create a temporary matrix to store the shuffled rows
    Mat2d tempMat = mat;
    
    // Reorder the rows based on the shuffled indices
    for (int i = 0; i < mat.rows(); ++i) {
        mat.row(i) = tempMat.row(indices[i]);
    }
}

void AddShare::Shuffle(AddShare &result, std::vector<Mat2d> &mask,
                       std::vector<Mat2d> &shumask1,
                       std::vector<Mat2d> &shumask2, int seed) {
  std::vector<int> indices(row);
  fixedShuffle(indices,row, seed);
  for (size_t i = 0; i < nParties; i++) {
    // std::cout << myIdx << " " << i << std::endl;
    if (i == myIdx) {
      std::vector<Mat2d> recvdatas(nParties);
      std::vector<std::thread> threads;

      for (size_t j = 0; j < nParties; j++) {
        if (j != i) {
          threads.emplace_back([&, j] {
            recvdatas[j] = Mat2d(row, col);
            // std::cout << "in thread " << row << " " << col << std::endl;
            recv_array(j, recvdatas[j]);
            // std::cout << "end thread" << recvdatas[j].rows() << " " << recvdatas[j].cols() << std::endl;
          });
        }
      }

      for (auto &t : threads) {
        t.join();
      }
      // std::cout << "finish recv\n";
      // fixedShuffle(result.data, seed);
      for (size_t j = 0; j < nParties; j++) {
        if (j != i) {
          // fixedShuffle(recvdatas[j], seed);
          result.data += recvdatas[j];
          result.data += shumask2[j];
        }
      }
      fixedShuffle(result.data, indices);
    } else {
      result -= mask[i];
      // std::cout << result.row << " " << result.col << std::endl;
      send_array(i, result.data);
      result.data = shumask1[i];
    }
    // std::cout << i << "hhh" << std::endl;
  }
}

void AddShare::Shuffle(AddShare &result, std::vector<AddShare> &shuffle_net,
                       bool off) {
  // result.resize(row, col);
  // offline generate network
  // AShare r;
  // AShare pier;
  if (off) {
    ShuffleOFF(shuffle_net);
  }
#ifdef DEBUG_INFO
  std::cout << off << "off " << row << " " << col << " " << shuffle_net[0].row
            << " " << shuffle_net[0].col << std::endl;
#endif
  // assert(this->row == pie.rows() * 2);
  // assert(.cols() == 1);
  // oc::Matrix<u64> data;
  // Eigen::Matrix<oc::u64, -1, 1> indata = result;
  // oc::Matrix<u64> outdata = ashare;
  oc::u64 temp, p0, p1;
  int midrow = row >> 1;
  int smidrow = midrow - 1;

  Mat2d mgate1(midrow, col);
  Mat2d mgate2(midrow, col);

  // first column
  oc::u64 k = 0;
  for (size_t j = 0; j < row; j += 2, k++) {
    mgate1.row(k) = data.row(j);
    mgate2.row(k) = data.row(j + 1);
    // for (size_t i = 0; i < col; i++)
    // {
    //     mgate1(k, i) = data(j + 1, i);
    //     mgate2(k, i) = data(j, i); // minus
    // }
  }
  // std::cout << "0: \n"
  //           << mgate1 << std::endl
  //           << mgate2 << std::endl;
  AddShare gatesminus(mgate2 - mgate1);
  // gates = gatesminus;
  // std::cout << "before mul\n";
  gatesminus *= shuffle_net[0]; // mul
  // std::cout << "mul\n";

  k = 0;
  mgate1 += gatesminus.data;
  mgate2 -= gatesminus.data;

#ifdef DEBUG_INFO
  std::cout << "0: \n" << mgate1 << std::endl << mgate2 << std::endl;
  // for (size_t j = 0; j < row; ++j)
  // {
  //     std::cout << mgate1(j) << " ";
  // }
  // std::cout << std::endl;
#endif

  // from second column
  // Mat2d lastRow;
  Mat2d tempgate(midrow, col); // m1
  for (size_t i = 1; i < shuffle_net.size(); ++i) {
    // std::cout << i << std::endl;
    // k = 0;
    // gatesminus(0) = (outdata(2) - outdata(0)); // first row
    // indata(k++) = outdata(0);
    // indata(k++) = outdata(2);
    tempgate.row(0) = mgate1.row(0);
    tempgate.block(1, 0, smidrow, col) = mgate2.block(0, 0, smidrow, col);
    // for (size_t j = 1; j < midrow; j++)
    // {
    //     tempgate.row(j) = mgate2.row(j - 1);
    // }
    mgate2.block(0, 0, smidrow, col) = mgate1.block(1, 0, smidrow, col);
    // for (size_t j = 0; j < smidrow; j++)
    // {
    //     mgate2.row(j) = mgate1.row(j + 1);
    // }
    mgate1 = tempgate;

    // lastRow = mgate2.row(smidrow);
    // for (int j = smidrow; j > 0; j--)
    // {
    //     mgate2.row(j) = mgate2.row(j - 1);
    // }
    // mgate2.row(0) = mgate1.row(0);
    // for (int j = 0; j < smidrow; j++)
    // {
    //     mgate1.row(j) = mgate1.row(j + 1);
    // }
    // mgate1.row(smidrow) = lastRow;

    // mgate1.swap(mgate2);

    gatesminus.data = mgate2 - mgate1;
    gatesminus *= shuffle_net[i]; // mul
    mgate1 += gatesminus.data;
    mgate2 -= gatesminus.data;

#ifdef DEBUG_INFO
    std::cout << i << std::endl;
    std::cout << "m1\n" << mgate1 << std::endl;
    std::cout << "m2\n" << mgate2 << std::endl;
    // std::cout << i << ":";
    // for (size_t j = 0; j < row; ++j)
    // {
    //     std::cout << outdata(j) << " ";
    // }
    // std::cout << std::endl;
#endif
  }

  result.resize(row, col);
  // result.data << mgate1, mgate2;
  for (size_t i = 0; i < midrow; i++) {
    result.data.row(i << 1) = mgate1.row(i);
  }
  for (size_t i = 0; i < midrow; i++) {
    result.data.row((i << 1) + 1) = mgate2.row(i);
  }

  // std::cout << "data\n"
  //           << result.data << std::endl;
}

AddShare &AddShare::operator*=(const AddShare &y) {
  assert(this->row == y.row);
#ifdef DEBUG_INFO
  std::cout << row << " " << col << " " << y.row << " " << y.col << std::endl;
#endif
  // x1 x2 x3    y
  if (row == y.row && y.col == 1) {
    // offline r1,r2, r1r2
    AddShare r1(row, col);
    AddShare r2(y.row, y.col);
    AddShare r12(row, col);

    Mat2d emat, fmat;
    // Mat2d emat = (*this + r1).Reveal();
    // Mat2d fmat = (y + r2).Reveal();
    // std::cout << "reveal\n";
    (*this + r1).Reveal(emat);
    // std::cout << "reveal1\n";
    (y + r2).Reveal(fmat);
    // std::cout << "reveal2\n";

#ifdef DEBUG_INFO

    // std::cout << "e\n"
    //           << emat << std::endl;
    // for (size_t i = 0; i < emat.rows(); i++)
    // {
    //     for (size_t j = 0; j < emat.cols(); ++j)
    //     {
    //         std::cout << emat(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "f\n"
    //           << fmat << std::endl;
    // for (size_t i = 0; i < fmat.rows(); i++)
    // {
    //     for (size_t j = 0; j < fmat.cols(); ++j)
    //     {
    //         std::cout << fmat(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }
#endif

    Mat2d ffmat(row, col);
    // AddShare result;
    for (size_t i = 0; i < col; i++) {
      ffmat.col(i) = fmat;
    }
    // std::cout << "ff\n"
    //           << ffmat << std::endl;
    // mul: ef - r1 * f - r2 * e + r1 * r2
    if (myIdx == 0) {
      *this = r12 - r1 * ffmat - r2 * emat + ffmat.cwiseProduct(emat);
      // #ifdef DEBUG_INFO
      //                 std::cout << myIdx << "\n";
      // #endif
    } else {
      *this = r12 - r1 * ffmat - r2 * emat;
      // #ifdef DEBUG_INFO
      //                 std::cout << myIdx << "\n";
      // #endif
    }
    return *this;
  }
}

// AddShare AddShare::operator-() const
// {
//     return AddShare(-data);
// }

AddShare AddShare::operator*(const Mat2d &c) const {
  if (col == c.cols() && row == c.rows()) {
    return AddShare(data.cwiseProduct(c));
  } else if (col == 1 && row == c.rows()) {
    Mat2d d(c.rows(), c.cols());
    for (size_t i = 0; i < c.cols(); i++) {
      d.col(i) = data.cwiseProduct(c.col(i));
    }
    return AddShare(d);
  }
}

AddShare AddShare::operator+(const Mat2d &share) const {
  if (myIdx == 0) {
    return AddShare(data + share);
  }
  return AddShare(data);
}

AddShare AddShare::operator+(const AddShare &share) const {

  return AddShare(share.data + data);
}

AddShare &AddShare::operator+=(const AddShare &share) {
  this->data += share.data;
  return *this;
}

AddShare AddShare::operator-(const Mat2d &share) const {
  if (myIdx == 0) {
    return AddShare(data - share);
  }
  return AddShare(data);
}

AddShare AddShare::operator-(const AddShare &share) const {

  return AddShare(data - share.data);
}

AddShare &AddShare::operator-=(const AddShare &share) {
  this->data -= share.data;
  return *this;
}

void AddShare::RevealThr(Mat2d &result) {
  // std::cout << "reveal\n";
  std::vector<Mat2d> recvdata(nParties);
  // recvdata[myIdx] = MatrixView<oc::u64>(data.data(), data.data() +
  // data.size(), 1);
  if (nParties < 7) {
    int centerpoint = 0;
    if (myIdx == centerpoint) {
      std::vector<std::thread> threads;
      std::vector<std::thread> thr2;
      for (size_t i = 0; i < nParties; i++) {
        if (i != myIdx) {
          recvdata[i].resize(row, col);
          threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
          // MC_AWAIT(socks[i].send(recvdata[p]));
          // MC_AWAIT(socks[i].recv(recvdata[i]));
          // std::cout << "i " << i << ": ";
        }
      }
      for (auto &thread : threads) {
        thread.join();
      }

      result = data;
      for (size_t i = 0; i < nParties; i++) {
        if (i != myIdx) {
          // Eigen::Map<Eigen::Matrix<u64, -1, -1>> temp(recvdata[i].begin(),
          // col, row); std::cout << i << ":\n"
          //           << temp.transpose() << std::endl;
          result += recvdata[i];
        }
        // std::cout << "i " << i << " ";
      }
      for (size_t i = 0; i < nParties; i++) {
        if (i != myIdx) {
          thr2.emplace_back(send_array, i, result);
          // MC_AWAIT(socks[i].send(recvdata[p]));
          // MC_AWAIT(socks[i].recv(recvdata[i]));
          // std::cout << "i " << i << ": ";
        }
      }
      for (auto &thread : thr2) {
        thread.join();
      }
    } else {
      send_array(centerpoint, data);
      result.resize(row, col);
      recv_array(centerpoint, result);
    }
  }
  /*
  if (nParties < 4)
  {
      for (size_t i = 0; i < nParties; i++)
      {
          if (i != myIdx)
          {
              threads.emplace_back(send_array, i, std::ref(data));
              // MC_AWAIT(socks[i].send(recvdata[p]));
              recvdata[i].resize(row, col);
              threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
              // MC_AWAIT(socks[i].recv(recvdata[i]));
              // std::cout << "i " << i << ": ";
          }
      }

      for (auto &thread : threads)
      {
          thread.join();
      }

      // reveal
      result = data;
      for (size_t i = 0; i < nParties; i++)
      {
          if (i != myIdx)
          {
              // Eigen::Map<Eigen::Matrix<u64, -1, -1>>
  temp(recvdata[i].begin(), col, row);
              // std::cout << i << ":\n"
              //           << temp.transpose() << std::endl;
              result += recvdata[i];
          }
          // std::cout << "i " << i << " ";
      }
  }
  else if (nParties < 6)
  {
      size_t cid = nParties - 1;
      if (myIdx == cid)
      {
          for (size_t i = 0; i < cid; i++)
          {
              // std::cout << "i " << i << "  ";
              recvdata[i].resize(row, col);
              threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
          }

          for (auto &thread : threads)
          {
              thread.join();
          }
          // std::cout << "send\n";

          // reveal
          result = data;
          for (size_t i = 0; i < cid; i++)
          {
              result += recvdata[i];
          }
          // std::cout << result << std::endl;
          // std::cout << "compute\n";

          std::vector<std::thread> newThreads;
          for (size_t i = 0; i < cid; i++)
          {
              newThreads.emplace_back(send_array, i, std::ref(result));
          }

          for (auto &thread : newThreads)
          {
              thread.join();
          }
          // std::cout << "receive\n";
      }
      else
      {
          send_array(cid, std::ref(data));
          // std::cout << "send\n";
          result.resize(row, col);
          recv_array(cid, std::ref(result));
          // std::cout << "receive\n";
          // std::cout << result << std::endl;
          // threads.emplace_back(send_array, cid, std::ref(data));
          // threads.emplace_back(recv_array, cid, std::ref(result));
          // for (auto &thread : threads)
          // {
          //     thread.join();
          // }
      }
  }
  */
  else if (nParties <= 8) {
    // std::cout << "reveal\n";
    int cid = nParties / 2;
    if (myIdx < cid) {
      std::vector<std::thread> threads;
      if (myIdx == 0) {
        std::vector<std::thread> thr2;
        std::vector<std::thread> thr3;
        for (size_t i = 1; i < cid; i++) {
          // std::cout << "i " << i << "  ";
          recvdata[i].resize(row, col);
          threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
        }

        for (auto &thread : threads) {
          thread.join();
        }
        // std::cout << "send\n";

        // reveal
        result = data;
        for (size_t i = 1; i < cid; i++) {
          result += recvdata[i];
        }
        // std::cout << "compute\n";

        // threads.clear();

        thr2.emplace_back(send_array, cid, std::ref(result));
        recvdata[cid].resize(row, col);
        thr2.emplace_back(recv_array, cid, std::ref(recvdata[cid]));
        for (auto &thread : thr2) {
          thread.join();
        }
        result += recvdata[cid];

        for (size_t i = 1; i < cid; i++) {
          thr3.emplace_back(send_array, i, std::ref(result));
        }

        for (auto &thread : thr3) {
          thread.join();
        }
        // std::cout << "receive\n";
      } else {
        send_array(0, std::ref(data));
        // std::cout << "send\n";
        result.resize(row, col);
        recv_array(0, std::ref(result));
        // std::cout << "receive\n";
      }
    } else {
      std::vector<std::thread> threads;
      if (myIdx == cid) {
        std::vector<std::thread> thr2;
        std::vector<std::thread> thr3;
        for (size_t i = cid + 1; i < nParties; i++) {
          // std::cout << "i " << i << "  ";
          recvdata[i].resize(row, col);
          threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
        }

        for (auto &thread : threads) {
          thread.join();
        }
        // std::cout << "send\n";

        // reveal
        result = data;
        for (size_t i = cid + 1; i < nParties; i++) {
          result += recvdata[i];
        }
        // std::cout << result << std::endl;
        // std::cout << "compute\n";

        thr2.emplace_back(send_array, 0, std::ref(result));
        recvdata[0].resize(row, col);
        thr2.emplace_back(recv_array, 0, std::ref(recvdata[0]));
        for (auto &thread : thr2) {
          thread.join();
        }
        result += recvdata[0];

        for (size_t i = cid + 1; i < nParties; i++) {
          thr3.emplace_back(send_array, i, std::ref(result));
        }

        for (auto &thread : thr3) {
          thread.join();
        }
        // std::cout << "receive\n";
      } else {
        send_array(cid, std::ref(data));
        // std::cout << "send\n";
        result.resize(row, col);
        recv_array(cid, std::ref(result));
        // std::cout << "receive\n";
      }
    }
  }
  /**
  else if (myIdx == 10)
  {
      int id1 = nParties / 3;
      int id2 = id1 + 2;
      int id3 = id2 + 3;
      if (myIdx == 0)
      {
          for (size_t i = 1; i < id1; i++)
          {
              // std::cout << "i " << i << "  ";
              recvdata[i].resize(row, col);
              threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
          }

          for (auto &thread : threads)
              thread.join();

          // std::cout << "send\n";

          // reveal
          result = data;
          for (size_t i = 1; i < id1; i++)
              result += recvdata[i];
          // std::cout << "compute\n";

          send_array(id1, result);
          recv_array(id1, std::ref(result));
      }
      else if (myIdx < id1)
      {
          send_array(0, data);
          result.resize(row, col);
          recv_array(id1, result);
      }
      else if (myIdx == id1)
      {
          recvdata[myIdx + 1].resize(row, col);
          threads.emplace_back(recv_array, myIdx + 1, std::ref(recvdata[myIdx +
  1])); recvdata[0].resize(row, col); threads.emplace_back(recv_array, 0,
  std::ref(recvdata[0]));

          for (auto &thread : threads)
              thread.join();

          result = data + recvdata[0] + recvdata[myIdx + 1];

          std::vector<std::thread> thr2;
          std::vector<std::thread> thr3;

          thr2.emplace_back(send_array, id3, std::ref(result));
          recvdata[id3].resize(row, col);
          thr2.emplace_back(recv_array, id3, std::ref(recvdata[id3]));
          for (auto &thread : thr2)
              thread.join();

          result += recvdata[id3];

          for (size_t i = 0; i < id2; i++)
          {
              if (i != myIdx)
                  thr3.emplace_back(send_array, i, std::ref(result));
          }
          for (auto &thread : thr3)
              thread.join();
      }
      else if (myIdx < id2)
      {
          send_array(id1, data);
          result.resize(row, col);
          recv_array(id1, result);
      }
      else if (myIdx == id2)
      {
          for (size_t i = id2 + 1; i < id3; i++)
          {
              // std::cout << "i " << i << "  ";
              recvdata[i].resize(row, col);
              threads.emplace_back(recv_array, i, std::ref(recvdata[i]));
          }

          for (auto &thread : threads)
              thread.join();

          // std::cout << "send\n";

          // reveal
          result = data;
          for (size_t i = id2 + 1; i < id3; i++)
              result += recvdata[i];
          // std::cout << "compute\n";

          send_array(id3, result);
          recv_array(id3, std::ref(result));
      }
      else if (myIdx < id3)
      {
          send_array(id2, data);
          result.resize(row, col);
          recv_array(id3, result);
      }
      else if (myIdx == id3)
      {
          recvdata[myIdx + 1].resize(row, col);
          threads.emplace_back(recv_array, myIdx + 1, std::ref(recvdata[myIdx +
  1])); recvdata[id2].resize(row, col); threads.emplace_back(recv_array, 0,
  std::ref(recvdata[id2]));

          for (auto &thread : threads)
              thread.join();

          result = data + recvdata[id2] + recvdata[myIdx + 1];

          std::vector<std::thread> thr2;
          std::vector<std::thread> thr3;

          thr2.emplace_back(send_array, id1, std::ref(result));
          recvdata[id3].resize(row, col);
          thr2.emplace_back(recv_array, id1, std::ref(recvdata[id1]));
          for (auto &thread : thr2)
              thread.join();

          result += recvdata[id1];

          for (size_t i = id2; i < nParties; i++)
          {
              if (i != myIdx)
                  thr3.emplace_back(send_array, i, std::ref(result));
          }
          for (auto &thread : thr3)
              thread.join();
      }
  }
  */
}

void AddShare::Reveal(Mat2d &result) {
  RevealThr(result);
  // macoro::sync_wait(RevealCo(result));
  // std::cout << result << "h\n";
}

Mat2d AddShare::Reveal() {
  Mat2d result;
  RevealThr(result);
  // macoro::sync_wait(RevealCo(result));
  // std::cout << result << "\n";
  return result;
}

void AddShare::Reveal(std::string str) {
  Mat2d result;
  RevealThr(result);
  // macoro::sync_wait(RevealCo(result));
  std::cout << str << ":\n" << result << "\n";
  // return result;
  // std::vector<Mat2d> recvdata(nParties);
  // for (size_t i = 0; i < nParties; i++)
  // {
  //     if (i != )
  //     {
  //         MC_AWAIT(socks[i].send(data.data()));
  //         recvdata[i].resize(row, col);
  //         MC_AWAIT(socks[i].recv(recvdata[i].data()));
  //         // std::cout << "i " << i << ": ";
  //     }
  // }

  // // reveal
  // recvdata[p] = Mat2d(data);
  // for (size_t i = 1; i < nParties; i++)
  // {
  //     std::cout << i << "\n"
  //               << recvdata[i] << std::endl;
  //     recvdata[0] += recvdata[i];
  //     // std::cout << "i " << i << " ";
  // }

  // std::cout << "mat:\n"
  //           << recvdata[0] << std::endl;
}

void AddShare::RevealCovoid() { macoro::sync_wait(RevealCo()); }

Proto AddShare::RevealCo() {
  MC_BEGIN(Proto, this, p = myIdx, result = Mat2d{}, socks = psocks, i = int{},
           recvdata = std::vector<Matrix<oc::u64>>{});
  // std::cout << p << std::endl;
  recvdata.resize(nParties);
  recvdata[p] = MatrixView<oc::u64>(data.data(), data.data() + data.size(), 1);
  if (myIdx == 0) {
    for (i = 0; i < nParties; i++) {
      if (i != myIdx) {
        // MC_AWAIT(socks[i].send(recvdata[p]));
        recvdata[i].resize(col, row);
        MC_AWAIT(socks[i].recv(recvdata[i]));
        // std::cout << "i " << i << ": ";
      }
    }
    result = data;
    for (size_t i = 0; i < nParties; i++) {
      if (i != myIdx) {
        auto *data_ptr = &(*recvdata[i].begin());
        Eigen::Map<Eigen::Matrix<u64, Eigen::Dynamic, Eigen::Dynamic>> temp(
            data_ptr, row, col);
        // Eigen::Map<Eigen::Matrix<u64, -1, -1>> temp(recvdata[i].begin(), col,
        // row); std::cout << i << ":\n"
        //           << temp.transpose() << std::endl;
        result += temp.transpose();
      }
      // std::cout << "i " << i << " ";
    }
  } else {
    MC_AWAIT(socks[0].send(recvdata[p]));
  }

  // reveal

  // std::cout << "result:\n"
  //           << result << std::endl;
  MC_END();
}

Proto AddShare::RevealCo(Mat2d &result) {
  MC_BEGIN(volePSI::Proto, this, &result, p = myIdx, socks = psocks, i = int{},
           recvdata = std::vector<Matrix<oc::u64>>{});
  // std::cout << p << std::endl;
  recvdata.resize(nParties);
  recvdata[p] = MatrixView<oc::u64>(data.data(), data.data() + data.size(), 1);
  for (i = 0; i < nParties; i++) {
    if (i != p) {
      MC_AWAIT(socks[i].send(recvdata[p]));
      recvdata[i].resize(col, row);
      MC_AWAIT(socks[i].recv(recvdata[i]));
      // std::cout << "i " << i << ": ";
    }
  }

  // reveal
  result = data;
  for (size_t i = 0; i < nParties; i++) {
    if (i != myIdx) {
      auto *data_ptr = &(*recvdata[i].begin());
      Eigen::Map<Eigen::Matrix<u64, Eigen::Dynamic, Eigen::Dynamic>> temp(
          data_ptr, row, col);
      // Eigen::Map<Eigen::Matrix<u64, -1, -1>> temp(recvdata[i].begin(), col,
      // row); std::cout << i << ":\n"
      //           << temp.transpose() << std::endl;
      result += temp.transpose();
    }
    // std::cout << "i " << i << " ";
  }
  // std::cout << "result:\n"
  //           << result << std::endl;
  MC_END();
}

volePSI::Proto AddShare::RevealCo(std::string str) {
  MC_BEGIN(volePSI::Proto, this, str, p = myIdx, socks = psocks, i = int{},
           recvdata = std::vector<Matrix<oc::u64>>{});
  // std::cout << p << std::endl;
  recvdata.resize(nParties);
  recvdata[p] = MatrixView<oc::u64>(data.data(), data.data() + data.size(), 1);
  for (i = 0; i < nParties; i++) {
    if (i != p) {
      MC_AWAIT(socks[i].send(recvdata[p]));
      recvdata[i].resize(size, 1);
      MC_AWAIT(socks[i].recv(recvdata[i]));
      // std::cout << "i " << i << ": ";
    }
  }

  // reveal
  // recvdata[p] = oc::Matrix<u64>(ashare);
  for (i = 1; i < nParties; i++) {
    for (size_t j = 0; j < size; j++) {
      recvdata[0](j) += recvdata[i](j);
    }
    // std::cout << "i " << i << " ";
  }

  std::cout << str << std::endl;
  if (col == 1) {
    std::cout << "size " << size << ":";
    for (size_t j = 0; j < recvdata[0].size(); j++) {
      std::cout << recvdata[0](j) << " ";
    }
    std::cout << std::endl;
  } else {
    for (size_t i = 0; i < row; i++) {
      for (size_t j = 0; j < col; j++) {
        std::cout << recvdata[0](j * row + i) << " ";
      }
      std::cout << std::endl;
    }
  }
  MC_END();
}
} // namespace volePSI