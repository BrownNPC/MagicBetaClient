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
		// yaw,pitch is star position in a sphere. roll is star rotation
		dir := gfx.NewVector3(
			rand.Float32()*2-1,
			rand.Float32()*2-1,
			rand.Float32()*2-1,
		)

		lengthSquared := dir.LengthSqr()

		if lengthSquared >= 1 || lengthSquared <= 0.025 {
			continue
		}

		starDirection := dir.Normalize()
		// Move in the direction of the star 100 units.
		// arbitrarily decide to place the star here.
		starWorldPos := starDirection.Scale(20)
		starSize := 0.025 + rand.Float32()*0.025/2

		up := gfx.NewVector3(0, 1, 0)
		if math.Abs(float64(starDirection.DotProduct(up))) > 0.99 {
			up = gfx.NewVector3(1, 0, 0)
		}
		normal := starWorldPos.Normalize().Negate()
		right := up.CrossProduct(normal).Normalize()
		forward := normal.CrossProduct(right).Normalize()
		forward = forward.Negate() // flip the quad so it faces origin

		// apply random roation to the star quad
		roll := rand.Float32() * 2 * math.Pi
		c := float32(math.Cos(float64(roll)))
		s := float32(math.Sin(float64(roll)))

		rolledRight := right.Scale(c).Add(forward.Scale(s))
		rolledForward := forward.Scale(c).Subtract(right.Scale(s))
		corners := []gfx.Vector3{
			rolledRight.Scale(-starSize).Add(rolledForward.Scale(-starSize)),
			rolledRight.Scale(-starSize).Add(rolledForward.Scale(starSize)),
			rolledRight.Scale(starSize).Add(rolledForward.Scale(starSize)),
			rolledRight.Scale(starSize).Add(rolledForward.Scale(-starSize)),
		}

		for _, offset := range corners {
			vert := starWorldPos.Add(offset)
			mesh.QuadVertex3f(vert.X, vert.Y, vert.Z)
			mesh.QuadEndVertex(true, false, false)
		}
	}
	return mesh.Upload(false)
}

func (state *ScreenInGameState) CalculateHorizonFanModelMatrix(cam gfx.Camera, celestialAngle float32, sunsetColor gfx.Color) gfx.Matrix {
	mat := gfx.MatrixIdentity()
	mat = mat.Multiply(gfx.MatrixRotateX(90 * gfx.Deg2rad))
	// if gfx.SinT(celestialAngle*gfx.Tau) < 0 {
	// 	mat = mat.Multiply(gfx.MatrixRotateZ(180 * gfx.Deg2rad))
	// }
	mat = mat.Multiply(gfx.MatrixRotateZ(90 * gfx.Deg2rad))

	mat = mat.Multiply(gfx.MatrixRotateX(state.acc * 360 * gfx.Deg2rad))
	mat = mat.Multiply(gfx.MatrixTranslate(cam.Position.X, cam.Position.Y, cam.Position.Z))
	mat = mat.Multiply(gfx.MatrixScale(1, 1, float32(sunsetColor.A)/255))
	return mat
}
func (state *ScreenInGameState) GenMeshHorizonFan(a mem.Allocator) gfx.Mesh {
	var m = gfx.NewMesh(a)
	const fanSteps = 16
	for step := range fanSteps {
		currentEdge := float32(step) / fanSteps
		sin, cos := gfx.SinCosT(currentEdge)

		// top of the fan is full white
		m.Color4ub(255, 255, 255, 255) // full white top
		m.Vertex3f(0, 100, 0)

		// edges fade to 0 opacity
		m.Color4ub(255, 255, 255, 0)
		m.Vertex3f(sin*120, cos*120, -cos*40)

		// next edge
		nextEdge := float32(step+1) / fanSteps
		sin, cos = gfx.SinCosT(nextEdge)
		m.Color4ub(255, 255, 255, 0)
		m.Vertex3f(sin*120, cos*120, -cos*40)
	}
	return m.Upload(false)
}

// from notch code
// Returns Celestial Angle in turns.
func (state *ScreenInGameState) CalculateCelestialAngle(gameTime float32) float32 {
	angle := float32(int(gameTime)%24000)/24000 - 0.25

	if angle < 0 {
		angle++
	}

	linear := angle
	angle = 1 - (gfx.CosT(angle*0.5)+1)*0.5
	return linear + (angle-linear)/3
}
func (state *ScreenInGameState) DrawSky3D(cam gfx.Camera) {
	state.acc += 0.001
	if state.acc > 1 {
		state.acc = 0
	}
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
	sunPosition := gfx.NewVector3(
		0,
		sunDistance*float32(gfx.CosT(celestialAngle)),
		sunDistance*float32(gfx.SinT(celestialAngle)),
	)
	// draw glare (horizon)
	sunsetColor := state.CalculateSunriseSunsetColors(celestialAngle)
	if sunsetColor.A > 0 {
		state.HorizonMesh.Draw(gfx.DefaultTexture(),
			sunsetColor,
			state.CalculateHorizonFanModelMatrix(cam, celestialAngle, sunsetColor),
		)
	}

	sunTexture := state.s.Pack.GetTexture(assets.Terrain_sun)
	gfx.BeginBlendMode(gfx.BLEND_ADD_COLORS)
	state.SunMesh.Draw(sunTexture, gfx.White,
		gfx.CalculateModelMatrix(
			cam.Position.Add(sunPosition),
			gfx.NewVector3(1, 0, 0), // Rotate around X to follow the arc
			celestialAngle*360,
			gfx.NewVector3(1, 1, 1),
		),
	)
	moonTexture := state.s.Pack.GetTexture(assets.Terrain_moon)
	state.SunMesh.Draw(moonTexture, gfx.White,
		gfx.CalculateModelMatrix(
			cam.Position.Add(sunPosition.Negate()),
			gfx.NewVector3(1, 0, 0), // Rotate around X to follow the arc
			360*celestialAngle+180,
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
