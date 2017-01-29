/**********************************************************************
                     Exterminate!
                   /
              ___
      D>=G==='   '.
            |======|
            |======|
        )--/]IIIIII]
           |_______|
           C O O O D
          C O  O  O D
         C  O  O  O  D
         C__O__O__O__D
        [_____________]

This is a sketch for the SainSmart 1.8" TFT display.

  It is a "binary" encoded clock, using a graphical protol/standard
  designed by Roman Black (http://romanblack.com)
  [I think the basic shape of the clock looks like a dalek.]

  Written by Steven Sugg
  copyright 2017
 
 **********************************************************************/

//  Libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <DateTime.h>
#include <DateTimeStrings.h>

// For the breakout, you can use any (2 or) 3 pins
//#define sclk 13
//#define mosi 11
#define cs   10
#define dc   9
#define rst  8  // you can also connect this to the Arduino reset

//Use these pins for the shield!
//#define cs   10
//#define dc   8
//#define rst  0  // you can also connect this to the Arduino reset

//  Time stuff
#define TIME_MSG_LEN  11   // time sync to PC is HEADER and unix time_t as ten ascii digits
#define TIME_HEADER  255   // Header tag for serial time sync message


#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// Option 1: use any pins but a little slower
//Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, mosi, sclk, rst);

/**********************************************************************
Option 2: must use the hardware SPI pins
(for UNO thats sclk = 13 and sid = 11) and pin 10 must be
an output. This is much faster - also required if you want
to use the microSD card (see the image drawing example)
**********************************************************************/
Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

/**********************************************************************
Constants related to the shapes of the sections of the clock
and the overall appearance, including the sizes of the elements,
the spaces between the elements, etc. By altering these constants
the clock's display can be tailored to a particular LCD screen
or just changed to create a better look. Some parameters are in
pixels, others are denoted as percentages of either the full
screen, or within the general size of the clock itself.
/**********************************************************************/
//  0 = unrotated, 1 = rotated 90 degrees, 2 = 180, 3 = 270
#define rotation 0        

#define topBottomMargin 40
#define leftRightMargin 15
#define gap             8
#define pctVertMin      0.1
#define pctVertHour     0.45
#define pctHorizQtr     0.75

#define ptsDim0 8   // Columns
#define ptsDim1 6   // Rows

#define fgColor  ST7735_BLUE


//  Global variables used in the program
uint16_t scrWidth;    //Screen width
uint16_t scrHeight;   //Screen height

//  Define a structure to represent a pair of integer values - x and y cartesian points
typedef struct point{
  uint16_t x;
  uint16_t y;
};

//  We will use a two-dimensional array of point values to define the dalek
point points[ptsDim0][ptsDim1];

//  Globals to hold the time data
byte h0 = 12, m0 = 59, s0 = 40;
long curTime = 0, origTime = -12 * 60 * 60 * 1000 -30000, elapTime = 0;
byte h = 0, m = 0, s = 0, q = 0, qm = 0;
byte hLast = 0, mLast, sLast = 0;

//  Colors
uint16_t bgColor = tft.Color565(150, 150, 200);

//  Define the center and radius of the circle to help draw the top part
uint16_t x0, y0;
float r;

void setup(void) {
  Serial.begin(19200);

  /**********************************************************************
  Our supplier changed the 1.8" display slightly after Jan 10, 2012
  so that the alignment of the TFT had to be shifted by a few pixels
  this just means the init code is slightly different. Check the
  color of the tab to see which init code to try. If the display is
  cut off or has extra 'random' pixels on the top & left, try the
  other option!
  If you are seeing red and green color inversion, use Black Tab
  **********************************************************************/
  // If your TFT's plastic wrap has a Black Tab, use the following:
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  // If your TFT's plastic wrap has a Red Tab, use the following:
  //tft.initR(INITR_REDTAB);   // initialize a ST7735R chip, red tab
  // If your TFT's plastic wrap has a Green Tab, use the following:
  //tft.initR(INITR_GREENTAB); // initialize a ST7735R chip, green tab
  
  Serial.println("Hello, world.");
  Serial.println("Initializing...");
  
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(rotation);
  scrHeight = tft.height();
  scrWidth = tft.width();
  
  Serial.print("Width:  ");
  Serial.println(tft.width(), DEC);
  Serial.print("Height: ");
  Serial.println(tft.height(), DEC);
  Serial.println("Done setup.");

  fillPointsArray();
  printPointsArray();

  /**********************************************************************
  At this point, we have all the "points" necessary to build the dalek,
  especially the quadrangles, but in order to draw the quarters, it is
  more conventient to have the center of the cirlce and the radius. It's
  easy to get  x value for the center of the circle, and we also have
  some points (4) that we know are ON the circle. We will use the x
  value  of the center of the circle and two points on the circle to
  calculalte the y value for the center, using Pythagorean geometry.
  Once we have the center, we'll use Pythagoras again to get the radius.
  /**********************************************************************/

  x0 = round(float(points[3][0].x) + float(gap)/2);
  float x1 = float(points[4][0].x);
  float y1 = float(points[4][0].y);
  float x2 = float(points[7][1].x);
  float y2 = float(points[7][1].y);
  y0 = round(((x2 * x2) - (x1 * x1) + (y2 * y2) - (y1 * y1) + (2 * x1 * float(x0)) - (2 * x2 * float(x0))) / (2 * (y2 - y1)));

  r = sqrt(((x2 - float(x0)) * (x2 - float(x0))) + ((float(y0) - y2) * (float(y0) - y2)));

  Serial.println(x1);
  Serial.println(y1);
  Serial.println(x2);
  Serial.println(y2);
  Serial.println();
  Serial.print("Center of circle: (");
  Serial.print(x0);
  Serial.print(", ");
  Serial.print(y0);
  Serial.println(")");
  Serial.print("Radius: ");
  Serial.println(r);
}

void loop() {
  byte test = 0;
  
  hLast = h;
  mLast = m;
  sLast = s;
  
  curTime = millis();
  elapTime = curTime - origTime;

  h = (h0 + round(elapTime/3600000)-1) % 12 + 1 ;
  m = (m0 + round(elapTime/60000)) % 60 ;
  s = (s0 + round(elapTime/1000)) % 60 ;

  if (s != sLast){test++;}
  if (m != mLast){test++;}
  if (h != hLast){test++;}

//  Serial.println(elapTime);
//  Serial.println(test);
//  Serial.println(hLast);
//  Serial.println(h);
//  Serial.println(mLast);
//  Serial.println(m);
//  Serial.println(sLast);
//  Serial.println(s);
//  Serial.println();
//  delay(2000);

  dalek(h, m, s, test);

//Serial.println(curTime);
//delay(200);

}

/**********************************************************************
The dalek function draws all of the "parts" that make up the
clock display. Based on the time, it either draws filled 
secitons with the hilight color or the background color.
The "test" parameter is to try to determin which sections
have changed since the last call, so we onlly update those sections.
/**********************************************************************/
void dalek(byte h, byte m, byte s, byte test) {
  //  h, m, and s represnet hours, minutes and seconds
  
  // q represents the quarter hour and can have values from 0 - 3.
  q = int(m/15);
  
  //  qm represents the minutes within the quarter hour and can
  //  have values from 0 to 14
  qm = m % 15;

  if (test > 0){
    
    //Need to fill in the code for seconds here
    
    if (test > 1){
      
//      if (bitRead(q, 0)){qtr1(fgColor);}
//      else{qtr1(bgColor);}
//    
//      if (bitRead(q, 1)){qtr2(fgColor);}
//      else{qtr2(bgColor);}
//
      // Minutes
      fourQuads(qm, 4, 5);
    
      if (test > 2){

      // Hours
      fourQuads(h, 2, 3);

      //  Quarters
      twoArcs(qm);

      }
    }
  }
}

void fourQuads(byte t, byte y1, byte y2){
  uint16_t color;

  for(uint16_t i = 0; i < ptsDim0; i += 2){
    color = bgColor;
    if(bitRead(t, (3-i/2))) {color = fgColor;}
    tft.fillTriangle(points[i][y1].x, points[i][y1].y, points[i+1][y1].x, points[i+1][y1].y, points[i][y2].x, points[i][y2].y, color);
    tft.fillTriangle(points[i+1][y2].x, points[i+1][y2].y, points[i+1][y1].x, points[i+1][y1].y, points[i][y2].x, points[i][y2].y, color);
  }
}

void twoArcs(byte t){
  uint16_t color;
  
  for(uint16_t i = 0; i < 2; i++){
    color = bgColor;
    if(bitRead(t, i)){color = fgColor;}
    fillTruncArc((-2 * i + 1), x0, y0, r, color);
  }
  
}

//void qtr1(uint16_t color) {
//  fillTruncArc(1, 62, 60, 44, 17, color);
//}
//
//void qtr2(uint16_t color) {
//  fillTruncArc(-1, 62, 60, 44, 17, color);
//}


void sec(uint16_t color) {
  uint16_t i = 0;
  uint16_t bgColor = tft.Color565(150, 150, 200);

  tft.fillRect(0, 130, tft.width(), 10, ST7735_BLACK);
  for (i = 1; i <= 64; i++){
    tft.drawFastVLine(2*i, 130, 10, bgColor);
    tft.drawFastVLine(2*i+2, 130, 10, bgColor);
    tft.drawFastVLine(2*i+4, 130, 10, bgColor);
    tft.drawFastVLine(2*i+6, 130, 10, bgColor);
    tft.drawFastVLine(2*i+8, 130, 10, bgColor);
    tft.drawFastVLine(2*i+10, 130, 10, bgColor);
    tft.drawFastVLine(2*i+12, 130, 10, color);
    tft.drawFastVLine(2*i+14, 130, 10, color);
    tft.drawFastVLine(2*i+16, 130, 10, color);
    tft.drawFastVLine(2*i+18, 130, 10, color);
    tft.drawFastVLine(2*i+20, 130, 10, bgColor);
    tft.drawFastVLine(2*i+22, 130, 10, bgColor);
    tft.drawFastVLine(2*i+24, 130, 10, bgColor);
    tft.drawFastVLine(2*i+26, 130, 10, bgColor);
    tft.drawFastVLine(2*i+28, 130, 10, bgColor);

    tft.drawFastVLine(2*i, 130, 10, ST7735_BLACK);
    delay(0);
  }
  tft.drawFastVLine(2*i+2, 130, 10, ST7735_BLACK);
 }
 
/**********************************************************************
fillTruncArc is a function that performs the seemingly simple task of
drawing a filled, but truncated, arc, representing the quarter hours
of the clock. Some trig functions are required to get the starting and
ending angles of the arc. After that, it's just a matter of drawing
filled triangles with enough resolution to give the illusion of
filling the arc.
/**********************************************************************/
void fillTruncArc(char whichArc, uint16_t x0, uint16_t y0, float r,  uint16_t color){
  #define aInc 4
  //whichArc contains 1 for right arc and -1 for left arc
  uint16_t a, aMin, aMax;
  uint16_t x, y, xLast, yLast, x1, y1, dx, dy;
  float fv;
  uint16_t yOffset = y0 - points[0][1].y;

  /**********************************************************************  
  Get the last angle through the maximum point on the arc (maximum y
  value). We don't know what the maximum y value will be, but we do know
  the x value where the maximum y value will occur, so we can use that
  to calculate the angle using the arccosine function.
  **********************************************************************/
  dx = round(float(gap)/2);
  aMax = round(acos(float(dx)/r) * (180/M_PI));

  /**********************************************************************  
  Get the first angle through the minumum point on the arc (minimum y
  value). Unlinke the maximum y value, we DO know what the minumim y
  value will be so we can use that to directly calcuate the angle using
  the arcsine function.
  **********************************************************************/
  dy = yOffset;
  aMin = round(asin(float(dy)/r) * (180/M_PI));

  //  Set the known values for the first two points of the first triangle
  //  using the functions input parameters and the pythagorean formula.
  x1 = round(float(x0) + float(whichArc) * round(float(gap)/2));
  y1 = y0 - yOffset;
  dx = round(sqrt((r * r) - (float(dy) * float(dy))));
  xLast = x0 + (whichArc * dx);
  yLast = y1;

  //  Now, draw a series of filled triangles, "walking" around the arc
  //  in incremental angled steps (aInc).
  for (a = aMin; a < aMax; a += aInc){
    dx = round(r * cos(float(a) * M_PI/180));
    dy = round(r * sin(float(a) * M_PI/180));
    x = x0 + (whichArc * dx);
    y = y0 - dy;
    tft.fillTriangle(x1, y1, xLast, yLast, x, y, color);
    xLast = x;
    yLast = y;
  }

  //  Draw the last triangle.
  //  As soon as the increment goes past the last point on the arc, we
  //  exit the for loop, but we need to draw one more triangle.
  dx = round(float(gap)/2);
  dy = round(sqrt((r * r) - (float(dx) * float(dx))));
  x = x0 + (whichArc * dx);
  y = y0 - dy;
  tft.fillTriangle(x1, y1, xLast, yLast, x, y, color);
}

void fillPointsArray(){
  uint16_t j, i;
  uint16_t imgHeight, imgWidth;

  imgHeight = scrHeight - (2 * topBottomMargin);
  imgWidth = scrWidth - (2 * leftRightMargin);
  
  // Initialize - fill with zeroes
  for(j = 0; j < ptsDim0; j++){
    for(i = 0; i<= ptsDim1; i++){
      points[i][j].x = 0;
      points[i][j].y = 0;
    }
  }
  
  //  The y values
  for(i = 0; i < ptsDim0; i++){
    points[i][5].y = scrHeight - topBottomMargin;
  }
  
  for(i = 0; i < ptsDim0; i++){
    points[i][4].y = points[i][5].y - round(pctVertMin * imgHeight);
  }
  
  for(i = 0; i < ptsDim0; i++){
    points[i][3].y = points[i][4].y - gap;
  }
  
  for(i = 0; i < ptsDim0; i++){
    points[i][2].y = points[i][3].y - round(pctVertHour * imgHeight);
  }
  
  for(i = 0; i < ptsDim0; i++){
    points[i][1].y = points[i][2].y - gap;
  }
  
  for(i = 0; i < ptsDim0; i++){
    points[i][0].y = topBottomMargin;
  }
  
  
  //  The x values
  float segWidth = (float(imgWidth) - (3 * float(gap))) / 4;
  for(j = 3; j < 5; j++){
    for(i = 0; i < ptsDim0; i += 2){
      points[i][j].x = leftRightMargin + round(float(i) * (segWidth + float(gap)) / 2);
      points[i+1][j].x = round(points[i][j].x + float(segWidth));
    }
  }

  points[0][1].x = round(float(leftRightMargin) + ((float(imgWidth) * (1 - pctHorizQtr)) / 2));
  points[7][1].x = round(float(leftRightMargin) + ((float(imgWidth) * (1 + pctHorizQtr)) / 2));

  //  Calculate the slope of the angle of the "tilt" of the quadrangle
  float m = (float(points[0][3].y) - float(points[0][1].y)) / (float(points[0][3].x) - float(points[0][1].x));

  //  Calculate the x values for the bottom row of quadrangles (quarter minutes)
  points[0][2].x = round(float(points[0][3].x) + ((float(points[0][2].y) - float(points[0][3].y)) / m));
  points[1][2].x = round(float(points[0][2].x) + segWidth);
  points[2][2].x = round(float(points[1][2].x) + float(gap));
  points[7][2].x = scrWidth - points[0][2].x;
  points[6][2].x = round(float(points[7][2].x) - segWidth);
  points[5][2].x = points[6][2].x - gap;

  //  Calculte the x vals for the middle row of quadrangles (hours)
  points[0][5].x = round(float(points[0][4].x) + ((float(points[0][4].y) - float(points[0][5].y)) / m));
  points[1][5].x = round(float(points[0][5].x) + segWidth);
  points[2][5].x = round(float(points[1][5].x) + float(gap));
  points[7][5].x = scrWidth - points[0][5].x;
  points[6][5].x = round(float(points[7][5].x) - segWidth);
  points[5][5].x = points[6][5].x - gap;

  //  Now do/redo all the "middle" x's - 3 & 4 to make sure they are all the same
  for(j = 0; j < ptsDim1; j++){
    points[3][j].x = round((float(scrWidth) - float(gap)) / 2.0);
    points[4][j].x = round((float(scrWidth) + float(gap)) / 2.0);
  }
}

void printPointsArray(){
  for(uint16_t j = 0; j < ptsDim1; j++){
    for(uint16_t i = 0; i < ptsDim0; i++){
      Serial.print("(");
      Serial.print(points[i][j].x);
      Serial.print(", ");
      Serial.print(points[i][j].y);
      Serial.print("),  ");
    }
    Serial.println();
  }
}

