import { WebSockectService } from './web-sockect.service';
import { MapService } from './map.service';
import { GeocodingService } from './geocoding.service';
import { DataService } from './data.service';
import { ConfigurationService } from './configuration.service';
import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { HttpModule } from '@angular/http';
import { MaterialModule } from '@angular/material';
import 'hammerjs';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { HeatmapComponent } from './heatmap/heatmap.component';
import { NavigatorComponent } from './heatmap/navigator/navigator.component';
import { Demo1Component } from './demo1/demo1.component';
import { Demo2Component } from './demo2/demo2.component';
import { Demo3Component } from './demo3/demo3.component';
import { Demo4Component } from './demo4/demo4.component';
import { TableComponent } from './table/table.component';

@NgModule({
  declarations: [
    AppComponent,
    HeatmapComponent,
    NavigatorComponent,
    Demo1Component,
    Demo2Component,
    Demo3Component,
    Demo4Component,
    TableComponent,
  ],
  imports: [
    BrowserModule,
    FormsModule,
    HttpModule,
    AppRoutingModule,
    MaterialModule
  ],
  providers: [ConfigurationService, DataService, GeocodingService, MapService, WebSockectService],
  bootstrap: [AppComponent]
})
export class AppModule { }
