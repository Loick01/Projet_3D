#include <Chunk.hpp>

#define CHUNK_SIZE 32

Chunk::Chunk(int i, int j, int k, FastNoise noiseGenerator, int nbTerrainChunk, glm::vec3 position, int typeChunk, unsigned char* dataPixels, unsigned char* dataPixelsCaveAC, int widthHeightmap, int lengthHeightMap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc){
    this->pos_i = i;
    this->pos_j = j;
    this->pos_k = k;
    this->position = position;
    //this->ID=rand()%3;
    // this->ID=0;
    if (typeChunk==0){
        this->buildFullChunk();
    }else if (typeChunk==1){
        this->buildCheeseChunk(i, j, k, noiseGenerator);
    }else if (typeChunk==1){
        this->buildSinusChunk();
    }else if (typeChunk==2){
        this->buildFlatChunk();
    }else if (typeChunk==3){
        if (k >= nbTerrainChunk){
            this->buildProceduralChunk(dataPixels, widthHeightmap, lengthHeightMap, posWidthChunk, posLengthChunk, seed, hauteurChunkTerrain, tc, noiseGenerator);
        }else{
            //this->buildCheeseChunk(i, j, k, noiseGenerator);
            //this->buildFullChunk();
            this->buildCaveChunk(k, dataPixelsCaveAC, posWidthChunk, posLengthChunk, widthHeightmap);
        }
    }
}

Chunk::Chunk(glm::vec3 position){
    this->position = position;
    this->buildEditorChunk();
}

Chunk::~Chunk(){
    // std::cout << "Destruction de Chunk\n";
    for (int i = 0 ; i < this->listeVoxels.size() ; i++){
        delete this->listeVoxels[i];
    }
    glDeleteBuffers(1, &(this->vertexbuffer));
    glDeleteBuffers(1, &(this->elementbuffer));
}

void Chunk::buildEditorChunk(){
    this->listeVoxels.clear();
    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){     
            for (int i=0;i<CHUNK_SIZE;i++){ 
                if (i == CHUNK_SIZE/2 && j == CHUNK_SIZE/2 && k == CHUNK_SIZE/2){
                    Voxel *vox = new Voxel(glm::vec3(0.0,0.0,0.0),STONE_BLOCK); 
                    this->listeVoxels.push_back(vox);
                }else{
                    this->listeVoxels.push_back(nullptr);
                }
            }
        }
    }
}

void Chunk::buildFullChunk(){
    this->listeVoxels.clear();

    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){
            for (int i=0;i<CHUNK_SIZE;i++){
                Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),rand()%NB_BLOCK); 
                if (i*j*k==0 || i==CHUNK_SIZE-1 || j==CHUNK_SIZE-1 || k==CHUNK_SIZE-1){
                    vox->setId(GRASS_BLOCK);
                }
                this->listeVoxels.push_back(vox);
            }
        }
    }
}

void Chunk::buildCaveChunk(int hauteurChunk, unsigned char* dataPixelsCaveAC, int posWidthChunk, int posLengthChunk, int widthHeightmap){
    this->listeVoxels.clear();

    // On remplit entièrement le chunk de STONE_BLOCK
    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){
            for (int i=0;i<CHUNK_SIZE;i++){
                Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),rand()%NB_BLOCK); 
                vox->setId(STONE_BLOCK);
                this->listeVoxels.push_back(vox);
            }
        }
    }

    // On génère la grotte (pour l'instant au chunk le plus bas, et sur 5 blocs de haut)
    // Penser à étendre la génération des grottes sur des chunks avec des hauteurs différentes
    if (hauteurChunk == 0){
        for (int j=0;j<CHUNK_SIZE;j++){
            for (int i=0;i<CHUNK_SIZE;i++){
                int indInText = posLengthChunk + posWidthChunk + j*widthHeightmap + i;
                unsigned char heightBlockCave = dataPixelsCaveAC[indInText];
                if (heightBlockCave>0){
                    for (int k = heightBlockCave ; k < heightBlockCave+5 ; k++){
                        if (k < 32){
                            unsigned int vox_index = k*CHUNK_SIZE*CHUNK_SIZE + j*CHUNK_SIZE + i;
                            Voxel *vox = this->listeVoxels[vox_index];
                            delete vox;
                            this->listeVoxels[vox_index] = nullptr;
                        }
                    }
                }
            }
        }
    }
}

void Chunk::buildCheeseChunk(int a, int b, int c, FastNoise noiseGenerator){
    this->listeVoxels.clear();

    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){     
            for (int i=0;i<CHUNK_SIZE;i++){ 
                float density = noiseGenerator.GetNoise((a*32 + i) * 1, (c*32 + k) * 1, (b*32 + j) * 1);
                if (density > 0){
                    int typeBlock = rand() % 500;
                    int sizeVein = rand() % 3;
                    Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),(typeBlock==0?DIAMOND_ORE:(typeBlock<10?IRON_ORE:STONE_BLOCK))); 

                    this->listeVoxels.push_back(vox);

                    /*
                    // Génération des filons de minerais
                    int idToGenerate = listeVoxels[k*1024+32*j+i]->getID();
                    if((idToGenerate==DIAMOND_ORE || idToGenerate==IRON_ORE) && j>=sizeVein && i>=sizeVein && k>=sizeVein && j<CHUNK_SIZE){
                        for(int l=1;l<=sizeVein;l++){
                            for(int m=1;m<=sizeVein;m++){
                                for(int n=1;n<=sizeVein;n++){
                                    if(rand()%(m*4)==0)listeVoxels[k*1024+32*(j-m)+i]->setId(idToGenerate);
                                    if(rand()%(n*4)==0)listeVoxels[k*1024+32*j+(i-n)]->setId(idToGenerate);
                                    if(rand()%(n*4)==0)listeVoxels[k*1024+32*(j-m)+(i-n)]->setId(idToGenerate);
                                    if(rand()%(l*4)==0)listeVoxels[(k-l)*1024+32*j+i]->setId(idToGenerate);
                                    if(rand()%(m*4)==0)listeVoxels[(k-l)*1024+32*(j-m)+i]->setId(idToGenerate);
                                    if(rand()%(l*4)==0)listeVoxels[(k-l)*1024+32*j+(i-n)]->setId(idToGenerate);
                                    if(rand()%(m*4)==0)listeVoxels[(k-l)*1024+32*(j-m)+(i-n)]->setId(idToGenerate);
                                }
                            }
                        }
                    }
                    */
                }else{
                    this->listeVoxels.push_back(nullptr);
                }
                
            }
        }
    }

    /*
    Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),rand()%NB_BLOCK); 
    if (i*j*k==0 || i==CHUNK_SIZE-1 || j==CHUNK_SIZE-1 ||k==CHUNK_SIZE-1){
        vox->setId(GRASS_BLOCK);
    }
    this->listeVoxels.push_back(vox);
    */    
}

void Chunk::buildFlatChunk(){
    this->listeVoxels.clear();
    int hauteurMax = 3; // Nombre de couches générées
    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){     
            for (int i=0;i<CHUNK_SIZE;i++){     
                    if (k >= hauteurMax){
                        this->listeVoxels.push_back(nullptr);
                    }else{
                        Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),rand()%NB_BLOCK); 
                        if (i*j*k==0 || i==CHUNK_SIZE-1 || j==CHUNK_SIZE-1 ||k==hauteurMax-1){
                            vox->setId(GRASS_BLOCK);
                        }
                        this->listeVoxels.push_back(vox);
                    }
            }
        }
    }
}

// Avec ce modèle de génération, on voit qu'il y a un problème sur la détection de l'extérieur. Il faudra un algo pour déterminer les contours d'un chunk
void Chunk::buildSinusChunk(){
    this->listeVoxels.clear();

    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){     
            for (int i=0;i<CHUNK_SIZE;i++){
                float t = (float)i / CHUNK_SIZE-1;
                float s = (float)j / CHUNK_SIZE-1;
                float value = (std::sin(4 * M_PI * ((t+s)/2)) + 1.0f) * 0.5f; // Valeur entre 0 et 1
                //float value = (std::sin(2 * M_PI * t) + 1.0f) * 0.5f; // Valeur entre 0 et 1
                int heightVox = value*30 + 1; // Hauteur du cube entre 1 et 31
                if (k <= heightVox){
                    Voxel *vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j), 0); 
                    this->listeVoxels.push_back(vox);
                }else{
                    this->listeVoxels.push_back(nullptr);
                }
            }
        }
    }
}

void Chunk::buildProceduralChunk(unsigned char* dataPixels, int widthHeightmap, int lengthHeightMap, int posWidthChunk, int posLengthChunk, int seed, int hauteurChunkTerrain, TerrainControler* tc, FastNoise noiseGenerator){
    this->listeVoxels.clear();

    srand(seed+posWidthChunk+posLengthChunk);

    // Génération du terrain
    int biomeID = 0;
    int posWidth = posWidthChunk;
    // Attention à ne pas utiliser posLengthChunk directement, la valeur en paramètre est modifié pour correspondre au dimension de la heightmap (d'où le calcul ci-dessous)
    int posLength = posLengthChunk/tc->getPlaneWidth()/32;
    for (int k=0;k<CHUNK_SIZE;k++){
        for (int j=0;j<CHUNK_SIZE;j++){     
            for (int i=0;i<CHUNK_SIZE;i++){ 
                int blockHeight = k + hauteurChunkTerrain*32;
                int indInText = posLengthChunk + posWidthChunk + j*widthHeightmap + i;
                if (tc->hasBiomeChart()){
                    // Utiliser i et j pour déterminer les valeurs de précipitation et d'humidité
                    // Attention à ne pas avoir de corrélation entre ces 2 valeurs, d'où l'utilisation d'offset (ici 1000 pour x et 1500 pour z)
                    // C'est ici qu'on peut faire varier la taille des biomes (peut être rendre ça modifiable via un slider ImGui)
                    float precipitation = (noiseGenerator.GetNoise((posWidth + i), (posLength + j))+1.0)/2.0;
                    float humidite = (noiseGenerator.GetNoise((posWidth + i + 1000), (posLength + j + 1500))+1.0)/2.0;
                    biomeID = tc->getBiomeID(precipitation,humidite); // On utilise la position x et z du block pour déterminer le biome
                }
                if (blockHeight <= ((int)dataPixels[indInText])){ 
                    int typeBlock = rand() % 500;
                    int sizeVein = rand() % 3;
                    Voxel *vox;
                    if(biomeID==0){
                        vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),blockHeight>=(int)dataPixels[indInText]-(2+rand()%4) ? DIRT_BLOCK : (typeBlock==0?DIAMOND_ORE:(typeBlock<10?IRON_ORE:STONE_BLOCK))); 
                    }else if(biomeID==1){
                        vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),blockHeight>=(int)dataPixels[indInText]-(2+rand()%4) ? SAND_BLOCK : (typeBlock==0?DIAMOND_ORE:(typeBlock<10?IRON_ORE:STONE_BLOCK))); 
                    }else if(biomeID==2){
                        vox = new Voxel(glm::vec3(this->position[0]+i,this->position[1]+k,this->position[2]+j),blockHeight>=(int)dataPixels[indInText]-(2+rand()%4) ? SNOW_BLOCK : (typeBlock==0?DIAMOND_ORE:(typeBlock<10?IRON_ORE:STONE_BLOCK))); 
                    }
                    
                    if (blockHeight==(int)dataPixels[indInText]){
                        if(biomeID==0){
                            vox->setId(GRASS_BLOCK);
                        }else if(biomeID==1){
                            vox->setId(SAND_BLOCK);
                        }else if(biomeID==2){
                            //vox->setId(SNOW_BLOCK);
                            vox->setId(STONE_BLOCK); // Pour mieux faire la différence avec les blocs de sable (biomeID == 1)
                        }
                    }
                    this->listeVoxels.push_back(vox);

                    /* A voir si on laisse la génération de minerais pour les chunks les plus haut (en fonction de ce qu'on décide pour les règles de génération)
                    // Génération des filons de minerais
                    int idToGenerate = listeVoxels[k*1024+32*j+i]->getID();
                    if((idToGenerate==DIAMOND_ORE || idToGenerate==IRON_ORE) && j>=sizeVein && i>=sizeVein && k>=sizeVein && j<CHUNK_SIZE){
                        for(int l=1;l<=sizeVein;l++){
                            for(int m=1;m<=sizeVein;m++){
                                for(int n=1;n<=sizeVein;n++){
                                    if(rand()%(m*4)==0)listeVoxels[k*1024+32*(j-m)+i]->setId(idToGenerate);
                                    if(rand()%(n*4)==0)listeVoxels[k*1024+32*j+(i-n)]->setId(idToGenerate);
                                    if(rand()%(n*4)==0)listeVoxels[k*1024+32*(j-m)+(i-n)]->setId(idToGenerate);
                                    if(rand()%(l*4)==0)listeVoxels[(k-l)*1024+32*j+i]->setId(idToGenerate);
                                    if(rand()%(m*4)==0)listeVoxels[(k-l)*1024+32*(j-m)+i]->setId(idToGenerate);
                                    if(rand()%(l*4)==0)listeVoxels[(k-l)*1024+32*j+(i-n)]->setId(idToGenerate);
                                    if(rand()%(m*4)==0)listeVoxels[(k-l)*1024+32*(j-m)+(i-n)]->setId(idToGenerate);
                                }
                            }
                        }
                    }
                    */
                }else{
                    this->listeVoxels.push_back(nullptr);
                }
            }
        }
    }

    /*
    // On place de la bedrock à la dernière couche
    for (int j=0;j<CHUNK_SIZE;j++){     
        for (int i=0;i<CHUNK_SIZE;i++){
            this->listeVoxels[j*CHUNK_SIZE+i]->setId(5);
        }
    }
    */
}

void Chunk::addIndices(int* compteur){
    int decalage = ((*compteur)++)*4; // 4 sommets par face
    this->indices.push_back(decalage + 2);
    this->indices.push_back(decalage + 0);
    this->indices.push_back(decalage + 3);
    this->indices.push_back(decalage + 3);
    this->indices.push_back(decalage + 0);
    this->indices.push_back(decalage + 1);
}

void Chunk::loadChunk(TerrainControler* tc){
    // ATTENTION : tc a une valeur apr défaut à nullptr

    // Très important de vider les vectors, sinon quand on modifie un chunk on ne voit aucune différence
    this->vertices.clear();
    this->indices.clear();
    this->objectIDs.clear();
    this->faceIDs.clear(); 

    // int compteur = 0; // Nombre de voxel déjà chargé, pour savoir où en est le décalage des indices
    int compteur = 0; // Nombre de face déjà chargé, pour savoir où en est le décalage des indices
    //std::vector<int> objectIDs;

    for (int i = 0 ; i < this->listeVoxels.size() ; i++){
        if (listeVoxels[i] != nullptr){
            /*
            if (listeVoxels[i]->getVisible()){
                listeVoxels[i]->setIdInChunk(compteur); // Les voxels auront besoin d'avoir connaissance de leurs positions dans les buffers (c'est pour l'animation de la destruction d'un bloc)
                std::vector<glm::vec3> verticesVoxel = listeVoxels[i]->getVertices();
                this->vertices.insert(this->vertices.end(), verticesVoxel.begin(), verticesVoxel.end());

                for (int c = 0 ; c < 6 ; c++){ // Toutes les faces d'un voxel
                    unsigned int decalage = 24*compteur + c*4;
                    this->indices.push_back(decalage + 2);
                    this->indices.push_back(decalage + 0);
                    this->indices.push_back(decalage + 3);
                    this->indices.push_back(decalage + 3);
                    this->indices.push_back(decalage + 0);
                    this->indices.push_back(decalage + 1);
                }
                compteur++;

                objectIDs.push_back(listeVoxels[i]->getID());
            }
            */

            std::vector<glm::vec3> voxel_vertices = listeVoxels[i]->getVertices();
            int voxel_id = listeVoxels[i]->getID();

            // Face inférieur
            if (i >= CHUNK_SIZE*CHUNK_SIZE){
                if (listeVoxels[i-CHUNK_SIZE*CHUNK_SIZE] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(0);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin(), voxel_vertices.begin()+4);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk en dessous (pos_k-1)
                Chunk *cnk = tc->getChunkAt(this->pos_i,this->pos_k-1,this->pos_j);
                if (cnk != nullptr && cnk->getListeVoxels()[i+(32*32*31)] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(0);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin(), voxel_vertices.begin()+4);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }

            // Face supérieur
            if (i/(CHUNK_SIZE*CHUNK_SIZE) != (CHUNK_SIZE-1)){
                if (listeVoxels[i+CHUNK_SIZE*CHUNK_SIZE] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(1);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+4, voxel_vertices.begin()+8);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk au dessus (pos_k+1)
                Chunk *cnk = tc->getChunkAt(this->pos_i,this->pos_k+1,this->pos_j);
                if (cnk != nullptr && cnk->getListeVoxels()[i-(32*32*31)] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(1);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+4, voxel_vertices.begin()+8);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }

            // Face arrière
            if (i%(CHUNK_SIZE*CHUNK_SIZE) >= CHUNK_SIZE){
                if (listeVoxels[i-CHUNK_SIZE] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(2);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+8, voxel_vertices.begin()+12);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk derrière (pos_j-1)
                Chunk *cnk = tc->getChunkAt(this->pos_i,this->pos_k,this->pos_j-1);
                if (cnk != nullptr && cnk->getListeVoxels()[i+(32*31)] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(2);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+8, voxel_vertices.begin()+12);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }

            // Face avant
            if (i%(CHUNK_SIZE*CHUNK_SIZE) < CHUNK_SIZE*(CHUNK_SIZE-1)){
                if (listeVoxels[i+CHUNK_SIZE] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(3);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+12, voxel_vertices.begin()+16);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk devant (pos_j+1)
                Chunk *cnk = tc->getChunkAt(this->pos_i,this->pos_k,this->pos_j+1);
                if (cnk != nullptr && cnk->getListeVoxels()[i-(32*31)] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(3);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+12, voxel_vertices.begin()+16);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }

            // Face gauche
            if (i%CHUNK_SIZE != 0){
                if (listeVoxels[i-1] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(4);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+16, voxel_vertices.begin()+20);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk à gauche (pos_i-1)
                Chunk *cnk = tc->getChunkAt(this->pos_i-1,this->pos_k,this->pos_j);
                if (cnk != nullptr && cnk->getListeVoxels()[i+31] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(4);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+16, voxel_vertices.begin()+20);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }

            // Face droite
            if (i%CHUNK_SIZE != (CHUNK_SIZE-1)){
                if (listeVoxels[i+1] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(voxel_id);
                    this->faceIDs.push_back(5);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+20, voxel_vertices.begin()+24);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }else{
                // Il faut aller le chunk à droite (pos_i+1)
                Chunk *cnk = tc->getChunkAt(this->pos_i+1,this->pos_k,this->pos_j);
                if (cnk != nullptr && cnk->getListeVoxels()[i-31] == nullptr){
                    addIndices(&compteur);
                    this->objectIDs.push_back(3);
                    this->faceIDs.push_back(5);
                    std::vector<glm::vec3> face_vertices(voxel_vertices.begin()+20, voxel_vertices.begin()+24);
                    this->vertices.insert(this->vertices.end(), face_vertices.begin(), face_vertices.end());
                }
            }
        }
    }
    
    this->sendVoxelMapToShader();
}

void Chunk::sendVoxelMapToShader(){
    glGenBuffers(1, &(this->vertexbuffer));
    glBindBuffer(GL_ARRAY_BUFFER, this->vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec3), &(this->vertices[0]), GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &(this->elementbuffer));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size()* sizeof(unsigned int), &(this->indices[0]) , GL_DYNAMIC_DRAW);
}

void Chunk::drawChunk(){

    // Pour les ID des blocs, on utilise des shaders storage buffers
    glGenBuffers(1, &(this->shaderstoragebuffer));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->shaderstoragebuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, this->objectIDs.size()*sizeof(int), this->objectIDs.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->shaderstoragebuffer); // Attention : Dans le shader, binding doit valoir la même chose que le 2è paramètre
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &(this->shaderstoragebuffer3));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->shaderstoragebuffer3);
    glBufferData(GL_SHADER_STORAGE_BUFFER, this->faceIDs.size()*sizeof(int), this->faceIDs.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->shaderstoragebuffer3); // Attention : Dans le shader, binding doit valoir la même chose que le 2è paramètre
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->vertexbuffer);
    glVertexAttribPointer(
                    0,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    (void*)0            // array buffer offset
                    );

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->elementbuffer);

    // Draw the triangles !
    glDrawElements(
                    GL_TRIANGLES,      // mode
                    this->indices.size(), // count
                    GL_UNSIGNED_INT,   // type
                    (void*)0           // element array buffer offset
                    );

    glDisableVertexAttribArray(0);

    glDeleteBuffers(1, &(this->shaderstoragebuffer)); // Attention à bien le supprimer, c'est ça qui causait la chute de FPS au bout d'un certain temps
    glDeleteBuffers(1, &(this->shaderstoragebuffer3));
}

std::vector<Voxel*> Chunk::getListeVoxels(){
    return this->listeVoxels;
}

void Chunk::setListeVoxels(std::vector<Voxel*> newListeVoxels){
    this->listeVoxels=newListeVoxels;
}

glm::vec3 Chunk::getPosition(){
    return this->position;
}
