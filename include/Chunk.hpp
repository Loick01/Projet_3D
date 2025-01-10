#pragma once

#include <Headers.hpp>
#include "variables.h"

#include <fstream>

#define NB_BLOCK 37
#define GRASS_BLOCK 13
#define DIRT_BLOCK 32
#define DIAMOND_ORE 19
#define IRON_ORE 21
#define STONE_BLOCK 0
#define SNOW_BLOCK 16
#define SAND_BLOCK 7
#define GLASS_BLOCK 20
#define LEAVES_BLOCK 23
#define PUMPKIN 26

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
        std::vector<int> luminosityIDs;
        // Peut être fusionner les 3 maps ci-dessous en utilisant une struct (InfoFace) pour stocker les informations d'une face, et du coup utiliser uniquement une unordered_map<std::string, InfoFace>
        std::map<std::string, int> map_objectIDs;
        std::map<std::string, int> map_faceIDs;
        std::map<std::string, int> map_luminosityIDs;
        std::map<std::string, std::vector<glm::vec3>> map_vertices;
        GLuint vertexbuffer;
        GLuint elementbuffer;
        GLuint shaderstoragebuffer;
        GLuint shaderstoragebuffer2;
        GLuint shaderstoragebuffer3;
        // int ID;
        TerrainControler* currentTerrainControler;
        static const std::set<int> transparentBlock;

        void addIndices(int decalage);
    public:
        Chunk(int i, int j, int k, FastNoise noiseGenerator, int nbTerrainChunk, glm::vec3 position, int typeChunk, unsigned char* dataPixels, unsigned char* dataPixelsCaveAC, int widthHeightmap, int heightHeightmap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc);
        Chunk(glm::vec3 position, bool referenceChunk); // Ce deuxième constructeur est utilisé uniquement pour construire le terrain en mode éditeur
        ~Chunk();
        void buildCheeseChunk(int a, int b, int c, FastNoise noiseGenerator);
        void buildFullChunk();
        void buildCaveChunk(int hauteurChunk, unsigned char* dataPixelsCaveAC, int posWidthChunk, int posLengthChunk, int widthHeightmap);
        void buildFlatChunk();
        void buildSinusChunk();
        void buildProceduralChunk(unsigned char* dataPixels, int widthHeightmap, int heightHeightmap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc, FastNoise noiseGenerator);
        void buildEditorChunk(bool referenceChunk);
        void addFace(Voxel* v_bottom,int orientation);
        void removeFace(Voxel* v_bottom,int orientation);
        void removeFaces(std::string racine_face_id);
        void buildFace(std::string unique_id_face, bool cond1,int a1, int dec, int a2, int voxel_id, int8_t v1, int8_t v2, int8_t v3, std::vector<glm::vec3> voxel_vertices, int lum_value);
        void loadChunk(TerrainControler* tc = nullptr);
        void drawChunk();
        std::vector<Voxel*> getListeVoxels();
        void setListeVoxels(std::vector<Voxel*> newListeVoxels);
        glm::vec3 getPosition();
        void updateLum(int i_v);

        void sendVoxelMapToShader();
};
