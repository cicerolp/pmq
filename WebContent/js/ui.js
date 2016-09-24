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

   resize();

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
               var format = d3.timeFormat("%Y/%m/%d %H:%m");
               return format(new Date(data * 1000));
            },
            "targets": 0
         }, {
            "render": function (data, type, row) {
               var vendor = String(data);
               vendor = vendor.replace("14", "<img style='vertical-align: middle;' src='/images/flags/none.png'/>");
               vendor = vendor.replace("13", "<img style='vertical-align: middle;' src='/images/flags/none.png'/>");
               vendor = vendor.replace("12", "<img style='vertical-align: middle;' src='/images/flags/ru.png'/>");
               vendor = vendor.replace("11", "<img style='vertical-align: middle;' src='/images/flags/pt.png'/>");
               vendor = vendor.replace("10", "<img style='vertical-align: middle;' src='/images/flags/pl.png'/>");
               vendor = vendor.replace("9", "<img style='vertical-align: middle;' src='/images/flags/nl.png'/>");
               vendor = vendor.replace("8", "<img style='vertical-align: middle;' src='/images/flags/ko.png'/>");
               vendor = vendor.replace("7", "<img style='vertical-align: middle;' src='/images/flags/none.png'/>");
               vendor = vendor.replace("6", "<img style='vertical-align: middle;' src='/images/flags/it.png'/>");
               vendor = vendor.replace("5", "<img style='vertical-align: middle;' src='/images/flags/fr.png'/>");
               vendor = vendor.replace("4", "<img style='vertical-align: middle;' src='/images/flags/none.png'/>");
               vendor = vendor.replace("3", "<img style='vertical-align: middle;' src='/images/flags/de.png'/>");
               vendor = vendor.replace("2", "<img style='vertical-align: middle;' src='/images/flags/es.png'/>");
               vendor = vendor.replace("1", "<img style='vertical-align: middle;' src='/images/flags/en.png'/>");
               vendor = vendor.replace("0", "<img style='vertical-align: middle;' src='/images/flags/pirate.png'/>");

               var cellHtml = "<span originalValue='" + data + "'>" + vendor + "</span>";
               return cellHtml;

            },
            "targets": 1
         }, {
            "render": function (data, type, row) {
               var vendor = String(data);
               vendor = vendor.replace("0", "<img style='vertical-align: middle;' src='/images/other-icon.png'/>");
               vendor = vendor.replace("1", "<img style='vertical-align: middle;' src='/images/apple-icon.ico'/>");
               vendor = vendor.replace("2", "<img style='vertical-align: middle;' src='/images/android-icon.png'/>");               
               vendor = vendor.replace("3", "<img style='vertical-align: middle;' src='/images/apple-icon.ico'/>");
               vendor = vendor.replace("4", "<img style='vertical-align: middle;' src='/images/windows-icon.ico'/>");

               var cellHtml = "<span originalValue='" + data + "'>" + vendor + "</span>";
               return cellHtml;

            },
            "targets": 2
         }, {
            "render": function (data, type, row) {
               var vendor = String(data);
               vendor = vendor.replace("0", "<img style='vertical-align: middle;' src='/images/chrome.png'/>");
               vendor = vendor.replace("1", "<img style='vertical-align: middle;' src='/images/twitter.jpg'/>");
               vendor = vendor.replace("2", "<img style='vertical-align: middle;' src='/images/foursquare.png'/>");
               vendor = vendor.replace("3", "<img style='vertical-align: middle;' src='/images/instagram.png'/>");

               var cellHtml = "<span originalValue='" + data + "'>" + vendor + "</span>";
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
      wait = 40;
   } else {
      wait = 40;
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
   update_table();
   request_data();
}