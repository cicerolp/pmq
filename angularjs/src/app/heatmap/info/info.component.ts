import { MapService } from './../../map.service';
import { GeocodingService } from './../../geocoding.service';
import { WebSockectService } from './../../web-sockect.service';
import { Component, OnInit, ElementRef, OnDestroy } from '@angular/core';

declare var d3: any;

@Component({
  selector: 'app-info',
  templateUrl: './info.component.html',
  styleUrls: ['./info.component.css']
})
export class InfoComponent implements OnInit, OnDestroy {
  formated_date: string = 'null';

  constructor(
    private geocodingService: GeocodingService,
    private mapService: MapService,
    private ref: ElementRef,
    private socketService: WebSockectService
  ) { }

  ngOnInit() {
    this.mapService.disableEvent(this.ref);
    this.socketService.register(this.ws_callback);
  }

  ngOnDestroy() {
    this.socketService.unregister(this.ws_callback);
  }

  public ws_callback = (evt: any) => {
    if (typeof evt.info === 'undefined') {
      return;
    }
    const format = d3.timeFormat('%Y-%m-%dT%H:%M:%S');
    this.formated_date = format(new Date(evt.info[0] * 1000));
  }

}
