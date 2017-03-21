import { MapService } from './../map.service';

export class Pin {
  private pin: L.Marker = null;
  private latlng: any = null;

  private callbacks: any[] = [];

  constructor(private mapService: MapService) {
    this.mapService.map.on('mousedown', this.onMouseDown, this);
  }

  private onMouseDown(ev: any): void {
    if (ev.originalEvent.button !== 2) {
      return;
    }

    if (this.pin !== null) {
      this.mapService.map.removeLayer(this.pin);
      this.pin = null;
    }

    this.latlng = ev.latlng;
    this.pin = L.marker(this.latlng);
    this.mapService.map.addLayer(this.pin);

    this.broadcast();
  }

  private broadcast(): void {
    for (const callback of this.callbacks) {
      callback(this.latlng, this.mapService.map.getZoom());
    }
  }

  public register(cb: any): void {
    this.callbacks.push(cb);
  }

  public unregister(cb: any): void {
    this.callbacks = this.callbacks.filter(el => el !== cb);
  }
}
