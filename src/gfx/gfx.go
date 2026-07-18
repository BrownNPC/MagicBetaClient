package gfx

import (
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/math"
	"solod.dev/so/mem"
	"solod.dev/so/slices"
	"solod.dev/so/unicode"
)

//so:include "rlgl-master.h"

// Some basic Defines
const (
	Pi      = 3.1415927
	Deg2rad = 0.017453292
	Rad2deg = 57.295776
	Tau     = 6.2831853
)
const tableSize = 4096

var sinTable [tableSize]float32

func init() {
	for i := range sinTable {
		sinTable[i] = float32(math.Sin(float64(i) * 2 * math.Pi / tableSize))
	}
}

// I read this article: https://www.computerenhance.com/p/turns-are-better-than-radians

func SinT(t float32) float32 {
	mask := tableSize - 1

	x := t * tableSize

	i := int(x)
	f := x - float32(i)

	i &= mask

	return sinTable[i] +
		(sinTable[(i+1)&mask]-sinTable[i])*f
}

// Cos function that accepts Turns.
func CosT(t float32) float32 {
	index := int(t*tableSize) + tableSize/4
	if index < 0 {
		index += tableSize
	}
	return sinTable[(index)%tableSize]
}
func SinCosT(t float32) (float32, float32) {
	return SinT(t), CosT(t)
}

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
func EndDrawing()   { DrawRenderBatchActive(); sdl.GLSwapWindow(Window) }

// Lock or unlock the mouse (FPS mode)
func SetMouseLock(v bool) {
	if sdl.GetWindowRelativeMouseMode(Window) == v {
		return
	}
	sdl.SetWindowRelativeMouseMode(Window, v)
}

func ClearBackground(c Color) {
	rlClearColor(c.R, c.G, c.B, c.A)
	rlClearScreenBuffers()
}
func BeginMode3D(cam Camera) {
	DrawRenderBatchActive()
	w, h := GetWindowSize()

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
func DrawCircle3D(center Vector3, radius float32, color Color) {
	rlPushMatrix()

	rlTranslatef(center.X, center.Y, center.Z)

	rlBegin(rlLINES)
	for i := float32(0); i < 360; i += 10 {
		rlColor4ub(color.R, color.G, color.B, color.A)

		sin, cos := Sincos(Deg2rad * i)
		rlVertex3f(sin*radius, cos*radius, 0)
		sin, cos = Sincos(Deg2rad * (i + 10))
		rlVertex3f(sin*radius, cos*radius, 0)
	}
	rlEnd()
	rlPopMatrix()
}
func DrawCube(position Vector3, width, height, length float32, color Color) {
	var x, y, z float32
	rlPushMatrix()
	{
		rlTranslatef(position.X, position.Y, position.Z)
		rlBegin(RL_TRIANGLES)
		{
			rlColor4ub(color.R, color.G, color.B, color.A)
			// Front face
			rlNormal3f(0.0, 0.0, 1.0)
			rlVertex3f(x-width/2, y-height/2, z+length/2) // Bottom Left
			rlVertex3f(x+width/2, y-height/2, z+length/2) // Bottom Right
			rlVertex3f(x-width/2, y+height/2, z+length/2) // Top Left

			rlVertex3f(x+width/2, y+height/2, z+length/2) // Top Right
			rlVertex3f(x-width/2, y+height/2, z+length/2) // Top Left
			rlVertex3f(x+width/2, y-height/2, z+length/2) // Bottom Right

			// Back face
			rlNormal3f(0.0, 0.0, -1.0)
			rlVertex3f(x-width/2, y-height/2, z-length/2) // Bottom Left
			rlVertex3f(x-width/2, y+height/2, z-length/2) // Top Left
			rlVertex3f(x+width/2, y-height/2, z-length/2) // Bottom Right

			rlVertex3f(x+width/2, y+height/2, z-length/2) // Top Right
			rlVertex3f(x+width/2, y-height/2, z-length/2) // Bottom Right
			rlVertex3f(x-width/2, y+height/2, z-length/2) // Top Left

			// Top face
			rlNormal3f(0.0, 1.0, 0.0)
			rlVertex3f(x-width/2, y+height/2, z-length/2) // Top Left
			rlVertex3f(x-width/2, y+height/2, z+length/2) // Bottom Left
			rlVertex3f(x+width/2, y+height/2, z+length/2) // Bottom Right

			rlVertex3f(x+width/2, y+height/2, z-length/2) // Top Right
			rlVertex3f(x-width/2, y+height/2, z-length/2) // Top Left
			rlVertex3f(x+width/2, y+height/2, z+length/2) // Bottom Right

			// Bottom face
			rlNormal3f(0.0, -1.0, 0.0)
			rlVertex3f(x-width/2, y-height/2, z-length/2) // Top Left
			rlVertex3f(x+width/2, y-height/2, z+length/2) // Bottom Right
			rlVertex3f(x-width/2, y-height/2, z+length/2) // Bottom Left

			rlVertex3f(x+width/2, y-height/2, z-length/2) // Top Right
			rlVertex3f(x+width/2, y-height/2, z+length/2) // Bottom Right
			rlVertex3f(x-width/2, y-height/2, z-length/2) // Top Left

			// Right face
			rlNormal3f(1.0, 0.0, 0.0)
			rlVertex3f(x+width/2, y-height/2, z-length/2) // Bottom Right
			rlVertex3f(x+width/2, y+height/2, z-length/2) // Top Right
			rlVertex3f(x+width/2, y+height/2, z+length/2) // Top Left

			rlVertex3f(x+width/2, y-height/2, z+length/2) // Bottom Left
			rlVertex3f(x+width/2, y-height/2, z-length/2) // Bottom Right
			rlVertex3f(x+width/2, y+height/2, z+length/2) // Top Left

			// Left face
			rlNormal3f(-1.0, 0.0, 0.0)
			rlVertex3f(x-width/2, y-height/2, z-length/2) // Bottom Right
			rlVertex3f(x-width/2, y+height/2, z+length/2) // Top Left
			rlVertex3f(x-width/2, y+height/2, z-length/2) // Top Right

			rlVertex3f(x-width/2, y-height/2, z+length/2) // Bottom Left
			rlVertex3f(x-width/2, y+height/2, z+length/2) // Top Left
			rlVertex3f(x-width/2, y-height/2, z-length/2) // Bottom Right
		}
		rlEnd()
	}
	rlPopMatrix()
}
func EndMode3D() {
	DrawRenderBatchActive()

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

func EndMode2D() { DrawRenderBatchActive(); rlLoadIdentity() }

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

	rlBegin(RL_QUADS)

	rlColor4ub(tint.R, tint.G, tint.B, tint.A)
	rlNormal3f(0, 0, 1)

	// Top-left
	rlTexCoord2f(0, 0)
	rlVertex2f(dest.X, dest.Y)

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

	rlBegin(RL_QUADS)

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

	// Pivot at center of the whole text block.
	pivot := position.Add(textSize.Half())
	rlPushMatrix()
	defer rlPopMatrix()

	// Move to pivot, rotate, then move back to local text space.
	rlTranslatef(pivot.X, pivot.Y, 0)
	rlRotatef(rotation, 0, 0, 1)
	rlTranslatef(-textSize.X*0.5, -textSize.Y*0.5, 0)

	textOffsetX := float32(0)
	textOffsetY := float32(0) // newlines
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
		if charCode == '\n' {
			textOffsetX = 0
			textOffsetY = +textSize.Y
			continue
		}
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
			Y: 0 + textOffsetY,
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

var texShapes = Texture{ID: 1, Width: 1, Height: 1}

// OpenGL version
const (
	RL_OPENGL_SOFTWARE = iota // Software rendering
	RL_OPENGL_11              // OpenGL 1.1
	RL_OPENGL_21              // OpenGL 2.1 (GLSL 120)
	RL_OPENGL_33              // OpenGL 3.3 (GLSL 330)
	RL_OPENGL_43              // OpenGL 4.3 (using GLSL 330)
	RL_OPENGL_ES_20           // OpenGL ES 2.0 (GLSL 100)
	RL_OPENGL_ES_30           // OpenGL ES 3.0 (GLSL 300 es)
)

//so:extern
func rlGetVersion() int

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

	rlBegin(RL_TRIANGLES)

	rlColor4ub(color.R, color.G, color.B, color.A)

	rlVertex2f(topLeft.X, topLeft.Y)
	rlVertex2f(bottomLeft.X, bottomLeft.Y)
	rlVertex2f(topRight.X, topRight.Y)

	rlVertex2f(topRight.X, topRight.Y)
	rlVertex2f(bottomLeft.X, bottomLeft.Y)
	rlVertex2f(bottomRight.X, bottomRight.Y)

	rlEnd()
}

type VertexCoord struct {
	X, Y, Z float32
}
type VertexTexcoord struct {
	X, Y float32
}
type VertexColor struct {
	R, G, B, A uint8
}

type Mesh struct {
	a  mem.Allocator
	sz int

	vertices  []VertexCoord
	texCoords []VertexTexcoord
	colors    []VertexColor
	vaoID     int
	vboID     [5]int

	// Quad tracking state
	quadVerts     [4]VertexCoord
	quadTexCoords [4]VertexTexcoord
	quadColors    [4]VertexColor
	quadCount     int
}

func (m *Mesh) VertexCount() int { return len(m.vertices) }

func (m *Mesh) Vertex3f(x, y, z float32) {
	m.vertices = slices.Append(m.a, m.vertices, VertexCoord{x, y, z})
}
func (m *Mesh) Color4ub(r, g, b, a uint8) {
	m.colors = slices.Append(m.a, m.colors, VertexColor{r, g, b, a})
}
func (m *Mesh) TexCoord2f(u, v float32) {
	m.texCoords = slices.Append(m.a, m.texCoords, VertexTexcoord{u, v})
}

// Stores the position for the current quad vertex.
func (m *Mesh) QuadVertex3f(x, y, z float32) {
	m.quadVerts[m.quadCount] = VertexCoord{x, y, z}
}

// Stores the texcoord for the current quad vertex.
func (m *Mesh) QuadTexCoord2f(u, v float32) {
	m.quadTexCoords[m.quadCount] = VertexTexcoord{u, v}
}

// Stores the color for the current quad vertex.
func (m *Mesh) QuadColor4ub(r, g, b, a uint8) {
	m.quadColors[m.quadCount] = VertexColor{r, g, b, a}
}

// Finishes the current quad vertex. After four vertices have been submitted,
// the quad is expanded into two triangles.
func (m *Mesh) QuadEndVertex(vertex, texcoord, color bool) {
	m.quadCount++

	if m.quadCount != 4 {
		return
	}

	const (
		v0 = 0
		v1 = 1
		v2 = 2
		v3 = 3
	)

	indices := [...]int{
		v0, v1, v2,
		v0, v2, v3,
	}

	for _, i := range indices {
		if vertex {
			m.Vertex3f(
				m.quadVerts[i].X,
				m.quadVerts[i].Y,
				m.quadVerts[i].Z,
			)
		}
		if texcoord {
			m.TexCoord2f(
				m.quadTexCoords[i].X,
				m.quadTexCoords[i].Y,
			)
		}
		if color {
			m.Color4ub(
				m.quadColors[i].R,
				m.quadColors[i].G,
				m.quadColors[i].B,
				m.quadColors[i].A,
			)
		}
	}

	m.quadCount = 0
}

// No-op if on opengl 1.1
func (m *Mesh) Upload(dynamic bool) {
	version := rlGetVersion()
	if version == RL_OPENGL_11 || version == RL_OPENGL_SOFTWARE {
		return
	}
	m.vboID = [5]int{} // reset

	// enable VAO
	m.vaoID = rlLoadVertexArray()
	rlEnableVertexArray(m.vaoID)

	if len(m.vertices) > 0 {
		m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION] =
			rlLoadVertexBuffer(&m.vertices[0], len(m.vertices)*3*4, dynamic)
		// Enable vertex attributes: position (shader-location = 0)
		rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION, 3, RL_FLOAT, false, 0, 0)
		rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION)
	}

	// Enable vertex attributes: texcoords (shader-location = 1)
	if len(m.texCoords) > 0 {
		m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD] = rlLoadVertexBuffer(&m.texCoords[0], len(m.texCoords)*2*4, dynamic)
		rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD, 2, RL_FLOAT, false, 0, 0)
		rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD)
	} else {
		var value [2]float32
		rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD, &value, RL_SHADER_ATTRIB_VEC2, 2)
		rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD)
	}
	c.Raw(`#ifndef SDL_PLATFORM_VITA`)
	var value = [3]float32{0, 0, 1}
	rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL, &value, RL_SHADER_ATTRIB_VEC3, 3)
	rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL)
	c.Raw(`#endif`)

	if len(m.colors) > 0 {
		// Enable vertex attribute: color (shader-location = 3)
		m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR] = rlLoadVertexBuffer(&m.colors[0], len(m.colors)*4, dynamic)
		rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR, 4, RL_UNSIGNED_BYTE, true, 0, 0)
		rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR)
	} else {
		var value = [4]float32{1, 1, 1, 1} //white
		rlSetVertexAttributeDefault(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR, &value, RL_SHADER_ATTRIB_VEC4, 4)
		rlDisableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR)
	}
	if m.vaoID > 0 {
		sdl.Log("VAO: [ID %i] Mesh uploaded successfully to VRAM (GPU)", m.vaoID)
	} else {
		sdl.Log("VBO: Mesh uploaded successfully to VRAM (GPU)")
	}
	sdl.Log("Vertices: %d", len(m.vertices))
	rlDisableVertexArray()
}
func (m *Mesh) Destroy() {
	mem.FreeSlice(m.a, m.vertices)
	mem.FreeSlice(m.a, m.texCoords)
	mem.FreeSlice(m.a, m.colors)
	m.Reset()
}

// Reset de-allocates ONLY GPU memory.
// CPU memory is not free'd.
// Allows Mesh to be reused.
func (m *Mesh) Reset() {
	m.vertices = m.vertices[:0]
	m.texCoords = m.texCoords[:0]
	m.colors = m.colors[:0]
	if m.vaoID > 0 {
		rlUnloadVertexArray(m.vaoID)
		m.vaoID = 0
	}

	for i := range m.vboID {
		if m.vboID[i] > 0 {
			rlUnloadVertexBuffer(m.vboID[i])
			m.vboID[i] = 0
		}
	}
	*m = Mesh{}
}
func DefaultTexture() Texture { return Texture{Width: 1, Height: 1, ID: rlGetTextureIdDefault()} }
func (m *Mesh) Draw(albedo Texture, tint Color, transform Matrix) {
	version := rlGetVersion()
	if version == RL_OPENGL_11 || version == RL_OPENGL_SOFTWARE {
		const (
			GL_VERTEX_ARRAY        = 0x8074
			GL_NORMAL_ARRAY        = 0x8075
			GL_COLOR_ARRAY         = 0x8076
			GL_TEXTURE_COORD_ARRAY = 0x8078
		)
		EnableTexture(albedo)
		defer DisableTexture()
		if len(m.vertices) > 0 {
			rlEnableStatePointer(GL_VERTEX_ARRAY, &m.vertices[0])
		}
		if len(m.texCoords) > 0 {
			rlEnableStatePointer(GL_TEXTURE_COORD_ARRAY, &m.texCoords[0])
		}
		if len(m.colors) > 0 {
			rlEnableStatePointer(GL_COLOR_ARRAY, &m.colors[0])
		}
		rlPushMatrix()
		{
			rlColor4ub(tint.R, tint.G, tint.B, tint.A)
			f := MatrixToFloat(transform)
			rlMultMatrixf(&f.V[0])
			rlDrawVertexArray(0, len(m.vertices))
		}
		rlPopMatrix()

		rlDisableStatePointer(GL_VERTEX_ARRAY)
		rlDisableStatePointer(GL_TEXTURE_COORD_ARRAY)
		rlDisableStatePointer(GL_NORMAL_ARRAY)
		rlDisableStatePointer(GL_COLOR_ARRAY)
		return
	}
	rlEnableShader(rlGetShaderIdDefault())
	matModel := MatrixIdentity()
	matView := rlGetMatrixModelview().Matrix()
	matModelView := MatrixIdentity()
	matProjection := rlGetMatrixProjection().Matrix()

	// albedo color (Diffuse in raylib)
	{
		var values = [4]float32{
			float32(tint.R) / 255,
			float32(tint.G) / 255,
			float32(tint.B) / 255,
			float32(tint.A) / 255,
		}
		rlSetUniform(getShaderLocDefault(RL_SHADER_LOC_COLOR_DIFFUSE), &values[0], RL_SHADER_UNIFORM_VEC4, 1)
	}

	// Upload view and projection matrices (if locations available)
	if getShaderLocDefault(RL_SHADER_LOC_MATRIX_VIEW) != -1 {
		rlSetUniformMatrix(RL_SHADER_LOC_MATRIX_VIEW, matView.toRlMatrix())
	}
	if getShaderLocDefault(RL_SHADER_LOC_MATRIX_PROJECTION) != -1 {
		rlSetUniformMatrix(RL_SHADER_LOC_MATRIX_PROJECTION, matProjection.toRlMatrix())
	}

	// Accumulate several model transformations:
	//    transform: model transformation provided (includes DrawModel() params combined with model.transform)
	//    rlGetMatrixTransform(): rlgl internal transform matrix due to push/pop matrix stack
	matModel = MatrixMultiply(transform, rlGetMatrixTransform().Matrix())

	if getShaderLocDefault(RL_SHADER_LOC_MATRIX_MODEL) != -1 {
		rlSetUniformMatrix(RL_SHADER_LOC_MATRIX_MODEL, matModel.toRlMatrix())
	}

	// Get model-view matrix
	matModelView = MatrixMultiply(matModel, matView)
	// MVP matrix
	matModelViewProjection := MatrixIdentity()
	matModelViewProjection = MatrixMultiply(matModelView, matProjection)
	rlSetUniformMatrix(getShaderLocDefault(RL_SHADER_LOC_MATRIX_MVP), matModelViewProjection.toRlMatrix())

	// setup albedo/diffuse texture
	rlActiveTextureSlot(0)
	rlEnableTexture(albedo.ID)

	slot := int32(0)
	rlSetUniform(getShaderLocDefault(RL_SHADER_LOC_MAP_ALBEDO), &slot, RL_SHADER_UNIFORM_INT, 1)

	if m.vaoID > 0 {
		rlEnableVertexArray(m.vaoID)
	} else {
		// for opengl ES 2.0
		if len(m.vertices) > 0 {
			rlEnableVertexBuffer(m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION])
			rlSetVertexAttribute(
				RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION,
				3,
				RL_FLOAT,
				false, 0, 0,
			)
			rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION)
		}
		if len(m.texCoords) > 0 {
			rlEnableVertexBuffer(m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD])
			rlSetVertexAttribute(
				RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD,
				2,
				RL_FLOAT,
				false,
				0,
				0,
			)
			rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD)
		}

		if len(m.colors) > 0 {
			rlEnableVertexBuffer(m.vboID[RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR])
			rlSetVertexAttribute(
				RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR,
				4,
				RL_UNSIGNED_BYTE,
				true,
				0,
				0,
			)
			rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR)
		}
	}
	rlDrawVertexArray(0, len(m.vertices))
	if m.vaoID > 0 {
		rlDisableVertexArray()
	} else {
		rlDisableVertexBuffer()
	}
	rlDisableShader()
}
func NewMesh(a mem.Allocator) *Mesh {
	var size = 1024
	var m Mesh
	m.a = a
	m.vertices = slices.MakeCap[VertexCoord](a, 0, size)
	m.texCoords = slices.MakeCap[VertexTexcoord](a, 0, size)
	m.colors = slices.MakeCap[VertexColor](a, 0, size)

	ptr := mem.Alloc[Mesh](mem.System)
	*ptr = m
	return ptr
}
func GenMeshPlane(a mem.Allocator, width, length float32, resX, resZ int) *Mesh {
	var mesh = NewMesh(a)

	if resX < 1 {
		resX = 1
	}
	if resZ < 1 {
		resZ = 1
	}

	halfWidth := width * 0.5
	halfLength := length * 0.5

	stepX := width / float32(resX)
	stepZ := length / float32(resZ)

	for z := 0; z < resZ; z++ {
		z0 := -halfLength + float32(z)*stepZ
		z1 := z0 + stepZ

		v0 := float32(z) / float32(resZ)
		v1 := float32(z+1) / float32(resZ)

		for x := 0; x < resX; x++ {
			x0 := -halfWidth + float32(x)*stepX
			x1 := x0 + stepX

			u0 := float32(x) / float32(resX)
			u1 := float32(x+1) / float32(resX)

			// Bottom-left
			mesh.QuadVertex3f(x0, 0, z0)
			mesh.QuadTexCoord2f(u0, v0)
			mesh.QuadEndVertex(true, true, false)

			// Bottom-right
			mesh.QuadVertex3f(x1, 0, z0)
			mesh.QuadTexCoord2f(u1, v0)
			mesh.QuadEndVertex(true, true, false)

			// Top-right
			mesh.QuadVertex3f(x1, 0, z1)
			mesh.QuadTexCoord2f(u1, v1)
			mesh.QuadEndVertex(true, true, false)

			// Top-left
			mesh.QuadVertex3f(x0, 0, z1)
			mesh.QuadTexCoord2f(u0, v1)
			mesh.QuadEndVertex(true, true, false)
		}
	}

	mesh.Upload(false)
	return mesh
}
func CalculateModelMatrix(position, rotationAxis Vector3, rotationAngle float32, scale Vector3) Matrix {
	// Calculate transformation matrix from function parameters
	// Get transform matrix (rotation -> scale -> translation)
	matScale := MatrixScale(scale.X, scale.Y, scale.Z)
	matRotation := MatrixRotate(rotationAxis, rotationAngle*Deg2rad)
	matTranslation := MatrixTranslate(position.X, position.Y, position.Z)

	matTransform := MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation)
	return matTransform
}
func DrawPlane(centerPos Vector3, size Vector2, color Color) {
	// NOTE: Plane is always created on XZ ground
	rlPushMatrix()
	rlTranslatef(centerPos.X, centerPos.Y, centerPos.Z)
	rlScalef(size.X, 1.0, size.Y)

	rlBegin(RL_QUADS)
	rlColor4ub(color.R, color.G, color.B, color.A)
	rlNormal3f(0.0, 1.0, 0.0)

	rlVertex3f(-0.5, 0.0, -0.5)
	rlVertex3f(-0.5, 0.0, 0.5)
	rlVertex3f(0.5, 0.0, 0.5)
	rlVertex3f(0.5, 0.0, -0.5)
	rlEnd()
	rlPopMatrix()
}

// Draw a billboard
func DrawBillboard(camera Camera, texture Texture, position Vector3, scale float32, tint Color) {
	source := Rectangle{0.0, 0.0, float32(texture.Width), float32(texture.Height)}

	DrawBillboardRec(camera, texture, source, position, Vector2{scale * float32(math.Abs(float64(source.W/source.H))), scale}, tint)
}

// Draw a billboard (part of a texture defined by a rectangle)
func DrawBillboardRec(camera Camera, texture Texture, source Rectangle, position Vector3, size Vector2, tint Color) {
	// NOTE: Billboard locked on axis-Y
	up := Vector3{0.0, 1.0, 0.0}
	DrawBillboardPro(camera, texture, source, position, up, size, Vector2Scale(size, 0.5), 0.0, tint)
}

// Draw a billboard with additional parameters
func DrawBillboardPro(camera Camera, texture Texture, source Rectangle, position, up Vector3, size, origin Vector2, rotation float32, tint Color) {
	// Compute the up vector and the right vector
	matView := MatrixLookAt(camera.Position, camera.Target, camera.Up)
	right := Vector3{matView.M0, matView.M4, matView.M8}
	right = Vector3Scale(right, size.X)
	up = Vector3Scale(up, size.Y)

	// Flip the content of the billboard while maintaining the counterclockwise edge rendering order
	if size.X < 0.0 {
		source.X -= size.X
		source.W *= -1.0
		right = Vector3Negate(right)
		origin.X *= -1.0
	}
	if size.Y < 0.0 {
		source.Y -= size.Y
		source.H *= -1.0
		up = Vector3Negate(up)
		origin.Y *= -1.0
	}

	// Draw the texture region described by source on the following rectangle in 3D space:
	//
	//                size.X          <--.
	//  3 ^---------------------------+ 2 \ rotation
	//    |                           |   /
	//    |                           |
	//    |   origin.X   position     |
	// up |..............             | size.y
	//    |             .             |
	//    |             . origin.y    |
	//    |             .             |
	//  0 +---------------------------> 1
	//                right
	var forward Vector3
	if rotation != 0.0 {
		forward = Vector3CrossProduct(right, up)
	}

	origin3D := Vector3Add(Vector3Scale(Vector3Normalize(right), origin.X), Vector3Scale(Vector3Normalize(up), origin.Y))

	var points [4]Vector3
	points[0] = Vector3Zero()
	points[1] = right
	points[2] = Vector3Add(up, right)
	points[3] = up

	for i := range 4 {
		points[i] = Vector3Subtract(points[i], origin3D)
		if rotation != 0.0 {
			points[i] = Vector3RotateByAxisAngle(points[i], forward, rotation*Deg2rad)
		}
		points[i] = Vector3Add(points[i], position)
	}

	var texcoords [4]Vector2
	w, h := float32(texture.Width), float32(texture.Height)
	texcoords[0] = Vector2{(source.X / w), (source.Y + source.H) / h}
	texcoords[1] = Vector2{(source.X + source.W) / w, (source.Y + source.H) / h}
	texcoords[2] = Vector2{(source.X + source.W) / w, (source.Y / h)}
	texcoords[3] = Vector2{(source.X / w), (source.Y / h)}

	rlSetTexture(texture.ID)
	rlBegin(RL_QUADS)

	rlColor4ub(tint.R, tint.G, tint.B, tint.A)
	for i := range 4 {
		rlTexCoord2f(texcoords[i].X, texcoords[i].Y)
		rlVertex3f(points[i].X, points[i].Y, points[i].Z)
	}

	rlEnd()
	rlSetTexture(0)
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
const RL_TRIANGLES = 0x0004

//so:extern RL_QUADS
const RL_QUADS = 0x0007

//so:extern rlBegin
func rlBegin(int)

//so:extern rlEnd
func rlEnd()

//so:extern rlColor4ub
func rlColor4ub(red uint8, green uint8, blue uint8, alpha uint8)

//so:extern
func rlNormal3f(nx float32, ny float32, nz float32)

//so:extern rlTranslatef
func rlTranslatef(nx float32, ny float32, nz float32)

//so:extern rlRotatef
func rlRotatef(nx float32, ny float32, nz float32, z float32)

//so:extern rlTexCoord2f
func rlTexCoord2f(s float32, t float32)

//so:extern
func rlVertex2f(x float32, y float32)

//so:extern rlVertex3f
func rlVertex3f(x float32, y float32, z float32)

//so:extern rlScalef
func rlScalef(x float32, y float32, z float32)

//so:extern rlClearColor
func rlClearColor(red, green, blue, alpha uint8)

//so:extern rlClear
func rlClear(int)

//so:extern
func rlClearScreenBuffers()

//so:extern rlDisableDepthTest
func rlDisableDepthTest()

//so:extern
func rlDisableBackfaceCulling()

//so:extern
func rlEnableBackfaceCulling()

//so:extern rlPopMatrix
func rlPopMatrix() {}

//so:extern rlMultMatrixf
func rlMultMatrixf(m *float32)

//so:extern rlDrawRenderBatchActive
func DrawRenderBatchActive()

func rlLoadTexture(data any, width, height, format, mipmaps int) int

//so:extern RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
const rlPIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 7

//so:extern
func rlUnloadTexture(int)

//so:extern rlPushMatrix
func rlPushMatrix()

//so:extern
func rlFrustum(left float64, right float64, bottom float64, top float64, near_val float64, far_val float64)

//so:extern
func rlEnableDepthTest()

// attributes buffer
//
//so:extern
func rlLoadVertexBuffer(vertices any, size int, dynamic bool) int

//so:extern
func rlLoadVertexArray() int

//so:extern
func rlEnableVertexArray(int)

//so:extern
func rlDisableVertexArray()

//so:extern
func rlSetVertexAttribute(index, compSize, typ int, normalized bool, stride, offset int)

//so:extern
func rlSetVertexAttributeDefault(locIndex int, value any, attribType, count int)

//so:extern
func rlEnableVertexAttribute(int)

//so:extern
func rlDrawVertexArray(offset, count int)

//so:extern
func rlDisableVertexAttribute(int)

//so:extern
func rlEnableStatePointer(vertexAttribType int, buffer any)

//so:extern
func rlDisableStatePointer(int)

//so:extern
func rlGetShaderIdDefault() int

//so:extern Matrix
type rlMatrix struct{}

func (m rlMatrix) Matrix() Matrix     { return *any(&m).(*Matrix) }
func (m Matrix) toRlMatrix() rlMatrix { return *any(&m).(*rlMatrix) }

//so:extern
func rlGetMatrixModelview() rlMatrix

//so:extern
func rlGetMatrixProjection() rlMatrix

//so:extern
func rlGetMatrixTransform() rlMatrix

//so:extern
func rlEnableShader(int)

//so:extern
func rlDisableShader()

const (
	RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION = 0
	RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD = 1
	RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL   = 2
	RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR    = 3
)
const (
	RL_UNSIGNED_BYTE = 0x1401
	RL_FLOAT         = 0x1406
)
const (
	RL_SHADER_ATTRIB_FLOAT = iota // Shader attribute type: float
	RL_SHADER_ATTRIB_VEC2         // Shader attribute type: vec2 (2 float)
	RL_SHADER_ATTRIB_VEC3         // Shader attribute type: vec3 (3 float)
	RL_SHADER_ATTRIB_VEC4         // Shader attribute type: vec4 (4 float)
)

// Shader location point type
const (
	RL_SHADER_LOC_VERTEX_POSITION   = iota // Shader location: vertex attribute: position
	RL_SHADER_LOC_VERTEX_TEXCOORD01        // Shader location: vertex attribute: texcoord01
	RL_SHADER_LOC_VERTEX_TEXCOORD02        // Shader location: vertex attribute: texcoord02
	RL_SHADER_LOC_VERTEX_NORMAL            // Shader location: vertex attribute: normal
	RL_SHADER_LOC_VERTEX_TANGENT           // Shader location: vertex attribute: tangent
	RL_SHADER_LOC_VERTEX_COLOR             // Shader location: vertex attribute: color
	RL_SHADER_LOC_MATRIX_MVP               // Shader location: matrix uniform: model-view-projection
	RL_SHADER_LOC_MATRIX_VIEW              // Shader location: matrix uniform: view (camera transform)
	RL_SHADER_LOC_MATRIX_PROJECTION        // Shader location: matrix uniform: projection
	RL_SHADER_LOC_MATRIX_MODEL             // Shader location: matrix uniform: model (transform)
	RL_SHADER_LOC_MATRIX_NORMAL            // Shader location: matrix uniform: normal
	RL_SHADER_LOC_VECTOR_VIEW              // Shader location: vector uniform: view
	RL_SHADER_LOC_COLOR_DIFFUSE            // Shader location: vector uniform: diffuse color
	RL_SHADER_LOC_COLOR_SPECULAR           // Shader location: vector uniform: specular color
	RL_SHADER_LOC_COLOR_AMBIENT            // Shader location: vector uniform: ambient color
	RL_SHADER_LOC_MAP_ALBEDO               // Shader location: sampler2d texture: albedo (same as: RL_SHADER_LOC_MAP_DIFFUSE)
	RL_SHADER_LOC_MAP_METALNESS            // Shader location: sampler2d texture: metalness (same as: RL_SHADER_LOC_MAP_SPECULAR)
	RL_SHADER_LOC_MAP_NORMAL               // Shader location: sampler2d texture: normal
	RL_SHADER_LOC_MAP_ROUGHNESS            // Shader location: sampler2d texture: roughness
	RL_SHADER_LOC_MAP_OCCLUSION            // Shader location: sampler2d texture: occlusion
	RL_SHADER_LOC_MAP_EMISSION             // Shader location: sampler2d texture: emission
	RL_SHADER_LOC_MAP_HEIGHT               // Shader location: sampler2d texture: height
	RL_SHADER_LOC_MAP_CUBEMAP              // Shader location: samplerCube texture: cubemap
	RL_SHADER_LOC_MAP_IRRADIANCE           // Shader location: samplerCube texture: irradiance
	RL_SHADER_LOC_MAP_PREFILTER            // Shader location: samplerCube texture: prefilter
	RL_SHADER_LOC_MAP_BRDF                 // Shader location: sampler2d texture: brdf
)
const (
	RL_SHADER_UNIFORM_FLOAT     = iota // Shader uniform type: float
	RL_SHADER_UNIFORM_VEC2             // Shader uniform type: vec2 (2 float)
	RL_SHADER_UNIFORM_VEC3             // Shader uniform type: vec3 (3 float)
	RL_SHADER_UNIFORM_VEC4             // Shader uniform type: vec4 (4 float)
	RL_SHADER_UNIFORM_INT              // Shader uniform type: int
	RL_SHADER_UNIFORM_IVEC2            // Shader uniform type: ivec2 (2 int)
	RL_SHADER_UNIFORM_IVEC3            // Shader uniform type: ivec3 (3 int)
	RL_SHADER_UNIFORM_IVEC4            // Shader uniform type: ivec4 (4 int)
	RL_SHADER_UNIFORM_UINT             // Shader uniform type: unsigned int
	RL_SHADER_UNIFORM_UIVEC2           // Shader uniform type: uivec2 (2 unsigned int)
	RL_SHADER_UNIFORM_UIVEC3           // Shader uniform type: uivec3 (3 unsigned int)
	RL_SHADER_UNIFORM_UIVEC4           // Shader uniform type: uivec4 (4 unsigned int)
	RL_SHADER_UNIFORM_SAMPLER2D        // Shader uniform type: sampler2d
)
const (
	BLEND_ALPHA             = iota // Blend textures considering alpha (default)
	BLEND_ADDITIVE                 // Blend textures adding colors
	BLEND_MULTIPLIED               // Blend textures multiplying colors
	BLEND_ADD_COLORS               // Blend textures adding colors (alternative)
	BLEND_SUBTRACT_COLORS          // Blend textures subtracting colors (alternative)
	BLEND_ALPHA_PREMULTIPLY        // Blend premultiplied textures considering alpha
	BLEND_CUSTOM                   // Blend textures using custom src/dst factors (use rlSetBlendFactors())
	BLEND_CUSTOM_SEPARATE          // Blend textures using custom src/dst factors (use rlSetBlendFactorsSeparate())
)

func BeginBlendMode(mode int) { rlSetBlendMode(mode) }
func EndBlendMode()           { rlSetBlendMode(BLEND_ALPHA) }

//so:extern rlGetShaderLocsDefault
func rlgsldf() *c.Int

func getShaderLocDefault(id int) int { return int(*c.PtrAt(rlgsldf(), id)) }

//so:extern
func rlSetUniformMatrix(locIndex int, mat rlMatrix)

//so:extern
func rlEnableVertexBuffer(int)

//so:extern
func rlDisableVertexBuffer()

//so:extern
func rlUnloadVertexBuffer(int)

//so:extern
func rlUnloadVertexArray(int)

//so:extern
func rlSetUniform(locIndex int, value any, uniformType any, count int)

//so:extern
func rlGetTextureIdDefault() int

//so:extern
func rlActiveTextureSlot(int)

//so:extern
func rlEnableTexture(int)

//so:extern
func rlDisableTexture()

//so:extern
func rlUpdateVertexBuffer(bufferId int, data any, dataSize, offset int)

//so:extern rlDisableDepthMask
func DisableDepthMask()

//so:extern rlEnableDepthMask
func EnableDepthMask()

//so:extern rlSetBlendMode
func rlSetBlendMode(int)
func PushMatrix()                { rlPushMatrix() }
func PopMatrix()                 { rlPopMatrix() }
func Translatef(x, y, z float32) { rlTranslatef(x, y, z) }
func Rotatef(x, y, z, w float32) { rlRotatef(x, y, z, w) }
func Begin(mode int)             { rlBegin(mode) }
func End()                       { rlEnd() }
func Color4ub(r, g, b, a uint8)  { rlColor4ub(r, g, b, a) }
func Vertex3f(x, y, z float32)   { rlVertex3f(x, y, z) }
