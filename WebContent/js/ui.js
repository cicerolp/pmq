// prevent right click
$(document).bind("contextmenu", function(event) {
   event.preventDefault();    
});

var rtime;
var timeout = false;
var delta = 200;
$(window).resize(function () {
   rtime = new Date();
   if (timeout === false) {
      timeout = true;
      setTimeout(resizeend, delta);
   }
});

function resizeend() {
   if (new Date() - rtime < delta) {
      setTimeout(resizeend, delta);
   } else {
      timeout = false;
      resize();
   }
}

function LatLngPoint() {
   this.p0;
   this.p1;
}

var heatmapLayer = null;
var simple_heat = null;
var custom_layer = null;

var drawing = false;
var marker = null;
var tile = new LatLngPoint();

var format = d3.timeFormat("%Y/%m/%d %H:%m");

init();

function init() {
   ws_init();
   map_init();

   update();
}

function reset_defaults() {
   up_to_date = false;
   map_move = false;
   map_zoom = false;
   heatmap_updating = false;
}

function ws_init() {
   var ws = new WebSocket(WS_URL);
   console.log(WS_URL);
         
   ws.onopen = function (ev) {
      reset_defaults();      
   };
   ws.onerror = function (ev) {
      up_to_date = false;
      setTimeout(ws_init, 250);
   };
   ws.onclose = function (ev) {
      up_to_date = false;
      setTimeout(ws_init, 250);
   };
   ws.onmessage = function (ev) {
      up_to_date = false;
      console.log("onmessage");
   };   
}

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
      blur: 0.5,
      radius: 5.0,         
      minOpacity: 0.5,
      maxOpacity: 1.0,
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

   simple_heat = L.heatLayer([[0.0, 0.0, 0.0]]);
   custom_layer = L.customLayer(cfg);
   heatmapLayer = new HeatmapOverlay(cfg);
   
   var grid = grid_layer();

   map = new L.Map('map', {
      layers: [black_base, simple_heat],
      center : new L.LatLng(38, -97),
      zoom : 4,
      minZoom: 0,
      maxZoom: 20,
      zoomControl: true,
      maxBounds: [
         [-90, -360],
         [+90, +360]
      ],
      maxBoundsViscosity: 1.0,
      worldCopyJump : false,
   });
   
   var baseMaps = {
      "Black": black_base,
      "White" : white_base,
   };
   
   var overlayMaps = {
      "Simpleheat.js": simple_heat,
      "CustomLayer": custom_layer,
      //"Heatmap.js": heatmapLayer,      
      "Debug Layer": grid
   };
   
   L.control.layers(baseMaps, overlayMaps).addTo(map);
     
   //map.attributionControl.setPrefix("");
   map.attributionControl.setPrefix("Federal University of Rio Grande do Sul");

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
   });

   resize();
}

function default_action(jqXHR, textStatus, errorThrown) { }

function call_assync_query(query, call_success, call_error) {
   var call_error_f = call_error || default_action;

   $.ajax({
         type: 'GET',
         url: S_URL + query,
         dataType: "json",
         success: function (data, textStatus, jqXHR) {
            call_success(data, textStatus, jqXHR);
         },
         error: function(jqXHR, textStatus, errorThrown) { 
            call_error_f(jqXHR, textStatus, errorThrown);
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
   update_table();
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

function resize() {
   var div = d3.select('#right-panel');
   var height = Math.max(1, div.node().getBoundingClientRect().height - 167) + "px";

   var table_cfg = {
      destroy: true,
      deferRender: true,
      scrollX: "100%",
      scrollY: height,
      scrollCollapse: false,
      lengthMenu: [[100, 250, 500, -1], [100, 250, 500, "All"]],
      columnDefs: [
         {
            "render": function (data, type, row) {               
               return format(new Date(data * 1000));
            },
            "targets": 0
         }, {
            "render": function (data, type, row) {
               var str = String(data);

               if (str === "14") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/none.png'/>";
               } else if (str === "13") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/none.png'/>";
               } else if (str === "12") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/ru.png'/>";
               } else if (str === "11") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/pt.png'/>";
               } else if (str === "10") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/pl.png'/>";
               } else if (str === "9") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/nl.png'/>";
               } else if (str === "8") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/ko.png'/>";
               } else if (str === "7") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/none.png'/>";
               } else if (str === "6") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/it.png'/>";
               } else if (str === "5") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/fr.png'/>";
               } else if (str === "4") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/none.png'/>";
               } else if (str === "3") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/de.png'/>";
               } else if (str === "2") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/es.png'/>";
               } else if (str === "1") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/en.png'/>";
               } else if (str === "0") {
                  str = "<img style='vertical-align: middle;' src='/images/flags/pirate.png'/>";
               }

               var cellHtml = "<span originalValue='" + data + "'>" + str + "</span>";
               return cellHtml;

            },
            "targets": 1
         }, {
            "render": function (data, type, row) {
               var str = String(data);

               if (str === "4") {
                  str = "<img style='vertical-align: middle;' src='/images/windows-icon.ico'/>";
               } else if (str === "3") {
                  str = "<img style='vertical-align: middle;' src='/images/apple-icon.ico'/>";
               } else if (str === "2") {
                  str = "<img style='vertical-align: middle;' src='/images/android-icon.png'/>";
               } else if (str === "1") {
                  str = "<img style='vertical-align: middle;' src='/images/apple-icon.ico'/>";
               } else if (str === "0") {
                  str = "<img style='vertical-align: middle;' src='/images/other-icon.png'/>";
               }

               var cellHtml = "<span originalValue='" + data + "'>" + str + "</span>";
               return cellHtml;
            },
            "targets": 2
         }, {
            "render": function (data, type, row) {
               var str = String(data);

               if (str === "3") {
                  str = "<img style='vertical-align: middle;' src='/images/instagram.png'/>";
               } else if (str === "2") {
                  str = "<img style='vertical-align: middle;' src='/images/foursquare.png'/>";
               } else if (str === "1") {
                  str = "<img style='vertical-align: middle;' src='/images/twitter.jpg'/>";
               } else if (str === "0") {
                  str = "<img style='vertical-align: middle;' src='/images/chrome.png'/>";
               }

               var cellHtml = "<span originalValue='" + data + "'>" + str + "</span>";
               return cellHtml;
            },
            "targets": 3
         }
      ],
      ajax: {
         type: 'GET',
         url: S_URL + "/query/data/0/0/0/0/0",
         dataType: "json"
      },
   }

   $('#async_table').DataTable(table_cfg);

   var div = d3.select('#right-panel');
   var right_panel_w = div.node().getBoundingClientRect().width;

   var container_width = (document.documentElement.clientWidth - right_panel_w) + "px";

   $("#container").css("width", container_width);
}

function update_table() {
   table = $('#async_table').DataTable({
      retrieve: true,
   });
      
   var region;
   if (marker === null || drawing) {
      region = get_coords_bounds(map.getBounds());
   } else {
      region = get_coords_bounds(L.latLngBounds(tile.p0, tile.p1), map.getZoom() + 8);
   }

   // /x0/y0/x1/y1/
   var query = "/query/data/" + region.z
      + "/" + region.x0
      + "/" + region.y0
      + "/" + region.x1
      + "/" + region.y1;

   
   table.ajax.url(S_URL + query);
   table.ajax.reload();
}

function update_marker() {
   if (marker === null || drawing) return;
   
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
   var data = response[0];
   
   var options = {
      minOpacity: 0.5,
      maxZoom: map.getZoom(),
      max: data.max,
      radius: 5.0,
      blur: 6.0,
   };

   if (map.hasLayer(custom_layer))
      custom_layer.setData(data);

   if (map.hasLayer(heatmapLayer))
      heatmapLayer.setData(data);

   if (map.hasLayer(simple_heat)) {
      simple_heat.setOptions(options);
      simple_heat.setLatLngs(data.data);
   }

   heatmap_updating = false;
}

function request_data() {
   var region = get_coords_bounds(map.getBounds());
   
   var query = "/query";
   
   query += "/tile/" + region.z + "/" + region.x0
   + "/" + region.y0 + "/" + region.x1 + "/" + region.y1;
      
   query += ("/resolution/" + 8);      
   
   call_assync_query(query, set_heatmap);
}

function update() {
   setTimeout(update, 40);

   if (up_to_date || map_move || map_zoom || heatmap_updating) return;

   heatmap_updating = true;
   
   // /x0/y0/x1/y1/
   var query = "/query/region/0/0/0/0/0";      
   call_assync_query(query, set_progressbar_max);
   
   update_marker();
   update_table();
   request_data();

   up_to_date = true;
}