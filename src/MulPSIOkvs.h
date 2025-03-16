#pragma once
// © 2022 Visa.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "volePSI/Defines.h"
#include "volePSI/config.h"
// #ifdef VOLE_PSI_ENABLE_MULPSI

#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/CuckooIndex.h"
#include "volePSI/RsOpprf.h"
#include "volePSI/RsCpsi.h"
#include "volePSI/GMW/Gmw.h"
#include "volePSI/SimpleIndex.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/BitVector.h"
#include <vector>
#include "MPC/ShareOp.h"
#include "MPC/BlongShare.h"
#include "MulPSIShare.h"

namespace volePSI
{

    class MulPSIOkvs
    {
        PRNG mPrng;

    public:
        MulPSIOkvs(block seed = ZeroBlock)
        {
            mPrng.SetSeed(seed);
        };

        template <typename Type>
        Proto send(span<block> Tx, span<Type> Ty, Socket &chl)
        {
            MC_BEGIN(Proto, this, Tx, Ty, &chl,
                     mPaxos = Baxos{},
                     n = u64{},
                     m = u64{},
                     //  val = oc::Matrix<block>{},
                     p = oc::Matrix<Type>{},
                     type = PaxosParam::Binary);
            n = Tx.size();
            m = 1;
            // m = sizeof();
            mPaxos.mSeed = pPrng.get();
            macoro::sync_wait(chl.send(std::move(mPaxos.mSeed)));
            type = m % sizeof(block) ? PaxosParam::Binary : PaxosParam::GF128;
            mPaxos.init(n, 1 << 14, 3, 40, type, mPaxos.mSeed);

            if (Tx.size() != Ty.size())
                throw RTE_LOC;
            if (!n || !m)
                throw RTE_LOC;

            // std::cout << "paxos receive begin" << mPaxos.size() << " " << m << std::endl;
            // encode okvs
            p.resize(mPaxos.size(), m);

            mPaxos.solve<Type>(Tx, Ty, oc::span<Type>(p), &mPrng);

            // std::cout << "send:" << std::endl;
            // for (size_t i = 0; i < Ty.size(); i++)
            // {
            //     std::cout << Tx[i] << ":" << Ty[i] << std::endl;
            // }
            // std::cout << p.size() << std::endl;

            MC_AWAIT(chl.send(p));
            MC_END();
        }
        // Proto send(span<block> Tx, span<u64> Ty, Socket &chl);

        template <typename Type>
        Proto receive(u64 mSenderSize, span<block> Tx, span<Type> ret, Socket &chl)
        {

            MC_BEGIN(Proto, this, mSenderSize, Tx, ret, &chl,
                     mPaxos = Baxos{},
                     //  n = u64{},
                     m = u64{},
                     //  val = oc::Matrix<block>{},
                     p = oc::Matrix<Type>{},
                     type = PaxosParam::Binary);

            // std::cout << "begin okvs recive" << std::endl;
            // n = Tx.size();
            m = 1;
            // u64 nm = n * m;
            // ret.resize(n, m);

            // if (ret.size() != n * m)
            //     throw RTE_LOC;

            // receive seed of paxos and init mpaxos
            macoro::sync_wait(chl.recv(mPaxos.mSeed));
            type = m % sizeof(block) ? PaxosParam::Binary : PaxosParam::GF128;
            mPaxos.init(mSenderSize, 1 << 14, 3, 40, type, mPaxos.mSeed);
            // std::cout << "paxos receive begin" << mPaxos.size() << " " << m << std::endl;
            p.resize(mPaxos.size(), m, oc::AllocType::Uninitialized);
            // std::cout << p.size() << std::endl;
            MC_AWAIT(chl.recv(p));
            // get okvs structure p
            // try
            // {
            //     macoro::sync_wait(chl.recv(p));
            //     std::cout << "paxos receive end" << std::endl;
            // }
            // catch (const std::exception &e)
            // {
            //     std::cerr << "Error: " << e.what() << std::endl;
            // }

            // decode
            mPaxos.template decode<Type>(Tx, ret, oc::span<Type>(p));

            // log
            // std::cout << "receive:" << std::endl;
            // for (size_t i = 0; i < ret.size(); i++)
            // {
            //     std::cout << Tx[i] << ":" << ret[i] << std::endl;
            // }

            MC_END();
        }
        // Proto receive(u64 mSenderSize, span<u64> Tx, span<u64> ret, Socket &chl);
    };
}

// #endif