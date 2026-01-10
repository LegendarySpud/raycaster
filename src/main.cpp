
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <shader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <camera.h>
#define STB_IMAGE_IMPLEMENTATION
#include <shader.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void castRay(glm::vec2 start_pos, glm::vec2 ray_dir, int* grid, int grid_size, glm::vec2 &out_pos);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

float PI = 3.141592653589f;
char title[] = "Raycaster";
int scr_x = 700;
int scr_y = 500;
float scr_rat = (float)scr_x/(float)scr_y;

class Player {
    public:
        glm::vec2 pos;
        float ang;
        glm::vec2 ang_dir;
        Camera cam;
        float fov;
        Player(float ratio, glm::vec2 aPos, float aAng, float aFov) {
            pos = aPos;
            setAng(aAng);
            ang_dir.x = cos(ang);
            ang_dir.y = sin(ang);
            fov = aFov;
            updateCam(ratio);
        }

        void updateCam(float screen_ratio) {
            if (ang > 360.0f) ang -= 360.0f;
            if (ang < 0.0f) ang += 360.0f;
            cam.Pos = glm::vec3(pos.x, 0.0f, pos.y);
            cam.FOV = fov*scr_rat;
            cam.Pitch = 0.0f;
            cam.Yaw = ang;
        }
        
        void setAng(float newAng) {
            ang = newAng;
            ang_dir.x = cos(glm::radians(ang));
            ang_dir.y = sin(glm::radians(ang));
        }
};

float rect[] = {
    // Position
    -1.0f, 1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
    -1.0f,-1.0f, 0.0f,
    
    -1.0f,-1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
     1.0f,-1.0f, 0.0f
};

int map[] = {
    1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1,
}; 
int map_area = 49;
int map_length = 7;

bool first_mouse = false;
double last_x = scr_x/2.0f;
double last_y = scr_y/2.0f;

Player player((float)scr_x/(float)scr_y, glm::vec2(2.5f, 2.5f), 0.0f, 30.0f);
float player_speed = 3.0f;

bool window_to_resize = false;
float window_resize_time = -1.0f;
int resize_x, resize_y;
float t;
float dt;

int main() {
    std::cout << title << "\n";

    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // init window
    GLFWwindow* window = glfwCreateWindow(scr_x, scr_y, title, NULL, NULL);
    if (window == NULL) {
        std::cout << "GLFW window creation failed.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD initialisation failed.\n";
        return 0;
    }
    
    // settings
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // init viewport
    glViewport(0, 0, scr_x, scr_y);
    
    // init shaders
    Shader mapShader("src/shaders/vMapShader.glsl", "src/shaders/fShader.glsl");
    Shader mapPlayerShader("src/shaders/vMapPlayerShader.glsl", "src/shaders/fShader.glsl");
    Shader columnShader("src/shaders/vColumnShader.glsl", "src/shaders/fShader2.glsl");
    
    // column vertices
    // pos
    int lines_stride = 3;
    float lines[scr_x*lines_stride*2];
    /*float lines[6] = {
        0.0f, 0.5f, 1.0f,
        0.0f, -0.5f, 1.0f
    };*/
    std::cout << sizeof(lines)/sizeof(float) << "\n";
    
    // create lines vbo, vao
    unsigned int linesVBO, linesVAO;
    glGenVertexArrays(1, &linesVAO);
    glGenBuffers(1, &linesVBO);
    glBindVertexArray(linesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, lines_stride*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(linesVAO);
    
    // create rect vbo, vao
    unsigned int rectVBO, rectVAO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(rectVAO);
    
    float prev_t = 0.0f;
    
    std::cout << player.cam.FOV << "\n";

    while(!glfwWindowShouldClose(window)) {
        t = glfwGetTime();
        dt = t - prev_t;
        // std::cout << 1.0f/dt << "\n";
        prev_t = t;
        //player.ang = t; 
        
        if (window_to_resize == true && t-window_resize_time>0.3f) {
            window_to_resize = false;
            scr_x = resize_x;
            scr_y = resize_y;
            glViewport(0, 0, scr_x, scr_y);
        }
        processInput(window);
        
        glClearColor(0.0, 0.0f, 0.2f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        player.updateCam(scr_rat);
        player.cam.updateCameraVectors();
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(player.cam.Pos, player.cam.Pos + player.cam.Front, player.cam.Up);
        glm::mat4 projection;
        projection = glm::perspective(glm::radians(player.fov/scr_rat), scr_rat, 0.1f, 100.0f);
        
        /*
        mapShader.use();
        glm::vec3 colour;
        for (int y=0; y<map_length; y++) {
            for (int x=0; x<map_length; x++) {
                int value = map[y*map_length + x];
                if (value == 0) colour = glm::vec3(0.0f, 0.0f, 0.0f);
                if (value == 1) colour = glm::vec3(1.0f, 0.0f, 1.0f);
                float scale = 1.0f/(float)map_length;
                mapShader.setFloat("scale", scale);
                mapShader.setVec2("pos", glm::vec2((float)x, (float)y));
                mapShader.setVec3("colour", colour);
                glBindVertexArray(rectVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        */
        
        for (int i=0; i<scr_x; i++) {
            float ray_ang;
            float ray_dist;
            glm::vec2 ray_pos;
            float ratio = (float)(i*2-1);
            glm::vec2 player_dir = player.ang_dir;
            glm::vec2 plane = {-player_dir.y, player_dir.x};
            plane *= tan((glm::radians(player.fov))/2.0f);
            float camera_x = 2.0f * i / float(scr_x) - 1.0f;
            glm::vec2 ray_dir = player_dir + plane * camera_x;
            ray_dir = glm::normalize(ray_dir);
            castRay(player.pos, ray_dir, map, map_length, ray_pos);
            //float corrected_dist = dot(ray_dir, player_dir) * ray_dist;
            lines[i*lines_stride*2+0] = ray_pos.x;
            lines[i*lines_stride*2+1] = 0.5f;
            lines[i*lines_stride*2+2] = ray_pos.y;
            lines[i*lines_stride*2+3] = ray_pos.x;
            lines[i*lines_stride*2+4] = -0.5f;
            lines[i*lines_stride*2+5] = ray_pos.y;
            //ray_ang = atan2(ray_dir.y, ray_dir.x);
            //std::cout << ray_dist << "\n";
        }
        
        columnShader.use();
        glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);
        glBindVertexArray(linesVAO);
        columnShader.setMat4("view", view);
        columnShader.setMat4("projection", projection);
        glDrawArrays(GL_LINES, 0, 2*scr_x);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteVertexArrays(1, &linesVAO);
    glDeleteBuffers(1, &linesVBO);
    mapShader.del();
    columnShader.del();

    glfwTerminate();
    return 0;
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_to_resize = true;
    window_resize_time = t;
    resize_x = width;
    resize_y = height;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        player.pos += player.ang_dir * dt * player_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        player.pos -= player.ang_dir * dt * player_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec2 perp_dir;
        perp_dir.x = player.ang_dir.y;
        perp_dir.y = -player.ang_dir.x;
        player.pos -= perp_dir * dt * player_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec2 perp_dir;
        perp_dir.x = -player.ang_dir.y;
        perp_dir.y = player.ang_dir.x;
        player.pos -= perp_dir * dt * player_speed;
    }
}

void castRay(glm::vec2 start_pos, glm::vec2 ray_dir, int* grid, int grid_size, glm::vec2 &out_pos) {
    float ray_x = start_pos.x;
    float ray_y = start_pos.y;
    int map_x = int(ray_x);
    int map_y = int(ray_y);
    float dist_x, dist_y;
    float step_x = (ray_dir.x == 0) ? 9999.0f : std::abs(1.0f/ray_dir.x);
    float step_y = (ray_dir.y == 0) ? 9999.0f : std::abs(1.0f/ray_dir.y);
    float dist;
    float maxDist = 100.0f;
    int map_step_x, map_step_y;
    bool hit = false;
    int side = 0;
    
    // Step calculations
    if (ray_dir.x < 0) {
        map_step_x = -1;
        dist_x = (ray_x - map_x) * step_x;
    }
    else  {
        map_step_x = 1;
        dist_x = (map_x + 1.0 - ray_x) * step_x;
    }
    if (ray_dir.y < 0) {
        map_step_y = -1;
        dist_y = (ray_y - map_y) * step_y;
    }
    else  {
        map_step_y = 1;
        dist_y  = (map_y + 1.0 - ray_y) * step_y;
    }
    
    // DDA
    while (!hit) {
        if (dist_x < dist_y) {
            dist_x += step_x;
            map_x += map_step_x;
            side = 0;
        }
        else {
            dist_y += step_y;
            map_y += map_step_y;
            side = 1;
        }
        if (map_x < 0 || map_x >= grid_size || map_y < 0 || map_y >= grid_size) {
            break;
        }
        if (grid[map_y*grid_size + map_x] != 0) hit = true;
    }
    if (side == 0) dist = dist_x - step_x;
    else dist = dist_y - step_y;
    
    out_pos = start_pos + ray_dir * dist;
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (first_mouse) {
        last_x = xpos;
        last_y = ypos;
        first_mouse = false;
    }
    
    float xoffset = xpos - last_x;
    float yoffset = last_y - ypos;
    last_x = xpos;
    last_y = ypos;
    
    player.setAng(player.ang - xoffset * 0.02);
}
