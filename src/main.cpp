#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "http_server.hpp"
#include "tile_renderer.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    try {
        // --- Argument Parsing ---
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Produce help message")
            ("pbf_file", po::value<std::string>()->required(), "Path to the input OSM PBF file")
            ("style_file", po::value<std::string>()->required(), "Path to the Mapnik XML style file")
            ("address", po::value<std::string>()->default_value("0.0.0.0"), "IP address to bind to")
            ("port", po::value<unsigned short>()->default_value(8080), "Port to listen on")
            ("threads", po::value<int>()->default_value(1), "Number of worker threads");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        po::notify(vm); // Throws if required options are missing

        // --- Configuration ---
        const auto address = net::ip::make_address(vm["address"].as<std::string>());
        const auto port = vm["port"].as<unsigned short>();
        const std::string pbf_file = vm["pbf_file"].as<std::string>();
        const std::string style_file = vm["style_file"].as<std::string>();
        int threads = vm["threads"].as<int>();
        if (threads <= 0) threads = 1;

        std::clog << "INFO: PBF file: " << pbf_file << std::endl;
        std::clog << "INFO: Style file: " << style_file << std::endl;
        std::clog << "INFO: Binding to " << address << ":" << port << std::endl;
        std::clog << "INFO: Using " << threads << " worker thread(s)." << std::endl;

        // --- Initialization ---
        net::io_context ioc{threads}; // IO context for the server

        // Initialize the Tile Renderer (shared among sessions)
        // This loads the Mapnik style and sets up datasources
        auto renderer = std::make_shared<TileRenderer>(style_file, pbf_file);

        // Create and launch the HTTP server
        auto server = std::make_shared<HttpServer>(ioc, tcp::endpoint{address, port}, renderer);
        server->run();

        // Capture SIGINT and SIGTERM to perform a clean shutdown
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](beast::error_code const&, int) {
            std::clog << "INFO: Shutting down server..." << std::endl;
            // Stop the io_context. This will cause run() to return immediately.
            ioc.stop();
        });

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for (auto i = threads - 1; i > 0; --i) {
            v.emplace_back([&ioc] { ioc.run(); });
        }
        // Main thread also runs the I/O service
        ioc.run();

        // Wait for all threads to finish (if any were created)
        for (auto& t : v) {
            t.join();
        }

        std::clog << "INFO: Server stopped." << std::endl;


    } catch (const po::error &e) {
        std::cerr << "Argument Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}