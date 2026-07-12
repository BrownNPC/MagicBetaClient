package game

import (
	"mbc/gfx"

	"solod.dev/so/mem"

	"solod.dev/so/math"
	"solod.dev/so/math/rand"
)

func (state *ScreenInGameState) GenMeshStars(a mem.Allocator) gfx.Mesh {
	const starAttempts = 1500
	var mesh = gfx.NewMesh(a)
	for range starAttempts {
		dir := gfx.Vector3{
			X: rand.Float32()*2 - 1,
			Y: rand.Float32()*2 - 1,
			Z: rand.Float32()*2 - 1,
		}

		starSize := 0.25 + rand.Float32()*0.25

		lengthSquared := dir.LengthSqr()
		if lengthSquared >= 1.0 || lengthSquared <= 0.01 {
			continue
		}

		dir = dir.Normalize()
		star := dir.Scale(100)

		// Orient the quad to face outward from the sphere.
		yaw := float32(math.Atan2(float64(dir.X), float64(dir.Z)))
		yawSin, yawCos := gfx.Sincos(yaw)

		pitch := float32(math.Atan2(
			math.Sqrt(float64(dir.X*dir.X+dir.Z*dir.Z)),
			float64(dir.Y),
		))
		pitchSin, pitchCos := gfx.Sincos(pitch)

		// Random rotation around the quad's normal.
		roll := rand.Float32() * math.Pi * 2
		rollSin, rollCos := gfx.Sincos(roll)
		for corner := range 4 {
			localX := float32((corner&2)-1) * starSize
			localZ := float32((((corner + 1) & 2) - 1)) * starSize

			// Roll
			rolledX := localX*rollCos - localZ*rollSin
			rolledZ := localZ*rollCos + localX*rollSin

			// Pitch
			pitchedY := rolledX * pitchSin
			pitchedX := -rolledX * pitchCos

			// Yaw
			offsetX := pitchedX*yawSin - rolledZ*yawCos
			offsetZ := rolledZ*yawSin + pitchedX*yawCos

			mesh.QuadVertex3f(
				star.X+offsetX,
				star.Y+pitchedY,
				star.Z+offsetZ,
				255, 255, 255, 255,
			)
		}
	}
	mesh.Upload(true)
	return mesh
}
