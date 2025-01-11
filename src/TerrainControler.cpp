#include <TerrainControler.hpp>

std::vector<std::vector<Structure>> TerrainControler::structures; // Permet d'éviter les erreurs de lien à la compilation

float creationDistance=3.0f;
float radius=1.0f;
bool brushTool = false;
bool sphereTool = false;
bool cubeTool = true;
bool erasor = false;

TerrainControler::TerrainControler(int planeWidth, int planeLength, int planeHeight, int typeChunk, int seedTerrain, int octave, std::vector<std::vector<std::string>> nomStructure){
    this->planeWidth = planeWidth;
    this->planeLength = planeLength; 
    this->planeHeight = planeHeight;
    this->typeChunk = typeChunk;
    this->seedTerrain = seedTerrain;
    this->octave = octave;
    this->accumulateurDestructionBlock = 0.0f;
    this->mouseLeftClickHold = false;
    this->mouseRightClickHold = false;
    this->previousIdInChunk = -2; // Attention à ne surtout pas initialiser avec -1 (sinon on tentera de casser un bloc hors liste de voxel)
    this->generateStructure = true;

    // Chargement des structures
    for (int i = 0 ; i < nomStructure.size() ; i++){
        std::vector<Structure> structuresBiome;
        for (int j = 0 ; j < nomStructure[i].size() ; j++){
            std::ifstream fileStructure(nomStructure[i][j]);
            structuresBiome.push_back(this->readStructureFile(fileStructure));
            fileStructure.close();
        }
        this->structures.push_back(structuresBiome);
    }

    this->biomeChart = false;
    this->mg = new MapGenerator(this->planeWidth, this->planeLength, this->seedTerrain, this->octave); 
    this->mg->generateImageSurface();
    this->mg->generateImageCave_AC();
    this->mg->generateImageCave_Perlin();
    int widthHeightmap, lengthHeightMap, channels;
    unsigned char* dataPixels = stbi_load("../Textures/terrain.png", &widthHeightmap, &lengthHeightMap, &channels, 1);
    unsigned char* dataPixelsCaveAC = stbi_load("../Textures/cave_AC.png", &widthHeightmap, &lengthHeightMap, &channels, 1);
    this->nbChunkTerrain = 1;
    this->buildPlanChunks(dataPixels, dataPixelsCaveAC, widthHeightmap, lengthHeightMap);
    if (this->generateStructure){
        this->buildStructures(dataPixels);
    }
    srand(time(NULL));
    this->loadTerrain(); // On a besoin que tous les chunks soient générés avant de charger le terrain pour la première fois
    stbi_image_free(dataPixels);
    stbi_image_free(dataPixelsCaveAC);
}

// Ce deuxième constructeur ne sera appelé que pour créer le terrain utilisé par le mode éditeur
TerrainControler::TerrainControler(){
    this->listeChunks.clear();
    // Par défaut, un seul chunk
    this->planeWidth = 1;
    this->planeLength = 1; 
    this->planeHeight = 1;
    this->buildEditorChunk();
    this->loadTerrain();
    this->mg = new MapGenerator(); // Pour ne pas poser problème avec le destructeur, on crée un MapGenerator vide 
}

TerrainControler::~TerrainControler(){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    delete mg;
}

Structure TerrainControler::readStructureFile(std::ifstream &file){
    Structure resStructure;
    std::string line;

    while (std::getline(file, line)) { 
        std::istringstream flux(line);
        BlockStructure bs;
        for (int i = 0 ; i < 4 ; i++){
            flux >> bs.infoBlock[i];
        }
        resStructure.blocks.push_back(bs);
    }
    return resStructure;
}

std::vector<Chunk*> TerrainControler::getListeChunks(){
    return this->listeChunks;
}

void TerrainControler::buildPlanChunks(unsigned char* dataPixels, unsigned char* dataPixelsCaveAC, int widthHeightmap, int lengthHeightMap){
    FastNoise ng = this->mg->getNoiseGenerator();
    
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    this->listeChunks.clear();
    for (int i = 0 ; i < this->planeWidth ; i++){
        for (int j = 0 ; j < this->planeLength ; j++){
            for (int k = 0 ; k < this->planeHeight ; k++){
                Chunk *c = new Chunk(i, j, k, ng, this->planeHeight-this->nbChunkTerrain, glm::vec3((this->planeWidth*32)/2*(-1.f) + i*32, k*32, (this->planeLength*32)/2*(-1.f) + j*32), this->typeChunk, dataPixels, dataPixelsCaveAC, widthHeightmap, lengthHeightMap, i*32,j*32*this->planeWidth*32, seedTerrain, this->nbChunkTerrain-(this->planeHeight-k), this);
                this->listeChunks.push_back(c);
            }
        }
    }
}

void TerrainControler::buildStructures(unsigned char* dataPixels){
    for (int j = 0 ; j < this->planeLength*CHUNK_SIZE ; j++){
        for (int i = 0 ; i < this->planeWidth*CHUNK_SIZE ; i++){
            int indInText = j*this->planeWidth*CHUNK_SIZE + i;
            // Attention à bien en compte les chunks en hauteur qui ne sont pas utilisés pour la génération du terrain
            int k = CHUNK_SIZE*(this->planeHeight-this->nbChunkTerrain) + ((int)dataPixels[indInText]) + 1;
            if (rand()%100 == 0){
                int biomeID = 0;
                if (this->biomeChart){
                    FastNoise noiseGenerator = this->mg->getNoiseGenerator();
                    float precipitation = (noiseGenerator.GetNoise((i), (j))+1.0)/2.0;
                    float humidite = (noiseGenerator.GetNoise((i + 1000), (j + 1500))+1.0)/2.0;
                    biomeID = this->getBiomeID(precipitation,humidite); // On utilise la position x et z du block pour déterminer le biome
                    
                }
                this->constructStructure(i,j,k,rand()%structures[biomeID].size(),true);
            }
        }
    }
}

void TerrainControler::constructStructure(int i, int j, int k,int idStruct,bool rand){ // rand = true si génération automatique, false si construction par le joueur
    // Peut être mettre un champ biomeID dans la classe Voxel au lieu de rechercher dans la biome chart ici
    int biomeID = 0;
    if (this->biomeChart){
        FastNoise noiseGenerator = this->mg->getNoiseGenerator();
        float precipitation = (noiseGenerator.GetNoise((i), (j))+1.0)/2.0;
        float humidite = (noiseGenerator.GetNoise((i + 1000), (j + 1500))+1.0)/2.0;
        biomeID = this->getBiomeID(precipitation,humidite); // On utilise la position x et z du block pour déterminer le biome
    }

    int indChunk = -1;
    glm::vec3 posChunk;
    Structure to_build;
    if(!rand){
        to_build = structures[3][idStruct]; // construction manuel donc on va chercher dans le vecteur 4 (indice 3)
    }else{
        to_build = structures[biomeID][idStruct]; // On construit l'une des structures disponibles 
    }

    std::vector<Voxel*> getListe;
    for (int n = 0 ; n < to_build.blocks.size() ; n++){
        int *infoBlock = to_build.blocks[n].infoBlock;
        if (!(i+infoBlock[1]<0||j+infoBlock[3]<0||k+infoBlock[2]<0||i+infoBlock[1]>=this->planeWidth*CHUNK_SIZE||j+infoBlock[3]>=this->planeLength*CHUNK_SIZE||k+infoBlock[2]>=this->planeHeight*CHUNK_SIZE)){
            int newIDChunk = ((i+infoBlock[1])/32)*this->planeHeight*this->planeLength + ((j+infoBlock[3])/32)*this->planeHeight + ((k+infoBlock[2])/32);
            if (indChunk != newIDChunk){ // On ne met à jour les informations que si c'est nécessaire
                indChunk = newIDChunk;
                posChunk = this->listeChunks[indChunk]->getPosition();
                getListe = this->listeChunks[indChunk]->getListeVoxels();
            }
            Voxel *actual_voxel = getListe[((k + infoBlock[2])%32)*1024 + ((j+infoBlock[3])%32) * 32 + ((i+infoBlock[1])%32)];
            if (actual_voxel != nullptr){ // Si un voxel du terrain existe déjà à cet endroit, on remplace son id par celui du bloc de la structure
                actual_voxel->setId(infoBlock[0]);
            }else{ // Besoin de créer un nouveau block
                Voxel *new_block = new Voxel(glm::vec3(posChunk[0]+(i+infoBlock[1])%32,posChunk[1]+(k+infoBlock[2])%32,posChunk[2]+(j+infoBlock[3])%32),infoBlock[0]); 
                getListe[((k + infoBlock[2])%32)*1024 + ((j+infoBlock[3])%32) * 32 + ((i+infoBlock[1])%32)] = new_block;
                this->listeChunks[indChunk]->setListeVoxels(getListe);
                actual_voxel=new_block;

            }
            if(!rand){
                LocalisationBlock loc;
                loc.numLongueur = i+infoBlock[1];
                loc.numHauteur = k+infoBlock[2];
                loc.numProfondeur = j+infoBlock[3];
                loc.indiceChunk = newIDChunk;
                loc.indiceVoxel = (loc.numHauteur%32)*1024 + (loc.numProfondeur%32)*32 + (loc.numLongueur%32);

                this->removeBlock(loc,actual_voxel->getRacineFaceID());
                this->addBlock(loc,actual_voxel);
            }
        }
    }
}


void TerrainControler::loadTerrain(){
    for (unsigned int i = 0 ; i < this->listeChunks.size() ; i++){
        this->listeChunks[i]->loadChunk(this);
    }
}

void TerrainControler::buildEditorChunk(){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    this->listeChunks.clear();
    for (int i = 0 ; i < this->planeWidth ; i++){
        for (int j = 0 ; j < this->planeLength ; j++){
            for (int k = 0 ; k < this->planeHeight ; k++){
                Chunk *c = new Chunk(glm::vec3((this->planeWidth*32)/2*(-1.f) + i*32, k*32, (this->planeLength*32)/2*(-1.f) + j*32), i+j+k==0);
                this->listeChunks.push_back(c);
            }
        }
    }
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
int* TerrainControler::getRefToPlaneHeight(){
    return &(this->planeHeight);
}
int* TerrainControler::getRefToSeedTerrain(){
    return &(this->seedTerrain);
}
int* TerrainControler::getRefToOctave(){
    return &(this->octave);
}
int* TerrainControler::getRefToNbChunkTerrain(){
    return &(this->nbChunkTerrain);
}
bool* TerrainControler::getRefToGenerateStructure(){
    return &(this->generateStructure);
}

bool TerrainControler::computeTargetedBlock(glm::vec3 target, int& numLongueur, int& numHauteur, int& numProfondeur, int& indiceV, int& indiceChunk){
    numLongueur = floor(target[0]) + 16*planeWidth;
    numHauteur = floor(target[1]);
    numProfondeur = floor(target[2]) + 16*planeLength;

    if (numLongueur < 0 || numLongueur > (planeWidth*32)-1 || numProfondeur < 0 || numProfondeur > (planeLength*32)-1 || numHauteur < 0 || numHauteur > (planeHeight*32)-1){
        return false; 
    }else{
        indiceV = (numHauteur%32)*1024 + (numProfondeur%32) * 32 + (numLongueur%32); // Indice du voxel que le joueur est en train de viser
        indiceChunk = (numLongueur/32) * planeLength * planeHeight + (numProfondeur/32) * planeHeight + numHauteur/32 ;
        return true;
    }
}

// http://www.cse.yorku.ca/~amana/research/grid.pdf
LocalisationBlock TerrainControler::detectTargetBlock(glm::vec3 startPoint, glm::vec3 endPoint){
    int x = static_cast<int>(std::floor(startPoint.x));
    int y = static_cast<int>(std::floor(startPoint.y));
    int z = static_cast<int>(std::floor(startPoint.z));
    int xEnd = static_cast<int>(std::floor(endPoint.x));
    int yEnd = static_cast<int>(std::floor(endPoint.y));
    int zEnd = static_cast<int>(std::floor(endPoint.z));

    float dx = endPoint.x - startPoint.x;
    float dy = endPoint.y - startPoint.y;
    float dz = endPoint.z - startPoint.z;

    int sx = (dx > 0) ? 1 : -1;
    int sy = (dy > 0) ? 1 : -1;
    int sz = (dz > 0) ? 1 : -1;

    dx = std::abs(dx);
    dy = std::abs(dy);
    dz = std::abs(dz);

    float tMaxX = (dx > 0) ? (sx > 0 ? (std::floor(startPoint.x) + 1 - startPoint.x) : (startPoint.x - std::floor(startPoint.x))) / dx : std::numeric_limits<float>::infinity();
    float tMaxY = (dy > 0) ? (sy > 0 ? (std::floor(startPoint.y) + 1 - startPoint.y) : (startPoint.y - std::floor(startPoint.y))) / dy : std::numeric_limits<float>::infinity();
    float tMaxZ = (dz > 0) ? (sz > 0 ? (std::floor(startPoint.z) + 1 - startPoint.z) : (startPoint.z - std::floor(startPoint.z))) / dz : std::numeric_limits<float>::infinity();

    float tDeltaX = (dx > 0) ? 1 / dx : std::numeric_limits<float>::infinity();
    float tDeltaY = (dy > 0) ? 1 / dy : std::numeric_limits<float>::infinity();
    float tDeltaZ = (dz > 0) ? 1 / dz : std::numeric_limits<float>::infinity();

    int numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk;
    int axisFace = 0;
    if (this->computeTargetedBlock(glm::vec3(x,y,z), numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk)){
        std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
        if (listeVoxels[indiceV] != nullptr){
            if (tMaxX < tMaxY && tMaxX < tMaxZ) {
                axisFace = (-1)*sx; // Face visé parallèle au plan YZ
            } else if (tMaxY < tMaxZ) {
                axisFace = (-2)*sy; // Face visé parallèle au plan XZ
            } else {
                axisFace = (-3)*sz; // Face visé parallèle au plan XY
            }
            return {indiceV, indiceChunk, numLongueur, numProfondeur, numHauteur, listeVoxels[indiceV]->getIdInChunk(),axisFace};
        }
    }

    while (x != xEnd || y != yEnd || z != zEnd) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                tMaxX += tDeltaX;
                x += sx;
                axisFace = (-1)*sx; // Face visé parallèle au plan YZ
            } else {
                tMaxZ += tDeltaZ;
                z += sz;
                axisFace = (-3)*sz; // Face visé parallèle au plan XY
            }
        } else {
            if (tMaxY < tMaxZ) {
                tMaxY += tDeltaY;
                y += sy;
                axisFace = (-2)*sy; // Face visé parallèle au plan XZ
            } else {
                tMaxZ += tDeltaZ;
                z += sz;
                axisFace = (-3)*sz; // Face visé parallèle au plan XY
            }
        }
        if (this->computeTargetedBlock(glm::vec3(x,y,z), numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk)){
            std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
            if (listeVoxels[indiceV] != nullptr){
                return {indiceV, indiceChunk, numLongueur, numProfondeur, numHauteur, listeVoxels[indiceV]->getIdInChunk(),axisFace};
            }
        }
    }
    return {-1,-1,-1,-1,-1,-1,0};
}

LocalisationBlock TerrainControler::tryBreakBlock(glm::vec3 camera_target, glm::vec3 camera_position){
    LocalisationBlock blockIsTargeted = detectTargetBlock(camera_position, camera_position + (float)RANGE*normalize(camera_target));

    // Ici, on vérifie si le bloc solide visé n'a pas un comportement particulier
    // Si aucun bloc n'est particulier, on pourrait retourner directement blockIsTargeted
    // Ici la bedrock est incassable
    if (blockIsTargeted.indiceVoxel != -1){
        std::vector<Voxel*> listeVoxels = this->listeChunks[blockIsTargeted.indiceChunk]->getListeVoxels();
        if (listeVoxels[blockIsTargeted.indiceVoxel] != nullptr && listeVoxels[blockIsTargeted.indiceVoxel]->getID() != 5){
            return blockIsTargeted;
        }
    }
    return {-1,-1,-1,-1,-1,-1,0};
}

std::string TerrainControler::saveModifBlocks(){
    std::stringstream res;
    for(std::unordered_map<PositionBlock,int>::iterator it = this->modifsBlock.begin(); it != modifsBlock.end(); ++it) {
        res << it->first.numLongueur << " " << it->first.numHauteur << " " << it->first.numProfondeur << " " << it->second << "\n";
    }
    return res.str();
}

void TerrainControler::applyModifBlock(std::string infoBlock){
    std::istringstream flux_infoBlock(infoBlock);
    std::string next_word;
    this->modifsBlock.clear();

    flux_infoBlock >> next_word;
    int nWidth = std::stoi(next_word);
    flux_infoBlock >> next_word;
    int nHeight = std::stoi(next_word);
    flux_infoBlock >> next_word;
    int nLength = std::stoi(next_word);
    flux_infoBlock >> next_word;
    int typeBlock = std::stoi(next_word);

    // Appliquer la modification
    int indiceVoxel = (nHeight%32)*1024 + (nLength%32) * 32 + (nWidth%32);
    int indiceChunk = (nWidth/32) * planeLength * planeHeight + (nLength/32) * planeHeight + nHeight/32 ;

    std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
    if (typeBlock == NO_BLOCK && listeVoxels[indiceVoxel]!=nullptr){
        delete listeVoxels[indiceVoxel];
        listeVoxels[indiceVoxel] = nullptr;
        this->listeChunks[indiceChunk]->setListeVoxels(listeVoxels);
    }else if (typeBlock != NO_BLOCK){
        if (listeVoxels[indiceVoxel]!=nullptr){ // Supprimer l'ancien bloc s'il existe
            delete listeVoxels[indiceVoxel];
            listeVoxels[indiceVoxel] = nullptr;
        }
        // Puis on crée le nouveau bloc
        glm::vec3 posChunk = this->listeChunks[indiceChunk]->getPosition();
        Voxel* vox = new Voxel(glm::vec3(posChunk[0]+nWidth%32,posChunk[1]+nHeight%32,posChunk[2]+nLength%32),typeBlock);
        listeVoxels[indiceVoxel] = vox;
        this->listeChunks[indiceChunk]->setListeVoxels(listeVoxels);
    }

    // Conserver la modification pour la prochaine sauvegarde
    PositionBlock pb;
    pb.numLongueur = nWidth;
    pb.numProfondeur = nLength;
    pb.numHauteur = nHeight;
    this->modifsBlock[pb]=typeBlock;
}

void TerrainControler::addBlock(LocalisationBlock lb, Voxel* newVox){
    // Code à condenser si on a le temps

    // On enlève les faces des blocs voisins (s'il existe)
    // Bloc en dessous (face supérieur)
    bool isTransparent = Chunk::transparentBlock.count(newVox->getID());
    if (lb.numHauteur>0){
        int i_chunk_bottom = (lb.numLongueur/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + (lb.numHauteur-1)/32 ;
        int i_voxel_bottom = ((lb.numHauteur-1)%32)*1024 + (lb.numProfondeur%32) * 32 + (lb.numLongueur%32);
        Voxel* v_bottom = this->listeChunks[i_chunk_bottom]->getListeVoxels()[i_voxel_bottom];
        if (v_bottom != nullptr && !isTransparent){
            this->listeChunks[i_chunk_bottom]->removeFace(v_bottom,1);
            if (i_chunk_bottom != lb.indiceChunk) this->listeChunks[i_chunk_bottom]->sendVoxelMapToShader();
        }
        if (v_bottom == nullptr || Chunk::transparentBlock.count(v_bottom->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,0);
        }
    }else{ // lb.numHauteur = 0
        this->listeChunks[lb.indiceChunk]->addFace(newVox,0);
    }

    // Bloc au dessus (face inférieur)
    if (lb.numHauteur < (planeHeight*32)-1){
        int i_chunk_top = (lb.numLongueur/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + (lb.numHauteur+1)/32 ;
        int i_voxel_top = ((lb.numHauteur+1)%32)*1024 + (lb.numProfondeur%32) * 32 + (lb.numLongueur%32);
        Voxel* v_top = this->listeChunks[i_chunk_top]->getListeVoxels()[i_voxel_top];
        if (v_top != nullptr && !isTransparent){
            this->listeChunks[i_chunk_top]->removeFace(v_top,0);
            if (i_chunk_top != lb.indiceChunk) this->listeChunks[i_chunk_top]->sendVoxelMapToShader();
        } 
        if (v_top == nullptr || Chunk::transparentBlock.count(v_top->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,1);
        }
    }else{ // lb.numHauteur = planeHeight*32 - 1
        this->listeChunks[lb.indiceChunk]->addFace(newVox,1);
    }

    // Bloc derrière (face avant)
    if (lb.numProfondeur > 0){
        int i_chunk_back = (lb.numLongueur/32) * planeLength * planeHeight + ((lb.numProfondeur-1)/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_back = ((lb.numHauteur)%32)*1024 + ((lb.numProfondeur-1)%32) * 32 + (lb.numLongueur%32);
        Voxel* v_back = this->listeChunks[i_chunk_back]->getListeVoxels()[i_voxel_back];
        if (v_back != nullptr && !isTransparent){
            this->listeChunks[i_chunk_back]->removeFace(v_back,3);
            if (i_chunk_back != lb.indiceChunk) this->listeChunks[i_chunk_back]->sendVoxelMapToShader();
        }
        if (v_back == nullptr || Chunk::transparentBlock.count(v_back->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,2);
        }
    }else{ // lb.numProfondeur = 0
        this->listeChunks[lb.indiceChunk]->addFace(newVox,2);
    }

    // Bloc devant (face arrière)
    if (lb.numProfondeur < (planeLength*32)-1){
        int i_chunk_front = (lb.numLongueur/32) * planeLength * planeHeight + ((lb.numProfondeur+1)/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_front = (lb.numHauteur%32)*1024 + ((lb.numProfondeur+1)%32) * 32 + (lb.numLongueur%32);
        Voxel* v_front = this->listeChunks[i_chunk_front]->getListeVoxels()[i_voxel_front];
        if (v_front != nullptr && !isTransparent){
            this->listeChunks[i_chunk_front]->removeFace(v_front,2);
            if (i_chunk_front != lb.indiceChunk) this->listeChunks[i_chunk_front]->sendVoxelMapToShader();
        }
        if (v_front == nullptr || Chunk::transparentBlock.count(v_front->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,3);
        }
    }else{ // lb.numProfondeur = planeLength*32-1
        this->listeChunks[lb.indiceChunk]->addFace(newVox,3);
    }


    // Bloc gauche (face droite)
    if (lb.numLongueur > 0){
        int i_chunk_left = ((lb.numLongueur-1)/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_left = ((lb.numHauteur)%32)*1024 + (lb.numProfondeur%32) * 32 + ((lb.numLongueur-1)%32);
        Voxel* v_left = this->listeChunks[i_chunk_left]->getListeVoxels()[i_voxel_left];
        if (v_left != nullptr && !isTransparent){
            this->listeChunks[i_chunk_left]->removeFace(v_left,5);
            if (i_chunk_left != lb.indiceChunk) this->listeChunks[i_chunk_left]->sendVoxelMapToShader();
        }
        if (v_left == nullptr || Chunk::transparentBlock.count(v_left->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,4);
        }
    }else{ // lb.numLongueur = 0
        this->listeChunks[lb.indiceChunk]->addFace(newVox,4);
    }

    // Bloc droite (face gauche)
    if (lb.numLongueur < (planeWidth*32)-1){
        int i_chunk_right = ((lb.numLongueur+1)/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_right = ((lb.numHauteur)%32)*1024 + (lb.numProfondeur%32) * 32 + ((lb.numLongueur+1)%32);
        Voxel* v_right = this->listeChunks[i_chunk_right]->getListeVoxels()[i_voxel_right];
        if (v_right != nullptr && !isTransparent){
            this->listeChunks[i_chunk_right]->removeFace(v_right,4);
            if (i_chunk_right != lb.indiceChunk) this->listeChunks[i_chunk_right]->sendVoxelMapToShader();
        }
        if (v_right == nullptr || Chunk::transparentBlock.count(v_right->getID())){
            this->listeChunks[lb.indiceChunk]->addFace(newVox,5);
        }
    }else{ // lb.numLongueur = planeWidth*32-1
        this->listeChunks[lb.indiceChunk]->addFace(newVox,5);
    }

    this->listeChunks[lb.indiceChunk]->sendVoxelMapToShader();
}


void TerrainControler::removeBlock(LocalisationBlock lb, std::string racine_id){
    this->listeChunks[lb.indiceChunk]->removeFaces(racine_id); // On enlève les faces du bloc cassé
    // On ajoute les faces des blocs voisins (s'il existe)
    // Bloc en dessous (face supérieur)*
    if (lb.numHauteur>0){
        int i_chunk_bottom = (lb.numLongueur/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + (lb.numHauteur-1)/32 ;
        int i_voxel_bottom = ((lb.numHauteur-1)%32)*1024 + (lb.numProfondeur%32) * 32 + (lb.numLongueur%32);
        Voxel* v_bottom = this->listeChunks[i_chunk_bottom]->getListeVoxels()[i_voxel_bottom];
        if (v_bottom != nullptr){
            this->listeChunks[i_chunk_bottom]->addFace(v_bottom,1);
            if (i_chunk_bottom != lb.indiceChunk) this->listeChunks[i_chunk_bottom]->sendVoxelMapToShader();
        }
    }
    // Bloc au dessus (face inférieur)
    if (lb.numHauteur < (planeHeight*32)-1){
        int i_chunk_top = (lb.numLongueur/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + (lb.numHauteur+1)/32 ;
        int i_voxel_top = ((lb.numHauteur+1)%32)*1024 + (lb.numProfondeur%32) * 32 + (lb.numLongueur%32);
        Voxel* v_top = this->listeChunks[i_chunk_top]->getListeVoxels()[i_voxel_top];
        if (v_top != nullptr){
            this->listeChunks[i_chunk_top]->addFace(v_top,0);
            if (i_chunk_top != lb.indiceChunk) this->listeChunks[i_chunk_top]->sendVoxelMapToShader();
        }
    }
    // Bloc derrière (face avant)
    if (lb.numProfondeur > 0){
        int i_chunk_back = (lb.numLongueur/32) * planeLength * planeHeight + ((lb.numProfondeur-1)/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_back = ((lb.numHauteur)%32)*1024 + ((lb.numProfondeur-1)%32) * 32 + (lb.numLongueur%32);
        Voxel* v_back = this->listeChunks[i_chunk_back]->getListeVoxels()[i_voxel_back];
        if (v_back != nullptr){
            this->listeChunks[i_chunk_back]->addFace(v_back,3);
            if (i_chunk_back != lb.indiceChunk) this->listeChunks[i_chunk_back]->sendVoxelMapToShader();
        }
    }
    // Bloc devant (face arrière)
    if (lb.numProfondeur < (planeLength*32)-1){
        int i_chunk_front = (lb.numLongueur/32) * planeLength * planeHeight + ((lb.numProfondeur+1)/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_front = (lb.numHauteur%32)*1024 + ((lb.numProfondeur+1)%32) * 32 + (lb.numLongueur%32);
        Voxel* v_front = this->listeChunks[i_chunk_front]->getListeVoxels()[i_voxel_front];
        if (v_front != nullptr){
            this->listeChunks[i_chunk_front]->addFace(v_front,2);
            if (i_chunk_front != lb.indiceChunk) this->listeChunks[i_chunk_front]->sendVoxelMapToShader();
        }
    }
    // Bloc gauche (face droite)
    if (lb.numLongueur > 0){
        int i_chunk_left = ((lb.numLongueur-1)/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_left = ((lb.numHauteur)%32)*1024 + (lb.numProfondeur%32) * 32 + ((lb.numLongueur-1)%32);
        Voxel* v_left = this->listeChunks[i_chunk_left]->getListeVoxels()[i_voxel_left];
        if (v_left != nullptr){
            this->listeChunks[i_chunk_left]->addFace(v_left,5);
            if (i_chunk_left != lb.indiceChunk) this->listeChunks[i_chunk_left]->sendVoxelMapToShader();
        }
    }
    // Bloc droite (face gauche)
    if (lb.numLongueur < (planeWidth*32)-1){
        int i_chunk_right = ((lb.numLongueur+1)/32) * planeLength * planeHeight + (lb.numProfondeur/32) * planeHeight + lb.numHauteur/32 ;
        int i_voxel_right = ((lb.numHauteur)%32)*1024 + (lb.numProfondeur%32) * 32 + ((lb.numLongueur+1)%32);
        Voxel* v_right = this->listeChunks[i_chunk_right]->getListeVoxels()[i_voxel_right];
        if (v_right != nullptr){
            this->listeChunks[i_chunk_right]->addFace(v_right,4);
            if (i_chunk_right != lb.indiceChunk) this->listeChunks[i_chunk_right]->sendVoxelMapToShader();
        }
    }
    this->listeChunks[lb.indiceChunk]->sendVoxelMapToShader();
}

void TerrainControler::breakBlock(LocalisationBlock lb){ // Il faut déjà avoir testé (au minimum) si lb.indiceVoxel != -1 avant d'appeler cette fonction
    std::vector<Voxel*> listeVoxels = this->listeChunks[lb.indiceChunk]->getListeVoxels();
    Voxel* voxel_deleted = listeVoxels[lb.indiceVoxel];
    this->removeBlock(lb, voxel_deleted->getRacineFaceID());
    delete voxel_deleted; // Ne pas oublier de bien libérer la mémoire
    listeVoxels[lb.indiceVoxel] = nullptr;
    this->listeChunks[lb.indiceChunk]->setListeVoxels(listeVoxels);

    // On enregistre la modification pour la sauvegarde
    PositionBlock pb;
    pb.numLongueur = lb.numLongueur;
    pb.numProfondeur = lb.numProfondeur;
    pb.numHauteur = lb.numHauteur;
    this->modifsBlock[pb]=NO_BLOCK;
}

bool TerrainControler::tryCreateBlock(glm::vec3 camera_target, glm::vec3 camera_position, int typeBlock){
    LocalisationBlock blockIsTargeted = detectTargetBlock(camera_position, camera_position + (float)RANGE*normalize(camera_target));
    if (blockIsTargeted.indiceVoxel != -1){ // Vérifie si un bloc est bien visé
        // On pose un bloc à droite du bloc visé
        LocalisationBlock newBlock = blockIsTargeted;
        int val = blockIsTargeted.targetedFace;
        if (abs(val) == 1){
            newBlock.numLongueur += val > 0 ? 1 : -1;
        }else if (abs(val) == 2){
            newBlock.numHauteur += val > 0 ? 1 : -1;
        }else if (abs(val) == 3){
            newBlock.numProfondeur += val > 0 ? 1 : -1;
        }

        if (!(newBlock.numLongueur < 0 || newBlock.numLongueur > (planeWidth*32)-1 || newBlock.numHauteur < 0 || newBlock.numHauteur > (planeHeight*32)-1 || newBlock.numProfondeur < 0 || newBlock.numProfondeur > (planeLength*32)-1)){
            // Le chunk du nouveau bloc peut être différent de celui visé, donc on est obligé de recalculer
            newBlock.indiceVoxel = (newBlock.numHauteur%32)*1024 + (newBlock.numProfondeur%32) * 32 + (newBlock.numLongueur%32);
            newBlock.indiceChunk = (newBlock.numLongueur/32) * planeLength * planeHeight + (newBlock.numProfondeur/32) * planeHeight + newBlock.numHauteur/32 ;

            std::vector<Voxel*> listeVoxels = this->listeChunks[newBlock.indiceChunk]->getListeVoxels();
            if (listeVoxels[newBlock.indiceVoxel] == nullptr){
                glm::vec3 posChunk = this->listeChunks[newBlock.indiceChunk]->getPosition();
                Voxel* vox = new Voxel(glm::vec3(posChunk[0]+newBlock.numLongueur%32,posChunk[1]+newBlock.numHauteur%32,posChunk[2]+newBlock.numProfondeur%32),typeBlock);
                listeVoxels[newBlock.indiceVoxel] = vox;

                this->listeChunks[newBlock.indiceChunk]->setListeVoxels(listeVoxels);
                this->addBlock(newBlock,vox);
                //this->listeChunks[newBlock.indiceChunk]->loadChunk(this);
                // On enregistre la modification pour la sauvegarde
                PositionBlock pb;
                pb.numLongueur = newBlock.numLongueur;
                pb.numProfondeur = newBlock.numProfondeur;
                pb.numHauteur = newBlock.numHauteur;
                this->modifsBlock[pb]=typeBlock;

                
                return true;
            }
        }
    }
    return false;
}

bool TerrainControler::tryCreatorCreateBlock(glm::vec3 camera_target, glm::vec3 camera_position, int typeBlock){
    glm::vec3 originPoint = camera_position;
    glm::vec3 direction = normalize(camera_target);
    float k = creationDistance; // Pour l'instant le joueur ne peut poser un block qu'à cette distance
    glm::vec3 target = originPoint + (float)k*direction;
    int numLongueur = floor(target[0]) + 16*this->planeWidth;
    int numHauteur = floor(target[1]);      
    int numProfondeur = floor(target[2]) + 16*this->planeLength;
    bool surface=true;
    if (numLongueur < 0 || numLongueur > (this->planeWidth*32)-1 || numProfondeur < 0 || numProfondeur > (this->planeLength*32)-1 || numHauteur < 0 || numHauteur > (this->planeHeight*32)-1){
        return false;
    }else{
        int indiceV = (numHauteur%32)*1024 + (numProfondeur%32) * 32 + (numLongueur%32); // Indice du voxel que le joueur est en train de viser
        int indiceChunk = (numLongueur/32) * planeLength * planeHeight + (numProfondeur/32) * planeHeight + numHauteur/32 ;
        
        std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();

        if(cubeTool){
            for(int n = -(radius)+1; n<radius;n++){
                for(int j = -(radius)+1; j<radius;j++){
                    for(int m = -(radius)+1; m<radius; m++){
                        if (!(numLongueur+n < 0 || numLongueur+n > (this->planeWidth*32)-1 || numProfondeur+j < 0 || numProfondeur+j > (this->planeLength*32)-1 || numHauteur+m < 0 || numHauteur+m > (this->planeHeight*32)-1)){
                            int i_chunk = (numLongueur+n)/32 * planeLength * planeHeight + (numProfondeur+j)/32 * planeHeight + (numHauteur+m)/32 ;
                            if (i_chunk == indiceChunk){
                                int indiceTotal = indiceV + m*1024 + j*32 + n;

                                if (listeVoxels[indiceTotal] == nullptr){
                                    glm::vec3 posChunk = this->listeChunks[indiceChunk]->getPosition();
                                    glm::vec3 posBlock = glm::vec3(posChunk[0]+(numLongueur+n)%32,posChunk[1]+(numHauteur+m)%32,posChunk[2]+(numProfondeur+j)%32);
                                    Voxel* vox = new Voxel(posBlock,typeBlock);
                        
                                    listeVoxels[indiceTotal] = vox;

                                    //gère la lumière
                                    //int luminosityBlock = vox->getLuminosity();

                                    // printf("hauteur de notre block : %f\n",posBlock.y);

                                    // printf("taille = %d\n",listeVoxels.size());

                                    
                                    /*
                                    int hauteurBlocDessous = -1000;
                                    for(double y2 = 1.0; y2<17.0+listeVoxels[indiceTotal]->getBackBottomLeftCorner().y; y2+=1 ){
                                        //printf("y2 = %f et max hauteur %f\n",y2,17.0+listeVoxels[indiceTotal]->getBackBottomLeftCorner().y);
                                        if(listeVoxels[indiceTotal - y2 * 1024]!=nullptr){
                                            hauteurBlocDessous=y2;
                                            break;
                                        }
                                    }
                                    int maxLuminosity = 0;

                                    for(double y = 1.0;y<16.0-posBlock.y;y+=1){
                                        if(listeVoxels[indiceTotal + y * 1024]!=nullptr){
                                            surface=false;
                                        }
                                    }
                                    if(surface==true){ //si on est a la surface
                                        // printf("il n'y a pas de bloc au dessus\n");
                                        // listeVoxels[indiceTotal]->setLuminosity(16); LUMINOSITY
                                    }else{
                                        
                                    }
                                    */

                                    /* LUMINOSITY
                                    if(hauteurBlocDessous!=-1000 && listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]!=nullptr){ // si il y'a bien un bloc en dessous
                                        //updateLight(listeVoxels,indiceTotal,hauteurBlocDessous);
                                        listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(0);
                                        // for(int i=-1;i<2;i++){
                                        //     for(int j=-1;j<2;j++){
                                        //         //for(int k=-1;k<2;k++){
                                        //             //int idBlocAdjacent = (numHauteur+k) *1024 + (numProfondeur%32+i) * 32 + (numLongueur%32+j); 
                                        //             int idBlocAdjacent = indiceTotal - hauteurBlocDessous*1024 + i*32 + j;
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
                                                
                                                //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(3);
                                                if(listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i]!=nullptr){
                                                    updateLight(listeVoxels,indiceTotal - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i,posBlock + glm::vec3(i,0,j));
                                                }
                                            }
                                        }
                                        //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(16);
                                        //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(maxLuminosity);
                            
                                    }
                                    */
                            //on a trouver le bloc en dessous
                            //printf("id block dessous = %d\n",hauteurBlocDessous);
                                }
                            }
                        }
                    }
                }
            }
        }else if(sphereTool){
            for(int n = -radius; n<radius+1;n++){
                for(int j = -radius; j<radius+1;j++){
                    for(int m = -radius; m<radius+1; m++){
                        if (!(numLongueur+n < 0 || numLongueur+n > (this->planeWidth*32)-1 || numProfondeur+j < 0 || numProfondeur+j > (this->planeLength*32)-1 || numHauteur+m < 0 || numHauteur+m > (this->planeHeight*32)-1)){
                            int i_chunk = (numLongueur+n)/32 * planeLength * planeHeight + (numProfondeur+j)/32 * planeHeight + (numHauteur+m)/32 ;
                            if (i_chunk == indiceChunk){
                                int indiceTotal = indiceV + m*1024 + j*32 + n;

                                if (listeVoxels[indiceTotal] == nullptr && (n*n+j*j+m*m)<(radius*radius)){
                                    glm::vec3 posChunk = this->listeChunks[indiceChunk]->getPosition();
                                    glm::vec3 posBlock = glm::vec3(posChunk[0]+(numLongueur+n)%32,posChunk[1]+(numHauteur+m)%32,posChunk[2]+(numProfondeur+j)%32);
                                    Voxel* vox = new Voxel(posBlock,typeBlock);
                        
                                    listeVoxels[indiceTotal] = vox;

                                /*
                                    //gère la lumière
                                //int luminosityBlock = vox->getLuminosity();

                                // printf("hauteur de notre block : %f\n",posBlock.y);

                                // printf("taille = %d\n",listeVoxels.size());

                                

                                int hauteurBlocDessous = -1000;
                                for(double y2 = 1.0; y2<17.0+listeVoxels[indiceTotal]->getBackBottomLeftCorner().y; y2+=1 ){
                                    //printf("y2 = %f et max hauteur %f\n",y2,17.0+listeVoxels[indiceTotal]->getBackBottomLeftCorner().y);
                                    if(listeVoxels[indiceTotal - y2 * 1024]!=nullptr){
                                        hauteurBlocDessous=y2;
                                        break;
                                    }
                                }
                                int maxLuminosity = 0;

                                for(double y = 1.0;y<16.0-posBlock.y;y+=1){
                                    if(listeVoxels[indiceTotal + y * 1024]!=nullptr){
                                        surface=false;
                                    }
                                }
                                if(surface==true){ //si on est a la surface
                                    // printf("il n'y a pas de bloc au dessus\n");
                                    //listeVoxels[indiceTotal]->setLuminosity(16);
                                }else{
                                    
                                }

                                if(hauteurBlocDessous!=-1000 && listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]!=nullptr){ // si il y'a bien un bloc en dessous
                                    //updateLight(listeVoxels,indiceTotal,hauteurBlocDessous);
                                    listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(0);
                                    // for(int i=-1;i<2;i++){
                                    //     for(int j=-1;j<2;j++){
                                    //         //for(int k=-1;k<2;k++){
                                    //             //int idBlocAdjacent = (numHauteur+k) *1024 + (numProfondeur%32+i) * 32 + (numLongueur%32+j); 
                                    //             int idBlocAdjacent = indiceTotal - hauteurBlocDessous*1024 + i*32 + j;
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
                                            
                                            //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(3);
                                            if(listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i]!=nullptr){
                                                updateLight(listeVoxels,indiceTotal - std::abs((hauteurBlocDessous)) * 1024 + j * 32 + i,posBlock + glm::vec3(i,0,j));
                                            }
                                        }
                                    }
                                    //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(16);
                                    //listeVoxels[indiceTotal - std::abs((hauteurBlocDessous)) * 1024 ]->setLuminosity(maxLuminosity);
                                }
                                */

                                }
                            }
                        }
                    }
                }
            }
        }
        
        this->listeChunks[indiceChunk]->setListeVoxels(listeVoxels);
        this->listeChunks[indiceChunk]->loadChunk(this);
        
        return true;
    }

    return false;
}

void TerrainControler::drawTerrain(){
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        this->listeChunks[i]->drawChunk();
    }
}

void TerrainControler::saveStructure(std::string filePath){
    filePath = "../Structures/" + filePath + ".txt";
    std::ofstream fileStructure(filePath);

    if (!fileStructure.is_open()) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier. Vérifiez le nom donné au fichier\n";
    }

    for (int n_chunk_height = 0 ; n_chunk_height < this->planeHeight ; n_chunk_height++){
        for (int n_chunk_length = 0 ; n_chunk_length < this->planeLength ; n_chunk_length++){
            for (int n_chunk_width = 0 ; n_chunk_width < this->planeWidth ; n_chunk_width++){
                std::vector<Voxel*> voxelsToSave = this->listeChunks[n_chunk_width*this->planeHeight*this->planeLength + n_chunk_length*this->planeHeight + n_chunk_height]->getListeVoxels();
                for (int i = 0 ; i < voxelsToSave.size() ; i++){
                    Voxel *v = voxelsToSave[i];
                    if (v != nullptr){
                        int dec_x = -16 + i%32 + n_chunk_width*CHUNK_SIZE; // Décalage en x
                        int dec_z = -16 + (i/32)%32 + n_chunk_height*CHUNK_SIZE; // Décalage en y
                        int dec_y = -16 + i/(32*32) + n_chunk_length*CHUNK_SIZE; // Décalage en z
                        // Attention à bien mettre un espace à la fin, avant le retour à la ligne
                        fileStructure << v->getID() << " " << dec_x << " " << dec_y << " " << dec_z << " \n";
                    }
                }
            }
        }
    }

    fileStructure.close();
}

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

bool TerrainControler::checkHoldRightClick(glm::vec3 camera_position, glm::vec3 camera_target, float deltaTime, bool modeJeu,int handBlock, GLuint programID){
    if (this->mouseRightClickHold){
        bool created;
        if (creatorMod){
            created = this->tryCreatorCreateBlock(camera_target, camera_position, handBlock);
        }else{
            created = this->tryCreateBlock(camera_target, camera_position, handBlock);
        }
        return true;
    }
    return false;
}

void TerrainControler::setMouseLeftClickHold(bool mouseLeftClickHold){
    this->mouseLeftClickHold = mouseLeftClickHold;
}

bool TerrainControler::getMouseLeftClickHold(){
    return mouseLeftClickHold;
}
void TerrainControler::setMouseRightClickHold(bool mouseRightClickHold){
    this->mouseRightClickHold = mouseRightClickHold;
}
bool TerrainControler::getMouseRightClickHold(){
    return mouseRightClickHold;
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

Chunk* TerrainControler::getChunkAt(int pos_i, int pos_k, int pos_j){
    if (pos_i < 0 || pos_k < 0 || pos_j < 0 || pos_i >= this->planeWidth || pos_k >= this->planeHeight || pos_j >= this->planeLength){
        //std::cout << "Chunk hors limite : i = " << pos_i << ", j = " << pos_j << ", k = " << pos_k << "\n";
        return nullptr;
    }
    return this->listeChunks[pos_i * (this->planeHeight * this->planeLength) + pos_j * this->planeHeight + pos_k];
}


CelluleBiome TerrainControler::getCellBiomeFromBlock(CelluleBiome currentCell, float precipitation, float humidite){
    if (currentCell.isDivide){
        for (unsigned int i = 0 ; i < currentCell.cs.size() ; i++){
            if (precipitation >= currentCell.cs[i].x_data[0] && precipitation <= currentCell.cs[i].x_data[2] && humidite >= currentCell.cs[i].y_data[0] && humidite <= currentCell.cs[i].y_data[1]){
                return this->getCellBiomeFromBlock(currentCell.cs[i], precipitation, humidite);
            }
        }
    }
    return currentCell;
}

int TerrainControler::getBiomeID(float precipitation, float humidite){
    return this->getCellBiomeFromBlock(this->racineBiomeChart, precipitation, humidite).typeBiome;
}

void TerrainControler::setBiomeChart(CelluleBiome racineBiomeChart){
    this->racineBiomeChart = racineBiomeChart;
    this->biomeChart = true;
}

void TerrainControler::setUseBiomeChart(bool biomeChart){
    this->biomeChart = biomeChart;
}

bool TerrainControler::hasBiomeChart(){
    return this->biomeChart;
}