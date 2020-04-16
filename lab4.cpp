#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <RayTriangleIntersection.h>
#include <Utils.h>
#include <Image.h>
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

struct objContent
{
  std::string objName;
  Colour colour;
  vector<ModelTriangle> faces;
};

struct RayObjectIntersection
{
  RayTriangleIntersection triangle;
  objContent object;
};

void update(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, vec3 lightPos, float canvasWidth, float canvasHeight,
 bool isWireframe, bool isRasterised, bool isRayTraced, bool wrapAround);
void handleEvent(SDL_Event event, vec3 *cameraPosition, mat3 *cameraOrientation, vec3 *lightPos, double translationValue, double rotationDegree,
 double orbitAngle, double panOrbitAngle, double tiltOrbitAngle, bool *isWireframe, bool *isRasterised, bool *isRayTraced, bool *wrapAround);

void initDepthBuffer(float depthBuffer[WIDTH][HEIGHT]);
void drawLine(CanvasPoint from, CanvasPoint to, float depthBuffer[WIDTH][HEIGHT], int colour, bool wrapAround);
void drawTriangle(CanvasTriangle t, int colour, float depthBuffer[WIDTH][HEIGHT]);
CanvasTriangle sortVertices(CanvasTriangle t);
void fillTriangle(CanvasTriangle t, uint32_t colour, float depthBuffer[WIDTH][HEIGHT]);


vector<Colour> readMTL(std::string filename);
vector<objContent> readObj(std::string filename, vector<Colour> &materials, float scalingFactor);
Image readPPM(const char *filename);
Colour getColour(std::string colourName, vector<Colour> colours);

CanvasTriangle modelToCanvas(ModelTriangle modelT, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth,
 float canvasHeight);
void drawWireframe(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 bool wrapAround);
void drawRasterised(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 bool wrapAround);

RayObjectIntersection getClosestIntersection(vector<objContent> o, vec3 cameraPosition, vec3 rayDirection);
float calcAoi(RayObjectIntersection intersection, vec3 lightPos);
float addProxAoiLight(RayObjectIntersection intersection, vec3 lightPos, float aoi);
float addSpecularLight(RayObjectIntersection intersection, vec3 lightPos, vec3 cameraPosition);
float addShadow(RayObjectIntersection intersection, vec3 lightPos, vector<objContent> o, vec3 cameraPosition, float brightness);
float addLighting(RayObjectIntersection intersection, vec3 lightPos, vec3 cameraPosition);
void drawRaytraced(vector<objContent> o, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 vec3 lightPos, bool wrapAround);
vec3 translateLight(vec3 translationVector, vec3 lightPos);
vec3 translateCamera(vec3 translationVector, vec3 cameraPosition);
mat3 tiltCamera(double rotationDegree, mat3 cameraOrientation);
mat3 panCamera(double rotationDegree, mat3 cameraOrientation);
mat3 verticalLookAt(vec3 cameraPosition);
mat3 lookAt(vec3 cameraPosition);
vec3 verticalOrbit(vec3 cameraPosition, double orbitAngle);
vec3 orbit(vec3 cameraPosition, double panOrbitAngle, double tiltOrbitAngle);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);



inline uint32_t packRGB(int r, int g, int b, float brightness){
  r *= brightness;
  g *= brightness;
  b *= brightness;
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
  vec3 lightPos = vec3(-0.2, 5, -2.5);
  vector<Colour> materials;
  double translationValue = 0.1;
  double rotationDegree = 1;
  double orbitAngle = 3;
  double panOrbitAngle = 3;
  double tiltOrbitAngle = 3;
  bool isWireframe = true;
  bool isRasterised = false;
  bool isRayTraced = false;
  bool wrapAround = false;

  vector <objContent> cornellBox = readObj("cornell-box.obj", materials, scalingFactor);
  Image ppm = readPPM("texture.ppm");

  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event, &cameraPosition, &cameraOrientation, &lightPos, translationValue, rotationDegree,
     orbitAngle, panOrbitAngle, tiltOrbitAngle, &isWireframe, &isRasterised, &isRayTraced, &wrapAround);

    update(cornellBox, focalLength, cameraPosition, cameraOrientation, lightPos, canvasWidth, canvasHeight, isWireframe, isRasterised, isRayTraced,
     wrapAround);

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

Image readPPM(const char *filename)
{
    std::ifstream ifs;
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

void update(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, vec3 lightPos, float canvasWidth, float canvasHeight,
 bool isWireframe, bool isRasterised, bool isRayTraced, bool wrapAround) {
  // Function for performing animation (shifting artifacts or moving the camera)
  if(isRasterised) {
    drawRasterised(o, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, wrapAround);
  }
  if (isRayTraced) {
    drawRaytraced(o, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, lightPos, wrapAround);
  }
  else if(isWireframe) {
    drawWireframe(o, focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight, wrapAround);
  }
}

void handleEvent(SDL_Event event, vec3 *cameraPosition, mat3 *cameraOrientation, vec3 *lightPos, double translationValue, double rotationDegree,
 double orbitAngle, double panOrbitAngle, double tiltOrbitAngle, bool *isWireframe, bool *isRasterised, bool *isRayTraced, bool *wrapAround)
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
    else if(event.key.keysym.sym == SDLK_1) {
      *isWireframe = true;
      *isRasterised = false;
      *isRayTraced = false;
    }
    else if(event.key.keysym.sym == SDLK_2) {
      *isWireframe = false;
      *isRasterised = true;
      *isRayTraced = false;
    }
    else if(event.key.keysym.sym == SDLK_3) {
      *isWireframe = false;
      *isRasterised = false;
      *isRayTraced = true;
    }
    else if(event.key.keysym.sym == SDLK_4) {
      *lightPos = translateLight(vec3(-translationValue, 0, 0), *lightPos);
    }
    else if(event.key.keysym.sym == SDLK_5) {
      *lightPos = translateLight(vec3(translationValue, 0, 0), *lightPos);
    }
    else if(event.key.keysym.sym == SDLK_6) {
      *lightPos = translateLight(vec3(0, 0, -translationValue), *lightPos);
    }
    else if(event.key.keysym.sym == SDLK_7) {
      *lightPos = translateLight(vec3(0, 0, translationValue), *lightPos);
    }
    else if(event.key.keysym.sym == SDLK_p) {
      *wrapAround = !*wrapAround;
    }
    else if(event.key.keysym.sym == SDLK_t) {
      std::cout << cameraPosition->x << " ";
      std::cout << cameraPosition->y << " ";
      std::cout << cameraPosition->z << " " << endl;
      *cameraOrientation = lookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_g) {
      *cameraPosition = orbit(*cameraPosition, -panOrbitAngle, 0);
      *cameraOrientation = lookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_j) {
      *cameraPosition = orbit(*cameraPosition, panOrbitAngle, 0);
      *cameraOrientation = lookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_y) {
      *cameraPosition = orbit(*cameraPosition, 0, -tiltOrbitAngle);
      *cameraOrientation = lookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_h) {
      *cameraPosition = orbit(*cameraPosition, 0, tiltOrbitAngle);
      *cameraOrientation = lookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_i) {
      *cameraPosition = verticalOrbit(*cameraPosition, -orbitAngle);
      *cameraOrientation = verticalLookAt(*cameraPosition);
    }
    else if(event.key.keysym.sym == SDLK_k) {
      *cameraPosition = verticalOrbit(*cameraPosition, orbitAngle);
      *cameraOrientation = verticalLookAt(*cameraPosition);
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

CanvasTriangle modelToCanvas(ModelTriangle modelT, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth,
 float canvasHeight) {
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

RayObjectIntersection getClosestIntersection(vector<objContent> o, vec3 cameraPosition, vec3 rayDirection) {
    RayObjectIntersection intersection;

    float closestDistance = std::numeric_limits<float>::infinity();

    intersection.triangle.intersectedTriangle.colour = Colour(0,0,0);
    for(int i=0; i < (int)o.size(); i++) {
        for(int j=0; j < (int)o[i].faces.size(); j++) {
            vec3 e0 = o[i].faces[j].vertices[1] - o[i].faces[j].vertices[0];
            vec3 e1 = o[i].faces[j].vertices[2] - o[i].faces[j].vertices[0];
            vec3 startPointVector = cameraPosition - o[i].faces[j].vertices[0];
            mat3 differenceMatrix(-rayDirection, e0, e1);
            vec3 possibleSolution = glm::inverse(differenceMatrix) * startPointVector;
            if(0 <= possibleSolution.x && 0 <= possibleSolution.y && 0 <= possibleSolution.z
            && possibleSolution.y + possibleSolution.z <= 1 && possibleSolution.x < closestDistance) {
                intersection.triangle.intersectedTriangle = o[i].faces[j];
                intersection.triangle.intersectionPoint = o[i].faces[j].vertices[0] + possibleSolution.y*e0 + possibleSolution.z*e1;
                intersection.triangle.distanceFromCamera = possibleSolution.x;
                intersection.object = o[i];
                closestDistance = possibleSolution.x;
            }
        }
    }
    return intersection;
}

float calcAoi(RayObjectIntersection intersection, vec3 lightPos) {
  vec3 normal, pointToLight;
  float aoi;

  normal = glm::normalize(glm::cross(intersection.triangle.intersectedTriangle.vertices[1]-intersection.triangle.intersectedTriangle.vertices[0],
                      intersection.triangle.intersectedTriangle.vertices[2]-intersection.triangle.intersectedTriangle.vertices[0]));

  pointToLight = glm::normalize(lightPos - intersection.triangle.intersectionPoint);
  aoi = glm::dot(normal, pointToLight);

  return aoi;
}

float addProxAoiLight(RayObjectIntersection intersection, vec3 lightPos, float aoi) {
  float brightness, length;

  length = glm::length(lightPos - intersection.triangle.intersectionPoint);
  brightness = 400 * glm::max(aoi, 0.0f) / (4 * M_PI * pow(length, 2));

  return brightness;
}

float addSpecularLight(RayObjectIntersection intersection, vec3 lightPos, vec3 cameraPosition) {
  vec3 normal, view, lightToPoint, reflection;
  float specBrightness;

  normal = glm::normalize(glm::cross(intersection.triangle.intersectedTriangle.vertices[1]-intersection.triangle.intersectedTriangle.vertices[0],
                      intersection.triangle.intersectedTriangle.vertices[2]-intersection.triangle.intersectedTriangle.vertices[0]));

  view = glm::normalize(cameraPosition - intersection.triangle.intersectionPoint);
  lightToPoint = glm::normalize(intersection.triangle.intersectionPoint - lightPos);
  reflection = lightToPoint - ((2.0f*normal) * glm::dot(normal, lightToPoint));

  specBrightness = glm::pow(glm::dot(view, reflection), 1.0f);

  return specBrightness;
}

float addLighting(RayObjectIntersection intersection, vec3 lightPos, vec3 cameraPosition) {
  float aoi, proxAoiBrightness, specBrightness, brightness;

  aoi = calcAoi(intersection, lightPos);
  proxAoiBrightness = addProxAoiLight(intersection, lightPos, aoi);
  specBrightness = addSpecularLight(intersection, lightPos, cameraPosition);

  brightness = proxAoiBrightness * specBrightness;

  if (brightness < 0.2f) brightness = 0.2f;
  if (brightness > 1.0f) brightness = 1.0f;

  return brightness;
}

float addShadow(RayObjectIntersection intersection, vec3 lightPos, vector<objContent> o, vec3 cameraPosition, float brightness) {
  RayObjectIntersection shadowIntersection;
  vec3 shadowRay;
  float shadowRayToObjectDistance, shadowRayToLightDistance, newBrightness;

  shadowRay = lightPos - intersection.triangle.intersectionPoint;

  for(int i=0; i < (int)o.size(); i++) {
        for(int j=0; j < (int)o[i].faces.size(); j++) {
            vec3 e0 = o[i].faces[j].vertices[1] - o[i].faces[j].vertices[0];
            vec3 e1 = o[i].faces[j].vertices[2] - o[i].faces[j].vertices[0];
            vec3 startPointVector = intersection.triangle.intersectionPoint - o[i].faces[j].vertices[0];
            mat3 differenceMatrix(-shadowRay, e0, e1);
            vec3 possibleSolution = glm::inverse(differenceMatrix) * startPointVector;
            if(0 <= possibleSolution.x && 0 <= possibleSolution.y && 0 <= possibleSolution.z
            && possibleSolution.y + possibleSolution.z <= 1 ) {
                shadowIntersection.triangle.intersectionPoint = o[i].faces[j].vertices[0] + possibleSolution.y*e0 + possibleSolution.z*e1;
                shadowIntersection.triangle.intersectedTriangle = o[i].faces[j];
                shadowIntersection.object = o[i];
            }
        }
    }

  shadowRayToObjectDistance = glm::distance(intersection.triangle.intersectionPoint, shadowIntersection.triangle.intersectionPoint);
  shadowRayToLightDistance = glm::distance(intersection.triangle.intersectionPoint, lightPos);

  if (shadowRayToObjectDistance < shadowRayToLightDistance && shadowRayToLightDistance > 5
      && intersection.object.objName != shadowIntersection.object.objName) {
    newBrightness = 0.1f;
  } else {
    newBrightness = brightness;
  }

  return newBrightness;
}

void drawWireframe(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 bool wrapAround) {
  float depthBuffer[WIDTH][HEIGHT];

  initDepthBuffer(depthBuffer);

  for(int i=0; i < (int)o.size(); i++) {
    for(int j=0; j < (int)o[i].faces.size(); j++) {
      CanvasTriangle t = modelToCanvas(o[i].faces[j], focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight);
      int colour = packRGB(o[i].colour.red, o[i].colour.green, o[i].colour.blue, 1.0f);
      drawTriangle(t, colour, depthBuffer, wrapAround);
    }
  }
}

void drawRasterised(vector<objContent> o, int focalLength, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
 bool wrapAround) {

  float depthBuffer[WIDTH][HEIGHT];

  initDepthBuffer(depthBuffer);

  for(int i=0; i < (int)o.size(); i++) {
    for(int j=0; j < (int)o[i].faces.size(); j++) {
      CanvasTriangle t = modelToCanvas(o[i].faces[j], focalLength, cameraPosition, cameraOrientation, canvasWidth, canvasHeight);
      int colour = packRGB(o[i].colour.red, o[i].colour.green, o[i].colour.blue, 1.0f);
      fillTriangle(t, colour, depthBuffer, wrapAround);
    }
  }
}

void drawRaytraced(vector<objContent> o, vec3 cameraPosition, mat3 cameraOrientation, float canvasWidth, float canvasHeight,
vec3 lightPos, bool wrapAround) {
    RayObjectIntersection intersection;
    vec3 ray;
    CanvasPoint drawPoint;
    float noShadowBrightness;

    for(int x=0; x < WIDTH; x++) {
        for(int y=0; y < HEIGHT; y++) {
            drawPoint.x = x - WIDTH/2;
            drawPoint.y = -(y - HEIGHT/2);
            ray = vec3(drawPoint.x, drawPoint.y, -(WIDTH/2) / tan(M_PI/8)) * cameraOrientation;
            intersection = getClosestIntersection(o, cameraPosition, ray);
            noShadowBrightness = addLighting(intersection, lightPos, cameraPosition);
            drawPoint.brightness = addShadow(intersection, lightPos, o, cameraPosition, noShadowBrightness);
            int colour = packRGB(intersection.triangle.intersectedTriangle.colour.red, intersection.triangle.intersectedTriangle.colour.green,
            intersection.triangle.intersectedTriangle.colour.blue, drawPoint.brightness);
            if(intersection.triangle.distanceFromCamera < INFINITY) {
                window.setPixelColour(x, y, colour);
            }
        }
    }
 }

vec3 translateLight(vec3 translationVector, vec3 lightPos) {
  vec3 newLightPos = lightPos + translationVector;

  return newLightPos;
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
