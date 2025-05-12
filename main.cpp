#include <GL/glew.h> //OpenGL Extension Wrangler Library
#include <GL/freeglut.h> //OpenGL Utility Toolkit
#include <SDL2/SDL_mixer.h> // responsible for running audio (Simple DirectMedia Layer)
#include <cmath>
#include <ctime>
#include <string>

// This tells the compiler to include the implementation of stb_image functions in this file.
// It should be defined in only ONE .cpp file to avoid multiple definition errors during linking.
#define STB_IMAGE_IMPLEMENTATION 

#include <stb/stb_image.h> // Loads the stb_image library to read image files and get pixel data for textures


enum GameState { PLAYING, PAUSED, GAME_OVER, WIN };// Defines the different possible states the game can be in
GameState gameState = PLAYING;

// maze dimensions
const int mazeWidth = 13;
const int mazeHeight = 13;
int maze[mazeHeight][mazeWidth] = { // 1 wall, 0 path, 2 gold
    {1,1,1,1,1,1,0,1,1,1,1,1,1},
    {1,0,0,0,1,0,2,0,0,0,0,0,1},
    {1,0,1,0,0,0,1,1,1,1,1,0,1},
    {1,0,1,0,1,0,1,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,1,1,0,0,0,1},
    {1,2,1,0,1,0,0,0,1,1,1,0,1},
    {1,1,1,0,1,0,1,0,1,0,0,0,1},
    {1,0,0,0,1,1,1,0,0,0,1,1,1},
    {1,0,1,1,1,0,1,0,1,1,1,2,1},
    {1,0,1,0,0,0,1,0,0,0,1,0,1},
    {1,0,1,0,1,0,1,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,1,1,1,1,1,1}
};

float playerX = 6.5f, playerY = 14.0f; 
float playerAngle = -89.6f; // Player's viewing angle in radians (0 = facing positive X-axis)
float moveSpeed = 0.04f; // 0.08 of a 1 cell
int score = 0; // Current score (incremented when collecting items)
int health = 100; // Player's health (0 = game over, starts at full health)
time_t startTime; // Timestamp for tracking game duration (initialized at game start)

float enemyX = 3.5f, enemyY = 5.5f; 
float enemySpeed = 0.03f;

float goalX = 6.5f, goalY = -1.0f;

int mouseX = 400, mouseY = 300;
bool mouseEnabled = true;
float mouseSensitivity = 0.001f;

float goldRotation = 0.0f;

//GLfloat: A data type for floating-point numbers in OpenGL. It ensures compatibility with OpenGL’s internal calculations.
//The light is placed at coordinates (5, 5, 5) in 3D world space.
// 1.0f Indicates this is a positional light (not directional). A value of 0.0f would mean the light is directional (like sunlight).
GLfloat lightPos[] = {5.0f, 5.0f, 5.0f, 1.0f};


// GLuint: OpenGL's unsigned integer type for texture object identifiers
GLuint wallTexture, floorTexture, collectibleTexture;

//Mix_Chunk: SDL_mixer structure containing loaded audio data
// Pointers to SDL_mixer sound effect chunks
Mix_Chunk *collectSound, *winSound, *hurtSound;

// Loads an image file and creates an OpenGL texture from it
// Returns the OpenGL texture ID (or 0 on failure)
GLuint loadTexture(const char* path) {

/*////////////////////////////////////////////////////////////////////
//  Declares three integer variables to store metadata about the image
//  width: The image’s width in pixels (e.g., 1024)
//  height: The image’s height in pixels (e.g., 768)
//  channels: The number of color channels in the image
//  3 = RGB (red, green, blue)
//  4 = RGBA (RGB + alpha/transparency)
//  1 = Grayscale (single channel)
*/////////////////////////////////////////////////////////////////////
    int width, height, channels;

/*////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Uses the stbi_load function from the stb_image library to load pixel data from an image file into memory.
//  path: The file path to the image (e.g., "textures/wall.jpg").
//  &width, &height, &channels: Pointers to the variables declared earlier.
//  stbi_load will update these variables with the image’s actual dimensions and channel count.
//  0: Instructs stbi_load to use the image’s original channel count (no forced conversion).
//  data: A pointer to the raw pixel data in memory.
//  Format: Each pixel is represented as channels bytes (e.g., RGB = 3 bytes per pixel).
//  Example: A 100x100 RGB image will have 100 * 100 * 3 = 30,000 bytes of data.
*/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4); // Will store image dimensions and color channels (RGB/RGBA/Gray)


// Check if the image failed to load (data is null)
    if (!data) {
        // Print error message with stb_image's failure reason
        printf("Failed to load picture: %s\n", stbi_failure_reason());
        return 0;
    }
    
    GLuint texture;// Declare a texture ID variable
    glGenTextures(1, &texture); // Generate one texture ID and store it in 'texture' (ID اديلو)

    
    glBindTexture(GL_TEXTURE_2D, texture);  // Bind the texture so that subsequent texture commands apply to this one

    /*///////////////////////////////////////////////
    // glTexParameteri(...)  الوظيفة العامة للدالة:
    // بتحدد خصائص أو إعدادات للتيكستشر المرتبطة حاليًا (اللي ربطناها بـ glBindTexture).
    *////////////////////////////////////////////////
    
    // Set texture wrapping mode for the S (horizontal) axis to repeat the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // Set texture wrapping mode for the T (vertical) axis to repeat the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // When the texture is displayed smaller (like when the object is far away), 
    // use mipmaps and smooth blending between levels for better quality
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // When the texture is zoomed in (object is close), use smooth scaling between pixels
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLenum format = GL_RGBA; // Set default texture format to RGB (3 channels: Red, Green, Blue)

    
    // Upload the image pixel data to the currently bound 2D texture on the GPU.
    // Parameters:
    // - GL_TEXTURE_2D: Specifies that we're working with a 2D texture.
    // - 0: The mipmap level (0 = base image level).
    // - format: Internal format to store the texture in GPU memory (e.g., GL_RGB or GL_RGBA).
    // - width, height: Dimensions of the texture image in pixels.
    // - 0: Border width (must always be 0).
    // - format: Format of the pixel data being passed (must match the number of channels in the image).
    // - GL_UNSIGNED_BYTE: Specifies the data type of each color component (each channel is 1 byte).
    // - data: Pointer to the image data loaded from disk (e.g., via stbi_load).
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 

    // Automatically generate mipmaps for the texture (used for better scaling and performance)
    glGenerateMipmap(GL_TEXTURE_2D);
    

    // Free the image data from CPU memory after it has been uploaded to the GPU.
    // This prevents memory leaks since the raw pixel data is no longer needed.
    stbi_image_free(data);

    // Return the generated OpenGL texture ID so it can be used for rendering later.
    return texture;
}

void initAudio() {
    // Initialize the SDL_mixer audio system with default settings:
    // 44100 Hz sample rate, default format, 2 channels (stereo), and 2048 byte buffer size
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        // If audio initialization fails, print the error message
        printf("Sound configuration error: %s\n", Mix_GetError());
    }

    // Load the "collect" sound effect from file into memory
    collectSound = Mix_LoadWAV("assets/sounds/collect.wav");
    if(!collectSound) printf("Error loading the plural sound: %s\n", Mix_GetError());
    
    // Load the "win" sound effect (played when the player wins)
    winSound = Mix_LoadWAV("assets/sounds/win.wav");
    if(!winSound) printf("Error loading victory sound: %s\n", Mix_GetError());
    
    // Load the "hurt" sound effect (played when the player takes damage)
    hurtSound = Mix_LoadWAV("assets/sounds/hurt.wav");
    if(!hurtSound) printf("Error loading injury sound: %s\n", Mix_GetError());
}

void drawText(float x, float y, std::string text) {
    glMatrixMode(GL_PROJECTION);// Switch to the projection matrix to set up 2D orthographic projection
    glPushMatrix();// Save the current projection matrix
    glLoadIdentity();// Reset the projection matrix to the identity
    gluOrtho2D(0, 800, 0, 600);// Define a 2D orthographic projection (screen size 800x600)
    
    glMatrixMode(GL_MODELVIEW);// Switch back to the modelview matrix
    glPushMatrix();// Save the current modelview matrix
    glLoadIdentity(); // Reset the modelview matrix
    
    glColor3f(1.0, 1.0, 1.0);// Set text color to white (R=1, G=1, B=1)
    glRasterPos2f(x, y); // Set the position on the screen to start rendering the text

    for (char c : text) { // Loop through each character in the string
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // Render each character using a GLUT bitmap font
    }
    
    glPopMatrix(); // Restore the previous modelview matrix
    glMatrixMode(GL_PROJECTION);// Switch to projection matrix again to restore it
    glPopMatrix();// Restore the previous projection matrix
    glMatrixMode(GL_MODELVIEW);// Switch back to modelview for future rendering
}

// Function to draw a textured vertical wall between (x1, y1) and (x2, y2) with a given height
void drawWall(float x1, float y1, float x2, float y2, float height) {

    glEnable(GL_TEXTURE_2D);// Enable 2D texturing to allow applying the wall texture
    glBindTexture(GL_TEXTURE_2D, wallTexture);// Bind the wall texture so it can be used in the following draw call
    
    glBegin(GL_QUADS);// Start drawing a quadrilateral (4-sided polygon)

    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, 0);// Bottom-left corner
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y2, 0);// Bottom-right corner
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, height);// Top-right corner
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y1, height);// Top-left corner
    glEnd();// End drawing the quadrilateral
    
    glDisable(GL_TEXTURE_2D);// Disable 2D texturing to prevent affecting other drawing operations
}

void drawCollectible(float x, float y) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, collectibleTexture);
    
    glPushMatrix();
    glTranslatef(x + 0.5f, y + 0.5f, 0.3f);
    glRotatef(goldRotation, 0.0f, 0.0f, 1.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.1f, 0.3f, 20, 50); 
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void drawEnemy() {
    glColor3f(1.0, 0.0, 0.0); //Red enemy :DDDDDDDDD
    glPushMatrix();
    glTranslatef(enemyX, enemyY, 0.3f);// Move the enemy to its position in the world (x, y, z)
    glutSolidSphere(0.2f, 20, 20);// Draw a solid sphere with radius 0.2, 20 slices (vertical lines) , and 20 stacks (horizontal lines)
    glPopMatrix();// Restore the previous transformation matrix
}

void drawGoal() {
    glColor3f(0.0, 1.0, 0.0);//Green is the safe color bro
    glPushMatrix();
    glTranslatef(goalX, goalY, 0.3f);
    glScalef(0.4f, 0.4f, 0.4f);
    glutSolidDodecahedron();
    glPopMatrix();
}

void updateEnemy() {
    float dx = playerX - enemyX;
    float dy = playerY - enemyY;
    float distance = sqrt(dx*dx + dy*dy);// الوتر فيثاغورث
    
    if (distance < 3.0f) {// If the player is close enough (less than 3 units), enemy will try to move towards them
        float newEnemyX = enemyX + (dx / distance) * enemySpeed;// Calculate the enemy's new X position based on direction and speed
        float newEnemyY = enemyY + (dy / distance) * enemySpeed;// Calculate the enemy's new Y position based on direction and speed
        
        if(maze[(int)newEnemyY][(int)newEnemyX] != 1) {// Check if the new position is not a wall (1 means wall)
            enemyX = newEnemyX;// Update the enemy's X position
            enemyY = newEnemyY;// Update the enemy's Y position
        }
    }
}

void checkCollisions() {
    if (sqrt(pow(playerX - enemyX, 2) + pow(playerY - enemyY, 2)) < 0.3f) {
        health -= 1;
        Mix_PlayChannel(-1, hurtSound, 0);
        if (health <= 0) gameState = GAME_OVER;
    }
    
    if (maze[(int)playerY][(int)playerX] == 2) {
        score += 10;
        maze[(int)playerY][(int)playerX] = 0;
        Mix_PlayChannel(-1, collectSound, 0);
    }
    
    if (playerX > goalX-0.3 && playerX < goalX+0.3 && playerY > goalY-0.3 && playerY < goalY+0.3) {
        Mix_PlayChannel(-1, winSound, 0);
        gameState = WIN;
    }
}

void resetGame() {
    playerX = 6.5f;
    playerY = 14.0f;
    playerAngle = -89.6f;
    score = 0;
    health = 100;
    enemyX = 3.5f;
    enemyY = 5.5f;
    gameState = PLAYING;
    startTime = time(0);
    
    int resetMaze[mazeHeight][mazeWidth] = {
        {1,1,1,1,1,1,0,1,1,1,1,1,1},
        {1,0,0,0,1,0,2,0,0,0,0,0,1},
        {1,0,1,0,0,0,1,1,1,1,1,0,1},
        {1,0,1,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,1,1,1,1,1,1,0,0,0,1},
        {1,2,1,0,1,0,0,0,1,1,1,0,1},
        {1,1,1,0,1,0,1,0,1,0,0,0,1},
        {1,0,0,0,1,1,1,0,0,0,1,1,1},
        {1,0,1,1,1,0,1,0,1,1,1,2,1},
        {1,0,1,0,0,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,0,1,1,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,0,1,1,1,1,1,1}
    };
    memcpy(maze, resetMaze, sizeof(resetMaze)); // Replace the current maze with the default layout to restart the game map
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// Clear the screen and the depth buffer to prepare for drawing the new frame
    glLoadIdentity();// Reset the current transformation matrix to the Identity
    
     goldRotation += 0.5f;
    if (goldRotation > 360) goldRotation -= 360;

    if (gameState == PLAYING) {
        float lookX = playerX + cos(playerAngle);// Calculate the X position the player is looking at using their angle
        float lookY = playerY + sin(playerAngle);// Calculate the Y position the player is looking at using their angle
        gluLookAt(playerX, playerY, 0.5f, lookX, lookY, 0.5f, 0.0f, 0.0f, 1.0f);// Set the camera position and direction based on player's position and view angle
        
        // Floor painting
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(0, 0, 0);
        glTexCoord2f(13.0f, 0.0f); glVertex3f(13, 0, 0);
        glTexCoord2f(13.0f, 13.0f); glVertex3f(13, 13, 0);
        glTexCoord2f(0.0f, 13.0f); glVertex3f(0, 13, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        
        // wall painting
        for(int y = 0; y < mazeHeight; y++) {
            for(int x = 0; x < mazeWidth; x++) {
                if(maze[y][x] == 1) {
                    // This line draws one wall segment, starting from (x, y) to (x+1, y).
                    // It draws the wall horizontally. The 1.0f is the height of the wall.
                    drawWall(x, y, x+1, y, 1.0f);
                    drawWall(x, y+1, x+1, y+1, 1.0f);
                    drawWall(x, y, x, y+1, 1.0f);
                    drawWall(x+1, y, x+1, y+1, 1.0f);
                }
            }
        }
        drawEnemy();
        drawGoal();
    }

    for(int y = 0; y < mazeHeight; y++) {
        for(int x = 0; x < mazeWidth; x++) {
            if(maze[y][x] == 2) {
                drawCollectible(x, y); 
            }
        }
    }

    // Player interface
    glDisable(GL_LIGHTING);
    time_t currentTime = time(0) - startTime;
    std::string stats = "Points: " + std::to_string(score) + 
                      " | Health: " + std::to_string(health) + 
                      " | Time: " + std::to_string(currentTime/60) + ":" + std::to_string(currentTime%60);
    drawText(10, 580, stats);
    
    if (gameState == GAME_OVER) {
    drawText(250, 300, "Game Over Baka :(");
    drawText(250, 280, "press R to repeat");
    }

    if (gameState == WIN) {
    drawText(280, 300, "Victooory ya king :D yeapeeee | press R to repeat");
    drawText(280, 280, "Team members:");
    drawText(280, 260, "1-Mostafa AbdelHamid");
    drawText(280, 240, "2-Mostafa Mohamed Ibrahim");
    drawText(280, 220, "3-yousef hatem");
    drawText(280, 200, "4-Mostafa Bayoumi");
    drawText(280, 180, "5-Yousef Kotb");
    drawText(280, 160, "6-Yassin salah");
    }

    if (gameState == PAUSED) drawText(350, 300, "Stopped");
    
    if (mouseEnabled) {
        glutWarpPointer(mouseX, mouseY); 
    }

    glutSwapBuffers();
}

void mouseMove(int x, int y) {
    if (gameState != PLAYING || !mouseEnabled) return;

    int deltaX = x - mouseX;
    playerAngle += deltaX * mouseSensitivity;

    glutWarpPointer(mouseX, mouseY);
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseEnabled = true;
        glutSetCursor(GLUT_CURSOR_NONE); 
    }
}

void keyboard(unsigned char key, int x, int y) {
    const float COLLISION_RADIUS = 0.3f;
    if (gameState == PLAYING) {
        float newX = playerX;
        float newY = playerY;
        
        switch(tolower(key)) {
            case 'w':
                newX += cos(playerAngle) * moveSpeed;
                newY += sin(playerAngle) * moveSpeed;
                break;
            case 's':
                newX -= cos(playerAngle) * moveSpeed;
                newY -= sin(playerAngle) * moveSpeed;
                break;
            case 'd':
                playerAngle -= 0.1f;
                break;
            case 'a':
                playerAngle += 0.1f;
                break;
            case 'p':
                gameState = PAUSED;
                break;
            case 'm':
                mouseEnabled = !mouseEnabled;
                glutSetCursor(mouseEnabled ? GLUT_CURSOR_NONE : GLUT_CURSOR_INHERIT);
                break;
        }
        
        if(maze[(int)newY][(int)newX] != 1) {
            playerX = newX;
            playerY = newY;
        }
        
        checkCollisions();
        updateEnemy();
    }
    else if (gameState == GAME_OVER || gameState == WIN) {
        if(key == 'r' || key == 'R') {
            resetGame();
        }
    }
    else if (gameState == PAUSED && (key == 'p' || key == 'P')) {
        gameState = PLAYING;
    }
    
    glutPostRedisplay();
}

void init() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("GLEW initialization failed\n");
        exit(1);
    }
    
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    glMatrixMode(GL_PROJECTION);
    gluPerspective(60, 800/600.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    
    glutSetCursor(GLUT_CURSOR_NONE); 
    glutMotionFunc(mouseMove);
    glutPassiveMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);

    wallTexture = loadTexture("assets/textures/wall.jpg");
    floorTexture = loadTexture("assets/textures/floor.jpg");
    collectibleTexture = loadTexture("assets/textures/gold.jpg");
    initAudio();
    startTime = time(0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D maze game");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}