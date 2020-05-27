#pragma once

#ifndef STARTING_POSITION_ITERATION_H
#define STARTING_POSITION_ITERATION_H

class CitySiteEvaluator;

/*  advc.027: New file. A "starting position" is a selection of one starting tile
	per civ player. This algorithm, inspired by Voronoi iteration, iteratively
	tries to improve the fairness of an initial starting position. */

class StartingPositionIteration
{
	class DistanceTable;

public:
	StartingPositionIteration();
	~StartingPositionIteration();
	void computeStartValues(
			EnumMap<PlayerTypes,short> const& kFoundValues,
			EnumMap<PlotNumTypes,scaled> const& kYieldValues,
			DistanceTable const& kPathDists,
			EnumMap<PlayerTypes,scaled>& kStartValues);

private:
	bool m_bRestrictedAreas;
	CitySiteEvaluator* m_pEval;

	void initSiteEvaluator();

	typedef std::set<PlotNumTypes> VoronoiCell;
	class PotentialSiteGen
	{
	public:
		PotentialSiteGen(CitySiteEvaluator const& kEval, bool bRestrictedAreas);
		~PotentialSiteGen();
		size_t numSites() const { return m_foundValuesPerSite.size(); }
		void removeOriginalSites();
		void getPlots(std::vector<CvPlot const*>& r) const;
		VoronoiCell* getCell(PlayerTypes eOriginalSite) const;
		void getCurrentFoundValues(
				EnumMap<PlayerTypes,short>& kFoundValuesPerPlayer) const;

	private:
		CitySiteEvaluator const& m_kEval;
		std::map<PlotNumTypes,short> m_foundValuesPerSite;
		std::map<PlayerTypes,VoronoiCell*> m_sitesClosestToOriginalSite;
		EnumMap<PlotNumTypes,scaled>* m_pVicinityPenaltiesPerPlot;

		scaled computeMinFoundValue() const;
		void recordSite(CvPlot const& kPlot, short iFoundValue, bool bAdd);
		int fewestPotentialSites() const;
	};

	class DistanceTable
	{
		/*	Internal ids so that m_aaiDistances can be a good deal smaller than
			the map dimensions */
		enum SourceID { NOT_A_SOURCE = -1 };
		enum DestinationID { NOT_A_DESTINATION = -1 };
	public:
		DistanceTable(std::vector<CvPlot const*>& kSources,
				std::vector<CvPlot const*>& kDestinations);
		short d(CvPlot const& kSource, CvPlot const& kDestination) const;

	private:
		std::vector<std::vector<short> > m_distances;
		std::vector<SourceID> m_sourceIDs;
		std::vector<DestinationID> m_destinationIDs;

		void computeDistances(CvPlot const& kSource);
		void setDistance(CvPlot const& kSource, CvPlot const& kDestination,
				short iDistance);
		static short stepDist(CvPlot const& kFrom, CvPlot const& kTo);
	};
};


#endif
