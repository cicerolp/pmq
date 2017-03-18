import { Subject } from 'rxjs/Subject';
import { Observable } from 'rxjs/Rx';
import { Pin } from './../heatmap/pin';
import { MapService } from './../map.service';
import { DataService } from './../data.service';
import { TableComponent } from './../table/table.component';
import { Component, OnInit, ViewChild, AfterViewInit } from '@angular/core';

@Component({
  selector: 'app-demo2',
  templateUrl: './demo2.component.html',
  styleUrls: ['./demo2.component.css']
})
export class Demo2Component implements OnInit, AfterViewInit {
  private alpha = 0.2;
  private radius = 30.0;
  private k = 100;
  private now = 1000;
  private time = 1000;
  private currLatlng: any = null;

  @ViewChild(TableComponent)
  private tableComponent: TableComponent;

  private pin: Pin;

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
    this.pin = new Pin(this.mapService);
    this.pin.register(this.callback);
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
}
