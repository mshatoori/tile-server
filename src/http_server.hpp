#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/beast/version.hpp> // Added for BOOST_BEAST_VERSION_STRING
#include <string>
#include <memory> // For shared_ptr
#include <vector> // For vector<uchar> tile data

#include "tile_renderer.hpp" // Include the renderer

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

// Forward declaration
class HttpSession;

// Main server class, accepts connections
class HttpServer : public std::enable_shared_from_this<HttpServer> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<TileRenderer> renderer_; // Share the renderer

public:
    HttpServer(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<TileRenderer> renderer);

    void run();

private:
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};

// Handles one HTTP connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<TileRenderer> renderer_; // Share the renderer
    http::request<http::string_body> req_;

public:
    HttpSession(tcp::socket&& socket, std::shared_ptr<TileRenderer> renderer);

    void run();

private:
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void handle_request();
    void send_response(http::response<http::vector_body<unsigned char>>&& response);
    void send_response(http::response<http::string_body>&& response); // Overload for string body
    void send_bad_request(beast::string_view why);
    void send_not_found();
    void send_server_error(beast::string_view what);
    void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred);
    void do_close();
};


#endif // HTTP_SERVER_HPP