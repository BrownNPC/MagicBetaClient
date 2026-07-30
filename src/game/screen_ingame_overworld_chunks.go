package game

import (
	"mbc/gfx"
	"mbc/net/mc"

	"solod.dev/so/math"
	"solod.dev/so/mem"
	"solod.dev/so/slices"
)

// We need to store face connections. eg. Up connects to Down.
// since Up connects to Down, and Down connects to up is the same thing,
// we can store them in 6*2 bits.
// function will panic if both faces are the same.
func (state *ScreenInGameState) faceToBitIndex(a, b mc.Direction) int {
	if a > b {
		tmp := b
		b = a
		a = tmp
	}
	if a == b {
		panic("faceToBitIndex: cannot exit same face we entered.")
	}
	var i uint8 = 255 // invalid, never actually returned.
	var facePairBitIndex = [...][6]uint8{
		//           D  U  N  S  W  E
		/* Down  */ {i, 0, 1, 2, 3, 4},
		/* Up    */ {0, i, 5, 6, 7, 8},
		/* North */ {1, 5, i, 9, 10, 11},
		/* South */ {2, 6, 9, i, 12, 13},
		/* West  */ {3, 7, 10, 12, i, 14},
		/* East  */ {4, 8, 11, 13, 14, i},
	}
	return int(facePairBitIndex[a][b])
}

// Must be called when an opaque block is changed in a section.
// Traverse each section and build a connectivity graph.
func (state *ScreenInGameState) BuildConnectivityGraphForSection(chunk *Chunk, section int) {
	// It’s rather straightforward to build the connectivity graph for a chunk when an opaque block changes, it follows a simple algorithm:
	// for each block that’s not opaque,
	// start a 3D flood fill, with an empty set of faces
	// every time the flood fill tries to exit the boundaries of the chunk through a face, add the face to the set
	// when the flood fill is done, connect together all the faces that were added to the set.

	state.s.Scratch.Reset()
	visited := slices.Make[bool](&state.s.Scratch, 16*16*16)
	type Vec3i struct {
		X, Y, Z int
	}
	stack := slices.MakeCap[Vec3i](&state.s.Scratch, 0, 16*16*16/4)

	chunk.ConnectivityGraph[section] = 0 // reset graph
	var FaceOffsets = []Vec3i{
		{X: 0, Y: -1, Z: 0}, {X: 0, Y: 1, Z: 0}, // down, up
		{X: 0, Y: 0, Z: 1}, {X: 0, Y: 0, Z: -1}, // north, south
		{X: -1, Y: 0, Z: 0}, {X: 1, Y: 0, Z: 0}, // west, east
	}
	for x := range 16 {
		for z := range 16 {
			for sectionY := range 16 {
				// local section coordinates to parent chunk coordinates
				y := sectionY + section*16
				visitedIdx := y + (z * 16) + (x * 16 * 16)

				// if opaque, or visited, skip.
				if visited[visitedIdx] || !chunk.data.IsBlockA(x, y, z, mc.NonOpaqueBlocks...) {
					continue
				}
				var faceSet [6]bool
				// set state
				stack = stack[:0]
				stack = append(stack, Vec3i{x, y, z})
				visited[visitedIdx] = true

				// for each opaque face, do a flood fill.
				// every time the flood fill tries to exit the
				// section boundaries though a face,
				// add the face to the faceSet.
				for len(stack) > 0 {
					p := stack[len(stack)-1]
					stack = stack[:len(stack)-1]

					for face := range mc.Direction(6) {
						// neighbour coordinates.
						nx := p.X + FaceOffsets[face].X
						ny := p.Y + FaceOffsets[face].Y
						nz := p.Z + FaceOffsets[face].Z
						// check if flood fill is trying to escape to neighbour section.
						if nx < 0 || nx >= 16 ||
							ny < 0 || ny >= section*16 ||
							nz < 0 || nz >= 16 {
							// add to set
							faceSet[face] = true
							continue
						}
						neighbourVisitedIdx := nx + (nz * 16) + (nx * 16 * 16)
						if visited[neighbourVisitedIdx] || chunk.data.IsBlockA(nx, ny, nz) {
							continue
						}
						visited[neighbourVisitedIdx] = true
						stack = append(stack, Vec3i{nx, ny, nz})
					}
				} // end of flood fill
				// connect together all the faces that were added to the faceSet.
				for f, added := range faceSet {
					face := mc.Direction(f)
					if !added {
						continue
					}
					for face2 := face + 1; face < 6; face2++ {
						if face2 == face {
							continue
						}
						i := state.faceToBitIndex(face, face2)
						chunk.ConnectivityGraph[section] |= 1 << i // set bit to 1
					}
				}
			}
		}
	}
}

func (state *ScreenInGameState) toVisibilityArrayIndex(camX, camY, camZ int, x, y, z int) int {
	// Offset coordinates so the camera is at the center of the grid
	gridSize := state.ChunkCullState.gridSize
	relX := (x - camX) + (gridSize / 2)
	relZ := (z - camZ) + (gridSize / 2)
	if relX < 0 || relX >= gridSize || y < 0 || y > 7 || relZ < 0 || relZ >= gridSize {
		return -1 // Out of bounds of our visited array
	}
	return (y*gridSize+relZ)*gridSize + relX
}
func (state *ScreenInGameState) SetRenderDistance(d int) {
	c := &state.ChunkCullState
	c.gridSize = d*2 + 1
}

func (state *ScreenInGameState) MarkVisibleChunks(cam gfx.Camera) {
	c := &state.ChunkCullState
	// Reset state
	c.queue = c.queue[:0]
	c.visibleChunks = c.visibleChunks[:0]
	state.s.Scratch.Reset()
	visited := slices.Make[bool](&state.s.Scratch, 8*c.gridSize*c.gridSize)
	camPos := cam.Position

	camX := int(math.Floor(float64(camPos.X) / 16.0))
	camY := int(math.Floor(float64(camPos.Y) / 16.0))
	camZ := int(math.Floor(float64(camPos.Z) / 16.0))
	camY = min(7, max(camY, 0))

	c.queue = slices.Append(mem.System, c.queue, ChunkBfsStep{
		X:         camX,
		Y:         camY,
		Z:         camZ,
		EntryFace: 255,
	})
	if i := state.toVisibilityArrayIndex(camX, camY, camZ, camX, camY, camZ); i != -1 {
		visited[i] = true
	}
	var FaceOffsets = []gfx.Vector3{
		{X: 0, Y: -1, Z: 0}, {X: 0, Y: 1, Z: 0}, // down, up
		{X: 0, Y: 0, Z: 1}, {X: 0, Y: 0, Z: -1}, // north, south
		{X: -1, Y: 0, Z: 0}, {X: 1, Y: 0, Z: 0}, // west, east
	}

	head := 0
	for head < len(c.queue) {
		curr := c.queue[head]
		head++

		chunk := state.Chunks.Get(ChunkCoordinate{int32(curr.X), int32(curr.Z)})
		if chunk == nil {
			continue
		}
		// frustum cull
		if cam.IsSphereInFrustum(chunk.GetSectionCenter(curr.Y), CHUNK_SECTION_SPHERE_RADIUS) {
			chunk.VisibleSections[curr.Y] = true // mark visible.
			c.visibleChunks =
				slices.Append(mem.System, c.visibleChunks, chunk)
		}
		for exitFace := range mc.Direction(6) {
			// Directional Culling (N dot V)
			// We don't want the BFS walking backward behind the player.
			// If the normal of the face we are trying to exit points away from our view vector, skip it.
			// Added a slight negative margin (-0.3) to allow peripheral vision for high FOV.
			normal := FaceOffsets[exitFace]
			dot := gfx.NewVector3(normal.X, normal.Y, normal.Z).DotProduct(cam.LookVector)
			if dot < -0.9 && curr.EntryFace != 255 {
				continue
			}

			if curr.EntryFace != 255 {
				bitIndex := state.faceToBitIndex(curr.EntryFace, exitFace)
				if chunk.ConnectivityGraph[curr.Y]&(1<<bitIndex) == 0 {
					continue // path blocked by solid blocks.
				}
			}
			nx, ny, nz := curr.X, curr.Y, curr.Z
			switch exitFace {
			case mc.DIRECTION_West:
				nx--
			case mc.DIRECTION_East:
				nx++
			case mc.DIRECTION_Down:
				ny--
			case mc.DIRECTION_Up:
				ny++
			case mc.DIRECTION_North:
				nz--
			case mc.DIRECTION_South:
				nz++
			}

			i := state.toVisibilityArrayIndex(camX, camY, camZ, nx, ny, nz)
			if i == -1 || visited[i] {
				continue
			}

			visited[i] = true
			nextEntryFace := mc.DirectionOpposite[exitFace]
			c.queue = slices.Append(mem.System, c.queue, ChunkBfsStep{
				X: nx, Y: ny, Z: nz,
				EntryFace: nextEntryFace,
			})
		}
	}
}

// AmbientOcclusion for each quad vertex
type AmbientOcclusion struct{ AO [4]int }

var ___FaceAOresult AmbientOcclusion

func (state *ScreenInGameState) FaceAO(chunk *Chunk, x, y, z int, dir mc.Direction) AmbientOcclusion {
	result := ___FaceAOresult
	for i := range 4 {
		result.AO[i] = 4
	}
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
		result.AO[0] = bl
		result.AO[1] = br
		result.AO[2] = tr
		result.AO[3] = tl
	case mc.DIRECTION_Down:
		result.AO[0] = tl
		result.AO[1] = tr
		result.AO[2] = br
		result.AO[3] = bl
	case mc.DIRECTION_East:
		result.AO[0] = bl
		result.AO[1] = br
		result.AO[2] = tr
		result.AO[3] = tl
	case mc.DIRECTION_West:
		result.AO[0] = br
		result.AO[1] = bl
		result.AO[2] = tl
		result.AO[3] = tr
	case mc.DIRECTION_North:
		result.AO[0] = br
		result.AO[1] = bl
		result.AO[2] = tl
		result.AO[3] = tr
	case mc.DIRECTION_South:
		result.AO[0] = bl
		result.AO[1] = br
		result.AO[2] = tr
		result.AO[3] = tl
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
				if c.data.IsBlockA(x, y, z, mc.NonOpaqueBlocks...) {
					mesh = c.Layer1[section]
				} else {
					mesh = c.Layer0[section]
				}

				for _, face := range Faces {
					// shade := faceShading[face.Direction]
					if state.IsBlockA(c, x+face.Dx, y+face.Dy, z+face.Dz, mc.NonOpaqueBlocks...) {
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
