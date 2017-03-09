import { Location } from './location';
import { Http } from '@angular/http';
import { Injectable, ElementRef } from '@angular/core';

declare var L: any;
declare var HeatmapOverlay: any;

@Injectable()
export class MapService {
  public map: L.Map;
  public layer: any;

  constructor(private http: Http) { }

  load(): void {
    const layers = {
      black: 'http://{s}.tiles.mapbox.com/v4/cicerolp.mgdebfa9/{z}/{x}/{y}.png?access_token=pk.eyJ1IjoiY2ljZXJvbHAiLCJhIjoia1IxYmtfMCJ9.3EMmwKCCFN-hmsrQY4_wUQ',
      white: 'http://{s}.tiles.mapbox.com/v4/cicerolp.pdni2p2n/{z}/{x}/{y}.png?access_token=pk.eyJ1IjoiY2ljZXJvbHAiLCJhIjoia1IxYmtfMCJ9.3EMmwKCCFN-hmsrQY4_wUQ'
    };

    const base_config = {
      subdomains: 'abcd',
      minZoom: 0,
      maxZoom: 20,
      maxNativeZoom: 20
    };

    const black_base = L.tileLayer(layers.black, base_config);
    const white_base = L.tileLayer(layers.white, base_config);

    const baseMaps = {
      'Black': black_base,
      'White': white_base,
    };

    const cfg = {
      blur: 0.5,
      radius: 5.0,
      minOpacity: 0.75,
      maxOpacity: 1.0,
      scaleRadius: false,
      useLocalExtrema: true,
      latField: '0',
      lngField: '1',
      valueField: '2',
      gradient: {
        '.0': 'blue',
        '.070': 'red',
        '.50': 'white'
      },
    };

    this.layer = new HeatmapOverlay(cfg);

    const overlayMaps = {
      'heatmap.js': this.layer
    };

    this.map = L.map('mapid', {
      // Interaction Options
      boxZoom: false,
      // Map State Options
      center: new L.LatLng(38, -97),
      zoom: 4,
      minZoom: 0,
      maxZoom: 20,
      layers: [black_base, this.layer],
      maxBounds: [
        [-90, -360],
        [+90, +360]
      ],
      // Panning Inertia Options
      worldCopyJump: false,
      maxBoundsViscosity: 1.0,
    });

    L.control.layers(baseMaps, overlayMaps).addTo(this.map);
  }

  disableEvent(el: ElementRef): void {
    L.DomEvent.disableClickPropagation(el.nativeElement);
    L.DomEvent.disableScrollPropagation(el.nativeElement);
  }

  fit(location: Location): void {
    this.map.fitBounds(location.viewBounds, {});
  }

  private tilex2lon(x, z) {
    return x / Math.pow(2.0, z) * 360.0 - 180;
  }

  private tiley2lat(y, z) {
    let n = Math.PI - 2.0 * Math.PI * y / Math.pow(2.0, z);
    return 180.0 / Math.PI * Math.atan(0.5 * (Math.exp(n) - Math.exp(-n)));
  }

  private lon2tilex(lon, z) {
    return ((lon + 180.0) / 360.0 * Math.pow(2.0, z));
  }

  private lat2tiley(lat, z) {
    return ((1.0 - Math.log(Math.tan(lat * Math.PI / 180.0) + 1.0 / Math.cos(lat * Math.PI / 180.0)) / Math.PI) / 2.0 * Math.pow(2.0, z));
  }

  get_coords_bounds(bound: any = undefined, zoom: number = undefined) {
    let b = bound || this.map.getBounds();
    let _z = zoom || this.map.getZoom();

    _z = Math.min(_z, 24);

    let lat0 = b.getNorthEast().lat
    let lon0 = b.getSouthWest().lng;
    let lat1 = b.getSouthWest().lat;
    let lon1 = b.getNorthEast().lng;

    // out of bounds check
    if (lon0 < -180) { lon0 = -180; }
    if (lon1 < -180) { lon1 = -180; }

    if (lon0 > 179) { lon0 = 179.9; }
    if (lon1 > 179) { lon1 = 179.9; }

    if (lat0 < -85) { lat0 = -85; }
    if (lat1 < -85) { lat1 = -85; }

    if (lat0 > 85) { lat0 = 85; }
    if (lat1 > 85) { lat1 = 85; }

    let _x0 = Math.floor(this.lon2tilex(lon0, _z));
    let _x1 = Math.ceil(this.lon2tilex(lon1, _z));

    let _y0 = Math.floor(this.lat2tiley(lat0, _z));
    let _y1 = Math.ceil(this.lat2tiley(lat1, _z));

    return { x0: _x0, y0: _y0, x1: _x1, y1: _y1, z: _z };
  }
}
