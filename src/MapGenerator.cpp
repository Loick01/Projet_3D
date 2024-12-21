#include <MapGenerator.hpp>

float MapGenerator::useContinentalnessSpline(float x, float y){
  float p_value = this->noiseGenerator.GetNoise(x/6,y/6);
  return this->getContinentalnessByInterpolation(p_value);
}

MapGenerator::MapGenerator(int wMap, int lMap, int seed, int octave){
  this->widthMap = wMap;
  this->lengthMap = lMap;
  this->seed = seed;
  this->octave = octave;
  this->has_spline = false;
  //enum NoiseType { Value, ValueFractal, Perlin, PerlinFractal, Simplex, SimplexFractal, Cellular, WhiteNoise, Cubic, CubicFractal };
  this->noiseGenerator.SetNoiseType(FastNoise::SimplexFractal);
  this->noiseGenerator.SetFractalOctaves(octave);
  this->noiseGenerator.SetSeed(this->seed);
  this->nbChunkTerrain = 1;
}

MapGenerator::MapGenerator(){
  // Vide (il permet de régler le problème avec la supression du terrain en mode éditeur)
}

MapGenerator::~MapGenerator(){
  // std::cout << "Destructeur de MapGenerator\n";
}

void MapGenerator::generateImageSurface(){
  int widthHeightmap=this->widthMap*32;
  int lengthHeightmap=this->lengthMap*32;

  int dataSize = widthHeightmap*lengthHeightmap;
  unsigned char* dataPixels=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);

  for(int i=0;i<lengthHeightmap;i++){
    for(int j=0;j<widthHeightmap;j++){
      float value;
      if (this->has_spline){
        value = useContinentalnessSpline(i,j);
      }else{
        value = ((this->noiseGenerator.GetNoise(i,j)+1)/2)*(this->nbChunkTerrain*32 - 1);
      }
      dataPixels[i*widthHeightmap+j] = value;
    }
  }

  stbi_write_png("../Textures/terrain.png", widthHeightmap, lengthHeightmap, 1, dataPixels, widthHeightmap);
  free(dataPixels);
}

int MapGenerator::countWallNeighbor(unsigned char* dataPixels, int widthHeightmap, int lengthHeightmap, int i, int j){
  int wall_neighbor = 0;
  for (int m = -1 ; m < 2 ; m++){
    for (int n = -1 ; n < 2 ; n++){
      int look_y = i+m;
      int look_x = j+n;
      // En dehors des limites de la carte
      if (!(look_x < 0 || look_x >= widthHeightmap || look_y < 0 || look_y >= lengthHeightmap) && dataPixels[look_y*widthHeightmap+look_x]==255){ 
        ++wall_neighbor;
      }
    }
  }
  return wall_neighbor;
}

// Faire varier la probabilité d'apparition initiale d'un cellule pleine, et le nombre d'itération
// Ce serait bien d'intégrer directement le résultat dans la fenêtre ImGui
void MapGenerator::generateImageCave_AC(){
  int widthHeightmap=this->widthMap*32;
  int lengthHeightmap=this->lengthMap*32;

  int dataSize = widthHeightmap*lengthHeightmap;
  unsigned char* dataPixels=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);

  // Initialisation --> Carte aléatoire
  for(int i=0;i<lengthHeightmap;i++){
    for(int j=0;j<widthHeightmap;j++){
      unsigned char value = (rand()%100 < 45 ? 0 : 255);
      dataPixels[i*widthHeightmap+j] = value;
    }
  }

  // Evolution selon règles de l'automate cellulaire
  unsigned int nb_iteration = 5;
  unsigned char* nextStepDataPixel=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);

  for (unsigned int k = 0 ; k < nb_iteration ; k++){
    for(int i=0;i<lengthHeightmap;i++){
      for(int j=0;j<widthHeightmap;j++){
        int wall_neighbor = this->countWallNeighbor(dataPixels, widthHeightmap, lengthHeightmap, i,j);
        nextStepDataPixel[i*widthHeightmap+j] = wall_neighbor < 5 ? 0 : 255;
      }
    }
    for(int i=0;i<lengthHeightmap;i++){
      for(int j=0;j<widthHeightmap;j++){
        dataPixels[i*widthHeightmap+j] = nextStepDataPixel[i*widthHeightmap+j];
      }
    }
  }

  stbi_write_png("../Textures/cave_AC.png", widthHeightmap, lengthHeightmap, 1, dataPixels, widthHeightmap);
  free(dataPixels);
  free(nextStepDataPixel);
}

void MapGenerator::setWidthMap(int widthMap){
  this->widthMap = widthMap;
}

void MapGenerator::setLengthMap(int lengthMap){
  this->lengthMap = lengthMap;
}

void MapGenerator::setHeightMap(int heightMap){
  this->heightMap = heightMap;
}

void MapGenerator::setSeed(int seed){
  this->seed = seed;
  this->noiseGenerator.SetSeed(seed);
}

void MapGenerator::setOctave(int octave){
  this->octave = octave;
  this->noiseGenerator.SetFractalOctaves(octave);
}

void MapGenerator::setNbChunkTerrain(int nbChunkTerrain){
  this->nbChunkTerrain = nbChunkTerrain;
}

void MapGenerator::setContinentalnessSpline(std::vector<float> perlin_values, std::vector<float> continentalness_values){
  this->perlin_values = perlin_values;
  this->continentalness_values = continentalness_values;
  this->has_spline = true;
}

float MapGenerator::getContinentalnessByInterpolation(float p_value){
    float index_start, index_end;
    for (unsigned int i = 0 ; i < this->perlin_values.size()-1 ; i++){
        if (p_value >= this->perlin_values[i] && p_value <= this->perlin_values[i+1]){
            index_start = i;
            index_end = i+1;
            break;
        }
    }

    return this->continentalness_values[index_start] + (p_value - this->perlin_values[index_start])*((this->continentalness_values[index_end]-this->continentalness_values[index_start])/(this->perlin_values[index_end]-this->perlin_values[index_start]));
}

void MapGenerator::setHasSpline(bool has_spline){
  this->has_spline = has_spline;
}

FastNoise MapGenerator::getNoiseGenerator(){
  return this->noiseGenerator;
}
