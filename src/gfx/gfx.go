package gfx

import (
	"mbc/gfx/gl"
	"mbc/sdl"

	"solod.dev/so/math"
)

// Some basic Defines
const (
	Pi      = 3.1415927
	Deg2rad = 0.017453292
	Rad2deg = 57.295776
)

// Set viewport for a provided width and height
func SetupViewport(width, height int) {
	gl.Viewport(0, 0, width, height)

	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()
	gl.Ortho(0, float64(width), float64(height), 0, 0.0, 1.0)

	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
}

var window *sdl.Window

func Init(win *sdl.Window) {
	window = win
	sdl.GLCreateContext(win)
	var width, height int
	sdl.GetWindowSizeInPixels(win, &width, &height)

	SetupViewport(width, height)
}

func BeginDrawing() { gl.LoadIdentity() }
func EndDrawing()   { sdl.GLSwapWindow(window) }
func BeginMode3D(cam Camera) {
	var w, h int
	sdl.GetWindowSizeInPixels(window, &w, &h)

	gl.MatrixMode(gl.PROJECTION)
	gl.PushMatrix()

	gl.LoadIdentity()

	aspect := float32(w) / float32(h)

	top := CameraCullDistanceNear * math.Tan(float64(cam.Fovy*0.5*Deg2rad))
	right := top * float64(aspect)

	// perspective projection
	gl.Frustum(-right, right, -top, top, CameraCullDistanceNear, CameraCullDistanceFar)

	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()

	matView := MatrixLookAt(cam.Position, cam.Target, cam.Up)
	// modelview * projection
	gl.MultMatrixf(matView.ToFloat())
}
