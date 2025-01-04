#include <TerrainControler.hpp>

std::vector<std::vector<Structure>> TerrainControler::structures; // Permet d'éviter les erreurs de lien à la compilation

TerrainControler::TerrainControler(int planeWidth, int planeLength, int planeHeight, int typeChunk, int seedTerrain, int octave, std::vector<std::vector<std::string>> nomStructure){
    this->planeWidth = planeWidth;
    this->planeLength = planeLength; 
    this->planeHeight = planeHeight;
    this->typeChunk = typeChunk;
    this->seedTerrain = seedTerrain;
    this->octave = octave;
    this->accumulateurDestructionBlock = 0.0f;
    this->mouseLeftClickHold = false;
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
                this->constructStructure(i,j,k);
            }
        }
    }
}

void TerrainControler::constructStructure(int i, int j, int k){
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
    Structure to_build = structures[biomeID][rand()%structures[biomeID].size()]; // On construit l'une des structures disponibles 
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
    int planeWidth = this->getPlaneWidth();
    int planeLength = this->getPlaneLength();
    int planeHeight = this->getPlaneHeight();

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
std::vector<glm::vec3> TerrainControler::detectTargetBlock(glm::vec3 startPoint, glm::vec3 endPoint){
    std::vector<glm::vec3> blocks;

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

    blocks.push_back(glm::vec3(x, y, z));
    while (x != xEnd || y != yEnd || z != zEnd) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                tMaxX += tDeltaX;
                x += sx;
            } else {
                tMaxZ += tDeltaZ;
                z += sz;
            }
        } else {
            if (tMaxY < tMaxZ) {
                tMaxY += tDeltaY;
                y += sy;
            } else {
                tMaxZ += tDeltaZ;
                z += sz;
            }
        }
        blocks.push_back(glm::vec3(x, y, z));
    }

    return blocks;
}

LocalisationBlock TerrainControler::tryBreakBlock(glm::vec3 camera_target, glm::vec3 camera_position){
    std::vector<glm::vec3> targetedBlocks = detectTargetBlock(camera_position, camera_position + (float)RANGE*normalize(camera_target));

    for (int i = 0 ; i < targetedBlocks.size() ; i++){
        int numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk;
        bool blockIsTargeted = this->computeTargetedBlock(targetedBlocks[i],numLongueur,numHauteur,numProfondeur,indiceV,indiceChunk);
        if (blockIsTargeted){
            std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
            if (listeVoxels[indiceV] != nullptr && listeVoxels[indiceV]->getID() != 5){ // Bloc de bedrock est incassable
                return {indiceV, indiceChunk, numLongueur, numProfondeur, numHauteur, listeVoxels[indiceV]->getIdInChunk()};
            }
        }
    }
    return {-1,-1,-1,-1,-1,-1};
}

void TerrainControler::breakBlock(LocalisationBlock lb){ // Il faut déjà avoir testé (au minimum) si lb.indiceVoxel != -1 avant d'appeler cette fonction
    std::vector<Voxel*> listeVoxels = this->listeChunks[lb.indiceChunk]->getListeVoxels();
    delete listeVoxels[lb.indiceVoxel]; // Ne pas oublier de bien libérer la mémoire
    listeVoxels[lb.indiceVoxel] = nullptr;

    this->listeChunks[lb.indiceChunk]->setListeVoxels(listeVoxels);
    this->listeChunks[lb.indiceChunk]->loadChunk(this);
}

bool TerrainControler::tryCreateBlock(glm::vec3 camera_target, glm::vec3 camera_position, int typeBlock){
    glm::vec3 originPoint = camera_position;
    glm::vec3 direction = normalize(camera_target);
    float k = 3.0; // Pour l'instant le joueur ne peut poser un block qu'à cette distance
    glm::vec3 target = originPoint + (float)k*direction;

    int numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk;
    std::vector<Voxel*> listeVoxels;
    bool blockIsTargeted = this->computeTargetedBlock(target,numLongueur,numHauteur,numProfondeur,indiceV,indiceChunk);

    if (!blockIsTargeted){
        return false;
    }else{
        std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
        if (listeVoxels[indiceV] == nullptr){
            glm::vec3 posChunk = this->listeChunks[indiceChunk]->getPosition();
            Voxel* vox = new Voxel(glm::vec3(posChunk[0]+numLongueur%32,posChunk[1]+numHauteur%32,posChunk[2]+numProfondeur%32),typeBlock);
            listeVoxels[indiceV] = vox;

            this->listeChunks[indiceChunk]->setListeVoxels(listeVoxels);
            this->listeChunks[indiceChunk]->loadChunk(this);

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
                        int dec_y = -16 + (i/32)%32 + n_chunk_height*CHUNK_SIZE; // Décalage en y
                        int dec_z = -16 + i/(32*32) + n_chunk_length*CHUNK_SIZE; // Décalage en z
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
