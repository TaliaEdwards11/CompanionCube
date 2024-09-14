// GET request 
// get the value of cell 1,1
function doGet(e) {
  try {
    // get the current spreadsheet from which this apps script was created
    const spreadSheet = SpreadsheetApp.getActive();

    const sheet = spreadSheet.getSheetByName('Sheet1');

    // get the value in cell 1,1  
    const cell = sheet.getRange(1,1); 
    const colour = cell.getValue();

    Logger.log('Current Colour: %d', colour);
    return ContentService.createTextOutput(colour);
  
  } 
  catch (error) {
    Logger.log('Error Detected: %s', error.message);
    return ContentService.createTextOutput("Error detected: " + error.message);
  }
}


// POST request
// set the value of cell 1,1
function doPost(e) {
  try { 
    // get the event's POST body content text 
    const body = JSON.parse(e.postData.contents);

    // get the cell 1,1 object (Range object)
    const spreadSheet = SpreadsheetApp.getActive();
    const sheet = spreadSheet.getSheetByName('Sheet1');  
    let cell = sheet.getRange(1,1); 

    
    // check if the property was set and is a valid integer
    let colour;
    if ("colour" in body){
      colour = body.colour;
    }else{
      throw new Error("The body of the request must contain the colour key.");
    }
    
    if (typeof colour !== "number"){
      throw new TypeError("The colour must be an integer.");
    } else if (colour <= 0 || colour > 10){
      throw new Error("The colour must be an integer in the inclusive range of 1 to 10.");
    }

    // write the value in the cell 
    cell.setValue(colour);

    Logger.log('New Colour: %d', colour);
    return ContentService.createTextOutput(colour);
  } 
  catch(error){
    Logger.log('Error Detected: %s', error.message);
    return ContentService.createTextOutput("Error detected: " + error.message);
  } 
}
