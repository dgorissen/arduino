/* Inspiration / source -  https://www.instructables.com/E-Ink-Family-Calendar-Using-ESP32/ */

function doGet(e) {
  var calendars = CalendarApp.getCalendarsByName('Dirk Kids');

  if (calendars == undefined) {
    Logger.log("No data");
    return ContentService.createTextOutput("no access to calendar");
  }

  var calendars_selected = [];

  for (var ii = 0; ii < calendars.length; ii++) {
    if (calendars[ii].isSelected()) {
      calendars_selected.push(calendars[ii]);
      Logger.log(calendars[ii].getName());
    }
  }

  Logger.log("Old: " + calendars.length + " New: " + calendars_selected.length);

  const now = new Date();
  var start = new Date(); start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24 * 3600000; // [msec]
  const stop = new Date(start.getTime() + 1 * oneday); //get appointments for the next 14 days

  var events = mergeCalendarEvents(calendars_selected, start, stop); //pull start/stop time

  var str = '';
  for (var ii = 0; ii < events.length; ii++) {

    var event = events[ii];
    var myStatus = event.getMyStatus();

    // Define valid entryStatus to populate array
    switch (myStatus) {
      case CalendarApp.GuestStatus.OWNER:
      case CalendarApp.GuestStatus.YES:
      case CalendarApp.GuestStatus.NO:
      case CalendarApp.GuestStatus.INVITED:
      case CalendarApp.GuestStatus.MAYBE:
      default:
        break;
    }

    // Show just every entry regardless of GuestStatus to also get events from shared calendars where you haven't set up the appointment on your own
    // str += event.getStartTime() + ';' +
    // event.getTitle() +';' + 
    // event.isAllDayEvent() + ';';

    if (event.getTitle().toLowerCase() == "kids") {
      Logger.log("returning true");
      return ContentService.createTextOutput("true");
    }
  }
  Logger.log("returning false");
  return ContentService.createTextOutput("false");
}

function mergeCalendarEvents(calendars, startTime, endTime) {
  var params = { start: startTime, end: endTime, uniqueIds: [] };
  return calendars.map(toUniqueEvents_, params)
    .reduce(toSingleArray_)
    .sort(byStart_);
}

function toUniqueEvents_(calendar) {
  return calendar.getEvents(this.start, this.end)
    .filter(onlyUniqueEvents_, this.uniqueIds);
}

function onlyUniqueEvents_(event) {
  var eventId = event.getId();
  var uniqueEvent = this.indexOf(eventId) < 0;
  if (uniqueEvent) this.push(eventId);
  return uniqueEvent;
}

function toSingleArray_(a, b) { return a.concat(b) }

function byStart_(a, b) {
  return a.getStartTime().getTime() - b.getStartTime().getTime();
}