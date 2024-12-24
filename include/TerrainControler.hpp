#pragma once

#include <Headers.hpp>
#include "variables.h"

#define RANGE 4
#define CHUNK_SIZE 32

struct BlockStructure{
    int infoBlock[4];
};

struct Structure{
    std::vector<BlockStructure> blocks;
};

class MapGenerator;
class Chunk;
class Voxel;

struct LocalisationBlock {
    int indiceVoxel, indiceChunk, numLongueur, numProfondeur, numHauteur, idInChunk;
};

class Entity;

class TerrainControler{
    private :
        // Ces 3 tailles sont en nombre de chunk
        int planeWidth; // De 1 à 32
        int planeLength; // De 1 à 32
        int planeHeight; // Pour simplifier on bloque cette valeur à 1
        std::vector<Chunk*> listeChunks;
        int typeChunk; // Chunk plein (0), Chunk sinus (1), Chunk plat (2), Chunk procédural (3)
        int seedTerrain; 
        int octave;
        MapGenerator *mg;
        float accumulateurDestructionBlock;
        bool mouseLeftClickHold;
        int previousIdInChunk; 

        int nbChunkTerrain; // Nombre de chunk en hauteur considéré comme appartenant à la surface (définit l'amplitude de hauteur du terrain)

        bool computeTargetedBlock(glm::vec3 target, int& numLongueur, int& numHauteur, int& numProfondeur, int& indiceV, int& indiceChunk);
        CelluleBiome racineBiomeChart;
        CelluleBiome getCellBiomeFromBlock(CelluleBiome currentCell, float precipitation, float humidite);
        bool biomeChart;

        static std::vector<std::vector<Structure>> structures;
        void constructStructure(int i, int j, int k);
        bool generateStructure;
        
    public :
        TerrainControler(int planeWidth, int planeLength, int planeHeight, int typeChunk, int seedTerrain, int octave, std::vector<std::vector<std::string>> nomStructure);
        TerrainControler(); // Ce deuxième constructeur ne sera appelé que pour créer le terrain utilisé par le mode éditeur
        ~TerrainControler();
        std::vector<Chunk*> getListeChunks();
        void buildPlanChunks(unsigned char* dataPixels, unsigned char* dataPixelsCaveAC, int widthHeightmap, int heightHeightmap);
        void buildStructures(unsigned char* dataPixels);
        void buildEditorChunk();
        int getPlaneWidth();
        int* getRefToPlaneWidth();
        int getPlaneLength();
        int* getRefToPlaneLength();
        int getPlaneHeight();
        int* getRefToPlaneHeight();
        int* getRefToSeedTerrain();
        int* getRefToOctave();
        int* getRefToNbChunkTerrain();
        bool* getRefToGenerateStructure();
        LocalisationBlock tryBreakBlock(glm::vec3 camera_target, glm::vec3 camera_position);
        void breakBlock(LocalisationBlock lb);
        bool tryCreateBlock(glm::vec3 camera_target, glm::vec3 camera_position, int typeBlock);
        void drawTerrain();
        //void saveStructure(std::string filePath);
        bool checkHoldLeftClick(glm::vec3 camera_position, glm::vec3 camera_target, float deltaTime, bool modeJeu, GLuint programID);
        void setMouseLeftClickHold(bool mouseLeftClickHold);
        bool getMouseLeftClickHold();
        int getPreviousIdInChunk();
        void setPreviousIdInChunk(int previousIdInChunk);
        void setAccumulation(float accumulation);
        MapGenerator* getMapGenerator();
        void loadTerrain();
        Chunk* getChunkAt(int pos_i, int pos_k, int pos_j);
        int getBiomeID(float precipitation, float humidite);
        void setBiomeChart(CelluleBiome racineBiomeChart);
        bool hasBiomeChart();
        void setUseBiomeChart(bool biomeChart);

        static Structure readStructureFile(std::ifstream &file);
};
