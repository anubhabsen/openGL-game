#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR color;
COLOR red = {0.882, 0.3333, 0.3333};
COLOR green = {0.1255, 0.75, 0.333};
COLOR black = {30 / 255.0, 30 / 255.0, 21 / 255.0};
COLOR steel = {196 / 255.0, 231 / 255.0, 249 / 255.0};

struct Sprite {
    string name;
    int exists;
    COLOR color;
    float x, y;
    float height, width;
    float velx, vely;
    VAO* object;
};

typedef struct Sprite Sprite;

map <string, Sprite> bricks;
map <string, Sprite> collect_baskets;
map <string, Sprite> turret;
map <string, Sprite> laser;
map <string, Sprite> mirror;

GLuint programID;
int numblocks = 0;
int score = 0;

void moveelem(Sprite *temp, float dx, float dy)
{
    if(temp->x + dx > 3.4)
    {
      return;
    }
    if(temp->x + dx < -3.4)
    {
      return;
    }
    if(temp->y > 3.5 && dy > 0)
    {
      return;
    }
    if(temp->y < -4.5 && dy < 0)
    {
      return;
    }
    temp->x += dx;
    temp->y += dy;
}

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    exit(0);
//    exit(EXIT_SUCCESS);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_A:
                moveelem(&collect_baskets["redbasket"], -0.1, 0);
                break;
            case GLFW_KEY_D:
                moveelem(&collect_baskets["redbasket"], 0.1, 0);
                break;
            case GLFW_KEY_J:
                moveelem(&collect_baskets["greenbasket"], -0.1, 0);
                break;
            case GLFW_KEY_L:
                moveelem(&collect_baskets["greenbasket"], 0.1, 0);
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
            quit(window);
            break;
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
            }
            break;
        default:
            break;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

// Creates the rectangle object used in this sample code
void createRectangle (string name, float x, float y, float width, float height, COLOR colour, string type)
{
  // create3DObject creates and returns a handle to a VAO that can be used later
  float w = width/2, h = height/2;
    GLfloat vertex_buffer_data [] = {
        -w,-h,0, // vertex 1
        -w,h,0, // vertex 2
        w,h,0, // vertex 3

        w,h,0, // vertex 3
        w,-h,0, // vertex 4
        -w,-h,0  // vertex 1
    };

  GLfloat color_buffer_data [] = {
        colour.r, colour.g, colour.b, // color 1
        colour.r, colour.g, colour.b, // color 2
        colour.r, colour.g, colour.b, // color 3

        colour.r, colour.g, colour.b, // color 4
        colour.r, colour.g, colour.b, // color 5
        colour.r, colour.g, colour.b // color 6
    };

  VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  Sprite elem = {};
  elem.color = colour;
  elem.exists = 1;
  elem.name = name;
  elem.object = rectangle;
  elem.x = x;
  elem.y = y;
  elem.height = height;
  elem.width = width;

  if(type == "basket")
  {
    collect_baskets[name] = elem;
  }
  else if(type == "brick")
  {
    bricks[name] = elem;
  }
  else if(type == "turret")
  {
    turret[name] = elem;
  }
}

float camera_rotation_angle = 90;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram(programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye(5 * cos(camera_rotation_angle * M_PI / 180.0f), 0,
                5 * sin(camera_rotation_angle * M_PI / 180.0f));
  // Target - Where is the camera looking at.  Don't change unless you are
  // sure!!
  glm::vec3 target(0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up(0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  /* Render your scene */
  //draw baskets
  for(map<string,Sprite>::iterator it=collect_baskets.begin(); it!=collect_baskets.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(collect_baskets[current].exists == 0)
    {
        continue;
    }
    glm::mat4 MVP;	// MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(collect_baskets[current].x, collect_baskets[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(collect_baskets[current].object);

    //glPopMatrix ();
  }
  for(map<string,Sprite>::iterator it=bricks.begin(); it!=bricks.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(bricks[current].exists==0)
    {
        continue;
    }
    glm::mat4 MVP;	// MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(bricks[current].x, bricks[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(bricks[current].object);

    //glPopMatrix ();
  }
  for(map<string,Sprite>::iterator it=turret.begin(); it!=turret.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(turret[current].exists==0)
    {
        continue;
    }
    glm::mat4 MVP;  // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(turret[current].x, turret[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(turret[current].object);

    //glPopMatrix ();
  }
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    // /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
  /* Objects should be created before any other gl function and shaders */
  // Create the models
  // GL3 accepts only Triangles. Quads are not supported

  createRectangle("redbasket", -2, -3.5, 0.5, 0.5, red, "basket");
  createRectangle("greenbasket", 2, -3.5, 0.5, 0.5, green, "basket");
  createRectangle("turretcanon", -4, 0, 1, 0.2, steel, "turret");
  createRectangle("turretbase", -4, 0, 0.5, 0.5, black, "turret");

  // Create and compile our GLSL program from the shaders
  programID = LoadShaders("Sample_GL.vert", "Sample_GL.frag");
  // Get a handle for our "MVP" uniform
  Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

  reshapeWindow(window, width, height);

  // Background color of the scene
  glClearColor(0.88f, 0.74f, 0.16f, 0.0f); // R, G, B, A
  glClearDepth(1.0f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
  cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
  cout << "VERSION: " << glGetString(GL_VERSION) << endl;
  cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

void blockFall()
{
  for (map<string, Sprite>::iterator it = bricks.begin(); it != bricks.end(); it++)
  {
    string current = it->first;
    moveelem(&bricks[current], 0, -0.3);
  }
  float randx = ((double) rand() / (RAND_MAX)) * 7;
  randx = randx - 3.5;

  srand(time(NULL));
  int rc = rand() % 3;
  COLOR randcolor;
  if (rc == 0)
  {
    randcolor = red;
  }
  else if (rc == 1)
  {
    randcolor = green;
  }
  else
  {
    randcolor = black;
  }

  stringstream ss;
  ss << numblocks;
  createRectangle(ss.str(), randx, 3.8, 0.1, 0.7, randcolor, "brick");
  numblocks++;
}

int main(int argc, char **argv)
{
  int width = 900;
  int height = 600;
  GLFWwindow *window = initGLFW(width, height);

  initGL(window, width, height);

  double last_update_time = glfwGetTime(), current_time;

  /* Draw in loop */
  while (!glfwWindowShouldClose(window)) {

    // OpenGL Draw commands

    draw();
    // draw3DObject draws the VAO given to it using current MVP matrix

    // Swap Frame Buffer in double buffering
    glfwSwapBuffers(window);

    // Poll for Keyboard and mouse events
    glfwPollEvents();

    // Control based on time (Time based transformation like 5 degrees
    // rotation every 0.5s)
    current_time = glfwGetTime(); // Time in seconds
    if ((current_time - last_update_time) >= 1) { // atleast 0.5s elapsed since last frame
      // do something every 0.5 seconds ..
      blockFall();
      last_update_time = current_time;
    }
  }
  glfwTerminate();
  return 0;
}
