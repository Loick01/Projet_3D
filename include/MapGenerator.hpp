#pragma once

#include <Headers.hpp>

class MapGenerator{
    private:
        int widthMap; // Nombre de chunk en longueur du terrain
        int lengthMap; // Nombre de chunk en largeur du terrain
        int heightMap;
        int seed;
        int octave;
    public:
        MapGenerator(int wMap, int hMap, int seed, int octave);
        MapGenerator();
        ~MapGenerator();
        float generatePerlinNoise(float x, float y);
        void generateImage();
        void setWidthMap(int widthMap);
        void setLengthMap(int lengthMap);
        void setHeightMap(int heightMap);
        void setSeed(int seed);
        void setOctave(int octave);
};