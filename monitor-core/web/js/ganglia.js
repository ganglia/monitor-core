$(function(){

  $("#tabs").tabs();
  $( "#range_menu" ).buttonset();
  $( "#sort_menu" ).buttonset();
  jQuery('#metric-search input[name="q"]').liveSearch({url: 'search.php?q=', typeDelay: 800});
  $( "#datepicker-cs" ).datepicker({
	  showOn: "button",
	  buttonImage: "img/calendar.gif",
	  buttonImageOnly: true
  });
  $( "#datepicker-ce" ).datepicker({
	  showOn: "button",
	  buttonImage: "img/calendar.gif",
	  buttonImageOnly: true
  });

  $( "#create-new-view-dialog" ).dialog({
    autoOpen: false,
    height: 200,
    width: 350,
    modal: true,
    close: function() {
      getViewsContent();
      $("#create-new-view-layer").toggle();
      $("#create-new-view-confirmation-layer").html("");
    }
  });

  $( "#metric-actions-dialog" ).dialog({
    autoOpen: false,
    height: 250,
    width: 450,
    modal: true
  });

});

function getViewsContent() {
  $.get('views.php', "" , function(data) {
    $("#tabs-views-content").html('<img src="img/spinner.gif">');
    $("#tabs-views-content").html(data);
    $("#create_view_button")
      .button()
      .click(function() {
	$( "#create-new-view-dialog" ).dialog( "open" );
      });;
    $( "#view_range_chooser" ).buttonset();
    document.getElementById('view_name').value = "default";
  });
  return false;
}

// This one avoids 
function getViewsContentJustGraphs(viewName,range, cs, ce) {
    $("#view_graphs").html('<img src="img/spinner.gif">');
    $.get('views.php', "view_name=" + viewName + "&just_graphs=1&r=" + range + "&cs=" + cs + "&ce=" + ce, function(data) {
	$("#view_graphs").html(data);
	document.getElementById('view_name').value = viewName;
     });
    return false;
}

function createView() {
  $("#create-new-view-confirmation-layer").html('<img src="img/spinner.gif">');
  $.get('views.php', $("#create_view_form").serialize() , function(data) {
    $("#create-new-view-layer").toggle();
    $("#create-new-view-confirmation-layer").html(data);
  });
  return false;
}

function addMetricToView() {
  $.get('views.php', $("#add_metric_to_view_form").serialize() + "&add_to_view=1" , function(data) {
      $("#metric-actions-dialog-content").html('<img src="img/spinner.gif">');
      $("#metric-actions-dialog-content").html(data);
  });
  return false;  
}
function metricActions(host_name,metric_name,type) {
    $( "#metric-actions-dialog" ).dialog( "open" );
    $("#metric-actions-dialog-content").html('<img src="img/spinner.gif">');
    $.get('actions.php', "action=show_views&host_name=" + host_name + "&metric_name=" + metric_name + "&type=" + type, function(data) {
      $("#metric-actions-dialog-content").html(data);
     });
    return false;
}

function autoRotationChooser() {
  $("#tabs-autorotation-chooser").html('<img src="img/spinner.gif">');
  $.get('autorotation.php', "" , function(data) {
      $("#tabs-autorotation-chooser").html(data);
  });
}
function updateViewTimeRange() {
  alert("Not implemented yet");
}

function ganglia_submit(clearonly) {
  document.getElementById("datepicker-cs").value = "";
  document.getElementById("datepicker-ce").value = "";
  if (! clearonly)
    document.ganglia_form.submit();
}