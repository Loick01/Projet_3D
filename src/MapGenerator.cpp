#include <MapGenerator.hpp>

float MapGenerator::generatePerlinNoise(float x, float y){
  if (!this->has_spline){
    FastNoise noiseGenerator;
    noiseGenerator.SetSeed(this->seed);
    float somme = noiseGenerator.GetNoise(x,y);
    for (int i = 1 ; i < this->octave ; i++){
      somme += noiseGenerator.GetNoise(x*2*i,y*2*i);
    }
    return somme/this->octave;
  }else{
    FastNoise noiseGenerator;
    noiseGenerator.SetSeed(this->seed);
    float p_value = noiseGenerator.GetNoise(x,y);
    return this->getContinentalnessByInterpolation(p_value);
  }
}

MapGenerator::MapGenerator(int wMap, int hMap, int seed, int octave){
  this->widthMap = wMap;
  this->heightMap = hMap;
  this->seed = seed;
  this->octave = octave;
  this->has_spline = false;
}

MapGenerator::MapGenerator(){
  // Vide (il permet de régler le problème avec la supression du terrain en mode éditeur)
}

MapGenerator::~MapGenerator(){
  // std::cout << "Destructeur de MapGenerator\n";
}

void MapGenerator::generateImage(){
    // A voir si on ne peut pas utiliser des images sur 3 canaux, voir en niveau de gris
    int dataSize = this->widthMap*32*this->heightMap*32*4;
    unsigned char* dataPixels=(unsigned char*)malloc(sizeof(unsigned char)*dataSize);
    for (int i = 0 ; i < dataSize ; i++){
        dataPixels[i] = 0;
        if(((i+1)%4)==0){
            dataPixels[i]=255;
        }
    }

    int widthHeightmap=this->widthMap*32;
    int heightHeightmap=this->heightMap*32;
  

  for(int i=0;i<heightHeightmap;i++){
    for(int j=0;j<widthHeightmap;j++){
      float value = ((generatePerlinNoise(i/3,j/3)+1)/2) * 31; // Revoir comment on interprète le résultat obtenu ici pour déterminer la hauteur du terrain
      dataPixels[i*widthHeightmap*4+j*4] = value;
    }
  }

  stbi_write_png("../Textures/terrain.png", widthHeightmap, heightHeightmap,4,dataPixels, 4*widthHeightmap);
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
}

void MapGenerator::setOctave(int octave){
  this->octave = octave;
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
