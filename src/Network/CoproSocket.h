#ifndef COPROTO_SOCKET
#define COPROTO_SOCKET

#include "coproto/Socket/AsioSocket.h"
#include "coproto/Socket/Socket.h"
#include "coproto/Common/Optional.h"
#include "coproto/Common/macoro.h"
#include <boost/asio.hpp>
#include "coproto/config.h"
#include <boost/asio.hpp>

namespace coproto
{
    struct AsioConnectP
    {
        using SocketType = boost::asio::ip::tcp::socket;
        // boost::asio::io_context& mIoc;
        boost::system::error_code mEc;
        SocketType mSocket;
        boost::asio::io_context &mIoc;
        boost::asio::ip::tcp::endpoint mEndpoint;
        macoro::stop_token mToken;
        macoro::optional_stop_callback mReg;

        boost::asio::cancellation_signal mCancelSignal;
        std::atomic<char> mCancellationRequested, mSynchronousFlag, mStarted;
        bool mRetryOnFailure;

        boost::posix_time::time_duration mRetryDelay;
        boost::asio::deadline_timer mTimer;

        coroutine_handle<> mHandle;
        // enum class Status
        //{
        //	Init
        // };

        // std::atomic<Status> mStatus;

        void log(std::string s)
        {
            std::lock_guard<std::mutex> l(::coproto::ggMtx);
            ggLog.push_back("connect::" + s);
        }

        AsioConnectP(
            std::string address,
            boost::asio::io_context &ioc,
            unsigned short port,
            macoro::stop_token token = {},
            bool retryOnFailure = true)
            : mSocket(boost::asio::make_strand(ioc)), mIoc(ioc), mToken(token), mCancellationRequested(0), mSynchronousFlag(0), mStarted(0), mRetryOnFailure(retryOnFailure), mRetryDelay(boost::posix_time::milliseconds(1)), mTimer(ioc)
        {

#ifdef COPROTO_ASIO_LOG
            log("init");
#endif
            // std::cout << port << std::endl;
            auto i = address.find(":");
            boost::asio::ip::tcp::resolver resolver(boost::asio::make_strand(ioc));
            if (i != std::string::npos)
            {
                auto prefix = address.substr(0, i);
                auto posfix = address.substr(i + 1);
                mEndpoint = *resolver.resolve(prefix, posfix);
            }
            else
            {
                mEndpoint = *resolver.resolve(address);
            }
            if (port != 0)
            {
                boost::asio::ip::tcp::endpoint localEndpoint(boost::asio::ip::make_address("127.0.0.1"), port);
                mSocket.open(localEndpoint.protocol());
                mSocket.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
                mSocket.bind(localEndpoint);
            }

            // if (i != std::string::npos)
            // {
            //     auto host = address.substr(0, i);
            //     auto port_str = address.substr(i + 1);
            //     boost::asio::ip::tcp::resolver::query query(host, port_str);
            //     boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
            //     boost::asio::ip::tcp::endpoint remote_endpoint = *it;
            //     mEndpoint = boost::asio::ip::tcp::endpoint(remote_endpoint.address(), port);

            //     // boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(prefix, posfix);

            //     // mEndpoint = boost::asio::ip::tcp::endpoint(endpoints.begin()->endpoint(), port);
            //     //  *resolver.resolve(prefix, posfix)
            // }
            // else
            // {
            //     // mEndpoint = *resolver.resolve(address);
            //     boost::asio::ip::tcp::resolver::query query(address);
            //     boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
            //     boost::asio::ip::tcp::endpoint remote_endpoint = *it;
            //     // Create the endpoint with the desired local port
            //     mEndpoint = boost::asio::ip::tcp::endpoint(remote_endpoint.address(), port);
            // }
        }

        AsioConnectP(const AsioConnectP &) = delete;
        AsioConnectP(AsioConnectP &&a)
            : mEc(a.mEc), mIoc(a.mIoc), mSocket(std::move(a.mSocket)), mEndpoint(std::move(a.mEndpoint)), mToken(std::move(a.mToken)), mCancellationRequested(0), mSynchronousFlag(0), mStarted(0), mRetryOnFailure(a.mRetryOnFailure), mRetryDelay(boost::posix_time::milliseconds(1)), mTimer(std::move(a.mTimer))
        {
            if (a.mStarted)
            {
                std::cout << "AsioConnect can not be moved after it has started. " << COPROTO_LOCATION << std::endl;
                std::terminate();
            }
        }

        bool await_ready()
        {

            mStarted = 1;
            if (mToken.stop_possible())
            {
                if (!mReg)
                {
                    assert(!mCancellationRequested);
                    mReg.emplace(mToken, [this]
                                 {

#ifdef COPROTO_ASIO_LOG
						log("cancel callback, emit");
#endif
						mCancellationRequested = 1;
						mCancelSignal.emit(boost::asio::cancellation_type::partial); });
                }
            }
            return false;
        }
#ifdef COPROTO_CPP20
        void await_suspend(std::coroutine_handle<> h)
        {
            await_suspend(coroutine_handle<>(h));
        }
#endif
        void await_suspend(coroutine_handle<> h)
        {
            mHandle = h;

#ifdef COPROTO_ASIO_LOG
            log("await_suspend");
#endif

            mSocket.async_connect(mEndpoint,
                                  boost::asio::bind_cancellation_slot(
                                      mCancelSignal.slot(),
                                      [this](boost::system::error_code ec)
                                      {

#ifdef COPROTO_ASIO_LOG
                                          log("async_connect callback");
#endif
                                          mEc = ec;

                                          auto f = mSynchronousFlag.exchange(1);

                                          if (f)
                                          {
                                              if (mEc == boost::system::errc::connection_refused && mRetryOnFailure && !mCancellationRequested)
                                              {
                                                  retry();
                                              }
                                              else
                                              {

#ifdef COPROTO_ASIO_LOG
                                                  log("async_connect callback resume");
#endif
                                                  mHandle.resume();
                                              }
                                          }
                                      }));

            if (mCancellationRequested)
            {

#ifdef COPROTO_ASIO_LOG
                log("emit cancellation");
#endif
                mCancelSignal.emit(boost::asio::cancellation_type::partial);
            }
            auto f = mSynchronousFlag.exchange(1);
            // we completed synchronously if f==true;
            // this is needed sure we aren't destroyed
            // before checking if we need to emit
            // the cancellation.
            if (f)
            {
                if (mEc == boost::system::errc::connection_refused && mRetryOnFailure && !mCancellationRequested)
                {
                    retry();
                }
                else
                {

#ifdef COPROTO_ASIO_LOG
                    log("async_connect sync resume");
#endif
                    mHandle.resume();
                }
            }
            // 			}
            // 			else
            // 			{

            // 				mSocket.async_connect(mEndpoint, [this](boost::system::error_code ec) {
            // 					if (ec == boost::system::errc::connection_refused && mRetryOnFailure)
            // 					{

            // #ifdef COPROTO_ASIO_LOG
            // 						log(" "+ std::to_string(ec.value())+ " " + ec.message());
            // #endif
            // 						mTimer.expires_from_now(mRetryDelay);
            // 						mTimer.async_wait(
            // 							[this](error_code ec) {
            // 							mSynchronousFlag = false;
            // 							mRetryDelay = std::min<boost::posix_time::time_duration>(
            // 								mRetryDelay * 2,
            // 								boost::posix_time::milliseconds(1000)
            // 								);

            // #ifdef COPROTO_ASIO_LOG
            // 							log("retry");
            // #endif
            // 							mSocket.close();
            // 							await_suspend(mHandle);
            // 							});
            // 					}
            // 					else
            // 					{
            // 						mEc = ec;
            // #ifdef COPROTO_ASIO_LOG
            // 						log("async_connect callback resume: " + ec.message());
            // #endif
            // 						mHandle.resume();
            // 					}
            // 					});
            // 			}
        }

        AsioSocket await_resume()
        {
            if (mEc)
                throw std::system_error(mEc);

            boost::asio::ip::tcp::no_delay option(true);
            mSocket.set_option(option);
            return {std::move(mSocket)};
        }

        void retry()
        {
            mTimer.expires_from_now(mRetryDelay);
            mTimer.async_wait(
                boost::asio::bind_cancellation_slot(
                    mCancelSignal.slot(),
                    [this](error_code ec)
                    {
                        mSynchronousFlag = 0;
                        mRetryDelay = std::min<boost::posix_time::time_duration>(
                            mRetryDelay * 2,
                            boost::posix_time::milliseconds(1000));

#ifdef COPROTO_ASIO_LOG
                        log("retry");
#endif
                        mSocket.close();
                        await_suspend(mHandle);
                    }));

            if (mToken.stop_possible() && mCancellationRequested)
                mCancelSignal.emit(boost::asio::cancellation_type::partial);
        }
    };
    //     struct AsioConnectP : AsioConnect
    //     {
    //         // unsigned short localPort = 0;
    //         boost::asio::ip::tcp::endpoint mLocalEndpoint;

    //         AsioConnectP(
    //             std::string address,
    //             boost::asio::io_context &ioc,
    //             unsigned short localPort,
    //             macoro::stop_token token = {},
    //             bool retryOnFailure = true)
    //             : AsioConnect(address, ioc, token, retryOnFailure), mLocalEndpoint(boost::asio::ip::tcp::v4(), localPort)
    //         {

    //             // socket.set_option(boost::asio::socket_base::reuse_address(true));
    // #ifdef BOOST_ASIO_HAS_SO_REUSEPORT
    //             mSocket.set_option(boost::asio::socket_base::reuse_port(true));
    // #endif
    //             mSocket.open(mEndpoint.protocol());
    //             mSocket.bind(mLocalEndpoint);
    //         }
    //     };

    inline AsioSocket asioConnect(std::string address, unsigned short port, bool server, boost::asio::io_context &ioc)
    {
        // std::cout << "asio connect in " << server << std::endl;
        if (server)
        {
            return macoro::sync_wait(
                macoro::make_task(AsioAcceptor(address, ioc, 1)));
        }
        else
        {
            // AsioConnectP(address,ioc,port);
            // std::cout << port << std::endl;
            return macoro::sync_wait(
                macoro::make_task(AsioConnectP(address, ioc, port)));
        }
    }

    inline AsioSocket asioConnect(std::string ip, unsigned short port, bool server)
    {
        // std::cout << "asio connect" << server << std::endl;
        return asioConnect(ip, port, server, global_io_context());
    }
}

#endif