import { ConfigurationService } from './configuration.service';
import { Injectable } from '@angular/core';
import { Headers, Http } from '@angular/http';

import { Observable } from 'rxjs/Observable';

@Injectable()
export class DataService {
  private actionUrl: string;
  private headers = new Headers();

  constructor(private http: Http, private config: ConfigurationService) {
    this.headers.append('Content-Type', 'application/json');
    this.headers.append('Accept', 'application/json');
  }

  private handleError(error: any): Observable<any> {
    console.error('An error occurred', error);
    return Observable.throw(error.json().error || 'Server error');
  }

  query(action: string): Observable<any> {
    return this.http
      .get(this.config.ServerWithApiUrl + 'query/' + action)
      .map(response => response.json());
  }
}
