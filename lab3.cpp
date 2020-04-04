#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480
#define CAMERAX 0
#define CAMERAY 2.5
#define CAMERAZ 10

#define ERROR_MTL 1
#define ERROR_OBJ_NAME 2
#define ERROR_OBJ_COLOUR 3

void draw();
void handleEvent(SDL_Event event, vec3 *cameraPosition, mat3 *cameraOrientation, double translationValue, double rotationDegree,
 double orbitAngle, double panOrbitAngle, double tiltOrbitAngle, bool *isRasterised, bool *wrapAround);

std::vector<float> interpolate(float from, float to, int numOfValues);

void initDepthBuffer(float depthBuffer[WIDTH][HEIGHT]);
void drawLine(CanvasPoint from, CanvasPoint to, float depthBuffer[WIDTH][HEIGHT], int colour, bool wrapAround);
void drawTriangle(CanvasTriangle t, int colour, float depthBuffer[WIDTH][HEIGHT]);
vec3 translateCamera(vec3 translationVector, vec3 cameraPosition);
mat3 tiltCamera(double rotationDegree, mat3 cameraOrientation);
mat3 panCamera(double rotationDegree, mat3 cameraOrientation);
mat3 horizontalLookAt(vec3 cameraPosition);
mat3 verticalLookAt(vec3 cameraPosition);
mat3 lookAt(vec3 cameraPosition);
vec3 horizontalOrbit(vec3 cameraPosition, double orbitAngle);
vec3 verticalOrbit(vec3 cameraPosition, double orbitAngle);
vec3 orbit(vec3 cameraPosition, double panOrbitAngle, double tiltOrbitAngle);

CanvasTriangle sortVertices(CanvasTriangle t);
void fillTriangle(CanvasTriangle t, uint32_t colour, float depthBuffer[WIDTH][HEIGHT]);
struct objContent
{
  std::string objName;
  Colour colour;
  vector<ModelTriangle> faces;
};

vector<Colour> readMTL(std::string filename);
vector<objContent> readObj(std::string filename, vector<Colour> &materials, float scalingFactor);
Colour getColour(std::string colourName, vector<Colour> colours);

CanvasTriangle modelToCanvas(ModelTriangle modelT, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight);
void drawWireframe(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight, bool wrapAround);
void drawRasterised(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight, bool wrapAround);
void update(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight, bool isRasterised, bool wrapAround);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

inline uint32_t packRGB(int r, int g, int b){
  return (255<<24) + (int(r)<<16) + (int(g)<<8) + int(b);
}

int main(int argc, char* argv[])
{
  SDL_Event event;

  float scalingFactor = 1;
  int focalLength = -50;
  float canvasWidth = 40;
  float canvasHeight = 30;
  vec3 cameraPosition = vec3(CAMERAX, CAMERAY, CAMERAZ);
  mat3 cameraOrientation = mat3(1.0f);
  vector<Colour> materials;
  double translationValue = 0.1;
  double rotationDegree = 1;
  double orbitAngle = 3;
  double panOrbitAngle = 3;
  double tiltOrbitAngle = 3;
  bool isRasterised = true;
  bool wrapAround = false;
  
  vector <objContent> cornellBox = readObj("cornell-box.obj", materials, scalingFactor);
  drawRasterised(cornellBox, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, wrapAround);

  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event, &cameraPosition, &cameraOrientation, translationValue, rotationDegree,
     orbitAngle, panOrbitAngle, tiltOrbitAngle, &isRasterised, &wrapAround);
    // std::cout << cameraPosition.x << endl;
    update(cornellBox, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, isRasterised, wrapAround);

    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
}

void initDepthBuffer(float depthBuffer[WIDTH][HEIGHT]) {
  for (int i=0; i < WIDTH; i++) {
    for (int j=0; j < HEIGHT; j++) {
      depthBuffer[i][j] = std::numeric_limits<float>::infinity();
    }
  }
}

vector<Colour> readMTL(std::string filename) {
  vector<Colour> material;
  Colour colour;
  std::ifstream in(filename, std::ios::in);
  std::string line;

  while (in.eof() == false) {
    std::string mtlCommand;
    std::string colourName;
    float r, g ,b;

    std::getline(in, line);
    std::istringstream inLine(line);
    inLine >> mtlCommand;
    if(mtlCommand == "newmtl"){
      inLine >> colourName;
    }
    else {
      std::cerr << "error reading newmtl" << endl;
    }
    
    std::getline(in, line);
    std::istringstream nextLine(line);
    nextLine >> mtlCommand;
    if(mtlCommand == "Kd") {
      nextLine >> r >> g >> b;
    }
    else {
      std::cerr << "error reading rgbvalues" << endl;
    }
    
    Colour colour(colourName, round(r*255), round(g*255), round(b*255));
    material.push_back(colour);  
    std::getline(in, line);   
  }

  return material;
}

Colour getColour(std::string colourName, vector<Colour> colours) {
  Colour colour;
  for(int i=0; i < (int)colours.size(); i++) {
    if(colourName == colours[i].name){
      colour = colours[i];
    }
  }
  return colour;
}

vector<objContent> readObj(std::string filename, vector<Colour> &materials, float scalingFactor) {
  vector<vec3> vertices;
  vector<objContent> objectVector;

  std::ifstream in(filename, std::ios::in);
  std::string line;

  std::string objCommand;
  std::string mtlFileName;

  std::getline(in, line);
  std::istringstream mtlFile(line);
  mtlFile >> objCommand;
  if(objCommand == "mtllib") {
    mtlFile >> mtlFileName;
    materials = readMTL(mtlFileName);
  }
  else {
    std::cerr << "error reading mtl file" << endl;
    exit(ERROR_MTL);
  }
  std::getline(in, line); 

  while(in.eof() == false) {
    objContent object;
    std::string colourName;

    std::getline(in, line);
    std::istringstream objNameLine(line);
    objNameLine >> objCommand;
    if(objCommand == "o"){
      objNameLine >> object.objName;
    }
    else{
      std::cerr << "error reading object name" << endl;
      exit(ERROR_OBJ_NAME);
    }

    std::getline(in, line);
    std::istringstream objColourLine(line);
    objColourLine >> objCommand;
    if(objCommand == "usemtl") {
      objColourLine >> colourName;
      object.colour = getColour(colourName, materials);
    }
    else {
      std::cerr << "error reading object colour" << endl;
      exit(ERROR_OBJ_COLOUR);
    }

    std::getline(in, line);
    std::istringstream objVertexFaceLine(line);
    objVertexFaceLine >> objCommand;
    while(objCommand == "v") {
      float x, y, z;
      vec3 vertex;

      objVertexFaceLine >> x >> y >> z;
      vertex.x = x*scalingFactor;
      vertex.y = y*scalingFactor;
      vertex.z = z*scalingFactor;
      vertices.push_back(vertex);

      std::getline(in, line);
      objVertexFaceLine.clear();
      objVertexFaceLine.str(line);
      objVertexFaceLine >> objCommand;
    }
    while(objCommand == "f" && !line.empty()) {
      std::string v0, v1, v2;
      int i0, i1, i2;
      std::string vertexIndex;
      std::string::size_type slashPos;

      objVertexFaceLine >> v0 >> v1 >> v2;

      i0 = std::stoi(v0, &slashPos);

      i1 = std::stoi(v1, &slashPos);

      i2 = std::stoi(v2, &slashPos);

      ModelTriangle face(vertices[i0-1], vertices[i1-1], vertices[i2-1], object.colour);
      object.faces.push_back(face);

      std::getline(in, line);
      objVertexFaceLine.clear();
      objVertexFaceLine.str(line);
      objVertexFaceLine >> objCommand;
    }
    objectVector.push_back(object);
  }
  return objectVector;
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

void update(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 bool isRasterised, bool wrapAround)
{
  // Function for performing animation (shifting artifacts or moving the camera)
  if(isRasterised) {
    drawRasterised(o, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, wrapAround);
  }
  else {
    drawWireframe(o, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, wrapAround);
  }
}

void handleEvent(SDL_Event event, vec3 *cameraPosition, mat3 *cameraOrientation, double translationValue, double rotationDegree,
 double orbitAngle, double panOrbitAngle, double tiltOrbitAngle, bool *isRasterised, bool *wrapAround)
{
  if(event.type == SDL_KEYDOWN) {
    window.clearPixels();
    if(event.key.keysym.sym == SDLK_LEFT) {
      // cout << "LEFT" << endl;
      *cameraPosition = translateCamera(vec3(-translationValue, 0, 0), *cameraPosition);
    } 
    else if(event.key.keysym.sym == SDLK_RIGHT) {
      // cout << "RIGHT" << endl;
      *cameraPosition = translateCamera(vec3(translationValue, 0, 0), *cameraPosition);
    } 
    else if(event.key.keysym.sym == SDLK_UP) {
      // cout << "UP" << endl;
      *cameraPosition = translateCamera(vec3(0, translationValue, 0), *cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_DOWN) {
      // cout << "DOWN" << endl;
      *cameraPosition = translateCamera(vec3(0, -translationValue, 0), *cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_f) {
      // cout << "FORWARD" << endl;
      *cameraPosition = translateCamera(vec3(0, 0, -translationValue), *cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_b) {
      // cout << "BACK" << endl;
      *cameraPosition = translateCamera(vec3(0, 0, translationValue), *cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_w) {
      // cout << "BACK" << endl;
      *cameraOrientation = tiltCamera(rotationDegree, *cameraOrientation);
    }
    else if(event.key.keysym.sym == SDLK_s) {
      // cout << "BACK" << endl;
      *cameraOrientation = tiltCamera(-rotationDegree, *cameraOrientation);
    }
    else if(event.key.keysym.sym == SDLK_a) {
      // cout << "pan left" << endl;
      *cameraOrientation = panCamera(rotationDegree, *cameraOrientation);
    }
    else if(event.key.keysym.sym == SDLK_d) {
      // cout << "pan right" << endl;
      *cameraOrientation = panCamera(-rotationDegree, *cameraOrientation);
    }
    else if(event.key.keysym.sym == SDLK_r) {
      *cameraPosition = vec3(CAMERAX, CAMERAY, CAMERAZ);
      *cameraOrientation = mat3(1.0f);
    }
    else if(event.key.keysym.sym == SDLK_q) {
      *isRasterised = !*isRasterised;
    }
    else if(event.key.keysym.sym == SDLK_p) {
      *wrapAround = !*wrapAround;
    }
    else if(event.key.keysym.sym == SDLK_t) {
      std::cout << cameraPosition->x << " ";
      std::cout << cameraPosition->y << " ";
      std::cout << cameraPosition->z << " " << endl;
      *cameraOrientation = horizontalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_y) {
      *cameraPosition = horizontalOrbit(*cameraPosition, -orbitAngle);
      *cameraOrientation = horizontalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_u) {
      *cameraPosition = horizontalOrbit(*cameraPosition, orbitAngle);
      *cameraOrientation = horizontalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_i) {
      *cameraPosition = verticalOrbit(*cameraPosition, -orbitAngle);
      *cameraOrientation = verticalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_k) {
      *cameraPosition = verticalOrbit(*cameraPosition, orbitAngle);
      *cameraOrientation = verticalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_z) {
      *cameraPosition = orbit(*cameraPosition, -panOrbitAngle, 0);
      *cameraOrientation = lookAt(*cameraPosition);
      // *cameraOrientation = horizontalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_x) {
      *cameraPosition = orbit(*cameraPosition, panOrbitAngle, 0);
      *cameraOrientation = lookAt(*cameraPosition);
      // *cameraOrientation = horizontalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_c) {
      *cameraPosition = orbit(*cameraPosition, 0, -tiltOrbitAngle);
      *cameraOrientation = lookAt(*cameraPosition);
      // *cameraOrientation = verticalLookAt(*cameraPosition);      
    }
    else if(event.key.keysym.sym == SDLK_v) {
      *cameraPosition = orbit(*cameraPosition, 0, tiltOrbitAngle);     
      *cameraOrientation = lookAt(*cameraPosition);
      // *cameraOrientation = verticalLookAt(*cameraPosition);
    }
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}

void drawLine(CanvasPoint from, CanvasPoint to, float depthBuffer[WIDTH][HEIGHT], int colour, bool wrapAround) {
  float xDiff = to.x - from.x;
  float yDiff = to.y - from.y;
  float depthDiff = to.depth - from.depth;
  float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
  float xStepSize = xDiff/numberOfSteps;
  float yStepSize = yDiff/numberOfSteps;
  float depthStepSize = depthDiff/numberOfSteps;

  for (float i=0.0; i<numberOfSteps; i++) {
    float x = from.x + (xStepSize*i);
    float y = from.y + (yStepSize*i);
    float depth = from.depth + (depthStepSize*i);
    if(wrapAround) {
      int modX = (int)x % WIDTH;
      int modY = (int)y % HEIGHT;
      modX = modX < 0 ? modX + WIDTH : modX;
      modY = modY < 0 ? modY + HEIGHT : modY;
      if (depth < depthBuffer[modX][modY]){
        depthBuffer[modX ][modY] = depth;
        window.setPixelColour(modX, modY, colour);
      }
    }
    else {
      if((x>=0) && (x<WIDTH) && (y>=0) && (y<HEIGHT)){
        if (depth < depthBuffer[(int)x][(int)y]){
          depthBuffer[(int)x ][(int)y] = depth;
          window.setPixelColour((int)x, (int)y, colour);
        }
      }
    }
  }
}

void drawTriangle(CanvasTriangle t, int colour, float depthBuffer[WIDTH][HEIGHT], bool wrapAround) {
  CanvasPoint t0 = t.vertices[0];
  CanvasPoint t1 = t.vertices[1];
  CanvasPoint t2 = t.vertices[2];

  drawLine(t0, t1, depthBuffer, colour, wrapAround);
  drawLine(t0, t2, depthBuffer, colour, wrapAround);
  drawLine(t1, t2, depthBuffer, colour, wrapAround);
}

CanvasTriangle sortVertices(CanvasTriangle t) {
  if (t.vertices[1].y < t.vertices[0].y) {
    std::swap(t.vertices[0].x, t.vertices[1].x);
    std::swap(t.vertices[0].y, t.vertices[1].y);
    std::swap(t.vertices[0].depth, t.vertices[1].depth);
  }
  if (t.vertices[2].y < t.vertices[1].y) {
    std::swap(t.vertices[1].x, t.vertices[2].x);
    std::swap(t.vertices[1].y, t.vertices[2].y);
    std::swap(t.vertices[1].depth, t.vertices[2].depth);

    if (t.vertices[1].y < t.vertices[0].y) {
      std::swap(t.vertices[0].x, t.vertices[1].x);
      std::swap(t.vertices[0].y, t.vertices[1].y);
      std::swap(t.vertices[0].depth, t.vertices[1].depth);
    }
  }
  return t;
}

void fillTriangle(CanvasTriangle t, uint32_t colour, float depthBuffer[WIDTH][HEIGHT], bool wrapAround) {
  CanvasTriangle sortedT = sortVertices(t);

  CanvasPoint minPoint = sortedT.vertices[0];
  CanvasPoint midPoint = sortedT.vertices[1];
  CanvasPoint maxPoint = sortedT.vertices[2];

  float minMaxSlope = (maxPoint.x - minPoint.x) / (maxPoint.y - minPoint.y);
  float minMidSlope = (midPoint.x - minPoint.x) / (midPoint.y - minPoint.y);
  float midMaxSlope = (maxPoint.x - midPoint.x) / (maxPoint.y - midPoint.y);

  float minMaxDepthSlope = (maxPoint.depth - minPoint.depth) / (maxPoint.y - minPoint.y);
  float minMidDepthSlope = (midPoint.depth - minPoint.depth) / (midPoint.y - minPoint.y);
  float midMaxDepthSlope = (maxPoint.depth - midPoint.depth) / (maxPoint.y - midPoint.y);

  float xExtraPoint = minPoint.x + minMaxSlope*(midPoint.y - minPoint.y);
  float depthExtraPoint = minPoint.depth + minMaxDepthSlope*(midPoint.y - minPoint.y);

  CanvasPoint extraPoint = {xExtraPoint, midPoint.y, depthExtraPoint};

  for (float y=minPoint.y; y < midPoint.y; ++y) {
    float x1 = minPoint.x +  minMaxSlope*(y - minPoint.y);
    float x2 = minPoint.x +  minMidSlope*(y - minPoint.y);
    float d1 = minPoint.depth + minMaxDepthSlope*(y - minPoint.y);
    float d2 = minPoint.depth + minMidDepthSlope*(y - minPoint.y);
    drawLine({x1, y, d1}, {x2, y, d2}, depthBuffer, colour, wrapAround);
  }

  for (float y=midPoint.y; y < maxPoint.y; ++y) {
    float x1 = extraPoint.x +  minMaxSlope*(y - midPoint.y);
    float x2 = midPoint.x +  midMaxSlope*(y - midPoint.y);
    float d1 = extraPoint.depth + minMaxDepthSlope*(y - midPoint.y);
    float d2 = midPoint.depth + midMaxDepthSlope*(y - midPoint.y);
    drawLine({x1, y, d1}, {x2, y, d2}, depthBuffer, colour, wrapAround);
  }
}

CanvasTriangle modelToCanvas(ModelTriangle modelT, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight) {
  float normalX, normalY;
  float rasterX, rasterY;
  float depth;
  vector<CanvasPoint> canvasPoints;
  vec3 newCameraPosition;

  for(int i=0; i < 3; i++) {
    newCameraPosition = cameraOrientation*(modelT.vertices[i] - cameraPosition);

    normalX = (focalLength*(newCameraPosition.x / newCameraPosition.z) + (canvasWidth / 2)) / canvasWidth;
    normalY = (focalLength*(newCameraPosition.y / newCameraPosition.z) + (canvasHeight / 2)) / canvasHeight;

    rasterX = floor(normalX * window.width);
    rasterY = floor((1 - normalY) * window.height);
    depth = 1 / newCameraPosition.z;
    CanvasPoint p(rasterX, rasterY, depth);
    canvasPoints.push_back(p);
  }
  
  CanvasTriangle canvasT(canvasPoints[0], canvasPoints[1], canvasPoints[2], modelT.colour);
  return canvasT;
}

void drawWireframe(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight, bool wrapAround) {
  float depthBuffer[WIDTH][HEIGHT];

  initDepthBuffer(depthBuffer);

  for(int i=0; i < (int)o.size(); i++) {
    for(int j=0; j < (int)o[i].faces.size(); j++) {
      CanvasTriangle t = modelToCanvas(o[i].faces[j], focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight);
      int colour = packRGB(o[i].colour.red, o[i].colour.green, o[i].colour.blue);
      drawTriangle(t, colour, depthBuffer, wrapAround);
    }
  }
}

void drawRasterised(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight, bool wrapAround) {
  float depthBuffer[WIDTH][HEIGHT];

  initDepthBuffer(depthBuffer);

  for(int i=0; i < (int)o.size(); i++) {
    for(int j=0; j < (int)o[i].faces.size(); j++) {
      CanvasTriangle t = modelToCanvas(o[i].faces[j], focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight);
      int colour = packRGB(o[i].colour.red, o[i].colour.green, o[i].colour.blue);
      fillTriangle(t, colour, depthBuffer, wrapAround);
    }
  }
}

vec3 translateCamera(vec3 translationVector, vec3 cameraPosition) {
  vec3 newCameraPosition = cameraPosition + translationVector;

  return newCameraPosition;
}

mat3 tiltCamera(double rotationDegree, mat3 cameraOrientation) {
  mat3 newCameraOrientation;
  mat3 rotationMatrix = mat3(1.0f);
  
  rotationMatrix[0] = vec3(1, 0, 0);
  rotationMatrix[1] = vec3(0, cos(rotationDegree*(M_PI/180)), -sin(rotationDegree*(M_PI/180)));
  rotationMatrix[2] = vec3(0, sin(rotationDegree*(M_PI/180)), cos(rotationDegree*(M_PI/180)));

  newCameraOrientation = rotationMatrix * cameraOrientation;
  return newCameraOrientation;
}

mat3 panCamera(double rotationDegree, mat3 cameraOrientation) {
  mat3 newCameraOrientation;
  mat3 rotationMatrix = mat3(1.0f);
  
  rotationMatrix[0] = vec3(cos(rotationDegree*(M_PI/180)), 0, sin(rotationDegree*(M_PI/180)));
  rotationMatrix[1] = vec3(0, 1, 0);
  rotationMatrix[2] = vec3(-sin(rotationDegree*(M_PI/180)), 0, cos(rotationDegree*(M_PI/180)));  

  newCameraOrientation = rotationMatrix * cameraOrientation;
  return newCameraOrientation;
}

mat3 horizontalLookAt(vec3 cameraPosition) {
  mat3 identity = mat3(1.0f);
  mat3 newCameraOrientation;

  double panRotationDegree = atan((cameraPosition.x - CAMERAX) / cameraPosition.z)*(180/M_PI);
  panRotationDegree = cameraPosition.z >= 0 ? panRotationDegree : panRotationDegree + 180;
  newCameraOrientation = panCamera(panRotationDegree, identity);

  double tiltRotationDegree = atan(cameraPosition.z / (cameraPosition.y - CAMERAY))*(180/M_PI);
  tiltRotationDegree = tiltRotationDegree >= 0 ? tiltRotationDegree - 90 : tiltRotationDegree + 90;
  newCameraOrientation = tiltCamera(tiltRotationDegree, newCameraOrientation);

  return newCameraOrientation;
}

mat3 verticalLookAt(vec3 cameraPosition) {
  mat3 identity = mat3(1.0f);
  mat3 newCameraOrientation;

  double tiltRotationDegree = atan(cameraPosition.z / (cameraPosition.y - CAMERAY))*(180/M_PI);
  tiltRotationDegree = cameraPosition.y >= CAMERAY ? tiltRotationDegree - 90: tiltRotationDegree + 90;
  newCameraOrientation = tiltCamera(tiltRotationDegree, identity);

  double panRotationDegree = atan((cameraPosition.x - CAMERAX) / cameraPosition.z)*(180/M_PI);
  panRotationDegree = panRotationDegree >= 0 ? panRotationDegree : panRotationDegree + 180;
  newCameraOrientation = panCamera(panRotationDegree, newCameraOrientation);

  return newCameraOrientation;
}

mat3 lookAt(vec3 cameraPosition) {
  mat3 identity = mat3(1.0f);
  mat3 newCameraOrientation;

  double panRotationDegree = atan((cameraPosition.x - CAMERAX) / cameraPosition.z)*(180/M_PI);
  panRotationDegree = cameraPosition.z > 0 ? panRotationDegree : panRotationDegree + 180;
  newCameraOrientation = panCamera(panRotationDegree, identity);

  double tiltRotationDegree = -atan((cameraPosition.y - CAMERAY) / sqrt(pow(cameraPosition.x - CAMERAX, 2) + pow(cameraPosition.z, 2)))*(180/M_PI);
  newCameraOrientation = tiltCamera(tiltRotationDegree, newCameraOrientation);
  
  return newCameraOrientation;
}

vec3 horizontalOrbit(vec3 cameraPosition, double orbitAngle) {
  vec3 newCameraPosition;
  
  newCameraPosition = vec3(cameraPosition.x*cos(orbitAngle*(M_PI/180)) + cameraPosition.z*sin(orbitAngle*(M_PI/180)),
                      cameraPosition.y,
                      -cameraPosition.x*sin(orbitAngle*(M_PI/180)) + cameraPosition.z*cos(orbitAngle*(M_PI/180)));

  return newCameraPosition;
}

vec3 verticalOrbit(vec3 cameraPosition, double orbitAngle) {
  vec3 newCameraPosition;

  newCameraPosition = vec3(cameraPosition.x,
                      cameraPosition.y*cos(orbitAngle*(M_PI/180)) - cameraPosition.z*sin(orbitAngle*(M_PI/180)),                    
                      cameraPosition.y*sin(orbitAngle*(M_PI/180)) + cameraPosition.z*cos(orbitAngle*(M_PI/180)));

  return newCameraPosition;
}

vec3 orbit(vec3 cameraPosition, double panOrbitAngle, double tiltOrbitAngle) {
  vec3 newCameraPosition;
  float radius, newX, newY, newZ;
  double theta, phi;

  radius = sqrt(pow(cameraPosition.x - CAMERAX, 2) + pow(cameraPosition.y - CAMERAY, 2) + pow(cameraPosition.z, 2));
  phi = atan2(cameraPosition.x - CAMERAX,  cameraPosition.z);
  theta = atan2(sqrt(pow(cameraPosition.z, 2) + pow(cameraPosition.x - CAMERAX, 2)), cameraPosition.y - CAMERAY);

  newX = radius * sin(theta + tiltOrbitAngle*(M_PI/180)) * sin(phi + panOrbitAngle*(M_PI/180)) + CAMERAX;
  newY = radius * cos(theta + tiltOrbitAngle*(M_PI/180)) + CAMERAY;
  newZ = radius * sin(theta + tiltOrbitAngle*(M_PI/180)) * cos(phi + panOrbitAngle*(M_PI/180));

  newCameraPosition = vec3(newX, newY, newZ);
  
  return newCameraPosition;
}