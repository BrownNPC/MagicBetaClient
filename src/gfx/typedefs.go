package gfx

// Vector2 type
type Vector2 struct {
	X float32
	Y float32
}

// NewVector2 - Returns new Vector2
func NewVector2(x, y float32) Vector2 {
	return Vector2{x, y}
}

// Vector3 type
type Vector3 struct {
	X float32
	Y float32
	Z float32
}

// NewVector3 - Returns new Vector3
func NewVector3(x, y, z float32) Vector3 {
	return Vector3{x, y, z}
}

// Vector4 type
type Vector4 struct {
	X float32
	Y float32
	Z float32
	W float32
}

// NewVector4 - Returns new Vector4
func NewVector4(x, y, z, w float32) Vector4 {
	return Vector4{x, y, z, w}
}

// Matrix type (OpenGL style 4x4 - right handed, column major)
type Matrix struct {
	M0, M4, M8, M12  float32
	M1, M5, M9, M13  float32
	M2, M6, M10, M14 float32
	M3, M7, M11, M15 float32
}

// NewMatrix - Returns new Matrix
func NewMatrix(m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15 float32) Matrix {
	return Matrix{m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15}
}

// Mat2 type (used for polygon shape rotation matrix)
type Mat2 struct {
	M00 float32
	M01 float32
	M10 float32
	M11 float32
}

// NewMat2 - Returns new Mat2
func NewMat2(m0, m1, m10, m11 float32) Mat2 {
	return Mat2{m0, m1, m10, m11}
}

// Quaternion, 4 components (Vector4 alias)
type Quaternion = Vector4

// NewQuaternion - Returns new Quaternion
func NewQuaternion(x, y, z, w float32) Quaternion {
	return Quaternion{x, y, z, w}
}

// Color type, RGBA (32bit)
// TODO remove later, keep type for now to not break code
type Color struct{
	R,G,B,A uint8
}

// NewColor - Returns new Color
func NewColor(r, g, b, a uint8) Color{
	return Color{r, g, b, a}
}

// Rectangle type
type Rectangle struct {
	X      float32
	Y      float32
	Width  float32
	Height float32
}

// NewRectangle - Returns new Rectangle
func NewRectangle(x, y, width, height float32) Rectangle {
	return Rectangle{x, y, width, height}
}

// ToInt32 converts rectangle to int32 variant
func (r *Rectangle) ToInt32() RectangleInt32 {
	rect := RectangleInt32{}
	rect.X = int32(r.X)
	rect.Y = int32(r.Y)
	rect.Width = int32(r.Width)
	rect.Height = int32(r.Height)

	return rect
}

// RectangleInt32 type
type RectangleInt32 struct {
	X      int32
	Y      int32
	Width  int32
	Height int32
}

// ToFloat32 converts rectangle to float32 variant
func (r *RectangleInt32) ToFloat32() Rectangle {
	rect := Rectangle{}
	rect.X = float32(r.X)
	rect.Y = float32(r.Y)
	rect.Width = float32(r.Width)
	rect.Height = float32(r.Height)

	return rect
}

// Camera type, defines a camera position/orientation in 3d space
type Camera struct {
	// Camera position
	Position Vector3
	// Camera target it looks-at
	Target Vector3
	// Camera up vector (rotation over its axis)
	Up Vector3
	// Camera field-of-view aperture in Y (degrees) in perspective, used as near plane height in world units in orthographic
	Fovy float32
}


// NewCamera3D - Returns new Camera3D
func NewCamera3D(pos, target, up Vector3, fovy float32) Camera {
	return Camera{pos, target, up, fovy}
}

// Camera2D type, defines position/orientation in 2d space
type Camera2D struct {
	// Camera offset (screen space offset from window origin)
	Offset Vector2
	// Camera target (world space target point that is mapped to screen space offset)
	Target Vector2
	// Camera rotation in degrees (pivots around target)
	Rotation float32
	// Camera zoom (scaling around target), must not be set to 0, set to 1.0f for no scale
	Zoom float32
}

type Texture struct{
	Width,Height int
	ID uint32
}
