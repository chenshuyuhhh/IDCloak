#include "PartyN.h"
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
    void NormalParty::printValues()
    {
        std::cout << "values " << mvalues.size() << " " << mvalues.rows() << " " << mvalues.cols() << std::endl;
        for (size_t i = 0; i < mvalues.rows(); i++)
        {
            for (size_t j = 0; j < mvalues.cols(); j++)
            {
                std::cout << +mvalues(i, j) << " ";
            }
            std::cout << " | ";
        }
        std::cout << std::endl;
    }

    void NormalParty::getFakeData()
    {
        Party::getFakeData();

        // std::vector<u8> temp;
        // u64 ccols = cols * 8;
        // valuearray = new u8[setSize * ccols];
        // mvalues.resize(setSize, cols * 8);
        // mvalues = MatrixView(setSize * vcols);
        // mvalues.reshape(setSize, cols * 8);
        for (size_t i = 0; i < setSize; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                vv.push_back(((i + 1) * 100 + j + 1));
                // mvalues(i, j) = i * 100 + j;
                // valuearray[i * ccols + j] = (i * 100 + j);
            }
        }

        // std::cout << "values " << mvalues.size() << " " << mvalues.rows() << " " << mvalues.cols() << std::endl;
        // for (size_t i = 0; i < mvalues.rows(); i++)
        // {
        //     for (size_t j = 0; j < mvalues.cols(); j++)
        //     {
        //         std::cout << +mvalues(i, j) << " ";
        //     }
        //     std::cout << " | ";
        // }
        // std::cout << std::endl;
    }

    void NormalParty::printHash()
    {
        assert(Tx.size() > 0);
        u64 k = 0;
        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];
            cout << i << "," << size << ":" << std::endl;
            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto &hj = hashers[j];
                auto b = bin[p].idx();
                // block x = hj.hashBlock(mset[b]);
                std::cout << Tx[k] << " " << mset[b] << std::endl;
                ++k;
            }
        }
    }

    void NormalParty::HashingInputID()
    {
#ifdef DEBUG_INFO
        // std::cout << "Hashing input\n";
#endif
        // MulPSINormal ln;
        ln.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);
        // auto start = std::chrono::system_clock::now();
        macoro::sync_wait(ln.cuckooSeedSync(cuckooSeed, psocks[lIdx]));
        // std::cout << cuckooSeed << std::endl;
        params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);
        shareinfo.numBins = params.numBins();
        sIdx.init(shareinfo.numBins, setSize, ssp, 3);
        sIdx.insertItems(mset, cuckooSeed);

        // mssp=40
        shareinfo.keyBitLength = ssp + oc::log2ceil(params.numBins()); // oc::log2ceil == bitwidth  y||j
        // shareinfo.keyByteLength = oc::divCeil(shareinfo.keyBitLength, 8); // keyBitLength/8
        shareinfo.keyByteLength = sizeof(u64);

#ifdef DEBUG_INFO
        // numBins = mSenderSize = Y/X.size  = 1<<nn
        // std::cout << "Info: numBins:" << shareinfo.numBins << " |mSenderSize:" << setSize << " |keyBitLength:" << shareinfo.keyBitLength << " |keyByteLength:" << shareinfo.keyByteLength << " |Ysize:" << mset.size() << " |valuesize:" << sizeof(mvalues) << std::endl;
#endif

        // std::array<oc::AES, 3> hashers;
        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);

        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // The OPPRF input value of the i'th input under the j'th cuckoo
        // hash function.
        Tx.resize(mset.size() * 3);

        // The value associated with the k'th OPPRF input
        Tv.resize(mset.size() * 3, shareinfo.keyByteLength, oc::AllocType::Uninitialized);

        // The special value assigned to the i'th bin.
        mFlag.resize(shareinfo.numBins, shareinfo.keyByteLength, oc::AllocType::Uninitialized);
        pPrng.get<u8>(mFlag);

        mvalues = MatrixView<u64>(vv.begin(), vv.end(), cols);
#ifdef DEBUG_INFO
        // printValues();
#endif

        std::vector<block>::iterator TxIter = Tx.begin();
        Matrix<u8>::iterator TvIter = Tv.begin();
        Matrix<u8>::iterator rIter = mFlag.begin();
        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];

            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto &hj = hashers[j];
                auto b = bin[p].idx();
                *TxIter = hj.hashBlock(mset[b]); // H(x) = AES(x) + x.

                // ret.mMapping[b][mapnum[b]] = i;
                // mapnum[b]++;

                memcpy(&*TvIter, &*rIter, shareinfo.keyByteLength);
                TvIter += shareinfo.keyByteLength; // r||value
                                                   // std::cout << p << " " << b << " " << *TvIter << " ";
                ++TxIter;
            }
            rIter += shareinfo.keyByteLength;
        }

        while (TxIter != Tx.end())
        {
            *TxIter = pPrng.get();
            ++TxIter;
        }
    }

    void NormalParty::HashingInputF()
    {
#ifdef DEBUG_INFO
        // std::cout << "Hashing input\n";
#endif
        // MulPSINormal ln;
        ln.init(setSize, setSize, cols, ssp, ZeroBlock, numThread);
        // auto start = std::chrono::system_clock::now();
        macoro::sync_wait(ln.cuckooSeedSync(cuckooSeed, psocks[lIdx]));
        // std::cout << cuckooSeed << std::endl;
        params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);
        shareinfo.numBins = params.numBins();
        SimpleIndex sIdx;
        sIdx.init(shareinfo.numBins, setSize, ssp, 3);
        sIdx.insertItems(mset, cuckooSeed);

        // mssp=40
        shareinfo.keyBitLength = ssp + oc::log2ceil(params.numBins()); // oc::log2ceil == bitwidth  y||j
        // shareinfo.keyByteLength = oc::divCeil(shareinfo.keyBitLength, 8); // keyBitLength/8
        shareinfo.keyByteLength = sizeof(u64);

#ifdef DEBUG_INFO
        // numBins = mSenderSize = Y/X.size  = 1<<nn
        // std::cout << "Info: numBins:" << shareinfo.numBins << " |mSenderSize:" << setSize << " |keyBitLength:" << shareinfo.keyBitLength << " |keyByteLength:" << shareinfo.keyByteLength << " |Ysize:" << mset.size() << " |valuesize:" << sizeof(mvalues) << std::endl;
#endif

        std::array<oc::AES, 3> hashers;
        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);

        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // The OPPRF input value of the i'th input under the j'th cuckoo
        // hash function.
        Tx.resize(mset.size() * 3);

        Tf.resize(mset.size() * 3, vcols);

        mvalues = MatrixView<u64>(vv.begin(), vv.end(), cols);

        // The special value assigned to the i'th bin.
        mFlag.resize(shareinfo.numBins, vcols, oc::AllocType::Uninitialized);
        pPrng.get<u8>(mFlag);

        // ret.print();

        rs.resize(shareinfo.numBins, vcols, oc::AllocType::Uninitialized);
        pPrng.get<u8>(rs); // mask v, every bin has the same r

        std::vector<block>::iterator TxIter = Tx.begin();
        Matrix<u8>::iterator fIter = Tf.begin();
        Matrix<u8>::iterator rIter = mFlag.begin();
        for (u64 i = 0; i < shareinfo.numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];

            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto &hj = hashers[j];
                auto b = bin[p].idx();
                *TxIter = hj.hashBlock(mset[b]); // H(x) = AES(x) + x.

                // ret.mMapping[b][mapnum[b]] = i;
                // mapnum[b]++;

                // assert(vcols == cols * 8);
                memcpy(&*fIter, &mvalues(b, 0), cols);
                // std::cout << vcols << " " << cols << std::endl;

                // assert(values.cols() % sizeof(u64) == 0);
                // auto ss = vcols / sizeof(u64);
                auto tv = reinterpret_cast<volePSI::u64*>(&(*fIter));
                auto rr = (u64 *)&rs(i, 0);
                for (u64 k = 0; k < cols; ++k)
                {
                    // for(size_t j = 0;j<8;++j){
                    //      =
                    // }
                    tv[k] -= rr[k];
                    // std::cout << tv[k] << " ";
                }

                fIter += vcols;
                ++TxIter;
            }
            rIter += shareinfo.keyByteLength;
        }

        while (TxIter != Tx.end())
        {
            *TxIter = pPrng.get();
            ++TxIter;
        }

        // printValues();
        // // tf + rs = v
        // std::cout << "tf " << Tf.size() << " " << Tf.rows() << " " << Tf.cols() << std::endl;
        // for (size_t i = 0; i < Tf.rows(); i++)
        // {
        //     for (size_t j = 0; j < Tf.cols(); j++)
        //     {
        //         std::cout << +Tf(i, j) << " ";
        //     }
        //     std::cout << " | ";
        // }
        // std::cout << std::endl;
        // std::cout << "rs " << rs.size() << " " << rs.rows() << " " << rs.cols() << std::endl;
        // for (size_t i = 0; i < rs.rows(); i++)
        // {
        //     for (size_t j = 0; j < rs.cols(); j++)
        //     {
        //         std::cout << +rs(i, j) << " ";
        //     }
        //     std::cout << " | ";
        // }
        // std::cout << std::endl;
    }

}