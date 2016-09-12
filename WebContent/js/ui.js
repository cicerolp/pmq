// prevent right click
$(document).bind("contextmenu", function(event) {
   event.preventDefault();    
});

function LatLngPoint() {
   this.p0;
   this.p1;
}

var heatmapLayer = null;
var drawing = false;
var marker = null;
var tile = new LatLngPoint();

map_init();

function map_init() {
   var layers = {
      black : 'http://{s}.tiles.mapbox.com/v4/cicerolp.mgdebfa9/{z}/{x}/{y}.png?access_token=pk.eyJ1IjoiY2ljZXJvbHAiLCJhIjoia1IxYmtfMCJ9.3EMmwKCCFN-hmsrQY4_wUQ',
      white : 'http://{s}.tiles.mapbox.com/v4/cicerolp.pdni2p2n/{z}/{x}/{y}.png?access_token=pk.eyJ1IjoiY2ljZXJvbHAiLCJhIjoia1IxYmtfMCJ9.3EMmwKCCFN-hmsrQY4_wUQ'
   }

   var baseLayer = L.tileLayer(layers["black"], {
      subdomains: "abcd",
      minZoom: 0,
      maxZoom: 20,        
      maxNativeZoom: 20
   });
    
   var cfg = {
      blur: 0.5,
      radius: 5.0,         
      minOpacity: 0.3,
      maxOpacity: 0.9,
      scaleRadius: false, 
      useLocalExtrema: true,
      latField: 'lat',
      lngField: 'lon',
      valueField: 'count',
      gradient: {
         '.0': 'blue',
         '.1': 'red',
         '.80': 'white'
      },
   };      

   heatmapLayer = new HeatmapOverlay(cfg);

   map = new L.Map('map', {
      layers: [baseLayer, heatmapLayer],
      center : new L.LatLng(38, -97),
      zoom : 4,
      minZoom: 0,
      maxZoom: 20,
      zoomControl: true,
   });
     
   map.attributionControl.setPrefix("");

   map.on('mousedown', onMouseDown); 
   map.on('mousemove', onMouseMove);
   map.on('mouseout',  onMouseUp); 
   map.on('mouseup',   onMouseUp); 
   
   map.on("zoomstart", function (e) {
      map_zoom = true;
   }); 
   map.on("zoomend", function (e) {
      map_zoom = false;
   }); 
   map.on("movestart", function (e) {
      map_move = true;
   });      
   map.on("moveend", function (e) {
      map_move = false;
      update_heatmap();
   });

   update_heatmap();
   var interval = window.setInterval(update_heatmap, 100);
}

function onMouseDown(e) {
   if (e.originalEvent.button != 2) {
      return;
   }
    
   if (marker != null) {        
      map.removeLayer(marker);
      marker = null;
   }

   drawing = true;
   tile.p0 = e.latlng;
}

function onMouseUp(e) {
   if (e.originalEvent.button != 2 || drawing == false) {
      return;
   }

   tile.p1 = e.latlng;
   
   if (tile.p0.lat == tile.p1.lat && tile.p0.lng == tile.p1.lng) {
      if (marker != null) {        
         map.removeLayer(marker);
         marker = null;
      }
   }

   drawing = false;

   if (marker == null) return;

   var b = L.latLngBounds(tile.p0, tile.p1);

   var lat0 = b._northEast.lat;
   var lon0 = b._southWest.lng;
   var lat1 = b._southWest.lat;
   var lon1 = b._northEast.lng;

   var z = map.getZoom() + 8;

   var x0 = roundtile(lon2tilex(lon0, z), z);
   var x1 = roundtile(lon2tilex(lon1, z), z);

   if (x0 > x1) {
      x0 = 0;
      x1 = Math.pow(2, z);
   }

   // /x0/y0/x1/y1/
   var query = "/region/" + z
      + "/" + x0
      + "/" + roundtile(lat2tiley(lat0, z), z)
      + "/" + x1
      + "/" + roundtile(lat2tiley(lat1, z), z);
      
   $.ajax({
         type: 'GET',
         url: S_URL + "/query" + query,
         dataType: "json",
         success: function (data, textStatus, jqXHR) {
            console.log(data);
         }
   });   
}

function onMouseMove(e) {
   if (drawing == false) return;

   tile.p1 = e.latlng; 

   if (marker != null) {
      map.removeLayer(marker);        
   }

   var bounds = L.latLngBounds(tile.p0, tile.p1);
   marker = L.rectangle(bounds, marker_opt);
   map.addLayer(marker);
}

function get_visible_tiles() {  
   var tileSize = 256;
   var zoom = map.getZoom();
   var bounds = map.getPixelBounds();
   
   var container = [];
   
   var nwTilePoint = new L.Point(Math.floor(bounds.min.x / tileSize),
       Math.floor(bounds.min.y / tileSize));

   var seTilePoint = new L.Point(Math.floor(bounds.max.x / tileSize),
       Math.floor(bounds.max.y / tileSize));

   var max = map.options.crs.scale(zoom) / tileSize; 

   for (var x = nwTilePoint.x; x <= seTilePoint.x; x++) {
      for (var y = nwTilePoint.y; y <= seTilePoint.y; y++) {
         var xTile = Math.abs(x % max);
         var yTile = Math.abs(y % max);
         
         var el = {x: xTile, y: yTile};
         
         if (container.length == 0) {
            container.push(el);
         } else {
            var lhs = container[container.length - 1];            
            if (lhs.x != el.x || lhs.y != el.y)
               container.push(el);
         }
       }
   }
   
   return container;      
};

function set_heatmap(response, textStatus) {
 if (textStatus != "success") {
      heatmapLayer.setData({data: [{lat: -90, lon:-180, count: 0}]});
      heatmap_updating = false;         
      return;
   }
   
   var heatmap_max = 0;
   var heatmap_min = Number.MAX_SAFE_INTEGER;
   var heatmap_data = [];
   
   response.forEach(function(el) {        
      heatmap_max = Math.max(heatmap_max, el[3]);
      heatmap_min = Math.min(heatmap_min, el[3]);
      
      var lon = tilex2lon(el[0] + 0.5, el[2]);
      var lat = tiley2lat(el[1] + 0.5, el[2]);            
      heatmap_data.push({lat: lat, lon:lon, count: el[3]})
   });
   
   heatmapLayer.setData({
      max: heatmap_max,
      min: heatmap_min,
      data: heatmap_data
   });
   
   heatmap_updating = false;
}

function request_data() {
   var curr_tiles = get_visible_tiles();
   
   var zoom = map.getZoom();
   
   var query = ("");
   
   curr_tiles.forEach(function(entry) {            
      query += ("/tile/" + entry.x + "/" + entry.y + "/" + zoom);
   });
   
   if (query.length != 0)
      query += ("/resolution/" + 8);
   
   $.ajax({
         type: 'GET',
         url: S_URL + "/query" + query,
         dataType: "json",
         success: function (data, textStatus, jqXHR) {
            set_heatmap(data, textStatus);
         },
         error: function(jqXHR, textStatus, errorThrown) { 
            set_heatmap(jqXHR, textStatus);
         } 
   });
}

function update_heatmap() {
   if (map_move  || map_zoom || heatmap_updating) return;
   
   heatmap_updating = true;
   
   $.ajax({
         type: 'GET',
         url: S_URL + "/update",
         dataType: "json",
         success: function (data, textStatus, jqXHR) {
            // request updated data
            if (data) {
               request_data();
            } else {
               heatmap_updating = false;
            }
         },
         error: function(jqXHR, textStatus, errorThrown) { 
            set_heatmap(jqXHR, textStatus);
         } 
   });
}