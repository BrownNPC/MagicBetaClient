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
	for x := range mc.CHUNK_SIZE_XZ {
		for z := range mc.CHUNK_SIZE_XZ {
			for y := range mc.CHUNK_SIZE_Y {
				idx := mc.ChunkIndex(x, y, z)
				block := c.data.Blocks[idx]
				if block == mc.BLOCK_Air {
					continue
				}
				
				metadata := c.data.Metadata[idx]
				X, Y, Z := float32(x), float32(y), float32(z)

				// Top face (+Y, Up)
				if state.IsAir(c, x, y+1, z) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_Up, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3] // [u_min, v_min, u_max, v_max]

					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, true, false)
				}

				// Bottom face (-Y, Down)
				if state.IsAir(c, x, y-1, z) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_Down, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3]

					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, true, false)

					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, true, false)
				}

				// Right face (+X, East)
				if state.IsAir(c, x+1, y, z) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_East, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3]

					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, false, false)
				}

				// Left face (-X, West)
				if state.IsAir(c, x-1, y, z) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_West, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3]

					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, false, false)
				}

				// Front face (+Z, South)
				if state.IsAir(c, x, y, z+1) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_South, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3]

					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, false, false)
				}

				// Back face (-Z, North)
				if state.IsAir(c, x, y, z-1) {
					t := mc.GetUVFromBlockSideAndMetadata(block, mc.DIRECTION_North, int(metadata)).UV
					u0, v0, u1, v1 := t[0], t[1], t[2], t[3]

					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadTexCoord2f(u0, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadTexCoord2f(u1, v1)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadTexCoord2f(u1, v0)
					mesh.QuadEndVertex(true, false, false)

					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadTexCoord2f(u0, v0)
					mesh.QuadEndVertex(true, false, false)
				}
			}
		}
	}
	mesh.Upload(false)
}
