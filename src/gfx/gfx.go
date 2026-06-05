package gfx

import (
	"mbc/gfx/gl"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/math"
	"solod.dev/so/slices"
	"solod.dev/so/unicode"
)

//so:embed rlgl.c

// Some basic Defines
const (
	Pi      = 3.1415927
	Deg2rad = 0.017453292
	Rad2deg = 57.295776
)

// Java edition chat colors
var (
	Black       = Color{0, 0, 0, 255}       // §0
	DarkBlue    = Color{0, 0, 170, 255}     // §1
	DarkGreen   = Color{0, 170, 0, 255}     // §2
	DarkAqua    = Color{0, 170, 170, 255}   // §3
	DarkRed     = Color{170, 0, 0, 255}     // §4
	DarkPurple  = Color{170, 0, 170, 255}   // §5
	Gold        = Color{255, 170, 0, 255}   // §6
	Gray        = Color{170, 170, 170, 255} // §7
	DarkGray    = Color{85, 85, 85, 255}    // §8
	Blue        = Color{85, 85, 255, 255}   // §9
	Green       = Color{85, 255, 85, 255}   // §a
	Aqua        = Color{85, 255, 255, 255}  // §b
	Red         = Color{255, 85, 85, 255}   // §c
	LightPurple = Color{255, 85, 255, 255}  // §d
	Yellow      = Color{255, 255, 0, 255}   //
	White       = Color{255, 255, 255, 255} // §f
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

var Window *sdl.Window
var AssetsPath string

func Init(win *sdl.Window) {
	Window = win
	sdl.GLCreateContext(win)
	width, height := GetWindowSize()
	initGLDefaultState()
	SetupViewport(width, height)
	switch sdl.GetPlatform() {
	default:
		AssetsPath = "./assets"
	case "Android":
		AssetsPath = sdl.GetBasePath()
	}
}
func GetWindowSize() (int, int) {
	var w, h sdl.Cint
	sdl.GetWindowSizeInPixels(Window, &w, &h)
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
func EndDrawing()   { sdl.GLSwapWindow(Window) }
func ClearBackground(c Color) {
	gl.ClearColor(float32(c.R)/255, float32(c.G)/255, float32(c.B)/255, float32(c.A)/255)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
}
func BeginMode3D(cam Camera) {
	var w, h sdl.Cint
	sdl.GetWindowSizeInPixels(Window, &w, &h)

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

	gl.PixelStorei(gl.UNPACK_ALIGNMENT, 1)

	gl.TexImage2D(
		gl.TEXTURE_2D,
		0,
		gl.RGBA,
		int32(t.Width),
		int32(t.Height),
		0,
		gl.RGBA,
		gl.UNSIGNED_BYTE,
		img.Surface.Pixels(),
	)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)

	return t, nil
}

var TexturesLoaded = 0

// Approximately the GPU memory used by textures in bytes.
var TextureMemoryUsed = 0

func LoadTexture(path string) (Texture, error) {
	img, err := LoadImage(path)
	defer img.Destroy()
	if err != nil {
		return Texture{}, err
	}
	t, err := LoadTextureFromImage(img)
	if err != nil {
		return t, err
	}
	TexturesLoaded++
	TextureMemoryUsed += t.Width * t.Height * 4
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
		TexturesLoaded--
		TextureMemoryUsed -= texture.Width * texture.Height * 4
		gl.DeleteTextures(1, &texture.ID)
	}
}
func DrawTexture(texture Texture, pos Vector2) {
	DrawTextureEx(texture,
		NewRectangle(0, 0, float32(texture.Width), float32(texture.Height)),
		NewRectangle(float32(pos.X), float32(pos.Y), float32(texture.Width), float32(texture.Height)),
	)
}
func DrawTextureEx(texture Texture, src, dst Rectangle) {
	DrawTexturePro(
		texture,
		src,
		dst,
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
	u := dest.W / tileW
	v := dest.H / tileH

	EnableTexture(texture)

	gl.Begin(gl.QUADS)

	gl.Color4ub(tint.R, tint.G, tint.B, tint.A)
	gl.Normal3f(0, 0, 1)

	// Top-left
	gl.TexCoord2f(0, 0)
	gl.Vertex2f(dest.X, dest.Y)

	// Bottom-left
	gl.TexCoord2f(0, v)
	gl.Vertex2f(dest.X, dest.Y+dest.H)

	// Bottom-right
	gl.TexCoord2f(u, v)
	gl.Vertex2f(dest.X+dest.W, dest.Y+dest.H)

	// Top-right
	gl.TexCoord2f(u, 0)
	gl.Vertex2f(dest.X+dest.W, dest.Y)

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

	if source.W < 0 {
		flipX = true
		source.W = -source.W
	}
	if source.H < 0 {
		source.Y -= source.H
		source.H = -source.H
	}

	if dest.W < 0 {
		dest.W = -dest.W
	}
	if dest.H < 0 {
		dest.H = -dest.H
	}

	var topLeft, topRight, bottomLeft, bottomRight Vector2

	if rotation == 0 {
		x := dest.X - origin.X
		y := dest.Y - origin.Y

		topLeft = Vector2{x, y}
		topRight = Vector2{x + dest.W, y}
		bottomLeft = Vector2{x, y + dest.H}
		bottomRight = Vector2{x + dest.W, y + dest.H}
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

		topRight.X = x + (dx+dest.W)*cosR - dy*sinR
		topRight.Y = y + (dx+dest.W)*sinR + dy*cosR

		bottomLeft.X = x + dx*cosR - (dy+dest.H)*sinR
		bottomLeft.Y = y + dx*sinR + (dy+dest.H)*cosR

		bottomRight.X = x + (dx+dest.W)*cosR - (dy+dest.H)*sinR
		bottomRight.Y = y + (dx+dest.W)*sinR + (dy+dest.H)*cosR
	}

	u0 := source.X / width
	v0 := source.Y / height
	u1 := (source.X + source.W) / width
	v1 := (source.Y + source.H) / height

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

// These are all the characters allowed by Minecraft.
func IsRuneAllowed(r rune) bool {
	return r >= 0 && r <= 255
}

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
//
// NOTE: only color formatting codes are supported in beta 1.7.3
const SectionSign rune = '§'

// TextHeight is the same as the full glyph bounding box in the Atlas.
func (fnt *Font) TextHeight() int {
	return fnt.Atlas.Width / glyphsPerRow
}
func (fnt *Font) TextSize(text []rune) Vector2 {
	return Vector2{X: float32(fnt.TextWidth(text)), Y: float32(fnt.TextHeight())}
}
func (fnt *Font) GlyphSize(charCode rune) Vector2 {

	return Vector2{X: float32(fnt.CharWidths[charCode]), Y: float32(fnt.TextHeight())}
}

// Get text width.
func (fnt *Font) TextWidth(text []rune) int {
	if len(text) == 0 {
		return 0
	}
	var width int = 0.0
	for i := 0; i < len(text); i++ {
		r := text[i]
		if r == SectionSign {
			i++
			continue
		}
		if IsRuneAllowed(r) {
			width += int(fnt.CharWidths[r])
		}
	}
	return width
}
func (fnt *Font) Destroy() {
	UnloadTexture(fnt.Atlas)
	*fnt = Font{}
}

func (fnt *Font) DrawRunes(text []rune, position Vector2, scale, rotation float32, color Color, darken bool) {
	if len(text) == 0 {
		return
	}

	if darken {
		color.R /= 4
		color.G /= 4
		color.B /= 4
	}

	cellSize := float32(fnt.TextHeight())
	textSize := fnt.TextSize(text).Scale(scale)

	// use drawTextureProUnsafe to avoid state switching per character.
	// Pivot at center of the whole text block.
	pivot := position.Add(textSize.Half())
	gl.PushMatrix()
	defer gl.PopMatrix()

	// Move to pivot, rotate, then move back to local text space.
	gl.Translatef(pivot.X, pivot.Y, 0)
	gl.Rotatef(rotation, 0, 0, 1)
	gl.Translatef(-textSize.X*0.5, -textSize.Y*0.5, 0)

	textOffsetX := float32(0)
	for i := 0; i < len(text); i++ {
		for len(text) > i+1 && text[i] == SectionSign { // colored text using format strings
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
			// no clue wtf this is, thanks Notch!
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
			X: float32(col) * cellSize,
			Y: float32(row) * cellSize,
			W: cellSize,
			H: cellSize,
		}

		dst := Rectangle{
			X: textOffsetX,
			Y: 0,
			W: cellSize * float32(scale),
			H: cellSize * float32(scale),
		}

		DrawTexturePro(fnt.Atlas, src, dst, Vector2{}, 0, color)

		textOffsetX += float32(fnt.CharWidths[charCode]) * scale
	}
}
func DrawRectangle(rectangle Rectangle, color Color) {
	DrawRectanglePro(rectangle, Vector2{}, 0, color)
}

// Draw a color-filled rectangle with pro parameters
// DrawRectanglePro draws a color-filled rectangle with rotation and origin.
//
// origin is relative to rectangle size, matching raylib semantics.
func DrawRectanglePro(rectangle Rectangle, origin Vector2, rotation float32, color Color) {
	var topLeft, topRight, bottomLeft, bottomRight Vector2

	// Normalize negative sizes
	if rectangle.W < 0 {
		rectangle.X += rectangle.W
		rectangle.W = -rectangle.W
	}

	if rectangle.H < 0 {
		rectangle.Y += rectangle.H
		rectangle.H = -rectangle.H
	}

	// Fast path: no rotation
	if rotation == 0 {
		x := rectangle.X - origin.X
		y := rectangle.Y - origin.Y

		topLeft = Vector2{x, y}
		topRight = Vector2{x + rectangle.W, y}
		bottomLeft = Vector2{x, y + rectangle.H}
		bottomRight = Vector2{x + rectangle.W, y + rectangle.H}
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

		topRight.X = x + (dx+rectangle.W)*cosR - dy*sinR
		topRight.Y = y + (dx+rectangle.W)*sinR + dy*cosR

		bottomLeft.X = x + dx*cosR - (dy+rectangle.H)*sinR
		bottomLeft.Y = y + dx*sinR + (dy+rectangle.H)*cosR

		bottomRight.X = x + (dx+rectangle.W)*cosR - (dy+rectangle.H)*sinR
		bottomRight.Y = y + (dx+rectangle.W)*sinR + (dy+rectangle.H)*cosR
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
