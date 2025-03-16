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

namespace volePSI
{

    struct ShareInfo{
        oc::u64 numBins;
        oc::u64 keyBitLength;
        oc::u64 keyByteLength;
    };

    // struct Share
    // {
    //     // Secret share of the values associated with the output
    //     // elements. These values are from the sender.
    //     oc::Matrix<u8> *mValues;

    //     // The mapping of the senders input rows to output rows.
    //     // Each input row might have been mapped to one of three
    //     // possible output rows.
    //     std::vector<std::array<u64, 3>> mMapping;

    //     oc::Matrix<u8> mFlag;
    //     oc::Matrix<u8> * mFlags;

    //     // void mergeFlag();
    //     void print();
    //     void GenflagShare(BlongShare &bshare);
    // };
}

// #endif