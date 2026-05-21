package gl

import "mbc/sdl"

var Window *sdl.Window
type enum int
const(
//so:extern GL_PROJECTION
PROJECTION enum = iota

//so:extern GL_MODELVIEW
MODELVIEW

//so:extern GL_TEXTURE
TEXTURE 
	
)

//so:extern glFrustum
func Frustum(left, right, bottom, top, znear, zfar float64)

//so:extern glOrtho
func Ortho(left, right, bottom, top, znear, zfar float64)

//so:extern glPushMatrix
func PushMatrix()
//so:extern glPopMatrix
func PopMatrix() 
//so:extern glLoadIdentity
func LoadIdentity() 
//so:extern glTranslatef
func Translatef(x, y, z float32) 
//so:extern glRotatef
func Rotatef(angle, x, y, z float32) 
//so:extern glScalef
func Scalef(x, y, z float32) 
//so:extern glMultMatrixf
func MultMatrixf(matf []float32) 
//so:extern glViewport
func Viewport(x,y,w,h int) 
//so:extern glMatrixMode
func MatrixMode(enum)
