#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <glog/logging.h>

#include "porter/http/http_session.h"

namespace porter {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;
  
// Accepts incoming connections and launches the sessions
class HttpListener : public std::enable_shared_from_this<HttpListener> {
  asio::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<std::string const> doc_root_;

 public:
  HttpListener(asio::io_context& ioc,
               tcp::endpoint endpoint,
               std::shared_ptr<std::string const> const& doc_root)
      : ioc_(ioc),
        acceptor_(asio::make_strand(ioc)),
        doc_root_(doc_root) {
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      fail(ec, "open");
      return;
    }

    // Allow address reuse
    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
      fail(ec, "set_option");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void Run() {
    DoAccept();
  }

 private:
  void DoAccept() {
    // The new connection gets its own strand
    acceptor_.async_accept(
        asio::make_strand(ioc_),
        beast::bind_front_handler(
            &HttpListener::HandleAccept,
            shared_from_this()));
  }

  void HandleAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
      fail(ec, "accept");
      return; // To avoid infinite loop
    } else {
      // Create the session and run it
      std::make_shared<HttpSession>(std::move(socket), doc_root_)->Run();
    }

    // Accept another connection
    DoAccept();
  }
}; // class HttpListener

} // namespace porter
