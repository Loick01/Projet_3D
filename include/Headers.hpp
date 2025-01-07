#pragma once

// Attention : Inclure GLAD avant GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../external/glm/ext/matrix_clip_space.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "implot.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdbool.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <thread>
#include "../external/glm/glm.hpp"
#include "../external/glm/gtc/matrix_transform.hpp"
#include "../external/glm/gtc/type_ptr.hpp"

#include <Window.hpp>
#include <FastNoise.h>
#include <Voxel.hpp>
#include <Shader.hpp>
#include <Player.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <Texture.h>
#include <Skybox.hpp>
#include <Hud.hpp>
#include <MapGenerator.hpp>
#include <Hitbox.hpp>
// #include <Entity.hpp>
#include <Transform.hpp>
#include <TerrainControler.hpp>
#include <Chunk.hpp>
#include <ParamsWindow.hpp>
#include <Agent.hpp>
#include <Sound.hpp>

#include <dirent.h>  // Pour opendir, readdir, closedir
#include <sys/types.h>
#include <algorithm>


