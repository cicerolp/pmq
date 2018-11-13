import { MapService } from './../map.service';

export class Marker {
  private marker: L.Rectangle = null;
  private drawing = false;
  private p0: any;
  private p1: any;

  private callbacks: any[] = [];

  public marker_opt = {
    color: '#ffffff',
    weight: 2,
  };

  constructor(
    private mapService: MapService,
    private raster: boolean = false) {

    if (raster === true) {
      this.mapService.map.on('mousedown', this.onMouseDown, this);
      this.mapService.map.on('mouseup', this.onMouseUp, this);

      this.mapService.map.on('mousemove', this.onMouseMove, this);
      this.mapService.map.on('mouseout', this.onMouseUp, this);
    }
  }

  public insert(p0: any, p1: any, timeout: number) {
    this.remove();

    this.marker = L.rectangle(L.latLngBounds(p0, p1), this.marker_opt);
    this.mapService.map.addLayer(this.marker);

    setTimeout(this.remove, timeout);
  }

  public remove = () => {
    if (this.marker != null) {
      this.mapService.map.removeLayer(this.marker);
      this.marker = null;
    }

    this.broadcast();
  }

  public register(callback: any): void {
    this.callbacks.push(callback);
  }

  public unregister(callback: any): void {
    this.callbacks = this.callbacks.filter(el => el !== callback);
  }

  public latLngBounds(): L.LatLngBounds {
    if (this.p0 === undefined || this.p1 === undefined) {
      return null;
    } else {
      return L.latLngBounds(this.p0, this.p1);
    }
  }

  private broadcast(): void {
    if (this.raster) {
      if (this.p0 === undefined || this.p1 === undefined) {
        return;
      }

      for (const callback of this.callbacks) {
        callback(L.latLngBounds(this.p0, this.p1), this.mapService.map.getZoom());
      }
    } else {
      for (const callback of this.callbacks) {
        callback(this);
      }
    }
  }

  private onMouseDown(ev: any): void {
    if (ev.originalEvent.button !== 2) {
      return;
    }

    if (this.marker != null) {
      this.mapService.map.removeLayer(this.marker);
      this.marker = null;
    }

    this.drawing = true;
    this.p0 = ev.latlng;
  }

  private onMouseUp(ev: any): void {
    if (ev.originalEvent.button !== 2 || this.drawing === false) {
      return;
    }

    this.p1 = ev.latlng;

    if (this.p0.lat === this.p1.lat && this.p0.lng === this.p1.lng) {
      if (this.marker !== null) {
        this.mapService.map.removeLayer(this.marker);
        this.marker = null;
      }
    }

    this.broadcast();

    this.drawing = false;
  }

  private onMouseMove(ev: any): void {
    if (this.drawing === false) { return; }

    this.p1 = ev.latlng;

    if (this.marker != null) {
      this.mapService.map.removeLayer(this.marker);
    }

    this.marker = L.rectangle(L.latLngBounds(this.p0, this.p1), this.marker_opt);
    this.mapService.map.addLayer(this.marker);
  }
}
