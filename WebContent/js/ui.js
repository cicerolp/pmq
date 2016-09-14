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
      blur: 0.25,
      radius: 5.0,         
      minOpacity: 0.3,
      maxOpacity: 1.0,
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
      maxBounds: [
         [-90, -180],
         [+90, +180]
      ],
      maxBoundsViscosity: 1.0,
      worldCopyJump : false,
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

function call_assync_query(query, call_success, call_error) {
   $.ajax({
         type: 'GET',
         url: S_URL + query,
         dataType: "json",
         success: function (data, textStatus, jqXHR) {
            call_success(data, textStatus, jqXHR);
         },
         error: function(jqXHR, textStatus, errorThrown) { 
            call_error(jqXHR, textStatus, errorThrown);
         } 
   });
}

function onMouseDown(e) {
   if (e.originalEvent.button != 2) {
      return;
   }
    
   if (marker != null) {        
      map.removeLayer(marker);
      marker = null;
   }

   set_progressbar_value([0]);

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

   update_marker();   
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
   var b = map.getBounds();

   var lat0 = b._northEast.lat;
   var lon0 = b._southWest.lng;
   var lat1 = b._southWest.lat;
   var lon1 = b._northEast.lng;

   var _z = map.getZoom();

   var _x0 = roundtile(lon2tilex(lon0, _z), _z);
   var _x1 = roundtile(lon2tilex(lon1, _z), _z);
   
   if (_x0 > _x1) {
      _x0 = 0;
      _x1 = Math.pow(2, _z);
   }   
   var _y0 = roundtile(lat2tiley(lat0, _z), _z);
   var _y1 = roundtile(lat2tiley(lat1, _z), _z);
   
   if (_y0 > _y1) {
      _y0 = 0;
      _y1 = Math.pow(2, _z);
   }   
   
   return {x0: _x0, y0: _y0, x1: _x1, y1: _y1, z: _z};
};

function update_marker() {
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
   var query = "/query/region/" + z
      + "/" + x0
      + "/" + roundtile(lat2tiley(lat0, z), z)
      + "/" + x1
      + "/" + roundtile(lat2tiley(lat1, z), z);
      
   call_assync_query(query, set_progressbar_value);
}

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
   var region = get_visible_tiles();
   
   var query = "/query";
   
   query += "/tile/" + region.z + "/" + region.x0
   + "/" + region.y0 + "/" + region.x1 + "/" + region.y1;
   
   query += ("/resolution/" + 8);      
   
   call_assync_query(query, set_heatmap, set_heatmap);
}

function update_heatmap() {
   if (map_move  || map_zoom || heatmap_updating) return;
   
   heatmap_updating = true;
   
   // /x0/y0/x1/y1/
   var query = "/query/region/1/0/0/1/1";      
   call_assync_query(query, set_progressbar_max);
   
   update_marker();   
   request_data();
}