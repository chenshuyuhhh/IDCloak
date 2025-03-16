#pragma once

#include <cryptoTools/Common/Defines.h>
#ifndef PLAYER_N
#define PLAYER_N

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
extern PRNG pPrng;

class NormalParty : public Party {
  std::vector<std::array<u64, 3>> mMapping;
  Matrix<u8> Tv; // for id intersection
  Matrix<u8> Tf; // for feature alignment
  oc::Matrix<u8> rs;
  MulPSINormal ln;
  std::vector<u64> vv;
  MatrixView<u64> mvalues;
  SimpleIndex sIdx;
  std::array<oc::AES, 3> hashers;
  std::vector<u64> oprf_ret;
  std::vector<u64> r;

  RsOprfSender mOprfSender;
  // coproto::Socket sock; // connection

public:
  NormalParty() : Party() { normal = true; };
  NormalParty(size_t size, std::vector<block> mset, MatrixView<u8> mvalues)
      : Party(size, mset, mvalues) {
    normal = true;
#ifdef DEBUG_INFO
    // std::cout << "has create normal party" << std::endl;
#endif
  };

  void printValues() override;
  void getFakeData() override;

  void printHash() override;

  void HashingInputID() override;

  void HashingInputF() override;

  int64_t IdIntersection() override {
    // for (size_t i = 0; i < Tv.size(); i++)
    // {
    //     std::cout << +(Tv(i)) << " ";
    // }
    // std::cout << std::endl;

    auto start = std::chrono::system_clock::now();
    // MulPSINormal ln;
    // ln.init(setSize, setSize, vcols, ssp, ZeroBlock, numThread);
    // std::cout << Tv.rows() << " " << Tv.cols() << std::endl;
    // for (size_t i = 0; i < Tv.rows(); i++)
    // {
    //     for (size_t j = 0; j < Tv.cols(); j++)
    //     {
    //         std::cout << +(Tv(i, j)) << " ";
    //     }
    //     std::cout << " | ";
    // }
    // std::cout << std::endl;
    macoro::sync_wait(ln.send(Tx, Tv, shareinfo.numBins, psocks[lIdx]));
    // std::cout << "finish opprf\n";
    Matrix<u64> mLongFlag;
    // convert64(mFlag, mLongFlag);
    u8tou64(mFlag, mLongFlag);

    // for (size_t i = 0; i < mLongFlag.size(); i++)
    // {
    //     std::cout << mLongFlag(i, 0) << " ";
    // }
    // std::cout << std::endl;
    idflag.setAshare(mLongFlag);
    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
    // std::cout << myIdx << " Normal done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done -
    // start).count() << "ms" << std::endl; // test right
  }

  void oprfEval(span<const block> Tx, RsOprfSender &oprf,
                std::vector<u64> &ret) {
    u64 n = Tx.size();
    u64 m = sizeof(u64);
    std::unique_ptr<u8[]> tempPtr;
    tempPtr.reset(new u8[n * sizeof(block)]);
    span<block> oprfOutput((block *)tempPtr.get(), n);
    // std::vector<block> oprfOutput(n);

    // std::cout << "begin oprf eval " << n << " " << oprfOutput.size() <<
    // std::endl;
    oprf.eval(Tx, oprfOutput);
    // std::cout << "end oprf eval" << std::endl;

    for (u64 i = 0; i < n; ++i)
      memcpy(&ret[i], &oprfOutput[i], m);
  }

  // void rands2l(int node)
  // {
  //   std::vector<u64> values(Tx.size(), 0);
  //   std::vector<u64> ret2(Tx.size());
  //   macoro::sync_wait(mokvs.receive<u64>(setSize * 3, Tx, ret2,
  //   psocks[node]));
  //   // std::cout << "receive okvs" << std::endl;

  //   for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i)
  //   {
  //     auto size = sIdx.mBinSizes[i];
  //     for (u64 p = 0; p < size; ++p, ++k)
  //     {
  //       values[k] = oprf_ret[k] + ret2[k] + r[i];
  //       // std::cout << i << ":" << ret[k] << " " << ret2[k] << " " << r[i]
  //       << std::endl;
  //     }
  //   }
  //   // std::cout << "begin send okvs" << std::endl;
  //   macoro::sync_wait(mokvs.send<u64>(Tx, values, psocks[lIdx]));
  // }

  void rands2n(int fromnode, int tonode) {
    std::vector<u64> values(Tx.size(), 0);
    std::vector<u64> ret2(Tx.size());
    macoro::sync_wait(
        mokvs.receive<u64>(setSize * 3, Tx, ret2, psocks[fromnode]));
    // std::cout << "receive okvs" << std::endl;

    for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
      auto size = sIdx.mBinSizes[i];
      auto rValue = r[i];
      for (u64 p = 0; p < size; ++p, ++k) {
        values[k] = oprf_ret[k] + ret2[k] + rValue;
        // std::cout << i << ":" << ret[k] << " " << ret2[k] << " " << r[i] <<
        // std::endl;
      }
    }
    // std::cout << "begin send okvs" << std::endl;
    macoro::sync_wait(mokvs.send<u64>(Tx, values, psocks[tonode]));
  }

  void rands2n(std::vector<int> fromnodes, int tonode) {
    std::vector<u64> values(Tx.size(), 0);
    std::vector<u64> ret2[fromnodes.size()];
    for (size_t i = 0; i < fromnodes.size(); i++) {
      ret2[i].resize(Tx.size());
    }
    std::vector<std::thread> threads;
    for (size_t i = 0; i < fromnodes.size(); i++) {
      threads.emplace_back([this, &ret2, fromnodes, i]() {
        macoro::sync_wait(
            mokvs.receive<u64>(setSize * 3, Tx, ret2[i], psocks[fromnodes[i]]));
      });
    }
    for (auto &thread : threads) {
      thread.join();
    }
    // macoro::sync_wait(mokvs.receive<u64>(setSize * 3, Tx, ret2,
    // psocks[fromnode])); std::cout << "receive okvs" << std::endl;
    for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
      auto size = sIdx.mBinSizes[i];
      auto rValue = r[i];
      for (u64 p = 0; p < size; ++p, ++k) {
        values[k] = oprf_ret[k] + rValue;
        // std::cout << i << ":" << ret[k] << " " << ret2[k] << " " << r[i] <<
        // std::endl;
      }
    }
    for (const auto &vec : ret2) {
      std::transform(values.begin(), values.end(), vec.begin(), values.begin(),
                     std::plus<u64>());
    }
    // std::cout << "begin send okvs" << std::endl;
    macoro::sync_wait(mokvs.send<u64>(Tx, values, psocks[tonode]));
  }

  void rands2l(std::vector<int> nodes) { rands2n(nodes, lIdx); }

  void send2n(int node) {
    std::vector<u64> values(Tx.size(), 0);
    for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
      auto size = sIdx.mBinSizes[i];
      auto rValue = r[i];
      // std::cout << i << ":";
      for (u64 p = 0; p < size; ++p, ++k) {
        values[k] = oprf_ret[k] + rValue;
        // std::cout << " " << p << " ";
        // std::cout << i << ":" << ret[k] << " " << r[i] << std::endl;
      }
      // std::cout << std::endl;
    }
    macoro::sync_wait(mokvs.send<u64>(Tx, values, psocks[node]));
  }
  void send2l() {
    std::vector<u64> values(Tx.size(), 0);
    for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
      auto size = sIdx.mBinSizes[i];
      auto rValue = r[i];
      // std::cout << i << ":";
      for (u64 p = 0; p < size; ++p, ++k) {
        values[k] = oprf_ret[k] + rValue;
        // std::cout << " " << p << " ";
        // std::cout << i << ":" << ret[k] << " " << r[i] << std::endl;
      }
      // std::cout << std::endl;
    }
    macoro::sync_wait(mokvs.send<u64>(Tx, values, psocks[lIdx]));
  }

  int64_t IdIntersectionDynamic() override {
    auto start = std::chrono::system_clock::now();
    // std::vector<std::thread> threads;
    u64 n = Tx.size();
    RsOprfSender oprf;
    // std::vector<u64> ret(n), ret2(n), values(n, 0);
    oprf_ret.resize(n);
    r.resize(shareinfo.numBins);
    pPrng.get<u64>(r);

    macoro::sync_wait(
        ln.oprf_send(shareinfo.numBins, Tx, oprf_ret, psocks[lIdx]));
    // auto mid = std::chrono::system_clock::now();
    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(mid -
    // start).count() << " " << myIdx << " oprf s\n";

    std::ifstream inputFile("graph_" + std::to_string(nParties) + ".json");
    nlohmann::json data;
    inputFile >> data;
    auto item = data[myIdx];
    int parent = item["parent"];
    // std::cout << "parent " << parent << std::endl;
    if (item.contains("children")) {
      std::vector<int> children = item["children"];
      int l = children.size();
      if (l > 1) {
        // for (int child : children)
        // {
        //   std::cout << child << std::endl;
        // }
        rands2n(children, parent);
      } else {
        rands2n(children[0], parent);
        // std::cout << children[0] << std::endl;
      }
    } else {
      // std::cout << "no child" << std::endl;
      send2n(parent);
    }
    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
  }

  int64_t IdIntersectionRing() override {
    // printHash();
    auto start = std::chrono::system_clock::now();
    // std::vector<std::thread> threads;
    u64 n = Tx.size();
    // u64 m = sizeof(u64);
    RsOprfSender oprf;
    std::vector<u64> ret(n), ret2(n), values(n, 0);
    std::vector<u64> r(shareinfo.numBins);
    pPrng.get<u64>(r);
    // macoro::sync_wait(oprf.init(shareinfo.numBins, ln.mPrng, psocks[lIdx]));
    // std::cout << "begin oprf" << std::endl;

    macoro::sync_wait(ln.oprf_send(shareinfo.numBins, Tx, ret, psocks[lIdx]));

    // std::cout << "end oprf" << std::endl;
    // std::vector<block> ret(Tx.size());
    // std::cout << Tx.size() << " " << ret.size() << " " << shareinfo.numBins
    // << std::endl; step1: oprf with lparty threads.emplace_back([this,
    // &oprf]()
    //                      { macoro::sync_wait(ln.oprf_send(shareinfo.numBins,
    //                      psocks[lIdx], oprf)); });

    if (myIdx < lIdx - 1) {
      // std::cout << "begin okvs in" << std::endl;
      // threads.emplace_back([this, &ret2]()
      //                      { macoro::sync_wait(mokvs.receive<u64>(setSize *
      //                      3, Tx, ret2, psocks[myIdx + 1])); }); // get okvs
      //                      and decode
      macoro::sync_wait(
          mokvs.receive<u64>(setSize * 3, Tx, ret2, psocks[myIdx + 1]));
      // std::cout << "begin oprf" << std::endl;
      // oprfEval(Tx, oprf, ret);

      // threads[1].join();
      // std::cout << "begin okvs" << std::endl;
      // calculate values of okvs
      for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
        auto size = sIdx.mBinSizes[i];
        for (u64 p = 0; p < size; ++p, ++k) {
          values[k] = ret[k] + ret2[k] + r[i];
          // std::cout << i << ":" << ret[k] << " " << ret2[k] << " " << r[i] <<
          // std::endl;
        }
      }
      // std::cout << "end" << std::endl;
    } else {
      // std::cout << "begin okvs 0 :" << shareinfo.numBins << std::endl;
      // oprfEval(Tx, oprf, ret);
      for (u64 i = 0, k = 0; i < shareinfo.numBins; ++i) {
        auto size = sIdx.mBinSizes[i];
        // std::cout << i << ":";
        for (u64 p = 0; p < size; ++p, ++k) {
          values[k] = ret[k] + r[i];
          // std::cout << " " << p << " ";
          // std::cout << i << ":" << ret[k] << " " << r[i] << std::endl;
        }
        // std::cout << std::endl;
      }
    }
    // std::cout << "end okvs" << std::endl;
    macoro::sync_wait(
        mokvs.send<u64>(Tx, values, psocks[myIdx > 0 ? myIdx - 1 : lIdx]));
    // std::cout << "end okvs" << std::endl;
    // threads[0].join();

    // for (size_t i = 0; i < ret.size(); i++)
    // {
    //   std::cout << Tx[i] << ":" << ret[i] << std::endl;
    // }

    // std::cout << "results" << std::endl;
    // for (u64 i = 0; i < shareinfo.numBins; ++i)
    // {
    //   std::cout << i << ":" << r[i] << std::endl;
    // }

    auto done = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
  }

  int64_t FeatureAlign() override {
    auto start = std::chrono::system_clock::now();
    // MulPSINormal ln;
    // ln.init(setSize, setSize, vcols, ssp, ZeroBlock, numThread);
    fs = new AShare[nParties];
    std::vector<std::thread> threads;

    // std::cout << Tf.rows() << " " << Tf.cols() << std::endl;
    // for (size_t i = 0; i < Tf.rows(); i++)
    // {
    //     for (size_t j = 0; j < Tf.cols(); j++)
    //     {
    //         std::cout << +(Tf(i, j)) << " ";
    //     }
    //     std::cout << " | ";
    // }
    // std::cout << std::endl;
    // threads.emplace_back([this]()
    //                      {
    // std::cout << Tx.size() << " " << Tf.rows() << " " << Tf.cols() << "\n";
    macoro::sync_wait(ln.send(Tx, Tf, shareinfo.numBins, psocks[lIdx]));
    // std::cout << "finish opprf\n";
#ifdef DEBUG_INFO
    // std::cout << "finish opprf\n";
#endif
    // });

    for (size_t i = 0; i < nParties; i++) {
      if (i == lIdx) {
        threads.emplace_back([this, i]() {
          fs[lIdx] = AShare(shareinfo.numBins, pcol[lIdx]);
          fs[lIdx].SecretShare(lIdx);
#ifdef DEBUG_INFO
          // std::cout << "finish leader\n";
#endif
        });
      } else if (i == myIdx) {
        threads.emplace_back(
            [this]() { // secret share to others from local data
              Matrix<u64> result;
              u8tou64(rs, result);
              fs[myIdx].setAshare(result);
              std::set<size_t> ids;

              // std::cout << "hhh " << myIdx << " " << lIdx << std::endl;
              for (int j = 0; j < nParties; j++) {
                if (j != myIdx && j != lIdx) {
                  // std::cout << j << std::endl;
                  ids.insert(j);
                }
              }
#ifdef DEBUG_INFO
          // std::cout << myIdx << " | ";
          // for (size_t i : ids)
          // {
          //     std::cout << i << " ";
          // }
          // std::cout << std::endl;

          // fs[myIdx].Print();
#endif
              fs[myIdx].SecretShare(myIdx, ids);
            });
      } else // get secret share from parties
      {
        threads.emplace_back([this, i]() {
          std::set<size_t> ids;
          for (size_t j = 0; j < nParties; j++) {
            if (j != i || j != lIdx)
              ids.insert(j);
          }
          fs[i] = AShare(shareinfo.numBins, pcol[i]);
          fs[i].SecretShare(i, ids);
        });
      }
    }

    for (auto &thread : threads) {
      thread.join();
    }

    // merge fs
    u64 rows = shareinfo.numBins;
    Mat2d tmpmerge(rows, col_sum + 1);
    tmpmerge.setZero();
    // std::cout << rows << " " << col_sum + 1 << std::endl;
    u64 colOffset = 0;
    for (size_t i = 0; i < nParties; ++i) {
      u64 curCol = fs[i].cols();
      Matrix<u64> tmp = fs[i].getShare();
      // std::cout << i << " " << tmp.rows() << " " << tmp.cols() << std::endl;
      for (u64 r = 0; r < rows; ++r) {
        for (u64 c = 0; c < curCol; ++c) {
          tmpmerge(r, colOffset + c) = tmp(r, c);
        }
      }
      colOffset += curCol;
    }
    // std::cout << "finish merge" << std::endl;

    AddShare mergef(tmpmerge);
    AddShare result(rows, col_sum + 1);
    std::vector<Mat2d> mask(nParties);
    std::vector<Mat2d> shumask1(nParties);
    std::vector<Mat2d> shumask2(nParties);

    mergef.ShuffleOFFTrick(mask, shumask1, shumask2); // fake offline
    // std::cout << "finish trick offline" << std::endl;
    mergef.Shuffle(result, mask, shumask1, shumask2, 1234 + myIdx);

    // // fake id rows
    // AddShare data1(rows, 1);
    // data1.data.setZero();
    // data1.Reveal();

    auto done = std::chrono::system_clock::now();
    // std::cout << myIdx << " Normal done: " <<
    // std::chrono::duration_cast<std::chrono::milliseconds>(done -
    // start).count() << "ms" << std::endl; // test right
    return std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
        .count();
  }

  // void mulCircuitPSI() override
  // {
  //   // std::cout << "mul psi" << std::endl;
  //   volePSI::RsMulpsiSender sender;
  //   sender.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);

  //   std::cout << "Sender start\n";
  //   auto start = std::chrono::system_clock::now();
  //   // Run psi protocol.
  //   // RsMulpsiSender::Share sss;
  //   ss.mValues = new oc::Matrix<u8>[nParties];
  //   // macoro::sync_wait(sender.send(mset, mvalues, ss, psocks[lIdx]));

  //   auto done = std::chrono::system_clock::now();
  //   std::cout << "Sender done, "
  //             << std::chrono::duration_cast<std::chrono::milliseconds>(done -
  //                                                                      start)
  //                    .count()
  //             << "ms" << std::endl; // test right
  //                                   // ss.GenflagShare(flagshare);
  //                                   // ss.print();
  //                                   // flagshare.RevealPrint();
  //                                   // ss.print();
  //                                   // zero
  //   // macoro::sync_wait(sender.isZero(sss, psocks[lIdx]));
  // }

  void testPSI() override { std::cout << "test" << std::endl; }
};
} // namespace volePSI

#endif