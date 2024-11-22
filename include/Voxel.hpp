#pragma once

#include <Headers.hpp>

struct Face{
    std::vector<glm::vec3> vertices;
    // int orientation; // 0 = Bottom; 1 = Top; 2 = Back; 3 = Front; 4 = Left; 5 = Right
};

class Voxel {
    private:
        //std::vector<glm::vec3> vertices;
        std::vector<Face> faces;
        int objectID;
        //bool isVisible;
        int idInChunk;
    public:
        glm::vec3 backBottomLeftCorner;
        Voxel(glm::vec3 position, int objectID);
        ~Voxel();
        glm::vec3 getBackBottomLeftCorner();
        void buildVoxel();
        std::vector<glm::vec3> getVertices();
        //void setVisible(bool isVisible);
        //bool getVisible();
        void setId(int new_id);
        int getID();
        void setIdInChunk(int idInChunk);
        int getIdInChunk();
};