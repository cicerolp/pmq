import { Injectable } from '@angular/core';

@Injectable()
export class ConfigurationService {
  public Server = 'http://localhost:7000/';
  public ApiUrl = 'rest/';
  public ServerWithApiUrl = this.Server + this.ApiUrl;

  constructor() { }
}
