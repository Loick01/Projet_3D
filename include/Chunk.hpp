#pragma once

#include <Headers.hpp>

#include <fstream>

#define NB_BLOCK 37
#define GRASS_BLOCK 13
#define DIRT_BLOCK 32
#define DIAMOND_ORE 19
#define IRON_ORE 21
#define STONE_BLOCK 0
#define SNOW_BLOCK 16
#define SAND_BLOCK 7


struct BlockStructure{
    int infoBlock[4];
};

/*
struct Structure{
    std::vector<BlockStructure> blocks;
};
*/

class Voxel; // Déclaration avancée (je ne comprends pas pourquoi on est obligé de faire ça, avant ce n'était pas nécéssaire)

// Chunk de taille 32x32x32
class Chunk{
    private:
        unsigned int pos_i;
        unsigned int pos_j;
        unsigned int pos_k;
        glm::vec3 position;
        std::vector<Voxel*> listeVoxels;
        std::vector<unsigned int> indices;
        std::vector<glm::vec3> vertices;
        std::vector<int> objectIDs;
        std::vector<int> faceIDs;
        GLuint vertexbuffer;
        GLuint elementbuffer;
        GLuint shaderstoragebuffer;
        GLuint shaderstoragebuffer3;
        // static std::vector<std::vector<Structure>> structures;
        // int ID;

        void addIndices(int* compteur);
    public:
        Chunk(int i, int j, int k, FastNoise noiseGenerator, bool extreme, bool terrainChunk, glm::vec3 position, int typeChunk, unsigned char* dataPixels, int widthHeightmap, int heightHeightmap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc);
        Chunk(glm::vec3 position); // Ce deuxième constructeur est utilisé uniquement pour construire le terrain en mode éditeur
        ~Chunk();
        void buildCheeseChunk(int a, int b, int c, FastNoise noiseGenerator, bool extreme);
        void buildFullChunk();
        void buildFlatChunk();
        void buildSinusChunk();
        void buildProceduralChunk(unsigned char* dataPixels, int widthHeightmap, int heightHeightmap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc, FastNoise noiseGenerator);
        void buildEditorChunk();
        void loadChunk(TerrainControler* tc = nullptr);
        void drawChunk();
        std::vector<Voxel*> getListeVoxels();
        void setListeVoxels(std::vector<Voxel*> newListeVoxels);
        glm::vec3 getPosition();

        void sendVoxelMapToShader();

        /*
        // Pour la génération des structures
        static Structure readFile(std::ifstream &file);
        static void setListeStructures(std::vector<std::vector<Structure>> liste);
        void buildStructure(int i, int j, int k);
        */
};
