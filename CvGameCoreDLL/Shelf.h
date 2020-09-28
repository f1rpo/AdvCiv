#pragma once

#ifndef SHELF_H
#define SHELF_H

class CvPlot;
class CvUnit;
enum RandPlotFlags;

/* <advc.300> New class representing a continental shelf, akin to a CvArea
   (but I don't want to inherit from that class). Used for spawning
   Barbarian sea units. */
class Shelf {

public:

	void add(CvPlot* plot);
	CvPlot* randomPlot(RandPlotFlags restrictions, int unitDistance,
			int* legalCount = NULL) const;
	int size() const;
	int countUnownedPlots() const;
	int countBarbarians() const;
	CvUnit* randomBarbarianTransport() const; // advc.306
	bool killBarbarian(); // Just any one Barbarian ship; false if none.


private:

	std::vector<CvPlot*> plots;

public:
	// Just to avoid nested pairs in map types
	class Id : public std::pair<int,int> {
	public: Id(int landId, int waterId); };
};
// </advc.300>

#endif
