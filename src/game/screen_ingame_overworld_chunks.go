package game

import (
	"mbc/gfx"
	"mbc/net/mc"
)

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
) {
	for i := range 4 {
		mesh.QuadVertex3f(
			x+vertices[i].X,
			y+vertices[i].Y,
			z+vertices[i].Z,
		)
		mesh.QuadTexCoord2f(uv.Corners[i][0], uv.Corners[i][1])
		mesh.QuadColor4ub(color.R, color.G, color.B, color.A)
		mesh.QuadEndVertex(true, true, true)
	}
}
func (state *ScreenInGameState) BuildChunkMesh(c *Chunk) {
	type Face struct {
		Direction  mc.Direction
		Dx, Dy, Dz int // world coordinate to check where air is

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
				mesh := c.Layer0

				switch block {
				case mc.BLOCK_Leaves:
					mesh = c.Layer1
				}

				for _, face := range Faces {
					if state.IsAir(c, x+face.Dx, y+face.Dy, z+face.Dz) {
						var color = gfx.White
						uv := mc.GetUVFromBlockSideAndMetadata(block, face.Direction, int(metadata))
						if block == mc.BLOCK_Grass && face.Direction == mc.DIRECTION_Up {
							color = gfx.NewColorHex(grassColor)
						}
						c.emitQuad(mesh, face.Vertices, X, Y, Z, uv, color)
						if block == mc.BLOCK_Grass && face.Direction != mc.DIRECTION_Up && face.Direction != mc.DIRECTION_Down {
							c.emitQuad(mesh, face.Vertices, X, Y, Z,
								mc.GetUV(38), gfx.NewColorHex(grassColor))
						}
					}
				}
			}
		}
	}

	c.Layer0.Upload(false)
	c.Layer1.Upload(false)
}
