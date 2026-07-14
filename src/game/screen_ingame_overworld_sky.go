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

		// starSize := 0.25 + rand.Float32()*0.25 //original
		var starSize float32 = 0.15 + rand.Float32()*0.15/2
		lengthSquared := dir.LengthSqr()
		if lengthSquared >= 1.0 || lengthSquared <= 0.025 {
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
			)
			mesh.QuadEndVertex(true, false, false)
		}
	}
	mesh.Upload(true)
	return mesh
}

// // notch code (or jeb idfk)
// func (state *ScreenInGameState) DrawSkyFan(celestialAngle float32, sunsetColor gfx.Color) {
// 	celestialAngleRadians := celestialAngle * gfx.Pi * 2

// 	gfx.PushMatrix()
// 	gfx.Translatef(state.Cam.Position.X, state.Cam.Position.Y, state.Cam.Position.Z)
// 	gfx.Rotatef(90, 1, 0, 0)

// 	if math.Sin(float64(celestialAngleRadians)) < 0 {
// 		gfx.Rotatef(180, 0, 0, 1)
// 	} else {
// 		gfx.Rotatef(0, 0, 0, 1)
// 	}
// 	gfx.Begin(gfx.RL_TRIANGLES)
// 	const fanSteps = 16
// 	for step := range fanSteps {
// 		angle1 := float32(step) * gfx.Pi * 2.0 / float32(fanSteps)
// 		sin1, cos1 := gfx.Sincos(angle1)

// 		angle2 := float32(step+1) * gfx.Pi * 2.0 / float32(fanSteps)
// 		sin2, cos2 := gfx.Sincos(angle2)

// 		// center of fan
// 		gfx.Color4ub(sunsetColor.R, sunsetColor.G, sunsetColor.B, sunsetColor.A)
// 		gfx.Vertex3f(0, 100, 0)

// 		// current edge
// 		gfx.Color4ub(sunsetColor.R, sunsetColor.G, sunsetColor.B, 0) //fade
// 		gfx.Vertex3f(
// 			sin1*120,
// 			cos1*120,
// 			-cos1*40*(float32(sunsetColor.A)/255),
// 		)
// 		// Next edge (Faded to 0 alpha)
// 		gfx.Color4ub(sunsetColor.R, sunsetColor.G, sunsetColor.B, 0) //fade
// 		gfx.Vertex3f(
// 			sin2*120,
// 			cos2*120,
// 			-cos2*40*(float32(sunsetColor.A)/255),
// 		)
// 	}
// 	gfx.End()
// 	gfx.PopMatrix()
// 	gfx.DrawRenderBatchActive()
// }

// from notch code
func (state *ScreenInGameState) CalculateCelestialAngle(gameTime float32) float32 {
	dayTime := float32(math.Mod(float64(gameTime), 24000))

	angle := dayTime/24000 - 0.25

	if angle < 0 {
		angle += 1
	}
	if angle > 1 {
		angle -= 1
	}

	linearAngle := angle

	angle = 1 - (float32(math.Cos(float64(angle)*math.Pi))+1)/2
	angle = linearAngle + (angle-linearAngle)/3

	return angle
}

func (state *ScreenInGameState) DrawSky3D() {
	var BaseSkyColor = gfx.NewColor(120, 167, 255, 255)
	celestialAngle := state.CalculateCelestialAngle(state.GameTimeFloat)
	daylight := state.CalculateDaylight(celestialAngle)
	skyColor := state.CalculateSkyColor(celestialAngle, daylight, BaseSkyColor)
	gfx.ClearBackground(skyColor)
	
	// bottom plane.
	// gfx.DrawPlane(state.Cam.Position.Add(gfx.NewVector3(0, -16, 0)),
	// 	gfx.NewVector2(768, 768), skyColor)

	starBrightness := state.CalculateStarBrightness(celestialAngle)
	if starBrightness > 0 {
		starColor := gfx.NewColor4f(gfx.NewVector4(starBrightness, starBrightness, starBrightness, starBrightness))
		state.Stars.Draw(gfx.DefaultTexture(), starColor, gfx.MatrixTranslate(
			state.Cam.Position.X,
			state.Cam.Position.Y,
			state.Cam.Position.Z,
		))
	}

	// TODO: draw base plane (glSkyList)

	// draw sunset
	// sunsetColors := state.CalculateSunriseSunsetColors(celestialAngle)
	// if sunsetColors.A > 0 {
	// 	state.DrawSkyFan(celestialAngle, sunsetColors)
	// }
	// draw sun, moon, stars

}

// CalculateDaylight from celestialAngle. taken from Notch code.
func (state *ScreenInGameState) CalculateDaylight(celestialAngle float32) float32 {
	daylight := math.Cos(float64(celestialAngle)*gfx.Pi*2)*2 + 0.5
	return float32(min(max(daylight, 0), 1))
}

func (state *ScreenInGameState) CalculateSkyColor(celestialAngle, daylight float32, BaseSkyColor gfx.Color) gfx.Color {
	skyColor := gfx.NewColor3f(BaseSkyColor.RGB().Scale(daylight))
	return skyColor
	//TODO
	/*

	   if (rainStrength > 0.0F) {
	       float grayscale = (red * 0.30F + green * 0.59F + blue * 0.11F) * 0.60F;
	       float rainBlend = 1.0F - rainStrength * 0.75F;

	       red   = red   * rainBlend + grayscale * (1.0F - rainBlend);
	       green = green * rainBlend + grayscale * (1.0F - rainBlend);
	       blue  = blue  * rainBlend + grayscale * (1.0F - rainBlend);
	   }

	   float thunderStrength = this.getWeightedThunderStrength(partialTick);

	   if (thunderStrength > 0.0F) {
	       float darkGray = (red * 0.30F + green * 0.59F + blue * 0.11F) * 0.20F;
	       float thunderBlend = 1.0F - thunderStrength * 0.75F;

	       red   = red   * thunderBlend + darkGray * (1.0F - thunderBlend);
	       green = green * thunderBlend + darkGray * (1.0F - thunderBlend);
	       blue  = blue  * thunderBlend + darkGray * (1.0F - thunderBlend);
	   }

	   if (this.field_27172_i > 0) {
	       float lightning = (float)this.field_27172_i - partialTick;

	       if (lightning > 1.0F) {
	           lightning = 1.0F;
	       }

	       lightning *= 0.45F;

	       red   = red   * (1.0F - lightning) + 0.8F * lightning;
	       green = green * (1.0F - lightning) + 0.8F * lightning;
	       blue  = blue  * (1.0F - lightning) + 1.0F * lightning;
	   }
	*/
}

// Notch code
func (state *ScreenInGameState) CalculateSunriseSunsetColors(celestialAngle float32) gfx.Color {
	horizonWindow := float32(0.4)

	celestialAngleRadians := float64(celestialAngle * gfx.Pi * 2)
	sunHorizonProximity := float32(math.Cos(celestialAngleRadians))

	if sunHorizonProximity >= -horizonWindow && sunHorizonProximity <= horizonWindow {
		progressFactor := sunHorizonProximity/horizonWindow*0.5 + 0.5
		alphaFade := float32(
			1.0 - (1.0-math.Sin(float64(progressFactor*gfx.Pi)))*0.99,
		)
		alphaFade *= alphaFade

		return gfx.NewColor4f(gfx.NewVector4(
			progressFactor*0.3+0.7,
			progressFactor*progressFactor*0.7+0.2,
			0.2,
			alphaFade,
		))
	}

	return gfx.NewColor(0, 0, 0, 0)
}
func (state *ScreenInGameState) CalculateStarBrightness(celestialAngle float32) float32 {
	starBrightness := 1 -
		(float32(math.Cos(float64(celestialAngle)*gfx.Pi*2))*2 + 12.0/16)

	starBrightness = max(min(1, starBrightness), 0)

	return starBrightness * starBrightness * 0.5
}
