import { Injectable } from '@angular/core';

import { $WebSocket } from 'angular2-websocket/angular2-websocket';

@Injectable()
export class WebSockectService {
  private ws = new $WebSocket('ws://localhost:7000');
  private callbacks: any[] = [];

  constructor() {
    this.resetConn();
  }

  public register(callback: any): void {
    this.callbacks.push(callback);
  }

  public unregister(callback: any): void {
    this.callbacks = this.callbacks.filter(el => el !== callback);
  }

  private onOpen() {
    this.ws.getDataStream().subscribe(
      msg => {
        for (const callback of this.callbacks) {
          callback(JSON.parse(msg.data));
        }
      }
    );
  }

  private onError() {
    this.ws = null;
    this.ws = new $WebSocket('ws://localhost:7000');
    //this.ws.reconnect();
    this.resetConn();
  }

  private resetConn() {
    this.ws.onError((cb) => {
      this.onError();
    });

    this.ws.onClose((cb) => {
      this.onError();
    });

    this.ws.onOpen((cb) => {
      this.onOpen();
    });
  }
}
