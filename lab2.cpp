#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void draw();
void update();
void handleEvent(SDL_Event event);

std::vector<float> interpolate(float from, float to, int numOfValues);

void drawLine(CanvasPoint from, CanvasPoint to, int colour);
void drawTriangle(CanvasTriangle t, int colour);
void drawStrandTriangle();
CanvasTriangle sortVertices(CanvasTriangle t);
void fillTriangle(CanvasTriangle t, uint32_t colour);
void drawFillTriangle();

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

inline uint32_t packRGB(int r, int g, int b){
  return (255<<24) + (int(r)<<16) + (int(g)<<8) + int(b);
}

int main(int argc, char* argv[])
{
  SDL_Event event;

  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event);
    update();
    // draw();
    drawLine({10, 10}, {100, 100}, packRGB(255, 0, 0));

    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
}

std::vector<float> interpolate(float from, float to, int numOfValues) {
  
  std::vector<float> a;
  float step = (to - from) / (numOfValues - 1);

  for(int i=0; i<numOfValues; i++) {
    a.push_back(from + (i*step));
  }

  return a;
}

std::vector<vec3> threeInterpolate(vec3 from, vec3 to, int numOfValues) {
  
  std::vector<vec3> a;
  vec3 step = (to - from) / (vec3)(float)(numOfValues - 1);
  vec3 acc = from;

  for(int i=0; i<numOfValues; i++) {
    a.push_back(acc);
    acc += step;
  }

  return a;
}

void draw() {

  vec3 topLeft (255, 0, 0);
  vec3 topRight (0,0,255);
  vec3 bottomLeft (255,255,0);
  vec3 bottomRight (0,255,0);

  window.clearPixels();
  std::vector<vec3> leftColumn = threeInterpolate(topLeft, bottomLeft, window.height);
  std::vector<vec3> rightColumn = threeInterpolate(topRight, bottomRight, window.height);

  for(int y=0; y<window.height ;y++) {
    vec3 from = leftColumn[y];
    vec3 to = rightColumn[y];

    std::vector<vec3> vals = threeInterpolate(from, to, window.width);

    for(int x=1; x<window.width ;x++) {
      vec3 val = vals[x];
      uint32_t colour = packRGB(val.x, val.y, val.z);
      window.setPixelColour(x, y, colour);
    }
  }
}

void greyscaledraw(){
  window.clearPixels();
  std::vector<float> values = interpolate(255, 0, window.width);
  for(int y=0; y<window.height ;y++) {
    for(int x=0; x<window.width ;x++) {
      int value = (int) values[x];
      uint32_t colour = packRGB(value, value, value);
      window.setPixelColour(x, y, colour);
    }
  }
}

void update()
{
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) cout << "LEFT" << endl;
    else if(event.key.keysym.sym == SDLK_RIGHT) cout << "RIGHT" << endl;
    else if(event.key.keysym.sym == SDLK_UP) cout << "UP" << endl;
    else if(event.key.keysym.sym == SDLK_DOWN) cout << "DOWN" << endl;
    else if(event.key.keysym.sym == SDLK_u) drawStrandTriangle();
    else if(event.key.keysym.sym == SDLK_f) drawFillTriangle();
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}

void drawLine(CanvasPoint from, CanvasPoint to, int colour) {
  float xDiff = to.x - from.x;
  float yDiff = to.y - from.y;
  float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
  float xStepSize = xDiff/numberOfSteps;
  float yStepSize = yDiff/numberOfSteps;

  for (float i=0.0; i<numberOfSteps; i++) {
    float x = from.x + (xStepSize*i);
    float y = from.y + (yStepSize*i);
    window.setPixelColour(x, y, colour);
  }
}

void drawTriangle(CanvasTriangle t, int colour) {
  CanvasPoint t0 = t.vertices[0];
  CanvasPoint t1 = t.vertices[1];
  CanvasPoint t2 = t.vertices[2];

  drawLine(t0, t1, colour);
  drawLine(t0, t2, colour);
  drawLine(t1, t2, colour);
}

void drawStrandTriangle() {
  int colour = packRGB(rand() % 255, rand() % 255, rand() % 255);
  float x0 = rand() % window.width;
  float y0 = rand() % window.height;
  float x1 = rand() % window.width;
  float y1 = rand() % window.height;
  float x2 = rand() % window.width;
  float y2 = rand() % window.height;
  drawTriangle({{x0, y0}, {x1, y1}, {x2, y2}}, colour);
}

void drawFillTriangle() {
  int colour = packRGB(rand() % 255, rand() % 255, rand() % 255);
  float x0 = rand() % window.width;
  float y0 = rand() % window.height;
  float x1 = rand() % window.width;
  float y1 = rand() % window.height;
  float x2 = rand() % window.width;
  float y2 = rand() % window.height;
  printf("x0 = %f\n", x0);
  printf("y0 = %f\n", y0);
  printf("x1 = %f\n", x1);
  printf("y1 = %f\n", y1);
  printf("x2 = %f\n", x2);
  printf("y2 = %f\n", y2);
  fillTriangle({{x0, y0}, {x1, y1}, {x2, y2}}, colour);
}

CanvasTriangle sortVertices(CanvasTriangle t) {
  if (t.vertices[1].y < t.vertices[0].y) {
    std::swap(t.vertices[0].x, t.vertices[1].x);
    std::swap(t.vertices[0].y, t.vertices[1].y);
  }
  if (t.vertices[2].y < t.vertices[1].y) {
    std::swap(t.vertices[1].x, t.vertices[2].x);
    std::swap(t.vertices[1].y, t.vertices[2].y);
    if (t.vertices[1].y < t.vertices[0].y) {
      std::swap(t.vertices[0].x, t.vertices[1].x);
      std::swap(t.vertices[0].y, t.vertices[1].y);
    }
  }
  return t;
}

void fillTriangle(CanvasTriangle t, uint32_t colour) {
  CanvasTriangle sortedT = sortVertices(t);

  CanvasPoint minPoint = sortedT.vertices[0];
  CanvasPoint midPoint = sortedT.vertices[1];
  CanvasPoint maxPoint = sortedT.vertices[2];

  printf("minpoint.x = %f\n", minPoint.x);
  printf("midpoint.x = %f\n", midPoint.x);
  printf("maxpoint.x = %f\n", maxPoint.x);


  float minMaxSlope = (maxPoint.x - minPoint.x) / (maxPoint.y - minPoint.y);
  float minMidSlope = (midPoint.x - minPoint.x) / (midPoint.y - minPoint.y);
  float midMaxSlope = (maxPoint.x - midPoint.x) / (maxPoint.y - midPoint.y);

  float xExtraPoint = minPoint.x + minMaxSlope*(midPoint.y - minPoint.y);

  CanvasPoint extraPoint = {xExtraPoint, midPoint.y};

  for (float y=minPoint.y; y < midPoint.y; ++y) {
    float x1 = minPoint.x +  minMaxSlope*(y - minPoint.y);
    float x2 = minPoint.x +  minMidSlope*(y - minPoint.y);
    drawLine({x1, y}, {x2, y}, colour);
  }

  for (float y=midPoint.y; y < maxPoint.y; ++y) {
    float x1 = extraPoint.x +  minMaxSlope*(y - midPoint.y);
    float x2 = midPoint.x +  midMaxSlope*(y - midPoint.y);
    drawLine({x1, y}, {x2, y}, colour);
  }
}


