#ifndef TILE_RENDERER_HPP
#define TILE_RENDERER_HPP

#include <string>
#include <vector>
#include <mutex>
#include <stdexcept> // For runtime_error
#include <iostream> // For cerr

#include <mapnik/projection.hpp>        // <<< ESSENTIAL: Defines mapnik::projection
#include <mapnik/layer.hpp>             // <<< ESSENTIAL: Defines mapnik::layer
#include <mapnik/params.hpp>            // <<< ESSENTIAL: Defines mapnik::parameters
#include <mapnik/config_error.hpp>      // <<< ESSENTIAL: For mapnik::config_error (or <mapnik/error.hpp>)
// Mapnik headers
#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/datasource.hpp> // Added for mapnik::datasource definition
#include <mapnik/box2d.hpp>
#include <mapnik/proj_transform.hpp> // For projections if needed manually

#include "projection.hpp" // For tile BBox calculation

class TileRenderer {
public:
    // Constructor: Loads the style XML and registers datasources
    TileRenderer(const std::string& style_path, const std::string& pbf_file_path, unsigned int tile_size = 256);

    // Renders a single tile Z/X/Y into a PNG image buffer
    std::vector<unsigned char> render_tile(int z, int x, int y);

private:
    unsigned int tile_size_;
    mapnik::Map map_prototype_; // A configured map instance used as a template
    std::string pbf_path_;      // Store PBF path to potentially update datasource params
    std::mutex map_mutex_;     // Mutex to protect access to map object if needed (rendering itself might be complex)

    // Mapnik projections
    mapnik::projection proj_web_mercator_; // EPSG:3857
    mapnik::projection proj_latlon_;       // EPSG:4326
};

#endif // TILE_RENDERER_HPP