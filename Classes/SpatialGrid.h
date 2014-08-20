#ifndef __SpatialGrid_H__
#define __SpatialGrid_H__

#include "cocos2d.h"
#include "Particle.h"
#include "Telemetry.h"

USING_NS_CC;

typedef std::list<Particle*> SpatialGridCell;

class SpatialGrid
{
public:
	SpatialGrid(Rect rect, double gridSize, double neighborRange)
	{
		xl = rect.getMinX();
		xh = rect.getMaxX();
		yl = rect.getMinY();
		yh = rect.getMaxY();
		xCount = (xh - xl) / gridSize + 1;
		yCount = (yh - yl) / gridSize + 1;
		size = xCount * yCount;
		grid = std::make_unique<SpatialGridCell[]>(size);
		this->gridSize = gridSize;
		this->neighborRange = neighborRange;
		neighborRangeSq = neighborRange * neighborRange;
		rangeInCellCount = (int)(neighborRange / gridSize) + 1;
	}

	void initializeGrid(std::vector<Particle>& particles)
	{
		for (int i = 0; i < size; i++)
		{
			grid[i].clear();
		}

		for (Particle& p : particles)
		{
			int cell = getCellForPosition(p.body->getPosition());
			if (cell >= 0 && cell < size)
				grid[cell].push_back(&p);
			p.neighbors.clear();
		}
	}

	void calculateNeighborsSymmetric()
	{
		for (int x = 0; x < xCount; x++)
		{
			for (int y = 0; y < yCount; y++)
			{
				int i = getCellForXY(x, y);
				if (grid[i].size() > 0)
				{
					for (Particle* pi : grid[i])
					{
						// Calculate neighbors within cell.
						for (Particle* pj : grid[i])
						{
							if (pi->pos.y < pj->pos.y || (pi->pos.y == pj->pos.y && pi->pos.x < pj->pos.x))
							{
								if (pi->getDistanceSq(*pj) <= neighborRangeSq)
								{
									appendNeighborSymmetric(pi, pj);
								}
							}
						}

						// Calculate neighbors on other cells.
						appendNeighborsOnCellSymmetric(pi, x + 1, y);
						appendNeighborsOnCellSymmetric(pi, x - 1, y + 1);
						appendNeighborsOnCellSymmetric(pi, x, y + 1);
						appendNeighborsOnCellSymmetric(pi, x + 1, y + 1);
					}
				}
			}
		}
	}

	void calculateNeighbors()
	{
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
		for (int x = 0; x < xCount; x++)
		{
			for (int y = 0; y < yCount; y++)
			{
				int i = getCellForXY(x, y);
				if (grid[i].size() > 0)
				{
					for (Particle* pi : grid[i])
					{
						// Calculate neighbors within cell.
						for (Particle* pj : grid[i])
						{
							if (pj->pos.x != pi->pos.x && pj->pos.y != pi->pos.y && pi->getDistanceSq(*pj) <= neighborRangeSq)
							{
								appendNeighbor(pi, pj);
							}
						}

						// Calculate neighbors on other cells.
						appendNeighborsOnCell(pi, x + 1, y);
						appendNeighborsOnCell(pi, x - 1, y + 1);
						appendNeighborsOnCell(pi, x, y + 1);
						appendNeighborsOnCell(pi, x + 1, y + 1);
						appendNeighborsOnCell(pi, x + 1, y - 1);
						appendNeighborsOnCell(pi, x, y - 1);
						appendNeighborsOnCell(pi, x - 1, y - 1);
						appendNeighborsOnCell(pi, x - 1, y);
					}
				}
			}
		}
	}

protected:
	std::unique_ptr<SpatialGridCell[]> grid;
	double xl, xh, yl, yh, gridSize, neighborRange, neighborRangeSq, rangeInCellCount;
	int xCount, yCount, size;

	int getCellForPosition(const Vec2& pos)
	{
		const auto& cell = getXYForPosition(pos);
		return getCellForXY(cell.first, cell.second);
	}

	std::pair<int, int> getXYForPosition(const Vec2& pos)
	{
		int x = (int)((pos.x - xl) / gridSize);
		int y = (int)((pos.y - yl) / gridSize);
		return std::make_pair(x, y);
	}

	int getCellForXY(int x, int y)
	{
		return x * yCount + y;
	}

	bool withinRange(int x, int y)
	{
		return x >= 0 && y >= 0 && x < xCount && y < yCount;
	}

	void appendNeighborsOnCellSymmetric(Particle* pi, int x, int y)
	{
		if (withinRange(x, y))
		{
			for (Particle* pj : grid[getCellForXY(x, y)])
			{
				if (pi->getDistanceSq(*pj) <= neighborRangeSq)
				{
					appendNeighborSymmetric(pi, pj);
				}
			}
		}
	}

	void appendNeighborSymmetric(Particle* pi, Particle* pj)
	{
		Vec2 r = pi->pos - pj->pos;
		pi->neighbors.push_back(Neighbor(pj, r));
		pj->neighbors.push_back(Neighbor(pi, -r));
	}

	void appendNeighborsOnCell(Particle* pi, int x, int y)
	{
		if (withinRange(x, y))
		{
			for (Particle* pj : grid[getCellForXY(x, y)])
			{
				if (pi->getDistanceSq(*pj) <= neighborRangeSq)
				{
					appendNeighbor(pi, pj);
				}
			}
		}
	}

	void appendNeighbor(Particle* pi, Particle* pj)
	{
		Vec2 r = pi->pos - pj->pos;
		pi->neighbors.push_back(Neighbor(pj, r));
	}
};

#endif //__SpatialGrid_H__