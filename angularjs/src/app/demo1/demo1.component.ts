import { Marker } from './../heatmap/marker';
import { TableComponent } from './../table/table.component';
import { MapService } from './../map.service';
import { DataService } from './../data.service';
import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';

import { Observable } from 'rxjs/Rx';
import { Subject } from 'rxjs/Subject';

@Component({
  selector: 'app-demo1',
  templateUrl: './demo1.component.html',
  styleUrls: ['./demo1.component.css']
})
export class Demo1Component implements OnInit, AfterViewInit {
  @ViewChild(TableComponent)
  private tableComponent: TableComponent;

  private marker: Marker;

  private data: Observable<any>;
  private queries = new Subject<any>();

  constructor(
    private dataService: DataService,
    private mapService: MapService) { }

  ngOnInit() {
    this.data = this.queries
      .debounceTime(40)
      // .distinctUntilChanged()
      .switchMap(term => term ? this.dataService.query(term) : Observable.of<any>([]))
      .catch(error => {
        console.log(error);
        return Observable.of<any>([]);
      });

    const subscription = this.data.subscribe(
      response => {
        this.tableComponent.setData(response.data);
      },
      error => console.log('error: ' + error),
      () => console.log('()')
    );
  }

  ngAfterViewInit() {
    this.marker = new Marker(this.mapService, true);
    this.marker.register(this.callback);
  }

  public callback = (latlng: any, zoom: number): void => {
    const z = zoom + 8;
    const region = this.mapService.get_coords_bounds(latlng, z);

    const action = 'data/' + z
      + '/' + region.x0
      + '/' + region.y0
      + '/' + region.x1
      + '/' + region.y1;

    this.queries.next(action);
  }
}
