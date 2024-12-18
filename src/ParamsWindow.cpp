#include <ParamsWindow.hpp>

// char ParamsWindow::nameStructure[512]; // Permet d'éviter les erreurs de lien à la compilation

ParamsWindow::ParamsWindow(GLFWwindow* window, int style, TerrainControler *terrainControler, Player *player){
    this->style = style;
    this->renduFilaire = false;
    this->terrainControler = terrainControler;
    this->mg = terrainControler->getMapGenerator();
    this->inEditor = false;
    this->clearEntity = false;
    this->use_spline = false;
    this->speedPlayer = player->getRefToSpeed();
    this->posJoueur = player->getHitbox()->getRefToBottomPoint();
    this->planeWidth = terrainControler->getRefToPlaneWidth();
    this->planeLength = terrainControler->getRefToPlaneLength();
    this->planeHeight = terrainControler->getRefToPlaneHeight();
    this->seedTerrain = terrainControler->getRefToSeedTerrain();
    this->octave = terrainControler->getRefToOctave();
    this->hitboxPlayer = player->getHitbox();
    this->init(window);
    this->useStyle();

    this->resetContinentalnessPlot();

    this->nbChunkTerrain = terrainControler->getRefToNbChunkTerrain();

    this->racineBiomeChart.isDivide = false;
    this->racineBiomeChart.typeBiome = 0;
    this->racineBiomeChart.cs.clear();
    float t_x[4] = { 0.0f, 0.0f, 1.0f, 1.0f};
    float t_y[4] = { 0.0f, 1.0f, 1.0f, 0.0f};
    for (unsigned int i = 0 ; i < 4 ; i++){
        this->racineBiomeChart.x_data[i] = t_x[i];
        this->racineBiomeChart.y_data[i] = t_y[i];
    }
    this->racineBiomeChart.sizeCell = 1.0f;
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

    ImPlot::CreateContext();
}

bool ParamsWindow::getInEditor(){
    return this->inEditor;
}

bool ParamsWindow::getClearEntity(){
	return this->clearEntity;
}

void ParamsWindow::resetClearEntity(){
	this->clearEntity = false;
}

void ParamsWindow::modifTerrain(){
	this->clearEntity = true; // On fera disparaître les entités au moment où on change le terrain
    if (this->use_spline){
        this->mg->setContinentalnessSpline(this->simplex_values, this->continentalness_values);
    }else{
        this->mg->setHasSpline(false);
    }
    this->mg->generateImage();
    int widthHeightmap, lengthHeightmap, channels;
    unsigned char* dataPixels = stbi_load("../Textures/terrain.png", &widthHeightmap, &lengthHeightmap, &channels, 1);
    this->terrainControler->buildPlanChunks(dataPixels, widthHeightmap, lengthHeightmap);
    stbi_image_free(dataPixels);
    this->hitboxPlayer->setPosition(glm::vec3(-0.5f,(terrainControler->getPlaneHeight())*32.0f,-0.5f));
    this->hitboxPlayer->resetCanTakeDamage(); // Le joueur ne prends pas de dégâts de chute s'il tombe de trop haut au moment du changement de terrain
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
    }else{
        return currentCell;
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

    if(this->inEditor){
        /*
        ImGui::Text("Nom du fichier de la structure : ");
        ImGui::SameLine();
        ImGui::InputText(" ", nameStructure, IM_ARRAYSIZE(nameStructure));
        if (ImGui::Button("Sauvegarder la structure")){
            std::string filePath = nameStructure;
            if (filePath.size() != 0){
                this->terrainControler->saveStructure(filePath);
            }else{
                std::cout << "Veuillez saisir un nom pour le fichier de la structure\n";
            }
        }
        */
    }else{
        if (ImGui::SliderInt("Longueur terrain", this->planeWidth, 1, 22)){
            this->mg->setWidthMap(*(this->planeWidth));
            this->modifTerrain();
        }

        ImGui::Spacing();

        if (ImGui::SliderInt("Largeur terrain", this->planeLength, 1, 22)){
            this->mg->setLengthMap(*(this->planeLength));
            this->modifTerrain();
        }

        if (ImGui::SliderInt("Hauteur terrain", this->planeHeight, 1, 22)){
            this->mg->setHeightMap(*(this->planeHeight));
            this->modifTerrain();
        }

        ImGui::Spacing();

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

        if (ImGui::Button("Mettre à jour le terrain")){
            this->modifTerrain();
        }

        ImGui::Checkbox("Utiliser la spline", &this->use_spline);

        ImGui::Spacing();

        /*
        // Type 0 = Plein ; Type 1 = Sinus ; Type 2 = Flat ; Type 3 = Procedural
        if (ImGui::SliderInt("Type de chunk", &typeChunk, 0, 3)){
            buildPlanChunks(dataPixels, widthHeightmap, heightHeightmap);
        }
        */
    }

    ImGui::Spacing();

    ImGui::Spacing();

    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, *(this->nbChunkTerrain)*32.0 - 1.0, ImGuiCond_Always);

    if (ImPlot::BeginPlot("Continentalness")) {
            ImPlot::SetupAxis(ImAxis_X1, "simplex values", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
            ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
            ImPlot::SetupAxis(ImAxis_Y1, "Continentalness", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);

            if (!this->simplex_values.empty() && !this->continentalness_values.empty()) {
                ImPlot::PlotScatter("Points", this->simplex_values.data(), this->continentalness_values.data(), this->simplex_values.size());
                ImPlot::PlotLine("Points", this->simplex_values.data(), this->continentalness_values.data(), this->simplex_values.size());
            }
            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Ajout d'un point
                ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                float simplex_value = mouse_pos.x;
                float continent_value = mouse_pos.y;
                if (this->simplex_values.size() != 0){
                    for (unsigned int i = 0 ; i < this->simplex_values.size() ; i++){
                        if (this->simplex_values[i] > simplex_value){
                            this->simplex_values.insert(this->simplex_values.begin()+i, simplex_value);
                            this->continentalness_values.insert(this->continentalness_values.begin()+i, continent_value);
                            break;
                        }else if (i == this->simplex_values.size()-1){
                            this->simplex_values.push_back(simplex_value);
                            this->continentalness_values.push_back(continent_value);
                            break;
                        }
                    }
                }else{
                    this->simplex_values.push_back(simplex_value);
                    this->continentalness_values.push_back(continent_value);
                }
            }else if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
                // Suppression d'un point (Pour l'instant, appuyer sur le clic droit ouvre aussi une fenetre dans ImPlot)
                ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                float minDistance = FLT_MAX;
                int indexToDelete = -1;
                for (unsigned int i = 0 ; i < this->simplex_values.size() ; i++){
                    float dx = this->simplex_values[i] - mouse_pos.x;
                    float dy = this->continentalness_values[i] - mouse_pos.y;
                    float distanceToPoint = dx*dx+dy*dy;
                    if (distanceToPoint < minDistance){
                        minDistance = distanceToPoint;
                        indexToDelete = i;
                    }
                }
                // On empeche la suppression du premier et du dernier point
                if (indexToDelete != 0 && indexToDelete != this->simplex_values.size()-1 && minDistance < 0.1f){
                    this->simplex_values.erase(this->simplex_values.begin() + indexToDelete);
                    this->continentalness_values.erase(this->continentalness_values.begin() + indexToDelete);
                }
            }

        ImPlot::EndPlot();
    }

    if (ImGui::Button("Reset Continentalness")){
        this->resetContinentalnessPlot();
    }
    ImGui::SetNextItemWidth(250.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("First Continentalness", &(this->continentalness_values[0]), 0.0, *(this->nbChunkTerrain)*32.0 - 1.0);
    ImGui::SetNextItemWidth(250.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("Last Continentalness", &(this->continentalness_values.back()), 0.0, *(this->nbChunkTerrain)*32.0 - 1.0);

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
            tc->isDivide = true;
            float midCell = tc->sizeCell/2.0f;
            for (unsigned int i = 0 ; i < 2 ; i++){
                for (unsigned int j = 0 ; j < 2 ; j++){
                    CelluleBiome new_cell;
                    new_cell.typeBiome = 0;
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

        ImPlot::EndPlot();
    }
    

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ParamsWindow::resetContinentalnessPlot(){
    this->simplex_values = {-1.0, 1.0};
    this->continentalness_values = {16.0, 16.0};
}

void ParamsWindow::attachNewTerrain(TerrainControler *terrainControler){
    this->terrainControler = terrainControler;
    // Attention à bien récupérer les références de la nouvelle instance de TerrainControler
    this->mg = terrainControler->getMapGenerator();
    this->planeWidth = terrainControler->getRefToPlaneWidth();
    this->planeLength = terrainControler->getRefToPlaneLength();
    this->seedTerrain = terrainControler->getRefToSeedTerrain();
    this->octave = terrainControler->getRefToOctave();
}
