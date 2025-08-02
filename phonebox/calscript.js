function doGet(e) {
  // Return true if today is a phone locking day
  var calendars = CalendarApp.getCalendarsByName('Dirk Kids');

  if (calendars == undefined) {
    Logger.log("No data");
    return ContentService.createTextOutput("no access to calendar");
  }

  var kids_cal = calendars[0];

  // Get the events for today
  var currentTime = new Date();
  var start = new Date(); start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + oneday); // the next 24 hours
  
  var kids_events = kids_cal.getEvents(start, stop)
                 .filter((x) => {
                    var r = x.isAllDayEvent() && x.getTitle().toLowerCase().trim() === "kids";
                    return r;
                  });

  if (kids_events.length < 1) {
    Logger.log("returning false");
    return ContentService.createTextOutput("false");
  }

  var event = kids_events[0];
  var eventStartDate = event.getStartTime();
  var eventEndDate = event.getEndTime(); //this resolves to the next day 00:00:00
  var cutoff = new Date(eventEndDate);
  cutoff.setHours(cutoff.getHours() - 12); //noon on the last day
  
  if (currentTime < cutoff) {
    Logger.log("returning true");
    return ContentService.createTextOutput("true");
  } else {
    Logger.log("returning false");
    return ContentService.createTextOutput("false");
  }
}
