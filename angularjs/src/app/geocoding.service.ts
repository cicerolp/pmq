import { Location } from './location';
import { Http } from '@angular/http';
import { Injectable } from '@angular/core';

import 'rxjs/add/operator/map';
import 'rxjs/add/operator/mergeMap';

@Injectable()
export class GeocodingService {

  constructor(private http: Http) { }

  geocode(address: string) {
    return this.http
      .get('http://maps.googleapis.com/maps/api/geocode/json?address=' + encodeURIComponent(address))
      .map(response => response.json())
      .map(response => {
        if (response.status !== 'OK') {
          throw new Error('unable to geocode address');
        }

        const location = new Location();
        location.address = response.results[0].formatted_address;
        location.latitude = response.results[0].geometry.location.lat;
        location.longitude = response.results[0].geometry.location.lng;

        const viewPort = response.results[0].geometry.viewport;
        location.viewBounds = L.latLngBounds(
          {
            lat: viewPort.southwest.lat,
            lng: viewPort.southwest.lng
          },
          {
            lat: viewPort.northeast.lat,
            lng: viewPort.northeast.lng
          });

        return location;
      });
  }

}
