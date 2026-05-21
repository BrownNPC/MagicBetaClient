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
	gl.Viewport(0, 0, int32(width), int32(height))

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
	mv := matView.ToFloat()
	gl.MultMatrixf(&mv.V[0])
	gl.Enable(gl.DEPTH_TEST)
}

func LoadTexture(path string) (Texture, error) {
	src := sdl.LoadSurface(path)
	if src == nil {
		return Texture{}, sdl.GetError()
	}
	defer sdl.DestroySurface(src)

	converted := sdl.ConvertSurface(src, sdl.PIXELFORMAT_RGBA32)
	defer sdl.DestroySurface(converted)

	t := Texture{
		Width:  converted.Width(),
		Height: converted.Height(),
	}

	gl.GenTextures(1, &t.ID)
	gl.BindTexture(gl.TEXTURE_2D, t.ID)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	gl.TexImage2D(t.ID, 0, gl.RGBA, int32(t.Width), int32(t.Height), 0, gl.RGBA, gl.UNSIGNED_BYTE, converted.Pixels())

	return t, nil
}

