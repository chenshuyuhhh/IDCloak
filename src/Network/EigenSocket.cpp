#include "EigenSocket.h"

// #ifdef EIGEN_CONNECT;
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <mutex>
#include "thread"
#include "volePSI/Defines.h"
#include <sys/types.h>
#include <unistd.h>

namespace volePSI
{
    std::mutex communication_size_mtx;
    std::mutex communication_size_one_mtx;
    oc::u64 communication_size = 0;
    oc::u64 communication_size_one = 0;
    extern size_t nParties;
    extern size_t myIdx;
    extern size_t lIdx;

    const int socket_buffer_size = 33554432;
    int *socket_fds = new int[nParties];

    void socket_init()
    {
        // #ifdef DEGUG_INFO
        // std::cout << "tcp socket init " << myIdx << " " << nParties << std::endl;
        // #endif
        for (int i = 0; i < nParties; i++)
            socket_fds[i] = 0;
        for (size_t i = 0; i < myIdx; i++)
        {
            oc::u32 port = 10200 + myIdx * 100 + i;
            std::string ip = "0.0.0.0";
            connect_others(i, ip.data(), port);
        }
        for (size_t i = myIdx + 1; i < nParties; i++)
        {
            oc::u32 port = 10200 + i * 100 + myIdx;
            wait_for_connect(i, port);
        }
        greet();
        // communication_size = 0;
        // std::cout << "socket end" << std::endl;
    }

    void socket_del()
    {
        for (int i = 0; i < nParties; i++)
        {
            if (socket_fds[i] != 0 || i != myIdx)
            {
                close(socket_fds[i]);
                socket_fds[i] = 0;
            }
        }
        delete[] socket_fds;
    }

    void socket_updata()
    {
        for (uint i = 0; i < nParties; i++)
        {
            if (i != myIdx)
            {
                send_n(i,0, sizeof(int));
                recv_n(i, 0, sizeof(int));
            }
        }
    }

    void wait_for_connect(int i, int port)
    {
        int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int opt = 1;
        setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
        setsockopt(serv_sock, SOL_SOCKET, SO_RCVBUF, (const char *)&socket_buffer_size, sizeof(int));
        setsockopt(serv_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&socket_buffer_size, sizeof(int));
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        serv_addr.sin_port = htons(port);
        ::bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        listen(serv_sock, 20);
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        // for (int i = myIdx + 1; i < nParties; i++)
        // {
        socket_fds[i] = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        // std::cout << socket_fds[i] << std::endl;
        setsockopt(socket_fds[i], SOL_SOCKET, SO_RCVBUF, (const char *)&socket_buffer_size, sizeof(int));
        setsockopt(socket_fds[i], SOL_SOCKET, SO_SNDBUF, (const char *)&socket_buffer_size, sizeof(int));
        // }
    }

    void connect_others(int i, const char *ip, int port)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in my_addr;
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        // my_addr.sin_port = htons(myport);
        ::bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        // for (int i = 0; i < nParties; i++)
        // {
        //     if (socket_fds[i] == 0)
        //     {
        socket_fds[i] = sock;
        setsockopt(socket_fds[i], SOL_SOCKET, SO_RCVBUF, (const char *)&socket_buffer_size, sizeof(int));
        setsockopt(socket_fds[i], SOL_SOCKET, SO_SNDBUF, (const char *)&socket_buffer_size, sizeof(int));
        // break;
        //     }
        // }
    }

    void greet()
    {
        int *temp_pid = new int[nParties];
        temp_pid[myIdx] = 0;
        for (int i = 0; i < nParties; i++)
        {
            if (socket_fds[i] != 0)
                send_n(socket_fds[i], (char *)&myIdx, 4);
        }
        for (int i = 0; i < nParties; i++)
        {
            int id;
            if (socket_fds[i] != 0)
            {
                recv_n(socket_fds[i], (char *)&id, 4);
                // #ifdef DEGUG_INFO
                // std::cout << "id " << id << std::endl;
                // #endif
                temp_pid[id] = socket_fds[i];
            }
        }
        for (int i = 0; i < nParties; i++)
            socket_fds[i] = temp_pid[i];
        delete[] temp_pid;
    }

    void send_n(int fd, char *data, uint64_t len)
    {
        uint64_t temp = 0;
        while (temp < len)
        {
            temp = temp + write(fd, data + temp, len - temp);
        }

#ifdef COMM_TEST
        communication_size_mtx.lock();
        communication_size = communication_size + len;
        communication_size_mtx.unlock();

        communication_size_one_mtx.lock();
        communication_size_one = communication_size_one + len;
        communication_size_one_mtx.unlock();
#endif
    }

    void recv_n(int fd, char *data, uint64_t len)
    {
        uint64_t temp = 0;
        while (temp < len)
        {
            temp = temp + read(fd, data + temp, len - temp);
        }
#ifdef COMM_TEST
        communication_size_one_mtx.lock();
        communication_size_one = communication_size_one + len;
        communication_size_one_mtx.unlock();
#endif
    }

    void send_array(int pid, const Mat2d &mat)
    {
        send_n(socket_fds[pid], (char *)mat.data(), mat.size() * sizeof(ringsize));
    }

    void recv_array(int pid, Mat2d &mat)
    {
        // int r = mat.rows();
        // int c = mat.cols();
        // send_n(socket_fds[pid], (char *)&r, 4);
        // send_n(socket_fds[pid], (char *)&c, 4);
        recv_n(socket_fds[pid], (char *)mat.data(), mat.size() * sizeof(ringsize));
    }

    oc::u64 get_comm_size()
    {
        return communication_size;
    }

    oc::u64 get_comm_size_one()
    {
        return communication_size_one;
    }
}
// #endif