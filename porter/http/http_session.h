#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <glog/logging.h>

#include "porter/http/http_handler.h"

namespace porter {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

// Report a failure
inline void fail(beast::error_code ec, char const* what) {
  LOG(ERROR) << what << ": " << ec.message();
}

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<std::string const> doc_root_;
  http::request<http::string_body> req_;

 public:
  // Take ownership of the stream
  HttpSession(tcp::socket&& socket,
              std::shared_ptr<std::string const> const& doc_root)
      : stream_(std::move(socket)),
        doc_root_(doc_root) {
  }

  // Start the asynchronous operation
  void Run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    asio::dispatch(stream_.get_executor(),
                   beast::bind_front_handler(
                       &HttpSession::DoRead,
                       shared_from_this()));
  }

  void DoRead() {
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout.
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream_, buffer_, req_,
                     beast::bind_front_handler(
                         &HttpSession::HandleRead,
                         shared_from_this()));
  }

  void HandleRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if(ec == http::error::end_of_stream)
      return DoClose();

    if(ec) {
      fail(ec, "read");
      return;
    }

    // Send the response
    send_response(
        HandleRequest(*doc_root_, std::move(req_)));
  }

  void send_response(http::message_generator&& msg) {
    bool keep_alive = msg.keep_alive();

    // Write the response
    beast::async_write(
        stream_,
        std::move(msg),
        beast::bind_front_handler(
            &HttpSession::HandleWrite, shared_from_this(), keep_alive));
  }

  void HandleWrite(
      bool keep_alive,
      beast::error_code ec,
      std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec)
      return fail(ec, "write");

    if(! keep_alive) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      return DoClose();
    }

    // Read another request
    DoRead();
  }

  void DoClose() {
    // Send a TCP shutdown
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
  }
};

} // namespace porter
