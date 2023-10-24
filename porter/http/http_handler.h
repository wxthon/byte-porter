
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

namespace porter {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Return a reasonable mime type based on the extension of a file.
inline beast::string_view GetMimeType(beast::string_view path) {
  using beast::iequals;
  auto const ext = [&path] {
    auto const pos = path.rfind(".");
    if(pos == beast::string_view::npos) {
      return beast::string_view{};
    }
    return path.substr(pos);
  }();
  if (iequals(ext, ".htm"))  return "text/html";
  if (iequals(ext, ".html")) return "text/html";
  if (iequals(ext, ".php"))  return "text/html";
  if (iequals(ext, ".css"))  return "text/css";
  if (iequals(ext, ".txt"))  return "text/plain";
  if (iequals(ext, ".js"))   return "application/javascript";
  if (iequals(ext, ".json")) return "application/json";
  if (iequals(ext, ".xml"))  return "application/xml";
  if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
  if (iequals(ext, ".flv"))  return "video/x-flv";
  if (iequals(ext, ".png"))  return "image/png";
  if (iequals(ext, ".jpe"))  return "image/jpeg";
  if (iequals(ext, ".jpeg")) return "image/jpeg";
  if (iequals(ext, ".jpg"))  return "image/jpeg";
  if (iequals(ext, ".gif"))  return "image/gif";
  if (iequals(ext, ".bmp"))  return "image/bmp";
  if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
  if (iequals(ext, ".tiff")) return "image/tiff";
  if (iequals(ext, ".tif"))  return "image/tiff";
  if (iequals(ext, ".svg"))  return "image/svg+xml";
  if (iequals(ext, ".svgz")) return "image/svg+xml";
  return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
inline std::string PathCat(beast::string_view base, beast::string_view path) {
  if(base.empty()) {
    return std::string(path);
  }
  std::string result(base);
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
  if(result.back() == path_separator) {
    result.resize(result.size() - 1);
  }
  result.append(path.data(), path.size());
  for(auto& c : result) {
    if(c == '/') {
      c = path_separator;
    }
  }
#else
  char constexpr path_separator = '/';
  if(result.back() == path_separator) {
    result.resize(result.size() - 1);
  }
  result.append(path.data(), path.size());
#endif
  return result;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator HandleRequest(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req) {
  // Returns a bad request response
  auto const bad_request =
      [&req](beast::string_view why) {
        http::response<http::string_body> res{
          http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
      };

  // Returns a not found response
  auto const not_found =
      [&req](beast::string_view target) {
        http::response<http::string_body> res{
          http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
      };

  // Returns a server error response
  auto const server_error =
      [&req](beast::string_view what) {
        http::response<http::string_body> res{
          http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
      };

  // Make sure we can handle the method
  if (req.method() != http::verb::get &&
      req.method() != http::verb::head) {
    return bad_request("Unknown HTTP-method");
  }

  // Request path must be absolute and not contain "..".
  if (req.target().empty() ||
      req.target()[0] != '/' ||
      req.target().find("..") != beast::string_view::npos) {
    return bad_request("Illegal request-target");
  }

  // Build the path to the requested file
  std::string path = PathCat(doc_root, req.target());
  if (req.target().back() == '/') {
    path.append("index.html");
  }

  // Attempt to open the file
  beast::error_code ec;
  http::file_body::value_type body;
  body.open(path.c_str(), beast::file_mode::scan, ec);

  // Handle the case where the file doesn't exist
  if (ec == beast::errc::no_such_file_or_directory) {
    return not_found(req.target());
  }

  // Handle an unknown error
  if (ec) {
    return server_error(ec.message());
  }

  // Cache the size since we need it after the move
  auto const size = body.size();

  // Respond to HEAD request
  if (req.method() == http::verb::head) {
    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, GetMimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
  }

  // Respond to GET request
  http::response<http::file_body> res{
    std::piecewise_construct,
    std::make_tuple(std::move(body)),
    std::make_tuple(http::status::ok, req.version())};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, GetMimeType(path));
  res.content_length(size);
  res.keep_alive(req.keep_alive());
  return res;
}

} // namespace porter
