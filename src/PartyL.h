#pragma once

#include <cstdint>
#include <volePSI/Defines.h>
#ifndef PLAYER_L
#define PLAYER_L

#include "MPC/ShareOp.h"
#include "MPC/SmallShare.h"
#include "MulPSILeader.h"
#include "MulPSINormal.h"
#include "MulPSIShare.h"
#include "Network/Communication.h"
#include "Party.h"
#include "RsMulpsi.h"
#include "cryptoTools/Common/BitVector.h"
#include "cryptoTools/Common/CLP.h"
#include "cryptoTools/Common/CuckooIndex.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/IOService.h"
#include "volePSI/GMW/Gmw.h"
#include "volePSI/RsOprf.h"
#include "volePSI/SimpleIndex.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

using namespace oc;

namespace volePSI {
extern std::vector<coproto::AsioSocket> psocks; // connection psocks
extern size_t nParties;                         // the party num
extern size_t myIdx;                            // the number of party
extern size_t lIdx;                             // the number of lead party
extern bool normal; // leader party(0), normal party(1)
extern std::string basepath;

class LeaderParty : public Party {

protected:
  // The mapping of the receiver's input rows to output rows.
  std::vector<u64> mMapping;
  std::vector<u64> mMapRev;
  MulPSILeader lp;
  Matrix<u64> Tf; // for feature alignment
  std::vector<u64> vv;
  MatrixView<u64> mvalues; // values-->mvalues
  RsOprfReceiver mOprfReceiver;
  oc::CuckooIndex<> cuckoo;
  std::array<oc::AES, 3> hashers;

public:
  void circuitPSI();
  void initShare(std::vector<block> &Tx);
  void mapValue();

public:
  LeaderParty() : Party() { normal = false; };
  LeaderParty(size_t size, std::vector<block> mset, MatrixView<u8> mvalues)
      : Party(size, mset, mvalues) {
    normal = false;
#ifdef DEBUG_INFO
    // std::cout << "has create leader party" << std::endl;
#endif
  };

  void printValues() override;

  void getFakeData() override;

  void printHash() override;

  // void HashingInput();
  void HashingInputID() override;

  void HashingInputF() override;

  int64_t IdIntersection() override {
    auto start = std::chrono::system_clock::now();
    // MulPSILeader lp;
    // lp.init(setSize, setSize, cols, ssp, ZeroBlock, numThread); // cols -
    // valuebytesize
    Matrix<u8> rets[nParties];
    std::vector<std::thread> threads;

    for (size_t i = 0; i < nParties; ++i) {
      if (i != myIdx) {
        // Matrix<u8> ret;
        rets[i].resize(Tx.size(), shareinfo.keyByteLength,
                       oc::AllocType::Uninitialized);
        // macoro::sync_wait(lp.receive(Tx, rets[i], psocks[i]));
        // macoro::sync_wait(lp.receive(Tx, ret, psocks[lIdx]));

        threads.emplace_back([this, i, &rets]() {
          macoro::sync_wait(lp.receive(Tx, rets[i], psocks[i]));
        });
      }
    }

    // Join the threads after they finish their execution
    for (auto &thread : threads) {
      thread.join();
    }
    // std::cout << "finish opprf\n";
    // merge flag
    // std::cout << shareinfo.numBins << std::endl;
    Matrix<u64> mLongFlag;
    mLongFlag.resize(shareinfo.numBins, 1, oc::AllocType::Zeroed);
    Matrix<u64> temp;
    for (size_t i = 0; i < nParties; ++i) {
      if (i != myIdx) {
        // convert64(rets[i], temp);
        u8tou64(rets[i], temp);
        // std::cout << i << std::endl;
        // rets[i].release();
        for (u64 j = 0; j < temp.size(); ++j) {
          mLongFlag(j) -= temp(j);
          // std::cout << temp(j) << " ";
        }
      }
    }

    // for (size_t i = 0; i < mLongFlag.size(); i++)
    // {
    //     std::cout << mLongFlag(i) << " ";
    // }
    // std::cout << std::endl;
    idflag.setAshare(mLongFlag);

    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
    // std::cout << myIdx << " Leader done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done -
    // start).count() << "ms" << std::endl;
  }

  std::vector<u64> receiven(std::vector<int> nodes) {
    u64 n = Tx.size();
    std::vector<u64> val[nodes.size()];
    for (size_t i = 0; i < nodes.size(); i++) {
      val[i].resize(n);
    }
    std::vector<std::thread> threads;
    for (size_t i = 0; i < nodes.size(); i++) {
      threads.emplace_back([this, &val, nodes, i]() {
        macoro::sync_wait(
            mokvs.receive<u64>(setSize * 3, Tx, val[i], psocks[nodes[i]]));
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    // std::cout << "after okvs" << std::endl;
    for (size_t j = 1; j < nodes.size(); j++) {
      for (size_t i = 0; i < n; i++) {
        val[0][i] += val[j][i];
      }
    }
    return val[0];
  }

  int64_t IdIntersectionDynamic() override {
    auto start = std::chrono::system_clock::now();

    u64 n = Tx.size();
    std::vector<u64> rets[nParties];
    // std::vector<block> rets[nParties];
    std::vector<std::thread> threads;

    // std::cout << "begin oprf" << std::endl;

    // step1: get oprf
    for (size_t i = 0; i < nParties; ++i) {
      if (i != myIdx) {
        rets[i].resize(n);

        threads.emplace_back([this, i, &rets]() {
          macoro::sync_wait(lp.oprf_receive(Tx, rets[i], psocks[i]));
        });
      }
    }

    for (auto &thread : threads) {
      thread.join();
    }

    // auto mid = std::chrono::system_clock::now();
    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(mid -
    // start).count() << " " << myIdx << " oprf s\n";

    // Matrix<u64> mLongFlag;
    // mLongFlag.resize(shareinfo.numBins, 1, oc::AllocType::Zeroed);
    // std::vector<int> nodes;
    // if (nParties <= 5)
    // {
    //   nodes = {0, 1};
    // }
    // else if (nParties <= 7 || nParties ==8)
    // {
    //   nodes = {0, 1, 2};
    // }
    // else if (nParties <= 8)
    // {
    //   nodes = {0, 1, 2, 3};
    // }
    std::ifstream inputFile("graph_" + std::to_string(nParties) + ".json");
    nlohmann::json data;
    inputFile >> data;
    auto item = data[myIdx];
    std::vector<int> nodes;
    for (const auto &child : item["children"]) {
      // std::cout << child << " ";
      nodes.push_back(child);
    }
    std::vector<u64> val = receiven(nodes);
    // std::cout << "after okvs" << std::endl;

    // for (size_t i = 0; i < n; i++)
    // {
    //   val[0][i] += val[1][i];
    // }

    // for (size_t j = 0; j < nParties; ++j)
    // {
    //   if (j != myIdx)
    //   {
    //     for (size_t i = 0; i < n; i++)
    //     {
    //       val[i] -= rets[j][i];
    //     }
    //   }
    // }
    for (size_t j = 0; j < nParties; ++j) {
      if (j != myIdx) {
        std::transform(val.begin(), val.end(), rets[j].begin(), val.begin(),
                       std::minus<u64>());
      }
    }

    // else if ()
    // {
    // }
    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
  }

  int64_t IdIntersectionRing() override {
    // printHash();
    auto start = std::chrono::system_clock::now();
    std::vector<u64> rets[nParties];
    // std::vector<block> rets[nParties];
    std::vector<std::thread> threads;

    // std::cout << "begin oprf" << std::endl;

    // step1: get oprf
    for (size_t i = 0; i < nParties; ++i) {
      if (i != myIdx) {
        rets[i].resize(Tx.size());

        threads.emplace_back([this, i, &rets]() {
          macoro::sync_wait(lp.oprf_receive(Tx, rets[i], psocks[i]));
        });
      }
    }

    for (auto &thread : threads) {
      thread.join();
    }

    // std::cout << "end oprf" << std::endl;
    // std::cout << "begin okvs" << setSize * 3 << " "<< Tx.size() << std::endl;

    // step2: get okvs
    std::vector<u64> val(Tx.size());
    // std::vector<block> val(Tx.size());
    // receive seed of paxos and init mpaxos
    macoro::sync_wait(mokvs.receive<u64>(setSize * 3, Tx, val, psocks[0]));
    // threads.emplace_back([this, &val]()
    //                      {  });

    // for (size_t i = 0; i < val.size(); i++)
    // {
    //   std::cout << i << ":" << val[i] << std::endl;
    // }

    // std::cout << "end okvs" << std::endl;

    // std::cout << "end" << std::endl;

    for (size_t j = 0; j < nParties; ++j) {
      if (j != myIdx) {
        for (size_t i = 0; i < val.size(); i++) {
          val[i] -= rets[j][i];
        }
      }
    }

    // log
    // for (size_t j = 0; j < nParties; ++j)
    // {
    //   if (j != myIdx)
    //   {
    //     for (size_t i = 0; i < rets[j].size(); i++)
    //     {
    //       std::cout << Tx[i] << ":" << rets[j][i] << std::endl;
    //     }
    //     std::cout << std::endl;
    //   }
    // }

    // log
    // for (size_t i = 0; i < val.size(); i++)
    // {
    //   std::cout << i << ":" << val[i] << std::endl;
    // }

    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
  }

  int64_t FeatureAlign() override {
    // std::cout << "3.Feature Align\n";
    auto start = std::chrono::system_clock::now();
    // MulPSILeader lp;
    // lp.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);
    fs = new AShare[nParties];
    Matrix<u8> rets[nParties];
    if (nParties == 2) {
      uint32_t i = 1 - myIdx;
      rets[i].resize(Tx.size(), pcol[i] << 3, oc::AllocType::Uninitialized);
      macoro::sync_wait(lp.receive(Tx, rets[i], psocks[i]));
      // std::cout << "finish opprf\n";
      Matrix<u64> temp;
      u8tou64(rets[i], temp);
      fs[i].setAshare(temp);

      fs[myIdx] = AShare(Tf);
      fs[myIdx].SecretShare(myIdx);

    } else {
      std::vector<std::thread> threads;

      for (size_t i = 0; i < nParties; ++i) {
        if (i != myIdx) {
          // macoro::sync_wait(lp.receive(Tx, rets[i], psocks[i]));
          // macoro::sync_wait(lp.receive(Tx, ret, psocks[lIdx]));
          rets[i].resize(Tx.size(), pcol[i] << 3, oc::AllocType::Uninitialized);
          threads.emplace_back([this, i, &rets]() {
            // Matrix<u8> ret;
            // std::cout << Tx.size() << " " << rets[i].rows() << " " <<
            // rets[i].cols() << "\n";
            macoro::sync_wait(lp.receive(Tx, rets[i], psocks[i]));
            // std::cout << "finish opprf\n";
            Matrix<u64> temp;
            u8tou64(rets[i], temp);
            fs[i].setAshare(temp);
          });
        }
      }

      // Join the threads after they finish their execution
      for (auto &thread : threads) {
        thread.join();
      }
#ifdef DEBUG_INFO
      // std::cout << "finish opprf\n";
#endif
      fs[myIdx] = AShare(Tf);
#ifdef DEBUG_INFO
      // fs[myIdx].Print();
#endif
      fs[myIdx].SecretShare(myIdx);
    }

    // auto done2 = std::chrono::system_clock::now();
    // std::cout << "OPPRF done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done2 -
    // start).count() << "ms" << std::endl; merge fs
    u64 rows = Tx.size();
    Mat2d tmpmerge(rows, col_sum + 1);
    tmpmerge.setZero();
    // std::cout << rows << " " << col_sum + 1 << std::endl;
    u64 colOffset = 0;
    for (size_t i = 0; i < nParties; ++i) {
      u64 curCol = fs[i].cols();
      Matrix<u64> tmp = fs[i].getShare();
      // std::cout << i << " " << tmp.rows() << " " << tmp.cols() <<
      // std::endl;
      for (u64 r = 0; r < rows; ++r) {
        for (u64 c = 0; c < curCol; ++c) {
          tmpmerge(r, colOffset + c) = tmp(r, c);
        }
      }
      colOffset += curCol;
    }
    // std::cout << "finish merge" << std::endl;
    // auto done3 = std::chrono::system_clock::now();
    // std::cout << "Local merge done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done3 -
    // done2).count() << "ms" << std::endl;

    AddShare mergef(tmpmerge);
    AddShare result(rows, col_sum + 1);
    std::vector<Mat2d> mask(nParties);
    std::vector<Mat2d> shumask1(nParties);
    std::vector<Mat2d> shumask2(nParties);

    mergef.ShuffleOFFTrick(mask, shumask1, shumask2); // fake offline
    // std::cout << "finish trick offline" << std::endl;
    mergef.Shuffle(result, mask, shumask1, shumask2, 1234 + myIdx);

    // auto done4 = std::chrono::system_clock::now();
    // std::cout << "Local merge done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done4 -
    // done3).count() << "ms" << std::endl;

    // // fake id rows
    // AddShare data1(rows, 1);
    // data1.Reveal();

    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
    // std::cout << myIdx << " Leader done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done -
    // start).count() << "ms" << std::endl;
  }

  // void mulCircuitPSI() override
  // {
  //   volePSI::RsMulpsiReceiver receiver;
  //   receiver.init(setSize, setSize, cols, ssp, ZeroBlock,
  //                 numThread); // 40 is the statistical param

  //   std::cout << "Recver start\n";
  //   auto start = std::chrono::system_clock::now();
  //   macoro::sync_wait(receiver.send(cuckooSeed, psocks, myIdx));

  //   std::vector<block> Tx; // Tx = hash(x)
  //   // RsMulpsiReceiver::Share rss;
  //   initShare(ref(Tx));
  //   // macoro::sync_wait(recevier.receive(mset, ss, psocks, lIdx));
  //   std::vector<std::thread> threads;

  //   for (size_t i = 0; i < nParties; ++i)
  //   {
  //     if (i != myIdx)
  //     {
  //       macoro::sync_wait(receiver.receive(Tx, ss, psocks[i], i));
  //     }
  //   }
  //   // ss.print();
  //   auto done = std::chrono::system_clock::now();
  //   std::cout << "Leader done, "
  //             << std::chrono::duration_cast<std::chrono::milliseconds>(done -
  //                                                                      start)
  //                    .count()
  //             << "ms" << std::endl; // test right
  //                                   // ss.print();
  //                                   // flagshare.RevealPrint();
  // }

  void testPSI() override { std::cout << "test " << setSize << std::endl; }

  void printMergeData() override {
    Matrix<u8> plain[nParties];
    for (size_t i = 0; i < nParties; i++) {
      // std::cout << i << ":";
      valueshare[i].Reveal(plain[i]);
    }

    Matrix<u64> indexs;
    indexPool.Reveal(indexs);
    int temp;
    std::vector<u64> ps;
    for (size_t j = 0; j < indexs.cols(); j++) {
      temp = 0;
      for (size_t k = 0; k < indexs.rows(); k++) {
        temp += indexs(k, j);
        if (indexs(k, j) == 1) // k is the index of hash table and T[k] is the
                               // intersection element.
        {
          ps.push_back(k);
        }
      }
      if (temp != 1) {
        std::cout << "Index failed\n";
        return;
      }
    }

    for (u64 p : ps) {                           // print T(p)
      std::cout << mset.at(mMapRev[p]) << ":\n"; // id
      for (size_t i = 0; i < nParties; i++) {
        std::cout << i << ":";
        for (size_t j = 0; j < cols; j++) {
          std::cout << +plain[i](p, j) << " ";
        }
        std::cout << std::endl;
      }
    }
  }

  void writeMergeData() override {
    Matrix<u8> plain[nParties];
    for (size_t i = 0; i < nParties; i++) {
      // std::cout << i << ":";
      valueshare[i].Reveal(plain[i]);
    }

    Matrix<u64> indexs;
    indexPool.Reveal(indexs);
    int temp;
    std::vector<u64> ps;
    for (size_t j = 0; j < indexs.cols(); j++) {
      temp = 0;
      for (size_t k = 0; k < indexs.rows(); k++) {
        temp += indexs(k, j);
        if (indexs(k, j) == 1) // k is the index of hash table and T[k] is the
                               // intersection element.
        {
          ps.push_back(k);
        }
      }
      if (temp != 1) {
        std::cout << "Index failed\n";
        return;
      }
    }

    std::ofstream file(basepath + "plaintext/data/0merged_data.csv");
    // block key;
    for (u64 p : ps) { // print T(p)
      // key = mset.at(mMapRev[p]);
      // std::cout << mset.at(mMapRev[p]).mData << ":\n";
      file << mset.at(mMapRev[p]);
      // std::cout << mset.at(mMapRev[p]) << ":\n"; // id
      for (size_t i = 0; i < nParties; i++) {
        // std::cout << i << ":";
        for (size_t j = 0; j < cols; j++) {
          file << "," << +plain[i](p, j);
        }
      }
      file << "\n";
    }
  }
};
} // namespace volePSI

#endif