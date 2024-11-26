#pragma once

#include <Headers.hpp>

// Temporaire (tant que ces variables n'ont pas été mise dans des classes)
#include "variables.h"

class ParamsWindow{
    private:
        int style; // 0 pour Dark, 1 pour Light, sinon Classic
        bool renduFilaire;
        float *speedPlayer;
        glm::vec3* posJoueur;
        int* planeWidth;
        int* planeLength;
        int* planeHeight;
        TerrainControler *terrainControler;
        MapGenerator *mg;
        int *seedTerrain;
        int *octave;
        bool inEditor;
        Hitbox* hitboxPlayer; // Nécessaire pour désactiver les dégâts de chutes après un changement de terrain
        static char nameStructure[512]; // On met cet attribut en static pour que le texte saisi reste le même entre 2 frames
        bool clearEntity; // Va servir à régler le problème de segfault qui apparaît au moment où on change la taille du terrain

	FastNoise noiseGenerator;
        float ContinentValue;
        float perlinValue;
        std::vector<float> perlin_values;
        std::vector<float> continentalness_values;
    public:
        ParamsWindow(GLFWwindow* window, int style, TerrainControler *terrainControler, Player *player);
        ~ParamsWindow();
        void useStyle();
        void init(GLFWwindow* window);
        void modifTerrain();
        void draw();
        bool getInEditor();
        void attachNewTerrain(TerrainControler *terrainControler);
        bool getClearEntity();
        void resetClearEntity();
        void resetContinentalnessPlot();
        float getContinentalnessByInterpolation(glm::vec3 position);
};
