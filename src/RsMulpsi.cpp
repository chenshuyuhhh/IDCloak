#include "RsMulpsi.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <bitset>

using namespace std;

namespace volePSI
{
    extern size_t nParties;
    extern size_t myIdx;
    void Share::print()
    {
        std::cout << "Result info:" << std::endl;
        std::cout << "values" << std::endl;
        for (size_t i = 0; i < nParties; i++)
        {
            std::cout << i << " " << mValues[i].size() << ": ";
            for (size_t j = 0; j < mValues[i].rows(); j++)
            {
                for (size_t k = 0; k < mValues[i].cols(); k++)
                {
                    std::cout << +mValues[i](j, k) << " ";
                }
                std::cout << " | ";
            }
            std::cout << std::endl;
        }

        std::cout << "map" << std::endl;
        if (mMapping.size() > 0)
        {
            for (size_t i = 0; i < mMapping.size(); i++)
            {
                std::cout << mMapping[i][0] << " " << mMapping[i][1] << " " << mMapping[i][2] << " ";
            }
        }
        std::cout << std::endl;
        //           << "flag " << std::endl;
        // flagshare.RevealPrint();

        // for (size_t i = 0; i < mLongFlag.size(); i++)
        // {
        //     // std::cout << i << ":";
        //     // for (size_t k = 0; k < mFlag.cols(); k++)
        //     // {
        //     std::cout << mLongFlag(i) << " ";
        //     // }
        //     // std::cout << std::endl;
        // }

        // if (mFlags != nullptr)
        // {
        //     std::cout << "flags" << std::endl;
        //     for (size_t i = 0; i < nParties; i++)
        //     {
        //         std::cout << i << ": ";
        //         for (int j = 0; j < mFlags[i].rows(); j++)
        //         {
        //             for (int k = 0; k < mFlags[i].cols(); k++)
        //             {
        //                 std::cout << +mFlags[i](j, k) << " ";
        //             }
        //             std::cout << std::endl;
        //         }
        //         std::cout << std::endl;
        //     }
        // }
    }

    // void Share::mergeFlag()
    // {
    //     mFlag.resize(numBins, keyByteLength, oc::AllocType::Zeroed);
    //     // std::cout << mFlag.size() << endl;
    //     for (size_t i = 0; i < nParties; i++)
    //     {
    //         if (i != myIdx)
    //         {
    //             // std::cout << i << ": ";
    //             for (int j = 0; j < mFlags[i].size(); j++)
    //             {
    //                 mFlag(j) -= mFlags[i](j);
    //             }
    //             // std::cout << std::endl;
    //         }
    //     }
    // }

    void  getMapping(SimpleIndex &sidx, std::vector<std::array<u64, 3>> &mMapping)
    {
        // assert(mMapping.size == itemSize);
        // int mapnum[itemSize];
        // for (int i = 0; i < itemSize; ++i)
        // {
        //     mapnum[i] = 0;
        // }
        u64 itemSize = mMapping.size();
        std::vector<int> mapnum(itemSize, 0);
        for (u64 i = 0; i < sidx.mNumBins; ++i)
        {
            auto bin = sidx.mBins[i];
            auto size = sidx.mBinSizes[i];

            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto b = bin[p].idx();
                mMapping[b][mapnum[b]] = i;
                mapnum[b]++;
            }
        }
    }

    void RsMulpsiSender::Sharing::print()
    {
        std::cout << "Send Result info:" << std::endl;
        std::cout << "values" << std::endl;
        for (size_t i = 0; i < mValues.size(); i++)
        {
            std::cout << +mValues(i) << " ";
        }

        std::cout << std::endl
                  << "map" << std::endl;
        for (size_t i = 0; i < mMapping.size(); i++)
        {
            std::cout << mMapping[i][0] << " " << mMapping[i][1] << " " << mMapping[i][2] << " ";
        }

        std::cout << std::endl
                  << "flag" << std::endl;

        for (size_t i = 0; i < mFlag.size(); i++)
        {
            std::cout << +mFlag(i) << " ";
        }
        std::cout << std::endl;
    }

    Proto RsMulpsiSender::send(span<block> Y, const oc::MatrixView<u8> values, Sharing &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, Y, values, &ret, &chl,
                 cuckooSeed = block{},
                 params = oc::CuckooParam{},
                 //  numBins = u64{},
                 sIdx = SimpleIndex{},
                 //  keyBitLength = u64{},
                 keyByteLength = u64{},
                 hashers = std::array<oc::AES, 3>{},
                 Ty = std::vector<block>{},
                 Tv = Matrix<u8>{},
                 r = Matrix<u8>{},
                 TyIter = std::vector<block>::iterator{},
                 TvIter = Matrix<u8>::iterator{},
                 rIter = Matrix<u8>::iterator{},
                 opprf = std::make_unique<RsOpprfSender>());

        setTimePoint("RsCpsiSender::send begin");
        if (mRecverSize != Y.size() || mValueByteLength != values.cols())
            throw RTE_LOC;

        // cuckooSeed = mPrng.get();

        MC_AWAIT(chl.recv(cuckooSeed));
        // std::cout << cuckooSeed << std::end;

        setTimePoint("RsCpsiSender::send recv");

        // stash size is 0
        params = oc::CuckooIndex<>::selectParams(mRecverSize, mSsp, 0, 3);
        ret.numBins = params.numBins();
        sIdx.init(ret.numBins, mSenderSize, mSsp, 3);
        sIdx.insertItems(Y, cuckooSeed);
        ret.mMapping.resize(Y.size());
        getMapping(sIdx,ret.mMapping);

        setTimePoint("RsCpsiSender::send simpleHash");

        ret.keyBitLength = mSsp + oc::log2ceil(params.numBins()); // oc::log2ceil == bitwidth  y||j
        keyByteLength = oc::divCeil(ret.keyBitLength, 8);         // keyBitLength/8

        // numBins = mSenderSize = Y/X.size  = 1<<nn
        // std::cout << "Info: numBins" << ret.numBins << " mSenderSize" << mSenderSize << " keyBitLength" << ret.keyBitLength << " keyByteLength" << keyByteLength << " Ysize" << Y.size() << " valuesize" << values.size() << std::endl;

        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);

        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // The OPPRF input value of the i'th input under the j'th cuckoo
        // hash function.
        Ty.resize(Y.size() * 3);

        // The value associated with the k'th OPPRF input
        Tv.resize(Y.size() * 3, keyByteLength + values.cols(), oc::AllocType::Uninitialized);

        // The special value assigned to the i'th bin.
        ret.mFlag.resize(ret.numBins, keyByteLength, oc::AllocType::Uninitialized);
        mPrng.get<u8>(ret.mFlag);

        // std::cout << "values" << values.size() << std::endl;
        if (values.size())
        {
            ret.mValues.resize(ret.numBins, values.cols(), oc::AllocType::Uninitialized);
            mPrng.get<u8>(ret.mValues); // mask v, every bin has the same r
        }

        TyIter = Ty.begin();
        TvIter = Tv.begin();
        rIter = ret.mFlag.begin();
        for (u64 i = 0; i < ret.numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];

            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto &hj = hashers[j];
                auto b = bin[p].idx();
                *TyIter = hj.hashBlock(Y[b]); // H(x) = AES(x) + x.

                // ret.mMapping[b][mapnum[b]] = i;
                // mapnum[b]++;

                memcpy(&*TvIter, &*rIter, keyByteLength);
                TvIter += keyByteLength; // r||value
                // std::cout << p << " " << b << " " << *TvIter << " ";

                if (values.size())
                {
                    memcpy(&*TvIter, &values(b, 0), values.cols());

                    if (mType == ValueShareType::Xor)
                    {
                        for (u64 k = 0; k < values.cols(); ++k)
                        {
                            TvIter[k] ^= ret.mValues(i, k);
                            // std::cout << "b" << b << " " << +values(b, 0) << " " << +ret.mValues(i, k) << " " << +TvIter[k] << endl;
                        }
                    }
                    else if (mType == ValueShareType::add32)
                    {
                        assert(values.cols() % sizeof(u32) == 0);
                        auto ss = values.cols() / sizeof(u32);
                        auto tv = reinterpret_cast<volePSI::u32*>(&(*TvIter));
                        auto rr = (u32 *)&ret.mValues(i, 0);
                        for (u64 k = 0; k < ss; ++k)
                            tv[k] -= rr[k];
                    }
                    else
                        throw RTE_LOC;
                    TvIter += values.cols();
                }

                ++TyIter;
            }
            rIter += keyByteLength;
        }

        while (TyIter != Ty.end())
        {
            *TyIter = mPrng.get();
            ++TyIter;
        }
        // above: Ty simple hash

        setTimePoint("RsCpsiSender::send setValues");

        if (mTimer)
            opprf->setTimer(*mTimer);

        // for (size_t i = 0; i < ret.mFlag.size(); i++)
        // {
        //     std::cout << +(ret.mFlag(i)) << " ";
        // }
        // std::cout << " send" << std::endl;

        MC_AWAIT(opprf->send(ret.numBins, Ty, Tv, mPrng, mNumThreads, chl));

        // for (size_t i = 0; i < Tv.size(); i++)
        // {
        //     std::cout << +(Tv(i)) << " ";
        // }
        // std::cout << " send" << Tv.size() << std::endl;

        MC_END();
    }

    Proto RsMulpsiSender::isZero(Sharing &ret, Socket &chl)
    {
        // MC_BEGIN(Proto, this, &chl, &ret, cmp = std::make_unique<Gmw>(), cir = BetaCircuit{});
        // cir = isZeroCircuit(ret.keyBitLength); // bits
        // cmp->init(ret.mFlag.rows(), cir, mNumThreads, 1, mPrng.get());

        // cmp->setInput(0, ret.mFlag);
        // MC_AWAIT(cmp->run(chl));

        // {

        //     auto ss = cmp->getOutputView(0);
        //     ret.mFlagBits.resize(ret.numBins);
        //     std::copy(ss.begin(), ss.begin() + ret.mFlagBits.sizeBytes(), ret.mFlagBits.data());

        //     // for (size_t i = 0; i < ss.size(); ++i)
        //     // {
        //     //     std::cout << ss(i) << " ";
        //     // }
        //     // std::cout << " send end" << std::endl;
        //     for (u64 i = 0; i < ret.mFlagBits.size(); ++i)
        //     {
        //         cout << ret.mFlagBits[i] << " ";
        //     }
        //     cout << endl;
        // }

        // MC_END();
    }

    Proto RsMulpsiSender::send(span<block> Y, const oc::MatrixView<u8> values, Share &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, Y, values, &ret, &chl,
                 cuckooSeed = block{},
                 params = oc::CuckooParam{},
                 //  mFlag = oc::Matrix<u8>{},
                 //  numBins = u64{},
                 sIdx = SimpleIndex{},
                 //  keyBitLength = u64{},
                 //  keyByteLength = u64{},
                 hashers = std::array<oc::AES, 3>{},
                 Ty = std::vector<block>{},
                 Tv = Matrix<u8>{},
                 r = Matrix<u8>{},
                 TyIter = std::vector<block>::iterator{},
                 TvIter = Matrix<u8>::iterator{},
                 rIter = Matrix<u8>::iterator{},
                 opprf = std::make_unique<RsOpprfSender>(),
                 cmp = std::make_unique<Gmw>(),
                 cir = BetaCircuit{});

        setTimePoint("RsCpsiSender::send begin");
        if (mRecverSize != Y.size() || mValueByteLength != values.cols())
            throw RTE_LOC;

        // cuckooSeed = mPrng.get();

        MC_AWAIT(chl.recv(cuckooSeed));
        // std::cout << cuckooSeed << std::end;

        setTimePoint("RsCpsiSender::send recv");

        // stash size is 0
        params = oc::CuckooIndex<>::selectParams(mRecverSize, mSsp, 0, 3);
        ret.numBins = params.numBins();
        sIdx.init(ret.numBins, mSenderSize, mSsp, 3);
        sIdx.insertItems(Y, cuckooSeed);
        ret.mMapping.resize(Y.size());
        getMapping(sIdx,ret.mMapping);

        setTimePoint("RsCpsiSender::send simpleHash");

        // mssp=40
        ret.keyBitLength = mSsp + oc::log2ceil(params.numBins()); // oc::log2ceil == bitwidth  y||j
        ret.keyByteLength = oc::divCeil(ret.keyBitLength, 8);     // keyBitLength/8

        // numBins = mSenderSize = Y/X.size  = 1<<nn
        // std::cout << "Info: numBins:" << ret.numBins << " |mSenderSize:" << mSenderSize << " |keyBitLength:" << ret.keyBitLength << " |keyByteLength:" << ret.keyByteLength << " |Ysize:" << Y.size() << " |valuesize:" << values.size() << std::endl;

        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);

        // std::cout << "hash" << hashers[1].getKey() << hashers[1].getKey() << hashers[2].getKey() << std::endl;

        // The OPPRF input value of the i'th input under the j'th cuckoo
        // hash function.
        Ty.resize(Y.size() * 3);

        // The value associated with the k'th OPPRF input
        Tv.resize(Y.size() * 3, ret.keyByteLength + values.cols(), oc::AllocType::Uninitialized);

        // The special value assigned to the i'th bin.
        ret.mFlag.resize(ret.numBins, ret.keyByteLength, oc::AllocType::Uninitialized);
        mPrng.get<u8>(ret.mFlag);

        if (values.size())
        {
            ret.mValues[myIdx].resize(ret.numBins, values.cols(), oc::AllocType::Uninitialized);
            mPrng.get<u8>(ret.mValues[myIdx]); // mask v, every bin has the same r
            // std::cout << "values" << values[myIdx].size() << " " << +values[myIdx][0] << std::endl;
        }

        // ret.print();

        TyIter = Ty.begin();
        TvIter = Tv.begin();
        rIter = ret.mFlag.begin();
        for (u64 i = 0; i < ret.numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];

            for (u64 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto &hj = hashers[j];
                auto b = bin[p].idx();
                *TyIter = hj.hashBlock(Y[b]); // H(x) = AES(x) + x.

                // ret.mMapping[b][mapnum[b]] = i;
                // mapnum[b]++;

                memcpy(&*TvIter, &*rIter, ret.keyByteLength);
                TvIter += ret.keyByteLength; // r||value
                // std::cout << p << " " << b << " " << *TvIter << " ";

                if (values.size())
                {
                    memcpy(&*TvIter, &values(b, 0), values.cols());

                    if (mType == ValueShareType::Xor)
                    {
                        for (u64 k = 0; k < values.cols(); ++k)
                        {
                            TvIter[k] ^= ret.mValues[myIdx](i, k);
                            // std::cout << "b" << b << " " << +values(b, 0) << " " << +ret.mValues(i, k) << " " << +TvIter[k] << endl;
                        }
                    }
                    else if (mType == ValueShareType::add32)
                    {
                        assert(values.cols() % sizeof(u32) == 0);
                        auto ss = values.cols() / sizeof(u32);
                        auto tv = reinterpret_cast<volePSI::u32*>(&(*TvIter));
                        auto rr = (u32 *)&ret.mValues[myIdx](i, 0);
                        for (u64 k = 0; k < ss; ++k)
                            tv[k] -= rr[k];
                    }
                    else
                        throw RTE_LOC;
                    TvIter += values.cols();
                }

                ++TyIter;
            }
            rIter += ret.keyByteLength;
        }

        while (TyIter != Ty.end())
        {
            *TyIter = mPrng.get();
            ++TyIter;
        }
        // above: Ty simple hash

        setTimePoint("RsCpsiSender::send setValues");

        if (mTimer)
            opprf->setTimer(*mTimer);

        // for (size_t i = 0; i < Ty.size(); i++)
        // {
        //     std::cout << Ty[i] << " ";
        // }
        // std::cout << std::endl;
        // for (size_t i = 0; i < Tv.size(); i++)
        // {
        //     std::cout << +(Tv(i)) << " ";
        // }
        // std::cout << std::endl;
        MC_AWAIT(opprf->send(ret.numBins, Ty, Tv, mPrng, mNumThreads, chl));

        // cir = isZeroCircuit(ret.keyBitLength); // bits
        // cmp->init(mFlag.rows(), cir, mNumThreads, 1, mPrng.get());

        // cmp->setInput(0, mFlag);
        // MC_AWAIT(cmp->run(chl));

        // {
        //     auto ss = cmp->getOutputView(0);
        //     ret.mFlagBit.resize(ret.numBins);
        //     std::copy(ss.begin(), ss.begin() + ret.mFlagBit.sizeBytes(), ret.mFlagBit.data());
        // }

        MC_END();
    }

    /**
     * RsMulpsiReceiver
     */

    Proto RsMulpsiReceiver::receive(span<block> X, Sharing &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, X, &ret, &chl,
                 cuckooSeed = block{},
                 cuckoo = oc::CuckooIndex<>{},
                 Tx = std::vector<block>{},
                 hashers = std::array<oc::AES, 3>{},
                 numBins = u64{},
                 keyBitLength = u64{},
                 keyByteLength = u64{},
                 r = Matrix<u8>{},
                 rIter = Matrix<u8>::iterator{},
                 rKeyIter = Matrix<u8>::iterator{},
                 rValIter = Matrix<u8>::iterator{},
                 opprf = std::make_unique<RsOpprfReceiver>());

        if (mRecverSize != X.size())
            throw RTE_LOC;

        setTimePoint("RsCpsiReceiver::receive begin");

        cuckooSeed = mPrng.get();
        MC_AWAIT(chl.send(std::move(cuckooSeed)));
        cuckoo.init(mRecverSize, mSsp, 0, 3);

        cuckoo.insert(X, cuckooSeed);
        Tx.resize(cuckoo.mNumBins);

        setTimePoint("RsCpsiReceiver::receive cuckoo");

        hashers[0].setKey(block(3242, 23423) ^ cuckooSeed);
        hashers[1].setKey(block(4534, 45654) ^ cuckooSeed);
        hashers[2].setKey(block(5677, 67867) ^ cuckooSeed);

        ret.mMapping.resize(X.size(), ~u64(0));
        numBins = cuckoo.mBins.size();
        std::cout << "numBins " << numBins << endl;
        ret.mMapRev.resize(numBins, ~u64(0));
        for (u64 i = 0; i < numBins; ++i)
        {
            auto &bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                // j = oc::CuckooIndex<>::minCollidingHashIdx(i, cuckoo.mHashes[b], 3, numBins);

                auto &hj = hashers[j];
                Tx[i] = hj.hashBlock(X[b]);
                ret.mMapping[b] = i; // x_index --> coockie_hashbin_index
                ret.mMapRev[i] = b;
            }
            else
            {
                Tx[i] = block(i, 0);
            }
        }
        setTimePoint("RsCpsiReceiver::receive values");
        // Tx coockie hash

        keyBitLength = mSsp + oc::log2ceil(Tx.size());
        keyByteLength = oc::divCeil(keyBitLength, 8);

        // std::cout << "Info: numBins" << numBins << " mReceiveSize" << mSenderSize << " keyBitLength" << keyBitLength << " keyByteLength" << keyByteLength << " Xsize" << X.size() << std::endl;

        r.resize(Tx.size(), keyByteLength + mValueByteLength, oc::AllocType::Uninitialized);

        if (mTimer)
            opprf->setTimer(*mTimer);

        // r is the result of opprf, which is r||value
        MC_AWAIT(opprf->receive(mSenderSize * 3, Tx, r, mPrng, mNumThreads, chl));

        // r is the result of opprf
        // for (size_t i = 0; i < r.size(); ++i)
        // {
        //     std::cout << +r(i) << " ";
        // }
        // std::cout << "split r " << r.size() << std::endl;

        // split r||value
        ret.mFlag.resize(numBins, keyByteLength, oc::AllocType::Uninitialized);
        rIter = r.begin();
        rKeyIter = ret.mFlag.begin();
        if (mValueByteLength)
        {
            ret.mValues.resize(numBins, mValueByteLength, oc::AllocType::Uninitialized);
            rValIter = ret.mValues.begin();
            for (u64 i = 0; i < numBins; ++i)
            {
                memcpy(&*rKeyIter, &*rIter, keyByteLength);
                rKeyIter += keyByteLength;
                rIter += keyByteLength;

                memcpy(&*rValIter, &*rIter, mValueByteLength);
                rValIter += mValueByteLength;
                rIter += mValueByteLength;
            }
        }
        else
        {
            for (u64 i = 0; i < numBins; ++i)
            {
                memcpy(&*rKeyIter, &*rIter, keyByteLength);
                rKeyIter += keyByteLength;
                rIter += keyByteLength;
            }
        }

        setTimePoint("RsCpsiReceiver::receive done");

        MC_END();
    }

    string i128tostring(__m128i v)
    {
        std::uint32_t a[4];
        _mm_store_si128((__m128i *)a, v);

        std::stringstream ss;
        for (size_t i = 3; i >= 0; --i)
        {
            ss << std::to_string(a[i]);
        }
        std::string str = ss.str();
        return str;
    }

    void RsMulpsiReceiver::Sharing::print()
    {
        std::cout << "Result info:" << std::endl;
        std::cout << "values " << mValues.size() << std::endl;
        for (size_t i = 0; i < mValues.size(); i++)
        {
            std::cout << +mValues(i) << " ";
        }

        std::cout << std::endl
                  << "map " << mMapping.size() << std::endl;
        for (size_t i = 0; i < mMapping.size(); i++)
        {
            std::cout << mMapping[i] << " ";
        }

        std::cout << std::endl
                  << "flag " << mFlag.size() << std::endl;

        for (size_t i = 0; i < mFlag.size(); i++)
        {
            std::cout << +mFlag(i) << " ";
        }
        std::cout << std::endl;
    }

    // void RsMulpsiReceiver::Share::print()
    // {
    //     std::cout << "Receive Result info:" << std::endl;
    //     std::cout << "values" << std::endl;
    //     for (size_t i = 0; i < nParties; i++)
    //     {
    //         std::cout << i << ": ";
    //         for (int j = 0; j < mValues[i].size(); j++)
    //         {
    //             std::cout << +mValues[i](j) << " ";
    //         }
    //         std::cout << std::endl;
    //     }

    //     std::cout << std::endl
    //               << "map" << std::endl;
    //     for (size_t i = 0; i < mMapping.size(); i++)
    //     {
    //         std::cout << mMapping[i] << " ";
    //     }

    //     std::cout << std::endl
    //               << "flag" << std::endl;
    //     for (size_t i = 0; i < nParties; i++)
    //     {
    //         std::cout << i << ": ";
    //         for (int j = 0; j < mFlagBits[i].size(); j++)
    //         {
    //             std::cout << +mFlagBits[i][j] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }

    Proto RsMulpsiReceiver::send(block &cuckooSeed, std::vector<Socket> &chls, const int p)
    {
        MC_BEGIN(Proto, this, &chls, &cuckooSeed, i = int{}, p);
        cuckooSeed = mPrng.get();
        for (i = 0; i < chls.size(); i++)
        {
            if (i != p)
            {
                MC_AWAIT(chls[i].send(std::move(cuckooSeed)));
            }
        }
        MC_END();
    }


    Proto RsMulpsiReceiver::send(block &cuckooSeed, std::vector<coproto::AsioSocket> &chls, const int p)
    {
        MC_BEGIN(Proto, this, &chls, &cuckooSeed, i = int{}, p);
        cuckooSeed = mPrng.get();
        for (i = 0; i < chls.size(); i++)
        {
            if (i != p)
            {
                MC_AWAIT(chls[i].send(std::move(cuckooSeed)));
            }
        }
        MC_END();
    }

    Proto RsMulpsiReceiver::receive(span<block> Tx, Share &ret, Socket &chl, const int p)
    {
        MC_BEGIN(Proto, this, Tx, &ret, &chl, p,
                 //  mFlag = oc::Matrix<u8>{},
                 r = Matrix<u8>{},
                 rIter = Matrix<u8>::iterator{},
                 rKeyIter = Matrix<u8>::iterator{},
                 rValIter = Matrix<u8>::iterator{},
                 opprf = std::make_unique<RsOpprfReceiver>(),
                 cmp = std::make_unique<Gmw>(),
                 cir = BetaCircuit{});

        setTimePoint("RsCpsiReceiver::receive begin");

        // ret.keyBitLength = mSsp + oc::log2ceil(Tx.size());
        // ret.keyByteLength = oc::divCeil(ret.keyBitLength, 8);

        // std::cout << "Info: numBins:" << ret.numBins << " |mReceiveSize:" << mSenderSize << " |keyBitLength:" << ret.keyBitLength << " |keyByteLength:" << ret.keyByteLength << " |Txsize:" << Tx.size() << std::endl;

        r.resize(Tx.size(), ret.keyByteLength + mValueByteLength, oc::AllocType::Uninitialized);

        if (mTimer)
            opprf->setTimer(*mTimer);

        for (size_t i = 0; i < Tx.size(); i++)
        {
            std::cout << Tx[i] << " ";
        }
        std::cout << std::endl;
        // r is the result of opprf, which is r||value
        MC_AWAIT(opprf->receive(mSenderSize * 3, Tx, r, mPrng, mNumThreads, chl));

        std::cout << "receive end" << std::endl;

        // split r||value
        rIter = r.begin();
        ret.mFlags[p].resize(ret.numBins, ret.keyByteLength, oc::AllocType::Zeroed);
        rKeyIter = ret.mFlags[p].begin();
        if (mValueByteLength)
        {
            ret.mValues[p].resize(ret.numBins, mValueByteLength, oc::AllocType::Uninitialized);
            rValIter = ret.mValues[p].begin();
            for (u64 i = 0; i < ret.numBins; ++i)
            {
                memcpy(&*rKeyIter, &*rIter, ret.keyByteLength);
                rKeyIter += ret.keyByteLength;
                rIter += ret.keyByteLength;

                memcpy(&*rValIter, &*rIter, mValueByteLength);
                rValIter += mValueByteLength;
                rIter += mValueByteLength;
            }
        }
        else
        {
            for (u64 i = 0; i < ret.numBins; ++i)
            {
                memcpy(&*rKeyIter, &*rIter, ret.keyByteLength);
                rKeyIter += ret.keyByteLength;
                rIter += ret.keyByteLength;
            }
        }
        setTimePoint("RsCpsiReceiver::receive done");

        // cir = isZeroCircuit(ret.keyBitLength);
        // cmp->init(mFlag.rows(), cir, mNumThreads, 0, mPrng.get());

        // cmp->implSetInput(0, mFlag, mFlag.cols());

        // MC_AWAIT(cmp->run(chl));

        // {
        //     auto ss = cmp->getOutputView(0);

        //     ret.mFlagBits[p].resize(ret.numBins);
        //     std::copy(ss.begin(), ss.begin() + ret.mFlagBits[p].sizeBytes(), ret.mFlagBits[p].data());

        //     // for (u64 i = 0; i < ret.mFlagBits[p].size(); ++i)
        //     // {
        //     //     cout << ret.mFlagBits[p][i] << " ";
        //     // }
        //     // cout << endl;
        // }

        setTimePoint("RsCpsiReceiver::receive done");

        MC_END();
    }

    void convert64(const Matrix<u8> &values, Matrix<u64> &result)
    {
        // Matrix<u64> result;
        result.resize(values.rows(), 1);
        // Matrix<u8>::iterator u8it = values.begin();
        // Matrix<u64>::iterator u64it = result.begin();
        u64 col = values.cols();
        u64 len = col > 8 ? 8 : col;
        u64 temp;
        // u64 col = values.cols();
        // std::cout << len << std::endl;
        for (u64 i = 0; i < values.rows(); ++i)
        {
            temp = 0;
            for (u64 j = 0; j < len; j++)
            {
                std::cout << std::bitset<8>(values(i, j)) << " ";
                temp = (temp << 8) + values(i, j);
                // std::cout << std::bitset<8>(temp) << "  | ";
            }
            result(i, 0) = temp;
            std::cout << std::endl;
        }
        // ashare.setAshare(result);
    }

    void Share::GenflagShare(BlongShare &bshare)
    {
        Matrix<u64> mLongFlag;
        if (mFlag.size() == numBins * keyByteLength)
        {
            convert64(mFlag, mLongFlag);
        }
        else
        {
            mLongFlag.resize(numBins, 1, oc::AllocType::Zeroed);
            // std::cout << mFlag.size() << endl;
            Matrix<u64> temp;
            for (size_t i = 0; i < nParties; i++)
            {
                if (i != myIdx)
                {
                    convert64(mFlags[i], temp);
                    mFlags[i].release();
                    for (u64 j = 0; j < temp.size(); ++j)
                    {
                        mLongFlag(j) ^= temp(j);
                    }
                }
            }
        }
        // mFlags.reset();
        mFlag.release();
        bshare.SetShare(mLongFlag);
    }
    /**
     * Tx is the set X after hash
     */
    // Proto RsMulpsiReceiver::receive(span<block> Tx, Share &ret, std::vector<Socket> &chls, const size_t p)
    // {
    //     MC_BEGIN(Proto, this, i = size_t{}, Tx, ret, &chls, p);
    //     for (i = 0; i < nParties; ++i)
    //     {
    //         if (i != p)
    //             this->receive(Tx, ret, chls[i], i);
    //     }
    //     MC_END();
    // }

    // Proto RsMulpsiReceiver::isZero(Share &ret, Socket &chl, const int p)
    // {
    // MC_BEGIN(Proto, this, &chl, &ret, p, cmp = std::make_unique<Gmw>(), cir = BetaCircuit{});
    // cir = isZeroCircuit(ret.keyBitLength);
    // cmp->init(ret.mFlag[p].rows(), cir, mNumThreads, 0, mPrng.get());

    // cmp->implSetInput(0, ret.mFlag[p], ret.mFlag[p].cols());

    // MC_AWAIT(cmp->run(chl));

    // {
    //     auto ss = cmp->getOutputView(0);

    //     ret.mFlagBits.resize(ret.numBins);
    //     std::copy(ss.begin(), ss.begin() + ret.mFlagBits.sizeBytes(), ret.mFlagBits.data());

    //     // if (mValueByteLength)
    //     // {
    //     //     ret.mValues.resize(numBins, mValueByteLength);

    //     //     for (u64 i = 0; i < numBins; ++i)
    //     //     {
    //     //         std::memcpy(&ret.mValues(i, 0), &r(i, keyByteLength), mValueByteLength);
    //     //     }
    //     // }
    //     for (u64 i = 0; i < ret.mFlagBits.size(); ++i)
    //     {
    //         cout << ret.mFlagBits[i] << " ";
    //     }
    //     cout << endl;
    // }

    // setTimePoint("RsCpsiReceiver::receive done");

    // MC_END();
    // }

    // nParties is the number of normal party
    // bool RsMulpsiReceiver::test(size_t setSize, size_t myIdx, size_t nParties, RsMulpsiReceiver::Sharing sss[], RsMulpsiReceiver::Sharing rss[], span<block> X)
    // {
    //     // test the right of sss
    //     // read plaintext
    //     map<string, string> kvs[nParties];
    //     string temp;
    //     for (size_t i = 0; i < nParties; i++)
    //     {
    //         std::ifstream infile;
    //         infile.open("set" + std::to_string(i) + ".txt");
    //         if (i == myIdx) // leader
    //         {
    //             for (size_t j = 0; j < setSize; j++)
    //             {
    //                 infile >> temp;
    //                 kvs[i].insert(make_pair(temp, ""));
    //             }
    //         }
    //         else
    //         {
    //             for (size_t j = 0; j < setSize; j++)
    //             {
    //                 infile >> temp;
    //                 kvs[i].insert(make_pair(temp.substr(0, 128), temp.substr(129, temp.size())));
    //             }
    //         }
    //     }
    //     // caculate plaintext
    //     map<string, vector<string>> intsecs;
    //     for (map<string, string>::iterator it = kvs[myIdx].begin(); it != kvs[myIdx].end(); it++)
    //     {
    //         string key = (*it).first;
    //         bool success = true;

    //         for (size_t i = 0; i < myIdx; i++)
    //         {
    //             if (kvs[i].find(key) == kvs[i].end())
    //             {
    //                 success = false;
    //                 break;
    //             }
    //         }
    //         for (size_t i = myIdx + 1; i < nParties; i++)
    //         {
    //             if (kvs[i].find(key) == kvs[i].end())
    //             {
    //                 success = false;
    //                 break;
    //             }
    //         }
    //         if (success)
    //         {
    //             vector<string> strs(nParties);
    //             for (size_t i = 0; i < nParties; i++)
    //             {
    //                 if (i != myIdx)
    //                 {
    //                     strs[i] = kvs[i].at(key);
    //                 }
    //             }
    //             intsecs["as"] = strs;
    //         }
    //     }

    //     // caculate cipher
    //     size_t l = rss[0].mFlag.size();
    //     for (size_t j = 0; j < l; j++)
    //     {
    //         u8 allx = 0;
    //         for (size_t i = 0; i < nParties; i++)
    //         {
    //             allx ^= (rss[i].mFlag(j) ^ sss[i].mFlag(j));
    //         }
    //         if (allx == 0) // x is intersection element
    //         {
    //             bool xsuc = false;
    //             string key = i128tostring(X[rss[0].mMapRev[j]]); // get key
    //             if (intsecs.find(key) != intsecs.end())          // check key
    //             {
    //                 // check value
    //                 bool vsuc = true;
    //                 vector<string> strs = intsecs[key];
    //                 for (size_t i = 0; i < nParties; i++)
    //                 {
    //                     if (i != myIdx)
    //                     {
    //                         string v = to_string(rss[i].mValues(j) ^ sss[i].mValues(j));
    //                         cout << v << " " << strs[i] << endl;
    //                         if (strs[i] != v)
    //                         {
    //                             vsuc = false;
    //                             break;
    //                         }
    //                     }
    //                 }
    //                 xsuc = vsuc; // both key and value are right
    //             }
    //             cout << key << " " << xsuc << " ";
    //         }
    //     }
    // }
}