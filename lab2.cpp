#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <Image.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480

void draw();
void update();
void handleEvent(SDL_Event event);
Image readPPM(std::string filename);

std::vector<float> interpolate(float from, float to, int numOfValues);

void drawLine(CanvasPoint from, CanvasPoint to, int colour);
void drawTriangle(CanvasTriangle t, int colour);
void drawStrandTriangle();
CanvasTriangle sortVertices(CanvasTriangle t);
void fillTriangle(CanvasTriangle t, uint32_t colour);
void textureTriangle(CanvasTriangle t, std::string texture);
void drawFillTriangle();
void drawTextureTriangle();

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
    // drawLine({10, 10}, {100, 100}, packRGB(255, 0, 0));

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
    else if(event.key.keysym.sym == SDLK_t) drawTextureTriangle();
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

void drawTextureLine(CanvasPoint from, CanvasPoint to, Image sourceTexture) {
  CanvasPoint pixel;
  int textureArrayIndex, textureRed, textureGreen, textureBlue;
  
  if (to.x < from.x) {
    std::swap(to.x, from.x);
    std::swap(to.y, from.y);
    std::swap(to.texturePoint.x, from.texturePoint.x);
    std::swap(to.texturePoint.y, from.texturePoint.y);
  }

  for (float x=from.x; x < to.x; ++x) {
    float lambda = (x - from.x) / (to.x - from.x);
    pixel.x = x;
    pixel.y = (1-lambda)*from.y + lambda*to.y;
    pixel.texturePoint.x = std::round((1-lambda)*from.texturePoint.x + lambda*to.texturePoint.x);
    pixel.texturePoint.y = std::round((1-lambda)*from.texturePoint.y + lambda*to.texturePoint.y); 
    textureArrayIndex = (pixel.texturePoint.y*sourceTexture.w) + pixel.texturePoint.x;
    textureRed = sourceTexture.pixels[textureArrayIndex].r*255;
    textureGreen = sourceTexture.pixels[textureArrayIndex].g*255;
    textureBlue = sourceTexture.pixels[textureArrayIndex].b*255;
    int colour = packRGB(textureRed, textureGreen, textureBlue);
    window.setPixelColour(pixel.x, pixel.y, colour);
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

  fillTriangle({{x0, y0}, {x1, y1}, {x2, y2}}, colour);
}

void drawTextureTriangle() {
  std::string texture = "brickTexture.ppm";
  int colour = packRGB(255, 255, 255);
  Image sourceTexture = readPPM(texture);
  CanvasTriangle t;
  CanvasPoint a, b, c;
  // a.x = 160;
  // a.y = 10;
  // b.x = 300;
  // b.y = 230;
  // c.x = 10;
  // c.y = 150;
  // a.texturePoint.x = 195;
  // a.texturePoint.y = 5;
  // b.texturePoint.x = 395;
  // b.texturePoint.y = 380;
  // c.texturePoint.x = 65;
  // c.texturePoint.y = 330;
  a.x = rand() % WIDTH;
  a.y = rand() % HEIGHT;
  b.x = rand() % WIDTH;
  b.y = rand() % HEIGHT;
  c.x = rand() % WIDTH;
  c.y = rand() % HEIGHT;
  a.texturePoint.x = rand() % sourceTexture.w;
  a.texturePoint.y = rand() % sourceTexture.h;
  b.texturePoint.x = rand() % sourceTexture.w;
  b.texturePoint.y = rand() % sourceTexture.h;
  c.texturePoint.x = rand() % sourceTexture.w;
  c.texturePoint.y = rand() % sourceTexture.h;
  t = CanvasTriangle(a, b, c);
  // drawTriangle(t, colour);
  textureTriangle(t, "logoTexture.ppm");
}

CanvasTriangle sortVertices(CanvasTriangle t) {
  if (t.vertices[1].y < t.vertices[0].y) {
    std::swap(t.vertices[0].x, t.vertices[1].x);
    std::swap(t.vertices[0].y, t.vertices[1].y);
    std::swap(t.vertices[0].texturePoint.x, t.vertices[1].texturePoint.x);
    std::swap(t.vertices[0].texturePoint.y, t.vertices[1].texturePoint.y);
  }
  if (t.vertices[2].y < t.vertices[1].y) {
    std::swap(t.vertices[1].x, t.vertices[2].x);
    std::swap(t.vertices[1].y, t.vertices[2].y);
    std::swap(t.vertices[1].texturePoint.x, t.vertices[2].texturePoint.x);
    std::swap(t.vertices[1].texturePoint.y, t.vertices[2].texturePoint.y);
    if (t.vertices[1].y < t.vertices[0].y) {
      std::swap(t.vertices[0].x, t.vertices[1].x);
      std::swap(t.vertices[0].y, t.vertices[1].y);
      std::swap(t.vertices[0].texturePoint.x, t.vertices[1].texturePoint.x);
      std::swap(t.vertices[0].texturePoint.y, t.vertices[1].texturePoint.y);
    }
  }
  return t;
}

// void fillTriangle(CanvasTriangle t, uint32_t colour) {
//   CanvasTriangle sortedT = sortVertices(t);

//   CanvasPoint minPoint = sortedT.vertices[0];
//   CanvasPoint midPoint = sortedT.vertices[1];
//   CanvasPoint maxPoint = sortedT.vertices[2];

//   float minMaxSlope = (maxPoint.x - minPoint.x) / (maxPoint.y - minPoint.y);
//   float minMidSlope = (midPoint.x - minPoint.x) / (midPoint.y - minPoint.y);
//   float midMaxSlope = (maxPoint.x - midPoint.x) / (maxPoint.y - midPoint.y);

//   float xExtraPoint = minPoint.x + minMaxSlope*(midPoint.y - minPoint.y);

//   CanvasPoint extraPoint = {xExtraPoint, midPoint.y};

//   for (float y=minPoint.y; y < midPoint.y; ++y) {
//     float x1 = minPoint.x +  minMaxSlope*(y - minPoint.y);
//     float x2 = minPoint.x +  minMidSlope*(y - minPoint.y);
//     drawLine({x1, y}, {x2, y}, colour);
//   }

//   for (float y=midPoint.y; y < maxPoint.y; ++y) {
//     float x1 = extraPoint.x +  minMaxSlope*(y - midPoint.y);
//     float x2 = midPoint.x +  midMaxSlope*(y - midPoint.y);
//     drawLine({x1, y}, {x2, y}, colour);
//   }
// }

void fillTriangle(CanvasTriangle t, uint32_t colour) {
  CanvasTriangle sortedT = sortVertices(t);

  CanvasPoint minPoint = sortedT.vertices[0];
  CanvasPoint midPoint = sortedT.vertices[1];
  CanvasPoint maxPoint = sortedT.vertices[2];

  float lambda = (midPoint.y - minPoint.y) / (maxPoint.y - minPoint.y); //lambda = barycentric weight

  float xExtraPoint = (1 - lambda)*minPoint.x + lambda*maxPoint.x;

  CanvasPoint extraPoint = {xExtraPoint, midPoint.y};

  for (float y=minPoint.y; y < midPoint.y; ++y) {
    float lambda1 = (y - minPoint.y) / (midPoint.y - minPoint.y);
    float x1 = (1-lambda1)*minPoint.x + lambda1*midPoint.x;
    float x2 = (1-lambda1)*minPoint.x + lambda1*extraPoint.x;
    drawLine({x1, y}, {x2, y}, colour);
  }

  for (float y=midPoint.y; y < maxPoint.y; ++y) {
    float lambda2 = (y - midPoint.y) / (maxPoint.y - midPoint.y);
    float x1 = (1-lambda2)*midPoint.x + lambda2*maxPoint.x;
    float x2 = (1-lambda2)*extraPoint.x + lambda2*maxPoint.x;
    drawLine({x1, y}, {x2, y}, colour);
  }
}

Image readPPM(std::string filename)
{
    std::ifstream ifs;
    std::string line;
    std::string ppmCommand;
    ifs.open(filename, std::ios::binary);
    Image ppm;
    try {
        if (ifs.fail()) {
            throw("Can't open input file");
        }
        std::string header;
        int w, h, b;
        ifs >> header;
        if (strcmp(header.c_str(), "P6") != 0) {
           throw("Can't read input file");
        }
        ifs >> ppmCommand;
        ifs.ignore(256, '\n');
        ifs >> w >> h >> b;
        ppm.w = w;
        ppm.h = h;
        ppm.pixels = new Image::Rgb[w * h];
        ifs.ignore(256, '\n');
        unsigned char pix[3];
        for (int i = 0; i < w * h; ++i) {
            ifs.read(reinterpret_cast<char *>(pix), 3);
            ppm.pixels[i].r = pix[0] / 255.f;
            ppm.pixels[i].g = pix[1] / 255.f;
            ppm.pixels[i].b = pix[2] / 255.f;
        }
        ifs.close();
    }
    catch (const char *err) {
        fprintf(stderr, "%s\n", err);
        ifs.close();
    }

    return ppm;
}

void textureTriangle(CanvasTriangle t, std::string texture) {
  
  Image sourceTexture = readPPM(texture);

  // for(int x=0; x < (int)sourceTexture.w; x++) {
  //   for(int y=0; y < (int)sourceTexture.h; y++) {
  //     int textureArrayIndex = (y*sourceTexture.w) + x;
  //     int textureRed = sourceTexture.pixels[textureArrayIndex].r*255;
  //     int textureGreen = sourceTexture.pixels[textureArrayIndex].g*255;
  //     int textureBlue = sourceTexture.pixels[textureArrayIndex].b*255;
  //     int colour = packRGB(textureRed, textureGreen, textureBlue);
  //     window.setPixelColour(x, y, colour);
  //   }
  // }
  
  CanvasTriangle sortedT = sortVertices(t);

  //sort image vertices and texture points

  CanvasPoint minPoint = sortedT.vertices[0];
  CanvasPoint midPoint = sortedT.vertices[1];
  CanvasPoint maxPoint = sortedT.vertices[2];

  float lambda = (midPoint.y - minPoint.y) / (maxPoint.y - minPoint.y); //lambda = barycentric weight

  float xExtraPoint = (1 - lambda)*minPoint.x + lambda*maxPoint.x;

  float xExtraPointTexture = (1 - lambda)*minPoint.texturePoint.x + lambda*maxPoint.texturePoint.x;
  float yExtraPointTexture = (1 - lambda)*minPoint.texturePoint.y + lambda*maxPoint.texturePoint.y;

  CanvasPoint extraPoint = {xExtraPoint, midPoint.y};

  extraPoint.texturePoint = TexturePoint(xExtraPointTexture, yExtraPointTexture);

  for (float y=minPoint.y; y < midPoint.y; ++y) {
    CanvasPoint rakeStart, rakeEnd;
    float lambda1 = (y - minPoint.y) / (midPoint.y - minPoint.y);
    rakeStart.x = (1-lambda1)*minPoint.x + lambda1*midPoint.x;
    rakeEnd.x = (1-lambda1)*minPoint.x + lambda1*extraPoint.x;
    rakeStart.y = y;
    rakeEnd.y = y;
    rakeStart.texturePoint.x = (1-lambda1)*minPoint.texturePoint.x + lambda1*midPoint.texturePoint.x;
    rakeStart.texturePoint.y = (1-lambda1)*minPoint.texturePoint.y + lambda1*midPoint.texturePoint.y;  
    rakeEnd.texturePoint.x = (1-lambda1)*minPoint.texturePoint.x + lambda1*extraPoint.texturePoint.x;
    rakeEnd.texturePoint.y = (1-lambda1)*minPoint.texturePoint.y + lambda1*extraPoint.texturePoint.y;

    drawTextureLine(rakeStart, rakeEnd, sourceTexture);    
  }

  for (float y=midPoint.y; y < maxPoint.y; ++y) {
    CanvasPoint rakeStart, rakeEnd;
    float lambda2 = (y - midPoint.y) / (maxPoint.y - midPoint.y);
    rakeStart.x = (1-lambda2)*midPoint.x + lambda2*maxPoint.x;
    rakeEnd.x = (1-lambda2)*extraPoint.x + lambda2*maxPoint.x;
    rakeStart.y = y;
    rakeEnd.y = y;
    rakeStart.texturePoint.x = (1-lambda2)*midPoint.texturePoint.x + lambda2*maxPoint.texturePoint.x;
    rakeStart.texturePoint.y = (1-lambda2)*midPoint.texturePoint.y + lambda2*maxPoint.texturePoint.y;  
    rakeEnd.texturePoint.x = (1-lambda2)*extraPoint.texturePoint.x + lambda2*maxPoint.texturePoint.x;
    rakeEnd.texturePoint.y = (1-lambda2)*extraPoint.texturePoint.y + lambda2*maxPoint.texturePoint.y;

    drawTextureLine(rakeStart, rakeEnd, sourceTexture);
  }
}
