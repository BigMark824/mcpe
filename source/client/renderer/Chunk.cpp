/********************************************************************
	Minecraft: Pocket Edition - Decompilation Project
	Copyright (C) 2023 iProgramInCpp
	
	The following code is licensed under the BSD 1 clause license.
	SPDX-License-Identifier: BSD-1-Clause
 ********************************************************************/

#include "Chunk.hpp"
#include "world/level/Level.hpp"
#include "world/level/Region.hpp"
#include "TileRenderer.hpp"

int Chunk::updates;

float Chunk::distanceToSqr(const Entity* pEnt) const
{
	float dX = pEnt->m_pos.x - float(m_pos2.x);
	float dY = pEnt->m_pos.y - float(m_pos2.y);
	float dZ = pEnt->m_pos.z - float(m_pos2.z);
	return dX * dX + dY * dY + dZ * dZ;
}
float Chunk::squishedDistanceToSqr(const Entity* pEnt) const
{
	float dX = pEnt->m_pos.x - float(m_pos2.x);
	float dY = pEnt->m_pos.y - float(m_pos2.y);
	float dZ = pEnt->m_pos.z - float(m_pos2.z);

	dY *= 2;

	return dX * dX + dY * dY + dZ * dZ;
}

void Chunk::reset()
{
	m_bEmpty[0] = true;
	m_bEmpty[1] = true;
	m_bVisible = false;
	m_bCompiled = false;
}

int Chunk::getList(int idx)
{
	if (!m_bVisible)
		return -1;

	if (m_bEmpty[idx])
		return -1;

	return m_lists + idx;
}

RenderChunk* Chunk::getRenderChunk(int idx)
{
	return &m_renderChunks[idx];
}

int Chunk::getAllLists(int* arr, int arr_idx, int idx)
{
	if (!m_bVisible)
		return arr_idx;

	if (m_bEmpty[idx])
		return arr_idx;

	arr[arr_idx++] = m_lists + idx;

	return arr_idx;
}

void Chunk::cull(Culler* pCuller)
{
	m_bVisible = pCuller->isVisible(m_aabb);
}

void Chunk::renderBB()
{

}

bool Chunk::isEmpty()
{
	if (!m_bCompiled)
		return false;

	if (!m_bEmpty[0])
		return false;

	if (!m_bEmpty[1])
		return false;

	return true;
}

void Chunk::setDirty()
{
	m_bDirty = true;
}

void Chunk::setPos(int x, int y, int z)
{
	if (m_pos.x == x && m_pos.y == y && m_pos.z == z)
		// No change.
		return;

	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
	m_pos2.x = x + m_xs  / 2;
	m_pos2.y = y + m_ys  / 2;
	m_pos2.z = z + m_zs / 2;

	m_aabb = AABB(float(m_pos.x - 1), float(m_pos.y - 1), float(m_pos.z - 1), float(m_pos.x + m_xs  + 1), float(m_pos.y + m_ys  + 1), float(m_pos.z + m_zs + 1));

	setDirty();
}

void Chunk::setClean()
{
	m_bDirty = false;
}

bool Chunk::isDirty()
{
	return m_bDirty;
}

void Chunk::rebuild()
{
	if (!m_bDirty)
		return;

	updates++;

	LevelChunk::touchedSky = false;

	m_bEmpty[0] = true;
	m_bEmpty[1] = true;

	int minX = m_pos.x, maxX = m_pos.x + m_xs ;
	int minY = m_pos.y, maxY = m_pos.y + m_ys ;
	int minZ = m_pos.z, maxZ = m_pos.z + m_zs;

	Region region(m_pLevel, minX - 1, minY - 1, minZ - 1, maxX + 1, maxY + 1, maxZ + 1);
	TileRenderer tileRenderer(&region);

	Tesselator& t = Tesselator::instance;

	for (int layer = 0; layer < 2; layer++)
	{
		bool bTesselatedAnything = false, bDrewThisLayer = false, bNeedAnotherLayer = false;
		for (int y = minY; y < maxY; y++)
		{
			for (int z = minZ; z < maxZ; z++)
			{
				for (int x = minX; x < maxX; x++)
				{
					TileID tile = region.getTile(x, y, z);
					if (tile <= 0) continue;

					if (!bTesselatedAnything)
					{
						bTesselatedAnything = true;
						t.begin();
						t.offset(float(-m_pos.x), float(-m_pos.y), float(-m_pos.z));
					}

					Tile* pTile = Tile::tiles[tile];

					if (layer == pTile->getRenderLayer())
					{
						if (tileRenderer.tesselateInWorld(pTile, x, y, z))
							bDrewThisLayer = true;
					}
					else
					{
						bNeedAnotherLayer = true;
					}
				}
			}
		}

		if (bTesselatedAnything)
		{
			RenderChunk rchk = t.end(field_90[layer]);
			RenderChunk* pRChk = &m_renderChunks[layer];

			*pRChk = rchk;
			pRChk->field_C = float(m_pos.x);
			pRChk->field_10 = float(m_pos.y);
			pRChk->field_14 = float(m_pos.z);

			t.offset(0.0f, 0.0f, 0.0f);

			if (bDrewThisLayer)
				m_bEmpty[layer] = false;
		}

		if (!bNeedAnotherLayer)
			break;
	}

	m_skyLit = LevelChunk::touchedSky;
	m_bCompiled = true;
}

void Chunk::translateToPos()
{
	glTranslatef(float(m_pos.x), float(m_pos.y), float(m_pos.z));
}

Chunk::Chunk(Level* level, int x, int y, int z, int a, int b, GLuint* bufs)
{
	field_4D = true;
	field_4E = false;
	m_bCompiled = false;
	m_bDirty = false;

	m_pLevel = level;
	m_xs  = a;
	m_ys  = a;
	m_zs = a;
	m_pTesselator = &Tesselator::instance;
	m_lists = b;
	m_pos.x = -999;
	m_radius = Mth::sqrt(float(m_xs  * m_xs  + m_ys  * m_ys  + m_zs * m_zs)) / 2;
	field_90 = bufs;

	setPos(x, y, z);
}
