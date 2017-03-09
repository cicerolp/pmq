import { MapService } from './../../map.service';
import { GeocodingService } from './../../geocoding.service';
import { Component, OnInit, ElementRef } from '@angular/core';
import { Map } from 'leaflet';

@Component({
  selector: 'app-navigator',
  templateUrl: './navigator.component.html',
  styleUrls: ['./navigator.component.css']
})
export class NavigatorComponent implements OnInit {
  address: string;

  constructor(
    private geocodingService: GeocodingService,
    private mapService: MapService,
    private ref: ElementRef
  ) { }

  ngOnInit() {
    this.mapService.disableEvent(this.ref);
  }

  goto(): void {
    if (!this.address) {
      return;
    }

    this.geocodingService.geocode(this.address)
      .subscribe(location => {
        this.mapService.fit(location);
        this.address = location.address;
      }, error => console.error(error));
  }
}
