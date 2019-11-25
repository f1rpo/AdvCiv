#pragma once

#ifndef UWAI_SETS_H
#define UWAI_SETS_H

/*  advc.104: Types for small sets of (unique) players, teams and cities.
	For wars simulated by the utility-based war AI component (a.k.a war-and-peace AI). */

// Tbd.: Test if flat_set is more efficient
/*  Would like to derive my set classes from boost::noncopyable (cf. advc.003e)
	to prevent errors like:
	CitySet kMyReference = kAnalyst.lostCities(ePlayer);
	However, this would also prevent vectors of sets from being initialized
	through vector::resize. */
typedef std::set<int> CitySet;
typedef CitySet::const_iterator CitySetIter;
// "Plyr" - make them all four letter prefixes
typedef std::set<PlayerTypes> PlyrSet;
typedef PlyrSet::const_iterator PlyrSetIter;
typedef std::set<TeamTypes> TeamSet;
typedef TeamSet::const_iterator TeamSetIter;

#endif
