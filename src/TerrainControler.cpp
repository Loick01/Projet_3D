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
    int widthHeightmap, lengthHeightMap, channels;
    unsigned char* dataPixels = stbi_load("../Textures/terrain.png", &widthHeightmap, &lengthHeightMap, &channels, 1);
    this->nbChunkTerrain = 1;
    this->buildPlanChunks(dataPixels, widthHeightmap, lengthHeightMap);
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

void TerrainControler::buildPlanChunks(unsigned char* dataPixels, int widthHeightmap, int lengthHeightMap){
    FastNoise ng = this->mg->getNoiseGenerator();
    
    for (int i = 0 ; i < this->listeChunks.size() ; i++){
        delete this->listeChunks[i];
    }
    this->listeChunks.clear();
    for (int i = 0 ; i < this->planeWidth ; i++){
        for (int j = 0 ; j < this->planeLength ; j++){
            for (int k = 0 ; k < this->planeHeight ; k++){
                bool extreme = (i*j*k == 0) || (j == this->planeLength-1) || (i == this->planeWidth-1);
                Chunk *c = new Chunk(i, j, k, ng, extreme, k >= this->planeHeight-this->nbChunkTerrain, glm::vec3((this->planeWidth*32)/2*(-1.f) + i*32, k*32, (this->planeLength*32)/2*(-1.f) + j*32), this->typeChunk, dataPixels, widthHeightmap, lengthHeightMap, i*32,j*32*this->planeWidth*32, seedTerrain, this->nbChunkTerrain-(this->planeHeight-k));
                this->listeChunks.push_back(c);
            }
        }
    }
    srand(time(NULL));

    this->loadTerrain(); // On a besoin que tous les chunks soient générés avant de charger le terrain pour la première fois
}

void TerrainControler::loadTerrain(){
    for (unsigned int i = 0 ; i < this->listeChunks.size() ; i++){
        this->listeChunks[i]->loadChunk(this);
    }
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

LocalisationBlock TerrainControler::tryBreakBlock(glm::vec3 camera_target, glm::vec3 camera_position){
    glm::vec3 originPoint = camera_position;
    glm::vec3 direction = normalize(camera_target);

    //for (int k = 1 ; k < RANGE+1 ; k++){ // Trouver une meilleure manière pour détecter le bloc à casser
    for (float k = 0.1 ; k < RANGE+1. ; k+=0.1){ // C'est mieux mais pas parfait
        glm::vec3 target = originPoint + (float)k*direction;

        int numLongueur, numHauteur, numProfondeur, indiceV, indiceChunk;
        std::vector<Voxel*> listeVoxels;
        bool blockIsTargeted = this->computeTargetedBlock(target,numLongueur,numHauteur,numProfondeur,indiceV,indiceChunk);

        if (!blockIsTargeted){
            continue;
        }else{
            std::vector<Voxel*> listeVoxels = this->listeChunks[indiceChunk]->getListeVoxels();
            if (listeVoxels[indiceV] == nullptr){
                continue;
            }else if (listeVoxels[indiceV]->getID() != 5){ // Le bloc de bedrock est incassable (donc attention si on en place un)
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

Chunk* TerrainControler::getChunkAt(int pos_i, int pos_k, int pos_j){
    if (pos_i < 0 || pos_k < 0 || pos_j < 0 || pos_i >= this->planeWidth || pos_k >= this->planeHeight || pos_j >= this->planeLength){
        //std::cout << "Chunk hors limite : i = " << pos_i << ", j = " << pos_j << ", k = " << pos_k << "\n";
        return nullptr;
    }
    return this->listeChunks[pos_i * (this->planeHeight * this->planeLength) + pos_j * this->planeHeight + pos_k];
}
