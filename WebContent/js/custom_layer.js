'use strict';

L.CustomLayer = (L.Layer ? L.Layer : L.Class).extend({

   initialize: function (confg) {
      this._config = confg;      
   },

   onAdd: function (map) {
      this._map = map;

      if (!this._canvas) {
         this._initCanvas();
      }

      if (this.options.pane) {
         this.getPane().appendChild(this._canvas);
      } else {
         map._panes.overlayPane.appendChild(this._canvas);
      }

      map.on('moveend', this._reset, this);

      if (map.options.zoomAnimation && L.Browser.any3d) {
         map.on('zoomanim', this._animateZoom, this);
      }

      this._reset();
    },

    onRemove: function (map) {
        if (this.options.pane) {
            this.getPane().removeChild(this._canvas);
        } else {
            map.getPanes().overlayPane.removeChild(this._canvas);
        }

        map.off('moveend', this._reset, this);

        if (map.options.zoomAnimation) {
            map.off('zoomanim', this._animateZoom, this);
        }
    },

    addTo: function (map) {
        map.addLayer(this);
        return this;
    },

    setData: function (data) {
       this._max = data.max || this._max;
       this._min = data.min || this._min;

       this._data = data.data;
       return this.redraw();
    },

    redraw: function () {
       if (!this._frame && !this._map._animating) {
          this._frame = L.Util.requestAnimFrame(this._redraw, this);
       }
       return this;
    },

   _initCanvas: function () {      
      var canvas = this._canvas = L.DomUtil.create('canvas', 'leaflet-heatmap-layer leaflet-layer');

      var originProp = L.DomUtil.testProp(['transformOrigin', 'WebkitTransformOrigin', 'msTransformOrigin']);
      canvas.style[originProp] = '50% 50%';

      var size = this._map.getSize();
      canvas.width  = size.x;
      canvas.height = size.y;

      var animated = this._map.options.zoomAnimation && L.Browser.any3d;
      L.DomUtil.addClass(canvas, 'leaflet-zoom-' + (animated ? 'animated' : 'hide'));

      this._ctx = canvas.getContext('2d');
   },
   
   _reset: function () {
      var topLeft = this._map.containerPointToLayerPoint([0, 0]);
      L.DomUtil.setPosition(this._canvas, topLeft);

      var size = this._map.getSize();

      this._canvas.width = size.x;
      this._canvas.height = size.y;

      this._redraw();
   },
   
   _ryw: function(count) {
      var max = this._max;
      var min = this._min;

      var lc = Math.log(count + 1) / Math.log(10);

      var r = Math.floor(256 * Math.min(1, lc));
      var g = Math.floor(256 * Math.min(1, Math.max(0, lc - 1)));
      var b = Math.floor(256 * Math.min(1, Math.max(0, lc - 2)));

      return "rgba(" + r + "," + g + "," + b + "," + 1 + ")";      
   },

   _redraw: function () {
      if (!this._map || !this._data) {
         return;
      }

      var data = this._data;
      var canvas = this._canvas;
      var ctx = this._ctx;

      ctx.globalCompositeOperation = 'lighter';
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      var len = data.length;
      
      while (len--) {
         var entry = data[len];
         var latlng = [entry[0], entry[1]];

         var point = this._map.latLngToContainerPoint(latlng);
         var latlngPoint = { x: Math.round(point.x), y: Math.round(point.y) };

         const size_px = 2.5;
         const offset = size_px / 2;

         ctx.fillStyle = this._ryw(entry[2]);
         ctx.fillRect(latlngPoint.x - offset, latlngPoint.y - offset, size_px, size_px);
      }

      this._frame = null;
   },

   _animateZoom: function (e) {
      var scale = this._map.getZoomScale(e.zoom),
          offset = this._map._getCenterOffset(e.center)._multiplyBy(-scale).subtract(this._map._getMapPanePos());

      if (L.DomUtil.setTransform) {
       L.DomUtil.setTransform(this._canvas, offset, scale);

      } else {
         this._canvas.style[L.DomUtil.TRANSFORM] = L.DomUtil.getTranslateString(offset) + ' scale(' + scale + ')';
      }
   }
});

L.customLayer = function (confg) {
   return new L.CustomLayer(confg);
};
