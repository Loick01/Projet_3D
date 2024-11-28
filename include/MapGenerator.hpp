#pragma once

#include <Headers.hpp>

class ParamsWindow;

class MapGenerator{
    private:
        int widthMap; // Nombre de chunk en longueur du terrain
        int lengthMap; // Nombre de chunk en largeur du terrain
        int heightMap;
        int seed;
        int octave;

        std::vector<float> perlin_values;
        std::vector<float> continentalness_values;
        bool has_spline;
        FastNoise noiseGenerator;

    public:
        MapGenerator(int wMap, int hMap, int seed, int octave);
        MapGenerator();
        ~MapGenerator();
        void generateImage();
        void setWidthMap(int widthMap);
        void setLengthMap(int lengthMap);
        void setHeightMap(int heightMap);
        void setSeed(int seed);
        void setOctave(int octave);
        void setHasSpline(bool has_spline);

	float useContinentalnessSpline(float x, float y);
        void setContinentalnessSpline(std::vector<float> perlin_values, std::vector<float> continentalness_values);
        float getContinentalnessByInterpolation(float p_value);
};
