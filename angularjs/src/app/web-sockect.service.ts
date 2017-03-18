import { Injectable } from '@angular/core';

import { $WebSocket } from 'angular2-websocket/angular2-websocket';

@Injectable()
export class WebSockectService {
  private ws = new $WebSocket('ws://localhost:7000');
  private callbacks: any[] = [];

  constructor() {
    this.ws.getDataStream().subscribe(
      msg => {
        for (const callback of this.callbacks) {
          callback(JSON.parse(msg.data));
        }
      }
    );
  }

  public register(callback: any): void {
    this.callbacks.push(callback);
  }

  public unregister(callback: any): void {
    this.callbacks = this.callbacks.filter(el => el !== callback);
  }
}
