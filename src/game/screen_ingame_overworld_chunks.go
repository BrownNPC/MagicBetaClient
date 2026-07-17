package game

import (
	"mbc/gfx"
	"mbc/net/mc"
)

func (state *ScreenInGameState) BuildChunkMesh(mesh *gfx.Mesh, c *mc.DecompressedChunk) {
	for x := range mc.CHUNK_SIZE_XZ {
		for z := range mc.CHUNK_SIZE_XZ {
			for y := range mc.CHUNK_SIZE_Y {
				idx := mc.ChunkIndex(x, y, z)
				block := c.Blocks[idx]
				if block == mc.BLOCK_Air {
					continue
				}
				mesh.QuadVertex3f(float32(x), float32(y), float32(z))
				mesh.QuadEndVertex(true, false, false)
			}
		}
	}
}
