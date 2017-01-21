#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//Music

#include <ao/ao.h>
#include <mpg123.h>
#define BITS 8

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
COLOR black = {0, 0, 0};
COLOR steel = {196 / 255.0, 231 / 255.0, 249 / 255.0};

struct Sprite {
    string name;
    int exists, mirror;
    COLOR color;
    float x, y;
    float height, width, angle;
    float velx, vely;
    VAO* object;
};

typedef struct Sprite Sprite;

map <string, Sprite> bricks;
map <string, Sprite> collect_baskets;
map <string, Sprite> turret;
map <string, Sprite> laser;
map <string, Sprite> mirror;
map <string, Sprite> scoredisp;

GLuint programID;
int laserfired = 0;
int numblocks = 0;
int score = 20;
float brickfalltimer = 0.7;
float brickformtimer = 3.9;
float screen_zoom = 1;
float screen_center_y = 0;
float screen_center_x = 0;
double last_update_time_lasershot = 0, current_time_lasershot;
int health = 100;

void turretHandler(Sprite *gun, float dx, float dy, float angle)
{
  if(gun->y + dy > 3.5)
  {
    return;
  }
  if(gun->y + dy < -3.5)
  {
    return;
  }
  gun->y += dy;
  if(gun->angle <= -50)
  {
    if(angle > 0)
    {
      gun->angle += angle;
    }
    else
    {
      return;
    }
  }
  else if(gun->angle >= 50)
  {
    if(angle < 0)
    {
      gun->angle += angle;
    }
    else
    {
      return;
    }
  }
  else
  {
    gun->angle += angle;
  }
}

void moveLaser(Sprite *laser, float dx, float dy)
{
  laser->x += dx;
  laser->y += dy;
}

void brickBasketHandler(Sprite *temp, float dx, float dy)
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

void createRectangle (string name, float x, float y, float width, float height, COLOR colour, string type,float angle)
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
  elem.mirror = 0;
  elem.object = rectangle;
  elem.x = x;
  elem.y = y;
  elem.height = height;
  elem.width = width;
  elem.angle = angle;

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
  else if(type == "laser")
  {
    laser[name] = elem;
  }
  else if(type == "mirror")
  {
    mirror[name] = elem;
  }
  else if(type == "scoredisp")
  {
    scoredisp[name] = elem;
  }
}

void fireTurret(Sprite *cannon)
{
  stringstream ss;
  ss << laserfired;
  createRectangle(ss.str(), cannon->x, cannon->y, 0.3, 0.1, black, "laser", cannon->angle);
  laserfired++;
}

/**************************
 * Customizable functions *
 **************************/

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

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
    double width, height, x, y;
    // glfwGetWindowSize(window, &width, &height);
    glfwGetCursorPos(window, &x, &y);
    float convx = (x / 900) * 8 - 4;
    float convy = (y / 600) * 8 - 4;
    if(action == GLFW_REPEAT || action == GLFW_RELEASE)
    {
      switch (button) {
          case GLFW_MOUSE_BUTTON_LEFT:
              if(convy >= 3.55 && convy <= 4.13 && convx <= collect_baskets["redbasket"].x + 0.25 && convx >= collect_baskets["redbasket"].x - 0.25)
              {
                // continue;
              }
              else if(convy >= 3.55 && convy <= 4.13 && convx <= collect_baskets["greenbasket"].x + 0.25 && convx >= collect_baskets["greenbasket"].x - 0.25)
              {
                // continue;
              }
              else
              {
                fireTurret(&turret["turretcanon"]);
              }
              break;
          case GLFW_MOUSE_BUTTON_RIGHT:
              if (action == GLFW_RELEASE) {
              }
              break;
          default:
              break;
      }
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

float camera_rotation_angle = 90;

/* Render the scene with openGL */
/* Edit this function according to your assignment */

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  if (yoffset==-1)
  {
    if(screen_zoom <= 5)
    {
      screen_zoom += 1;
    }
  }
  else if(yoffset==1)
  {
    if(screen_zoom >= 2)
    {
      screen_zoom -= 1;
      screen_center_x = 0;
      screen_center_y = 0;
    }
  }
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) {
        switch (key) {
          //glfwGetKey(window, GLFW_KEY_LEFT_ALT)
            case GLFW_KEY_LEFT:
                if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                  brickBasketHandler(&collect_baskets["redbasket"], -0.1, 0);
                else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
                  brickBasketHandler(&collect_baskets["greenbasket"], -0.1, 0);
                else
                {
                  if(screen_zoom > 1)
                  {
                    screen_center_x -= 0.1;
                  }
                }
                break;
            case GLFW_KEY_RIGHT:
                if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                  brickBasketHandler(&collect_baskets["redbasket"], 0.1, 0);
                else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
                  brickBasketHandler(&collect_baskets["greenbasket"], 0.1, 0);
                else
                {
                  if(screen_zoom > 1)
                  {
                    screen_center_x += 0.1;
                  }
                }
                break;
            case GLFW_KEY_UP:
                if(screen_zoom > 1)
                {
                  screen_center_y += 0.1;
                }
                break;
            case GLFW_KEY_DOWN:
                if(screen_zoom > 1)
                {
                  screen_center_y -= 0.1;
                }
                break;
            case GLFW_KEY_S:
                turretHandler(&turret["turretcanon"], 0, 0.3, 0);
                turretHandler(&turret["turretbase"], 0, 0.3, 0);
                break;
            case GLFW_KEY_F:
                turretHandler(&turret["turretcanon"], 0, -0.3, 0);
                turretHandler(&turret["turretbase"], 0, -0.3, 0);
                break;
            case GLFW_KEY_T:
                turretHandler(&turret["turretcanon"], 0, 0, 10);
                break;
            case GLFW_KEY_G:
                turretHandler(&turret["turretcanon"], 0, 0, -10);
                break;
            case GLFW_KEY_SPACE:
                current_time_lasershot = glfwGetTime();
                if ((current_time_lasershot - last_update_time_lasershot) >= 1)
                {
                    fireTurret(&turret["turretcanon"]);
                    last_update_time_lasershot = current_time_lasershot;
                }
                break;
            case GLFW_KEY_M:
                if(brickfalltimer <= 2)
                {
                  brickfalltimer += 0.1;
                  brickformtimer += 0.1;
                }
                break;
            case GLFW_KEY_N:
                if(brickfalltimer >= 0.1)
                {
                  brickfalltimer -= 0.1;
                  brickformtimer -= 0.5;
                }
                break;
            case GLFW_KEY_EQUAL:
                if(screen_zoom <= 5)
                {
                  screen_zoom += 1;
                }
                break;
            case GLFW_KEY_MINUS:
                if(screen_zoom >= 2)
                {
                  screen_zoom -= 1;
                  screen_center_x = 0;
                  screen_center_y = 0;
                }
            default:
                break;
        }
    }
    else if (action == GLFW_REPEAT) {
        switch(key) {
          case GLFW_KEY_LEFT:
                if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                  brickBasketHandler(&collect_baskets["redbasket"], -0.1, 0);
                else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
                  brickBasketHandler(&collect_baskets["greenbasket"], -0.1, 0);
                else
                {
                  if(screen_zoom > 1)
                  {
                    screen_center_x -= 0.1;
                  }
                }
                break;
            case GLFW_KEY_RIGHT:
                if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                  brickBasketHandler(&collect_baskets["redbasket"], 0.1, 0);
                else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
                  brickBasketHandler(&collect_baskets["greenbasket"], 0.1, 0);
                else
                {
                  if(screen_zoom > 1)
                  {
                    screen_center_x += 0.1;
                  }
                }
                break;
            case GLFW_KEY_UP:
                if(screen_zoom > 1)
                {
                  screen_center_y += 0.1;
                }
                break;
            case GLFW_KEY_DOWN:
                if(screen_zoom > 1)
                {
                  screen_center_y -= 0.1;
                }
                break;
            case GLFW_KEY_S:
                turretHandler(&turret["turretcanon"], 0, 0.3, 0);
                turretHandler(&turret["turretbase"], 0, 0.3, 0);
                break;
            case GLFW_KEY_F:
                turretHandler(&turret["turretcanon"], 0, -0.3, 0);
                turretHandler(&turret["turretbase"], 0, -0.3, 0);
                break;
            case GLFW_KEY_T:
                turretHandler(&turret["turretcanon"], 0, 0, 10);
                break;
            case GLFW_KEY_G:
                turretHandler(&turret["turretcanon"], 0, 0, -10);
                break;
            case GLFW_KEY_SPACE:
                fireTurret(&turret["turretcanon"]);
                break;
            case GLFW_KEY_M:
                if(brickfalltimer <= 2)
                {
                  brickfalltimer += 0.1;
                  brickformtimer += 0.1;
                }
                break;
            case GLFW_KEY_N:
                if(brickfalltimer >= 0.1)
                {
                  brickfalltimer -= 0.1;
                  brickformtimer -= 0.5;
                }
                break;
            case GLFW_KEY_EQUAL:
                if(screen_zoom <= 5)
                {
                  screen_zoom += 0.5;
                }
                break;
            case GLFW_KEY_MINUS:
                if(screen_zoom >= 2)
                {
                  screen_zoom -= 0.5;
                  screen_center_x = 0;
                  screen_center_y = 0;
                }
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


void draw (GLFWwindow *window, int width, int height)
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

  float top = (screen_center_y + 4) / screen_zoom;
  float bottom = (screen_center_y - 4) / screen_zoom;
  float left = (screen_center_x - 4) / screen_zoom;
  float right = (screen_center_x + 4) / screen_zoom;
  Matrices.projection = glm::ortho(left, right, bottom, top, 0.1f, 500.0f);

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;
  double x, y;
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
    glm::mat4 rotateTriangle = glm::rotate((float)((turret[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(turret[current].object);

    //glPopMatrix ();
  }

  glfwSetScrollCallback(window, scroll_callback);
  glfwGetWindowSize(window, &width, &height);
  glfwGetCursorPos(window, &x, &y);
  float xcoord = x / width * 8 - 4 - turret["turretcanon"].x;
  float ycoord = - y / height * 8 + 4 - turret["turretcanon"].y;
  float angle = 1 * atan(ycoord / xcoord) * 180 / M_PI;
  if(angle <= 50 && angle >= -50)
  {
    turret["turretcanon"].angle = angle;
  }

  for(map<string,Sprite>::iterator it=laser.begin(); it!=laser.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(laser[current].exists == 0)
    {
        continue;
    }
    glm::mat4 MVP;  // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(laser[current].x, laser[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((laser[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(laser[current].object);

    //glPopMatrix ();
  }
  for(map<string,Sprite>::iterator it=mirror.begin(); it!=mirror.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(mirror[current].exists==0)
    {
        continue;
    }
    glm::mat4 MVP;  // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(mirror[current].x, mirror[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((mirror[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirror[current].object);

    //glPopMatrix ();
  }
  for(map<string,Sprite>::iterator it=scoredisp.begin(); it!=scoredisp.end(); it++)
  {
    string current = it->first; //The name of the current object
    if(scoredisp[current].exists==0)
    {
        continue;
    }
    glm::mat4 MVP;  // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate (glm::vec3(scoredisp[current].x, scoredisp[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((scoredisp[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)

    ObjectTransform=translateObject*rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(scoredisp[current].object);

    //glPopMatrix ();
  }

  float convx = (x / 900) * 8 - 4;
  float convy = (y / 600) * 8 - 4;

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (state == GLFW_PRESS)
  {
    if(convy >= 3.55 && convy <= 4.13 && convx <= collect_baskets["redbasket"].x + 0.25 && convx >= collect_baskets["redbasket"].x - 0.25)
    {
      collect_baskets["redbasket"].x = convx;
    }
    else if(convy >= 3.55 && convy <= 4.13 && convx <= collect_baskets["greenbasket"].x + 0.25 && convx >= collect_baskets["greenbasket"].x - 0.25)
    {
      collect_baskets["greenbasket"].x = convx;
    }
    convy = -1 * convy;
    if(convx <= -3.75 && convy <= turret["turretbase"].y + 0.25 && convy >= turret["turretbase"].y - 0.25)
    {
      turret["turretbase"].y = convy;
      turret["turretcanon"].y = convy;
    }
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

  createRectangle("redbasket", -2, -3.8, 0.5, 0.5, red, "basket", 0);
  createRectangle("greenbasket", 2, -3.8, 0.5, 0.5, green, "basket", 0);
  createRectangle("turretcanon", -4, 0, 1, 0.2, steel, "turret", 0);
  createRectangle("turretbase", -4, 0, 0.5, 0.5, black, "turret", 0);
  createRectangle("mirror1", 3, 3, 0.05, 0.8, steel, "mirror", 45);
  createRectangle("mirror2", 3, -2.5, 0.05, 0.8, steel, "mirror", -45);
  createRectangle("mirror3", 1, 1.5, 0.05, 0.8, steel, "mirror", 30);
  createRectangle("mirror4", 1, -1.5, 0.05, 0.8, steel, "mirror", -30);

  createRectangle("sign", 0.7, 3.3, 0.2, 0.05, steel, "scoredisp", 0);

  createRectangle("score1.1", 1, 3.5, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score1.2", 1, 3.3, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score1.3", 1, 3.1, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score1.4", 0.9, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score1.5", 1.1, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score1.6", 0.9, 3.2, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score1.7", 1.1, 3.2, 0.05, 0.2, steel, "scoredisp", 0);

  createRectangle("score2.1", 0.7, 3.5, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score2.2", 0.7, 3.3, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score2.3", 0.7, 3.1, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score2.4", 0.6, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score2.5", 0.8, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score2.6", 0.6, 3.2, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score2.7", 0.8, 3.2, 0.05, 0.2, steel, "scoredisp", 0);

  createRectangle("score3.1", 0.4, 3.5, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score3.2", 0.4, 3.3, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score3.3", 0.4, 3.1, 0.2, 0.05, steel, "scoredisp", 0);
  createRectangle("score3.4", 0.3, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score3.5", 0.5, 3.4, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score3.6", 0.3, 3.2, 0.05, 0.2, steel, "scoredisp", 0);
  createRectangle("score3.7", 0.5, 3.2, 0.05, 0.2, steel, "scoredisp", 0);

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

float dist(float x1, float y1, float x2, float y2)
{
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void laserTimer()
{
  for (map<string, Sprite>::iterator it = laser.begin(); it != laser.end(); it++)
  {
    string current = it->first;
    moveLaser(&laser[current], 0.1 * cos (laser[current].angle * M_PI / 180), 0.1 * sin (laser[current].angle * M_PI / 180));
    for (map<string, Sprite>::iterator it1 = bricks.begin(); it1 != bricks.end(); it1++)
    {
      string current_brick = it1->first;
      if(dist(laser[current].x, laser[current].y, bricks[current_brick].x, bricks[current_brick].y) <= sqrt(14.5 - 10.5 * sin(laser[current].angle)) / 10 && laser[current].exists == 1 && bricks[current_brick].exists == 1)
      {
        if(bricks[current_brick].color.r == 0 && bricks[current_brick].color.g == 0 && bricks[current_brick].color.b == 0)
        {
          score += 10;
        }
        else
        {
          score -= 5;
        }
        bricks[current_brick].exists = 0;
        laser[current].exists = 0;
      }
    }
    for(map<string, Sprite>::iterator it1 = mirror.begin(); it1 != mirror.end(); it1++)
    {
      string current_mirror = it1->first;
      if(dist(laser[current].x, laser[current].y, mirror[current_mirror].x, mirror[current_mirror].y) <= sqrt(18.25 + 12 * cos((laser[current].angle + mirror[current_mirror].angle) * M_PI / 180)) / 10 && dist(laser[current].x, laser[current].y, mirror[current_mirror].x, mirror[current_mirror].y) >= sqrt(18.25 - 12 * cos((laser[current].angle + mirror[current_mirror].angle) * M_PI / 180)) / 10)
      {
        if((laser[current].mirror == 1 && mirror[current_mirror].name != "mirror1") || (laser[current].mirror == 2 && mirror[current_mirror].name != "mirror2") || (laser[current].mirror == 3 && mirror[current_mirror].name != "mirror3") || (laser[current].mirror == 4 && mirror[current_mirror].name != "mirror4") || laser[current].mirror == 0)
        {
          laser[current].angle += 2 * (mirror[current_mirror].angle - laser[current].angle) + 180;
          laser[current].mirror = (int)mirror[current_mirror].name[6];
        }
      }
    }
    if(laser[current].x > 4 || laser[current].x < -4 || laser[current].y > 4 || laser[current].y < -4)
    {
      laser[current].exists = 0;
    }
  }
}

void blockFall()
{
  for (map<string, Sprite>::iterator it = bricks.begin(); it != bricks.end(); it++)
  {
    string current = it->first;
    brickBasketHandler(&bricks[current], 0, -0.3);
    for (map<string, Sprite>::iterator it1 = collect_baskets.begin(); it1 != collect_baskets.end(); it1++)
    {
      string current_basket = it1->first;
      if(bricks[current].y <= -3.7 && bricks[current].exists == 1 && bricks[current].x <= collect_baskets[current_basket].x + 0.26 && bricks[current].x >= collect_baskets[current_basket].x - 0.26)
      {
        if(bricks[current].color.r == collect_baskets[current_basket].color.r && bricks[current].color.g == collect_baskets[current_basket].color.g && bricks[current].color.b == collect_baskets[current_basket].color.b)
        {
          if(collect_baskets["redbasket"].x - collect_baskets["greenbasket"].x > 1 || collect_baskets["redbasket"].x - collect_baskets["greenbasket"].x < -1)
          {
            score += 10;
            bricks[current].exists = 0;
          }
        }
        else
        {
          score -=  2;
          health -= 5;
          if(health <= 0)
          {
            cout<<"You ran out of health :("<<endl;
            exit(0);
          }
          bricks[current].exists = 0;
        }
      }
    }
    if(bricks[current].y < -5)
    {
      score -= 1;
      bricks[current].exists = 0;
    }
  }
}

void disp1(int digit)
{
  if(digit == 0)
  {
    scoredisp["score1.2"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.6"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 1)
  {
    scoredisp["score1.1"].exists = 0;
    scoredisp["score1.2"].exists = 0;
    scoredisp["score1.3"].exists = 0;
    scoredisp["score1.4"].exists = 0;
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 2)
  {
    scoredisp["score1.4"].exists = 0;
    scoredisp["score1.7"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.6"].exists = 1;
  }
  else if(digit == 3)
  {
    scoredisp["score1.4"].exists = 0;
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 4)
  {
    scoredisp["score1.1"].exists = 0;
    scoredisp["score1.3"].exists = 0;
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 5)
  {
    scoredisp["score1.5"].exists = 0;
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 6)
  {
    scoredisp["score1.5"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.6"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 7)
  {
    scoredisp["score1.2"].exists = 0;
    scoredisp["score1.3"].exists = 0;
    scoredisp["score1.4"].exists = 0;
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 8)
  {
    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.6"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
  else if(digit == 9)
  {
    scoredisp["score1.6"].exists = 0;

    scoredisp["score1.1"].exists = 1;
    scoredisp["score1.2"].exists = 1;
    scoredisp["score1.3"].exists = 1;
    scoredisp["score1.4"].exists = 1;
    scoredisp["score1.5"].exists = 1;
    scoredisp["score1.7"].exists = 1;
  }
}

void disp10(int digit)
{
  if(digit == 0)
    {
      scoredisp["score2.2"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.6"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 1)
    {
      scoredisp["score2.1"].exists = 0;
      scoredisp["score2.2"].exists = 0;
      scoredisp["score2.3"].exists = 0;
      scoredisp["score2.4"].exists = 0;
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 2)
    {
      scoredisp["score2.4"].exists = 0;
      scoredisp["score2.7"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.6"].exists = 1;
    }
    else if(digit == 3)
    {
      scoredisp["score2.4"].exists = 0;
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 4)
    {
      scoredisp["score2.1"].exists = 0;
      scoredisp["score2.3"].exists = 0;
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 5)
    {
      scoredisp["score2.5"].exists = 0;
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 6)
    {
      scoredisp["score2.5"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.6"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 7)
    {
      scoredisp["score2.2"].exists = 0;
      scoredisp["score2.3"].exists = 0;
      scoredisp["score2.4"].exists = 0;
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 8)
    {
      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.6"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
    else if(digit == 9)
    {
      scoredisp["score2.6"].exists = 0;

      scoredisp["score2.1"].exists = 1;
      scoredisp["score2.2"].exists = 1;
      scoredisp["score2.3"].exists = 1;
      scoredisp["score2.4"].exists = 1;
      scoredisp["score2.5"].exists = 1;
      scoredisp["score2.7"].exists = 1;
    }
}

void disp100(int digit)
{
  if(digit == 0)
    {
      scoredisp["score3.2"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.6"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 1)
    {
      scoredisp["score3.1"].exists = 0;
      scoredisp["score3.2"].exists = 0;
      scoredisp["score3.3"].exists = 0;
      scoredisp["score3.4"].exists = 0;
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 2)
    {
      scoredisp["score3.4"].exists = 0;
      scoredisp["score3.7"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.6"].exists = 1;
    }
    else if(digit == 3)
    {
      scoredisp["score3.4"].exists = 0;
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 4)
    {
      scoredisp["score3.1"].exists = 0;
      scoredisp["score3.3"].exists = 0;
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 5)
    {
      scoredisp["score3.5"].exists = 0;
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 6)
    {
      scoredisp["score3.5"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.6"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 7)
    {
      scoredisp["score3.2"].exists = 0;
      scoredisp["score3.3"].exists = 0;
      scoredisp["score3.4"].exists = 0;
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 8)
    {
      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.6"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
    else if(digit == 9)
    {
      scoredisp["score3.6"].exists = 0;

      scoredisp["score3.1"].exists = 1;
      scoredisp["score3.2"].exists = 1;
      scoredisp["score3.3"].exists = 1;
      scoredisp["score3.4"].exists = 1;
      scoredisp["score3.5"].exists = 1;
      scoredisp["score3.7"].exists = 1;
    }
}

void Dispscore()
{
  int temp = score;
  if(temp < 0)
  {
    temp *= -1;
    scoredisp["sign"].exists = 1;
    cout<<scoredisp["sign"].exists<<endl;
  }
  if(temp <= 999)
  {
    scoredisp["sign"].exists = 0;
    disp1(temp % 10);
    temp /= 10;
    disp10(temp % 100);
    temp /= 10;
    disp100(temp % 1000);
  }
  else
  {
    scoredisp["sign"].exists = 0;
    disp1(9);
    disp10(9);
    disp100(9);
  }
}

void blockCreate()
{
  float randx = ((double) rand() / (RAND_MAX)) * 6;
  randx = randx - 3;

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
  createRectangle(ss.str(), randx, 3.8, 0.1, 0.7, randcolor, "brick", 0);
  numblocks++;
}

int main(int argc, char **argv)
{
  int width = 900;
  int height = 600;
  GLFWwindow *window = initGLFW(width, height);

  initGL(window, width, height);

  //Music

  // mpg123_handle *mh;
  // unsigned char *buffer;
  // size_t buffer_size;
  // size_t done;
  // int err;

  // int driver;
  // ao_device *dev;

  // ao_sample_format format;
  // int channels, encoding;
  // long rate;

  // if(argc < 2)
  //     exit(0);

  // /* initializations */
  // ao_initialize();
  // driver = ao_default_driver_id();
  // mpg123_init();
  // mh = mpg123_new(NULL, &err);
  // buffer_size = 64;
  // buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

  // /* open the file and get the decoding format */
  // mpg123_open(mh, argv[1]);
  // mpg123_getformat(mh, &rate, &channels, &encoding);

  // /* set the output format and open the output device */
  // format.bits = mpg123_encsize(encoding) * BITS;
  // format.rate = rate;
  // format.channels = channels;
  // format.byte_format = AO_FMT_NATIVE;
  // format.matrix = 0;
  // dev = ao_open_live(driver, &format, NULL);

  // /* decode and play */
  // if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
  //   ao_play(dev, buffer, done);
  // else mpg123_seek(mh, 0, SEEK_SET);

  // Music ends

  double last_update_time_brick_form = glfwGetTime(), current_time_brick_form, current_time_laser, last_update_time_laser = glfwGetTime();
  double last_update_time_brick_fall = glfwGetTime(), current_time_brick_fall;
  /* Draw in loop */
  scoredisp["score1.2"].exists = 0;
  scoredisp["score2.2"].exists = 0;
  scoredisp["score3.2"].exists = 0;
  while (!glfwWindowShouldClose(window)) {

    // OpenGL Draw commands

    draw(window, width, height);
    // draw3DObject draws the VAO given to it using current MVP matrix

    // Swap Frame Buffer in double buffering
    glfwSwapBuffers(window);

    // Poll for Keyboard and mouse events
    glfwPollEvents();

    Dispscore();

    current_time_laser = glfwGetTime();
    if((current_time_laser - last_update_time_laser) >= 1 / 12)
    {
      laserTimer();
      last_update_time_laser = current_time_laser;
    }
    // Control based on time (Time based transformation like 5 degrees
    // rotation every 0.5s)
    current_time_brick_form = glfwGetTime(); // Time in seconds
    if ((current_time_brick_form - last_update_time_brick_form) >= brickformtimer)
    { // atleast 0.5s elapsed since last frame
      // do something every 0.5 seconds ..
      blockCreate();
      last_update_time_brick_form = current_time_brick_form;
    }

    current_time_brick_fall = glfwGetTime(); // Time in seconds
    if ((current_time_brick_fall - last_update_time_brick_fall) >= brickfalltimer)
    { // atleast 0.5s elapsed since last frame
      // do something every 0.5 seconds ..
      blockFall();
      last_update_time_brick_fall = current_time_brick_fall;
    }
  }
  glfwTerminate();
  return 0;
}
