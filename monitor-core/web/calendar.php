<?php

# LGPL javascript calendar is optional, but very nice.
# Snag it from http://www.dynarch.com/demos/jscalendar/
# and install in jscalendar-1.0/ inside this web dir.

$calendar_head = '
<script type="text/javascript">
function ganglia_submit(clearonly) {
  document.getElementById("cs").value = "";
  document.getElementById("ce").value = "";
  if (! clearonly)
    document.ganglia_form.submit();
}
</script>
';

if ( ! is_readable('./jscalendar-1.0/calendar.js') ) {

  $calendar = '';

 } else {

  $calendar_head .= '
<link rel="stylesheet" type="text/css" media="all" href="./jscalendar-1.0/calendar-system.css" title="calendar-system" />
<script type="text/javascript" src="./jscalendar-1.0/calendar.js"></script>
<script type="text/javascript" src="./jscalendar-1.0/lang/calendar-en.js"></script>
<script type="text/javascript" src="./jscalendar-1.0/calendar-setup.js"></script>
<script type="text/javascript">
var fmt = "%b %d %Y %H:%M";     // must be a format that RRDtool likes
function isDisabled(date, y, m, d) {
  var today = new Date();
  return (today.getTime() - date.getTime()) < -1 * Date.DAY;
}
function checkcal(cal) {        // ensure cs < ce
  var date = cal.date;
  var time = date.getTime();
  var field = document.getElementById("cs");
  if (field == cal.params.inputField) {
    // cs was changed: change ce
    field = document.getElementById("ce");
    var other = new Date(Date.parseDate(field.value, fmt));
    if (time >= other) {
      date = new Date(time + Date.HOUR);
      field.value = date.print(fmt);
    }
  } else {
    // ce was changed: change cs
    field = document.getElementById("cs");
    var other = new Date(Date.parseDate(field.value, fmt));
    if (other >= time) {
      date = new Date(time - Date.HOUR);
      field.value = date.print(fmt);
    }
  }
}
</script>
';

  $calendar = '
<script type="text/javascript">
Calendar.setup({
  inputField     : "cs",
  ifFormat       : fmt,
  showsTime      : true,
  step           : 1,
  weekNumbers    : false,
  onUpdate       : checkcal,
  dateStatusFunc : isDisabled
});
Calendar.setup({
  inputField     : "ce",
  ifFormat       : fmt,
  showsTime      : true,
  step           : 1,
  weekNumbers    : false,
  onUpdate       : checkcal,
  dateStatusFunc : isDisabled
});
</script>
';

 }
?>
