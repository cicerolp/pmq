$( function() {
   $("#progressbar .ui-progressbar-value").addClass("ui-corner-right");
   $("#progressbar").progressbar({
      value: 0
   });
});

function update_progressbar_label() {
   var progressbar = $( "#progressbar" ),
         progressLabel = $( "#progress-label" );

   progressLabel.text("Count: " + progressbar.progressbar("value") + " of " + progressbar.progressbar("option", "max"));
}

function set_progressbar_max(value) {
   $( "#progressbar" ).progressbar( "option", "max", value[0]);
   update_progressbar_label();
}

function set_progressbar_value(value) {
   $( "#progressbar" ).progressbar( "value", value[0]);
   update_progressbar_label();
}