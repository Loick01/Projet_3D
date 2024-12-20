// Pour l'instant on utilise ce fichier pour partager des déclarations de variables entre main.cpp et ParamsWindow.cpp
// Ca permet d'avoir accès aux références de ces variables pour les modifier dans la fenêtre ImGui, sans avoir à les passer en paramètres
// du constructeur de ParamsWindow (comme on le faisait avant)
// C'est un peu de la triche, mais tant qu'il n'y a pas les classes nécéssaires on ne peut pas faire autrement
#ifndef VARIABLE_H
#define VARIABLE_H

extern bool showHud;
extern bool modeJeu; 
extern int speedCam;
extern float FoV;
extern bool cameraOrbitale;
extern bool cameraLibre;
extern bool cameraMouseLibre;
extern bool cameraMousePlayer;

struct CelluleBiome{
    bool isDivide;
    int typeBiome;
    float x_data[4];
    float y_data[4];
    float sizeCell;

    std::vector<CelluleBiome> cs;
};

#endif