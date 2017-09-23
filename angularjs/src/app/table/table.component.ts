import { Component, OnInit, AfterViewInit, ElementRef } from '@angular/core';

declare var $: any;
declare var d3: any;
declare var window: any;

@Component({
  selector: 'app-table',
  templateUrl: './table.component.html',
  styleUrls: ['./table.component.css']
})
export class TableComponent implements OnInit, AfterViewInit {
  private data: any[] = [];
  private height = 0;

  constructor(private ref: ElementRef) {

  }

  ngOnInit(): void {

  }

  ngAfterViewInit(): void {
    this.redraw();
    $(window).resize(function () {
      if (this.resizeTO) {
        clearTimeout(this.resizeTO);
      }
      this.resizeTO = setTimeout(function () {
        $(this).trigger('resizeEnd');
      }, 500);
    });

    $(window).bind('resizeEnd', this.redraw);
  }

  setData(data: any[]): void {
    this.data = data;

    const datatable = $('#async_table').dataTable().api();

    datatable.clear();
    datatable.rows.add(this.data);
    datatable.draw();
  }

  public redraw = (): void => {
    console.log();
    const topk_h = Math.max($('#topk').height() || 0, 0);

    const height = Math.max($(this.ref.nativeElement).parent().height() - topk_h - 114, 50);

    // const widht = Math.max($(this.ref.nativeElement).parent().width(), 50);

    if (this.height === height) {
      return;
    }

    this.height = height;

    $('#async_table').DataTable({
      iDisplayLength: 100,
      deferRender: true,
      info: false,
      searching: false,
      destroy: true,
      scrollY: height,
      responsive: {
        details: true,
      },
      data: this.data,
      columnDefs: [
        {
          'render': function (data, type, row) {
            let format = d3.timeFormat('%Y-%m-%dT%H:%M:%S');
            return format(new Date(data * 1000));
            // return data;
          },
          'targets': 0
        }, {
          'render': function (data, type, row) {
            const str = String(data);

            const cellHtml = '<textarea wrap="soft" rows="5" cols="30" readonly>' + str + '</textarea>';
            return cellHtml;
          },
          'targets': 1
        }

        /*{
          'render': function (data, type, row) {
            let str = String(data);

            if (str === '14') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/none.png\'/>';
            } else if (str === '13') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/none.png\'/>';
            } else if (str === '12') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/ru.png\'/>';
            } else if (str === '11') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/pt.png\'/>';
            } else if (str === '10') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/pl.png\'/>';
            } else if (str === '9') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/nl.png\'/>';
            } else if (str === '8') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/ko.png\'/>';
            } else if (str === '7') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/none.png\'/>';
            } else if (str === '6') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/it.png\'/>';
            } else if (str === '5') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/fr.png\'/>';
            } else if (str === '4') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/none.png\'/>';
            } else if (str === '3') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/de.png\'/>';
            } else if (str === '2') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/es.png\'/>';
            } else if (str === '1') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/en.png\'/>';
            } else if (str === '0') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/flags/pirate.png\'/>';
            }

            const cellHtml = '<span originalValue=\'' + data + '\'>' + str + '</span>';
            return cellHtml;

          },
          'targets': 1
        }, {
          'render': function (data, type, row) {
            let str = String(data);

            if (str === '4') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/windows-icon.ico\'/>';
            } else if (str === '3') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/apple-icon.ico\'/>';
            } else if (str === '2') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/android-icon.png\'/>';
            } else if (str === '1') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/apple-icon.ico\'/>';
            } else if (str === '0') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/other-icon.png\'/>';
            }

            const cellHtml = '<span originalValue=\'' + data + '\'>' + str + '</span>';
            return cellHtml;
          },
          'targets': 2
        }, {
          'render': function (data, type, row) {
            let str = String(data);

            if (str === '3') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/instagram.png\'/>';
            } else if (str === '2') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/foursquare.png\'/>';
            } else if (str === '1') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/twitter.jpg\'/>';
            } else if (str === '0') {
              str = '<img style=\'vertical-align: middle;\' src=\'/src/assets/chrome.png\'/>';
            }

            const cellHtml = '<span originalValue=\'' + data + '\'>' + str + '</span>';
            return cellHtml;
          },
          'targets': 3
        }*/
      ],
      dom: '<"top">rt<"bottom"><"clear">'
    });
  }
}
