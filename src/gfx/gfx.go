package gfx

import (
	"mbc/gfx/gl"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/math"
	"solod.dev/so/slices"
	"solod.dev/so/unicode"
)

// Some basic Defines
const (
	Pi      = 3.1415927
	Deg2rad = 0.017453292
	Rad2deg = 57.295776
)

var (
	// Light Gray
	LightGray = Color{200, 200, 200, 255}
	// Gray
	Gray = Color{130, 130, 130, 255}
	// Dark Gray
	DarkGray = Color{80, 80, 80, 255}
	// Yellow
	Yellow = Color{253, 249, 0, 255}
	// Gold
	Gold = Color{255, 203, 0, 255}
	// Orange
	Orange = Color{255, 161, 0, 255}
	// Pink
	Pink = Color{255, 109, 194, 255}
	// Red
	Red = Color{230, 41, 55, 255}
	// Maroon
	Maroon = Color{190, 33, 55, 255}
	// Green
	Green = Color{0, 228, 48, 255}
	// Lime
	Lime = Color{0, 158, 47, 255}
	// Dark Green
	DarkGreen = Color{0, 117, 44, 255}
	// Sky Blue
	SkyBlue = Color{102, 191, 255, 255}
	// Blue
	Blue = Color{0, 121, 241, 255}
	// Dark Blue
	DarkBlue = Color{0, 82, 172, 255}
	// Purple
	Purple = Color{200, 122, 255, 255}
	// Violet
	Violet = Color{135, 60, 190, 255}
	// Dark Purple
	DarkPurple = Color{112, 31, 126, 255}
	// Beige
	Beige = Color{211, 176, 131, 255}
	// Brown
	Brown = Color{127, 106, 79, 255}
	// Dark Brown
	DarkBrown = Color{76, 63, 47, 255}
	// White
	White = Color{255, 255, 255, 255}
	// Black
	Black = Color{0, 0, 0, 255}
	// Blank (Transparent)
	Blank = Color{0, 0, 0, 0}
	// Magenta
	Magenta = Color{255, 0, 255, 255}
	// Ray White (RayLib Logo White)
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

func EnableTexture(t Texture) {
	gl.Enable(gl.TEXTURE_2D)
	gl.BindTexture(gl.TEXTURE_2D, t.ID)
}
func DisableTexture() {
	gl.Disable(gl.TEXTURE_2D)
}

var window *sdl.Window

func Init(win *sdl.Window) {
	window = win
	sdl.GLCreateContext(win)
	width, height := GetWindowSize()
	initGLDefaultState()
	SetupViewport(width, height)
}
func GetWindowSize() (int, int) {
	var w, h sdl.Cint
	sdl.GetWindowSizeInPixels(window, &w, &h)
	return int(w), int(h)
}

func initGLDefaultState() {
	gl.DepthFunc(gl.LEQUAL)
	gl.Disable(gl.DEPTH_TEST)                          // Disable depth testing for 2D (only used for 3D)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA) // Color blending function (how colors are mixed)
	gl.Enable(gl.BLEND)                                // Enable color blending (required to work with transparencies)

	// Init state: Culling
	// NOTE: All shapes/models triangles are drawn CCW
	gl.CullFace(gl.BACK)    // Cull the back face (default)
	gl.FrontFace(gl.CCW)    // Front face are defined counter clockwise (default)
	gl.Enable(gl.CULL_FACE) // Enable backface culling
	gl.EnableClientState(gl.VERTEX_ARRAY)
	gl.EnableClientState(gl.NORMAL_ARRAY)
	gl.EnableClientState(gl.TEXTURE_COORD_ARRAY)

	gl.Enable(gl.RESCALE_NORMAL)
	gl.ShadeModel(gl.SMOOTH) // Smooth shading between vertex (vertex colors interpolation)
	// Init state: Color/Depth buffers clear
	gl.ClearColor(0.0, 0.0, 0.0, 1.0) // Set clear color (black)
	gl.ClearDepth(1.0)                // Set clear depth value (default)
	// Clear color and depth buffers (depth buffer required for 3D)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
}

func BeginDrawing() { gl.LoadIdentity() }
func EndDrawing()   { sdl.GLSwapWindow(window) }
func ClearBackground(c Color) {
	gl.ClearColor(float32(c.R)/255, float32(c.G)/255, float32(c.B)/255, float32(c.A)/255)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
}
func BeginMode3D(cam Camera) {
	var w, h sdl.Cint
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

func EndMode3D() {
	gl.MatrixMode(gl.PROJECTION) // Switch to projection matrix
	gl.PopMatrix()               // Restore previous matrix (projection) from matrix stack

	gl.MatrixMode(gl.MODELVIEW) // Switch back to modelview matrix
	gl.LoadIdentity()           // Reset current matrix (modelview)

	gl.Disable(gl.DEPTH_TEST) // Disable DEPTH_TEST for 2D}
}

func GetCameraMatrix2D(cam Camera2D) Matrix {
	// The camera in world-space is set by
	//   1. Move it to target
	//   2. Rotate by -rotation and scale by (1/zoom)
	//      When setting higher scale, it's more intuitive for the world to become bigger (= camera become smaller),
	//      not for the camera getting bigger, hence the invert. Same deal with rotation
	//   3. Move it by (-offset);
	//      Offset defines target transform relative to screen, but since effectively "moving" screen (camera)
	//      it needs to be moved into opposite direction (inverse transform)

	// Having camera transform in world-space, inverse of it gives the modelview transform
	// Since (A*B*C)' = C'*B'*A', the modelview is
	//   1. Move to offset
	//   2. Rotate and Scale
	//   3. Move by -target
	matOrigin := MatrixTranslate(-cam.Target.X, -cam.Target.Y, 0)
	matRotation := MatrixRotate(Vector3{0.0, 0.0, 1.0}, cam.Rotation*Deg2rad)
	matScale := MatrixScale(cam.Zoom, cam.Zoom, 1.0)
	matTranslation := MatrixTranslate(cam.Offset.X, cam.Offset.Y, 0.0)

	matTransform := MatrixMultiply(MatrixMultiply(matOrigin, MatrixMultiply(matScale, matRotation)), matTranslation)
	return matTransform
}

func BeginMode2D(cam Camera2D) {
	gl.LoadIdentity() // Reset current matrix (modelview)
	matCamera := GetCameraMatrix2D(cam).ToFloat().V[0]
	// Apply 2d camera transformation to modelview
	gl.MultMatrixf(&matCamera)
}
func EndMode2D() { gl.LoadIdentity() }

// Get the screen space position for a 2d camera world space position
func GetWorldToScreen2D(position Vector2, camera Camera2D) Vector2 {
	matCamera := GetCameraMatrix2D(camera)
	transform := Vector3Transform(Vector3{position.X, position.Y, 0}, matCamera)

	return Vector2{transform.X, transform.Y}
}

// Get the world space position for a 2d camera screen space position
func GetScreenToWorld2D(position Vector2, camera Camera2D) Vector2 {
	invMatCamera := MatrixInvert(GetCameraMatrix2D(camera))
	transform := Vector3Transform(Vector3{position.X, position.Y, 0}, invMatCamera)

	return Vector2{transform.X, transform.Y}
}

// Image backed by an RGBA32 SDL3 surface.
type Image struct {
	Surface *sdl.Surface
}

func LoadImage(path string) (Image, error) {
	src := sdl.LoadSurface(path)
	defer sdl.DestroySurface(src)
	if src == nil {
		return Image{}, sdl.GetError()
	}

	converted := sdl.ConvertSurface(src, sdl.PIXELFORMAT_RGBA32)
	if converted == nil {
		return Image{}, sdl.GetError()
	}
	return Image{Surface: converted}, nil
}
func (i *Image) Destroy() {
	sdl.DestroySurface(i.Surface)
}
func (i *Image) Size() (int, int) {
	return i.Surface.Width(), i.Surface.Height()
}

// Get a pixel from the image.
func (i *Image) Get(x, y int) Color {
	if x < 0 || y < 0 || x >= i.Surface.Width() || y >= i.Surface.Height() {
		panic("out of bounds")
	}
	s := i.Surface
	base := s.Pixels()
	p := c.PtrAdd(base, y*s.Pitch()+x*4)

	return Color{
		R: *p,
		G: *(c.PtrAdd(p, 1)),
		B: *(c.PtrAdd(p, 2)),
		A: *(c.PtrAdd(p, 3)),
	}
}
func (i *Image) Pixels() []uint8 {
	base := i.Surface.Pixels()
	size := 4 * i.Surface.Width() * i.Surface.Height()
	return c.Slice(base, size, size)
}
func LoadTextureFromImage(img Image) (Texture, error) {
	t := Texture{}
	t.Width, t.Height = img.Size()

	gl.GenTextures(1, &t.ID)
	gl.BindTexture(gl.TEXTURE_2D, t.ID)

	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA, int32(t.Width), int32(t.Height), 0, gl.RGBA, gl.UNSIGNED_BYTE, img.Surface.Pixels())
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)

	return t, nil
}
func LoadTexture(path string) (Texture, error) {
	img, err := LoadImage(path)
	// this code is ugly because of https://github.com/solod-dev/solod/issues/76
	defer img.Destroy()
	if err != nil {
		return Texture{}, err
	}
	t, err := LoadTextureFromImage(img)
	if err != nil {
		return t, err
	}
	return t, nil

}
func SetTextureConfig(t Texture, blur bool, clamp bool) {
	EnableTexture(t)
	defer DisableTexture()
	if blur {
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	} else {
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
	}
	if clamp {
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP)
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP)
	} else {
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
		gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
	}
}

func UnloadTexture(texture Texture) {
	if texture.ID != 0 {
		gl.DeleteTextures(1, &texture.ID)
	}
}
func DrawTexture(texture Texture, x, y float32) {
	DrawTexturePro(
		texture,
		NewRectangle(0, 0, float32(texture.Width), float32(texture.Height)),
		NewRectangle(float32(x), float32(y), float32(texture.Width), float32(texture.Height)),
		Vector2{}, 0, White)
}
func DrawTextureRec(texture Texture, src, dst Rectangle) {
	DrawTexturePro(texture, src, dst, Vector2{}, 0, White)
}
func DrawTextureTiled(
	texture Texture,
	dest Rectangle,
	scale float32,
	tint Color,
) {
	if texture.ID == 0 {
		return
	}

	if scale <= 0 {
		scale = 1
	}

	tileW := float32(texture.Width) * scale
	tileH := float32(texture.Height) * scale

	// UVs larger than 1.0 cause GL_REPEAT wrapping
	u := dest.Width / tileW
	v := dest.Height / tileH

	EnableTexture(texture)

	gl.Begin(gl.QUADS)

	gl.Color4ub(tint.R, tint.G, tint.B, tint.A)
	gl.Normal3f(0, 0, 1)

	// Top-left
	gl.TexCoord2f(0, 0)
	gl.Vertex2f(dest.X, dest.Y)

	// Bottom-left
	gl.TexCoord2f(0, v)
	gl.Vertex2f(dest.X, dest.Y+dest.Height)

	// Bottom-right
	gl.TexCoord2f(u, v)
	gl.Vertex2f(dest.X+dest.Width, dest.Y+dest.Height)

	// Top-right
	gl.TexCoord2f(u, 0)
	gl.Vertex2f(dest.X+dest.Width, dest.Y)

	gl.End()

	DisableTexture()
}

// DrawTexturePro draws a portion of a texture into a destination rectangle,
// optionally rotated around origin.
//
// origin is relative to dest's size, matching raylib-style semantics.
func DrawTexturePro(texture Texture, source, dest Rectangle, origin Vector2, rotation float32, tint Color) {
	if texture.ID == 0 {
		return
	}

	width := float32(texture.Width)
	height := float32(texture.Height)

	flipX := false

	if source.Width < 0 {
		flipX = true
		source.Width = -source.Width
	}
	if source.Height < 0 {
		source.Y -= source.Height
		source.Height = -source.Height
	}

	if dest.Width < 0 {
		dest.Width = -dest.Width
	}
	if dest.Height < 0 {
		dest.Height = -dest.Height
	}

	var topLeft, topRight, bottomLeft, bottomRight Vector2

	if rotation == 0 {
		x := dest.X - origin.X
		y := dest.Y - origin.Y

		topLeft = Vector2{x, y}
		topRight = Vector2{x + dest.Width, y}
		bottomLeft = Vector2{x, y + dest.Height}
		bottomRight = Vector2{x + dest.Width, y + dest.Height}
	} else {
		rad := rotation * (math.Pi / 180.0)
		sinR := float32(math.Sin(float64(rad)))
		cosR := float32(math.Cos(float64(rad)))

		x := dest.X
		y := dest.Y
		dx := -origin.X
		dy := -origin.Y

		topLeft.X = x + dx*cosR - dy*sinR
		topLeft.Y = y + dx*sinR + dy*cosR

		topRight.X = x + (dx+dest.Width)*cosR - dy*sinR
		topRight.Y = y + (dx+dest.Width)*sinR + dy*cosR

		bottomLeft.X = x + dx*cosR - (dy+dest.Height)*sinR
		bottomLeft.Y = y + dx*sinR + (dy+dest.Height)*cosR

		bottomRight.X = x + (dx+dest.Width)*cosR - (dy+dest.Height)*sinR
		bottomRight.Y = y + (dx+dest.Width)*sinR + (dy+dest.Height)*cosR
	}

	u0 := source.X / width
	v0 := source.Y / height
	u1 := (source.X + source.Width) / width
	v1 := (source.Y + source.Height) / height

	EnableTexture(texture)
	gl.Begin(gl.QUADS)

	gl.Color4ub(tint.R, tint.G, tint.B, tint.A)
	gl.Normal3f(0, 0, 1)

	// Top-left
	if flipX {
		gl.TexCoord2f(u1, v0)
	} else {
		gl.TexCoord2f(u0, v0)
	}
	gl.Vertex2f(topLeft.X, topLeft.Y)

	// Bottom-left
	if flipX {
		gl.TexCoord2f(u1, v1)
	} else {
		gl.TexCoord2f(u0, v1)
	}
	gl.Vertex2f(bottomLeft.X, bottomLeft.Y)

	// Bottom-right
	if flipX {
		gl.TexCoord2f(u0, v1)
	} else {
		gl.TexCoord2f(u1, v1)
	}
	gl.Vertex2f(bottomRight.X, bottomRight.Y)

	// Top-right
	if flipX {
		gl.TexCoord2f(u0, v0)
	} else {
		gl.TexCoord2f(u1, v0)
	}
	gl.Vertex2f(topRight.X, topRight.Y)

	gl.End()
	DisableTexture()
}

// drawTexturePro but the user should call:
// EnableTexture(texture)
// gl.Begin(gl.QUADS)
//
// drawTextureProUnsafe()
//
// gl.End()
// DisableTexture()
func drawTextureProUnsafe(texture Texture, source, dest Rectangle, tint Color) {

	if texture.ID == 0 {
		return
	}

	width := float32(texture.Width)
	height := float32(texture.Height)

	flipX := false

	if source.Width < 0 {
		flipX = true
		source.Width = -source.Width
	}
	if source.Height < 0 {
		source.Y -= source.Height
		source.Height = -source.Height
	}

	if dest.Width < 0 {
		dest.Width = -dest.Width
	}
	if dest.Height < 0 {
		dest.Height = -dest.Height
	}

	var topLeft, topRight, bottomLeft, bottomRight Vector2

	x := dest.X
	y := dest.Y

	topLeft = Vector2{x, y}
	topRight = Vector2{x + dest.Width, y}
	bottomLeft = Vector2{x, y + dest.Height}
	bottomRight = Vector2{x + dest.Width, y + dest.Height}

	u0 := source.X / width
	v0 := source.Y / height
	u1 := (source.X + source.Width) / width
	v1 := (source.Y + source.Height) / height

	EnableTexture(texture)
	gl.Begin(gl.QUADS)

	gl.Color4ub(tint.R, tint.G, tint.B, tint.A)
	gl.Normal3f(0, 0, 1)

	// Top-left
	if flipX {
		gl.TexCoord2f(u1, v0)
	} else {
		gl.TexCoord2f(u0, v0)
	}
	gl.Vertex2f(topLeft.X, topLeft.Y)

	// Bottom-left
	if flipX {
		gl.TexCoord2f(u1, v1)
	} else {
		gl.TexCoord2f(u0, v1)
	}
	gl.Vertex2f(bottomLeft.X, bottomLeft.Y)

	// Bottom-right
	if flipX {
		gl.TexCoord2f(u0, v1)
	} else {
		gl.TexCoord2f(u1, v1)
	}
	gl.Vertex2f(bottomRight.X, bottomRight.Y)

	// Top-right
	if flipX {
		gl.TexCoord2f(u0, v0)
	} else {
		gl.TexCoord2f(u1, v0)
	}
	gl.Vertex2f(topRight.X, topRight.Y)

	gl.End()
	DisableTexture()
}

const glyphsPerRow = 16

// Load Minecraft bitmap font
func LoadFont(path string) (Font, error) {
	img, err := LoadImage(path)
	defer img.Destroy()
	if err != nil {
		return Font{}, err
	}
	fnt := Font{}
	fnt.Atlas, err = LoadTextureFromImage(img)
	if err != nil {
		return fnt, err
	}

	atlasSize := img.Surface.Width()
	glyphSize := atlasSize / glyphsPerRow

	for charCode := range 256 {
		col := charCode % glyphsPerRow
		row := charCode / glyphsPerRow

		glyphWidth := glyphSize - 1

		for glyphWidth >= 0 {
			emptyColumn := true

			pixelX := col*glyphSize + glyphWidth

			for y := range glyphSize {
				pixelY := row*glyphSize + y

				if img.Get(pixelX, pixelY).A > 0 {
					emptyColumn = false
					break
				}
			}

			if !emptyColumn {
				break
			}

			glyphWidth--
		}

		if charCode == ' ' {
			glyphWidth = 2
		}

		fnt.CharWidths[charCode] = uint8(glyphWidth + 2)
	}
	return fnt, nil
}

// https://minecraft.wiki/w/Formatting_codes
const SectionSign rune = '§'

func (fnt *Font) DrawString(text string, x, y float32, size float32, color Color) {
	fnt.DrawRunes([]rune(text), x, y, size, color, false)
}
func (fnt *Font) DrawRunes(text []rune, x, y float32, size float32, color Color, darken bool) {
	if len(text) == 0 {
		return
	}

	if darken {
		color.R /= 4
		color.G /= 4
		color.B /= 4
	}

	cellSize := float32(fnt.Atlas.Width / glyphsPerRow)
	penX := x

	// use drawTextureProUnsafe to avoid state switching per character.
	EnableTexture(fnt.Atlas)
	defer DisableTexture()
	gl.Begin(gl.QUADS)
	defer gl.End()

	for i := 0; i < len(text); i++ {
		// exotic notch code :D
		for len(text) > i+1 && text[i] == '§' { //colored text using format strings
			colorCode := slices.Index(
				[]rune("0123456789abcdef"),
				unicode.ToLower(text[i+1]),
			)
			if colorCode < 0 {
				colorCode = 15
			}
			i += 2

			colorIndex := uint8(colorCode)
			if darken {
				colorIndex += 16
			}

			base := uint8((colorIndex >> 3 & 1) * 85)
			red := uint8((colorIndex>>2&1)*170 + base)
			green := uint8((colorIndex>>1&1)*170 + base)
			blue := uint8((colorIndex>>0&1)*170 + base)

			if colorIndex == 6 {
				green += 85
			}
			if colorIndex >= 16 {
				red /= 4
				green /= 4
				blue /= 4
			}

			color = Color{red, green, blue, color.A}
		}

		charCode := text[i]
		col := charCode % glyphsPerRow
		row := charCode / glyphsPerRow

		src := Rectangle{
			X:      float32(col) * cellSize,
			Y:      float32(row) * cellSize,
			Width:  cellSize,
			Height: cellSize,
		}

		dst := Rectangle{
			X:      penX,
			Y:      y,
			Width:  cellSize * size,
			Height: cellSize * size,
		}

		drawTextureProUnsafe(fnt.Atlas, src, dst, color) // slight performance increase?
		// DrawTexturePro(fnt.Atlas, src, dst, Vector2{}, 0, color)

		penX += float32(fnt.CharWidths[charCode]) * size
	}
}

// Draw a color-filled rectangle with pro parameters
// DrawRectanglePro draws a color-filled rectangle with rotation and origin.
//
// origin is relative to rectangle size, matching raylib semantics.
func DrawRectanglePro(rectangle Rectangle, origin Vector2, rotation float32, color Color) {
	var topLeft, topRight, bottomLeft, bottomRight Vector2

	// Normalize negative sizes
	if rectangle.Width < 0 {
		rectangle.X += rectangle.Width
		rectangle.Width = -rectangle.Width
	}

	if rectangle.Height < 0 {
		rectangle.Y += rectangle.Height
		rectangle.Height = -rectangle.Height
	}

	// Fast path: no rotation
	if rotation == 0 {
		x := rectangle.X - origin.X
		y := rectangle.Y - origin.Y

		topLeft = Vector2{x, y}
		topRight = Vector2{x + rectangle.Width, y}
		bottomLeft = Vector2{x, y + rectangle.Height}
		bottomRight = Vector2{x + rectangle.Width, y + rectangle.Height}
	} else {
		rad := rotation * Deg2rad

		sinR := float32(math.Sin(float64(rad)))
		cosR := float32(math.Cos(float64(rad)))

		x := rectangle.X
		y := rectangle.Y

		dx := -origin.X
		dy := -origin.Y

		topLeft.X = x + dx*cosR - dy*sinR
		topLeft.Y = y + dx*sinR + dy*cosR

		topRight.X = x + (dx+rectangle.Width)*cosR - dy*sinR
		topRight.Y = y + (dx+rectangle.Width)*sinR + dy*cosR

		bottomLeft.X = x + dx*cosR - (dy+rectangle.Height)*sinR
		bottomLeft.Y = y + dx*sinR + (dy+rectangle.Height)*cosR

		bottomRight.X = x + (dx+rectangle.Width)*cosR - (dy+rectangle.Height)*sinR
		bottomRight.Y = y + (dx+rectangle.Width)*sinR + (dy+rectangle.Height)*cosR
	}

	gl.Disable(gl.TEXTURE_2D)

	gl.Begin(gl.QUADS)

	gl.Color4ub(color.R, color.G, color.B, color.A)
	gl.Normal3f(0, 0, 1)

	gl.Vertex2f(topLeft.X, topLeft.Y)
	gl.Vertex2f(bottomLeft.X, bottomLeft.Y)
	gl.Vertex2f(bottomRight.X, bottomRight.Y)
	gl.Vertex2f(topRight.X, topRight.Y)

	gl.End()
}
