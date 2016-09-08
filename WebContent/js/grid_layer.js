function roundtile(v, z) {
    v = Math.floor(v);
    while (v < 0) v += 1 << z;
    return v % (1 << z);
}

function tilex2lon(x, z) {
    return x / Math.pow(2.0, z) * 360.0 - 180;
}

function tiley2lat(y, z) {
    var n = Math.PI - 2.0 * Math.PI * y / Math.pow(2.0, z);
    return 180.0 / Math.PI * Math.atan(0.5 * (Math.exp(n) - Math.exp(-n)));
}

function lon2tilex(lon, z) {
    return ((lon + 180.0) / 360.0 * Math.pow(2.0, z));
}

function lat2tiley(lat, z) {
    return ((1.0 - Math.log(Math.tan(lat * Math.PI / 180.0) + 1.0 / Math.cos(lat * Math.PI / 180.0)) / Math.PI) / 2.0 * Math.pow(2.0, z));
}

function grid_layer() {

   var layer = L.gridLayer({});

   layer.createTile = function(coords, done) {
      var tile = L.DomUtil.create('canvas', 'leaflet-tile');

      var size = this.getTileSize();
      tile.width = size.x;
      tile.height = size.y;
   
      var ctx = tile.getContext('2d');
      //ctx.font = "15px sans-serif";
      //ctx.fillText(coords, size.x/2, size.y/2);

      var query_map = "/tile/" + coords.x + "/" + coords.y + "/" + coords.z + "/8";

      $.ajax({
         type: 'GET',
         url: S_URL + query_map,
         dataType: "json",
         success: function (data) {
            var datum = {
               ctx: ctx,
               size: size,                  
               data: data,
               done: done,                                
               coords: coords,
             };

             color_tile(datum);
         }
      });     
      
      return tile;
   };

   return layer;
};

function color_tile(entry) {
   entry.ctx.font = "15px sans-serif";
   entry.data.forEach(function(d) {

      entry.ctx.fillStyle = ryw(d[3]);      

      var lon0 = tilex2lon(d[0], d[2]);
      var lat0 = tiley2lat(d[1], d[2]);
      var lon1 = tilex2lon(d[0] + 1, d[2]);
      var lat1 = tiley2lat(d[1] + 1, d[2]);

      var x0 = (lon2tilex(lon0, entry.coords.z) - entry.coords.x) * 256;
      var y0 = (lat2tiley(lat0, entry.coords.z) - entry.coords.y) * 256;
      var x1 = (lon2tilex(lon1, entry.coords.z) - entry.coords.x) * 256;
      var y1 = (lat2tiley(lat1, entry.coords.z) - entry.coords.y) * 256;

      const size_px = 0.5;
      var width = x1 - x0;
      var height = y1 - y0;
      entry.ctx.fillRect(x0 - size_px, y0 - size_px, width + size_px, height + size_px);



      entry.ctx.fillText("DEMO", entry.size.x/2, entry.size.y/2);
   });

   var error;
   entry.done(error, entry.tile);
}

function ryw (count) {
   var lc = Math.log(count + 1) / Math.log(10);

   var r = Math.floor(256 * Math.min(1, lc));
   var g = Math.floor(256 * Math.min(1, Math.max(0, lc - 1)));
   var b = Math.floor(256 * Math.min(1, Math.max(0, lc - 2)));

   return "rgba(" + r + "," + g + "," + b + "," + 1 + ")";
}
