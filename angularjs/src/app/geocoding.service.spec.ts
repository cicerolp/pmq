import { TestBed, inject } from '@angular/core/testing';

import { GeocodingService } from './geocoding.service';

describe('GeocodingService', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      providers: [GeocodingService]
    });
  });

  it('should ...', inject([GeocodingService], (service: GeocodingService) => {
    expect(service).toBeTruthy();
  }));
});
