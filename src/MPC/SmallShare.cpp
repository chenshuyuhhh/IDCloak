#include "SmallShare.h"

namespace volePSI
{
    SmallShare::SmallShare()
    {
    }

    SmallShare::SmallShare(oc::Matrix<u8> ashare)
    {
        this->ashare = ashare;
        this->col = ashare.cols();
        this->row = ashare.rows();
    }

    void SmallShare::SetShare(oc::Matrix<u8> ashare)
    {
        this->ashare = ashare;
        this->col = ashare.cols();
        this->row = ashare.rows();
    }

    // gen n shares
    void SmallShare::GenSecrets(std::vector<oc::Matrix<u8>> &secrets, size_t n)
    {
        secrets.resize(n);
        // std::cout << ashare.rows() << " " << ashare.cols() << std::endl;
        secrets[0] = ashare;
        for (size_t i = 1; i < n; i++)
        {
            secrets[i].resize(ashare.rows(), ashare.cols());
            pPrng.get<u8>(secrets[i]);
            for (size_t j = 0; j < secrets[i].size(); j++)
            {
                secrets[0](j) ^= secrets[i](j);
            }
            // std::cout <<
        }
    }

    void SmallShare::GenSecrets(std::vector<oc::Matrix<u8>> &secrets)
    {
        GenSecrets(secrets, nParties);
    }

    /**
     * id secret share the ashare to all ids
     * id: share party
     */
    Proto SmallShare::SecretShareCo(size_t id, std::vector<size_t> ids)
    {
        // generate share
        if (myIdx == id)
        {
            std::vector<oc::Matrix<u8>> secrets;
            // std::cout << ids.size() << std::endl;
            GenSecrets(secrets, ids.size() + 1);
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
            MC_BEGIN(Proto, this, secrets, chls = psocks, i = size_t{}, ids);
            for (i = 0; i < ids.size(); i++)
            {
                MC_AWAIT(chls[ids[i]].send(secrets[i]));
                // std::cout << "send ss to " << ids[i] << " , size is " << secrets[i].size() << std::endl;
            }
            ashare = secrets[ids.size()];
            MC_END();
        }
        else
        {
            // check size
            MC_BEGIN(Proto, this, chl = psocks[id], id = id);
            // ashare.resize(ashare.rows(), ashare.cols());
            MC_AWAIT(chl.recv(ashare));
            // std::cout << "recv ss from " << id << " , size is " << ashare.size() << std::endl;
            // ashare = secret;
            MC_END();
        }
        // std::cout << "end" << id << " " << ashare.size() << std::endl;
    }

    /**
     * ss the ashare to all parties
     * id: share party
     */
    Proto SmallShare::SecretShareCo(size_t id)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, ids = std::vector<size_t>{});
        // std::vector<size_t> ids;
        for (i = 0; i < nParties; i++)
        {
            if (i != id)
            {
                ids.push_back(i);
            }
        }
        MC_AWAIT(SecretShareCo(id, ids));
        MC_END();
    }

    Proto SmallShare::SecretShareSizeCo(size_t id, std::vector<size_t> ids)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, ids, chls = psocks, info = std::vector<u64>{});
        // share size
        // std::cout << id << std::endl;
        info.resize(2);
        if (myIdx == id)
        {
            for (i = 0; i < ids.size(); i++)
            {
                info[0] = row;
                info[1] = col;
                MC_AWAIT(chls[ids[i]].send(info));
                // std::cout << ids[i] << " send size " << info[0] << " " << info[1] << std::endl;
                // MC_AWAIT(chls[ids[i]].send(ashare.cols()));
            }
        }
        else
        {
            // MC_AWAIT(chls[id].recv(info));
            // macoro::sync_wait(chls[id].recv(info));
            MC_AWAIT(chls[id].recv(info));

            row = info[0];
            col = info[1];
            // std::cout << "recv size " << row << " " << col << std::endl;
            ashare.resize(this->row, this->col);
        }
        // std::cout << "begin ss" << std::endl;
        MC_AWAIT(SecretShareCo(id, ids));
        MC_END();
    }

    Proto SmallShare::SecretShareSizeCo(size_t id)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, ids = std::vector<size_t>{});
        // std::vector<size_t> ids;
        for (i = 0; i < nParties; i++)
        {
            if (i != id)
            {
                ids.push_back(i);
            }
        }
        MC_AWAIT(SecretShareSizeCo(id, ids));
        MC_END();
    }
    void SmallShare::SecretShareSize(size_t id)
    {
        // std::cout << id << std::endl;
        macoro::sync_wait(SecretShareSizeCo(id));
    }

    Proto SmallShare::RevealCo(oc::Matrix<u8> &plain)
    {
        MC_BEGIN(Proto, this, &plain, p = myIdx, socks = psocks, i = int{}, size = this->ashare.size(), recvdata = std::vector<oc::Matrix<u8>>{});
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
        recvdata[p] = oc::Matrix<u8>(ashare);
        plain.resize(row, col, oc::AllocType::Zeroed);
        for (i = 0; i < nParties; i++)
        {
            for (int j = 0; j < size; j++)
            {
                plain(j) ^= recvdata[i](j);
            }
            // std::cout << "i " << i << " ";
        }
        MC_END();
    }

    void SmallShare::Reveal(oc::Matrix<u8> &plain)
    {
        macoro::sync_wait(RevealCo(plain));
    }

    Proto SmallShare::RevealCo()
    {
        MC_BEGIN(Proto, this, p = myIdx, socks = psocks, i = int{}, size = this->ashare.size(), recvdata = std::vector<oc::Matrix<u8>>{});
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
        recvdata[p] = oc::Matrix<u8>(ashare);
        for (i = 1; i < nParties; i++)
        {
            for (int j = 0; j < size; j++)
            {
                recvdata[0](j) ^= recvdata[i](j);
            }
            // std::cout << "i " << i << " ";
        }
        if (recvdata[0].cols() == 1)
        {
            std::cout << "size " << size << ":";
            for (int i = 0; i < recvdata[0].size(); i++)
            {
                std::cout << +recvdata[0](i) << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "size " << size << ":" << std::endl;
            for (int i = 0; i < recvdata[0].rows(); i++)
            {
                for (int j = 0; j < recvdata[0].cols(); j++)
                {
                    std::cout << +recvdata[0](i, j) << " ";
                }
                std::cout << std::endl;
            }
        }
        MC_END();
    }

    void SmallShare::RevealPrint()
    {
        macoro::sync_wait(RevealCo());
    }

    void SmallShare::Print()
    {
        std::cout << "size " << ashare.size() << ": ";
        for (int i = 0; i < ashare.size(); i++)
        {
            std::cout << +ashare(i) << " ";
        }
        std::cout << std::endl;
    }

}