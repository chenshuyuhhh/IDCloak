#include "ShareOp.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Network/Channel.h"
#include <vector>
#include <random>
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <algorithm>

namespace volePSI
{

    BoolShare::BoolShare() {}
    BoolShare::BoolShare(oc::BitVector bshare)
    {
        this->bshare = bshare;
        this->size = bshare.size();
    }

    BoolShare::BoolShare(std::string bshare)
    {
        this->bshare = oc::BitVector(bshare);
        this->size = bshare.size();
    }

    BoolShare::BoolShare(u64 size)
    {
        this->bshare.resize(size);
        this->size = size;
    }

    void BoolShare::SetShare(oc::BitVector bshare)
    {
        this->bshare = bshare;
        this->size = bshare.size();
    }

    void BoolShare::resize(u64 newSize, u8 val)
    {
        this->bshare.resize(newSize, val);
        this->size = newSize;
    }

    void BoolShare::Print()
    {
        std::cout << "size " << bshare.size() << ": ";
        for (size_t i = 0; i < bshare.size(); i++)
        {
            std::cout << bshare[i];
        }
        std::cout << std::endl;
    }

    void BoolShare::GenShare(const oc::BitVector share, std::vector<oc::BitVector> &shares) const
    {
        shares.resize(nParties);
        std::vector<u8> temp(share.sizeBytes());
        shares[0] = share;
        for (size_t i = 1; i < nParties; i++)
        {
            pPrng.get<u8>(temp);
            shares[i] = oc::BitVector(temp.data(), share.size());
            shares[0] ^= shares[i];
        }
    }

    Proto BoolShare::AndReveal(const BoolShare &bshare2, BoolShare &result) const
    {
        MC_BEGIN(Proto, this, &result, &bshare2, chls = psocks,
                 rx = oc::BitVector{},
                 sy = oc::BitVector{},
                 i = size_t{},
                 xdata = std::vector<std::vector<u8>>{},
                 ydata = std::vector<std::vector<u8>>{},
                 xplain = oc::BitVector{},
                 yplain = oc::BitVector{},
                 shares = std::vector<oc::BitVector>{});

        // reveal by TEE
        if (myIdx == trustp)
        {
            xdata.resize(nParties);
            ydata.resize(nParties);
            for (i = 0; i < nParties; i++)
            {
                if (i != myIdx)
                {
                    xdata[i].resize(bshare.sizeBytes());
                    ydata[i].resize(bshare.sizeBytes());
                    MC_AWAIT(chls[i].recv(xdata[i]));
                    MC_AWAIT(chls[i].recv(ydata[i]));
                }
            }
            // reveal rx^sy
            xplain = bshare;
            yplain = bshare2.GetShare();
            for (i = 0; i < nParties; i++)
            {
                if (i != myIdx)
                {
                    xplain ^= oc::BitVector(xdata[i].data(), size);
                    yplain ^= oc::BitVector(ydata[i].data(), size);
                }
            }
            xplain = xplain & yplain;

            // send to party
            // std::vector<oc::BitVector> shares;
            GenShare(xplain, shares);
            for (i = 0; i < nParties; i++)
            {
                if (i != myIdx)
                {
                    MC_AWAIT(chls[i].send(shares[i]));
                }
            }
            result.SetShare(shares[myIdx]);
        }
        else
        {
            MC_AWAIT(chls[trustp].send(this->GetShare()));
            MC_AWAIT(chls[trustp].send(bshare2.GetShare()));

            xplain.resize(bshare.size());
            MC_AWAIT(chls[trustp].recv(xplain));
            result.SetShare(xplain);
        }

        MC_END();
    }

    /**
     * c is plain value
     */
    BoolShare BoolShare::operator&(const oc::BitVector &c) const
    {
        return BoolShare(this->bshare & c);
    }

    Proto BoolShare::GetRand(oc::BitVector &result) const
    {
        MC_BEGIN(Proto, this, chls = psocks, &result, node = 1,
                 i = size_t{},
                 temp = std::vector<u8>{});
        temp.resize(this->bshare.sizeBytes());
        if (myIdx == node)
        {
            // std::vector<u8> temp(bshare.sizeBytes());
            pPrng.get<u8>(temp);
            for (i = 0; i < nParties; i++)
            {
                if (myIdx != i)
                {
                    MC_AWAIT(chls[i].send(temp));
                }
            }
        }
        else
        {
            MC_AWAIT(chls[node].recv(temp));
            // std::cout << +temp[0];
        }

        result = oc::BitVector(temp.data(), size);
        MC_END();
    }

    // (x^r)(y^s)=xy^xs^ry^rs
    BoolShare BoolShare::operator&(const BoolShare &y) const
    {
        // mask x using plainr
        // mask y using plains
        oc::BitVector r(size);
        oc::BitVector s(size);
        // r and s are plain known by parties, but TEE not know
        if (OFFLINE) // init r and s
        {
            macoro::sync_wait(GetRand(r));
            macoro::sync_wait(GetRand(s));
        }
        // std::cout << "r\n"
        //           << r << std::endl;
        // std::cout << "s\n"
        //           << s << std::endl;

        BoolShare rx = *this ^ r; // r^x
        BoolShare sy = y ^ s;     // s^y
        // std::cout << size << std::endl;
        // std::cout << "rx\n";
        // rx.RevealPrint();
        // std::cout << "sy\n";
        // sy.RevealPrint();

        // x^y = (r^x ^ r) & (s^y ^ s)
        //     = (r^x & s^y) ^ (r & s^y) ^ (s & r^x) ^ (r & s)
        BoolShare result;                            // result is the share of (r^x & s^y)
        macoro::sync_wait(rx.AndReveal(sy, result)); // TEE
        // std::cout << "(r^x & s^y)\n";
        // result.RevealPrint();
        return result ^ (sy & r) ^ (rx & s) ^ (r & s);
    }

    oc::BitVector BoolShare::orBitVec(const oc::BitVector &c) const
    {
        return bshare ^ c;
    }

    // x^r
    BoolShare BoolShare::operator^(const oc::BitVector &c) const
    {
        if (myIdx == 1) // only one party mask
        {
            return BoolShare(bshare ^ c);
        }
        else
        {
            return BoolShare(bshare);
        }
    }

    // x ^ y
    BoolShare BoolShare::operator^(const BoolShare &c) const
    {
        return c.GetShare() ^ bshare;
    }

    BoolShare BoolShare::operator~() const
    {
        return BoolShare(~bshare);
    }

    bool BoolShare::operator[](const u64 i) const
    {
        return bshare[i];
    }

    AShare::AShare(const oc::Matrix<u64> ashare)
    {
        this->ashare = ashare;
        this->size = ashare.size();
        this->col = ashare.cols();
        this->row = ashare.rows();
    }

    void AShare::setAshare(const oc::Matrix<u64> ashare)
    {
        this->ashare = ashare;
        this->size = ashare.size();
        this->col = ashare.cols();
        this->row = ashare.rows();
    }

    oc::Matrix<u64> AShare::getShare()
    {
        return ashare;
    }

    void AShare::Print()
    {
        std::cout << "size " << ashare.size() << ": ";
        for (size_t i = 0; i < ashare.size(); i++)
        {
            std::cout << ashare(i) << " ";
        }
        std::cout << std::endl;
    }

    Proto BoolShare::Reveal()
    {
        MC_BEGIN(Proto, this, p = myIdx, socks = psocks, i = int{}, size = this->bshare.size(), sizeByte = this->bshare.sizeBytes(), recvdata = std::vector<std::vector<u8>>{}, bplain = oc::BitVector{});
        // std::cout << p << std::endl;
        recvdata.resize(nParties);
        for (i = 0; i < nParties; i++)
        {
            if (i != p)
            {
                MC_AWAIT(socks[i].send(bshare));
                recvdata[i].resize(sizeByte);
                MC_AWAIT(socks[i].recv(recvdata[i]));
                // std::cout << "i " << i << ": ";
            }
        }
        // reveal
        bplain = oc::BitVector(bshare);
        for (i = 0; i < nParties; i++)
        {
            if (i != p)
                bplain ^= oc::BitVector(recvdata[i].data(), size);
            // std::cout << "i " << i << " ";
        }

        // print
        std::cout << "size " << size << ":";
        for (size_t j = 0; j < size; j++)
            std::cout << bplain[j] << " ";
        std::cout << std::endl;
        MC_END();
    }

    void BoolShare::RevealPrint()
    {
        macoro::sync_wait(Reveal());
    }

    // offline: <r_a> <r_b>
    Proto BoolShare::Convert2ACo(AShare &ashare)
    {
        MC_BEGIN(Proto, this, &ashare, p = trustp, socks = psocks,
                 i = int{}, j = u64{},
                 size = this->bshare.size(),
                 sizeByte = this->bshare.sizeBytes(),
                 ashares = std::vector<oc::Matrix<u64>>{},
                 brecv = std::vector<std::vector<u8>>{},
                 bshares = std::vector<oc::BitVector>{});
        if (myIdx == trustp)
        {
            brecv.resize(nParties);
            // std::cout << sizeByte << " " << size << std::endl;
            // mask ashare
            for (i = 1; i < nParties; i++)
            {
                brecv[i].resize(sizeByte);
                MC_AWAIT(socks[i].recv(brecv[i]));
                // std::cout << "i " << i << ": ";

                // for (j = 0; j < sizeByte; j++)
                // {
                //     // std::cout << +(brecv[i][j]) << " ";
                //     for (int k = 0; k <= 7; k++)
                //     {
                //         std::cout << ((brecv[i][j] >> k) & 1);
                //     }
                //     std::cout << " ";
                // }
            }
            // std::cout << std::endl;

            {
                // reveal
                bshares.resize(nParties);
                bshares[0] = oc::BitVector(this->bshare.data(), size);
                for (i = 1; i < nParties; i++)
                {
                    bshares[i] = oc::BitVector(brecv[i].data(), size);
                    bshares[0] ^= bshares[i];
                }
                // std::cout << "reveal ";
                // for (j = 0; j < size; j++)
                // {
                //     std::cout << bshares[0][j];
                // }
                // std::cout << std::endl;

                // generate arith share
                ashares.resize(nParties);
                ashares[0].resize(size, 1);
                for (j = 0; j < size; j++)
                {
                    ashares[0](j) = bshares[0][j];
                    // std::cout << ashares[0](j) << " ";
                }
                // std::cout << std::endl;
                // std::cout << "generate share " << std::endl;
                for (i = 1; i < nParties; i++)
                {
                    ashares[i].resize(size, 1);
                    pPrng.get<u64>(ashares[i]);
                    // std::cout << "i " << i << ": ";
                    for (j = 0; j < size; j++)
                    {
                        ashares[0](j) -= ashares[i](j);
                        // std::cout << ashares[0](j) << " ";
                    }
                    // std::cout << std::endl;
                }
                // send to parties
                for (i = 1; i < nParties; i++)
                {
                    MC_AWAIT(socks[i].send(ashares[i]));
                }
            }
        }
        else // send to trust party
        {
            // mask rb
            MC_AWAIT(socks[p].send(bshare));
            // MC_AWAIT(socks[p].send(coproto::copy(data)));

            // std::cout << "send" << std::endl;

            ashares.resize(1);
            ashares[0].resize(size, 1);
            MC_AWAIT(socks[p].recv(ashares[0]));
        }
        // remove mask r
        ashare.setAshare(ashares[0]);
        MC_END();
    }

    void BoolShare::Convert2A(AShare &ashare)
    {
        macoro::sync_wait(Convert2ACo(ashare));
    }

    Proto AShare::RevealCo(oc::Matrix<u64> &plain)
    {
        MC_BEGIN(Proto, this, &plain, p = myIdx, socks = psocks, i = int{}, size = this->ashare.size(), recvdata = std::vector<oc::Matrix<u64>>{});
        // std::cout << p << std::endl;
        recvdata.resize(nParties);
        for (i = 0; i < nParties; i++)
        {
            if (i != p)
            {
                MC_AWAIT(socks[i].send(ashare));
                recvdata[i].resize(row, col);
                MC_AWAIT(socks[i].recv(recvdata[i]));
                // std::cout << "i " << i << ": ";
            }
        }
        // reveal
        recvdata[p] = oc::Matrix<u64>(ashare);
        plain.resize(row, col, oc::AllocType::Zeroed);
        for (i = 0; i < nParties; i++)
        {
            for (size_t j = 0; j < size; j++)
            {
                plain(j) += recvdata[i](j);
            }
            // std::cout << "i " << i << " ";
        }
        MC_END();
    }

    void AShare::Reveal(oc::Matrix<u64> &plain)
    {
        macoro::sync_wait(RevealCo(plain));
    }

    Proto AShare::RevealCo()
    {
        MC_BEGIN(Proto, this, p = myIdx, socks = psocks, i = int{}, size = this->ashare.size(), recvdata = std::vector<oc::Matrix<u64>>{});
        // std::cout << p << std::endl;
        recvdata.resize(nParties);
        for (i = 0; i < nParties; i++)
        {
            if (i != p)
            {
                MC_AWAIT(socks[i].send(ashare));
                recvdata[i].resize(row, col);
                MC_AWAIT(socks[i].recv(recvdata[i]));
                // std::cout << "i " << i << ": ";
            }
        }
        // reveal
        recvdata[p] = oc::Matrix<u64>(ashare);
        for (i = 1; i < nParties; i++)
        {
            for (size_t j = 0; j < size; j++)
            {
                recvdata[0](j) += recvdata[i](j);
            }
            // std::cout << "i " << i << " ";
        }

        if (recvdata[0].cols() == 1)
        {
            std::cout << "size " << size << ":";
            for (size_t j = 0; j < recvdata[0].size(); j++)
            {
                std::cout << recvdata[0](j) << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            for (size_t i = 0; i < recvdata[0].rows(); i++)
            {
                for (size_t j = 0; j < recvdata[0].cols(); j++)
                {
                    std::cout << recvdata[0](i, j) << " ";
                }
                std::cout << std::endl;
            }
        }
        MC_END();
    }

    void AShare::RevealPrint()
    {
        macoro::sync_wait(RevealCo());
    }

    // Proto AShare::MSB(BoolShare &bshare)
    // {
    //     // TO_DO
    // }

    // offline: plain mask r
    Proto AShare::BiggerPlainCo(u64 c, oc::Matrix<bool> &result)
    {
        // every party has the plain r
        // offline
        oc::Matrix<u64> rs(ashare.size(), 1);
        for (size_t j = 0; j < ashare.size(); j++)
        {
            rs(j) = 1;
        }

        // online
        MC_BEGIN(Proto, this, &c, &result, msb = 0x8000000000000000, p = trustp, socks = psocks, i = int{}, size = this->ashare.size(), rs, recvdata = std::vector<oc::Matrix<u64>>{});
        // std::cout << msb << " " << size << std::endl;
        if (myIdx == trustp)
        {
            // mask value
            recvdata.resize(nParties);
            recvdata[0].resize(row, col);
            for (i = 0; i < size; i++)
            {
                recvdata[0](i) = (ashare(i) - c) * rs(i);
            }

            /**trust party**/
            for (i = 1; i < nParties; i++)
            {
                recvdata[i].resize(row, col);
                MC_AWAIT(socks[i].recv(recvdata[i]));
                // std::cout << "i " << i << ": ";
            }

            // reveal
            // recvdata[0] = oc::Matrix<u64>(ashare);
            for (i = 1; i < nParties; i++)
            {
                for (size_t j = 0; j < size; j++)
                {
                    recvdata[0](j) += recvdata[i](j);
                }
                // std::cout << "i " << i << " ";
            }
            // std::cout << std::endl;

            for (size_t j = 0; j < size; j++)
            {
                result(j) = (recvdata[0](j) & msb) ? false : true;
                // std::cout << recvdata[0](j) << " " << result(j) << std::endl;
            }
            // send to other party
            for (i = 1; i < nParties; i++)
            {
                MC_AWAIT(socks[i].send(result));
            }
        }
        else
        {
            // send to trust party
            recvdata.resize(1);
            recvdata[0].resize(row, col);
            for (i = 0; i < size; i++)
            {
                recvdata[0](i) = ashare(i) * rs(i);
            }
            MC_AWAIT(socks[p].send(recvdata[0]));
            MC_AWAIT(socks[p].recv(result));
        }

        // remove mask r
        for (size_t j = 0; j < size; j++)
        {
            // std::cout << rs(j) << " " << result(j);
            result(j) = rs(j) > 0 ? result(j) : !result(j);
            // std::cout << " " << result(j) << std::endl;
        }
        MC_END();
    }

    // Proto AShare::EqualZeroCo(oc::Matrix<bool> &result){
    //     oc::Matrix<u64> value;
    //     Reveal(value);
    //     result.resize(row, col);
    //     // std::cout << value.size() << " "<< size <<" "<<result.size()<<std::endl;
    //     for(size_t i = 0;i<size;i++){
    //         // std::cout << value(i) << std::endl;
    //         result(i) = (value(i)==0);
    //     }
    // }

    void AShare::EqualZero(AShare &result)
    {
        oc::Matrix<u64> value;
        Reveal(value);
        result.resize(row, col);
        // std::cout << value.size() << " "<< size <<" "<<result.size()<<std::endl;
        for (size_t i = 0; i < size; i++)
        {
            // std::cout << value(i) << std::endl;
            value(i) = (value(i) == 0);
        }
        if (myIdx == trustp)
        {
            result.setAshare(value);
        }
    }

    // void AShare::EqualZero(BoolShare &result){
    //     oc::Matrix<u64> value;
    //     Reveal(value);
    //     // result.resize(row, col);
    //     // std::cout << value.size() << " "<< size <<" "<<result.size()<<std::endl;
    //     for(size_t i = 0;i<size;i++){
    //         // std::cout << value(i) << std::endl;
    //         value(i)=(value(i)==0);
    //     }
    // }

    void AShare::BiggerPlain(u64 c, oc::Matrix<bool> &result)
    {
        macoro::sync_wait(BiggerPlainCo(c, result));
    }

    void AShare::Sum(AShare &result)
    {
        u64 sum = 0;
        for (size_t i = 0; i < ashare.size(); i++)
        {
            sum += ashare(i);
        }
        oc::Matrix<u64> temp(1, 1);
        temp(0) = sum;
        result.setAshare(temp);
    }

    void AShare::GenSecrets(std::vector<oc::Matrix<u64>> &secrets)
    {
        secrets.resize(nParties);
        secrets[0] = ashare;
        for (size_t i = 1; i < nParties; i++)
        {
            secrets[i].resize(row, col);
            pPrng.get<u64>(secrets[i]);
            for (size_t j = 0; j < size; j++)
            {
                secrets[0](j) -= secrets[i](j);
            }
        }
    }

    void AShare::GenSecrets(std::vector<oc::Matrix<u64>> &secrets, size_t n)
    {
        secrets.resize(n);
        secrets[0] = ashare;
        for (size_t i = 1; i < n; i++)
        {
            secrets[i].resize(row, col);
            pPrng.get<u64>(secrets[i]);
            for (size_t j = 0; j < size; j++)
            {
                secrets[0](j) -= secrets[i](j);
            }
        }
    }

    // ss the ashare to all parties
    Proto AShare::SecretShareCo(size_t id)
    {
        // generate share
        if (myIdx == id)
        {

            // send to other
            MC_BEGIN(Proto, this, chls = psocks, i = size_t{}, secrets = std::vector<oc::Matrix<u64>>{});
            // std::vector<oc::Matrix<u64>> secrets;
            GenSecrets(secrets);
            for (i = 0; i < nParties; i++)
            {
                if (i != myIdx)
                {
                    // MC_AWAIT(chls[i].send(secrets[i]));
                    // std::cout << secrets[i].rows() << " " << secrets[i].cols() << std::endl;
                    MC_AWAIT(chls[i].send(secrets[i]));
                }
                else
                {
                    ashare = secrets[i];
                }
            }
            MC_END();
        }
        else
        {
            // check size
            MC_BEGIN(Proto, this, chls = psocks, chl = psocks[id]);
            // std::cout << ashare.rows() << " " << ashare.cols() << std::endl;
            // secret.resize(row, col);
            MC_AWAIT(chl.recv(ashare));
            // ashare = secret;
            MC_END();
        }
    }

    void AShare::SecretShare(size_t id)
    {
        // std::vector<u64> info;
        // std::cout << "size" << std::endl;
        // macoro::sync_wait(SecretShareSizeCo(id, info));
        // if (myIdx != id)
        // {
        //     row = info[0];
        //     col = info[1];
        //     std::cout << row << " " << col << std::endl;
        //     ashare.resize(row, col);
        // }else{
        // std::cout << "size " << row << " " << col << std::endl;
        // }
        macoro::sync_wait(SecretShareCo(id));
    }

    Proto AShare::SecretShareSizeCo(size_t id, std::set<size_t> ids)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, ids, chls = psocks,
                 info = std::vector<u64>{},
                 iditer = std::set<size_t>::iterator{});
        info.resize(2);
        if (myIdx == id)
        {
            for (iditer = ids.begin(); iditer != ids.end(); ++iditer)
            {
                info[0] = row;
                info[1] = col;
                MC_AWAIT(chls[*iditer].send(info));
                // std::cout << ids[i] << " send size " << info[0] << " " << info[1] << std::endl;
                // MC_AWAIT(chls[ids[i]].send(ashare.cols()));
            }
        }
        else if (ids.find(myIdx) != ids.end())
        {
            // MC_AWAIT(chls[id].recv(info));
            // macoro::sync_wait(chls[id].recv(info));
            MC_AWAIT(chls[id].recv(info));
        }
        MC_END();
    }

    Proto AShare::SecretShareSizeCo(size_t id, std::vector<u64> &info)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, chls = psocks, &info);
        info.resize(2);
        if (myIdx == id)
        {
            for (i = 0; i < nParties; ++i)
            {
                if (i != myIdx)
                {
                    info[0] = row;
                    info[1] = col;
                    MC_AWAIT(chls[i].send(info));
                    // std::cout << ids[i] << " send size " << info[0] << " " << info[1] << std::endl;
                    // MC_AWAIT(chls[ids[i]].send(ashare.cols()));
                }
            }
        }
        else
        {
            // MC_AWAIT(chls[id].recv(info));
            // macoro::sync_wait(chls[id].recv(info));
            MC_AWAIT(chls[id].recv(info));

            // row = info[0];
            // col = info[1];
            // std::cout << "recv size " << row << " " << col << std::endl;
            // ashare.resize(this->row, this->col);
        }
        MC_END();
    }

    void AShare::SecretShareSize(size_t id)
    {
        std::vector<u64> info;
        macoro::sync_wait(SecretShareSizeCo(id, info));
        if (myIdx != id)
        {
            row = info[0];
            col = info[1];
            std::cout << "recv size " << row << " " << col << std::endl;
            ashare.resize(this->row, this->col);
        }
    }

    void AShare::SecretShareSize(size_t id, std::set<size_t> ids)
    {
        macoro::sync_wait(SecretShareSizeCo(id, ids));
    }

    Proto AShare::SecretShareCo(size_t id, std::set<size_t> ids)
    {
        // generate share
        if (myIdx == id)
        {
            // std::cout << "ss size" << secrets.size() << std::endl;
            // for (int k = 0; k < secrets.size(); k++)
            // {
            //     std::cout << k << " " << secrets[k].size() << ":";
            //     for (size_t j = 0; j < secrets[k].size(); j++)
            //     {
            //         std::cout << +secrets[k](j) << " ";
            //     }
            //     std::cout << std::endl;
            // }
            // std::cout << "ss size" << secrets.size() << std::endl;

            // send to other
            MC_BEGIN(Proto, this, id, chls = psocks, i = size_t{}, ids,
                     iditer = std::set<size_t>::iterator{},
                     secrets = std::vector<oc::Matrix<u64>>{});

            // std::vector<oc::Matrix<u64>> secrets;
            // std::cout << ids.size() << std::endl;
            GenSecrets(secrets, ids.size() + 1);
            i = 0;
            for (iditer = ids.begin(); iditer != ids.end(); ++iditer)
            {
#ifdef DEBUG_INFO
                // std::cout << "send ss to " << *iditer << " , size is " << secrets[i].size() << std::endl;
#endif
                MC_AWAIT(chls[*iditer].send(secrets[i]));
                ++i;
            }

            ashare = secrets[i];
            MC_END();
        }
        else
        {
            if (ids.find(myIdx) != ids.end())
            {
                // check size
                MC_BEGIN(Proto, id, this, chl = psocks[id]);
                // ashare.resize(ashare.rows(), ashare.cols());
                MC_AWAIT(chl.recv(ashare));
#ifdef DEBUG_INFO
                // std::cout << "recv ss from " << id << " , size is " << ashare.size() << std::endl;
#endif
                // ashare = secret;
                MC_END();
            }
        }
    }

    void AShare::SecretShare(size_t id, std::set<size_t> ids)
    {
// SecretShareSize(id, ids);
#ifdef DEBUG_INFO
        // std::cout << row << " " << col << std::endl;
#endif

        macoro::sync_wait(SecretShareCo(id, ids));
    }

    Proto AShare::ShuffleOFFCo(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier)
    {
        MC_BEGIN(Proto, this, &r, &pie, &pie_inv, &pier,
                 rmat = Matrix<u64>{},
                 piemat = Matrix<u64>{},
                 piemulrmat = Matrix<u64>{},
                 piemat_inv = Matrix<u64>{});
        rmat.resize(size, 1);                                 // mask randomness
        piemat.resize(size, size, oc::AllocType::Zeroed);     // pie
        piemulrmat.resize(size, 1);                           // pie(r)
        piemat_inv.resize(size, size, oc::AllocType::Zeroed); // pie^-1
        if (myIdx == trustp)                                  // get value
        {
            pPrng.get<u64>(rmat);
            int arr[size]; // the random order of pie
            // get the value of pie
            for (size_t i = 0; i < size; i++)
            {
                arr[i] = i;
            }
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(arr, arr + size, g);

            for (size_t i = 0; i < size; i++)
            {
                piemat[i][arr[i]] = 1;
                piemat_inv[arr[i]][i] = 1;    // pie^(-1)
                piemulrmat(i) = rmat(arr[i]); // pie*r
                // std::cout << arr[i] << " ";
            }
            // std::cout << std::endl;
        }
        r.setAshare(rmat);
        pie.setAshare(piemat);
        pie_inv.setAshare(piemat_inv);
        pier.setAshare(piemulrmat);
        r.SecretShareCo(trustp);
        pie.SecretShareCo(trustp);
        pie_inv.SecretShareCo(trustp);
        pier.SecretShareCo(trustp);
        // macoro::sync_wait(r.SecretShareCo(trustp));
        // macoro::sync_wait(pie.SecretShareCo(trustp));
        // macoro::sync_wait(pie_inv.SecretShareCo(trustp));
        // macoro::sync_wait(pier.SecretShareCo(trustp));

        // std::cout << "r ";
        // r.RevealPrint();
        // std::cout << "pie ";
        // pie.RevealPrint();
        // std::cout << "pie inv ";
        // pie_inv.RevealPrint();
        // std::cout << "pie*r ";
        // pier.RevealPrint();
        MC_END();
    }

    void AShare::ShuffleOFF(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier)
    {
        if (OFFLINE)
        {
            ShuffleOFFCo(r, pie, pie_inv, pier);
        }
        else
        {
            if (myIdx == 0)
            {
                // tricky offline randomness
                oc::Matrix<u64> piemat(size, size, oc::AllocType::Zeroed);
                oc::Matrix<u64> pieinvmat(size, size, oc::AllocType::Zeroed);
                for (size_t i = 0; i < size; i++)
                {
                    piemat(i, i) = 1;
                    pieinvmat(i, i) = 1;
                }
                pie.setAshare(piemat);
                pie_inv.setAshare(pieinvmat);
            }
        }
    }

    Proto AShare::ShuffleOnCo(AShare &result, const AShare &r, const AShare &pie, const AShare &pier)
    {
        // offline: <r> <pie(r)>
        // tee generate r and pie, caculate pie(r), secret share to parties
        MC_BEGIN(Proto, this, &result, &r, &pie, &pier,
                 rx = AShare{size}, rx_plain = oc::Matrix<u64>{size, 1});

        // AShare r;
        // AShare pie;
        // AShare pie_inv;
        // AShare pier;
        // pieinv.resize(size, size);
        // if (OFFLINE)
        // {
        //     macoro::sync_wait(ShuffleOFF(r, pie, pieinv, pier));
        // }
        // else
        // {
        // }
        // std::cout << "end" << std::endl;
        // online:
        // step1 : reveal(<x>+<r>)

        // AShare rx(size);
        // oc::Matrix<u64> rx_plain(size, 1);
        rx = *this + r;
        // rx.RevealPrint();
        rx.Reveal(rx_plain);
        // rx.RevealPrint();

        result = pie * rx_plain - pier; // pie(x+r) - pie(r) = pie(x)
        // std::cout << "result ";
        // result.RevealPrint();
        MC_END();
    }

    void AShare::ShuffleOn(AShare &result, const AShare &r, const AShare &pie, const AShare &pier)
    {
        macoro::sync_wait(ShuffleOnCo(result, r, pie, pier));
    }

    void AShare::ShuffleDimOFF(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier)
    {
        size_t piesize = sqrt(sqrt(size));
        r.resize(size, 1);
        pier.resize(size, 1);
        if (OFFLINE)
        {
        }
        else
        {
#ifdef DEBUG_INFO
            std::cout << size << " " << piesize << std::endl;
#endif
            Matrix<u64> piemat(size, piesize << 2);
            size_t pp = piesize + piesize;
            size_t ppp = pp + piesize;
            if (myIdx == 0)
            {
                // size_t pp = piesize * piesize;
                // size_t ppp = pp * piesize;
                size_t temp;
                for (size_t i = 0; i < size; i++)
                {
                    piemat(i, i % piesize) = 1;
                    temp = i / piesize;
                    piemat(i, piesize + temp % piesize) = 1;
                    temp = temp / piesize;
                    piemat(i, pp + temp % piesize) = 1;
                    temp = temp / piesize;
                    piemat(i, ppp + temp % piesize) = 1;
                }
            }
            pie.setAshare(piemat);
            pie_inv.setAshare(piemat);
        }
    }

    Proto AShare::ShuffleCo(AShare &result, AShare &pieinv)
    {
        // offline: <r> <pie(r)>
        // tee generate r and pie, caculate pie(r), secret share to parties
        MC_BEGIN(Proto, this, &result, &pieinv, r = AShare{size, 1}, pie = AShare{size, size}, pier = AShare{size, size},
                 rx = AShare{size}, rx_plain = oc::Matrix<u64>{size, 1});

        // AShare r;
        // AShare pie;
        // AShare pie_inv;
        // AShare pier;
        // pieinv.resize(size, size);
        // if (OFFLINE)
        // {
        //     macoro::sync_wait(ShuffleOFF(r, pie, pieinv, pier));
        // }
        // else
        // {
        // }
        ShuffleOFF(r, pie, pieinv, pier);
        std::cout << "off" << std::endl;
        // online:
        // step1:
        // reveal(<x> + <r>)

        // AShare rx(size);
        // oc::Matrix<u64> rx_plain(size, 1);
        rx = *this + r;
        // rx.RevealPrint();
        rx.Reveal(rx_plain);
        std::cout << "rx" << std::endl;
        // rx.RevealPrint();

        result = pie * rx_plain - pier; // pie(x+r) - pie(r) = pie(x)
        // std::cout << "result ";
        // result.RevealPrint();
        MC_END();
    }

    void AShare::ShuffleDim(AShare &result, AShare &pieinv)
    {
        size_t piesize = sqrt(sqrt(size));
        AShare r;
        AShare pier;
        AShare pie;
        AShare pieInv;
        ShuffleDimOFF(r, pie, pieInv, pier);
#ifdef DEBUG_INFO
        std::cout << "off" << std::endl;
        pie.RevealPrint();
        pieInv.RevealPrint();
#endif
        // MC_BEGIN(Proto, this, &result, &pieinv, &r, &pie,
        //          &pier, rx = AShare{size, 1}, rx_plain = oc::Matrix<u64>{size, 1});
        // online:
        // step1:
        // reveal(<x> + <r>)
        r += *this;
        oc::Matrix<u64> rx_plain;
        r.Reveal(rx_plain);
#ifdef DEBUG_INFO
        std::cout << "rx" << std::endl;
        r.RevealPrint();
#endif

        result = pie * rx_plain - pier; // pie(x+r) - pie(r) = pie(x)
        // std::cout << "result ";
        // result.RevealPrint();
        // MC_END();
    }

    void AShare::ShuffleNetOFF(AShare &r, AShare &pier, AShare &pie_net)
    {
        r.resize(row, col);
        pier.resize(row, col);
        // ShuffleNetOFF(pier);
    }

    void AShare::ShuffleNetOFF(std::vector<AShare> &pie)
    {
        // 12       16           20           24
        // 5256     85196        1380625      22369621
        // 60181    1317856      26904089     525798781
        // 23       31           39           48
        u64 pcol;
        u64 prow = ceil(size / 2.0);
        switch (size)
        {
        case 5256:
            pcol = 23;
            break;
        case 85196:
            pcol = 31;
            break;
        case 1380625:
            pcol = 39;
            break;
        case 22369621:
            pcol = 48;
            break;
        default:
            pcol = ceil(size * (log(size) / log(2)) - 0.91 * size + 1) / prow + 1;
        }
#ifdef DEBUG_INFO
        std::cout << "col " << pcol << std::endl;
#endif
        for (size_t i = 0; i < pcol; i++)
        {
            pie.push_back(AShare(prow, 1));
        }
    }

    // void AShare::ToShuffle(AShare &result, const AShare plain){

    // }
    //     void mul(Matrix<u64> &ashare, const Matrix<u64> &c)
    //     {
    //         assert(ashare.rows() == c.rows() && ashare.cols()==1 && c.cols()==1);
    // #ifdef DEBUG_INFO
    //         std::cout << row << " " << col << " " << c.row << " " << c.col << std::endl;
    // #endif
    //         u64 row = ashare.rows();
    //             // offline r1,r2, r1r2
    //             Matrix<u64> r1(row, 1);
    //             Matrix<u64> r2(row, 1);
    //             Matrix<u64> r12(row, 1);

    //             Matrix<u64> emat, fmat;
    //             // (ashare + r1).Reveal(emat);
    //             // (c + r2).Reveal(fmat);

    // #ifdef DEBUG_INFO
    //             std::cout << "e\n";
    //             for (size_t i = 0; i < emat.rows(); i++)
    //             {
    //                 for (size_t j = 0; j < emat.cols(); ++j)
    //                 {
    //                     std::cout << emat(i, j) << " ";
    //                 }
    //                 std::cout << std::endl;
    //             }
    //             std::cout << "f\n";
    //             for (size_t i = 0; i < fmat.rows(); i++)
    //             {
    //                 for (size_t j = 0; j < fmat.cols(); ++j)
    //                 {
    //                     std::cout << fmat(i, j) << " ";
    //                 }
    //                 std::cout << std::endl;
    //             }
    // #endif

    //             // mul: ef - r1 * f - r2 * e + r1 * r2
    //             // Matrix<u64> result(row, 1);
    //             u64 x, e, f;
    //             if (myIdx == 0)
    //             {

    //                 for (size_t j = 0; j < row; ++j)
    //                 {
    //                     e = emat(j);
    //                     f = fmat(j);
    //                     ashare(j) = e * f - r1(j) * f - r2(j) * e + r12(j);
    // #ifdef DEBUG_INFO
    //                     std::cout << result(j) << " ";
    // #endif
    //                 }
    // #ifdef DEBUG_INFO
    //                 std::cout << "\n";
    // #endif
    //             }
    //             else
    //             {

    //                 for (size_t j = 0; j < row; ++j)
    //                 {
    //                     e = emat(j);
    //                     f = fmat(j);
    //                     ashare(j) = r1(j) * f - r2(j) * e + r12(j);
    // #ifdef DEBUG_INFO
    //                     std::cout << result(j) << " ";
    // #endif
    //                 }
    // #ifdef DEBUG_INFO
    //                 std::cout << "\n";
    // #endif
    //             }
    //         }

    void AShare::ShuffleNet(AShare &result, std::vector<AShare> &pie)
    {
        // result.resize(row, col);
        // offline generate network
        // AShare r;
        // AShare pier;
        ShuffleNetOFF(pie);
        // #ifdef DEBUG_INFO
        std::cout << "off " << this->row << " " << pie.size() << std::endl;
        // #endif
        // assert(this->row == pie.rows() * 2);
        // assert(.cols() == 1);
        // oc::Matrix<u64> data;
        oc::Matrix<u64> indata(row, col);
        oc::Matrix<u64> outdata = ashare;
        u64 temp;
        u64 p0;
        u64 p1;
        u64 midrow = row >> 1;
        u64 endrow = midrow - 1;
        AShare gates;
        oc::Matrix<u64> gatesminus(midrow, 1);
        // if (this->row == pie.rows() * 2) // even
        // {
        // first column
        u64 k = 0;
        for (size_t j = 0; j < row; j += 2, ++k)
        {
            gatesminus(k) = (ashare(j + 1) - ashare(j)); // minus
        }
        gates = gatesminus;
        std::cout << "before mul\n";
        gates *= pie[0]; // mul
        std::cout << "mul\n";

        k = 0;
        for (size_t j = 0; j < row; j += 2, ++k)
        {
            outdata(j) += gates(k);
            outdata(j + 1) -= gates(k);
        }
#ifdef DEBUG_INFO
        std::cout << "0: ";
        for (size_t j = 0; j < row; ++j)
        {
            std::cout << outdata(j) << " ";
        }
        std::cout << std::endl;
#endif

        // from second column
        for (size_t i = 1; i < pie.size(); ++i)
        {
            std::cout << i << std::endl;
            k = 0;
            gatesminus(0) = (outdata(2) - outdata(0)); // first row
            indata(k++) = outdata(0);
            indata(k++) = outdata(2);

            p0 = 1;
            p1 = 4;
            for (size_t j = 1; j < endrow; j++, p0 += 2, p1 += 2)
            {
                gatesminus(j) = (outdata(p1) - outdata(p0));
                indata(k++) = outdata(p0);
                indata(k++) = outdata(p1);
            }
            p1--;
            gatesminus(endrow) = (outdata(p1) - outdata(p0)); // last row
            indata(k++) = outdata(p0);
            indata(k++) = outdata(p1);

            gates.setAshare(gatesminus);
            gates *= pie[i]; // mul
            k = 0;
            // outdata = indata;
            for (size_t j = 0; j < row; j += 2, ++k)
            {
                outdata(j) = indata(j) + gates(k);
                outdata(j + 1) = indata(j + 1) - gates(k);
            }
#ifdef DEBUG_INFO
            std::cout << i << ":";
            for (size_t j = 0; j < row; ++j)
            {
                std::cout << outdata(j) << " ";
            }
            std::cout << std::endl;
#endif
        }
        // }
        // else // odd
        // {
        // }
        result.setAshare(outdata);
    }

    void AShare::ToShuffle(AShare &result, const oc::Matrix<u64> plain)
    {
        assert(plain.rows() == row * 2 || plain.rows() == row * 2 - 1);
        assert(plain.cols() == 1);
        oc::Matrix<u64> indata = plain;
        oc::Matrix<u64> outdata(plain.rows(), 1);
        u64 temp;
        u64 p0;
        u64 p1;
        u64 endrow = row - 1;
        if (plain.rows() == row * 2)
        {                                                // even
            for (size_t j = 0; j < plain.rows(); j += 2) // first column
            {
                temp = ashare(j / 2, 0) * (plain(j + 1) - plain(j));
                indata(j) += temp;
                indata(j + 1) -= temp;
                std::cout << indata(j) << " " << indata(j + 1) << " ";
            }
            std::cout << std::endl;
            for (size_t i = 1; i < col; ++i) // from second column
            {
                temp = ashare(0, i) * (indata(2) - indata(0)); // first row
                outdata(0) = indata(0) + temp;
                outdata(1) = indata(2) - temp;
                p0 = 1;
                p1 = 4;
                for (size_t j = 1; j < endrow; j++, p0 += 2, p1 += 2)
                {
                    // p1 = p0 + 3;
                    temp = ashare(j, i) * (indata(p1) - indata(p0));
                    outdata(p0 + 1) = indata(p0) + temp;
                    outdata(p0 + 2) = indata(p1) - temp;
                }
                p1--;
                temp = ashare(endrow, i) * (indata(p1) - indata(p0)); // last row
                outdata(p0 + 1) = indata(p0) + temp;
                outdata(p0 + 2) = indata(p1) - temp;
                indata = outdata;
                std::cout << i << " : ";
                for (size_t j = 0; j < indata.rows(); j++)
                {
                    std::cout << indata(j) << " ";
                }
                std::cout << std::endl;
            }
        }
        else // odd
        {
            u64 prow = plain.rows() - 1;
            u64 prowminus = prowminus;
            for (size_t j = 0; j < endrow; ++j) // first column
            {
                temp = ashare(j, 0) * (plain(j + 1) - plain(j));
                indata(j) = plain(j) + temp;
                indata(j + 1) = plain(j + 1) - temp;
            }
            temp = ashare(endrow) * (plain(prow) - indata(prowminus));
            indata(prowminus) = indata(prowminus) + temp;
            indata(prow) = plain(prow) - temp;
            for (size_t i = 1; i < col; ++i) // from second column
            {
                temp = ashare(0, i) * (indata(2) - indata(0)); // first row
                outdata(0) = indata(0) + temp;
                outdata(1) = indata(2) - temp;
                p0 = 1;
                for (size_t j = 1; j < endrow; ++j, p0 += 2)
                {
                    p1 = p0 + 3;
                    temp = ashare(j, i) * (indata(p1) - indata(p0));
                    outdata(p0 + 1) = indata(p0) + temp;
                    outdata(p0 + 2) = indata(p1) - temp;
                }
                temp = ashare(endrow, i) * (indata(prow) - outdata(prowminus)); // last row
                outdata(prowminus) = outdata(prowminus) + temp;
                outdata(prow) = indata(prow) - temp;
                indata = outdata;
            }
        }
        result.setAshare(outdata);
    }

    void AShare::ToShuffleInv(AShare &result, const oc::Matrix<u64> plain)
    {
        assert(plain.rows() == row * 2 || plain.rows() == row * 2 - 1);
        assert(plain.cols() == 1);
        oc::Matrix<u64> indata = plain;
        oc::Matrix<u64> outdata(plain.rows(), 1);
        u64 temp;
        u64 p0;
        u64 p1;
        u64 endrow = row - 1;
        u64 pcol = plain.cols() - 1;
        if (plain.rows() == row * 2)
        {                                                // even
            for (size_t j = 0; j < plain.rows(); j += 2) // first column
            {
                temp = ashare(j >> 1, 0) * (plain(j + 1) - plain(j));
                indata(j) += temp;
                indata(j + 1) -= temp;
            }
            for (size_t i = pcol - 1; i >= 0; --i) // from second column
            {
                temp = ashare(0, i) * (indata(2) - indata(0)); // first row
                outdata(0) = indata(0) + temp;
                outdata(1) = indata(2) - temp;
                p0 = 1;
                for (size_t j = 1; j < endrow; ++j, p0 += 2)
                {
                    p1 = p0 + 3;
                    temp = ashare(j, i) * (indata(p1) - indata(p0));
                    outdata(p0 + 1) = indata(p0) + temp;
                    outdata(p0 + 2) = indata(p1) - temp;
                }
                p1++;
                temp = ashare(endrow, i) * (indata(p1) - indata(p0)); // last row
                outdata(p0 + 1) = indata(p0) + temp;
                outdata(p0 + 2) = indata(p1) - temp;
                indata = outdata;
            }
        }
        else // odd
        {
            u64 prow = plain.rows() - 1;
            u64 prowminus = prowminus;
            for (size_t j = 0; j < endrow; ++j) // first column
            {
                temp = ashare(pcol, 0) * (plain(j + 1) - plain(j));
                indata(j) = plain(j) + temp;
                indata(j + 1) = plain(j + 1) - temp;
            }
            temp = ashare(endrow) * (plain(prow) - indata(prowminus));
            indata(prowminus) = indata(prowminus) + temp;
            indata(prow) = plain(prow) - temp;
            for (size_t i = pcol - 1; i >= 0; --i) // from second column
            {
                temp = ashare(0, i) * (indata(2) - indata(0)); // first row
                outdata(0) = indata(0) + temp;
                outdata(1) = indata(2) - temp;
                p0 = 1;
                for (size_t j = 1; j < endrow; ++j, p0 += 2)
                {
                    p1 = p0 + 3;
                    temp = ashare(j, i) * (indata(p1) - indata(p0));
                    outdata(p0 + 1) = indata(p0) + temp;
                    outdata(p0 + 2) = indata(p1) - temp;
                }
                temp = ashare(endrow, i) * (indata(prow) - outdata(prowminus)); // last row
                outdata(prowminus) = outdata(prowminus) + temp;
                outdata(prow) = indata(prow) - temp;
                indata = outdata;
            }
        }
        result.setAshare(outdata);
    }

    void AShare::Shuffle(AShare &result, AShare &pieinv)
    {
        macoro::sync_wait(ShuffleCo(result, pieinv));
    }

    u64 AShare::operator()(u64 i) const
    {
        return ashare(i);
    }

    AShare AShare::operator+(const AShare &share) const
    {
        // AShare result(ashare);
        oc::Matrix<u64> plus(ashare);
        for (size_t i = 0; i < this->ashare.size(); i++)
        {
            plus(i) += share(i);
        }
        return AShare(plus);
    }

    AShare &AShare::operator+=(const AShare &share)
    {
        // oc::Matrix<u64> plus(ashare);
        for (size_t i = 0; i < this->ashare.size(); i++)
        {
            ashare(i) += share(i);
        }
        return *this;
    }

    AShare &AShare::operator-=(const AShare &share)
    {
        for (size_t i = 0; i < this->ashare.size(); i++)
        {
            ashare(i) -= share(i);
        }
        return *this;
    }

    AShare AShare::operator*(const oc::Matrix<u64> &c) const
    {
        // assert(this->ashare.cols() == c.rows());
        if (this->ashare.cols() == c.rows())
        {
            oc::Matrix<u64> mat(ashare.rows(), c.cols());
            // std::cout << ashare.rows() << " " << c.cols() << std::endl;
            // for (int k = 0; k < c.cols(); k++)
            // {
            //     for (size_t j = 0; j < c.rows(); j++)
            //     {
            //         std::cout << c[j][k] << " ";
            //     }
            // }
            // std::cout << std::endl;
            // for (size_t i = 0; i < this->ashare.rows(); i++)
            // {
            //     for (size_t j = 0; j < c.rows(); j++)
            //     {
            //         std::cout << ashare[i][j] << " ";
            //     }
            // }

            u64 temp;
            for (size_t i = 0; i < this->ashare.rows(); i++)
            {
                for (int k = 0; k < c.cols(); k++)
                {
                    temp = 0;
                    for (size_t j = 0; j < c.rows(); j++)
                    {
                        temp += c[j][k] * ashare[i][j];
                        // std::cout << i << " " << k << " " << j << " " << temp << std::endl;
                    }
                    mat[i][k] = temp;
                }
            }
            return AShare(mat);
        }
        else if (this->ashare.rows() == c.rows())
        {
            oc::Matrix<u64> mat(ashare.rows(), c.cols());
            // u64 temp;
            u64 atemp;
            size_t len = ashare.cols() / 4;
            size_t p;
            size_t ccols = c.cols();
            // size_t num = c.rows();
            for (size_t i = 0; i < this->ashare.rows(); i++)
            {
                size_t num = c.rows() / len;
#ifdef DEBUG_INFO
                std::cout << i << " " << len << " " << num << std::endl; // log
#endif
                p = 0;
                u64 data[num][ccols]; // init data
                std::fill(&data[0][0], &data[0][0] + num * ccols, 0);
                // for (int k = 0; k < num; k++)
                // {
                //     for (size_t cl = 0; cl < cclos; cl++)
                //     {
                //         data[k][cl] = 0;
                //     }
                // }

                // caculate
                for (int k = 0; k < num; k++)
                {
                    for (size_t j = 0; j < len; j++)
                    {
                        atemp = ashare[i][j];
                        for (size_t cl = 0; cl < ccols; cl++)
                        {
                            data[k][cl] += atemp * c(p, cl);
                            // std::cout << i << " " << k << " " << j << " " << temp << std::endl;
                        }
                        p++;
                    }
#ifdef DEBUG_INFO
                    for (size_t cl = 0; cl < ccols; cl++)
                    {
                        std::cout << data[k][cl] << " "; // log
                    }
#endif
                    // std::cout << std::endl;
                    // mat[i][k] = temp;
                }
#ifdef DEBUG_INFO
                std::cout << std::endl;
#endif
                int r = 1;
                while (r < 4)
                {
                    p = 0;
                    num = num / len;
                    // std::cout << r << ": " << num << " : ";
                    for (int k = 0; k < num; k++)
                    {
                        // temp = 0;
                        u64 temp[c.cols()];
                        std::fill(temp, temp + ccols, 0);
                        for (size_t j = len * r; j < len * r + len; j++)
                        {
                            atemp = ashare[i][j];
                            for (size_t cl = 0; cl < ccols; cl++)
                            {
                                temp[cl] += atemp * data[p][cl];
                                // std::cout << i << " " << k << " " << j << " " << temp << std::endl;
                            }
                            p++;
                        }
                        std::copy(temp, temp + ccols, data[k]);
#ifdef DEBUG_INFO
                        for (size_t cl = 0; cl < ccols; cl++)
                        {

                            std::cout << temp[cl] << " "; // log
                        }
#endif
                    }
#ifdef DEBUG_INFO
                    std::cout << std::endl;
#endif
                    r++;
                }
#ifdef DEBUG_INFO
                for (size_t cl = 0; cl < ccols; cl++)
                {
                    mat(i, cl) = data[0][cl];
                    std::cout << mat(i, cl) << " "; // log
                }
#endif
            }
            return AShare(mat);
        }
    }

    AShare AShare::operator*(const AShare &c) const
    {
        assert(this->row == c.row);
#ifdef DEBUG_INFO
        std::cout << row << " " << col << " " << c.row << " " << c.col << std::endl;
#endif
        if (col == 1 && c.cols() == 1)
        {
            // offline r1,r2, r1r2
            Matrix<u64> r1(row, 1);
            Matrix<u64> r2(row, 1);
            Matrix<u64> r12(row, 1);

            Matrix<u64> emat, fmat;
            (*this + r1).Reveal(emat);
            (c + r2).Reveal(fmat);

#ifdef DEBUG_INFO
            std::cout << "e\n";
            for (size_t i = 0; i < emat.rows(); i++)
            {
                for (size_t j = 0; j < emat.cols(); ++j)
                {
                    std::cout << emat(i, j) << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "f\n";
            for (size_t i = 0; i < fmat.rows(); i++)
            {
                for (size_t j = 0; j < fmat.cols(); ++j)
                {
                    std::cout << fmat(i, j) << " ";
                }
                std::cout << std::endl;
            }
#endif

            // mul: ef - r1 * f - r2 * e + r1 * r2
            Matrix<u64> result(row, 1);
            u64 x, e, f;
            if (myIdx == 0)
            {

                for (size_t j = 0; j < row; ++j)
                {
                    e = emat(j);
                    f = fmat(j);
                    result(j) = e * f - r1(j) * f - r2(j) * e + r12(j);
#ifdef DEBUG_INFO
                    std::cout << result(j) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
            else
            {

                for (size_t j = 0; j < row; ++j)
                {
                    e = emat(j);
                    f = fmat(j);
                    result(j) = r1(j) * f - r2(j) * e + r12(j);
#ifdef DEBUG_INFO
                    std::cout << result(j) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
            return AShare(result);
        }
    }

    AShare &AShare::operator*=(const AShare &c)
    {
        assert(this->row == c.row);
#ifdef DEBUG_INFO
        std::cout << row << " " << col << " " << c.row << " " << c.col << std::endl;
#endif
        if (col == 1 && c.cols() == 1)
        {
            // offline r1,r2, r1r2
            Matrix<u64> r1(row, 1);
            Matrix<u64> r2(row, 1);
            Matrix<u64> r12(row, 1);

            Matrix<u64> emat, fmat;
            (*this + r1).Reveal(emat);
            (c + r2).Reveal(fmat);

#ifdef DEBUG_INFO
            std::cout << "e\n";
            for (size_t i = 0; i < emat.rows(); i++)
            {
                for (size_t j = 0; j < emat.cols(); ++j)
                {
                    std::cout << emat(i, j) << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "f\n";
            for (size_t i = 0; i < fmat.rows(); i++)
            {
                for (size_t j = 0; j < fmat.cols(); ++j)
                {
                    std::cout << fmat(i, j) << " ";
                }
                std::cout << std::endl;
            }
#endif

            // mul: ef - r1 * f - r2 * e + r1 * r2
            // Matrix<u64> result(row, 1);
            u64 x, e, f;
            if (myIdx == 0)
            {

                for (size_t j = 0; j < row; ++j)
                {
                    e = emat(j);
                    f = fmat(j);
                    ashare(j) = e * f - r1(j) * f - r2(j) * e + r12(j);
#ifdef DEBUG_INFO
                    std::cout << result(j) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
            else
            {

                for (size_t j = 0; j < row; ++j)
                {
                    e = emat(j);
                    f = fmat(j);
                    ashare(j) = r1(j) * f - r2(j) * e + r12(j);
#ifdef DEBUG_INFO
                    std::cout << result(j) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
            return *this;
        }
    }

    // dot product
    // self(m*c) * c(m*1)
    void AShare::DotProduct(AShare &output, const AShare &c)
    {
        assert(this->row == c.row);
#ifdef DEBUG_INFO
        // std::cout << row << " " << col << " " << c.row << " " << c.col << std::endl;
#endif
        // offline r1,r2, r1r2
        Matrix<u64> r1(row, col);
        Matrix<u64> r2(c.row, c.col);
        Matrix<u64> r12[col];
        for (size_t i = 0; i < col; i++)
        {
            r12[i] = Matrix<u64>(c.row, c.col);
        }
        Matrix<u64> emat, fmat;

        (*this + r1).Reveal(emat);
        (c + r2).Reveal(fmat);

#ifdef DEBUG_INFO
        std::cout << "e\n";
        for (size_t i = 0; i < emat.rows(); i++)
        {
            for (size_t j = 0; j < emat.cols(); ++j)
            {
                std::cout << emat(i, j) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "f\n";
        for (size_t i = 0; i < fmat.rows(); i++)
        {
            for (size_t j = 0; j < fmat.cols(); ++j)
            {
                std::cout << fmat(i, j) << " ";
            }
            std::cout << std::endl;
        }
#endif

        // mul: ef - r1 * f - r2 * e + r1 * r2
        Matrix<u64> result(col, c.col);
        u64 x, e, f;
        if (myIdx == 0)
        {
            for (size_t i = 0; i < col; ++i)
            {
                for (size_t k = 0; k < c.col; ++k)
                {
                    x = 0;
                    for (size_t j = 0; j < row; ++j)
                    {
                        e = emat(j, i);
                        f = fmat(j, k);
                        x += e * f - r1(j, i) * f - r2(j, k) * e + r12[i](j, k);
                    }
                    result(i, k) = x;
#ifdef DEBUG_INFO
                    std::cout << result(i, k) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
        }
        else
        {
            for (size_t i = 0; i < col; ++i)
            {
                for (size_t k = 0; k < c.col; ++k)
                {
                    x = 0;
                    for (size_t j = 0; j < row; ++j)
                    {
                        e = emat(j, i);
                        f = fmat(j, k);
                        x += r1(j, i) * f - r2(j, k) * e + r12[i](j, k);
                    }
                    result(i, k) = x;
#ifdef DEBUG_INFO
                    std::cout << result(i, k) << " ";
#endif
                }
#ifdef DEBUG_INFO
                std::cout << "\n";
#endif
            }
        }
        // std::cout << "h\n";
        output.setAshare(result);
    }

    AShare AShare::operator-(const AShare &share) const
    {
        oc::Matrix<u64> minor(ashare);
        for (u64 i = 0; i < this->ashare.size(); i++)
        {
            minor(i) -= share(i);
        }
        return AShare(minor);
    }

    // void AShare::Shuffle(AShare &result, AShare &pieinv)
    // {
    //     macoro::sync_wait(ShuffleCo(result, pieinv));
    // }
}