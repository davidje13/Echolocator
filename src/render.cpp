#include "render.hpp"

#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>

#include <cstdlib>

static const GLfloat FS_VERTICES[]    = {-1,  1,   1,  1,   1, -1,  -1, -1};
static const GLfloat FS_COORDINATES[] = { 0,  0,   1,  0,   1,  1,   0,  1};
static bitmap_window *active = nullptr;

void bitmap_window::global_idle(void) {
	active->idle();
}

void bitmap_window::global_display(void) {
	active->display();
}

void bitmap_window::global_exit(void) {
	active->exit();
}

void bitmap_window::idle(void) {
	glutPostRedisplay();
}

void bitmap_window::display(void) {
	if(displayFunc) {
		displayFunc->perform();
	}
	glTexImage2D(
		GL_TEXTURE_2D, 0,
		GL_LUMINANCE,
		w, h, 0,
		GL_LUMINANCE,
		GL_UNSIGNED_BYTE, &img[0]
	);
	glVertexPointer(2, GL_FLOAT, 0, FS_VERTICES);
	glTexCoordPointer(2, GL_FLOAT, 0, FS_COORDINATES);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();
	glutSwapBuffers();
}

void bitmap_window::exit(void) {
	if(exitFunc) {
		exitFunc->perform();
	}
}

void bitmap_window::initialise(int *argc, char **argv) {
	glutInit(argc, argv);
}

bitmap_window::bitmap_window(int width, int height, const char *title)
	: w(width)
	, h(height)
	, img(std::size_t(w * h), 0)
	, data(nullptr)
	, exitFunc(nullptr)
{
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(w, h);
	GLint windowID = glutCreateWindow(title);
	data = new GLint(windowID);
}

int bitmap_window::width(void) const {
	return w;
}

int bitmap_window::height(void) const {
	return h;
}

std::vector<unsigned char> &bitmap_window::image_data(void) {
	return img;
}

const std::vector<unsigned char> &bitmap_window::image_data(void) const {
	return img;
}

void bitmap_window::run(void) {
	active = this;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glutIdleFunc(&bitmap_window::global_idle);
	glutDisplayFunc(&bitmap_window::global_display);
	std::atexit(&bitmap_window::global_exit);

	glutMainLoop(); // never returns
}

bitmap_window::~bitmap_window(void) {
	GLint *windowID = (GLint*) data;
	glutDestroyWindow(*windowID);
	delete windowID;
	data = nullptr;
}
