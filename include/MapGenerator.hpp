#pragma once

#include <Headers.hpp>

// class ParamsWindow;

class MapGenerator{
    private:
        int widthMap; // Nombre de chunk en longueur du terrain
        int lengthMap; // Nombre de chunk en largeur du terrain
        int heightMap;
        int seed;
        int octave;

        std::vector<float> terrain_simplex_values;
        std::vector<float> continentalness_values;
        bool has_terrain_spline;
        std::vector<float> cave_simplex_values;
        std::vector<float> cave_height_values;
        bool has_cave_spline;
        FastNoise noiseGenerator;
        int countWallNeighbor(unsigned char* dataPixels, int widthHeightmap, int lengthHeightmap, int i, int j);

    public:
        MapGenerator(int wMap, int hMap, int seed, int octave);
        MapGenerator();
        ~MapGenerator();
        void generateImageSurface();
        void generateImageCave_AC();
        void generateImageCave_Perlin();
        void setWidthMap(int widthMap);
        void setLengthMap(int lengthMap);
        void setHeightMap(int heightMap);
        void setSeed(int seed);
        void setOctave(int octave);
        void setHasTerrainSpline(bool has_terrain_spline);
        void setHasCaveSpline(bool has_cave_spline);
        void setCaveSpline(std::vector<float> cave_simplex_values, std::vector<float> cave_height_values);
        FastNoise getNoiseGenerator();

        int nbChunkTerrain;

        void setNbChunkTerrain(int nbChunkTerrain);


	    float useContinentalnessSpline(float x, float y);
        void setContinentalnessSpline(std::vector<float> perlin_values, std::vector<float> continentalness_values);
        float getContinentalnessByInterpolation(float p_value);
        float getCaveHeightByInterpolation(float p_value);

        float useCaveSpline(float x, float y);

};
