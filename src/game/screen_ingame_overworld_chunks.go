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

func (state *ScreenInGameState) BuildChunkMesh(c *Chunk) {
	var mesh *gfx.Mesh = c.mesh

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

				for _, face := range Faces {
					if state.IsAir(c, x+face.Dx, y+face.Dy, z+face.Dz) {
						t := mc.GetUVFromBlockSideAndMetadata(block, face.Direction, int(metadata))
						for i := range 4 { // 4 vertices to make a quad
							vx := X + face.Vertices[i].X
							vy := Y + face.Vertices[i].Y
							vz := Z + face.Vertices[i].Z

							mesh.QuadVertex3f(vx, vy, vz)
							mesh.QuadTexCoord2f(t.Corners[i][0], t.Corners[i][1])
							mesh.QuadEndVertex(true, true, false)
						}
					}
				}
			}
		}
	}
	mesh.Upload(false)
}
