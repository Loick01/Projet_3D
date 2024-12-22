#include <MapGenerator.hpp>

float MapGenerator::useContinentalnessSpline(float x, float y){
  float p_value = this->noiseGenerator.GetNoise(x/6,y/6);
  return this->getContinentalnessByInterpolation(p_value);
}

float MapGenerator::useCaveSpline(float x, float y){
  float p_value = this->noiseGenerator.GetNoise(x/4,y/4);
  return this->getCaveHeightByInterpolation(p_value);
}

MapGenerator::MapGenerator(int wMap, int lMap, int seed, int octave){
  this->widthMap = wMap;
  this->lengthMap = lMap;
  this->seed = seed;
  this->octave = octave;
  this->has_terrain_spline = false;
  this->has_cave_spline = false;
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
      if (this->has_terrain_spline){
        value = this->useContinentalnessSpline(i,j);
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
      if (!(look_x < 0 || look_x >= widthHeightmap || look_y < 0 || look_y >= lengthHeightmap) && dataPixels[look_y*widthHeightmap+look_x]>0){ 
        ++wall_neighbor;
      }
    }
  }
  return wall_neighbor;
}

// Faire varier la probabilité d'apparition initiale d'un cellule pleine, le nombre d'itération, les règles d'évolution de l'automate
// Ce serait bien d'intégrer directement le résultat dans la fenêtre ImGui
void MapGenerator::generateImageCave_AC(){
  int widthHeightmap=this->widthMap*32;
  int lengthHeightmap=this->lengthMap*32;

  int dataSize = widthHeightmap*lengthHeightmap;
  unsigned char* dataPixels=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);

  // Initialisation --> Carte aléatoire
  for(int i=0;i<lengthHeightmap;i++){
    for(int j=0;j<widthHeightmap;j++){
      unsigned char value = (rand()%100 < 50 ? 0 : 255);
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

        if (this->has_cave_spline && k == nb_iteration-1){ // A la dernière itération, on détermine la hauteur de la grotte à chaque block en fonction de la spline correspondante
          nextStepDataPixel[i*widthHeightmap+j] = wall_neighbor < 5 ? 0 : this->useCaveSpline(i,j);
        }else{
          nextStepDataPixel[i*widthHeightmap+j] = wall_neighbor < 5 ? 0 : 16;
        }
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

void MapGenerator::generateImageCave_Perlin(){
  int widthHeightmap=this->widthMap*32;
  int lengthHeightmap=this->lengthMap*32;

  int dataSize = widthHeightmap*lengthHeightmap;
  unsigned char* dataPixels=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);

  for(int i=0;i<lengthHeightmap;i++){
    for(int j=0;j<widthHeightmap;j++){
      float value = this->noiseGenerator.GetNoise(i*5,j*5);
      dataPixels[i*widthHeightmap+j] = abs(value) < 0.1 ? 255 : 0;
    }
  }

  stbi_write_png("../Textures/cave_Perlin.png", widthHeightmap, lengthHeightmap, 1, dataPixels, widthHeightmap);
  free(dataPixels);
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

void MapGenerator::setContinentalnessSpline(std::vector<float> terrain_simplex_values, std::vector<float> continentalness_values){
  this->terrain_simplex_values = terrain_simplex_values;
  this->continentalness_values = continentalness_values;
  this->has_terrain_spline = true;
}

void MapGenerator::setCaveSpline(std::vector<float> cave_simplex_values, std::vector<float> cave_height_values){
  this->cave_simplex_values = cave_simplex_values;
  this->cave_height_values = cave_height_values;
  this->has_cave_spline = true;
}

float MapGenerator::getContinentalnessByInterpolation(float p_value){
    float index_start, index_end;
    for (unsigned int i = 0 ; i < this->terrain_simplex_values.size()-1 ; i++){
        if (p_value >= this->terrain_simplex_values[i] && p_value <= this->terrain_simplex_values[i+1]){
            index_start = i;
            index_end = i+1;
            break;
        }
    }

    return this->continentalness_values[index_start] + (p_value - this->terrain_simplex_values[index_start])*((this->continentalness_values[index_end]-this->continentalness_values[index_start])/(this->terrain_simplex_values[index_end]-this->terrain_simplex_values[index_start]));
}

float MapGenerator::getCaveHeightByInterpolation(float p_value){
    float index_start, index_end;
    for (unsigned int i = 0 ; i < this->cave_simplex_values.size()-1 ; i++){
        if (p_value >= this->cave_simplex_values[i] && p_value <= this->cave_simplex_values[i+1]){
            index_start = i;
            index_end = i+1;
            break;
        }
    }

    return this->cave_height_values[index_start] + (p_value - this->cave_simplex_values[index_start])*((this->cave_height_values[index_end]-this->cave_height_values[index_start])/(this->cave_simplex_values[index_end]-this->cave_simplex_values[index_start]));
}

void MapGenerator::setHasTerrainSpline(bool has_terrain_spline){
  this->has_terrain_spline = has_terrain_spline;
}

void MapGenerator::setHasCaveSpline(bool has_cave_spline){
  this->has_cave_spline = has_cave_spline;
}

FastNoise MapGenerator::getNoiseGenerator(){
  return this->noiseGenerator;
}
