#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <cmath>
#include <numbers> // Requires C++20, or define M_PI manually

// Define M_PI if not using C++20 <numbers>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const double EARTH_RADIUS = 6378137.0;
const double DEG_TO_RAD = M_PI / 180.0;
const double RAD_TO_DEG = 180.0 / M_PI;
const int TILE_SIZE = 256; // Standard web map tile size

struct Point {
    double x;
    double y;
};

struct BBox {
    double min_lon;
    double min_lat;
    double max_lon;
    double max_lat;
};

// Convert longitude/latitude to Web Mercator coordinates (EPSG:3857)
// Mapnik uses Proj internally, but sometimes useful for manual calcs
inline Point lonLatToMercator(double lon, double lat) {
    // Clamp latitude to avoid issues at poles with tan()
    lat = std::max(-85.05112878, std::min(85.05112878, lat));
    double x = EARTH_RADIUS * lon * DEG_TO_RAD;
    double y = EARTH_RADIUS * std::log(std::tan((M_PI / 4.0) + (lat * DEG_TO_RAD / 2.0)));
    return {x, y};
}

// Convert Web Mercator coordinates to longitude/latitude
inline Point mercatorToLonLat(double x, double y) {
    double lon = (x / EARTH_RADIUS) * RAD_TO_DEG;
    double lat = (2.0 * std::atan(std::exp(y / EARTH_RADIUS)) - M_PI / 2.0) * RAD_TO_DEG;
    return {lon, lat};
}

// Calculate the total number of tiles at a given zoom level
inline double numTiles(int z) {
    return std::pow(2.0, z);
}

// Convert tile coordinates (z, x, y) to pixel coordinates in the world map
inline Point tileToPixels(int x, int y) {
    return { (double)x * TILE_SIZE, (double)y * TILE_SIZE };
}

// Convert pixel coordinates in the world map to Mercator coordinates
inline Point pixelsToMercator(double px, double py, int z) {
    double total_pixels = numTiles(z) * TILE_SIZE;
    // Circumference at equator in meters for EPSG:3857
    double half_circumference = M_PI * EARTH_RADIUS;
    double resolution = (2 * half_circumference) / total_pixels; // meters per pixel

    double mx = -half_circumference + px * resolution;
    double my = half_circumference - py * resolution; // Y is flipped in pixel vs mercator
    return {mx, my};
}


// Get the geographic bounding box (lon/lat) for a given tile Z/X/Y
// Note: Mapnik often works directly with Mercator coordinates for bounding boxes.
// We calculate the Mercator box first, then convert corners to Lon/Lat if needed,
// or more commonly, provide the Mercator box directly to Mapnik.
inline mapnik::box2d<double> tileToMercatorBoundingBox(int z, int x, int y) {
    Point tile_min_px = tileToPixels(x, y);
    Point tile_max_px = tileToPixels(x + 1, y + 1);

    // Calculate Mercator coordinates for the corners
    Point mercator_min = pixelsToMercator(tile_min_px.x, tile_max_px.y, z); // Use max Y pixel for min Y Mercator
    Point mercator_max = pixelsToMercator(tile_max_px.x, tile_min_px.y, z); // Use min Y pixel for max Y Mercator

    return mapnik::box2d<double>(mercator_min.x, mercator_min.y, mercator_max.x, mercator_max.y);
}


#endif // PROJECTION_HPP