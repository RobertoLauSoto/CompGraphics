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

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

inline uint32_t packRGB(int r, int g, int b){
  return (255<<24) + (int(r)<<16) + (int(g)<<8) + int(b);
}

int main(int argc, char* argv[])
{
  SDL_Event event;

  // std::vector<float> vals = interpolate(2.2, 8.5, 7);
  
  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event);
    update();
    draw();
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
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}




