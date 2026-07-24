package gfx

import "solod.dev/so/math"

var CameraCullDistanceNear = 0.05
var CameraCullDistanceFar = 4000.0

// convert fov in degrees to radians and get vertical and horizontal fov
func (c *Camera) CalculateFOV(degFOV float32) {
	w, h := GetWindowSize()
	aspect := float32(w) / float32(h)
	fovyRad := degFOV * Deg2rad
	fovxRad := 2 * math.Atan(math.Tan(float64(fovyRad/2))*float64(aspect))
	c.FovyRad = fovyRad
	c.FovxRad = float32(fovxRad)
}
func (c *Camera) CalculateLookVector() Vector3 {
	dir := Vector3{
		X: float32(math.Cos(float64(c.Pitch)) * math.Sin(float64(c.Yaw))),
		Y: float32(math.Sin(float64(c.Pitch))),
		Z: float32(math.Cos(float64(c.Pitch)) * math.Cos(float64(c.Yaw))),
	}
	return dir
}

// pos is new camera position. pitch,yaw are new pitch and new yaw in radians
func (c *Camera) Update(pos Vector3, yaw, pitch float32) {
	c.CalculateFOV(c.FOV)
	c.Yaw = yaw
	c.Pitch = pitch

	const pitchLimit = Pi/2 - 0.01
	if c.Pitch > pitchLimit {
		c.Pitch = pitchLimit
	}
	if c.Pitch < -pitchLimit {
		c.Pitch = -pitchLimit
	}
	c.LookVector = c.CalculateLookVector()


	c.Position = pos
	c.Target = pos.Add(c.LookVector)
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
