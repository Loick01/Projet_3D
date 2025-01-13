#include <ParamsWindow.hpp>

char ParamsWindow::nameStructure[512]; // Permet d'éviter les erreurs de lien à la compilation
int ParamsWindow::widthScreen = 0;
int ParamsWindow::heightScreen = 0;
std::string currentStruct =" ";

ParamsWindow::ParamsWindow(GLFWwindow* window, int style, TerrainControler *terrainControler, Player *player){
    this->style = style;
    this->renduFilaire = false;
    this->terrainControler = terrainControler;
    this->mg = terrainControler->getMapGenerator();
    this->inEditor = false;
    this->clearEntity = false;
    this->use_terrain_spline = false;
    this->use_cave_spline = false;
    this->useBiomeChart = false;
    this->speedPlayer = player->getRefToSpeed();
    this->posJoueur = player->getHitbox()->getRefToBottomPoint();
    this->planeWidth = terrainControler->getRefToPlaneWidth();
    this->planeLength = terrainControler->getRefToPlaneLength();
    this->planeHeight = terrainControler->getRefToPlaneHeight();
    this->seedTerrain = terrainControler->getRefToSeedTerrain();
    this->octave = terrainControler->getRefToOctave();
    this->generateStructure = terrainControler->getRefToGenerateStructure();
    this->hitboxPlayer = player->getHitbox();
    this->init(window);
    this->useStyle();

    this->resetContinentalnessPlot();
    this->resetCavePlot();

    this->nbChunkTerrain = terrainControler->getRefToNbChunkTerrain();

    this->resetBiomeChart();

    this->terrainControler->setBiomeChart(this->racineBiomeChart); // Biome Chart par défaut
    this->showBuilderWindow = true;
    this->newStructure = false;

    const std::string directoryPath = "../Structures";
    
    // Ouvrir le répertoire
    DIR* dir = opendir(directoryPath.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Vérifie si c'est un fichier régulier (pas un répertoire)
            if (entry->d_type == DT_REG) {
                std::string fileName = entry->d_name;

                // Vérifie l'extension .txt
                if (fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".txt") {
                    this->txtFiles.push_back(fileName);
                }
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Impossible d'ouvrir le répertoire : " << directoryPath << std::endl;
    }

    // Trie les fichiers par ordre alphabétique
    std::sort(this->txtFiles.begin(), this->txtFiles.end());

    for (const auto& fileName : this->txtFiles) {
        this->buttonStates.push_back(false);
        GLuint id = loadTexture2DFromFilePath("../Structures/" + fileName.substr(0, fileName.size() - 4) + ".png");
        if (!id) {
            std::cerr << "Erreur de chargement de la texture pour " << fileName << std::endl;
        }
        this->textureIDs.push_back(id);
    }


}

ParamsWindow::~ParamsWindow(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ImPlot::DestroyContext();
    // Les champs qui sont dans cette classe sont en grande partie des pointeurs vers des champs
    // d'autres classes, ils seront supprimés dans leurs classes respectives
    // Il n'y a donc rien d'autre à delete ici
}

void ParamsWindow::useStyle(){
    if( this->style == 0){
        ImGui::StyleColorsDark();
    }else if (this->style == 1){
        ImGui::StyleColorsLight();
    }else{
        ImGui::StyleColorsClassic();
    }
}

void ParamsWindow::init(GLFWwindow* window){
    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    glfwGetFramebufferSize(window, &ParamsWindow::widthScreen, &ParamsWindow::heightScreen);
    ImPlot::CreateContext();
}

bool ParamsWindow::getInEditor(){
    return this->inEditor;
}

void ParamsWindow::saveScreenshot(const std::string& filename, int width, int height, int captureWidth, int captureHeight)
{
    // Calculer la position du centre pour capturer une zone spécifique
    int xStart = (width - captureWidth) / 2;  // Début du rectangle horizontal
    int yStart = (height - captureHeight) / 2; // Début du rectangle vertical

    // Allouer un tableau pour stocker les pixels de l'image
    unsigned char* pixels = new unsigned char[3 * captureWidth * captureHeight];  // 3 canaux (RGB)

    // Lire les pixels du framebuffer spécifié
    glReadPixels(xStart, yStart, captureWidth, captureHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Inverser l'image (les pixels sont capturés de bas en haut, donc on inverse)
    for (int y = 0; y < captureHeight / 2; ++y)
    {
        for (int x = 0; x < captureWidth; ++x)
        {
            int topIndex = (y * captureWidth + x) * 3;
            int bottomIndex = ((captureHeight - y - 1) * captureWidth + x) * 3;

            // Échanger les pixels
            std::swap(pixels[topIndex], pixels[bottomIndex]);
            std::swap(pixels[topIndex + 1], pixels[bottomIndex + 1]);
            std::swap(pixels[topIndex + 2], pixels[bottomIndex + 2]);
        }
    }

    // Enregistrer l'image en PNG à l'emplacement spécifié
    if (stbi_write_png(filename.c_str(), captureWidth, captureHeight, 3, pixels, captureWidth * 3) == 0)
    {
        std::cerr << "Erreur lors de l'enregistrement de l'image !" << std::endl;
    }

    // Libérer la mémoire allouée pour les pixels
    delete[] pixels;
}

bool ParamsWindow::getClearEntity(){
	return this->clearEntity;
}

void ParamsWindow::resetClearEntity(){
	this->clearEntity = false;
}

void ParamsWindow::openBuilderTools()
{
    if (showBuilderWindow)
    {
        // Vecteur pour stocker les noms des fichiers .txt
        if (this->newStructure){
            this->newStructure=false;
            txtFiles.push_back(nameNewStructure + ".txt");
            textureIDs.push_back(loadTexture2DFromFilePath("../Structures/" + nameNewStructure + ".png"));
            buttonStates.push_back(false);
        }
        ImGui::SetNextWindowSize(ImVec2(NB_COLONNE_ICONE*STRUCTURE_ICONE_SIZE + (NB_COLONNE_ICONE-1) * ImGui::GetStyle().ItemSpacing.x, 800), ImGuiCond_Always);
        ImGui::Begin("ToolWindow");
        ImGui::Columns(NB_COLONNE_ICONE, nullptr, false);
        for (int i = 0 ; i < NB_COLONNE_ICONE ; i++){
            ImGui::SetColumnWidth(i, STRUCTURE_ICONE_SIZE); 
        }
        for(int i=0;i<textureIDs.size();i++){
            
            ImGui::Image((ImTextureID)(intptr_t)textureIDs[i], ImVec2(STRUCTURE_ICONE_SIZE, STRUCTURE_ICONE_SIZE));
            ImGui::Spacing();
            std::string buttonLabel = txtFiles[i].substr(0, txtFiles[i].size() - 4);
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(STRUCTURE_ICONE_SIZE, 32))) {
                buttonStates[i] = !buttonStates[i]; // alterne l'état
                buttonChecked=i;
                currentStruct=buttonLabel;
            }
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        
        // Si aucun fichier trouvé
        if (this->txtFiles.empty()) {
            ImGui::NewLine();
            ImGui::Text("Aucun fichier .txt trouvé dans ../Structures.");
        }

        if (currentStruct!=" ") {
            ImGui::NewLine();
            ImGui::Text("Structure selectionnée : %s",  currentStruct.c_str());
        }

        ImGui::NewLine();

        ImGui::Spacing();

        ImGui::Checkbox("Mode créateur", &creatorMod);

        ImGui::Spacing();

        if(creatorMod){
            ImGui::SliderFloat("Distance creation block", &creationDistance, 1.0, 20.0,"%1.f");
            ImGui::SliderFloat("radius", &radius, 1.0, 10.0,"%1.f");
            if (ImGui::Checkbox("Sphere", &sphereTool)){
                cubeTool = false;
            };
            if (ImGui::Checkbox("Cube", &cubeTool)){
                sphereTool = false;
            };

            ImGui::Spacing();
        }

        // Fin de la fenêtre ImGui
        ImGui::End();
        
    }
}

void ParamsWindow::modifTerrain(bool needToLoad){
    if (!this->inEditor){
        this->clearEntity = true; // On fera disparaître les entités au moment où on change le terrain

        if (*this->nbChunkTerrain > *this->planeHeight){ // On prend pas de risque, on réinitialise la spline
            *this->nbChunkTerrain = 1;
            this->mg->setNbChunkTerrain(*(this->nbChunkTerrain));
            this->resetContinentalnessPlot();
            this->resetCavePlot();
        }

        if (this->use_terrain_spline) this->mg->setContinentalnessSpline(this->terrain_simplex_values, this->continentalness_values);
        else this->mg->setHasTerrainSpline(false);

        if (this->use_cave_spline) this->mg->setCaveSpline(this->cave_simplex_values, this->cave_height_values);
        else this->mg->setHasCaveSpline(false);

        if (this->useBiomeChart) this->terrainControler->setBiomeChart(this->racineBiomeChart);   
        else this->terrainControler->setUseBiomeChart(false);

        this->mg->generateImageSurface();
        this->mg->generateImageCave_AC();
        this->mg->generateImageCave_Perlin();
        int widthHeightmap, lengthHeightmap, channels;
        unsigned char* dataPixels = stbi_load("../Textures/terrain.png", &widthHeightmap, &lengthHeightmap, &channels, 1);
        unsigned char* dataPixelsCaveAC = stbi_load("../Textures/cave_AC.png", &widthHeightmap, &lengthHeightmap, &channels, 1);
        this->terrainControler->buildPlanChunks(dataPixels, dataPixelsCaveAC, widthHeightmap, lengthHeightmap);
        if (*(this->generateStructure)){
            this->terrainControler->buildStructures(dataPixels);
        }
        if (needToLoad) this->terrainControler->loadTerrain();
        stbi_image_free(dataPixels);
        stbi_image_free(dataPixelsCaveAC);
        this->hitboxPlayer->setPosition(glm::vec3(-0.5f,(terrainControler->getPlaneHeight())*32.0f,-0.5f));
        this->hitboxPlayer->resetCanTakeDamage(); // Le joueur ne prends pas de dégâts de chute s'il tombe de trop haut au moment du changement de terrain
    }else{
        this->terrainControler->buildEditorChunk();
        if (needToLoad) this->terrainControler->loadTerrain();
    }
}

void ParamsWindow::drawCellInChart(CelluleBiome cb){
    ImPlot::PlotScatter("Points", cb.x_data, cb.y_data, 4);
    ImPlot::PlotLine("Points", cb.x_data, cb.y_data, 4);

    if (cb.isDivide){
        for (unsigned int i = 0 ; i < 4 ; i++){
            this->drawCellInChart(cb.cs[i]);
        }
    }
}

CelluleBiome* ParamsWindow::getSelectedCellBiome(CelluleBiome* currentCell, ImPlotPoint pos){
    if (currentCell->isDivide){
        for (unsigned int i = 0 ; i < currentCell->cs.size() ; i++){
            // Vérifier l'intersection entre le clic de la souris et les enfants de la cellule courante
            if (pos.x >= currentCell->cs[i].x_data[0] && pos.x <= currentCell->cs[i].x_data[2] && pos.y >= currentCell->cs[i].y_data[0] && pos.y <= currentCell->cs[i].y_data[1]){
                return this->getSelectedCellBiome(&(currentCell->cs[i]), pos);
            }
        }
    }
    return currentCell;
}

std::string ParamsWindow::saveBiomeChart(CelluleBiome* currentCell, int numCell){
    std::string partial_res = "";
    if (currentCell->isDivide){
        partial_res = std::to_string(numCell);
        bool one_is_divide = false;
        for (unsigned int i = 0 ; i < currentCell->cs.size() ; i++){
            if (currentCell->cs[i].isDivide){
                if (!one_is_divide){
                    partial_res += "/";
                    one_is_divide = true;
                }
                partial_res += this->saveBiomeChart(&(currentCell->cs[i]), i);
            }
        }
        if (one_is_divide){
            partial_res += "\\";
        }
    }
    return partial_res;
}

void ParamsWindow::divisionCell(CelluleBiome* tc){
    tc->isDivide = true;
    float midCell = tc->sizeCell/2.0f;
    for (unsigned int i = 0 ; i < 2 ; i++){
        for (unsigned int j = 0 ; j < 2 ; j++){
            CelluleBiome new_cell;
            new_cell.typeBiome = std::min((int)i*2+(int)j,2); // Plus tard on fera en sorte de contrôler le type des biomes via la fenêtre ImPlot
            new_cell.isDivide = false;
            for (unsigned int m = 0 ; m < 2 ; m++){
                for (unsigned int n = 0 ; n < 2 ; n++){
                    new_cell.x_data[m*2+n] = tc->x_data[0] + i*midCell + m*midCell;
                    new_cell.y_data[m*2+n] = tc->y_data[0] + j*midCell + (m==0?n:1-n)*midCell;
                }
            }
            new_cell.sizeCell = midCell;
            
            tc->cs.push_back(new_cell);
        }
    }
}

// Normalement il n'y a plus d'erreur dans la reconstruction de la biome chart
void ParamsWindow::rebuildBiomeChart(CelluleBiome* currentCell, std::string next_word, int startPos, bool isInCC){
    this->divisionCell(currentCell);
    bool isFirst = true;
    bool isIn = isInCC;
    int count_enter = 0;
    for (int i = startPos ; i < next_word.size() ; i++){
        char nc = next_word[i];
        if (nc != '/' && nc != '\\' && isIn){
            this->rebuildBiomeChart(&currentCell->cs[nc-'0'], next_word, i+1, false);
        }else if (nc == '/' && !isFirst && isIn){
            isIn = false;
            ++count_enter;
        }else if (nc == '/' && !isFirst){
            ++count_enter;
        }else if (nc == '\\'){
            --count_enter;
            if (count_enter == 0){
                isIn = true;
            }
        }else if (nc =='/' && isFirst){
            isIn = true;
        }

        if (count_enter==-1) break;
        isFirst = false;
    }
}

void ParamsWindow::saveConfigTerrain(){
    std::string saveFileName = "../configTerrainSave.txt";
    std::ofstream saveFile(saveFileName);

    if (saveFile.is_open()) {
        saveFile << *this->planeWidth << " " << *this->planeHeight << " " << *this->planeLength << " ";
        saveFile << *this->seedTerrain << " " << *this->octave << " " << *this->nbChunkTerrain << "\n";
        saveFile << this->saveBiomeChart(&this->racineBiomeChart,0) << "\n";
        // Sauvegarde de la spline utilisée pour la surface du terrain
        for (unsigned int i = 0 ; i < this->terrain_simplex_values.size() ; i++){
            saveFile << this->terrain_simplex_values[i] << "/" << this->continentalness_values[i] << " ";
        }
        saveFile << "\n";
        // Sauvegarde de la spline utilisée pour la hauteur de la grotte
        for (unsigned int i = 0 ; i < this->cave_simplex_values.size() ; i++){
            saveFile << this->cave_simplex_values[i] << "/" << this->cave_height_values[i] << " ";
        }
        saveFile << "\n";
        saveFile << this->terrainControler->saveModifBlocks();
        saveFile.close();
    } else {
        std::cerr << "Erreur : impossible d'ouvrir le fichier.\n";
    }
}

void ParamsWindow::openConfigTerrain(){
    std::string saveFileName = "../configTerrainSave.txt";
    std::ifstream saveFile(saveFileName);
    if (saveFile.is_open()) {
        this->terrain_simplex_values.clear();
        this->continentalness_values.clear();
        this->cave_simplex_values.clear();
        this->cave_height_values.clear();
        this->resetBiomeChart();
        std::string next_line;
        int n_line = 0;
        while (std::getline(saveFile, next_line)) {
            std::istringstream flux_next_line(next_line);
            std::string next_word;
            if (n_line==0){
                flux_next_line >> next_word;
                *this->planeWidth = std::stoi(next_word);
                this->mg->setWidthMap(*(this->planeWidth));
                flux_next_line >> next_word;
                *this->planeHeight = std::stoi(next_word);
                this->mg->setHeightMap(*(this->planeHeight));
                flux_next_line >> next_word;
                *this->planeLength = std::stoi(next_word);
                this->mg->setLengthMap(*(this->planeLength));
                flux_next_line >> next_word;
                *this->seedTerrain = std::stoi(next_word);
                this->mg->setSeed(*(this->seedTerrain));
                flux_next_line >> next_word;
                *this->octave = std::stoi(next_word);
                this->mg->setOctave(*(this->octave));
                flux_next_line >> next_word;
                *this->nbChunkTerrain = std::stoi(next_word);
                this->mg->setNbChunkTerrain(*(this->nbChunkTerrain));
            }
            while (flux_next_line >> next_word) {
                if (n_line == 1 && next_word.size() > 0){ // Reconstruction de la biome chart
                    this->rebuildBiomeChart(&this->racineBiomeChart, next_word, 2, true);
                }
                if (n_line == 2){ // Ouverture de la spline de la surface du terrain
                    std::size_t separation = next_word.find("/");
                    this->terrain_simplex_values.push_back(std::stof(next_word.substr(0,separation)));
                    this->continentalness_values.push_back(std::stof(next_word.substr(separation+1)));
                }else if (n_line == 3){ // Ouverture de la spline de hauteur de la grotte
                    std::size_t separation = next_word.find("/");
                    this->cave_simplex_values.push_back(std::stof(next_word.substr(0,separation)));
                    this->cave_height_values.push_back(std::stof(next_word.substr(separation+1)));
                }
            }
            ++n_line;
            if (n_line > 3) break;
        }
        this->modifTerrain(false);

        // A partir de là, on lit les modifications des blocs
        // Attention à bien prendre en compte les changements si on fait une autre sauvegarde par la suite
        // Donc bien penser à supprimer les modifs faites jusque là, et sauvegarder les modifs du fichier de sauvegarde (fait dans applyModifBlock)
        this->terrainControler->clearModifsBlock();
        while (std::getline(saveFile, next_line)) {
            this->terrainControler->applyModifBlock(next_line);
        }
        saveFile.close();
        this->terrainControler->loadTerrain();
    } else {
        std::cerr << "Erreur : Aucun fichier de configuration existant.\n";
    }
}



void ParamsWindow::draw(){
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    float fps = ImGui::GetIO().Framerate;

    ImGui::NewFrame();
    ImGui::Begin("Panneau de contrôle");
    if (ImGui::SliderInt("Style de la fenêtre ImGui", &(this->style), 0, 2)){
        this->useStyle();
    }
    ImGui::Text("FPS : %.2f", fps);

    ImGui::Spacing();

    ImGui::SliderInt("Vitesse caméra", &speedCam, 5, 200);

    ImGui::Spacing();

    if(!this->inEditor){

        ImGui::Text("Position : %.2f / %.2f / %.2f", (*posJoueur)[0], (*posJoueur)[1], (*posJoueur)[2]);

        ImGui::Spacing();

        ImGui::SliderFloat("Vitesse Joueur", this->speedPlayer, 0.0, 50.0);

        ImGui::Spacing();

        ImGui::SliderFloat("FoV", &FoV, 1.0, 179.9);

        ImGui::Spacing();
    }

    if (ImGui::Checkbox("Caméra orbitale", &cameraOrbitale)){
        cameraLibre = false;
        cameraMouseLibre = false;
        cameraMousePlayer = false;
    }

    ImGui::Spacing();
    
    if (ImGui::Checkbox("Caméra libre", &cameraLibre)){
        cameraOrbitale = false;
        cameraMouseLibre = false;
        cameraMousePlayer = false;
    }

    ImGui::Spacing();
    
    if (ImGui::Checkbox("Caméra souris", &cameraMouseLibre)){
        cameraLibre = false;
        cameraOrbitale = false;
        cameraMousePlayer = false;
    }

    if (!this->inEditor){
        if (ImGui::Checkbox("Caméra player", &cameraMousePlayer)){
            cameraLibre = false;
            cameraOrbitale = false;
            cameraMouseLibre = false;
        }
    }

    ImGui::Spacing();

    if (ImGui::Checkbox("Rendu filaire", &renduFilaire)) {
        if (renduFilaire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
    ImGui::Spacing();

    ImGui::Checkbox("Afficher l'hud", &showHud);

    ImGui::Spacing();

    ImGui::Checkbox("Mode de jeu (créatif/survie)", &modeJeu);

    ImGui::Spacing();

    if(!(this->inEditor)){
        ImGui::Checkbox("Mode éditeur", &(this->inEditor));
    }

    ImGui::Spacing();

    if (ImGui::SliderInt("Longueur terrain", this->planeWidth, 1, 22)){
        this->mg->setWidthMap(*(this->planeWidth));
        this->modifTerrain(true);
    }

    if (ImGui::SliderInt("Largeur terrain", this->planeLength, 1, 22)){
        this->mg->setLengthMap(*(this->planeLength));
        this->modifTerrain(true);
    }

    if (ImGui::SliderInt("Hauteur terrain", this->planeHeight, 1, 22)){
        this->mg->setHeightMap(*(this->planeHeight));
        this->modifTerrain(true);
    }

    ImGui::Spacing();

    if(this->inEditor){
        ImGui::Text("Nom du fichier de la structure : ");
        ImGui::SameLine();
        ImGui::InputText(" ", nameStructure, IM_ARRAYSIZE(nameStructure));
        if (ImGui::Button("Sauvegarder la structure")){
            std::string filePath = nameStructure;
            if (filePath.size() != 0){
                this->terrainControler->saveStructure(filePath);
                std::string pngStructure = std::string(nameStructure) + ".png";
                this->saveScreenshot("../Structures/" + pngStructure,ParamsWindow::widthScreen,ParamsWindow::heightScreen,SIZE_SCREEN_STRUCTURE,SIZE_SCREEN_STRUCTURE);
                this->newStructure = true;
                this->nameNewStructure=nameStructure;
            }else{
                std::cout << "Veuillez saisir un nom pour le fichier de la structure\n";
            }
        }
    }else{
        if(ImGui::SliderInt("Seed de génération", this->seedTerrain, 0, 10000)){
            this->mg->setSeed(*(this->seedTerrain));
        }

        ImGui::Spacing();

        if(ImGui::SliderInt("Nombre d'octaves", this->octave, 1, 8)){
            this->mg->setOctave(*(this->octave));
        }

        ImGui::Spacing();

        if(ImGui::SliderInt("Amplitude du terrain en chunk", this->nbChunkTerrain, 1, this->terrainControler->getPlaneHeight())){
            this->mg->setNbChunkTerrain(*(this->nbChunkTerrain));
            this->resetContinentalnessPlot();
        }

        ImGui::Spacing();

        ImGui::Checkbox("Placer les structures sur le terrain", this->generateStructure);

        ImGui::Checkbox("Builder Tools", &this->showBuilderWindow);

        if (ImGui::Button("Mettre à jour le terrain")){
            this->modifTerrain(true);
        }

        if (ImGui::Button("Save Terrain Config")){
            this->saveConfigTerrain();
        }
        if (ImGui::Button("Open Terrain Config")){
            this->openConfigTerrain();
        }

        ImGui::Checkbox("Utiliser la spline pour le terrain", &this->use_terrain_spline);

        ImGui::Spacing();

        ImGui::Checkbox("Utiliser la spline pour la grotte", &this->use_cave_spline);

        ImGui::Spacing();

        ImGui::Checkbox("Utiliser la Biome Chart", &this->useBiomeChart);

        ImGui::Spacing();

        /*
        // Type 0 = Plein ; Type 1 = Sinus ; Type 2 = Flat ; Type 3 = Procedural
        if (ImGui::SliderInt("Type de chunk", &typeChunk, 0, 3)){
            buildPlanChunks(dataPixels, widthHeightmap, heightHeightmap);
        }
        */

        ImGui::Spacing();

        ImGui::Spacing();

        float max_value_continentalness = *(this->nbChunkTerrain)*32.0 - 1.0;
        ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, max_value_continentalness, ImGuiCond_Always);

        if (ImPlot::BeginPlot("Continentalness")) {
                ImPlot::SetupAxis(ImAxis_X1, "Terrain simplex values", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
                ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
                ImPlot::SetupAxis(ImAxis_Y1, "Continentalness", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);

                if (!this->terrain_simplex_values.empty() && !this->continentalness_values.empty()) {
                    ImPlot::PlotScatter("Points", this->terrain_simplex_values.data(), this->continentalness_values.data(), this->terrain_simplex_values.size());
                    ImPlot::PlotLine("Points", this->terrain_simplex_values.data(), this->continentalness_values.data(), this->terrain_simplex_values.size());
                }
                if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Ajout d'un point
                    ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                    float simplex_value = mouse_pos.x;
                    float continent_value = mouse_pos.y;
                    if (this->terrain_simplex_values.size() != 0){
                        for (unsigned int i = 0 ; i < this->terrain_simplex_values.size() ; i++){
                            if (this->terrain_simplex_values[i] > simplex_value){
                                this->terrain_simplex_values.insert(this->terrain_simplex_values.begin()+i, simplex_value);
                                this->continentalness_values.insert(this->continentalness_values.begin()+i, continent_value);
                                break;
                            }else if (i == this->terrain_simplex_values.size()-1){
                                this->terrain_simplex_values.push_back(simplex_value);
                                this->continentalness_values.push_back(continent_value);
                                break;
                            }
                        }
                    }else{
                        this->terrain_simplex_values.push_back(simplex_value);
                        this->continentalness_values.push_back(continent_value);
                    }
                }else if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
                    // Suppression d'un point (Pour l'instant, appuyer sur le clic droit ouvre aussi une fenetre dans ImPlot)
                    ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                    float minDistance = FLT_MAX;
                    int indexToDelete = -1;
                    for (unsigned int i = 0 ; i < this->terrain_simplex_values.size() ; i++){
                        float dx = this->terrain_simplex_values[i] - mouse_pos.x;
                        float dy = this->continentalness_values[i] - mouse_pos.y;
                        float distanceToPoint = dx*dx+dy*dy;
                        if (distanceToPoint < minDistance){
                            minDistance = distanceToPoint;
                            indexToDelete = i;
                        }
                    }
                    // On empeche la suppression du premier et du dernier point
                    if (indexToDelete != 0 && indexToDelete != this->terrain_simplex_values.size()-1 && minDistance < 0.1f){
                        this->terrain_simplex_values.erase(this->terrain_simplex_values.begin() + indexToDelete);
                        this->continentalness_values.erase(this->continentalness_values.begin() + indexToDelete);
                    }
                }

            ImPlot::EndPlot();
        }

        if (ImGui::Button("Reset Continentalness Plot")){
            this->resetContinentalnessPlot();
        }
        ImGui::SetNextItemWidth(250.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("First Continentalness", &(this->continentalness_values[0]), 0.0, max_value_continentalness);
        ImGui::SetNextItemWidth(250.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("Last Continentalness", &(this->continentalness_values.back()), 0.0, max_value_continentalness);

        ImGui::Spacing();

        ImGui::Spacing();

        ImVec2 biome_chart_size(400, 400);
        
        if (ImPlot::BeginPlot("Biome Chart", biome_chart_size, ImPlotFlags_NoMenus | ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxis(ImAxis_X1, "Précipitation", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 1.0);
            ImPlot::SetupAxis(ImAxis_Y1, "Humidité", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0);

            this->drawCellInChart(this->racineBiomeChart); // Deessine récursivement toutes les cellules de la biome chart

            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Division en 4 du biome sélectionné dans la chart
                ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                CelluleBiome* tc = this->getSelectedCellBiome(&this->racineBiomeChart, mouse_pos);
                this->divisionCell(tc);
            }

            ImPlot::EndPlot();
        }

        if (ImGui::Button("Reset Biome Chart")){
            this->resetBiomeChart();
        }

        ImGui::Spacing();

        float max_value_cave = (*this->planeHeight - *this->nbChunkTerrain)*32.0 - 1.0;
        // ImPlot::SetNextAxisLimits(ImAxis_Y1, 1, 31); // Plus tard, faire en sorte que cet axe s'adapte en fonction du nombre de chunk utilisable réellement possible
        ImPlot::SetNextAxisLimits(ImAxis_Y1, 1, max_value_cave, ImGuiCond_Always);

        if (ImPlot::BeginPlot("Cave")) {
                ImPlot::SetupAxis(ImAxis_X1, "Cave simplex values", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
                ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
                ImPlot::SetupAxis(ImAxis_Y1, "Cave height values", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);

                if (!this->cave_simplex_values.empty() && !this->cave_height_values.empty()) {
                    ImPlot::PlotScatter("Points", this->cave_simplex_values.data(), this->cave_height_values.data(), this->cave_simplex_values.size());
                    ImPlot::PlotLine("Points", this->cave_simplex_values.data(), this->cave_height_values.data(), this->cave_simplex_values.size());
                }
                if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Ajout d'un point
                    ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                    float simplex_value = mouse_pos.x;
                    float cave_height_value = mouse_pos.y;
                    if (this->cave_height_values.size() != 0){
                        for (unsigned int i = 0 ; i < this->cave_simplex_values.size() ; i++){
                            if (this->cave_simplex_values[i] > simplex_value){
                                this->cave_simplex_values.insert(this->cave_simplex_values.begin()+i, simplex_value);
                                this->cave_height_values.insert(this->cave_height_values.begin()+i, cave_height_value);
                                break;
                            }else if (i == this->cave_simplex_values.size()-1){
                                this->cave_simplex_values.push_back(simplex_value);
                                this->cave_height_values.push_back(cave_height_value);
                                break;
                            }
                        }
                    }else{
                        this->cave_height_values.push_back(simplex_value);
                        this->cave_height_values.push_back(cave_height_value);
                    }
                }else if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
                    // Suppression d'un point (Pour l'instant, appuyer sur le clic droit ouvre aussi une fenetre dans ImPlot)
                    ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                    float minDistance = FLT_MAX;
                    int indexToDelete = -1;
                    for (unsigned int i = 0 ; i < this->cave_simplex_values.size() ; i++){
                        float dx = this->cave_simplex_values[i] - mouse_pos.x;
                        float dy = this->cave_height_values[i] - mouse_pos.y;
                        float distanceToPoint = dx*dx+dy*dy;
                        if (distanceToPoint < minDistance){
                            minDistance = distanceToPoint;
                            indexToDelete = i;
                        }
                    }
                    // On empeche la suppression du premier et du dernier point
                    if (indexToDelete != 0 && indexToDelete != this->cave_simplex_values.size()-1 && minDistance < 0.1f){
                        this->cave_simplex_values.erase(this->cave_simplex_values.begin() + indexToDelete);
                        this->cave_height_values.erase(this->cave_height_values.begin() + indexToDelete);
                    }
                }

            ImPlot::EndPlot();
        }

        if (ImGui::Button("Reset Cave Plot")){
            this->resetCavePlot();
        }

        ImGui::SetNextItemWidth(250.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("First Cave Height Value", &(this->cave_height_values[0]), 1.0, max_value_cave);
        ImGui::SetNextItemWidth(250.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("Last Cave Height Value", &(this->cave_height_values.back()), 1.0, max_value_cave);
    }

    ImGui::End();

    this->openBuilderTools();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ParamsWindow::resetContinentalnessPlot(){
    this->terrain_simplex_values = {-1.0, 1.0};
    this->continentalness_values = {16.0, 16.0};
}

void ParamsWindow::resetCavePlot(){
    this->cave_simplex_values = {-1.0, 1.0};
    this->cave_height_values = {16.0, 16.0};
}

void ParamsWindow::resetBiomeChart(){
    this->racineBiomeChart.isDivide = false;
    this->racineBiomeChart.typeBiome = 2;
    this->racineBiomeChart.cs.clear();
    float t_x[4] = { 0.0f, 0.0f, 1.0f, 1.0f};
    float t_y[4] = { 0.0f, 1.0f, 1.0f, 0.0f};
    for (unsigned int i = 0 ; i < 4 ; i++){
        this->racineBiomeChart.x_data[i] = t_x[i];
        this->racineBiomeChart.y_data[i] = t_y[i];
    }
    this->racineBiomeChart.sizeCell = 1.0f;
}

void ParamsWindow::attachNewTerrain(TerrainControler *terrainControler){
    this->terrainControler = terrainControler;
    // Attention à bien récupérer les références de la nouvelle instance de TerrainControler
    this->mg = terrainControler->getMapGenerator();
    this->planeWidth = terrainControler->getRefToPlaneWidth();
    this->planeLength = terrainControler->getRefToPlaneLength();
    this->planeHeight = terrainControler->getRefToPlaneHeight();
}
