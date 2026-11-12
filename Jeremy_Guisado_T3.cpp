#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// Configuraciones de ventana
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Cámara FPS
glm::vec3 cameraPos = glm::vec3(0.0f, 1.15f, 5.5f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov = 60.0f;

// Tiempos y Animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float muzzleFlashTimer = 0.0f;
float recoilOffset = 0.0f;

// Estado del Juego (RF08 y RF09)
enum GameState { PLAYING, GAME_OVER, VICTORY };
GameState estadoJuego = PLAYING;

// Armas (RF01 y RF02)
int armaActual = 1; // 1: Rifle Plasma, 2: Pistola Táctica
int municionArma1 = 15;
int municionArma2 = 12;

// Enemigos (RF04, RF05 y RF07)
struct Zombi {
    glm::vec3 posicion;
    glm::vec3 spawnPos;
    int vida;
    bool activo;
    float anguloMirada;
    float faseCaminata;
    float respawnTimer;
};

std::vector<Zombi> zombis = {
    {glm::vec3(-1.8f, 0.0f, -3.5f), glm::vec3(-1.8f, 0.0f, -3.5f), 2, true, 0.0f, 0.0f, 0.0f},
    {glm::vec3(3.5f, 0.0f, -5.5f), glm::vec3(3.5f, 0.0f, -5.5f), 2, true, 0.0f, 1.5f, 0.0f},
    {glm::vec3(-1.0f, 0.0f, -8.0f), glm::vec3(-1.0f, 0.0f, -8.0f), 2, true, 0.0f, 3.0f, 0.0f}
};

// Coleccionables (RF10 y RF02)
struct CajaMunicion {
    glm::vec3 posicion;
    bool activa;
};
std::vector<CajaMunicion> cajas = {
    {glm::vec3(-4.0f, 0.2f,  2.0f), true},
    {glm::vec3(4.5f, 0.2f, -1.0f), true}
};

// Callbacks y funciones
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void resetGame(GLFWwindow* window);

// Shaders
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"out vec3 FragPos;\n"
"out vec3 LocalPos;\n"
"out vec3 Normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"   LocalPos = aPos;\n"
"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
"   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 FragPos;\n"
"in vec3 LocalPos;\n"
"in vec3 Normal;\n"
"uniform vec3 objectColor;\n"
"uniform int materialType;\n"
"uniform float flashIntensity;\n"
"uniform vec3 flashColor;\n"
"uniform vec3 playerPos;\n"
"\n"
"float hash(vec2 p) {\n"
"   p = floor(p * 16.0);\n"
"   return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);\n"
"}\n"
"\n"
"void main() {\n"
"   if (materialType == 3) { FragColor = vec4(objectColor, 1.0); return; }\n"
"   if (materialType == 4) { FragColor = vec4(0.05, 0.08, 0.04, 1.0); return; }\n"
"\n"
"   vec3 baseColor = objectColor;\n"
"   if (materialType == 1) {\n"
"       float groove = step(0.18, fract(FragPos.y * 5.0));\n"
"       float grit = hash(vec2(FragPos.x + FragPos.z, FragPos.y * 2.0));\n"
"       vec3 woodGreen = mix(vec3(0.32, 0.36, 0.08), vec3(0.48, 0.45, 0.14), grit);\n"
"       if (mod(floor(FragPos.y * 5.0), 4.0) == 0.0) woodGreen *= 0.75;\n"
"       baseColor = woodGreen * (0.45 + 0.55 * groove);\n"
"   } else if (materialType == 2) {\n"
"       float grit = hash(FragPos.xz * 0.8);\n"
"       baseColor = mix(vec3(0.30, 0.48, 0.22), vec3(0.42, 0.60, 0.30), grit);\n"
"   } else {\n"
"       float grit = hash(LocalPos.xy * 4.0 + LocalPos.z * 4.0);\n"
"       baseColor = objectColor * (0.88 + 0.22 * grit);\n"
"   }\n"
"\n"
"   vec3 norm = normalize(Normal);\n"
"   vec3 lightPos = vec3(0.0, 4.2, -2.0);\n"
"   vec3 lightDir = normalize(lightPos - FragPos);\n"
"   float diff = max(dot(norm, lightDir), 0.0);\n"
"   float dist = length(lightPos - FragPos);\n"
"   float atten = 1.0 / (1.0 + 0.04 * dist + 0.008 * dist * dist);\n"
"\n"
"   vec3 ambient = 0.42 * baseColor;\n"
"   vec3 diffuse = 0.65 * diff * baseColor * atten;\n"
"   vec3 dynamicLuz = vec3(0.0);\n"
"   if (flashIntensity > 0.0) {\n"
"       float dPlayer = length(playerPos - FragPos);\n"
"       float fAtten = 1.0 / (1.0 + 0.15 * dPlayer + 0.05 * dPlayer * dPlayer);\n"
"       dynamicLuz = flashColor * flashIntensity * fAtten * 1.5;\n"
"   }\n"
"\n"
"   FragColor = vec4(ambient + diffuse + dynamicLuz * baseColor, 1.0);\n"
"}\n\0";

void drawPart(unsigned int shader, glm::mat4 parent, glm::vec3 pos, glm::vec3 scale, glm::vec3 color, int matType = 0, float rotAngle = 0.0f, glm::vec3 rotAxis = glm::vec3(1, 0, 0)) {
    glm::mat4 model = glm::translate(parent, pos);
    if (rotAngle != 0.0f) model = glm::rotate(model, glm::radians(rotAngle), rotAxis);
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shader, "objectColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(shader, "materialType"), matType);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// Renderizador de texto pixelado 2D para mostrar mensajes en pantalla sin librerías externas
void drawText2D(unsigned int shader, const std::string& text, float centerX, float startY, float pixelSize, glm::vec3 color) {
    glm::mat4 id = glm::mat4(1.0f);
    float totalWidth = text.length() * 6.0f * pixelSize;
    float cursorX = centerX - (totalWidth * 0.5f);

    for (char c : text) {
        const char* glyph[7] = { "00000","00000","00000","00000","00000","00000","00000" };
        switch (c) {
        case 'A': glyph[0] = "01110"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "11111"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "10001"; break;
        case 'C': glyph[0] = "01111"; glyph[1] = "10000"; glyph[2] = "10000"; glyph[3] = "10000"; glyph[4] = "10000"; glyph[5] = "10000"; glyph[6] = "01111"; break;
        case 'D': glyph[0] = "11110"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "10001"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "11110"; break;
        case 'E': glyph[0] = "11111"; glyph[1] = "10000"; glyph[2] = "10000"; glyph[3] = "11110"; glyph[4] = "10000"; glyph[5] = "10000"; glyph[6] = "11111"; break;
        case 'G': glyph[0] = "01111"; glyph[1] = "10000"; glyph[2] = "10000"; glyph[3] = "10111"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "01111"; break;
        case 'I': glyph[0] = "11111"; glyph[1] = "00100"; glyph[2] = "00100"; glyph[3] = "00100"; glyph[4] = "00100"; glyph[5] = "00100"; glyph[6] = "11111"; break;
        case 'L': glyph[0] = "10000"; glyph[1] = "10000"; glyph[2] = "10000"; glyph[3] = "10000"; glyph[4] = "10000"; glyph[5] = "10000"; glyph[6] = "11111"; break;
        case 'M': glyph[0] = "10001"; glyph[1] = "11011"; glyph[2] = "10101"; glyph[3] = "10001"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "10001"; break;
        case 'N': glyph[0] = "10001"; glyph[1] = "11001"; glyph[2] = "10101"; glyph[3] = "10011"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "10001"; break;
        case 'O': glyph[0] = "01110"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "10001"; glyph[4] = "10001"; glyph[5] = "10001"; glyph[6] = "01110"; break;
        case 'P': glyph[0] = "11110"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "11110"; glyph[4] = "10000"; glyph[5] = "10000"; glyph[6] = "10000"; break;
        case 'R': glyph[0] = "11110"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "11110"; glyph[4] = "10100"; glyph[5] = "10010"; glyph[6] = "10001"; break;
        case 'S': glyph[0] = "01111"; glyph[1] = "10000"; glyph[2] = "10000"; glyph[3] = "01110"; glyph[4] = "00001"; glyph[5] = "00001"; glyph[6] = "11110"; break;
        case 'T': glyph[0] = "11111"; glyph[1] = "00100"; glyph[2] = "00100"; glyph[3] = "00100"; glyph[4] = "00100"; glyph[5] = "00100"; glyph[6] = "00100"; break;
        case 'V': glyph[0] = "10001"; glyph[1] = "10001"; glyph[2] = "10001"; glyph[3] = "10001"; glyph[4] = "10001"; glyph[5] = "01010"; glyph[6] = "00100"; break;
        case '!': glyph[0] = "00100"; glyph[1] = "00100"; glyph[2] = "00100"; glyph[3] = "00100"; glyph[4] = "00100"; glyph[5] = "00000"; glyph[6] = "00100"; break;
        default: break;
        }
        for (int r = 0; r < 7; ++r) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[r][col] == '1') {
                    float px = cursorX + col * pixelSize;
                    float py = startY - r * pixelSize;
                    drawPart(shader, id, glm::vec3(px, py, 0.0f), glm::vec3(pixelSize * 0.92f, pixelSize * 0.92f, 1.0f), color, 3);
                }
            }
        }
        cursorX += 6.0f * pixelSize;
    }
}

void drawHumanoidZombie(unsigned int shader, const Zombi& z, float time) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, z.posicion);
    base = glm::rotate(base, z.anguloMirada, glm::vec3(0.0f, 1.0f, 0.0f));

    float dmgTint = (z.vida == 1) ? 0.35f : 0.0f;
    glm::vec3 armorColor = glm::vec3(0.38f + dmgTint, 0.40f - dmgTint * 0.5f, 0.25f);
    glm::vec3 limbColor = glm::vec3(0.25f + dmgTint, 0.28f - dmgTint * 0.4f, 0.18f);
    float swing = sin(time * 6.0f + z.faseCaminata) * 28.0f;

    drawPart(shader, base, glm::vec3(-0.25f, 0.02f, 0.25f), glm::vec3(0.95f, 0.02f, 0.65f), glm::vec3(0.0f), 4, -20.0f, glm::vec3(0, 1, 0));
    drawPart(shader, base, glm::vec3(-0.14f, 0.38f, 0.0f), glm::vec3(0.18f, 0.75f, 0.18f), limbColor, 0, swing, glm::vec3(1, 0, 0));
    drawPart(shader, base, glm::vec3(0.14f, 0.38f, 0.0f), glm::vec3(0.18f, 0.75f, 0.18f), limbColor, 0, -swing, glm::vec3(1, 0, 0));
    drawPart(shader, base, glm::vec3(0.0f, 1.02f, 0.0f), glm::vec3(0.46f, 0.62f, 0.28f), armorColor, 0);
    drawPart(shader, base, glm::vec3(0.0f, 1.48f, 0.0f), glm::vec3(0.28f, 0.28f, 0.28f), glm::vec3(0.20f, 0.25f, 0.18f), 0);
    drawPart(shader, base, glm::vec3(0.0f, 1.49f, 0.15f), glm::vec3(0.22f, 0.10f, 0.05f), glm::vec3(0.95f, 0.25f, 0.10f), 3);
    drawPart(shader, base, glm::vec3(-0.32f, 1.12f, 0.18f), glm::vec3(0.15f, 0.18f, 0.52f), limbColor, 0, -15.0f + swing * 0.3f, glm::vec3(1, 0, 0));
    drawPart(shader, base, glm::vec3(0.32f, 1.12f, 0.18f), glm::vec3(0.15f, 0.18f, 0.52f), limbColor, 0, -25.0f, glm::vec3(1, 0, 0));
}

void drawFirstPersonWeapon(unsigned int shader, float time) {
    if (estadoJuego != PLAYING) return;

    glClear(GL_DEPTH_BUFFER_BIT);
    glm::mat4 viewIdentity = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(viewIdentity));

    float bobX = sin(time * 5.0f) * 0.012f;
    float bobY = abs(cos(time * 5.0f)) * 0.012f;
    glm::mat4 gunBase = glm::mat4(1.0f);

    if (armaActual == 1) { // Rifle de Plasma
        gunBase = glm::translate(gunBase, glm::vec3(0.38f + bobX, -0.34f - bobY, -0.72f + recoilOffset));
        gunBase = glm::rotate(gunBase, glm::radians(8.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        gunBase = glm::rotate(gunBase, glm::radians(-recoilOffset * 35.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        drawPart(shader, gunBase, glm::vec3(0.05f, -0.04f, 0.15f), glm::vec3(0.26f, 0.20f, 0.42f), glm::vec3(0.55f, 0.14f, 0.06f), 0);
        drawPart(shader, gunBase, glm::vec3(0.0f, 0.00f, -0.15f), glm::vec3(0.15f, 0.14f, 0.26f), glm::vec3(0.20f, 0.70f, 0.15f), 0);
        drawPart(shader, gunBase, glm::vec3(-0.02f, -0.01f, -0.36f), glm::vec3(0.12f, 0.12f, 0.20f), glm::vec3(0.55f, 0.14f, 0.06f), 0);
        drawPart(shader, gunBase, glm::vec3(-0.01f, 0.24f, -0.26f), glm::vec3(0.045f, 0.008f, 0.01f), glm::vec3(0.75f, 0.15f, 0.65f), 3);

        if (muzzleFlashTimer > 0.0f) {
            drawPart(shader, gunBase, glm::vec3(-0.03f, 0.02f, -0.52f), glm::vec3(0.36f, 0.36f, 0.02f), glm::vec3(0.45f, 1.0f, 0.35f), 3, 45.0f, glm::vec3(0, 0, 1));
        }
    }
    else { // Pistola Táctica
        gunBase = glm::translate(gunBase, glm::vec3(0.25f + bobX, -0.28f - bobY, -0.6f + recoilOffset * 0.7f));
        gunBase = glm::rotate(gunBase, glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        gunBase = glm::rotate(gunBase, glm::radians(-recoilOffset * 45.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        drawPart(shader, gunBase, glm::vec3(0.0f, -0.08f, 0.0f), glm::vec3(0.05f, 0.18f, 0.08f), glm::vec3(0.12f), 0, 15.0f, glm::vec3(1, 0, 0));
        drawPart(shader, gunBase, glm::vec3(0.0f, 0.04f, -0.1f), glm::vec3(0.06f, 0.07f, 0.28f), glm::vec3(0.25f, 0.26f, 0.28f), 0);
        drawPart(shader, gunBase, glm::vec3(0.0f, 0.08f, 0.02f), glm::vec3(0.012f, 0.02f, 0.012f), glm::vec3(0.08f), 0);
        drawPart(shader, gunBase, glm::vec3(0.0f, 0.08f, -0.22f), glm::vec3(0.01f, 0.02f, 0.01f), glm::vec3(0.9f, 0.1f, 0.1f), 3);

        if (muzzleFlashTimer > 0.0f) {
            drawPart(shader, gunBase, glm::vec3(0.0f, 0.04f, -0.28f), glm::vec3(0.2f, 0.2f, 0.02f), glm::vec3(1.0f, 0.6f, 0.1f), 3, 20.0f, glm::vec3(0, 0, 1));
            drawPart(shader, gunBase, glm::vec3(0.0f, 0.04f, -0.26f), glm::vec3(0.1f, 0.1f, 0.02f), glm::vec3(1.0f, 0.9f, 0.5f), 3, 70.0f, glm::vec3(0, 0, 1));
        }
    }
}

void drawHUD(unsigned int shader, GLFWwindow* window) {
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 orthoProj = glm::ortho(0.0f, (float)winW, 0.0f, (float)winH, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(orthoProj));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glm::mat4 id = glm::mat4(1.0f);

    if (estadoJuego == PLAYING) {
        // Mira central
        drawPart(shader, id, glm::vec3(winW / 2.0f, winH / 2.0f, 0.0f), glm::vec3(4.0f, 16.0f, 1.0f), glm::vec3(1.0f), 3);
        drawPart(shader, id, glm::vec3(winW / 2.0f, winH / 2.0f, 0.0f), glm::vec3(16.0f, 4.0f, 1.0f), glm::vec3(1.0f), 3);

        // Indicador visual de balas disponibles
        int municionActual = (armaActual == 1) ? municionArma1 : municionArma2;
        glm::vec3 hudColor = (armaActual == 1) ? glm::vec3(0.3f, 0.9f, 0.3f) : glm::vec3(0.9f, 0.7f, 0.2f);
        for (int i = 0; i < municionActual; i++) {
            float posX = winW - 30.0f - (i * 18.0f);
            drawPart(shader, id, glm::vec3(posX, 35.0f, 0.0f), glm::vec3(10.0f, 28.0f, 1.0f), hudColor, 3);
        }
    }
    else if (estadoJuego == GAME_OVER) {
        drawPart(shader, id, glm::vec3(winW * 0.5f, winH * 0.5f, 0.0f), glm::vec3(680.0f, 220.0f, 1.0f), glm::vec3(0.12f, 0.0f, 0.0f), 3);
        drawText2D(shader, "GAME OVER", winW * 0.5f, winH * 0.55f, 10.0f, glm::vec3(1.0f, 0.2f, 0.2f));
        drawText2D(shader, "PRESIONA R PARA REINICIAR", winW * 0.5f, winH * 0.42f, 4.0f, glm::vec3(1.0f, 0.85f, 0.85f));
    }
    else if (estadoJuego == VICTORY) {
        drawPart(shader, id, glm::vec3(winW * 0.5f, winH * 0.5f, 0.0f), glm::vec3(940.0f, 220.0f, 1.0f), glm::vec3(0.0f, 0.18f, 0.10f), 3);
        drawText2D(shader, "MISION COMPLETADA!", winW * 0.5f, winH * 0.55f, 8.0f, glm::vec3(0.25f, 1.0f, 0.45f));
        drawText2D(shader, "PRESIONA R PARA REINICIAR", winW * 0.5f, winH * 0.42f, 4.0f, glm::vec3(0.85f, 1.0f, 0.90f));
    }

    glEnable(GL_DEPTH_TEST);
}

void resetGame(GLFWwindow* window) {
    estadoJuego = PLAYING;
    cameraPos = glm::vec3(0.0f, 1.15f, 5.5f);
    yaw = -90.0f;
    pitch = 0.0f;
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    armaActual = 1;
    municionArma1 = 15;
    municionArma2 = 12;
    muzzleFlashTimer = 0.0f;
    recoilOffset = 0.0f;

    for (auto& z : zombis) {
        z.posicion = z.spawnPos;
        z.vida = 2;
        z.activo = true;
        z.respawnTimer = 0.0f;
    }
    for (auto& c : cajas) {
        c.activa = true;
    }

    glfwSetWindowTitle(window, "Operacion Extraccion - T3");
    std::cout << "\n--- PARTIDA REINICIADA ---" << std::endl;
}

int main() {
    if (GetConsoleWindow() == NULL) {
        AllocConsole();
        FILE* stream;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Operacion Extraccion - T3", NULL, NULL);
    if (window == NULL) { std::cout << "Fallo GLFW" << std::endl; return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "INICIO DE MISION: Sobrevive y alcanza la plataforma verde de extraccion." << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (muzzleFlashTimer > 0.0f) muzzleFlashTimer -= deltaTime;
        if (recoilOffset > 0.0f) recoilOffset = glm::max(0.0f, recoilOffset - deltaTime * 0.6f);

        processInput(window);

        if (estadoJuego == GAME_OVER) glClearColor(0.25f, 0.02f, 0.02f, 1.0f);
        else if (estadoJuego == VICTORY) glClearColor(0.02f, 0.25f, 0.14f, 1.0f);
        else glClearColor(0.05f, 0.07f, 0.05f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glm::vec3 flashColor = (armaActual == 1) ? glm::vec3(0.2f, 1.0f, 0.25f) : glm::vec3(1.0f, 0.7f, 0.2f);
        float flashVal = (muzzleFlashTimer > 0.0f) ? 1.0f : 0.0f;
        glUniform1f(glGetUniformLocation(shaderProgram, "flashIntensity"), flashVal);
        glUniform3fv(glGetUniformLocation(shaderProgram, "flashColor"), 1, glm::value_ptr(flashColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "playerPos"), 1, glm::value_ptr(cameraPos));

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glm::mat4 world = glm::mat4(1.0f);

        // 1. RENDERIZAR BÚNKER (RF03)
        drawPart(shaderProgram, world, glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(16.0f, 0.2f, 26.0f), glm::vec3(0.35f, 0.52f, 0.25f), 2);
        drawPart(shaderProgram, world, glm::vec3(0.0f, 2.25f, -12.5f), glm::vec3(16.0f, 4.7f, 0.4f), glm::vec3(1.0f), 1);
        drawPart(shaderProgram, world, glm::vec3(0.0f, 2.25f, 9.5f), glm::vec3(16.0f, 4.7f, 0.4f), glm::vec3(1.0f), 1);
        drawPart(shaderProgram, world, glm::vec3(-7.8f, 2.25f, -1.5f), glm::vec3(0.4f, 4.7f, 22.0f), glm::vec3(1.0f), 1);
        drawPart(shaderProgram, world, glm::vec3(7.8f, 2.25f, -1.5f), glm::vec3(0.4f, 4.7f, 22.0f), glm::vec3(1.0f), 1);

        glm::vec3 doorPos = glm::vec3(-2.5f, 0.0f, -9.25f);
        drawPart(shaderProgram, world, doorPos + glm::vec3(-0.95f, 1.35f, 0.0f), glm::vec3(0.18f, 2.7f, 0.25f), glm::vec3(0.28f, 0.18f, 0.08f), 0);
        drawPart(shaderProgram, world, doorPos + glm::vec3(0.95f, 1.35f, 0.0f), glm::vec3(0.18f, 2.7f, 0.25f), glm::vec3(0.28f, 0.18f, 0.08f), 0);
        drawPart(shaderProgram, world, doorPos + glm::vec3(0.0f, 2.75f, 0.0f), glm::vec3(2.1f, 0.18f, 0.25f), glm::vec3(0.28f, 0.18f, 0.08f), 0);

        // ZONA DE EXTRACCIÓN (RF08)
        drawPart(shaderProgram, world, glm::vec3(-2.5f, 0.05f, -11.5f), glm::vec3(2.4f, 0.15f, 2.4f), glm::vec3(0.2f, 1.0f, 0.4f), 3);

        // Validación de Victoria al pisar la plataforma verde
        if (estadoJuego == PLAYING && cameraPos.z <= -9.8f && cameraPos.x >= -4.2f && cameraPos.x <= -0.8f) {
            estadoJuego = VICTORY;
            glfwSetWindowTitle(window, "¡MISION COMPLETADA! Has llegado a la zona de extraccion.");
            std::cout << std::endl << "========================================================" << std::endl;
            std::cout << "  ¡MISION COMPLETADA! Has llegado a la zona de extraccion. " << std::endl;
            std::cout << "========================================================" << std::endl;
        }

        // Cajas de Munición (RF10 y RF02)
        for (auto& caja : cajas) {
            if (caja.activa) {
                drawPart(shaderProgram, world, caja.posicion, glm::vec3(0.5f, 0.35f, 0.35f), glm::vec3(0.2f, 0.45f, 0.15f), 0, currentFrame * 45.0f, glm::vec3(0, 1, 0));
                if (estadoJuego == PLAYING && glm::distance(glm::vec2(cameraPos.x, cameraPos.z), glm::vec2(caja.posicion.x, caja.posicion.z)) < 1.2f) {
                    caja.activa = false;
                    municionArma1 += 10;
                    municionArma2 += 8;
                    std::cout << "+ RECARGA! Balas A1: " << municionArma1 << " | A2: " << municionArma2 << std::endl;
                }
            }
        }

        // 2. RENDERIZAR ZOMBIS / LÓGICA DE PERSECUCIÓN Y GAME OVER (RF05 y RF09)
        for (auto& zombi : zombis) {
            if (zombi.activo) {
                glm::vec3 dir = cameraPos - zombi.posicion;
                dir.y = 0.0f;
                float distancia = glm::length(dir);

                if (estadoJuego == PLAYING && distancia < 1.15f) {
                    estadoJuego = GAME_OVER;
                    glfwSetWindowTitle(window, "GAME OVER - Has sido atrapado por un zombi");
                    std::cout << std::endl << "========================================================" << std::endl;
                    std::cout << "         GAME OVER - ¡UN ZOMBI TE HA ATRAPADO!         " << std::endl;
                    std::cout << "========================================================" << std::endl;
                }

                if (estadoJuego == PLAYING && distancia > 0.001f) {
                    glm::vec3 dirNorm = glm::normalize(dir);
                    zombi.anguloMirada = atan2(dirNorm.x, dirNorm.z);
                    if (distancia < 12.0f && distancia > 0.9f) {
                        zombi.posicion += dirNorm * 1.8f * deltaTime;
                        zombi.faseCaminata += deltaTime * 5.0f;
                    }
                }
                drawHumanoidZombie(shaderProgram, zombi, currentFrame);
            }
            else if (estadoJuego == PLAYING) {
                zombi.respawnTimer -= deltaTime;
                if (zombi.respawnTimer <= 0.0f) {
                    zombi.activo = true;
                    zombi.vida = 2;
                    zombi.posicion = zombi.spawnPos;
                    std::cout << "¡Un zombi ha reaparecido en su punto inicial!" << std::endl;
                }
            }
        }

        // 3. RENDERIZAR ARMA Y HUD
        drawFirstPersonWeapon(shaderProgram, currentFrame);
        drawHUD(shaderProgram, window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (estadoJuego != PLAYING) {
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            resetGame(window);
        }
        return;
    }

    float cameraSpeed = 5.0f * deltaTime;
    glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * cameraSpeed;

    cameraPos.y = 1.15f;
    cameraPos.x = glm::clamp(cameraPos.x, -7.0f, 7.0f);
    cameraPos.z = glm::clamp(cameraPos.z, -12.0f, 8.6f);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && armaActual != 1) {
        armaActual = 1;
        std::cout << "Rifle Equipado." << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && armaActual != 2) {
        armaActual = 2;
        std::cout << "Pistola Equipada." << std::endl;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (estadoJuego != PLAYING) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        int* municionActual = (armaActual == 1) ? &municionArma1 : &municionArma2;

        if (*municionActual > 0) {
            (*municionActual)--;
            muzzleFlashTimer = 0.09f;
            recoilOffset = (armaActual == 1) ? 0.08f : 0.12f;

            for (auto& zombi : zombis) {
                if (zombi.activo) {
                    glm::vec3 centroZombi = zombi.posicion + glm::vec3(0.0f, 1.0f, 0.0f);
                    glm::vec3 dirZombi = glm::normalize(centroZombi - cameraPos);
                    float distanciaAlZombi = glm::length(centroZombi - cameraPos);

                    if (glm::dot(cameraFront, dirZombi) > 0.94f && distanciaAlZombi < 16.0f) {
                        zombi.vida -= 1;
                        if (zombi.vida <= 0) {
                            zombi.activo = false;
                            zombi.respawnTimer = 5.0f;
                            std::cout << "¡Zombi eliminado! (Revivira pronto)" << std::endl;
                        }
                        break;
                    }
                }
            }
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (estadoJuego != PLAYING) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    yaw += (xpos - lastX) * 0.1f;
    pitch += (lastY - ypos) * 0.1f;
    lastX = xpos; lastY = ypos;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}