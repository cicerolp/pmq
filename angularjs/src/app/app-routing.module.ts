import { Demo4Component } from './demo4/demo4.component';
import { Demo3Component } from './demo3/demo3.component';
import { Demo2Component } from './demo2/demo2.component';
import { Demo1Component } from './demo1/demo1.component';
import { HeatmapComponent } from './heatmap/heatmap.component';
import { AppComponent } from './app.component';
import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    component: HeatmapComponent
  },
  {
    path: 'app-demo1',
    component: Demo1Component
  },
  {
    path: 'app-demo2',
    component: Demo2Component
  },
  {
    path: 'app-demo3',
    component: Demo3Component
  }/*,
  {
    path: 'app-demo4',
    component: Demo4Component
  }*/
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule],
  providers: []
})
export class AppRoutingModule { }
