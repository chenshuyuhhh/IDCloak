#ifndef EIEGEN_SOCKET
#define EIEGEN_SOCKET
#include <Eigen/Dense>
#include <cstdint>

// #ifdef EIGEN_CONNECT;
#include "volePSI/Defines.h"

typedef oc::u64 ringsize;
typedef Eigen::Matrix<oc::u64, -1, -1> Mat2d;

#define COMM_TEST

namespace volePSI
{
    void socket_init();
    void socket_del();
    void greet();
    void wait_for_connect(int i, int port);
    void connect_others(int i, const char *ip, int port);
    void send_n(int fd, char *data, uint64_t len);
    void recv_n(int fd, char *data, uint64_t len);
    void send_array(int pid, const Mat2d &mat);
    void recv_array(int pid, Mat2d &mat);
    oc::u64 get_comm_size();
    oc::u64 get_comm_size_one();
}
// #endif
#endif