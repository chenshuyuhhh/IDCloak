#ifndef SMALL_SHARE
#define SMALL_SHARE

#include "volePSI/Defines.h"
#include "cryptoTools/Network/Channel.h"
#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/MatrixView.h"
#include "cryptoTools/Common/block.h"
#include "vector"
#include "cryptoTools/Crypto/PRNG.h"

namespace volePSI
{
    extern std::vector<coproto::AsioSocket> psocks; // connection psocks
    extern size_t nParties;                     // the party num
    extern size_t myIdx;                           // the number of party
    extern size_t lIdx;                            // the number of lead party
    extern bool normal;                         // leader party(0), normal party(1)
    extern PRNG pPrng;

    class SmallShare
    {
    private:
        oc::Matrix<u8> ashare; // bool share
        u64 col, row;

    public:
        SmallShare();
        SmallShare(oc::Matrix<u8> ashare);
        void SetShare(oc::Matrix<u8> ashare);

        Proto SecretShareCo(size_t id, std::vector<size_t> ids);         // id share to ids
        void GenSecrets(std::vector<oc::Matrix<u8>> &secrets, size_t n); // gen n shares

        Proto SecretShareCo(size_t id);
        void GenSecrets(std::vector<oc::Matrix<u8>> &secrets);

        Proto SecretShareSizeCo(size_t id, std::vector<size_t> ids);
        Proto SecretShareSizeCo(size_t id);
        void SecretShareSize(size_t id);

        Proto RevealCo(oc::Matrix<u8> &plain);
        Proto RevealCo();
        void Reveal(oc::Matrix<u8> &plain);
        void RevealPrint();
        void Print();
    };
}

#endif