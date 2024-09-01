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

  // Get the events for today
  var currentTime = new Date();
  var start = new Date(); start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + oneday); // the next 24 hours

  var events = mergeCalendarEvents(calendars_selected, start, stop); //pull start/stop time
    
  var str = '';

  for (var ii = 0; ii < events.length; ii++) {
    var event = events[ii];

    // Check if the event is named "Kids", is an all-day event, and it's not past midday on the last day
    if (event.getTitle().toLowerCase() === "kids" && event.isAllDayEvent()) {
      var eventEndDate = event.getEndTime();
      var midday = new Date(eventEndDate);
      midday.setHours(12, 0, 0, 0); // set to 12:00 PM on the last day of the event

      if (currentTime < midday) {
        Logger.log("returning true");
        return ContentService.createTextOutput("true");
      }
    }
  }

  Logger.log("returning false");
  return ContentService.createTextOutput("false");
}

function mergeCalendarEvents(calendars, startTime, endTime) {
  var params = { start:startTime, end:endTime, uniqueIds:[] };
  return calendars.map(toUniqueEvents_, params)
                  .reduce(toSingleArray_)
                  .sort(byStart_);
}

function toUniqueEvents_ (calendar) {
  return calendar.getEvents(this.start, this.end)
                 .filter(onlyUniqueEvents_, this.uniqueIds);
}

function onlyUniqueEvents_(event) {
  var eventId = event.getId();
  var uniqueEvent = this.indexOf(eventId) < 0;
  if(uniqueEvent) this.push(eventId);
  return uniqueEvent;
}

function toSingleArray_(a, b) { return a.concat(b) }

function byStart_(a, b) {
  return a.getStartTime().getTime() - b.getStartTime().getTime();
}