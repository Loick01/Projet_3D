#include <Voxel.hpp>

Voxel::Voxel(glm::vec3 position, int objectID){
    this->backBottomLeftCorner = position;
    this->buildVoxel();
    this->objectID = objectID;
}

Voxel::~Voxel(){
    //std::cout << "Destructeur de Voxel\n";
}

glm::vec3 Voxel::getBackBottomLeftCorner(){
    return this->backBottomLeftCorner;
}

// Voxel de taille 1.0
void Voxel::buildVoxel(){
    std::string racine_id = std::to_string((int)backBottomLeftCorner.x) + " " + std::to_string((int)backBottomLeftCorner.y) + " " + std::to_string((int)backBottomLeftCorner.z) + " ";
    for (int i = 0 ; i < 6 ; i++){
        Face f;
        f.unique_id = racine_id + std::to_string(i);
        //f.orientation = i;
        for (int h = 0; h < 2 ; h++) {
            for (int w = 0; w < 2; w++) {
                float x, y, z;
                int n = (i%2 == 0 ? 1 : -1);

                if (i < 2){ // Faces bottom et top
                    x = this->backBottomLeftCorner[0] + (float)w;
                    y = this->backBottomLeftCorner[1] + i;
                    z = this->backBottomLeftCorner[2] + i + (float)h * n; 
                }else if (i == 2){ // Faces back
                    x = this->backBottomLeftCorner[0] + (1-w);
                    y = this->backBottomLeftCorner[1] + (float)h; 
                    z = this->backBottomLeftCorner[2];
                }else if (i == 3){ // Faces front
                    x = this->backBottomLeftCorner[0] + (float)w;
                    y = this->backBottomLeftCorner[1] + (float)h; 
                    z = this->backBottomLeftCorner[2] + 1.0f;
                }else if (i == 4){ // Face left
                    x = this->backBottomLeftCorner[0] + (i-4);
                    y = this->backBottomLeftCorner[1] + (float)h;
                    z = this->backBottomLeftCorner[2] + (float)w; 
                }else if (i == 5){ // Face right
                    x = this->backBottomLeftCorner[0] + (i-4);
                    y = this->backBottomLeftCorner[1] + (float)h;
                    z = this->backBottomLeftCorner[2] + (1-w); 
                }
                //this->vertices.push_back(glm::vec3(x,y,z));
                f.vertices.push_back(glm::vec3(x,y,z));
            }
        }
        this->faces.push_back(f);
    }
}

std::vector<glm::vec3> Voxel::getVertices(){
    std::vector<glm::vec3> res;
    for (unsigned int i = 0 ; i < this->faces.size() ; i++){
        for (unsigned int j = 0 ; j < 4 ; j++){
            res.push_back(this->faces[i].vertices[j]);
        }
    }
    return res;
}

void Voxel::setId(int new_id){
    this->objectID = new_id;
}

int Voxel::getID(){
    return this->objectID;
}

void Voxel::setIdInChunk(int idInChunk){
    this->idInChunk = idInChunk;
}

int Voxel::getIdInChunk(){
    return this->idInChunk;
}

std::string Voxel::getFaceID(int orientationFace){
    return this->faces[orientationFace].unique_id;
}

std::string Voxel::getRacineFaceID(){
    return std::to_string((int)backBottomLeftCorner.x) + " " + std::to_string((int)backBottomLeftCorner.y) + " " + std::to_string((int)backBottomLeftCorner.z) + " ";
}

std::vector<glm::vec3> Voxel::getVerticesFromFace(int orientation){
    return this->faces[orientation].vertices;
}