function drawTextBG(ctx, txt, font, x, y, padding) {
  
    padding = padding || 5;
    
    ctx.save();
    ctx.font = font;
    ctx.textBaseline = 'top';
    ctx.fillStyle = '#f50';
    
    var width = ctx.measureText(txt).width;
    ctx.fillRect(x-padding, y-padding, width+(padding*2), parseInt(font, 10) + (padding*2));
    
    ctx.fillStyle = '#000';
    ctx.fillText(txt, x, y);
    
}

function grid_layer() {

   var canvas_layer = L.gridLayer({
      minZoom: 0,
      maxZoom: 20,
      zIndex: 999
   });

   canvas_layer.createTile = function (coords) {
      var tile = L.DomUtil.create('canvas', 'leaflet-tile');
      var size = this.getTileSize();
      tile.width = size.x;
      tile.height = size.y;
      
      var ctx = tile.getContext('2d');

      ctx.strokeStyle = "#f50";
      ctx.strokeRect(1,1,255,255);

      var text = "x: " + coords.x + ", y: " + coords.y + ", z: " + coords.z;
      drawTextBG(ctx, text, '14px arial', 0, 0);
      
      return tile;        
   };

   return canvas_layer;
}