$(function(){

  // Follow tab's URL instead of loading its content via ajax
  $("#tabs").tabs();
  // Restore previously selected tab
  var selected_tab = $.cookie("ganglia-selected-tab");
  if ((selected_tab != null) && (selected_tab.length > 0)) {
    try {
      var tab_index = parseInt(selected_tab, 10);
      if (!isNaN(tab_index) && (tab_index >= 0)) {
        //alert("ganglia-selected-tab: " + tab_index);
        $("#tabs").tabs("select", tab_index);
        switch (tab_index) {
          case 2:
            getViewsContent();
            break;
          case 4:
            autoRotationChooser();
            break;
        }
      }
    } catch (err) {
      alert("Error(ganglia.js): Unable to select tab: " + 
            tab_index + ". " + err.getDescription());
    }
  }

  $("#tabs").bind("tabsselect", function(event, ui) {
    // Store selected tab in a session cookie
    $.cookie("ganglia-selected-tab", ui.index);
  });

  $( "#range_menu" ).buttonset();
  $( "#sort_menu" ).buttonset();

  jQuery('#metric-search input[name="q"]').liveSearch({url: 'search.php?q=', typeDelay: 500});

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

function selectView(view_name) {
  $.cookie('ganglia-selected-view', view_name); 
  var range = $.cookie('ganglia-view-range');
  if (range == null)
    range = '1hour';
  getViewsContentJustGraphs(view_name, range, '', '');
}

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

    // Restore previously selected view
    var view_name = document.getElementById('view_name');
    var selected_view = $.cookie("ganglia-selected-view");
    if (selected_view != null) {
        view_name.value = selected_view;
	var range = $.cookie("ganglia-view-range");
	if (range == null)
          range = "hour";
	$("#view-range-"+range).click();
    } else
      view_name.value = "default";
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
