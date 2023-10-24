#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <glog/logging.h>

#include "porter/http/http_listener.h"

namespace porter {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class HttpServer {
 public:
  HttpServer(const std::string& ip,
             int port,
             int threads,
             const std::string& doc_root)
      : ip_(ip),
        port_(port),
        threads_(threads),
        doc_root_(doc_root) {
  }

  void Run() {
    auto address = net::ip::make_address(ip_);
    auto port = static_cast<unsigned short>(port_);

    LOG(INFO) << "Http server starting ...";
    net::io_context ioc(std::max<int>(1, threads_));
    std::make_shared<HttpListener>(
        ioc,
        tcp::endpoint{address, port},
        std::make_shared<std::string>(doc_root_))->Run();
        
    std::vector<std::thread> v;
    v.reserve(threads_ - 1);
    for(auto i = threads_ - 1; i > 0; --i) {
      v.emplace_back([&ioc] { ioc.run(); });
    }

    LOG(INFO) << "Http server listening to " << address << ":" << port << " ...";
    
    ioc.run();
  }

 private:
  std::string ip_;
  int port_;
  int threads_{1};
  std::string doc_root_;
}; // class HttpServer

} // namespace porter
