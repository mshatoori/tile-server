#include "http_server.hpp"
#include <iostream>
#include <string>
#include <regex> // For parsing URL path

//------------------------------------------------------------------------------
// HttpServer Implementation
//------------------------------------------------------------------------------

HttpServer::HttpServer(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<TileRenderer> renderer)
    : ioc_(ioc), acceptor_(ioc), renderer_(std::move(renderer)) {
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Acceptor open failed: " << ec.message() << std::endl;
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Acceptor set_option failed: " << ec.message() << std::endl;
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Acceptor bind failed: " << ec.message() << std::endl;
        return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Acceptor listen failed: " << ec.message() << std::endl;
        return;
    }
     std::clog << "INFO: Server listening on " << endpoint << std::endl;
}

void HttpServer::run() {
    if (!acceptor_.is_open()) {
        return;
    }
    do_accept();
}

void HttpServer::do_accept() {
    // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_), // Ensure handlers run serially per connection
        beast::bind_front_handler(
            &HttpServer::on_accept,
            shared_from_this()));
}

void HttpServer::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Accept failed: " << ec.message() << std::endl;
    } else {
        // Create the http session and run it
        std::make_shared<HttpSession>(std::move(socket), renderer_)->run();
    }

    // Accept the next connection
    do_accept();
}


//------------------------------------------------------------------------------
// HttpSession Implementation
//------------------------------------------------------------------------------

HttpSession::HttpSession(tcp::socket&& socket, std::shared_ptr<TileRenderer> renderer)
    : stream_(std::move(socket)), renderer_(std::move(renderer)) {}

void HttpSession::run() {
    // We need to be executing within a strand to perform async operations
    // on the stream. Although not strictly necessary for simple cases,
    // it's good practice. We use dispatch here to run do_read().
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(
                      &HttpSession::do_read,
                      shared_from_this()));
}

void HttpSession::do_read() {
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout.
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream_, buffer_, req_,
                     beast::bind_front_handler(
                         &HttpSession::on_read,
                         shared_from_this()));
}

void HttpSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec) {
         std::cerr << "Read failed: " << ec.message() << std::endl;
        return do_close(); // Consider sending bad request?
    }


    // Handle the request
    handle_request();
}

// Helper to create a response
template<class Body>
http::response<Body> make_response(http::status status, beast::string_view content_type, unsigned version, bool keep_alive) {
    http::response<Body> res{status, version};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    if (!content_type.empty()) {
        res.set(http::field::content_type, content_type);
    }
    res.keep_alive(keep_alive);
    return res;
}


void HttpSession::handle_request() {
    // Only handle GET requests
    if (req_.method() != http::verb::get) {
        return send_bad_request("Unsupported HTTP-method");
    }

    // Basic health check or root path
     if (req_.target() == "/" || req_.target() == "/health") {
        auto res = make_response<http::string_body>(http::status::ok, "text/plain", req_.version(), req_.keep_alive());
        res.body() = "OK";
        res.prepare_payload();
         return send_response(std::move(res)); // Need to adjust send_response template or create overload
    }

    // Regex to parse /z/x/y.png (or other image formats if needed)
    // Target looks like: "/12/2048/1365.png"
    static const std::regex tile_regex(R"(\/(\d+)\/(\d+)\/(\d+)\.png)");
    std::smatch match;
    std::string target_str = req_.target().to_string(); // Convert beast::string_view

    if (std::regex_match(target_str, match, tile_regex)) {
        try {
            int z = std::stoi(match[1].str());
            int x = std::stoi(match[2].str());
            int y = std::stoi(match[3].str());

            // Validate zoom level (example range)
            if (z < 0 || z > 20) {
                 return send_bad_request("Invalid zoom level");
            }

            // Validate X/Y against zoom level
             double max_coord = std::pow(2.0, z);
             if (x < 0 || x >= max_coord || y < 0 || y >= max_coord) {
                 return send_bad_request("Invalid tile coordinates for zoom level");
             }


             std::clog << "INFO: Requesting tile Z=" << z << ", X=" << x << ", Y=" << y << std::endl;


            // Render the tile using the renderer
            std::vector<unsigned char> png_data = renderer_->render_tile(z, x, y);

            // Create the HTTP response with the PNG data
            auto res = make_response<http::vector_body<unsigned char>>(http::status::ok, "image/png", req_.version(), req_.keep_alive());
            res.body() = std::move(png_data); // Move the vector data
            res.prepare_payload(); // Sets Content-Length

            return send_response(std::move(res));

        } catch (const std::invalid_argument& e) {
            std::cerr << "ERROR: Invalid coordinate in URL: " << target_str << " - " << e.what() << std::endl;
            return send_bad_request("Invalid tile coordinates format");
        } catch (const std::out_of_range& e) {
             std::cerr << "ERROR: Coordinate out of range in URL: " << target_str << " - " << e.what() << std::endl;
            return send_bad_request("Tile coordinate out of range");
        }
         catch (const std::exception& e) {
             std::cerr << "ERROR rendering tile Z=" << match[1].str() << " X=" << match[2].str() << " Y=" << match[3].str() << ": " << e.what() << std::endl;
            return send_server_error("Tile rendering failed");
         }

    } else {
        // Path didn't match the tile format
        return send_not_found();
    }
}


// Overload or template specialization for string body needed for bad_request etc.
void HttpSession::send_response(http::response<http::string_body>&& response) {
     // For string bodies (used by error handlers)
    auto sp = shared_from_this(); // Keep session alive

    // Write the response
    http::async_write(stream_, std::move(response),
        [sp](beast::error_code ec, std::size_t bytes) {
            sp->on_write(sp->req_.keep_alive(), ec, bytes); // Check keep_alive from original request
        });
}


void HttpSession::send_response(http::response<http::vector_body<unsigned char>>&& response) {
    // For vector<uchar> bodies (used for PNG data)
     auto self = shared_from_this(); // Keep session alive

    // The lifetime of the response body must extend until the write is complete.
    // Since we are moving the response, its body (the vector) is moved too.
    // We capture the response by value in the lambda to ensure it lives long enough.
    // Note: For large bodies, consider using a shared_ptr for the body data.
     http::async_write(stream_, std::move(response),
        [self](beast::error_code ec, std::size_t bytes) {
            self->on_write(self->req_.keep_alive(), ec, bytes); // Use req_.keep_alive() from *this* session's request
        });
}

void HttpSession::send_bad_request(beast::string_view why) {
    auto res = make_response<http::string_body>(http::status::bad_request, "text/plain", req_.version(), false); // Close connection on bad request
    res.body() = std::string(why);
    res.prepare_payload();
    send_response(std::move(res));
}

void HttpSession::send_not_found() {
    auto res = make_response<http::string_body>(http::status::not_found, "text/plain", req_.version(), req_.keep_alive());
    res.body() = "The resource '" + req_.target().to_string() + "' was not found.";
    res.prepare_payload();
    send_response(std::move(res));
}

void HttpSession::send_server_error(beast::string_view what) {
    auto res = make_response<http::string_body>(http::status::internal_server_error, "text/plain", req_.version(), false); // Close on server error
    res.body() = "Internal server error: " + std::string(what);
    res.prepare_payload();
    send_response(std::move(res));
}


void HttpSession::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "Write failed: " << ec.message() << std::endl;
         return do_close();
    }


    if (!keep_alive) {
        // This means we should close the connection, usually because
        // the response indicated the connection should be closed.
        return do_close();
    }

    // Read another request
    do_read();
}

void HttpSession::do_close() {
    // Send a TCP shutdown
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}