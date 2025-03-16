#include "PartyL.h"
#include "coproto/Socket/AsioSocket.h"
#include "volePSI/fileBased.h"
#include <set>
#include <string>
#include <bitset>
#include "MPC/BlongShare.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include <map>

using namespace std;
// #include <algorithm>

namespace volePSI
{
    void LeaderParty::printValues()
    {
        std::cout << "values " << mvalues.size() << " " << mvalues.rows() << " " << mvalues.cols() << std::endl;
        for (size_t i = 0; i < mvalues.rows(); i++)
        {
            for (size_t j = 0; j < mvalues.cols(); j++)
            {
                std::cout << mvalues(i, j) << " ";
            }
            std::cout << " | ";
        }
        std::cout << std::endl;
    }

    void LeaderParty::getFakeData()
    {
        Party::getFakeData();

        // std::vector<u64> temp;
        for (size_t i = 0; i < setSize; ++i)
        {
            for (size_t j = 0; j < pcol[myIdx]; ++j)
            {
                vv.push_back(((i + 1) * 100 + (j + 1)));
                // mvalues(i, j) = i * 100 + j;
                // valuearray[i * ccols + j] = (i * 100 + j);
            }
        }
    }

    void LeaderParty::printHash()
    {
        assert(Tx.size() > 0);
        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            std::cout << i << ":" << Tx[i];
            auto &bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto b = bin.idx();
                std::cout << " " << mset[b];
            }
            std::cout << std::endl;
        }
    }

    void LeaderParty::HashingInputID()
    {
        this->params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);
        // MulPSILeader lp;
        lp.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);
        // auto start = std::chrono::system_clock::now();
        macoro::sync_wait(lp.cuckooSeedSync(cuckooSeed, psocks, myIdx));
        // std::vector<block> Tx;
        cuckoo.init(setSize, ssp, 0, 3);
        cuckoo.insert(mset, cuckooSeed);
        Tx.resize(cuckoo.mNumBins);

        shareinfo.keyBitLength = ssp + oc::log2ceil(Tx.size());
        // shareinfo.keyByteLength = oc::divCeil(shareinfo.keyBitLength, 8);
        shareinfo.keyByteLength = sizeof(u64);
        shareinfo.numBins = cuckoo.mBins.size();
        // std::cout << "Info: numBins:" << shareinfo.numBins << std::endl;
#ifdef DEBUG_INFO
        std::cout << "Info: numBins:" << shareinfo.numBins << " |mSenderSize:" << setSize << " |keyBitLength:" << shareinfo.keyBitLength << " |keyByteLength:" << shareinfo.keyByteLength << " |Ysize:" << mset.size() << " |valuesize:" << mvalues.size() << std::endl;
#endif

        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);
        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // ss.mValues = new oc::Matrix<u8>[nParties];
        // ss.mFlagBits = new oc::BitVector[nParties];
        // ss.mFlags.reset(new oc::Matrix<u8>[nParties]);
        // ss.mFlags = new oc::Matrix<u8>[nParties];

        mMapping.resize(mset.size(), ~u64(-1));
        mMapRev.resize(shareinfo.numBins, ~u64(-1));
        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            auto &bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                // j = oc::CuckooIndex<>::minCollidingHashIdx(i, cuckoo.mHashes[b], 3, shareinfo.numBins);

                auto &hj = hashers[j];
                Tx[i] = hj.hashBlock(mset[b]);
                // std::cout << i << " ";
                mMapping[b] = i; // x_index --> coockie_hashbin_index
                mMapRev[i] = b;
            }
            else
            {
                Tx[i] = block(i, 0);
            }
        }
#ifdef DEBUG_INFO
        // std::cout << "map" << std::endl;
        // for (size_t i = 0; i < mMapping.size(); i++)
        // {
        //     std::cout << mMapping[i] << " ";
        // }
        // std::cout << std::endl;
#endif
    }

    void LeaderParty::HashingInputF()
    {
        // printValues();
        this->params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);
        // MulPSILeader lp;
        lp.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);
        // auto start = std::chrono::system_clock::now();
        macoro::sync_wait(lp.cuckooSeedSync(cuckooSeed, psocks, myIdx));
        // std::vector<block> Tx;
        oc::CuckooIndex<> cuckoo;
        cuckoo.init(setSize, ssp, 0, 3);
        cuckoo.insert(mset, cuckooSeed);
        Tx.resize(cuckoo.mNumBins);

        shareinfo.keyBitLength = ssp + oc::log2ceil(Tx.size());
        // shareinfo.keyByteLength = oc::divCeil(shareinfo.keyBitLength, 8);
        shareinfo.keyByteLength = sizeof(u64);
        shareinfo.numBins = cuckoo.mBins.size();
        Tf.resize(shareinfo.numBins, cols);
#ifdef DEBUG_INFO
        // std::cout << "Info: numBins:" << shareinfo.numBins << " |mSenderSize:" << setSize << " |keyBitLength:" << shareinfo.keyBitLength << " |keyByteLength:" << shareinfo.keyByteLength << " |Ysize:" << mset.size() << " |valuesize:" << mvalues.size() << std::endl;
#endif
        std::array<oc::AES, 3> hashers;
        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);
        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // ss.mValues = new oc::Matrix<u8>[nParties];
        // ss.mFlagBits = new oc::BitVector[nParties];
        // ss.mFlags.reset(new oc::Matrix<u8>[nParties]);
        // ss.mFlags = new oc::Matrix<u8>[nParties];

        mMapping.resize(mset.size(), ~u64(-1));
        mMapRev.resize(shareinfo.numBins, ~u64(-1));
        // printValues();

        // std::cout << "values " << mvalues.size() << " " << mvalues.rows() << " " << mvalues.cols() << std::endl;
        // for (size_t i = 0; i < mvalues.rows(); i++)
        // {
        //     for (size_t j = 0; j < mvalues.cols(); j++)
        //     {
        //         std::cout << mvalues(i, j) << " ";
        //     }
        //     std::cout << " | ";
        // }
        // std::cout << std::endl;
        mvalues = MatrixView<u64>(vv.begin(), vv.end(), cols);
#ifdef DEBUG_INFO
        // printValues();
#endif

        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            auto &bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                // j = oc::CuckooIndex<>::minCollidingHashIdx(i, cuckoo.mHashes[b], 3, shareinfo.numBins);

                auto &hj = hashers[j];
                Tx[i] = hj.hashBlock(mset[b]);
                for (size_t j = 0; j < cols; ++j)
                {
                    Tf(i, j) = mvalues(b, j);
                    // std::cout << Tf(i, j) << " " << i << " " << j << " " << b << " " << mvalues(b, j) << " y \n ";
                }

                // Tf[i] = mvalues[b];
                // std::cout << i << " ";
                mMapping[b] = i; // x_index --> coockie_hashbin_index
                mMapRev[i] = b;
            }
            else
            {
                // for (size_t j = 0; j < pcol[myIdx]; ++j)
                // {
                //     Tf(i, j) = i + j;
                //     std::cout << Tf(i, j) << "n \n";
                // }
                Tx[i] = block(i, 0);
            }
        }
#ifdef DEBUG_INFO
        // std::cout << "map" << std::endl;
        // for (size_t i = 0; i < mMapping.size(); i++)
        // {
        //     std::cout << mMapping[i] << " ";
        // }
        // std::cout << std::endl;
#endif
    }

    void LeaderParty::circuitPSI()
    {
        volePSI::RsMulpsiReceiver recevier;
        recevier.init(setSize, setSize, cols, ssp, ZeroBlock, numThread); // 40 is the statistical param

        std::cout << "Recver start\n";
        auto start = std::chrono::system_clock::now();

        // RsMulpsiReceiver::Sharing rss;
        // Run the protocol.
        // macoro::sync_wait(recevier.receive(mset, rss, psocks));
        // std::cout << psocks.size() << std::endl;
        std::vector<RsMulpsiReceiver::Sharing> rss(nParties - 1);
        for (size_t i = 0; i < psocks.size(); ++i)
        {
            macoro::sync_wait(recevier.receive(mset, rss[i], psocks[i]));
            rss[i].print();
        }
        auto done = std::chrono::system_clock::now();
        std::cout << "Sender done, " << std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count() << "ms" << std::endl;
    }

    void LeaderParty::initShare(std::vector<block> &Tx)
    {
        oc::CuckooIndex<> cuckoo;
        cuckoo.init(setSize, ssp, 0, 3);

        cuckoo.insert(mset, cuckooSeed);
        Tx.resize(cuckoo.mNumBins);
        std::array<oc::AES, 3> hashers;

        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);
        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        ss.keyBitLength = ssp + oc::log2ceil(Tx.size());
        ss.keyByteLength = oc::divCeil(ss.keyBitLength, 8);

        // ss.nParties = nParties;
        ss.numBins = cuckoo.mBins.size();

        ss.mValues = new oc::Matrix<u8>[nParties];
        // ss.mFlagBits = new oc::BitVector[nParties];
        // ss.mFlags.reset(new oc::Matrix<u8>[nParties]);
        ss.mFlags = new oc::Matrix<u8>[nParties];

        mMapping.resize(mset.size(), ~u64(-1));
        mMapRev.resize(ss.numBins, ~u64(-1));
        for (u64 i = 0; i < ss.numBins; ++i)
        {
            auto &bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                // j = oc::CuckooIndex<>::minCollidingHashIdx(i, cuckoo.mHashes[b], 3, ss.numBins);

                auto &hj = hashers[j];
                Tx[i] = hj.hashBlock(mset[b]);
                std::cout << i << " ";
                mMapping[b] = i; // x_index --> coockie_hashbin_index
                mMapRev[i] = b;
            }
            else
            {

                Tx[i] = block(i, 0);
            }
        }

        std::cout << "map" << std::endl;
        for (size_t i = 0; i < mMapping.size(); i++)
        {
            std::cout << mMapping[i] << " ";
        }
        std::cout << std::endl;
        mapValue();
        // mMapping.resize(mset.size(), ~u64(0));

        // u64 keyBitLength = ssp + oc::log2ceil(Tx.size());
        // u64 keyByteLength = oc::divCeil(keyBitLength, 8);
        // ret.mFlag.resize(ret.numBins, keyByteLength, oc::AllocType::Zeroed);
        // std::memset(ret.mFlag.data(), 0, ret.mFlag.size() * sizeof(u8));
    }

    void LeaderParty::mapValue()
    {
        size_t col = mvalues.cols();
        ss.mValues[myIdx].resize(ss.numBins, col);
        u64 p;
        for (u64 i = 0; i < mMapping.size(); ++i)
        {
            // std::cout << mMapRev[i] << " ";
            p = mMapping[i];
            for (u64 j = 0; j < col; ++j)
            {
                ss.mValues[myIdx](p, j) = mvalues(i, j);
            }
            // else
            // {
            //     std::cout << i << " ";
            // for (u64 j = 0; j < col; ++j)
            // {
            //     ss.mValues[myIdx](i, j) = rand;
            // }
            // }
        }
    }
}