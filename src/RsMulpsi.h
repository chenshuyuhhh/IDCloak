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
#include "coproto/Socket/AsioSocket.h"
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
    constexpr bool RsMulpsiSenderDebug = false;

    struct Share
    {
        // The sender's share of the bit vector indicating that
        // the i'th row is a real row (1) or a row (0).
        // oc::BitVector mFlagBit;

        // The sender's share of the bit vector indicating that
        // the i'th row is a real row (1) or a row (0).
        // oc::BitVector *mFlagBits = nullptr;

        // Secret share of the values associated with the output
        // elements. These values are from the sender.
        oc::Matrix<u8> *mValues;

        // The mapping of the senders input rows to output rows.
        // Each input row might have been mapped to one of three
        // possible output rows.
        std::vector<std::array<u64, 3>> mMapping;

        oc::Matrix<u8> mFlag;
        oc::Matrix<u8> * mFlags;

        // Secret share of the values associated with the output
        // elements. These values are from the sender.
        // int nParties;
        oc::u64 numBins;
        oc::u64 keyBitLength;
        oc::u64 keyByteLength;
        // oc::Matrix<u64> mLongFlag;

        // void mergeFlag();
        void print();
        void GenflagShare(BlongShare &bshare);
    };

    class RsMulpsiSender : public details::RsCpsiBase, public oc::TimerAdapter
    {
        // private:
        //     u64 keyByteLength;

    public:
        struct Sharing
        {
            // The sender's share of the bit vector indicating that
            // the i'th row is a real row (1) or a row (0).
            oc::BitVector mFlagBits;

            // Secret share of the values associated with the output
            // elements. These values are from the sender.
            oc::Matrix<u8> mValues;

            // The mapping of the senders input rows to output rows.
            // Each input row might have been mapped to one of three
            // possible output rows.
            std::vector<std::array<u64, 3>> mMapping;

            // The share of the flag which indicates whether the bucket has intersection ele
            // the i'th bucket has intersection element (1)
            oc::Matrix<u8> mFlag;

            oc::u64 numBins;

            // oc::BitVector mFlagBits;

            oc::u64 keyBitLength;

            void print();
        };

        // struct Share
        // {
        //     // The sender's share of the bit vector indicating that
        //     // the i'th row is a real row (1) or a row (0).
        //     oc::BitVector mFlagBits;

        //     // Secret share of the values associated with the output
        //     // elements. These values are from the sender.
        //     oc::Matrix<u8> mValues;

        //     // The mapping of the senders input rows to output rows.
        //     // Each input row might have been mapped to one of three
        //     // possible output rows.
        //     std::vector<std::array<u64, 3>> mMapping;

        //     // The share of the flag which indicates whether the bucket has intersection ele
        //     // the i'th bucket has intersection element (1)

        //     oc::u64 numBins;

        //     oc::u64 keyBitLength;

        //     void print();
        // };

        // perform the join with Y being the join keys with associated values.
        // The output is written to s.
        // using coroutine
        Proto send(span<block> Y, const oc::MatrixView<u8> values, Sharing &ret, Socket &chl); // id and feature
                                                                                               // r is flag of the intersection item
        Proto send(span<block> Y, const oc::MatrixView<u8> values, Share &ret, Socket &chl);   // id and feature
                                                                                               // r is flag of the intersection item

        Proto isZero(Sharing &ret, Socket &chl);
        // intersection with TCP socket
        // RsOprfSender mSender;
        // void setMultType(oc::MultType type) { mSender.setMultType(type); };
        // Proto run(span<block> Y, oc::MatrixView<u8> values, Sharing &s, Socket &chl);
        // Proto getResult(span<block> Y, oc::Matrix<u8> mValues, oc::Matrix<u8> mFlag, Socket &chl);
        // bool test(RsMulpsiSender::Sharing &sss, Socket &chl);
    };

    class RsMulpsiReceiver : public details::RsCpsiBase, public oc::TimerAdapter
    {
    public:
        struct Sharing
        {
            // The sender's share of the bit vector indicating that
            // the i'th row is a real row (1) or a row (0).
            // oc::BitVector mFlagBits;

            // Secret share of the values associated with the output
            // elements. These values are from the sender.
            oc::Matrix<u8> mValues;

            // The mapping of the receiver's input rows to output rows.
            std::vector<u64> mMapping;

            oc::Matrix<u8> mFlag;

            std::vector<u64> mMapRev;

            oc::u64 numBins;

            void print();
        };

        // struct Share
        // {

        //     // Secret share of the values associated with the output
        //     // elements. These values are from the sender.
        //     int nParties;
        //     oc::u64 numBins;
        //     oc::u64 keyBitLength;
        //     oc::u64 keyByteLength;

        //     // The mapping of the receiver's input rows to output rows.
        //     std::vector<u64> mMapping;
        //     std::vector<u64> mMapRev;

        //     // oc::Matrix<u8> *mFlag;
        //     oc::Matrix<u8> *mValues;

        //     void print();
        // };

        // perform the join with X being the join keys.
        // The output is written to s.
        Proto receive(span<block> X, Sharing &s, Socket &chl);

        Proto send(block &cuckooSeed, std::vector<coproto::AsioSocket> &chls, const int p);

        // p is the leader party number
        Proto send(block &cuckooSeed, std::vector<Socket> &chl, const int p);
        // p is the the send party number.
        Proto receive(span<block> Tx, Share &ret, Socket &chl, const int p);
        // Proto receive(span<block> Tx, Share &ret, std::vector<Socket> &chls, const size_t p);
    };

}

// #endif