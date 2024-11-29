#include <TerrainControler.hpp>

TerrainControler::TerrainControler(int planeWidth, int planeLength, int planeHeight, int typeChunk, int seedTerrain, int octave/*, std::vector<std::vector<std::string>> nomStructure*/){
    this->planeWidth = planeWidth;
    this->planeLength = planeLength; 
    this->planeHeight = planeHeight;
    this->typeChunk = typeChunk;
    this->seedTerrain = seedTerrain;
    this->octave = octave;
    this->accumulateurDestructionBlock = 0.0f;
    this->mouseLeftClickHold = false;
    this->previousIdInChunk = -2; // Attention à ne surtout pas initialiser avec -1 (sinon on tentera de casser un bloc hors liste de voxel)

    /*
    // Chargement des structures
    std::vector<std::vector<Structure>> structures;
    for (int i = 0 ; i < nomStructure.size() ; i++){
        std::vector<Structure> structuresBiome;
        for (int j = 0 ; j < nomStructure[i].size() ; j++){
            std::ifstream fileStructure(nomStructure[i][j]);
            structuresBiome.push_back(Chunk::readFile(fileStructure));
            fileStructure.close();
        }
        structures.push_back(structuresBiome);
    }
    */
    //Chunk::setListeStructures(structures);

    this->mg = new MapGenerator(this->planeWidth, this->planeLength, this->seedTerrain, this->octave); 
    this->mg->generateImage();
    int widthHeightmap, heightHeightmap, channels;
    unsigned char* dataPixels = stbi_load("../Textures/terrain.png", &widthHeightmap, &heightHeightmap, &channels, 4);
    this->buildPlanChunks(dataPixels, widthHeightmap, heightHeightmap);
    stbi_image_free(dataPixels);
}

// Ce deuxième constructeur ne sera appelé que pour créer le terrain utilisé par le mode éditeur
TerrainControler::TerrainControler(){
    this->listeChunks.clear();
    // On est obligé de définir les 3 valeurs ci-dessous
    this->planeWidth = 1;
    this->planeLength = 1; 
    this->planeHeight = 1;
    this->buildEditorChunk();
    this->mg = new MapGenerator(); // Pour ne pas poser problème avec le destructeur, on crée un MapGenerator vide 
}

TerrainControler::~TerrainControler(){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    delete mg;
}

std::vector<Chunk*> TerrainControler::getListeChunks(){
    return this->listeChunks;
}

void TerrainControler::buildPlanChunks(unsigned char* dataPixels, int widthHeightmap, int heightHeightmap){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    this->listeChunks.clear();
    for (int i = 0 ; i < this->planeWidth ; i++){
        for (int j = 0 ; j < this->planeLength ; j++){
            for (int k = 0 ; k < this->planeHeight ; k++){
                Chunk *c = new Chunk(glm::vec3((this->planeWidth*32)/2*(-1.f) + i*32,(this->planeHeight*32)/2*(-1.f) + k*32,(this->planeLength*32)/2*(-1.f) + j*32), this->typeChunk, dataPixels, widthHeightmap, heightHeightmap, i*32,j*32*this->planeWidth*32, seedTerrain); 
                c->loadChunk();
                this->listeChunks.push_back(c);
            }
        }
    }
    srand(time(NULL));
}

void TerrainControler::buildEditorChunk(){
    // Le terrain dans le mode éditeur est composé d'un unique chunk
    //this->listeChunks.clear();
    Chunk *c = new Chunk(glm::vec3(-16.0,-16.0,-16.0)); 
    c->loadChunk();
    this->listeChunks.push_back(c);
}

int TerrainControler::getPlaneWidth(){
    return this->planeWidth;
}
int* TerrainControler::getRefToPlaneWidth(){
    return &(this->planeWidth);
}
int TerrainControler::getPlaneLength(){
    return this->planeLength;
}
int* TerrainControler::getRefToPlaneLength(){
    return &(this->planeLength);
}
int TerrainControler::getPlaneHeight(){
    return this->planeHeight;
}
int* TerrainControler::getRefToSeedTerrain(){
    return &(this->seedTerrain);
}
int* TerrainControler::getRefToOctave(){
    return &(this->octave);
}


void TerrainControler::updateLight(std::vector<Voxel*> listeVoxels,int indiceV, glm::vec3 posBlock){
    int maxLuminosity=0;
    bool surface = true;

    listeVoxels[indiceV]->setLuminosity(0);
    for(int i=-1;i<2;i++){
        for(int j=-1;j<2;j++){
            for(int k=-1;k<2;k++){
                //int idBlocAdjacent = (numHauteur+k) *1024 + (numProfondeur%32+i) * 32 + (numLongueur%32+j); 
                
                int idBlocAdjacent = indiceV + k*1024 + i*32 + j;
                printf("il y'a un bloc idbloc %d\n",idBlocAdjacent);
                // printf("id adjacent = %d\n",idBlocAdjacent);
                if(listeVoxels[idBlocAdjacent]!=nullptr && idBlocAdjacent>0){
                    
                    maxLuminosity = std::max(maxLuminosity,listeVoxels[idBlocAdjacent]->getLuminosity()-1);
                    //printf("il y'a un bloc idbloc %d\n",idBlocAdjacent);

                    //printf("\nid block = %d et id block adjacent = %d et sa luminosite = %d\n\n",indiceV,idBlocAdjacent,listeVoxels[idBlocAdjacent]->getLuminosity());
                    
                    // printf("max luminosity des blocs adjacent : %d\n",listeVoxels[idBlocAdjacent]->getLuminosity());
                    // printf("i = %d j = %d\n",i,j);
                    // printf("MAX luminosity = %d\n",maxLuminosity);
                    
                }
            }
        }
    }
    //listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(16);

    for(double y = 1.0;y<16.0-posBlock.y;y+=1){ // y donne la couche du bloc en dessous 
        if(listeVoxels[indiceV + y * 1024]!=nullptr){
            surface=false;
            printf("max block = %f\n",16.0-posBlock.y);
            printf("et je check %f\n", y);
            
            continue;
        }
    }
    printf("surface = %d\n",surface);
    if(surface==true){ //si on est a la surface
        // printf("il n'y a pas de bloc au dessus\n");
        listeVoxels[indiceV]->setLuminosity(16);
        //printf("block surface\n");
    }else{
        //block directement au dessus
        
         listeVoxels[indiceV]->setLuminosity(maxLuminosity);  
         printf("update bloc en dessous\n"); 
        

        
        //printf("block  pas surface et max luminosity = %d\n",maxLuminosity);
        //listeVoxels[indiceV]->setLuminosity(0);
    }
}

LocalisationBlock TerrainControler::tryBreakBlock(glm::vec3 camera_target, glm::vec3 camera_position){
    glm::vec3 originPoint = camera_position;
    glm::vec3 direction = normalize(camera_target);

    //for (int k = 1 ; k < RANGE+1 ; k++){ // Trouver une meilleure manière pour détecter le bloc à casser
    //for (float k = 0.1 ; k < RANGE+1. ; k+=0.1){ // C'est mieux mais pas parfait
        //glm::vec3 target = originPoint + (float)k*direction;
        // int numLongueur = floor(target[0]) + 16*this->planeWidth;
        // int numHauteur = floor(target[1]) + 16;
        // int numProfondeur = floor(target[2]) + 16*this->planeLength;
        // int indiceChunk = (numLongueur/32) * this->planeLength + numProfondeur/32;
        
        // if (numLongueur < 0 || numLongueur > (this->planeWidth*32)-1 || numProfondeur < 0 || numProfondeur > (this->planeLength*32)-1 || numHauteur < 0 || numHauteur > 31){
        //     continue; // Attention à ne pas mettre return même si c'est tentant (par exemple si le joueur regarde vers le bas en étant au sommet d'un chunk)
        // }else{
    int step_x,step_y,step_z;
    if(direction.x > 0) step_x = 0; else step_x =-1;
    if(direction.y > 0) step_y = 0; else step_y =-1;
    if(direction.z > 0) step_z = 0; else step_z =-1;

    int numLongueur = floor(originPoint[0]) + 16*this->planeWidth;
    int numHauteur = floor(originPoint[1]) + 16;
    int numProfondeur = floor(originPoint[2]) + 16*this->planeLength;
    // int indiceV = numHauteur *1024 + (numProfondeur%32) * 32 + (numLongueur%32); // Indice du voxel que le joueur est en train de viser
    // int indiceChunk = (numLongueur/32) * this->planeLength + numProfondeur/32;
    // std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();

    int indiceVise;
    int indiceChunkVise;
    std::vector<Voxel*> listeVoxelsVise;


    glm::vec3 posChunk;
    glm::vec3 posBlock;

    float t_max_x;
    float t_delta_x;

    float t_max_y;
    float t_delta_y;

    float t_max_z;
    float t_delta_z;

    float vx = direction.x;
    float vy = direction.y;
    float vz = direction.z;

    int taille_cellule = 1;

    if(vx!=0){
        t_max_x = ((numLongueur + (step_x > 0)) * taille_cellule - originPoint.x) / vx;
        t_delta_x = std::abs(taille_cellule / vx);
    }else{
        t_max_x = float('inf');
        t_delta_x = float('inf');
    }

    if(vy!=0){
        t_max_y = ((numHauteur + (step_y > 0)) * taille_cellule - originPoint.y) / vy;
        t_delta_y = std::abs(taille_cellule / vy);
    }else{
        t_max_y = float('inf');
        t_delta_y = float('inf');
    }

    if(vz!=0){
        t_max_z = ((numProfondeur+ (step_z > 0)) * taille_cellule - originPoint.z) / vz;
        t_delta_z = std::abs(taille_cellule / vz);
    }else{
        t_max_z = float('inf');
        t_delta_z = float('inf');
    }

    float distance_parcourue = 0.0f;

    float t_avance = 0.0f;

    bool blocTrouve = false;

    while(distance_parcourue<3){
        if(t_max_x< t_max_y && t_max_x< t_max_z){
            t_avance = t_max_x;
            t_max_x += t_delta_x;
            numLongueur +=step_x;
        }else if(t_max_y< t_max_x && t_max_y< t_max_z){
            t_avance = t_max_y;
            t_max_y += t_delta_y;
            numHauteur +=step_y;
        }else if(t_max_z< t_max_y && t_max_z< t_max_x){
            t_avance = t_max_z;
            t_max_z += t_delta_z;
            numProfondeur +=step_z;
        }

        if(distance_parcourue <= 3){
            indiceVise = numHauteur *1024 + (numProfondeur%32) * 32 + (numLongueur%32); // Indice du voxel que le joueur est en train de viser
            indiceChunkVise = (numLongueur/32) * this->planeLength + numProfondeur/32;
            listeVoxelsVise = this->listeChunks[indiceChunkVise]->getListeVoxels();
            glm::vec3 posChunk = this->listeChunks[indiceChunkVise]->getPosition();
            glm::vec3 posBlock = glm::vec3(posChunk[0]+numLongueur%32,posChunk[1]+numHauteur,posChunk[2]+numProfondeur%32);
        }

        if(listeVoxelsVise[indiceVise]!=nullptr){
            blocTrouve=true;
            break;
        }
    }

    distance_parcourue = t_avance;

    

    

            

 
    if(blocTrouve && listeVoxelsVise[indiceVise]->getID() != 5){ // Le bloc de bedrock est incassable (donc attention si on en place un)
        
        // bloc qui émet de la lumière 
        // if(listeVoxels[indiceV]->getID()==26 || listeVoxels[indiceV]->getID()==8 ){ // pour l'instant on ne test que avec la pumpkin
        //     for(int i=-5;i<6;i++){
        //         for(int j=-5;j<6;j++){
        //             for(int k=-5;k<6;k++){
        //                 int indiceB = (numHauteur+k) *1024 + (numProfondeur%32+i) * 32 + (numLongueur%32+j); 
        //                 if(numHauteur+k<=31 && numHauteur+k>=0 && (numLongueur%32)+j<=31 && (numLongueur%32)+j>=0 && (numProfondeur%32)+i<=31 && (numProfondeur%32)+i>=0){ 
        //                         printf("indice B = %d\n",indiceV);
        //                         printf(" i = %d, j = %d, k = %d\n",i,j,k);
                                
        //                 }
        //                 if(listeVoxels[indiceB]!=nullptr){
                            
        //                     listeVoxels[indiceB]->setLuminosity(3);
        //                     if(listeVoxels[indiceB]->getID()==26){
        //                             listeVoxels[indiceB]->setLuminosity(16);
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }

        bool surface = true;
        int hauteurBlocDessous = -1000;

        for(double y2 = 1.0; y2<17.0+listeVoxelsVise[indiceVise]->getBackBottomLeftCorner().y; y2+=1 ){
                if(listeVoxelsVise[indiceVise - y2 * 1024]!=nullptr){
                    hauteurBlocDessous=y2;
                    break;
                }
            }

        for(double y = 1.0;y<12.0-listeVoxelsVise[indiceVise]->getBackBottomLeftCorner().y;y+=1){
            if(listeVoxelsVise[indiceVise + y * 1024]!=nullptr){
                surface=false;
            }
        }
        if(surface==true){ // il y'a un block au dessus
            listeVoxelsVise[indiceVise - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(16);
        }else{ 

        } 
        for(int i=-3;i<4;i++){
            for(int j=-3;j<4;j++){
                int idBlocAdjacent = (numProfondeur%32+i) * 32 + (numLongueur%32+j);
                
                //listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(3);
                if(listeVoxelsVise[indiceVise - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i]!=nullptr && (indiceVise - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i)>0 ){
                    if(surface==true && i!=0 && j!=0)updateLight(listeVoxelsVise,indiceVise - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i,posBlock + glm::vec3(i,0,j));
                    if(surface==false)updateLight(listeVoxelsVise,indiceVise - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i,posBlock + glm::vec3(i,0,j));
                }
            }
        }
            
        return {indiceVise, indiceChunkVise, numLongueur, numProfondeur, numHauteur, listeVoxelsVise[indiceVise]->getIdInChunk()};
    }
        //}
    //}
    return {-1,-1,-1,-1,-1,-1};
}

void TerrainControler::breakBlock(LocalisationBlock lb){ // Il faut déjà avoir testé (au minimum) si lb.indiceVoxel != -1 avant d'appeler cette fonction
    std::vector<Voxel*> listeVoxels = this->listeChunks[lb.indiceChunk]->getListeVoxels();

    delete listeVoxels[lb.indiceVoxel]; // Ne pas oublier de bien libérer la mémoire
    listeVoxels[lb.indiceVoxel] = nullptr;

    // Rendre visible les 6 cubes adjacents (s'ils existent et s'ils ne sont pas déjà visible)
    // Il faudrait chercher une meilleure façon de faire ça
    for (int c = -1 ; c < 2 ; c+=2){
        int numLongueurVoisin = (lb.numLongueur%32) + c;
        int numHauteurVoisin = lb.numHauteur + c;
        int numProfondeurVoisin = (lb.numProfondeur%32) + c;
        int indiceVoisin;

        if (numLongueurVoisin >= 0 && numLongueurVoisin <= 31){
            indiceVoisin = lb.numHauteur *1024 + (lb.numProfondeur%32) * 32 + numLongueurVoisin;
            if (listeVoxels[indiceVoisin] != nullptr && !(listeVoxels[indiceVoisin]->getVisible())){ // On vérifie si le voxel n'est pas déjà visible (en vrai c'est pas obligatoire)
                listeVoxels[indiceVoisin]->setVisible(true);
            }
        }
        if (numHauteurVoisin >= 0 && numHauteurVoisin <= 31){
            indiceVoisin = numHauteurVoisin *1024 + (lb.numProfondeur%32) * 32 + (lb.numLongueur%32);
            if (listeVoxels[indiceVoisin] != nullptr && !(listeVoxels[indiceVoisin]->getVisible())){
                listeVoxels[indiceVoisin]->setVisible(true);
            }
        }
        if (numProfondeurVoisin >= 0 && numProfondeurVoisin <= 31){
            indiceVoisin = lb.numHauteur *1024 + numProfondeurVoisin * 32 + (lb.numLongueur%32);
            if (listeVoxels[indiceVoisin] != nullptr && !(listeVoxels[indiceVoisin]->getVisible())){
                listeVoxels[indiceVoisin]->setVisible(true);
            }
        }
    }

    this->listeChunks[lb.indiceChunk]->setListeVoxels(listeVoxels);
    this->listeChunks[lb.indiceChunk]->loadChunk();
    //return; 
}

bool TerrainControler::tryCreateBlock(glm::vec3 camera_target, glm::vec3 camera_position, int typeBlock){
    glm::vec3 originPoint = camera_position;
    glm::vec3 direction = normalize(camera_target);
    float k = 3.0; // Pour l'instant le joueur ne peut poser un block qu'à cette distance
    glm::vec3 target = originPoint + (float)k*direction;
    int numLongueur = floor(target[0]) + 16*this->planeWidth;
    int numHauteur = floor(target[1]) + 16;      
    int numProfondeur = floor(target[2]) + 16*this->planeLength;
    bool surface=true;
    if (numLongueur < 0 || numLongueur > (this->planeWidth*32)-1 || numProfondeur < 0 || numProfondeur > (this->planeLength*32)-1 || numHauteur < 0 || numHauteur > 31){
        return false;
    }else{
        int indiceV = numHauteur *1024 + (numProfondeur%32) * 32 + (numLongueur%32); // Indice du voxel que le joueur est en train de viser
        int indiceChunk = (numLongueur/32) * this->planeLength + numProfondeur/32;
        std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();

        if (listeVoxels[indiceV] == nullptr){
            glm::vec3 posChunk = this->listeChunks[indiceChunk]->getPosition();
            glm::vec3 posBlock = glm::vec3(posChunk[0]+numLongueur%32,posChunk[1]+numHauteur,posChunk[2]+numProfondeur%32);
            Voxel* vox = new Voxel(posBlock,typeBlock);
            vox->setVisible(true);
            listeVoxels[indiceV] = vox;

            //gère la lumière
            int luminosityBlock = vox->getLuminosity();

            // printf("hauteur de notre block : %f\n",posBlock.y);

            // printf("taille = %d\n",listeVoxels.size());

            

            int hauteurBlocDessous = -1000;
            for(double y2 = 1.0; y2<17.0+listeVoxels[indiceV]->getBackBottomLeftCorner().y; y2+=1 ){
                printf("y2 = %f et max hauteur %f\n",y2,17.0+listeVoxels[indiceV]->getBackBottomLeftCorner().y);
                if(listeVoxels[indiceV - y2 * 1024]!=nullptr){
                    hauteurBlocDessous=y2;
                    break;
                }
            }
            int maxLuminosity = 0;

            for(double y = 1.0;y<16.0-posBlock.y;y+=1){
                if(listeVoxels[indiceV + y * 1024]!=nullptr){
                    surface=false;
                }
            }
            if(surface==true){ //si on est a la surface
                // printf("il n'y a pas de bloc au dessus\n");
                listeVoxels[indiceV]->setLuminosity(16);
            }else{
                
            }

            if(hauteurBlocDessous!=-1000 && listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]!=nullptr){ // si il y'a bien un bloc en dessous
                //updateLight(listeVoxels,indiceV,hauteurBlocDessous);
                listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(0);
                // for(int i=-1;i<2;i++){
                //     for(int j=-1;j<2;j++){
                //         //for(int k=-1;k<2;k++){
                //             //int idBlocAdjacent = (numHauteur+k) *1024 + (numProfondeur%32+i) * 32 + (numLongueur%32+j); 
                //             int idBlocAdjacent = indiceV - hauteurBlocDessous*1024 + i*32 + j;
                //             // printf("id adjacent = %d\n",idBlocAdjacent);
                //             if(listeVoxels[idBlocAdjacent]!=nullptr){
                //                 maxLuminosity = std::max(maxLuminosity,listeVoxels[idBlocAdjacent]->getLuminosity()-1);
                //                 printf("max luminosity des blocs adjacent : %d\n",listeVoxels[idBlocAdjacent]->getLuminosity());
                //                 printf("i = %d j = %d\n",i,j);
                //             }
                //         //}
                //     }
                // }

                for(int i=-3;i<4;i++){
                    for(int j=-3;j<4;j++){
                        
                        int idBlocAdjacent = (numProfondeur%32+i) * 32 + (numLongueur%32+j);
                        
                        //listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(3);
                        if(listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i]!=nullptr){
                            updateLight(listeVoxels,indiceV - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i,posBlock + glm::vec3(i,0,j));
                        }
                    }
                }
                //listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(16);
                //listeVoxels[indiceV - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(maxLuminosity);
            }

            
            
            //on a trouver le bloc en dessous
            //printf("id block dessous = %d\n",hauteurBlocDessous);
            

            

            this->listeChunks[indiceChunk]->setListeVoxels(listeVoxels);
            this->listeChunks[indiceChunk]->loadChunk();
            

            return true;
        }
    }

    return false;
}

void TerrainControler::drawTerrain(){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        this->listeChunks[i]->drawChunk();
    }
}

/*
void TerrainControler::saveStructure(std::string filePath){
    filePath = "../Structures/" + filePath + ".txt";
    std::ofstream fileStructure(filePath);

    if (!fileStructure.is_open()) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier. Vérifiez le nom donné au fichier\n";
    }

    // On récupère l'unique chunk du terrain (car on est en mode édition)
    Chunk *c = this->listeChunks[0];
    std::vector<Voxel*> voxelsToSave = c->getListeVoxels();
    for (int i = 0 ; i < voxelsToSave.size() ; i++){
        Voxel *v = voxelsToSave[i];
        if (v != nullptr){
            // Attention à bien mettre un espace à la fin, avant le retour à la ligne
            fileStructure << v->getID() << " " << floor(v->getBackBottomLeftCorner()[0]) << " " << floor(v->getBackBottomLeftCorner()[1]) << " " << floor(v->getBackBottomLeftCorner()[2]) << " \n";
        }
    }
    fileStructure.close();
}
*/

bool TerrainControler::checkHoldLeftClick(glm::vec3 camera_position, glm::vec3 camera_target, float deltaTime, bool modeJeu, GLuint programID){
    if (this->mouseLeftClickHold){
        LocalisationBlock lb = this->tryBreakBlock(camera_target, camera_position);
        // On part du principe que c'est impossible pour le joueur de viser un bloc d'un chunk à la frame n, puis le bloc équivalent d'un chunk adjacent à la frame n+1
        // Puisque la portée de son coup est limité à 4 (RANGE dans TerrainControler.hpp)
        // Du coup, il y a certains tests qu'on peut se permettre d'éviter
        if (lb.idInChunk == this->previousIdInChunk){
            this->accumulateurDestructionBlock += deltaTime;
            glUniform1i(glGetUniformLocation(programID, "accumulateur_destruction"), this->accumulateurDestructionBlock*100); // Dans le shader on utilise cette valeur avec % donc il faut que ce soit un int
        }else if (lb.idInChunk != -1){ // A la première frame où on maintient le clic gauche, on rentre dans cette condition, et donc on définit à ce moment previousIdInChunk 
            this->previousIdInChunk = lb.idInChunk;
            this->setAccumulation(0.0f);
            glUniform1i(glGetUniformLocation(programID, "accumulateur_destruction"), this->accumulateurDestructionBlock*100); // On renvoie l'accumulation ici pour bien reprendre l'animation de 0 (sinon on voyait l'ancienne état de destruction s'afficher pendant une frame)
            glUniform1i(glGetUniformLocation(programID, "indexBlockToBreak"), this->previousIdInChunk); // On envoie l'indice du voxel visé aux shaders (pour savoir où appliquer la texture de destruction)
        }else{
            this->setAccumulation(0.0f);
            this->previousIdInChunk = -2;
            glUniform1i(glGetUniformLocation(programID, "indexBlockToBreak"), this->previousIdInChunk);
        }

        // Si on est en mode créatif, on vérifie si on vise bien un bloc (c'est qui causait la segfault)
        if ((modeJeu && this->previousIdInChunk != -2)|| this->accumulateurDestructionBlock >= 1.0f){ // En mode créatif, les blocs se cassent directement
            this->breakBlock(lb);
            this->accumulateurDestructionBlock = 0.0f;
            return true; // Permettra de savoir quand jouer le son de cassage d'un bloc
            
            // Si on voulait être précis, ici il aurait fallu remettre à jour previousIdInChunk en rappelant tryBreakBlock
            // Mais ce n'est pas nécéssaire, ce sera fait seulement avec une frame de retard (pas trop grave)
        }
    }
    return false;
}

void TerrainControler::setMouseLeftClickHold(bool mouseLeftClickHold){
    this->mouseLeftClickHold = mouseLeftClickHold;
}

bool TerrainControler::getMouseLeftClickHold(){
    return mouseLeftClickHold;
}

int TerrainControler::getPreviousIdInChunk(){
    return this->previousIdInChunk;
}

void TerrainControler::setPreviousIdInChunk(int previousIdInChunk){
    this->previousIdInChunk = previousIdInChunk;
}

void TerrainControler::setAccumulation(float accumulation){
    this->accumulateurDestructionBlock = accumulation;
}

MapGenerator* TerrainControler::getMapGenerator(){
    return this->mg;
}
