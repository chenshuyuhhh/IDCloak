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
    class MulPSILeader : public details::RsCpsiBase, public oc::TimerAdapter
    {
    public:
        // p is the leader party number
        Proto cuckooSeedSync(block &cuckooSeed, std::vector<Socket> &chl, const int p);
        Proto cuckooSeedSync(block &cuckooSeed, std::vector<coproto::AsioSocket> &chl, const int p);
        // p is the the send party number.
        Proto receive(span<block> Tx, Matrix<u8> &ret, Socket &chl);
        Proto receive(span<block> Tx, std::vector<Matrix<u8>> &rets, std::vector<Socket> &chls, const int p);
        
        Proto oprf_receive(span<block> Tx, std::vector<u64> &ret, Socket &chl);
        Proto oprf_receive(span<block> Tx, std::vector<block> &ret, Socket &chl);
        Proto oprf_receive(span<block> Tx, Matrix<u8> &ret, Socket &chl);
    };

}

// #endif