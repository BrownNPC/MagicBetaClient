package game

import (
	"mbc/gfx"
	"mbc/net/mc"
)

// Helper that also checks neighbour chunks if needed.
func (state *ScreenInGameState) IsAir(c Chunk, x, y, z int) bool {
	if y < 0 || y >= mc.CHUNK_SIZE_Y {
		return true
	}

	switch {
	case x < 0:
		left := state.Chunks.Get(ChunkCoordinate{X: int32(c.data.X - 1), Z: int32(c.data.Z)})
		return left.data == nil || left.data.IsAir(15, y, z)

	case x >= mc.CHUNK_SIZE_XZ:
		right := state.Chunks.Get(ChunkCoordinate{X: int32(c.data.X + 1), Z: int32(c.data.Z)})
		return right.data == nil || right.data.IsAir(0, y, z)

	case z < 0:
		back := state.Chunks.Get(ChunkCoordinate{X: int32(c.data.X), Z: int32(c.data.Z - 1)})
		return back.data == nil || back.data.IsAir(x, y, 15)

	case z >= mc.CHUNK_SIZE_XZ:
		front := state.Chunks.Get(ChunkCoordinate{X: int32(c.data.X), Z: int32(c.data.Z + 1)})
		return front.data == nil || front.data.IsAir(x, y, 0)

	default:
		return c.data.IsAir(x, y, z)
	}
}
func (state *ScreenInGameState) BuildChunkMesh(c Chunk) {
	// reference: https://github.com/BrownNPC/Mine/blob/master/scenes/start/buildChunkMesh.go
	var mesh *gfx.Mesh = &c.mesh
	for x := range mc.CHUNK_SIZE_XZ {
		for z := range mc.CHUNK_SIZE_XZ {
			for y := range mc.CHUNK_SIZE_Y {
				idx := mc.ChunkIndex(x, y, z)
				block := c.data.Blocks[idx]
				if block == mc.BLOCK_Air {
					continue
				}
				// top face:
				X, Y, Z := float32(x), float32(y), float32(z)
				if state.IsAir(c, x, y+1, z) {
					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
				}
				// bottom face:
				if state.IsAir(c, x, y-1, z) {
					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
				}
				// right face:
				if state.IsAir(c, x+1, y, z) {
					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
				}
				// left face:
				if state.IsAir(c, x-1, y, z) {
					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
				}
				// back face:
				if state.IsAir(c, x, y, z-1) {
					mesh.QuadVertex3f(X, Y, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y, Z)
					mesh.QuadEndVertex(true, false, false)
				}
				// front face:
				if state.IsAir(c, x, y, z+1) {
					mesh.QuadVertex3f(X, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y+1, Z+1)
					mesh.QuadEndVertex(true, false, false)
					mesh.QuadVertex3f(X+1, Y, Z+1)
					mesh.QuadEndVertex(true, false, false)
				}
			}
		}
	}
	mesh.Upload(false)
}
