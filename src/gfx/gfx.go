package gfx

import (
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/math"
	"solod.dev/so/slices"
	"solod.dev/so/unicode"
)

//so:include "rlgl-master.h"

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
	// gl.Viewport(0, 0, int32(width), int32(height))
	rlViewport(0, 0, int32(width), int32(height))
	rlMatrixMode(rlPROJECTION)
	rlLoadIdentity()
	rlOrtho(0, float64(width), float64(height), 0, 0.0, 1.0)

	rlMatrixMode(rlMODELVIEW)
	rlLoadIdentity()
}

func EnableTexture(t Texture) {
	rlSetTexture(int(t.ID))
}
func DisableTexture() {
	rlSetTexture(0)
}

var Window *sdl.Window
var AssetsPath string

func Init(win *sdl.Window) {
	Window = win
	sdl.GLCreateContext(win)
	width, height := GetWindowSize()
	rlLoadExtensions(sdl.GLGetProcAddress)

	rlglInit(width, height)

	// initGLDefaultState()
	SetupViewport(width, height)
	switch sdl.GetPlatform() {
	default:
		AssetsPath = "./assets"
	case "Android":
		AssetsPath = "./assets"
	}
}
func GetWindowSize() (int, int) {
	var w, h c.Int
	sdl.GetWindowSizeInPixels(Window, &w, &h)
	return int(w), int(h)
}

func BeginDrawing() { rlLoadIdentity() }
func EndDrawing()   { rlDrawRenderBatchActive(); sdl.GLSwapWindow(Window) }

func ClearBackground(c Color) {
	rlClearColor(float32(c.R)/255, float32(c.G)/255, float32(c.B)/255, float32(c.A)/255)
	rlClearScreenBuffers()
}
func BeginMode3D(cam Camera) {
	var w, h c.Int
	sdl.GetWindowSizeInPixels(Window, &w, &h)

	rlMatrixMode(rlPROJECTION)
	rlPushMatrix()

	rlLoadIdentity()

	aspect := float32(w) / float32(h)

	top := CameraCullDistanceNear * math.Tan(float64(cam.Fovy*0.5*Deg2rad))
	right := top * float64(aspect)

	// perspective projection
	rlFrustum(-right, right, -top, top, CameraCullDistanceNear, CameraCullDistanceFar)

	rlMatrixMode(rlMODELVIEW)
	rlLoadIdentity()

	matView := MatrixLookAt(cam.Position, cam.Target, cam.Up)
	// modelview * projection
	mv := matView.ToFloat()
	rlMultMatrixf(&mv.V[0])
	rlEnableDepthTest()
}

func EndMode3D() {
	rlMatrixMode(rlPROJECTION) // Switch to projection matrix
	rlPopMatrix()              // Restore previous matrix (projection) from matrix stack

	rlMatrixMode(rlMODELVIEW) // Switch back to modelview matrix
	rlLoadIdentity()          // Reset current matrix (modelview)

	// Disable DEPTH_TEST for 2D
	rlDisableDepthTest()
}

func BeginMode2D(cam Camera2D) {
	rlLoadIdentity() // Reset current matrix (modelview)
	matCamera := GetCameraMatrix2D(cam).ToFloat().V[0]
	// Apply 2d camera transformation to modelview
	rlMultMatrixf(&matCamera)
}

func EndMode2D() { rlDrawRenderBatchActive(); rlLoadIdentity() }

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
	t.ID = rlLoadTexture(img.Surface.Pixels(), t.Width, t.Height, rlPIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1)
	return t, nil

	// gl.GenTextures(1, &t.ID)
	// gl.BindTexture(gl.TEXTURE_2D, t.ID)

	// gl.PixelStorei(gl.UNPACK_ALIGNMENT, 1)

	// gl.TexImage2D(
	// 	gl.TEXTURE_2D,
	// 	0,
	// 	gl.RGBA,
	// 	int32(t.Width),
	// 	int32(t.Height),
	// 	0,
	// 	gl.RGBA,
	// 	gl.UNSIGNED_BYTE,
	// 	img.Surface.Pixels(),
	// )

	// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
	// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)

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
	// if blur {
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	// } else {
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
	// }
	// if clamp {
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP)
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP)
	// } else {
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
	// 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
	// }
}

func UnloadTexture(texture Texture) {
	if texture.ID != 0 {
		TexturesLoaded--
		TextureMemoryUsed -= texture.Width * texture.Height * 4
		rlUnloadTexture(texture.ID)
		// gl.DeleteTextures(1, &texture.ID)
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

	rlBegin(rlQUADS)

	rlColor4ub(tint.R, tint.G, tint.B, tint.A)
	rlNormal3f(0, 0, 1)

	// Top-left
	rlTexCoord2f(0, 0)
	rlVertex2f(dest.X, dest.Y)

	DisableTexture()
	rlTexCoord2f(0, v)
	rlVertex2f(dest.X, dest.Y+dest.H)

	// Bottom-right
	rlTexCoord2f(u, v)
	rlVertex2f(dest.X+dest.W, dest.Y+dest.H)

	// Top-right
	rlTexCoord2f(u, 0)
	rlVertex2f(dest.X+dest.W, dest.Y)

	rlEnd()

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
		source.W *= -1
	}

	// Match raylib exactly
	if source.H < 0 {
		source.Y -= source.H
	}

	if dest.W < 0 {
		dest.W *= -1
	}

	if dest.H < 0 {
		dest.H *= -1
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

	EnableTexture(texture)

	rlBegin(rlQUADS)

	rlColor4ub(tint.R, tint.G, tint.B, tint.A)
	rlNormal3f(0, 0, 1)

	// Top-left
	if flipX {
		rlTexCoord2f((source.X+source.W)/width, source.Y/height)
	} else {
		rlTexCoord2f(source.X/width, source.Y/height)
	}
	rlVertex2f(topLeft.X, topLeft.Y)

	// Bottom-left
	if flipX {
		rlTexCoord2f((source.X+source.W)/width, (source.Y+source.H)/height)
	} else {
		rlTexCoord2f(source.X/width, (source.Y+source.H)/height)
	}
	rlVertex2f(bottomLeft.X, bottomLeft.Y)

	// Bottom-right
	if flipX {
		rlTexCoord2f(source.X/width, (source.Y+source.H)/height)
	} else {
		rlTexCoord2f((source.X+source.W)/width, (source.Y+source.H)/height)
	}
	rlVertex2f(bottomRight.X, bottomRight.Y)

	// Top-right
	if flipX {
		rlTexCoord2f(source.X/width, source.Y/height)
	} else {
		rlTexCoord2f((source.X+source.W)/width, source.Y/height)
	}
	rlVertex2f(topRight.X, topRight.Y)

	rlEnd()

	DisableTexture()
}

// These are all the characters allowed by Minecraft.
func IsRuneAllowed(r rune) bool {
	return r >= 0 && r <= unicode.MaxLatin1
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
	rlPushMatrix()
	defer rlPopMatrix()

	// Move to pivot, rotate, then move back to local text space.
	rlTranslatef(pivot.X, pivot.Y, 0)
	rlRotatef(rotation, 0, 0, 1)
	rlTranslatef(-textSize.X*0.5, -textSize.Y*0.5, 0)

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

	DisableTexture()

	rlBegin(rlQUADS)

	rlColor4ub(color.R, color.G, color.B, color.A)
	rlNormal3f(0, 0, 1)

	rlVertex2f(topLeft.X, topLeft.Y)
	rlVertex2f(bottomLeft.X, bottomLeft.Y)
	rlVertex2f(bottomRight.X, bottomRight.Y)
	rlVertex2f(topRight.X, topRight.Y)

	rlEnd()
}

/* RLGL IMPORTS*/
//so:extern
func rlViewport(x int32, y int32, width int32, height int32)

//so:extern
func rlMatrixMode(int)

//so:extern RL_MODELVIEW
const rlMODELVIEW = 0x1700

//so:extern RL_PROJECTION
const rlPROJECTION = 0x1701

//so:extern RL_TEXTURE
const rlTEXTURE = 0x1702

//so:extern
func rlLoadIdentity()

//so:extern
func rlOrtho(left float64, right float64, bottom float64, top float64, near_val float64, far_val float64)

//so:extern
func rlglInit(int, int)

//so:extern
func rlLoadExtensions(any)

//so:extern
func rlSetTexture(int)

//so:extern RL_LINES
const rlLINES = 0x0001

//so:extern RL_TRIANGLES
const rlTRIANGLES = 0x0004

//so:extern RL_QUADS
const rlQUADS = 0x0007

//so:extern
func rlBegin(int)

//so:extern
func rlEnd()

//so:extern
func rlColor4ub(red uint8, green uint8, blue uint8, alpha uint8)

//so:extern
func rlNormal3f(nx float32, ny float32, nz float32)

//so:extern
func rlTranslatef(nx float32, ny float32, nz float32)

//so:extern
func rlRotatef(nx float32, ny float32, nz float32, z float32)

//so:extern
func rlTexCoord2f(s float32, t float32)

//so:extern
func rlVertex2f(x float32, y float32)

//so:extern rlClearColor
func rlClearColor(red float32, green float32, blue float32, alpha float32)

//so:extern rlClear
func rlClear(int)

//so:extern
func rlClearScreenBuffers()

//so:extern rlDisableDepthTest
func rlDisableDepthTest()

//so:extern rlPopMatrix
func rlPopMatrix() {}

//so:extern rlMultMatrixf
func rlMultMatrixf(m *float32)

//so:extern
func rlDrawRenderBatchActive()

func rlLoadTexture(data any, width, height, format, mipmaps int) int

// so:extern RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
const rlPIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 7

//so:extern
func rlUnloadTexture(int)

//so:extern
func rlPushMatrix()

//so:extern
func rlFrustum(left float64, right float64, bottom float64, top float64, near_val float64, far_val float64)

//so:extern
func rlEnableDepthTest()
