#pragma once

#include <Headers.hpp>

// Temporaire (tant que ces variables n'ont pas été mise dans des classes)
#include "variables.h"

struct CelluleBiome{
    bool isDivide;
    int typeBiome;
    float x_data[4];
    float y_data[4];
    float sizeCell;

    std::vector<CelluleBiome> cs;
};

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

        std::vector<float> simplex_values;
        std::vector<float> continentalness_values;
        bool use_spline;

        CelluleBiome racineBiomeChart;

        int *nbChunkTerrain;

        void drawCellInChart(CelluleBiome cb);
        CelluleBiome* getSelectedCellBiome(CelluleBiome* currentCell, ImPlotPoint pos);
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
};
