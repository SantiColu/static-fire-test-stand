import processing.serial.*;
import java.awt.event.KeyEvent;
import java.io.IOException;
Serial myPort;

int smoothLevel = 8;
int[] sizes = { 18, 25, 30, 35, 40, 100, 250 };
PFont[] fonts;

static final String SERIAL_PORT = "/dev/ttyACM0";
static final int SERIAL_RATE = 9600;

String[] stateLabels = {"IDLE", "Countdown", "Ignition", "Active mode"};
String millis = " - ";
String state = "";
String rawData = "";
String value = "";
String ping = "0";
boolean ignited = false;
boolean aborted = false;
String[] allValues = {};
String lastBroadcast = "";

boolean overAbort = false;
boolean overGo = false;

String getStateLabel() {
  if (state.equals("-1")) {
    return "ABORTED";
  }
  return stateLabels[int(state)];
}

void setup() {
  size(1050, 600, P2D);
  
  fonts = new PFont[sizes.length];
  for (int i = 0; i < sizes.length; i++) {
    fonts[i] = createFont("Arial", sizes[i], true);
  }
  
  myPort = new Serial(this, SERIAL_PORT, SERIAL_RATE); // starts the serial communication
  myPort.bufferUntil('\n');
  smooth();
}

void drawBox(String label, String value) {
  rect(0, 0, 160, 55);
  fill(255, 255, 255);
  textFont(fonts[0]);
  textSize(sizes[0]);
  text(label, 5, 20);
  textFont(fonts[1]);
  textSize(sizes[1]);
  text(value, 5, 45);
}

void rawData() {
  fill(153);
  rect(0, 0, 160, 580);
  fill(255, 255, 255);
  textFont(fonts[2]);
  textSize(sizes[2]);
  text("Raw data: ", 10, 40);
  textFont(fonts[0]);
  textSize(sizes[0]);
  text("Ping: " + ping, 10, 70);
  text("State: " + state, 10, 100);
  text("Value: " + value, 10, 130);
  text("Millis: " + millis, 10, 160);
  text("Ignited: " + new Boolean(ignited).toString(), 10, 190);
  text("Aborted: " + new Boolean(aborted).toString(), 10, 220);
  text("Length: " + allValues.length, 10, 250);
  text("LB: \n" + lastBroadcast, 10, 280);
  text("Raw: \n" + rawData, 10, 360);
}

void abortButton() {
  fill(255, 0, 0);
  if (ignited) {
    fill(243, 159, 162);
  } else if (overAbort) {
    fill(178, 0, 0);
  }
  rect(0, 0, 160, 55);
  fill(255, 255, 255);
  textFont(fonts[4]);
  textSize(sizes[4]);
  text("ABORT", 10, 40);
}

void ignitedBox() {
  if (ignited) {
    fill(0, 255, 0);
  } else {
    fill(153);
  }
  rect(0, 0, 180, 55);
  fill(255, 255, 255);
  textFont(fonts[4]);
  textSize(sizes[4]);
  text("IGNITED", 10, 40);
}

void mainBox() {
  fill(180);
  rect(0, 0, 860, 515);
  fill(255, 255, 255);
  translate(10, 10);
  
  switch(state) {
    case "0" : //idle
      fill(0, 255, 0);
      
      if (overGo) {
        fill(0, 180, 0);
      }
      
      rect(0, 0, 840, 495);
      
      fill(255, 255, 255);
      textFont(fonts[6]);
      textSize(sizes[6]);
      text("GO!", 200, 350);
      
      break;
    case "1" : //60s countdown
      int r = abs(((int(value) * 255) / 60) - 255);
      int g = int((int(value) * 255) / 60);
      
      fill(r, g, 0);
      
      rect(0, 0, 840, 495);
      fill(255, 255, 255);
      textAlign(CENTER);
      textFont(fonts[6]);
      textSize(sizes[6]);
      text(value, 400, 350);
      textAlign(LEFT);
      
      break;
    case "2" : //ignition sequence
      case "3" : //active mode (recording)
      
      fill(0, 49, 54);
      
      rect(0, 0, 840, 495);
      fill(255, 255, 255);
      textFont(fonts[5]);
      textSize(sizes[5]);
      text("Thrust:", 200, 150);
      text(value + "g", 200, 250);
      text(String.valueOf(int(value) * 0.0098) + "N", 200, 350);
      
      break;
    case "-1" : //ABORTED
      fill(255, 0, 0);
      rect(0, 0, 840, 495);
      
      fill(255, 255, 255);
      textFont(fonts[5]);
      textSize(sizes[5]);
      text("ABORTED", 150, 280);
      break;
    
    default : //initialising
    fill(156, 36, 255);
    rect(0, 0, 840, 495);
    
    fill(255, 255, 255);
    textFont(fonts[5]);
    textSize(sizes[5]);
    text("Calibrating...", 150, 280);
    break;
  }
  
  stroke(0);
  strokeWeight(0);
}

boolean overButton(int x, int y, int width, int height)  {
  if (mouseX >= x && mouseX <= x + width && 
    mouseY >= y && mouseY <= y + height) {
    return true;
  } else {
    return false;
  }
}

void update() {
  overGo = (overButton(191, 85 , 838, 494) && state.equals("0"));
  overAbort = (overButton(880 , 10 , 160, 55) && !ignited);
}

void mousePressed() {
  if (overAbort) {
    println("Abort!");
    myPort.write("ABORT");
  }
  if (overGo) {
    myPort.write("GO");
    println("Go!");
  }
}

void draw() {
  update();
  background(33);
  if (aborted || state.equals("-1")) {
    background(229, 115, 115);
  }
  
  pushMatrix();
  translate(10, 10);
  
  rawData();
  
  fill(153);
  translate(170, 0);
  drawBox("Ping:", ping + " ms");
  
  fill(153);  
  translate(170, 0);
  drawBox("Time:", millis + " ms");
  
  fill(153);
  translate(170, 0);
  drawBox("State:", getStateLabel());
  
  translate(170, 0);
  ignitedBox();
  
  translate(190, 0);
  abortButton();
  
  popMatrix();
  translate(180, 75);
  mainBox();
}

void serialEvent(Serial myPort) { 
  
  String data = myPort.readStringUntil('\n');
  
  if (data != null) {
    data = trim(data);
    rawData = data;
    
    println(data);
    
    String items[] = split(data, '|');
    
    if (items.length != 3) {
      println("received malformed package", data);
      return;
    }
    
    if (items[0].equals("internal")) {
      lastBroadcast = items[1] + "|" + items[2];
      return;
    }
    
    ping = String.valueOf(int(items[0]) - int(millis));
    
    millis = items[0];
    state = items[1];
    String newValue = items[2];
    
    switch(state) {
      case "0" : //idle
        break;
      case "1" : //60s countdown
        value = newValue;
        break;
      case "2" : //ignition sequence
        case"3" : //active mode (recording)
        if (newValue.equals("HIGH")) {
          ignited = true;
        } else {
          value = newValue;
        }
        
        allValues = append(allValues, millis + "|" + value);
        
        saveStrings("export.txt", allValues);
        
        break;
      case "-1" : //ABORTED
        break;
    }
  }
}
