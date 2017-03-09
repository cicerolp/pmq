import { PmqPage } from './app.po';

describe('pmq App', () => {
  let page: PmqPage;

  beforeEach(() => {
    page = new PmqPage();
  });

  it('should display message saying app works', () => {
    page.navigateTo();
    expect(page.getParagraphText()).toEqual('app works!');
  });
});
