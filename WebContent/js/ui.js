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

   var base_config = {
      subdomains: "abcd",
      minZoom: 0,
      maxZoom: 20,        
      maxNativeZoom: 20
   };
   
   var black_base = L.tileLayer(layers["black"], base_config);
   var white_base = L.tileLayer(layers["white"], base_config);
    
   var cfg = {
      blur: 1.0,
      radius: 5.0,         
      minOpacity: 0.4,
      maxOpacity: 0.9,
      scaleRadius: false, 
      useLocalExtrema: true,
      latField: 0,
      lngField: 1,
      valueField: 2,
      gradient: {
         '.0': 'blue',
         '.070': 'red',
         '.50': 'white'
      },
   };      

   heatmapLayer = new HeatmapOverlay(cfg);

   var grid = grid_layer();

   map = new L.Map('map', {
      layers: [black_base, heatmapLayer, grid],
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
   
   var baseMaps = {
      "Black": black_base,
      "White" : white_base,
   };
   
   var overlayMaps = {
      "Heatmap.js": heatmapLayer,
      "Debug Layer": grid
   };
   
   L.control.layers(baseMaps, overlayMaps).addTo(map);
     
   map.attributionControl.setPrefix("");
   //map.attributionControl.setPrefix("Federal University of Rio Grande do Sul");

   map.on('mousedown', onMouseDown); 
   map.on('mousemove', onMouseMove);
   map.on('mouseout',  onMouseUp); 
   map.on('mouseup',   onMouseUp); 
   
   map.on("zoomstart", function (e) {
      map_zoom = true;
   }); 
   map.on("zoomend", function (e) {
      map_zoom = false;
      up_to_date = false;
   }); 
   map.on("movestart", function (e) {
      map_move = true;
   });      
   map.on("moveend", function (e) {
      map_move = false;
      up_to_date = false;
      update();
   });

   up_to_date = false;
   update();

   call_update();
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

function get_coords_bounds(b, zoom) {
   var _z = zoom || map.getZoom();
   
   _z = Math.min(_z, 24);
   
   var lat0 = b._northEast.lat;
   var lon0 = b._southWest.lng;   
   var lat1 = b._southWest.lat;
   var lon1 = b._northEast.lng;
      
   // out of bounds check
   if(lon0 < -180) lon0 = -180;
   if(lon1 < -180) lon1 = -180;
   
   if(lon0 > 179) lon0 = 179.9;
   if(lon1 > 179) lon1 = 179.9;
   
   if(lat0 < -85) lat0 = -85;
   if(lat1 < -85) lat1 = -85;
   
   if(lat0 > 85) lat0 = 85;
   if(lat1 > 85) lat1 = 85;
   
   var _x0 = Math.floor(lon2tilex(lon0, _z));
   var _x1 = Math.floor(lon2tilex(lon1, _z));
   
   var _y0 = Math.floor(lat2tiley(lat0, _z));
   var _y1 = Math.floor(lat2tiley(lat1, _z)); 
   
   return {x0: _x0, y0: _y0, x1: _x1, y1: _y1, z: _z};
}

function update_marker() {
   if (marker === null ||drawing) return;
   
   var zoom = map.getZoom() + 8;
   var coords = get_coords_bounds(L.latLngBounds(tile.p0, tile.p1), zoom);

   // /x0/y0/x1/y1/
   var query = "/query/region/" + coords.z
      + "/" + coords.x0
      + "/" + coords.y0
      + "/" + coords.x1
      + "/" + coords.y1;

   call_assync_query(query, set_progressbar_value);
}

function set_heatmap(response, textStatus) {
   heatmap_updating = false; 
   
   var data = null;   
   if (textStatus !== "success") {
      data = {
         max: 0,
         min: 0,
         data: [[-90, -180, 0]]
      };      
   } else {      
      data = response[0];
   }
   
   heatmapLayer.setData(data);
}

function request_data() {
   var region = get_coords_bounds(map.getBounds());
   
   var query = "/query";
   
   query += "/tile/" + region.z + "/" + region.x0
   + "/" + region.y0 + "/" + region.x1 + "/" + region.y1;
      
   query += ("/resolution/" + 8);      
   
   call_assync_query(query, set_heatmap, set_heatmap);
}

function call_update(response, textStatus) {
   if (textStatus != "success") {
      up_to_date = true;
   } else {
      up_to_date = response[0];
   }
   
   var wait = 0;
   if (!up_to_date) {
      wait = 1000;
   } else {
      wait = 1000;
   }
   
   update();
   
   setTimeout(function() {
      call_assync_query("/update", call_update, call_update);
   }, wait);
}

function update() {   
   if (up_to_date || map_move || map_zoom || heatmap_updating) return;

   heatmap_updating = true;
   
   // /x0/y0/x1/y1/
   var query = "/query/region/0/0/0/1/1";      
   call_assync_query(query, set_progressbar_max);
   
   update_marker();
   request_data();
}