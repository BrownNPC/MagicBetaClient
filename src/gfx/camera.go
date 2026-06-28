package gfx

var CameraCullDistanceNear = 0.05
var CameraCullDistanceFar = 4000.0

// GetForward - Returns the cameras forward vector (normalized)
func (camera *Camera) GetForward() Vector3 {
	return Vector3Normalize(Vector3Subtract(camera.Target, camera.Position))
}

// GetUp - Returns the cameras up vector (normalized)
// Note: The up vector might not be perpendicular to the forward vector
func (camera *Camera) GetUp() Vector3 {
	return Vector3Normalize(camera.Up)
}

// GetRight - Returns the cameras right vector (normalized)
func (camera *Camera) GetRight() Vector3 {
	forward := camera.GetForward()
	up := camera.GetUp()

	return Vector3CrossProduct(forward, up)
}

// MoveForward - Moves the camera in its forward direction
func (camera *Camera) MoveForward(distance float32, moveInWorldPlane bool) {
	forward := camera.GetForward()

	if moveInWorldPlane {
		// Project vector onto world plane
		forward.Y = float32(0)
		forward = Vector3Normalize(forward)
	}

	// Scale by distance
	forward = Vector3Scale(forward, distance)

	// Move position and target
	camera.Position = Vector3Add(camera.Position, forward)
	camera.Target = Vector3Add(camera.Target, forward)
}

// MoveUp - Moves the camera in its up direction
func (camera *Camera) MoveUp(distance float32) {
	up := camera.GetUp()

	// Scale by distance
	up = Vector3Scale(up, distance)

	// Move position and target
	camera.Position = Vector3Add(camera.Position, up)
	camera.Target = Vector3Add(camera.Target, up)
}

// MoveRight - Moves the camera target in its current right direction
func (camera *Camera) MoveRight(distance float32, moveInWorldPlane bool) {
	right := camera.GetRight()

	if moveInWorldPlane {
		// Project vector onto world plane
		right.Y = float32(0)
		right = Vector3Normalize(right)
	}

	// Scale by distance
	right = Vector3Scale(right, distance)

	// Move position and target
	camera.Position = Vector3Add(camera.Position, right)
	camera.Target = Vector3Add(camera.Target, right)
}

// MoveToTarget - Moves the camera position closer/farther to/from the camera target
func (camera *Camera) MoveToTarget(delta float32) {
	distance := Vector3Distance(camera.Position, camera.Target)

	// Apply delta
	distance = distance + delta

	// Distance must be greater than 0
	if distance <= float32(0) {
		distance = 0.001
	}

	// Set new distance by moving the position along the forward vector
	forward := camera.GetForward()
	camera.Position = Vector3Add(camera.Target, Vector3Scale(forward, -distance))
}

// Yaw - Rotates the camera around its up vector
// Yaw is "looking left and right"
// If rotateAroundTarget is false, the camera rotates around its position
// Note: angle must be provided in radians
func (camera *Camera) Yaw(angle float32, rotateAroundTarget bool) {
	// Rotation axis
	var up = camera.GetUp()

	// View vector
	var targetPosition = Vector3Subtract(camera.Target, camera.Position)

	// Rotate view vector around up axis
	targetPosition = Vector3RotateByAxisAngle(targetPosition, up, angle)

	if rotateAroundTarget {
		// Move position relative to target
		camera.Position = Vector3Subtract(camera.Target, targetPosition)
	} else {
		// Move target relative to position
		camera.Target = Vector3Add(camera.Position, targetPosition)
	}
}

// Pitch - Rotates the camera around its right vector, pitch is "looking up and down"
//   - lockView prevents camera overrotation (aka "somersaults")
//   - rotateAroundTarget defines if rotation is around target or around its position
//   - rotateUp rotates the up direction as well (typically only useful in CAMERA_FREE)
//
// NOTE: angle must be provided in radians
func (camera *Camera) Pitch(angle float32, lockView bool, rotateAroundTarget bool, rotateUp bool) {
	// Up direction
	var up = camera.GetUp()

	// View vector
	var targetPosition = Vector3Subtract(camera.Target, camera.Position)

	if lockView {
		// In these camera modes we clamp the Pitch angle
		// to allow only viewing straight up or down.

		// Clamp view up
		maxAngleUp := Vector3Angle(up, targetPosition)
		maxAngleUp = maxAngleUp - 0.001 // avoid numerical errors
		if angle > maxAngleUp {
			angle = maxAngleUp
		}

		// Clamp view down
		maxAngleDown := Vector3Angle(Vector3Negate(up), targetPosition)
		maxAngleDown = maxAngleDown * -1.0  // downwards angle is negative
		maxAngleDown = maxAngleDown + 0.001 // avoid numerical errors
		if angle < maxAngleDown {
			angle = maxAngleDown
		}
	}

	// Rotation axis
	var right = camera.GetRight()

	// Rotate view vector around right axis
	targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle)

	if rotateAroundTarget {
		// Move position relative to target
		camera.Position = Vector3Subtract(camera.Target, targetPosition)
	} else {
		// Move target relative to position
		camera.Target = Vector3Add(camera.Position, targetPosition)
	}

	if rotateUp {
		// Rotate up direction around right axis
		camera.Up = Vector3RotateByAxisAngle(camera.Up, right, angle)
	}
}

// Roll - Rotates the camera around its forward vector
// Roll is "turning your head sideways to the left or right"
// Note: angle must be provided in radians
func (camera *Camera) Roll(angle float32) {
	// Rotation axis
	var forward = camera.GetForward()

	// Rotate up direction around forward axis
	camera.Up = Vector3RotateByAxisAngle(camera.Up, forward, angle)
}

// ViewMatrix - Returns the camera view matrix
func (camera *Camera) ViewMatrix() Matrix {
	return MatrixLookAt(camera.Position, camera.Target, camera.Up)
}

// ProjectionMatrix - Returns the camera projection matrix
func (camera *Camera) ProjectionMatrix(aspect float32) Matrix {
	return MatrixPerspective(camera.Fovy*(Pi/180.0), aspect, 0.01, 1000.0)
}

// Update - Update camera movement, movement/rotation values should be provided by user
// Required values
// movement.X - Move forward/backward
// movement.Y - Move right/left
// movement.Z - Move up/down
// rotation.X - yaw
// rotation.Y - pitch
// rotation.Z - roll
// zoom - Move towards target
func (camera *Camera) Update(movement Vector3, rotation Vector3, zoom float32) {

	lockView := true
	rotateAroundTarget := false
	rotateUp := false
	moveInWorldPlane := true

	// Camera rotation
	camera.Pitch(-rotation.Y*(Pi/180.0), lockView, rotateAroundTarget, rotateUp)
	camera.Yaw(-rotation.X*(Pi/180.0), rotateAroundTarget)
	camera.Roll(rotation.Z * (Pi / 180.0))

	// Camera movement
	camera.MoveForward(movement.X, moveInWorldPlane)
	camera.MoveRight(movement.Y, moveInWorldPlane)
	camera.MoveUp(movement.Z)

	// Zoom target distance
	camera.MoveToTarget(zoom)
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
