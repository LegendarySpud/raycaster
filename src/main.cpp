
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include "../include/shader.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void castRay(glm::vec2 start_pos, glm::vec2 ray_dir, int* grid, int grid_sx, int grid_sy, float* pDist, int* wall_type, float* tex_x);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool isColour(float* col_1, float* col_2);
void setColour(float* col, float r, float g, float b);

class Player {
    public:
        glm::vec2 pos;
        float ang;
        glm::vec2 ang_dir;
        float fov;
        float vfov;
        float eye_lev;
        Player(float ratio, glm::vec2 aPos, float aAng, float aFov, float wall_h) {
            pos = aPos;
            ang = aAng;
            ang_dir.x = cos(ang);
            ang_dir.y = sin(ang);
            fov = aFov;
            vfov = 2.0f * atan(tan(fov*0.5f)/ratio);
            eye_lev = 0.75f*wall_h;
        }
        
        void setAng(float newAng) {
            ang = newAng;
            ang_dir.x = cos(ang);
            ang_dir.y = sin(ang);
        }
};

float PI = 3.141592653589f;

char title[] = "Raycaster";
int scr_x = 700;
int scr_y = 500;
float scr_rat = (float)scr_x/(float)scr_y;

float rect[] = {
    // Position
    -1.0f, 1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
    -1.0f,-1.0f, 0.0f,
    
    -1.0f,-1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
     1.0f,-1.0f, 0.0f
};

/*int map[] = {
    1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 2, 1,
    1, 1, 1, 1, 1, 1, 1,
};
int map_x = 7;
int map_y = 7;*/
int map_x, map_y;
float wall_height = 4.0f;

Player player(scr_rat, glm::vec2(2.5f, 3.45f), glm::radians(0.0f), 30.0f, wall_height);
float player_speed = 3.0f;

bool first_mouse = false;
double last_x, last_y; 
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
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD initialisation failed.\n";
        return 0;
    }
    
    // settings
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);
    
    // init viewport
    int fbx, fby;
    glfwGetFramebufferSize(window, &fbx, &fby);
    glViewport(0, 0, fbx, fby);
    
    // init shaders
    Shader mapShader("src/shaders/vMapShader.glsl", "src/shaders/fShader.glsl");
    Shader mapPlayerShader("src/shaders/vMapPlayerShader.glsl", "src/shaders/fShader.glsl");
    Shader columnShader("src/shaders/vColumnShader.glsl", "src/shaders/fShader2.glsl");
    
    // collumn vertices
    // pos, wall type, texture_pos
    int lines_stride = 6;
    float lines[fbx*lines_stride*2];
    //std::cout << sizeof(lines)/sizeof(float) << "\n";
    for (int i = 0; i < fbx; i++) {
        lines[i*lines_stride*2+0] = ((float)i / fbx) * 2 - 1;
        lines[i*lines_stride*2+1] = 0.5f;
        lines[i*lines_stride*2+2] = 1.0f;
        lines[i*lines_stride*2+3] = 0.0f;
        lines[i*lines_stride*2+4] = ((float)i / fbx) * 2 - 1;
        lines[i*lines_stride*2+5] = 1.0f;
        lines[i*lines_stride*2+6] = ((float)i / fbx) * 2 - 1;
        lines[i*lines_stride*2+7] = 0.5f;
        lines[i*lines_stride*2+8] = 1.0f;
        lines[i*lines_stride*2+9] = 0.0f;
        lines[i*lines_stride*2+10] = ((float)i / fbx) * 2 - 1;
        lines[i*lines_stride*2+11] = 0.0f;
    }
    
    // create lines vbo, vao
    unsigned int linesVBO, linesVAO;
    glGenVertexArrays(1, &linesVAO);
    glGenBuffers(1, &linesVBO);
    glBindVertexArray(linesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, lines_stride*sizeof(float), (void*)0);  // pos
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, lines_stride*sizeof(float), (void*)(3*sizeof(float)));  // wall type
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, lines_stride*sizeof(float), (void*)(4*sizeof(float)));  // texture coord
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
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
    
    // map loading
    //stbi_set_flip_vertically_on_load();
    int width, height, nrChannels;
    unsigned char *data = stbi_load("maps/map1/walls.png", &width, &height, &nrChannels, 0);
    std::cout << nrChannels << "\n";
    if (!data) std::cout << "Map did not load.\n";
    map_x = width;
    map_y = height;
    int map[width * height];
    for (int i=0; i<width*height; i++) {
        float colour[] = {
            (float)data[i*nrChannels+0]/256.0f,
            (float)data[i*nrChannels+1]/256.0f,
            (float)data[i*nrChannels+2]/256.0f
        };
        float tile_colour[] = {1.0f, 1.0f, 1.0f};
        if (isColour(colour, tile_colour)) map[i] = 0;
        setColour(tile_colour, 0.0f, 0.0f, 0.0f);
        if (isColour(colour, tile_colour)) map[i] = 1;
    }
    std::cout << map_x << "\n";
    std::cout << map_y << "\n";

    // texture loading
    //
    data = stbi_load("assets/bricks.png", &width, &height, &nrChannels, 0);
    if (!data) std::cout << "Texture did not load.\n";
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    // parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float prev_t = 0.0f;
    
    while(!glfwWindowShouldClose(window)) {
        t = glfwGetTime();
        dt = t - prev_t;
        // std::cout << 1.0f/dt << "\n";
        prev_t = t;
        //player.eye_lev = sin(t)*wall_height/2.0f + wall_height/2.0f;
        
        if (window_to_resize == true && t-window_resize_time>0.3f) {
            window_to_resize = false;
            /*fbx = resize_x;
            fby = resize_y;
            glViewport(0, 0, fbx, fby);*/
        }
        processInput(window);
        std::cout << "(" << player.pos.x << ", " << player.pos.y << ") " << player.ang << "\n";
        
        glClearColor(0.0, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
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
        
        for (int i=0; i<fbx; i++) {
            float ray_ang;
            float ray_dist;
            int wall_type;
            float tex_x;
            float ratio = ((float)i/(float)fbx)*2-1;
            glm::vec2 player_dir = player.ang_dir;
            glm::vec2 plane = {-player_dir.y, player_dir.x};
            plane *= tan(player.fov/2.0f);
            float camera_x = 2.0f * i / float(fbx) - 1.0f;
            glm::vec2 ray_dir = player_dir + plane * camera_x;
            ray_dir = glm::normalize(ray_dir);
            castRay(player.pos, ray_dir, map, map_x, map_y, &ray_dist, &wall_type, &tex_x);
            float corrected_dist = dot(ray_dir, player_dir) * ray_dist;
            float proj_scale = 1.0f/(2*tan(player.vfov/2.0f));
            lines[i*lines_stride*2+0] = ratio;
            lines[i*lines_stride*2+1] = (wall_height)/corrected_dist*proj_scale;
            lines[i*lines_stride*2+2] = ray_dist;
            lines[i*lines_stride*2+3] = wall_type;
            lines[i*lines_stride*2+4] = tex_x/wall_height;
            // number 5 is tex_y, not changed
            lines[i*lines_stride*2+6] = ratio;
            lines[i*lines_stride*2+7] = 0-(wall_height)/corrected_dist*proj_scale;
            lines[i*lines_stride*2+8] = ray_dist;
            lines[i*lines_stride*2+9] = wall_type;
            lines[i*lines_stride*2+10] = tex_x/wall_height;
            // number 11 is tex_y, not changed
        }
        
        columnShader.use();
        glm::vec3 colour(1.0f, 1.0f, 1.0f);
        columnShader.setVec3("aColour", colour);
        glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);
        glBindVertexArray(linesVAO);
        glDrawArrays(GL_LINES, 0, fbx*2);
        
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

void castRay(glm::vec2 start_pos, glm::vec2 ray_dir, int* grid, int grid_sx, int grid_sy, float* pDist, int* wall_type, float* tex_x) {
    float ray_x = start_pos.x;
    float ray_y = start_pos.y;
    int grid_x = int(ray_x);
    int grid_y = int(ray_y);
    float dist_x, dist_y;
    float step_x = (ray_dir.x == 0) ? 9999.0f : std::abs(1.0f/ray_dir.x);
    float step_y = (ray_dir.y == 0) ? 9999.0f : std::abs(1.0f/ray_dir.y);
    float dist;
    float maxDist = 100.0f;
    int grid_step_x, grid_step_y;
    bool hit = false;
    int side;
    
    // Step calculations
    if (ray_dir.x < 0) {
        grid_step_x = -1;
        dist_x = (ray_x - grid_x) * step_x;
    }
    else  {
        grid_step_x = 1;
        dist_x = (grid_x + 1.0 - ray_x) * step_x;
    }
    if (ray_dir.y < 0) {
        grid_step_y = -1;
        dist_y = (ray_y - grid_y) * step_y;
    }
    else  {
        grid_step_y = 1;
        dist_y  = (grid_y + 1.0 - ray_y) * step_y;
    }
    
    // DDA
    int grid_val;
    while (!hit) {
        if (dist_x < dist_y) {
            dist_x += step_x;
            grid_x += grid_step_x;
            side = 0;
        }
        else {
            dist_y += step_y;
            grid_y += grid_step_y;
            side = 1;
        }
        if (grid_x < 0 || grid_x >= grid_sx || grid_y < 0 || grid_y >= grid_sy) {
            break;
        }
        grid_val = grid[grid_y*grid_sy + grid_x];
        if (grid_val != 0) hit = true;
    }
    if (side == 0) dist = dist_x - step_x;
    else dist = dist_y - step_y;
    
    float perpDist;
    if (side == 0)
        perpDist = (grid_x - start_pos.x + (1 - grid_step_x) * 0.5f) / ray_dir.x;
    else
        perpDist = (grid_y - start_pos.y + (1 - grid_step_y) * 0.5f) / ray_dir.y;


    *pDist = dist;
    *wall_type = grid_val;
    if (side == 0) *tex_x = start_pos.y + perpDist * ray_dir.y;
    else if (side == 1) *tex_x = start_pos.x + perpDist * ray_dir.x;
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
    
    player.setAng(player.ang - xoffset * dt * 0.2);
    
    glm::vec2 direction;
}

bool isColour(float* col_1, float* col_2) {
    float margin = 0.05;
    bool is = (abs(col_1[0] - col_2[0]) <= margin) and
              (abs(col_1[1] - col_2[1]) <= margin) and
              (abs(col_1[2] - col_2[2]) <= margin);

    return is;
}

void setColour(float* col, float r, float g, float b) {
    col[0] = r;
    col[1] = g;
    col[2] = b;
};
