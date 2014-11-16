#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <GL/glut.h>
using namespace std;
#include "CImg.h" 
using namespace cimg_library;
#include "SOIL.h"

#define PI 3.14159265358979323846
#define sqr(x) ((x)*(x))

GLint n = 4;
GLint TOL = 10;
GLdouble tolerance = sqr(TOL);
typedef struct { GLfloat x, y, z; } point3D_type;
GLint viewport[4];
GLdouble mvmatrix[16], projmatrix[16];

float vAng = 40.0f, asp = 1.0f, nearD = 0.2f, farD = 40.0f;
bool perspectiva = false;
bool fillCurve = true;
bool showControls = false;

point3D_type* ctrlpoints;
int M, N;

class CPoint3D {
public:
	float x, y, z;
	CPoint3D()
	{};
	CPoint3D(float x1, float y1, float z1)
	{
		x = x1; y = y1; z = z1;
	}
	void Set(float x1, float y1, float z1)
	{
		x = x1; y = y1; z = z1;
	}
	CPoint3D Normalize()
	{
		float L = Abs();
		return (L > 0.0f) ? (*this) / L : (*this);
	}

	CPoint3D Dir()
	{
		return Normalize();
	}

	// angle in radians
	CPoint3D Rotate(float angle, CPoint3D dir)
	{
		CPoint3D base = (*this) * cos(angle);
		CPoint3D elev = dir.Dir() * sin(angle);
		//cout << "orig = " + (*this).ToString() + ", base = " + base.ToString() + ", elev = " + elev.ToString() << "\n";
		return base + elev;
	}

	CPoint3D RotateDeg(float angleDegrees, CPoint3D dir)
	{
		return Rotate(angleDegrees * PI / 180, dir);
	}

	CPoint3D CrossProduct(CPoint3D p)
	{
		return CPoint3D(y * p.z - z * p.y, z * p.x - x * p.z, x * p.y - y * p.x);
	}

	float Abs()
	{
		return sqr(x) + sqr(y) + sqr(z);
	}

	CPoint3D operator +(CPoint3D p)
	{
		return CPoint3D(x + p.x, y + p.y, z + p.z);
	}
	CPoint3D operator -(CPoint3D p)
	{
		return CPoint3D(x - p.x, y - p.y, z - p.z);
	}
	CPoint3D operator *(CPoint3D p)
	{
		return CPoint3D(x * p.x, y * p.y, z * p.z);
	}
	CPoint3D operator /(CPoint3D p)
	{
		return CPoint3D(x / p.x, y / p.y, z / p.z);
	}
	CPoint3D operator *(float f)
	{
		return CPoint3D(x * f, y * f, z * f);
	}
	CPoint3D operator /(float f)
	{
		return CPoint3D(x / f, y / f, z / f);
	}
	CPoint3D operator - () {
		return CPoint3D(-x, -y, -z);
	}

	string ToString() {
		return "(" + to_string(x) + ", " + to_string(y) + ", " + to_string(z) + ")";
	}

};

class CCamera{
public:
	CPoint3D P0, At, Up;
	float AngX, AngY, AngZ, Scale;

	CCamera()
	{
		P0 = CPoint3D(0.0f, 0.0f, 0.0f);
		At = CPoint3D(0.0f, 0.0f, -1.0f);
		Up = CPoint3D(0.0f, 1.0f, 0.0f);
		Scale = 8.0;
		AngX = 0.0f;
		AngY = 0.0f;
		AngZ = 0.0f;
	}

	void Set(CPoint3D p0, CPoint3D p, CPoint3D up)
	{
		P0 = p0; At = p; Up = up;
		AngX = 0.0f; AngY = 0.0f;  AngZ = 0.0f;
	}

	void Zoom(float amount) {
		Scale *= amount;
	}

	void Pitch(float ang) {
		AngX += ang;

		CPoint3D rotDir = -(At - P0).Dir();
		At = P0 + (At - P0).RotateDeg(ang, Up);
		Up = Up.RotateDeg(ang, rotDir);
	}

	void Yaw(float ang) {
		AngY += ang;

		At = P0 + (At - P0).RotateDeg(ang, -Right());
	}

	void Roll(float ang) {
		AngZ -= ang;

		Up = Up.RotateDeg(ang, Right());
	}

	void LookAt()  // call gluLookAt
	{
		gluLookAt((GLdouble)P0.x, (GLdouble)P0.y, (GLdouble)P0.z,
			(GLdouble)At.x, (GLdouble)At.y, (GLdouble)At.z,
			(GLdouble)Up.x, (GLdouble)Up.y, (GLdouble)Up.z);
	}

	void Update()
	{
		glScalef(Scale, Scale, Scale);
		glRotatef(-AngX, 1.0, 0.0, 0.0);
		glRotatef(-AngY, 0.0, 1.0, 0.0);
		glRotatef(-AngZ, 0.0, 0.0, 1.0);
		glTranslatef(-P0.x, -P0.y, -P0.z);

	}
	string ToString()
	{
		return "P0 = " + P0.ToString() + ", At = " + At.ToString() + ", Up = " + Up.ToString() + ", dir = " + (At - P0).ToString();
	}
private:

	CPoint3D Right() {
		return (At - P0).CrossProduct(Up).Dir();
	}
};

//atenção: n é o grau do polinomio.
void Casteljau(GLfloat t, point3D_type a[], point3D_type b[], point3D_type c[], int n)
{
	int i, j;
	GLfloat  t_1 = 1 - t;
	for (i = 0; i <= n; i++)
		c[i] = a[i];
	b[0] = a[0];
	for (j = 1; j <= n; j++)
	{
		for (i = 0; i <= n - j; i++)
		{
			c[i].x = t_1*c[i].x + t*c[i + 1].x;
			c[i].y = t_1*c[i].y + t*c[i + 1].y;
			c[i].z = t_1*c[i].z + t*c[i + 1].z;
		}
		b[j] = c[0];
	}

}

void SaveBezier(char* file_name, point3D_type* a, GLint n, GLint m)
{
	FILE *file;
	file = fopen(file_name, "w");
	fprintf(file, "%d %d\n", n, m);
	for (int j = 0; j<m; j++)
	{
		for (int i = 0; i < n; i++)
		{
			fprintf(file, "%f %f %f , ", a[j*n + i].x, a[j*n + i].y, a[j*n + i].z);
		}
		fprintf(file, "\n");
	}
	fclose(file);
}

bool LoadBezier(char* file_name, point3D_type** ap, GLint* np, GLint* mp)
{
	FILE *file;
	file = fopen(file_name, "r");

	if (file == NULL) {
		return false;
	}

	int n, m;
	fscanf(file, "%d %d\n", &n, &m);
	*np = n;
	*mp = m;

	point3D_type* a = (point3D_type*)malloc(m * n * sizeof(point3D_type));

	for (int j = 0; j<m; j++)
	{
		for (int i = 0; i < n; i++)
		{
			fscanf(file, "%f %f %f , ", &a[j*n + i].x, &a[j*n + i].y, &a[j*n + i].z);
		}
		fscanf(file, "\n");
	}
	*ap = a;

	fclose(file);
	return true;
}

void BezierSubdivision(GLfloat u, GLfloat v, point3D_type a[],
	point3D_type b[], point3D_type c[], point3D_type d[], point3D_type e[],
	GLint n, GLint m)
{
	point3D_type *col_a = new point3D_type[m];
	point3D_type *col_b = new point3D_type[m];
	point3D_type *col_c = new point3D_type[m];
	int i, j;
	for (i = 0; i<n; i++)
	{
		for (j = 0; j<m; j++)
			col_a[j] = a[j*n + i];
		Casteljau(v, col_a, col_b, col_c, m - 1);
		for (j = 0; j<m; j++)
		{
			b[j*n + i] = col_b[j];
			c[j*n + i] = col_c[j];
		}
	}

	for (j = 0; j<m; j++)
	{
		Casteljau(u, &b[j*n], &b[j*n], &d[j*n], n - 1);
		Casteljau(u, &c[j*n], &c[j*n], &e[j*n], n - 1);
	}

}

void DrawBezier(point3D_type a[], GLint n, GLint m, GLint nn, GLint mm)
{
	glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, n, 0, 1, n * 3, m, &(GLfloat &)a[0]);
	glMapGrid2f(n, 0.0, 1.0, m, 0.0, 1.0);
	glEvalMesh2((fillCurve) ? GL_FILL : GL_LINE, 0, nn, 0, mm);
}

bool Limit(point3D_type* a, GLint n, GLint m, char* prefix)
{
	int i, j;
	GLdouble x1, y1, z1, x2, y2, z2, max = 0.0, d;
	GLint ans;
	// analisar os pares (j,i) (j,i+1)
	for (j = 0; j < m; j++)
	{
		for (i = 0; i < n - 1; i++)
		{
			// projetar os pontos de controle (j,i) e (j,i+1)
			ans = gluProject((GLdouble)a[j*n + i].x, (GLdouble)a[j*n + i].y, (GLdouble)a[j*n + i].z,
				mvmatrix, projmatrix, viewport, &x1, &y1, &z1);
			if (ans == GL_FALSE) continue;
			ans = gluProject((GLdouble)a[j*n + i + 1].x, (GLdouble)a[j*n + i + 1].y, (GLdouble)a[j*n + i + 1].z,
				mvmatrix, projmatrix, viewport, &x2, &y2, &z2);
			if (ans == GL_FALSE) continue;
			d = sqr(x2 - x1) + sqr(y2 - y1);
			if (d > max) max = d;
		}
	}

	// analisar os pares (j,i) (j+1,i)
	for (j = 0; j < m - 1; j++)
	{
		for (i = 0; i < n; i++)
		{
			// projetar os pontos de controle (j,i) e (j+1,i)
			ans = gluProject((GLdouble)a[j*n + i].x, (GLdouble)a[j*n + i].y, (GLdouble)a[j*n + i].z,
				mvmatrix, projmatrix, viewport, &x1, &y1, &z1);
			if (ans == GL_FALSE) continue;
			ans = gluProject((GLdouble)a[(j + 1)*n + i].x, (GLdouble)a[(j + 1)*n + i].y, (GLdouble)a[(j + 1)*n + i].z,
				mvmatrix, projmatrix, viewport, &x2, &y2, &z2);
			if (ans == GL_FALSE) continue;
			d = sqr(x2 - x1) + sqr(y2 - y1);
			if (d > max) max = d;
		}
	}

	bool stop = (max <= tolerance);

	//cout << prefix << "max = " + to_string(max) + ", tolerance = " + to_string(tolerance) << "\n";
	return stop;
}

char* repeat(char c, int n) {
	char* str = (char*)malloc(n * sizeof(char) + 1);
	for (int i = 0; i < n; i++) {
		str[i] = c;
	}
	str[n] = '\0';
	return str;
}

void BezierRecursiveSubdivision(point3D_type* a, GLint n, GLint m, int depth = 0, char* orig = "")
{
	int maxd = 50;
	if (depth >= maxd) {
		cout << repeat(' ', depth) << "HALT!\n";
	}

	if (depth >= maxd || Limit(a, n, m, strcat(repeat(' ', depth), orig)))
	{
		DrawBezier(a, n, m, n, m);
	}
	else
	{
		point3D_type* b = new point3D_type[m*n];
		point3D_type* c = new point3D_type[m*n];
		point3D_type* d = new point3D_type[m*n];
		point3D_type* e = new point3D_type[m*n];
		BezierSubdivision(0.5f, 0.5f, a, b, c, d, e, n, m);
		//Limit(a, n, m, "a limit: ");
		//Limit(b, n, m, "b limit: ");

		point3D_type* controls[] = { b, c, d, e };
		for (int i = 0; i < sizeof(controls) / sizeof(point3D_type*); i++)
		{
			char* s = (i == 0) ? "b: " : "c: ";
			BezierRecursiveSubdivision(controls[i], n, m, depth + 1, s);
		}
	}
}

void initlights(void)
{
	GLfloat ambient[] =
	{ 0.1, 0.7, 0.7, 1.0 };
	GLfloat position[] =
	{ 0.0, 0.0, 3.0, 2.0 };
	GLfloat mat_diffuse[] =
	{ 0.1, 0.9, 0.9, 1.0 };
	GLfloat mat_specular[] =
	{ 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] =
	{ 50.0 };

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}

void inittextures(void)
{
	GLuint texid;
	GLfloat texpts[2][2][2] = { { { 0.0, 0.0 }, { 0.0, 1.0 } },
	{ { 1.0, 0.0 }, { 1.0, 1.0 } } };

	CImg<unsigned char> src("texture.bmp");
	src.mirror("y");

	glGenTextures(1, &texid);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D, texid);

	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, src.width(), src.height(), GL_LUMINANCE, GL_UNSIGNED_BYTE, src.data());

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &texpts[0][0][0]);

	glEnable(GL_MAP2_TEXTURE_COORD_2);

	glEnable(GL_TEXTURE_2D);

}

float ratio = 1.0;
int frame, timebase = 0;
char s[30];
CCamera cam;

void DrawControlPoints(point3D_type* points, int n, int m) {
	for (int j = 0; j < m; j++)
	{
		for (int i = 0; i < n; i++)
		{
			point3D_type p = points[j*n + i];
			glPushMatrix();
			glTranslatef(p.x, p.y, p.z);
			glColor3f(1.0f, 0.2f, 0.2f);
			glutSolidSphere(0.01, 10, 10);
			glPopMatrix();
		}
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMapGrid2f(n, 0.0, 1.0, n, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	glLoadIdentity();
	cam.Update();

	glRotatef(85.0, 1.0, 1.0, 1.0);
	if (perspectiva)
	{
		tolerance = sqr(TOL * 2);
		gluPerspective(vAng, asp, nearD, farD);
	}

	if (showControls)
	{
		DrawControlPoints(ctrlpoints, N, M);
	}

	BezierRecursiveSubdivision(ctrlpoints, N, M);

	glPopMatrix();

	glFlush();
	glutSwapBuffers(); // display the screen just made 
}

float angle = 0.0;
float x = 0.0f, y = 0.0f, z = 0.0f;
float lx = 0.0f, ly = 0.0f, lz = -1.0f;

void myinit(void)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_DEPTH_TEST);
	glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &(GLfloat &)ctrlpoints);
	glEnable(GL_MAP2_VERTEX_3);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	cam.Set(
		CPoint3D(x, y, z),
		CPoint3D(x + lx, y + ly, z + lz),
		CPoint3D(0.0f, 1.0f, 0.0f));

	glMapGrid2f(n, 0.0, 1.0, n, 0.0, 1.0);
	initlights();       /* for lighted version only */
	inittextures();
}

void myReshape(int w, int h)
{
	cout << "w = " << to_string(w) << ", h = " + to_string(h) << "\n";

	GLfloat f = 1;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspectiva)
	{
		gluPerspective(vAng, asp, nearD, farD);
	}
	else {
		if (w <= h)
			glOrtho(-4.0, 4.0, -4.0 * (GLfloat)h / (GLfloat)w,
			4.0 * (GLfloat)h / (GLfloat)w, -4.0, 4.0);
		else
			glOrtho(-4.0 * (GLfloat)w / (GLfloat)h,
			4.0 * (GLfloat)w / (GLfloat)h, -4.0, 4.0, -4.0, 4.0);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//cam.setAspect(((float)w)/h);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);
}

void mouse(int button, int state, int x, int y)
{
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {       /*  increase n */
			//   Put code here

			glutPostRedisplay();
		}
		break;

	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN) {      /*  change blue  */
			//   Put code here

			glutPostRedisplay();
		}
		break;
	default:
		break;
	}
}

float anglDelta = 1.5;
const float MIN_ANGL = 0.001;
float distDelta = 1.05;
const float MIN_DIST = 0.001;
float max(float a, float b) { return (a > b) ? a : b; }
void inputKey(unsigned char c, int x, int y) {
	int a = c;
	if ('A' <= c && c <= 'Z') z = z - 'A' + 'a';
	switch (c) {
	case 'i': cam.Zoom(distDelta); break;   // Forward
	case 'k': cam.Zoom(1 / distDelta); break;  // Backward
	case 'w': cam.Pitch(anglDelta); break;         // Look up
	case 's': cam.Pitch(-anglDelta); break;        // Look down
	case 'a': cam.Yaw(-anglDelta); break;          // Look right
	case 'd': cam.Yaw(anglDelta); break;           // Look left
	case 'q': cam.Roll(anglDelta); break;          // Roll CW
	case 'e': cam.Roll(-anglDelta);	break;         // Roll CCW
	case 'm':
		anglDelta = (anglDelta > 1) ? anglDelta + 0.5 : (1 / 0.6) * anglDelta;
		break;
	case 'n':
		anglDelta = (anglDelta > 1) ? anglDelta - 0.5 : max(MIN_ANGL, 0.6 * anglDelta);
		break;
	case 'f': fillCurve = !fillCurve; break;
	case 'g': showControls = !showControls; break;
	}
	glutPostRedisplay();
	//cout << "angle = " << to_string(anglDelta) << ", dist = " << to_string(distDelta) << "\n";
}


int main(int argc, char **argv)
{
	/*
	bool loaded;
	do {
		cout << "Enter bezier control points file: ";
		string filename = "bezier.dat";
		getline(cin, filename);
		loaded = LoadBezier((char*)filename.c_str(), &ctrlpoints, &N, &M);
	} while (!loaded);
	*/
	string filename = "bezier.dat";
	LoadBezier((char*)filename.c_str(), &ctrlpoints, &N, &M);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow(argv[0]);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	myinit();

	glutReshapeFunc(myReshape);
	glutDisplayFunc(display);

	glutKeyboardFunc(inputKey);
	glutMouseFunc(mouse);
	glutMainLoop();
	return 0;             /* ANSI C requires main to return int. */
}
