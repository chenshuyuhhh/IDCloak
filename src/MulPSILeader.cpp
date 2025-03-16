#include "MulPSILeader.h"

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

    Proto MulPSILeader::cuckooSeedSync(block &cuckooSeed, std::vector<Socket> &chls, const int p)
    {
        MC_BEGIN(Proto, this, &chls, &cuckooSeed, i = size_t{}, p);
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

    Proto MulPSILeader::cuckooSeedSync(block &cuckooSeed, std::vector<coproto::AsioSocket> &chls, const int p)
    {
        MC_BEGIN(Proto, this, &chls, &cuckooSeed, i = size_t{}, p);
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

    Proto MulPSILeader::receive(span<block> Tx, Matrix<u8> &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, Tx, &ret, &chl,
                 opprf = std::make_unique<RsOpprfReceiver>());

        // r.resize(Tx.size(), ret.keyByteLength + mValueByteLength, oc::AllocType::Uninitialized);

        // if (mTimer)
        //     opprf->setTimer(*mTimer);

        // for (size_t i = 0; i < Tx.size(); i++)
        // {
        //     std::cout << Tx[i] << " ";
        // }
        // std::cout << std::endl;
        // r is the result of opprf, which is r||value
        MC_AWAIT(opprf->receive(mSenderSize * 3, Tx, ret, pPrng, mNumThreads, chl));
        // MC_AWAIT(opprf->receive(mSenderSize * 3, Tx, ret, pPrng, mNumThreads, chl));

        // std::cout << "receive end" << std::endl;

        MC_END();
    }

    Proto MulPSILeader::oprf_receive(span<block> Tx, std::vector<block> &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, Tx, &ret, &chl,
                 oprf = std::make_unique<RsOprfReceiver>());

        MC_AWAIT(oprf->receive(Tx, ret, pPrng, chl, mNumThreads));

        MC_END();
    }

    Proto MulPSILeader::oprf_receive(span<block> Tx, std::vector<u64> &ret, Socket &chl)
    {
        MC_BEGIN(Proto, this, Tx, &ret, &chl,
                 n = u64{},
                 m = u64{},
                 temp = std::move(BasicVector<block>{}),
                 oprfOutput = span<block>{},
                 oprf = std::make_unique<RsOprfReceiver>());

        // std::cout << Tx.size() << " " << ret.size() << std::endl;

        if (Tx.size() != ret.size())
            throw RTE_LOC;

        n = ret.size();
        m = sizeof(u64);
        temp.resize(n);
        oprfOutput = temp;

        MC_AWAIT(oprf->receive(Tx, oprfOutput, pPrng, chl, mNumThreads));

        for (u64 i = 0; i < n; ++i)
            memcpy(&ret[i], &oprfOutput[i], m);

        MC_END();
    }

    Proto MulPSILeader::oprf_receive(span<block> Tx, Matrix<u8> &outputs, Socket &chl)
    {
        MC_BEGIN(Proto, this, Tx, &outputs, &chl,
                 n = u64{},
                 m = u64{},
                 temp = std::move(BasicVector<block>{}),
                 oprfOutput = span<block>{},
                 oprf = std::make_unique<RsOprfReceiver>());

        n = outputs.rows();
        m = outputs.cols();
        if (outputs.cols() >= sizeof(block))
        {
            // reuse memory. extra logic to make sure oprfOutput is properly aligned.
            auto ptr = outputs.data() + outputs.size();
            auto offset = (u64)ptr % sizeof(block);
            if (offset)
                ptr -= offset;
            assert((((u64)ptr) % sizeof(block)) == 0);

            oprfOutput = span<block>((block *)(ptr)-n, n);

            assert(
                (void *)(oprfOutput.data() + oprfOutput.size()) <=
                (void *)(outputs.data() + outputs.size()));
        }
        else
        {
            temp.resize(n);
            oprfOutput = temp;
        }
        MC_AWAIT(oprf->receive(Tx, oprfOutput, pPrng, chl, mNumThreads));

        // decode

        if (m > sizeof(block))
        {

            std::vector<block> buffer(oc::divCeil(m, sizeof(block)));
            auto buffPtr = (u8 *)buffer.data();

            for (u64 i = 0; i < n; ++i)
            {
                oc::AES enc(oprfOutput[i]);
                enc.ecbEncCounterMode(0, buffer);
                memcpy(&outputs(i * m), buffPtr, m);
            }
        }
        else
        {
            for (u64 i = 0; i < n; ++i)
                memcpy(&outputs(i * m), &oprfOutput[i], m);
        }

        temp = {};

        MC_END();
    }

    Proto MulPSILeader::receive(span<block> Tx, std::vector<Matrix<u8>> &rets, std::vector<Socket> &chls, const int p)
    {
        MC_BEGIN(Proto, this, Tx, &rets, &chls, p,
                 i = size_t{},
                 opprf = std::make_unique<RsOpprfReceiver>());

        // r.resize(Tx.size(), ret.keyByteLength + mValueByteLength, oc::AllocType::Uninitialized);

        if (mTimer)
            opprf->setTimer(*mTimer);

        // for (size_t i = 0; i < Tx.size(); i++)
        // {
        //     std::cout << Tx[i] << " ";
        // }
        // std::cout << std::endl;
        // r is the result of opprf, which is r||value
        for (i = 0; i < chls.size(); i++)
        {
            if (i != p)
            {
                MC_AWAIT(opprf->receive(mSenderSize * 3, Tx, rets[i], pPrng, mNumThreads, chls[i]));
            }
        }

        // std::cout << "receive end" << std::endl;

        MC_END();
    }
}