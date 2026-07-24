package game

import (
	"mbc/gfx"
	"mbc/net/mc"
)

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
		top = !state.IsAir(chunk, x+0, y, z-1)
		bottom = !state.IsAir(chunk, x+0, y, z+1)
		left = !state.IsAir(chunk, x-1, y, z+0)
		right = !state.IsAir(chunk, x+1, y, z+0)

		topLeft = !state.IsAir(chunk, x-1, y, z-1)
		topRight = !state.IsAir(chunk, x+1, y, z-1)

		bottomLeft = !state.IsAir(chunk, x-1, y, z+1)
		bottomRight = !state.IsAir(chunk, x+1, y, z+1)
	case mc.DIRECTION_West, mc.DIRECTION_East:
		if dir == mc.DIRECTION_West {
			x -= 1
		} else {
			x += 1
		}
		top = !state.IsAir(chunk, x+0, y+1, z)
		bottom = !state.IsAir(chunk, x+0, y-1, z)
		left = !state.IsAir(chunk, x, y, z+1)
		right = !state.IsAir(chunk, x, y, z-1)

		topLeft = !state.IsAir(chunk, x, y+1, z+1)
		topRight = !state.IsAir(chunk, x, y+1, z-1)

		bottomLeft = !state.IsAir(chunk, x, y-1, z+1)
		bottomRight = !state.IsAir(chunk, x, y-1, z-1)
	case mc.DIRECTION_South, mc.DIRECTION_North:
		if dir == mc.DIRECTION_South {
			z += 1
		} else {
			z -= 1
		}
		top = !state.IsAir(chunk, x+0, y+1, z)
		bottom = !state.IsAir(chunk, x+0, y-1, z)
		left = !state.IsAir(chunk, x-1, y, z)
		right = !state.IsAir(chunk, x+1, y, z)

		topLeft = !state.IsAir(chunk, x-1, y+1, z)
		topRight = !state.IsAir(chunk, x+1, y+1, z)

		bottomLeft = !state.IsAir(chunk, x-1, y-1, z)
		bottomRight = !state.IsAir(chunk, x+1, y-1, z)
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
func (state *ScreenInGameState) IsAir(c *Chunk, x, y, z int) bool {
	if y < 0 || y >= mc.CHUNK_SIZE_Y {
		return true
	}

	switch {
	case x < 0:
		left := state.Chunks.Get(ChunkCoordinate{X: int32(c.coord.X - 1), Z: int32(c.coord.Z)})
		return left == nil || left.data.IsAir(15, y, z)

	case x >= mc.CHUNK_SIZE_XZ:
		right := state.Chunks.Get(ChunkCoordinate{X: int32(c.coord.X + 1), Z: int32(c.coord.Z)})
		return right == nil || right.data.IsAir(0, y, z)

	case z < 0:
		back := state.Chunks.Get(ChunkCoordinate{X: int32(c.coord.X), Z: int32(c.coord.Z - 1)})
		return back == nil || back.data.IsAir(x, y, 15)

	case z >= mc.CHUNK_SIZE_XZ:
		front := state.Chunks.Get(ChunkCoordinate{X: int32(c.coord.X), Z: int32(c.coord.Z + 1)})
		return front == nil || front.data.IsAir(x, y, 0)

	default:
		return c.data.IsAir(x, y, z)
	}
}

func (c *Chunk) emitQuad(
	mesh *gfx.Mesh,
	vertices [4]gfx.Vector3,
	x, y, z float32,
	uv mc.AtlasUV,
	color gfx.Color,
	AO AmbientOcclusion,
	FaceShading float32,
) {
	var AOValues = [4]float32{1.0, .5, .25, .1}

	for i := range 4 {
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
				X, Y, Z := float32(x), float32(y), float32(z)
				const grassColor = 0x62c742 // taken from misc/grasscolor.png
				const oakLeafColor = 0x63aa44
				mesh := c.Layer0

				switch block {
				case mc.BLOCK_Leaves:
					mesh = c.Layer1
				}
				for _, face := range Faces {
					// shade := faceShading[face.Direction]
					if state.IsAir(c, x+face.Dx, y+face.Dy, z+face.Dz) {
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

	c.Layer0.Upload(false)
	c.Layer1.Upload(false)
}
