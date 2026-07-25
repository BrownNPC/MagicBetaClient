package game

import (
	"mbc/gfx"
	"mbc/net/mc"

	"solod.dev/so/mem"
	"solod.dev/so/slices"
)

// port of Vintage Story culling code: https://github.com/tyronx/occlusionculling

// must be called on view distance changed
func (state *ScreenInGameState) GenerateShellVectors(viewDistance int) {

	const chunkSectionSize = 16
	radius := (viewDistance / chunkSectionSize) + 1
	state.s.Scratch.Reset()
	points := gfx.GetVoxelOctagonPoints(&state.s.Scratch, 0, 0, int32(radius))

	shellPositions := make(map[gfx.Vector3i]bool, 1000)
	for _, point := range points {
		for cy := int32(-128); cy <= 128; cy++ { // 128 build limit
			shellPositions[gfx.Vector3i{X: point.X, Y: cy, Z: point.Y}] = true
		}
	}
	for r := 0; r < radius; r++ {
		points = gfx.GetVoxelOctagonPoints(&state.s.Scratch, 0, 0, int32(r))
		for _, point := range points {
			// Overextend the shell positions on the vertical axis to prevent over-culling issues
			shellPositions[gfx.Vector3i{X: point.X, Y: -128, Z: point.Y}] = true
			shellPositions[gfx.Vector3i{X: point.X, Y: 128, Z: point.Y}] = true
		}
	}
	state.CubicShellPositions = state.CubicShellPositions[:0]
	state.CubicShellPositionsNormalized = state.CubicShellPositionsNormalized[:0]
	for pos := range shellPositions {
		state.CubicShellPositions = slices.Append(mem.System, state.CubicShellPositions, pos)
		state.CubicShellPositionsNormalized = slices.Append(mem.System,
			state.CubicShellPositionsNormalized,
			gfx.NewVector3(float32(pos.X), float32(pos.Y), float32(pos.Z)).
				Normalize())
	}
}
func (state *ScreenInGameState) CullInvisibleChunks() {
	
}

// AmbientOcclusion for each quad vertex
type AmbientOcclusion struct{ AO [4]int }

func (state *ScreenInGameState) FaceAO(chunk *Chunk, x, y, z int, dir mc.Direction) AmbientOcclusion {
	var result AmbientOcclusion
	var top, bottom, left, right, topLeft, topRight, bottomLeft, bottomRight bool
	switch dir {
	case mc.DIRECTION_Down, mc.DIRECTION_Up:
		if dir == mc.DIRECTION_Up {
			y += 1
		} else {
			y -= 1
		}
		top = !state.IsBlockA(chunk, x+0, y, z-1)
		bottom = !state.IsBlockA(chunk, x+0, y, z+1)
		left = !state.IsBlockA(chunk, x-1, y, z+0)
		right = !state.IsBlockA(chunk, x+1, y, z+0)

		topLeft = !state.IsBlockA(chunk, x-1, y, z-1)
		topRight = !state.IsBlockA(chunk, x+1, y, z-1)

		bottomLeft = !state.IsBlockA(chunk, x-1, y, z+1)
		bottomRight = !state.IsBlockA(chunk, x+1, y, z+1)
	case mc.DIRECTION_West, mc.DIRECTION_East:
		if dir == mc.DIRECTION_West {
			x -= 1
		} else {
			x += 1
		}
		top = !state.IsBlockA(chunk, x+0, y+1, z)
		bottom = !state.IsBlockA(chunk, x+0, y-1, z)
		left = !state.IsBlockA(chunk, x, y, z+1)
		right = !state.IsBlockA(chunk, x, y, z-1)

		topLeft = !state.IsBlockA(chunk, x, y+1, z+1)
		topRight = !state.IsBlockA(chunk, x, y+1, z-1)

		bottomLeft = !state.IsBlockA(chunk, x, y-1, z+1)
		bottomRight = !state.IsBlockA(chunk, x, y-1, z-1)
	case mc.DIRECTION_South, mc.DIRECTION_North:
		if dir == mc.DIRECTION_South {
			z += 1
		} else {
			z -= 1
		}
		top = !state.IsBlockA(chunk, x+0, y+1, z)
		bottom = !state.IsBlockA(chunk, x+0, y-1, z)
		left = !state.IsBlockA(chunk, x-1, y, z)
		right = !state.IsBlockA(chunk, x+1, y, z)

		topLeft = !state.IsBlockA(chunk, x-1, y+1, z)
		topRight = !state.IsBlockA(chunk, x+1, y+1, z)

		bottomLeft = !state.IsBlockA(chunk, x-1, y-1, z)
		bottomRight = !state.IsBlockA(chunk, x+1, y-1, z)
	}
	tl := calcAO(left, top, topLeft)
	tr := calcAO(top, right, topRight)
	br := calcAO(bottom, right, bottomRight)
	bl := calcAO(bottom, left, bottomLeft)

	switch dir {
	case mc.DIRECTION_Up:
		result.AO = [4]int{bl, br, tr, tl}
	case mc.DIRECTION_Down:
		result.AO = [4]int{tl, tr, br, bl}
	case mc.DIRECTION_East:
		result.AO = [4]int{bl, br, tr, tl}
	case mc.DIRECTION_West:
		result.AO = [4]int{br, bl, tl, tr}
	case mc.DIRECTION_North:
		result.AO = [4]int{br, bl, tl, tr}
	case mc.DIRECTION_South:
		result.AO = [4]int{bl, br, tr, tl}
	}
	return result
}

func calcAO(edge1, edge2, corner bool) int {
	if edge1 && edge2 {
		return 3
	}
	return (btoi(edge1) + btoi(edge2) + btoi(corner))
}
func btoi(b bool) int {
	if b {
		return 1
	}
	return 0
}

// Helper that also checks neighbour chunks if needed.
func (state *ScreenInGameState) IsBlockA(c *Chunk, x, y, z int, b ...mc.BlockID) bool {
	if y < 0 || y >= mc.CHUNK_SIZE_Y {
		return true
	}

	cx := int32(c.coord.X)
	cz := int32(c.coord.Z)

	if x < 0 {
		cx--
		x += mc.CHUNK_SIZE_XZ
	} else if x >= mc.CHUNK_SIZE_XZ {
		cx++
		x -= mc.CHUNK_SIZE_XZ
	}

	if z < 0 {
		cz--
		z += mc.CHUNK_SIZE_XZ
	} else if z >= mc.CHUNK_SIZE_XZ {
		cz++
		z -= mc.CHUNK_SIZE_XZ
	}

	// If the coordinates point to a different chunk, fetch the neighbor
	if cx != int32(c.coord.X) || cz != int32(c.coord.Z) {
		neighbor := state.Chunks.Get(ChunkCoordinate{X: cx, Z: cz})
		if neighbor == nil {
			return true
		}
		return neighbor.data.IsBlockA(x, y, z, b...)
	}

	return c.data.IsBlockA(x, y, z, b...)
}

func (chunk *Chunk) emitQuad(
	mesh *gfx.Mesh,
	vertices [4]gfx.Vector3,
	x, y, z float32,
	uv mc.AtlasUV,
	color gfx.Color,
	AO AmbientOcclusion,
	FaceShading float32,
) {
	var AOValues = [4]float32{1.0, .5, .25, .1}

	// Anisotropy fix: flip the quad diagonal when AO interpolation
	// would produce artifacts along the default triangulation.
	flip := AO.AO[0]+AO.AO[2] > AO.AO[1]+AO.AO[3]

	for vi := range 4 {
		i := vi
		if flip {
			i = (vi + 1) % 4
		}
		mesh.QuadVertex3f(
			x+vertices[i].X,
			y+vertices[i].Y,
			z+vertices[i].Z,
		)
		ao := AOValues[AO.AO[i]]
		vertColor := gfx.NewColor3f(color.RGB().Scale(ao * FaceShading))
		// FaceShading
		mesh.QuadTexCoord2f(uv.Corners[i][0], uv.Corners[i][1])
		mesh.QuadColor4ub(vertColor.R, vertColor.G, vertColor.B, vertColor.A)
		mesh.QuadEndVertex(true, true, true)
	}
}

// NOTE: set NeedsRebuilt properly before calling.
func (state *ScreenInGameState) BuildChunkMesh(c *Chunk) {
	type Face struct {
		Direction  mc.Direction
		Dx, Dy, Dz int // state coordinate to check where air is

		Vertices [4]gfx.Vector3
	}
	var Faces = []Face{
		{ // Top (+Y)
			Direction: mc.DIRECTION_Up, Dx: 0, Dy: 1, Dz: 0,
			Vertices: [4]gfx.Vector3{{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
		},
		{ // Bottom (-Y)
			Direction: mc.DIRECTION_Down, Dx: 0, Dy: -1, Dz: 0,
			Vertices: [4]gfx.Vector3{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
		},
		{ // North (-Z)
			Direction: mc.DIRECTION_North, Dx: 0, Dy: 0, Dz: -1,
			Vertices: [4]gfx.Vector3{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
		},
		{ // South (+Z)
			Direction: mc.DIRECTION_South, Dx: 0, Dy: 0, Dz: 1,
			Vertices: [4]gfx.Vector3{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
		},
		{ // West (-X)
			Direction: mc.DIRECTION_West, Dx: -1, Dy: 0, Dz: 0,
			Vertices: [4]gfx.Vector3{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
		},
		{ // East (+X)
			Direction: mc.DIRECTION_East, Dx: 1, Dy: 0, Dz: 0,
			Vertices: [4]gfx.Vector3{{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
		},
	}
	var faceShading = [6]float32{
		0.5, 1.0, // down, up
		0.8, 0.5, // north, south
		0.5, 0.8, // right, left
	}
	for x := range 16 {
		for z := range 16 {
			for y := range 128 {
				idx := mc.ChunkIndex(x, y, z)
				block := c.data.Blocks[idx]
				if block == mc.BLOCK_Air {
					continue
				}
				metadata := c.data.Metadata[idx]
				section := y >> 4
				X, Y, Z := float32(x), float32(y-section*16), float32(z)
				const grassColor = 0x62c742 // taken from misc/grasscolor.png
				const oakLeafColor = 0x63aa44
				// select the correct sub-mesh based on block coordinats
				var mesh *gfx.Mesh
				if c.data.IsBlockA(x, y, z, mc.TransparentBlocks...) {
					mesh = c.Layer1[section]
				} else {
					mesh = c.Layer0[section]
				}

				for _, face := range Faces {
					// shade := faceShading[face.Direction]
					if state.IsBlockA(c, x+face.Dx, y+face.Dy, z+face.Dz, mc.TransparentBlocks...) {
						var color = gfx.White
						AO := state.FaceAO(c, x, y, z, face.Direction)
						uv := mc.GetUVFromBlockSideAndMetadata(block, face.Direction, int(metadata))
						if block == mc.BLOCK_Grass && face.Direction == mc.DIRECTION_Up {
							color = gfx.NewColorHex(grassColor)
						}
						if block == mc.BLOCK_Leaves {
							color = gfx.NewColorHex(oakLeafColor)
						}
						c.emitQuad(mesh, face.Vertices, X, Y, Z, uv, color, AO, faceShading[face.Direction])
						if block == mc.BLOCK_Grass && face.Direction != mc.DIRECTION_Up && face.Direction != mc.DIRECTION_Down {
							c.emitQuad(mesh, face.Vertices, X, Y, Z,
								mc.GetUV(38), gfx.NewColorHex(grassColor), AO, 1)
						}
					}
				}
			}
		}
	}

	c.UploadMeshes()
}

// Chunk struct methods.
func (chunk *Chunk) ResetMeshes() {
	for i := range 8 {
		chunk.Layer0[i].Reset()
		chunk.Layer1[i].Reset()
	}
}

func (chunk *Chunk) UploadMeshes() {
	for i := range 8 {
		chunk.Layer0[i].Upload(false)
		chunk.Layer1[i].Upload(false)

	}
}

// GetPosition returns chunk position in world coordinates.
func (chunk *Chunk) GetPosition() gfx.Vector3 {
	return gfx.NewVector3(float32(chunk.coord.X*16), 0, float32(chunk.coord.Z*16))
}

// GetSectionPosition returns the chunk section position in world coordinates.
func (chunk *Chunk) GetSectionPosition(section int) gfx.Vector3 {
	return gfx.NewVector3(
		float32(chunk.coord.X*16),
		float32(section*16),
		float32(chunk.coord.Z*16),
	)
}
func (chunk *Chunk) GetSectionCenter(section int) gfx.Vector3 {
	return gfx.NewVector3(
		float32(chunk.coord.X*16+8),
		float32(section*16+8),
		float32(chunk.coord.Z*16+8),
	)
}

// GetCenter returns chunk center in world coordinates.
func (chunk *Chunk) GetCenter() gfx.Vector3 {
	return gfx.NewVector3(
		float32(chunk.coord.X*16+8),
		64,
		float32(chunk.coord.Z*16+8),
	)
}
func (chunk *Chunk) DrawSectionMesh(section int, terrain gfx.Texture) {
	basePos := chunk.GetSectionPosition(section)

	mat := gfx.MatrixTranslate(
		basePos.X,
		basePos.Y,
		basePos.Z,
	)

	if mesh := chunk.Layer0[section]; mesh != nil {
		mesh.Draw(terrain, gfx.White, mat)
	}

	if mesh := chunk.Layer1[section]; mesh != nil {
		mesh.Draw(terrain, gfx.White, mat)
	}
}
