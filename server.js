const express = require('express');
const app = express();
const port = 3000;

// Placeholder for tile rendering logic
const { renderTile } = require('./renderer');

app.get('/:z/:x/:y.png', (req, res) => {
  const { z, x, y } = req.params;

  renderTile(z, x, y, (err, tileBuffer) => {
    if (err) {
      console.error("Error rendering tile:", err);
      res.status(500).send("Error rendering tile");
      return;
    }
    res.setHeader('Content-Type', 'image/png');
    res.send(tileBuffer);
  });
});

app.listen(port, () => {
  console.log(`Tile server listening at http://localhost:${port}`);
});