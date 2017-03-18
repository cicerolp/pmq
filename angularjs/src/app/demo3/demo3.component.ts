import { WebSockectService } from './../web-sockect.service';
import { Subject } from 'rxjs/Subject';
import { Observable } from 'rxjs/Rx';
import { Marker } from './../heatmap/marker';
import { MapService } from './../map.service';
import { DataService } from './../data.service';
import { TableComponent } from './../table/table.component';
import { Component, OnInit, ViewChild, AfterViewInit, OnDestroy } from '@angular/core';

@Component({
  selector: 'app-demo3',
  templateUrl: './demo3.component.html',
  styleUrls: ['./demo3.component.css']
})
export class Demo3Component implements OnInit, AfterViewInit, OnDestroy {
  @ViewChild(TableComponent)
  private tableComponent: TableComponent;

  private marker: Marker;
  private triggers: Marker[] = new Array<Marker>();

  private data: Observable<any>;
  private queries = new Subject<any>();

  constructor(
    private dataService: DataService,
    private mapService: MapService,
    private socketService: WebSockectService) { }

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

  ngOnDestroy() {
    this.socketService.unregister(this.topk_ws);
    for (const el of this.triggers) {
      el.remove();
    }
  }

  ngAfterViewInit() {
    this.marker = new Marker(this.mapService, true);
    this.marker.register(this.topk_cb);

    this.socketService.register(this.topk_ws);
  }

  public callback = (latlng: any, zoom: number): void => {
    const z = zoom + 8;
    const region = this.mapService.get_coords_bounds(latlng, z);

    const action = 'topk/' + z
      + '/' + region.x0
      + '/' + region.y0
      + '/' + region.x1
      + '/' + region.y1;

    this.queries.next(action);
  }

  public topk_ws = (evt: any) => {
    if (evt.renew !== false) {
      return;
    }

    for (const coord of evt.triggers) {
      const trigger: Marker = new Marker(this.mapService, false);
      trigger.marker_opt.color = '#3399ff';
      trigger.register(this.topk_cb);
      trigger.insert({ lat: coord[0], lon: coord[1] }, { lat: coord[2], lon: coord[3] }, 3000);
      this.triggers.push(trigger);
    }
  }

  public topk_cb = (marker: any) => {
    this.triggers = this.triggers.filter((el) => {
      return el !== marker;
    });
  }
}

