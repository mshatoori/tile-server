#include "tile_renderer.hpp"
#include <sstream> // For string stream formatting of PNG

// Constructor
TileRenderer::TileRenderer(const std::string& style_path, const std::string& pbf_file_path, unsigned int tile_size)
    : tile_size_(tile_size),
      map_prototype_(tile_size, tile_size), // Initialize prototype with tile dimensions
      pbf_path_(pbf_file_path),
      proj_web_mercator_("+init=epsg:3857"), // Define Web Mercator projection
      proj_latlon_("+init=epsg:4326")        // Define Lat/Lon projection
{
    try {
        // Register default Mapnik fonts and plugins if not done automatically
        // mapnik::freetype_engine::register_fonts("/path/to/fonts", true); // Example: Register fonts if needed
        mapnik::datasource_cache::instance().register_datasources("/usr/lib/mapnik/input/"); // Adjust path as per your Mapnik installation

        // Load the map style XML
        mapnik::load_map(map_prototype_, style_path, true); // true = strict mode

        // *** CRITICAL: Update the PBF file path in the datasource ***
        // The XML style likely has a placeholder or default path. We need to set the
        // actual path provided at runtime.
        // This assumes the layer using the PBF is the FIRST layer (index 0). Adjust if needed.
        if (!map_prototype_.layers().empty()) {
            mapnik::parameters params = map_prototype_.layers()[0].datasource()->params();
            params["file"] = pbf_path_; // Set the 'file' parameter to the actual PBF path
            // Optional: Set encoding if needed (often utf-8 for OSM)
            // params["encoding"] = "utf-8";

            // Create a *new* datasource with updated parameters and assign it
            std::shared_ptr<mapnik::datasource> ds = mapnik::datasource_cache::instance().create(params);
            map_prototype_.layers()[0].set_datasource(ds);
             std::clog << "INFO: Mapnik Datasource 'file' parameter updated to: " << pbf_path_ << std::endl;
        } else {
             std::cerr << "WARNING: Mapnik style loaded, but no layers found. Cannot set PBF path." << std::endl;
        }


        // Set the map's projection to Web Mercator (EPSG:3857)
        map_prototype_.set_srs(proj_web_mercator_.params());

        std::clog << "INFO: Mapnik style '" << style_path << "' loaded successfully." << std::endl;

    } catch (const mapnik::config_error& e) {
        std::cerr << "Mapnik Config ERROR: " << e.what() << std::endl;
        throw std::runtime_error("Failed to configure Mapnik: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "ERROR loading Mapnik style: " << e.what() << std::endl;
        throw std::runtime_error("Failed to load Mapnik style: " + std::string(e.what()));
    }
}

// Render tile implementation
std::vector<unsigned char> TileRenderer::render_tile(int z, int x, int y) {
    // Lock mutex for thread safety if Map object is shared or modified.
    // Rendering might be safe depending on Mapnik internals, but safer to lock.
    // std::lock_guard<std::mutex> lock(map_mutex_); // Lock if needed

    // Create a working copy of the map for this request
    // This might be slow; consider a pool of pre-initialized map objects if performance is critical.
    mapnik::Map map_instance = map_prototype_; // Copy constructor
    map_instance.resize(tile_size_, tile_size_);

    // Calculate the bounding box for the tile in Web Mercator coordinates
    mapnik::box2d<double> merc_bbox = tileToMercatorBoundingBox(z, x, y);

    // Set the map extent to the tile's bounding box
    map_instance.zoom_to_box(merc_bbox);

    // Render the map to an image
    mapnik::image_rgba8 image(map_instance.width(), map_instance.height());
    mapnik::agg_renderer<mapnik::image_rgba8> renderer(map_instance, image);
    renderer.apply(); // Perform the rendering

    // Encode the image to PNG format in memory
    std::string png_string = mapnik::save_to_string(image, "png"); // Corrected function name

    // Convert the PNG string to a byte vector
    return std::vector<unsigned char>(png_string.begin(), png_string.end());
}