import { TestBed, inject } from '@angular/core/testing';

import { WebSockectService } from './web-sockect.service';

describe('WebSockectService', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      providers: [WebSockectService]
    });
  });

  it('should ...', inject([WebSockectService], (service: WebSockectService) => {
    expect(service).toBeTruthy();
  }));
});
