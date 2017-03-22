import { Pin } from './../heatmap/pin';
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
  private alpha = 0.2;
  private radius = 30.0;
  private k = 100;
  private now = 1000;
  private time = 1000;
  private trigger_frquency = 5;
  private trigger_timeout = 3000;
  private max_triggers = 20;
  private currLatlng: any = null;

  @ViewChild(TableComponent)
  private tableComponent: TableComponent;

  private pin: Pin;

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
        if (response.data !== undefined) {
          this.tableComponent.setData(response.data);
        }
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
    this.pin = new Pin(this.mapService);
    this.pin.register(this.callback);

    this.socketService.register(this.topk_ws);
  }

  public callback = (latlng: any, zoom: number): void => {
    this.currLatlng = 'topk/' + zoom
      + '/' + latlng.lat
      + '/' + latlng.lng;

    this.request();
  }

  private request() {
    const action = this.currLatlng
      + '/' + this.alpha
      + '/' + this.radius
      + '/' + this.k
      + '/' + this.now
      + '/' + this.time;

    this.queries.next(action);
  }

  private onAlphaChange(evt: any) {
    if (this.currLatlng !== null) {
      this.request();
    }
  }

  private onRadiusChange(evt: any) {
    if (this.currLatlng !== null) {
      this.request();
    }
  }

  private onKChange(evt: any) {
    if (this.currLatlng !== null) {
      this.request();
    }
  }

  private onNowChange(evt: any) {
    if (this.currLatlng !== null) {
      this.request();
    }
  }

  private onTimeChange(evt: any) {
    if (this.currLatlng !== null) {
      this.request();
    }
  }

  public topk_ws = (evt: any) => {
    if (evt.renew !== false) {
      return;
    }

    for (const coord of evt.triggers) {
      if (this.triggers.length >= this.max_triggers) {
        this.triggers[0].remove();
      }

      const trigger: Marker = new Marker(this.mapService, false);
      trigger.marker_opt.color = '#f4f142';
      trigger.register(this.topk_cb);
      trigger.insert({ lat: coord[0], lon: coord[1] }, { lat: coord[2], lon: coord[3] }, this.trigger_timeout);
      this.triggers.push(trigger);
    }
  }

  public topk_cb = (marker: any) => {
    this.triggers = this.triggers.filter((el) => {
      return el !== marker;
    });
  }

  private onTriggerFrquencyChange(ev: any) {
    const action = 'triggers/'
      + '/' + this.trigger_frquency;

    this.queries.next(action);
  }
}

