import { WebSockectService } from './../web-sockect.service';
import { Component, OnInit } from '@angular/core';

import { Observable } from 'rxjs/Rx';
import { Subject } from 'rxjs/Subject';

import { MapService } from './../map.service';
import { DataService } from './../data.service';

@Component({
  selector: 'app-heatmap',
  templateUrl: './heatmap.component.html',
  styleUrls: ['./heatmap.component.css']
})
export class HeatmapComponent implements OnInit {
  triggers = false;
  data: Observable<any>;
  private queries = new Subject<any>();

  constructor(
    private dataService: DataService,
    private mapService: MapService,
    private socketService: WebSockectService) {
  }

  ngOnInit() {
    this.mapService.load();

    this.mapService.map.on('moveend', this.onViewReset, this);
    this.mapService.map.on('mousedown', this.onMouseDown, this);
    this.mapService.map.on('mouseup', this.onMouseUp, this);

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
        const data = response[0];

        const options = {
          minOpacity: 0.5,
          maxZoom: this.mapService.map.getZoom(),
          max: data.max,
          radius: 5.0,
          blur: 6.0,
        };

        this.mapService.layer.setData(data);
      },
      error => console.log('error: ' + error),
      () => console.log('()')
    );

    this.socketService.register(this.addNextSubject);
  }

  private onViewReset(): void {
    this.addNextSubject({'renew': true});
  }

  private onMouseDown(ev: any): void {
    if (ev.originalEvent.button !== 2) {
      return;
    }
    this.mapService.map.dragging.disable();
  }

  private onMouseUp(ev: any): void {
    this.mapService.map.dragging.enable();
  }

  public addNextSubject = (evt: any) => {
    if (evt.renew !== true) {
      return;
    }

    const region = this.mapService.get_coords_bounds();

    const action = 'tile/' + region.z + '/' + region.x0
      + '/' + region.y0 + '/' + region.x1 + '/' + region.y1;

    this.queries.next(action);
  }
}
