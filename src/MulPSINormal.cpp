#include "MulPSINormal.h"

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
    extern PRNG pPrng;

    Proto MulPSINormal::cuckooSeedSync(block &cuckooSeed, Socket &chl)
    {
        MC_BEGIN(Proto, this, &chl, &cuckooSeed);
        MC_AWAIT(chl.recv(cuckooSeed));
        MC_END();
    }

    Proto MulPSINormal::send(span<block> Ty, const Matrix<u8> Tv, oc::u64 numBins, Socket &chl)
    {
        MC_BEGIN(Proto, this, Ty, Tv, numBins, &chl,
                 opprf = std::make_unique<RsOpprfSender>());

        // setTimePoint("RsCpsiSender::send begin");
        // if (mRecverSize != Y.size() || mValueByteLength != values.cols())
        //     throw RTE_LOC;

        // if (mTimer)
        //     opprf->setTimer(*mTimer);

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
        MC_AWAIT(opprf->send(numBins, Ty, Tv, pPrng, mNumThreads, chl));
        // MC_AWAIT(opprf->send(numBins, Ty, Tv, pPrng, mNumThreads, chl));

        MC_END();
    }


    // Proto MulPSINormal::oprf_send(oc::u64 numBins, Socket &chl, RsOprfSender &oprf)
    // {
    //     MC_BEGIN(Proto, this, numBins, &chl, &oprf);

    //     MC_AWAIT(oprf.sendAfterInit(numBins, pPrng, chl, mNumThreads));

    //     MC_END();
    // }

    Proto MulPSINormal::oprf_send(oc::u64 numBins, span<block> Tx, std::vector<u64> &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, numBins, &chl, Tx, &ret,
                 n = u64{},
                 m = u64{},
                 oprfOutput = span<block>{},
                 tempPtr = std::unique_ptr<u8[]>{},
                 oprf = std::make_unique<RsOprfSender>());

        // std::cout << "oprf " << Tx.size() << " " << ret.size() << std::endl;

        if (Tx.size() != ret.size())
            throw RTE_LOC;

        n = ret.size();
        m = sizeof(u64);

        tempPtr.reset(new u8[n * sizeof(block)]);
        oprfOutput = span<block>((block *)tempPtr.get(), n);

        MC_AWAIT(oprf->send(numBins, pPrng, chl, mNumThreads));

        // std::cout << "after oprf" << std::endl;
        oprf->eval(Tx, oprfOutput, mNumThreads);

        for (u64 i = 0; i < n; ++i)
            memcpy(&ret[i], &oprfOutput[i], m);

        MC_END();
    }

    Proto MulPSINormal::oprf_send(oc::u64 numBins, span<block> Tx, std::vector<block> &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, numBins, &chl, Tx, &ret,
                 oprf = std::make_unique<RsOprfSender>());
        MC_AWAIT(oprf->send(numBins, pPrng, chl, mNumThreads));

        oprf->eval(Tx, ret, mNumThreads);

        MC_END();
    }

    Proto MulPSINormal::oprf_send(oc::u64 numBins, span<block> Tx, Matrix<u8> &out, Socket &chl)
    {
        MC_BEGIN(Proto, this, numBins, &chl, Tx, &out,
                 n = u64{},
                 m = u64{},
                 oprfOutput = span<block>{},
                 tempPtr = std::unique_ptr<u8[]>{},
                 oprf = std::make_unique<RsOprfSender>());

        if (m == sizeof(block))
            oprfOutput = span<block>((block *)out.data(), n);
        else
        {
            tempPtr.reset(new u8[n * sizeof(block)]);
            oprfOutput = span<block>((block *)tempPtr.get(), n);
        }

        MC_AWAIT(oprf->send(numBins, pPrng, chl, mNumThreads));

        oprf->eval(Tx, oprfOutput, mNumThreads);

        if (m <= sizeof(block))
        {
            // short string case
            for (u64 i = 0; i < n; ++i)
                memcpy(&out(i * m), &oprfOutput[i], m);
        }
        else
        {
            // long string case
            std::vector<block> buffer(oc::divCeil(m, sizeof(block)));
            auto buffPtr = (u8 *)buffer.data();
            for (u64 i = 0, ij = 0; i < n; ++i)
            {
                oc::AES enc(oprfOutput[i]);
                enc.ecbEncCounterMode(0, buffer);
                memcpy(&out(i * m), buffPtr, m);
            }
        }

        MC_END();
    }

}