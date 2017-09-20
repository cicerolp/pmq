'use strict';

function convertToRange(value, srcRange, dstRange) {
  // value is outside source range return
  if (value < srcRange[0] || value > srcRange[1]) {
    return NaN;
  }

  var srcMax = srcRange[1] - srcRange[0],
    dstMax = dstRange[1] - dstRange[0],
    adjValue = value - srcRange[0];

  return (adjValue * dstMax / srcMax) + dstRange[0];

}

L.CustomLayer = (L.Layer ? L.Layer : L.Class).extend({

  initialize: function (confg) {
    this._config = confg;
  },

  onAdd: function (map) {
    this._map = map;

    var pane = map.getPane(this.options.pane);

    if (!this._canvas) {
      this._initCanvas();
    }

    pane.appendChild(this._canvas);

    map.on('zoomstart', this._clear, this);

    this._reset();
  },

  onRemove: function (map) {
    this.getPane().removeChild(this._canvas);

    map.off('zoomstart', this._clear, this);
  },

  setData: function (data) {
    if (this._data &&
      this._data.length === data.data.length &&
      this._max === data.max && this._min === data.min) {
      return;
    }

    this._max = data.max || this._max;
    this._min = data.min || this._min;

    this._data = data.data;
    return this.redraw();
  },

  redraw: function () {
    if (!this._frame) {
      this._frame = L.Util.requestAnimFrame(this._redraw, this);
    }
    this._reset();
    return this;
  },

  _initCanvas: function () {
    var canvas = this._canvas = L.DomUtil.create('canvas', 'leaflet-heatmap-layer leaflet-layer');

    var size = this._map.getSize();
    canvas.width = size.x;
    canvas.height = size.y;

    var animated = this._map.options.zoomAnimation && L.Browser.any3d;
    L.DomUtil.addClass(canvas, 'leaflet-zoom-' + 'hide');

    this._ctx = canvas.getContext('2d');
  },

  _clear: function () {
    var canvas = this._canvas;
    var ctx = this._ctx;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
  },

  _reset: function () {
    if (!this._map || !this._data) {
      return;
    }

    var topLeft = this._map.containerPointToLayerPoint([0, 0]);
    L.DomUtil.setPosition(this._canvas, topLeft);

    var size = this._map.getSize();

    this._canvas.width = size.x;
    this._canvas.height = size.y;

    //this._redraw();
  },

  _ryw: function (count) {
    //var max = this._max;
    //var min = this._min;

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

    ctx.globalCompositeOperation = 'screen';
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    var len = data.length;
    var bounds = this._map.getBounds();

    while (len--) {
      var entry = data[len];
      var latlng = [entry[0], entry[1]];

      if (!bounds.contains(latlng)) {
        continue;
      }

      var point = this._map.latLngToContainerPoint(latlng);

      const size_px = convertToRange(entry[2], [this._min, this._max], [1.2, 4.0]);
      const offset = size_px / 2.0;

      ctx.fillStyle = this._ryw(entry[2]);

      // draw a rectangle
      ctx.fillRect(point.x - offset, point.y - offset, size_px + offset, size_px + offset);
    }

    this._frame = null;
  }
});

let HeatmapOverlay = function (confg) {
  return new L.CustomLayer(confg);
};
/**/
