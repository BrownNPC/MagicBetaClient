package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"

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
func (state *ScreenInGameState) DrawSky3D(cam gfx.Camera) {
	var BaseSkyColor = gfx.NewColor(120, 167, 255, 255)
	celestialAngle := state.CalculateCelestialAngle(state.GameTimeFloat)
	daylight := state.CalculateDaylight(celestialAngle)
	skyColor := state.CalculateSkyColor(celestialAngle, daylight, BaseSkyColor)
	gfx.ClearBackground(skyColor)
	gfx.DisableDepthMask()
	defer gfx.EnableDepthMask()

	starBrightness := state.CalculateStarBrightness(celestialAngle)
	if starBrightness > 0 {
		starColor := gfx.NewColor4f(gfx.NewVector4(starBrightness, starBrightness, starBrightness, starBrightness))
		state.Stars.Draw(gfx.DefaultTexture(), starColor, gfx.MatrixTranslate(
			state.Cam.Position.X,
			state.Cam.Position.Y,
			state.Cam.Position.Z,
		))
	}
	// draw sun and moon.
	const sunDistance = 100
	angleDeg := celestialAngle * 360
	sunPosition := gfx.NewVector3(
		0,
		sunDistance*float32(math.Cos(float64(angleDeg*math.Pi/180))),
		sunDistance*float32(math.Sin(float64(angleDeg*math.Pi/180))),
	)
	sunTexture := state.s.Pack.GetTexture(assets.Terrain_sun)
	gfx.BeginBlendMode(gfx.BLEND_ADDITIVE)
	state.SunMesh.Draw(sunTexture, gfx.White,
		gfx.CalculateModelMatrix(
			cam.Position.Add(sunPosition),
			gfx.NewVector3(1, 0, 0), // Rotate around X to follow the arc
			angleDeg,
			gfx.NewVector3(1, 1, 1),
		),
	)
	moonTexture := state.s.Pack.GetTexture(assets.Terrain_moon)
	state.SunMesh.Draw(moonTexture, gfx.White,
		gfx.CalculateModelMatrix(
			cam.Position.Add(sunPosition.Negate()),
			gfx.NewVector3(1, 0, 0), // Rotate around X to follow the arc
			angleDeg+180,
			gfx.NewVector3(1.5, 1.5, 1.5), // moon is bigger than sun
		),
	)
	gfx.EndBlendMode()
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
