function grid_layer() {

   var layer = L.gridLayer({});

   layer.createTile = function(coords) {
      var tile = L.DomUtil.create('canvas', 'leaflet-tile');

      var size = this.getTileSize();
      tile.width = size.x;
      tile.height = size.y;
   
      var ctx = tile.getContext('2d');
      ctx.font = "15px sans-serif";
      ctx.fillText(coords, size.x/2, size.y/2);

      return tile;
   };

   return layer;
};
