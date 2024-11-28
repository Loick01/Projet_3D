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

    this->noiseGenerator.SetSeed(*(this->seedTerrain));
    this->resetContinentalnessPlot();
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
        this->mg->setContinentalnessSpline(this->perlin_values, this->continentalness_values);
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

    this->ContinentValue = this->getContinentalnessByInterpolation(*this->posJoueur);
    ImGui::Text("Continentalness : %.2f", this->ContinentValue);

    ImGui::Spacing();

    if (ImPlot::BeginPlot("Continentalness")) {
            ImPlot::SetupAxis(ImAxis_X1, "Perlin values", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
            ImPlot::SetupAxisLimits(ImAxis_X1, -1.0, 1.0);
            ImPlot::SetupAxis(ImAxis_Y1, "Continentalness", ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 31.0);

            if (!this->perlin_values.empty() && !this->continentalness_values.empty()) {
                ImPlot::PlotScatter("Points", this->perlin_values.data(), this->continentalness_values.data(), this->perlin_values.size());
                ImPlot::PlotLine("Points", this->perlin_values.data(), this->continentalness_values.data(), this->perlin_values.size());

                ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImPlot::PlotScatter("Player Continent", &(this->perlinValue), &(this->ContinentValue), 1);
                ImPlot::PopStyleColor();
            }
            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
                float perlin_value = mouse_pos.x;
                float continent_value = mouse_pos.y;
                if (this->perlin_values.size() != 0){
                    for (unsigned int i = 0 ; i < this->perlin_values.size() ; i++){
                        if (this->perlin_values[i] > perlin_value){
                            this->perlin_values.insert(this->perlin_values.begin()+i, perlin_value);
                            this->continentalness_values.insert(this->continentalness_values.begin()+i, continent_value);
                            break;
                        }else if (i == this->perlin_values.size()-1){
                            this->perlin_values.push_back(perlin_value);
                            this->continentalness_values.push_back(continent_value);
                            break;
                        }
                    }
                }else{
                    this->perlin_values.push_back(perlin_value);
                    this->continentalness_values.push_back(continent_value);
                }
            }

        ImPlot::EndPlot();
    }

    if (ImGui::Button("Reset Continentalness")){
        this->resetContinentalnessPlot();
    }
    ImGui::SetNextItemWidth(250.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("First Continentalness", &(this->continentalness_values[0]), 0.0, 31.0);
    ImGui::SetNextItemWidth(250.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("Last Continentalness", &(this->continentalness_values.back()), 0.0, 31.0);


    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ParamsWindow::resetContinentalnessPlot(){
    this->perlin_values = {-1.0, 1.0};
    this->continentalness_values = {16.0, 16.0};
}

float ParamsWindow::getContinentalnessByInterpolation(glm::vec3 position){
    this->perlinValue = this->noiseGenerator.GetNoise(position.x,position.z);
    float index_start, index_end;
    for (unsigned int i = 0 ; i < this->perlin_values.size()-1 ; i++){
        if (this->perlinValue >= this->perlin_values[i] && this->perlinValue <= this->perlin_values[i+1]){
            index_start = i;
            index_end = i+1;
            break;
        }
    }

    return this->continentalness_values[index_start] + (this->perlinValue - this->perlin_values[index_start])*((this->continentalness_values[index_end]-this->continentalness_values[index_start])/(this->perlin_values[index_end]-this->perlin_values[index_start]));
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
