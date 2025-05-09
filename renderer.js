const mapnik = require('mapnik');
const path = require('path');

// Register fonts and datasources
mapnik.register_system_fonts();
mapnik.register_local_datasources();

// Load the map style
const map = new mapnik.Map(256, 256);
const stylesheet = path.join(__dirname, 'styles', 'basic_style.xml');

map.load(stylesheet, (err) => {
  if (err) {
    console.error("Error loading map stylesheet:", err);
    // In a real application, you might want to handle this error more gracefully
    // and potentially exit the process or provide a fallback.
  } else {
    console.log("Map stylesheet loaded successfully.");
  }
});

const mercator = new mapnik.Projection('+init=epsg:3857');

const renderTile = (z, x, y, callback) => {
  if (!map.loaded()) {
    return callback(new Error("Map stylesheet not loaded."));
  }

  const max_extent = [-20037508.342789244,-20037508.342789244,20037508.342789244,20037508.342789244];
  const tile_extent = mercator.forward([
    max_extent[0] + (x * (max_extent[2] - max_extent[0]) / (1 << z)),
    max_extent[1] + (y * (max_extent[3] - max_extent[1]) / (1 << z)),
    max_extent[0] + ((x + 1) * (max_extent[2] - max_extent[0]) / (1 << z)),
    max_extent[1] + ((y + 1) * (max_extent[3] - max_extent[1]) / (1 << z))
  ]);

  map.extent = tile_extent;

  const im = new mapnik.Image(256, 256);
  map.render(im, (err, im) => {
    if (err) {
      return callback(err);
    }
    im.encode('png', (err, buffer) => {
      if (err) {
        return callback(err);
      }
      callback(null, buffer);
    });
  });
};

module.exports = { renderTile };