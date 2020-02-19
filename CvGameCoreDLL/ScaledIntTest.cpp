// advc.fract: Test code for the ScaledInt class

#include "CvGameCoreDLL.h"
#include "ScaledInt.h"

//#define SCALED_INT_TEST
#ifdef SCALED_INT_TEST
#include "TSCProfiler.h"
#include "CvInfo_GameOption.h"

// Can't define these in a function body (need to have linkage)
TYPEDEF_SCALED_ENUM(int,166320,MovementPts) // Divisible by most numbers between 1 and 30
TYPEDEF_SCALED_ENUM(int,1024,CurrCombatStr)
#endif

/*  To be called once XML data has been loaded. (Need some test data that is unknown
	at compile time.) */
void TestScaledInt()
{
#ifndef SCALED_INT_TEST
	return;
#else

	/*	These numbers match the running example commented on in pow.
		(The example assumes scale 1024, hence the explicit constructor calls.) */
	FAssert((ScaledInt<int,1024>(fixp(5.2)).pow(ScaledInt<int,1024>(fixp(2.1))).round() == 32));

	// Spotty unit test
	FAssert((fixp(4/3.) * 1000).getInt() == 1333);
	FAssert(scaled_int(0) == per100(0));
	FAssert(fixp(-0.3125) * 1024 == -320);
	FAssert(per1000(2254u).getPercent() == 225);
	FAssert(per10000(22547u).getPermille() == 2255);
	int iSuccesses = 0;
	CvRandom& kRand = GC.getASyncRand();
	for(int i = 0; i < 1000; i++)
		if(fixp(0.4).bernoulliSuccess(kRand, ""))
			iSuccesses++;
	FAssertBounds(360, 440, iSuccesses);
	FAssert(scaled_int(2).pow(10) == 1024);
	FAssert(scaled_int(10).pow(-2) == per100(1));
	FAssert((scaled_int(2).sqrt() * 100).getInt() == 141);
	FAssert((fixp(0.3).pow(fixp(1.7))*100).getInt() == 13);
	FAssert(scaled_int(24).pow(0) == 1);
	FAssert(scaled_int(0).pow(24) == 0);
	FAssert(scaled_int(-2).pow(3) == -8);
	scaled_int rTest = fixp(2.4);
	rTest.increaseTo(3);
	FAssert(rTest == 3);
	rTest.decreaseTo(scaled_int(1, 2));
	FAssert(rTest == fixp(0.5));
	rTest.clamp(1, 2);
	FAssert(rTest == 1);
	FAssert(rTest >= 1);
	FAssert(rTest <= 1);
	FAssert(rTest * fixp(0.93) < 1);
	FAssert(rTest < fixp(1.01));
	FAssert(rTest > fixp(0.999));
	FAssert(rTest.approxEquals(fixp(1.01), fixp(0.05)));
	rTest -= fixp(2.5);
	FAssert(rTest.approxEquals(fixp(-1.5), fixp(0.01)));
	rTest = per100(250u);
	rTest.mulDiv(4, 5);
	FAssert(rTest == 2);
	FAssert(std::strcmp(scaled_int(2).str(100), "200 percent") == 0);
	FAssert(std::strcmp(ScaledInt<int,1024>(2).str(), "2048/1024") == 0);
	FAssert(std::strcmp(scaled_int(2).str(1), "2") == 0);
	FAssert(std::strcmp(fixp(2.2).str(1), "ca. 2") == 0);
	FAssert(scaled_int(42).roundToMultiple(5) == 40);
	FAssert(scaled_int(-43).roundToMultiple(5) == -45);

	MovementPts rMoves = fixp(1.5);
	FAssert(rMoves == scaled_int(3, 2)); // Allowed: operations on ScaledInt<*,*,int>
	FAssert(rMoves - 1 == MovementPts(1, 2));
	CurrCombatStr rCombat = fixp(4.9);
	// Not allowed: MovementPts and CurrCombatStr are incompatible w/ each other
	//if (rMoves < rCombat) ;

	/*	Will do something "non-const" at the end based on the value of iDummy.
		To prevent the compiler from discarding code, results of test computations
		can be added to iDummy. */
	int iDummy = scaled_int(1, 2).round();

	// Speed measurements
	// (CPU cycles noted in comments can be out of date)
	// Exponentiation speed measurements
	{
		scaled_int rSum = 0;
		for (int i = 0; i < 10; i++)
		{
			// Result (average over 10 samples): 7384 cycles on the first launch
			// after compilation. Then 4568 on the next launch, 4688, 4975.
			// Due to cache? But std::pow shows a similar pattern ...
			//TSC_PROFILE("POW_SCALED");
			for (int j = i; j < 10; j++)
			{
				scaled_int b = per100(GC.getInfo((TechTypes)0).getResearchCost() + j);
				rSum += b.pow(fixp(1.24));
			}
		}
		iDummy += rSum.round();
		double dSum = 0;
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("POW_DOUBLE"); // Results (averages): 9357, 5423, 4915, 4989 cycles
			for (int j = i; j < 10; j++)
			{
				double b = (GC.getInfo((TechTypes)0).getResearchCost() + j) / 100.0;
				dSum += std::pow(b, 1.24);
			}
		}
		iDummy += ::round(dSum);
	}

	// Addition speed measurements
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("ADDITION_INT"); // Result: 180 cycles on average
			int x = GC.getInfo((TechTypes)0).getResearchCost();
			int y = GC.getInfo((TechTypes)1).getResearchCost();
			int z = 0;
			for (int j = i; j < 10; j++)
			{
				z += x + j;
				z -= y - j;
			}
			iDummy += z;
		}
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("ADDITION_SCALED"); // Result: 272 cycles on average
			scaled_int x = GC.getInfo((TechTypes)0).getResearchCost();
			int y = GC.getInfo((TechTypes)1).getResearchCost(); // Mix it up a bit
			scaled_int z = 0;
			for (int j = i; j < 10; j++)
			{
				z += x + j;
				z -= y - j;
			}
			iDummy += z.round();
		}
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("ADDITION_DOUBLE"); // Result: 406 cycles on average
			double x = GC.getInfo((TechTypes)0).getResearchCost();
			int y = GC.getInfo((TechTypes)1).getResearchCost();
			double z = 0;
			for (int j = i; j < 10; j++)
			{
				z += x + j;
				z -= y - j;
			}
			iDummy += ::round(z);
		}
	}

	// Modifier speed measurements; inspired by CvTeam::getResearchCost.
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("MODIFIERS_BTS_STYLE"); // Result: 927 cycles on average
			int iCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				iCost += j;
				iCost *= GC.getInfo((HandicapTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= GC.getInfo((HandicapTypes)0).getAIResearchPercent();
				iCost /= 100;
				EraTypes eTechEra = (EraTypes)GC.getInfo((TechTypes)0).getEra();
				iCost *= (100 + GC.getInfo(eTechEra).getTechCostModifier());
				iCost /= 100;
				iCost *= GC.getInfo((WorldSizeTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= GC.getInfo((SeaLevelTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= 105;
				iCost /= 100;
				iCost *= GC.getInfo((GameSpeedTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= GC.getInfo((EraTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= (GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) + 100);
				iCost /= 100;
				iCost -= iCost % 5;
				iCost = range(iCost, 10, 2000);
			}
			iDummy += iCost;
		}
	}
	{
		for (int i = 0; i < 10; i++)
		{
			//	Result: 2855 cycles on average with MulDiv. 3932 with cast to __int64.
			//	1584 for scaled_uint, 1045 for scaled_uint w/o overflow handling.
			//	892 if the per100 arguments are also cast to uint.
			//TSC_PROFILE("MODIFIERS_SCALED");
			scaled_uint rCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				rCost += j;
				rCost *= per100(GC.getInfo((HandicapTypes)0).getResearchPercent());
				rCost *= per100(GC.getInfo((HandicapTypes)0).getAIResearchPercent());
				EraTypes eTechEra = (EraTypes)GC.getInfo((TechTypes)0).getEra();
				rCost *= per100((100 + GC.getInfo(eTechEra).getTechCostModifier()));
				rCost *= per100(GC.getInfo((WorldSizeTypes)0).getResearchPercent());
				rCost *= per100(GC.getInfo((SeaLevelTypes)0).getResearchPercent());
				rCost *= fixp(1.05);
				rCost *= per100(GC.getInfo((GameSpeedTypes)0).getResearchPercent());
				rCost *= per100(GC.getInfo((EraTypes)0).getResearchPercent());
				rCost *= per100((GC.getDefineINT(
						CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) + 100));
				int iCost = rCost.roundToMultiple(5);
				rCost = range(iCost, 10, 2000);
			}
			iDummy += rCost.round();
		}
	}
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("MODIFIERS_DOUBLE"); // Result: 659 cycles on average
			double dCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				dCost += j;
				dCost *= GC.getInfo((HandicapTypes)0).getResearchPercent() / 100.;
				dCost *= GC.getInfo((HandicapTypes)0).getAIResearchPercent() / 100.;
				EraTypes eTechEra = (EraTypes)GC.getInfo((TechTypes)0).getEra();
				dCost *= (100 + GC.getInfo(eTechEra).getTechCostModifier()) / 100.;
				dCost *= GC.getInfo((WorldSizeTypes)0).getResearchPercent() / 100.;
				dCost *= GC.getInfo((SeaLevelTypes)0).getResearchPercent() / 100.;
				dCost *= 1.05;
				dCost *= GC.getInfo((GameSpeedTypes)0).getResearchPercent() / 100.;
				dCost *= GC.getInfo((EraTypes)0).getResearchPercent() / 100.;
				dCost *= (GC.getDefineINT(
						CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) + 100) / 100.;
				int iCost = ::round(dCost) % 5;
				dCost = range(iCost, 10, 2000);
			}
			iDummy += (int)dCost;
		}
	}
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("MODIFIERS_FLOAT"); // Result: 641 cycles on average
			double fCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				fCost += j;
				fCost *= GC.getInfo((HandicapTypes)0).getResearchPercent() / 100.f;
				fCost *= GC.getInfo((HandicapTypes)0).getAIResearchPercent() / 100.f;
				EraTypes eTechEra = (EraTypes)GC.getInfo((TechTypes)0).getEra();
				fCost *= (100 + GC.getInfo(eTechEra).getTechCostModifier()) / 100.f;
				fCost *= GC.getInfo((WorldSizeTypes)0).getResearchPercent() / 100.f;
				fCost *= GC.getInfo((SeaLevelTypes)0).getResearchPercent() / 100.f;
				fCost *= 1.05f;
				fCost *= GC.getInfo((GameSpeedTypes)0).getResearchPercent() / 100.f;
				fCost *= GC.getInfo((EraTypes)0).getResearchPercent() / 100.f;
				fCost *= (GC.getDefineINT(
						CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) + 100) / 100.f;
				int iCost = ::round(fCost) % 5;
				fCost = range(iCost, 10, 2000);
			}
			iDummy += (int)fCost;
		}
	}

	// More modifier measurements; mostly compile-time constants this time.
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("CONST_MODIFIERS_BTS_STYLE"); // Result: 647 cycles on average
			int iCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				iCost += j;
				iCost *= 125;
				iCost /= 100;
				iCost *= GC.getInfo((HandicapTypes)0).getAIResearchPercent();
				iCost /= 100;
				iCost *= 75;
				iCost /= 100;
				iCost *= 200;
				iCost /= 100;
				iCost *= GC.getInfo((SeaLevelTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= 67;
				iCost /= 100;
				iCost *= GC.getInfo((GameSpeedTypes)0).getResearchPercent();
				iCost /= 100;
				iCost *= 80;
				iCost /= 100;
				iCost = range(iCost, 10, 2000);
			}
			iDummy += iCost;
		}
	}
	{
		for (int i = 0; i < 10; i++)
		{
			//	Result: 2087 cycles on average (scaled_int w/ MulDiv).
			//	672 for scaled_uint, 579 for scaled_uint w/o overflow handling.
			//	394 if the per100 arguments are also cast to uint.
			//TSC_PROFILE("CONST_MODIFIERS_SCALED");
			scaled_int rCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				rCost += j;
				rCost *= per100(125);
				rCost *= per100(GC.getInfo((HandicapTypes)0).getAIResearchPercent());
				rCost *= per100(75);
				rCost *= per100(200);
				rCost *= per100(GC.getInfo((SeaLevelTypes)0).getResearchPercent());
				rCost *= fixp(2/3.);
				rCost *= per100(GC.getInfo((GameSpeedTypes)0).getResearchPercent());
				rCost *= per100(80);
				rCost.clamp(10, 2000);
			}
			iDummy += rCost.round();
		}
	}
	{
		for (int i = 0; i < 10; i++)
		{
			//TSC_PROFILE("CONST_MODIFIERS_DOUBLE"); // Result: 491 cycles on average
			double dCost = GC.getInfo((TechTypes)0).getResearchCost();
			for (int j = i; j < 10; j++)
			{
				dCost += j;
				dCost *= 1.25;
				dCost *= GC.getInfo((HandicapTypes)0).getAIResearchPercent() / 100.;
				dCost *= 0.75;
				dCost *= 2.;
				dCost *= GC.getInfo((SeaLevelTypes)0).getResearchPercent() / 100.;
				dCost *= 2/3.;
				dCost *= GC.getInfo((GameSpeedTypes)0).getResearchPercent() / 100.;
				dCost *= 0.8;
				dCost = dRange(dCost, 10, 2000);
			}
			iDummy += (int)dCost;
		}
	}

	// Syntax test
	/*	Based on some calculations in InvasionGraph::step. Out of context,
		the excerpts used here and their names are nonsensical. */
	scaled_int rOtherDist = 2;
	scaled_int rMod = scaled_int::max(100 - 2 * rOtherDist, 50) / 100;
	scaled_int rOtherPow = 1282;
	rOtherPow *= rMod;
	scaled_int rPow = 1417;
	scaled_int rDist = 5;
	rPow *= scaled_int::max(100 - fixp(1.55) * rDist.pow(fixp(1.15)), 50) / 100;
	scaled_int rWeight, rOtherWeight;
	rWeight = rOtherWeight = 1;
	rOtherWeight *= (rOtherPow > 1000 ? fixp(1.5) : fixp(4/3.));
	rWeight += fixp(.25);
	rWeight.clamp(fixp(.5), 1);
	rPow *= rWeight;
	if(rOtherWeight <= per100(99))
		rOtherPow *= rOtherWeight;
	iDummy += (rPow * (rOtherPow / 2) + rWeight).round();

	if (iDummy == -7)
	{
		FAssert(false);
		CvGlobals::getInstance().getLogging() = true;
	}
#endif // SCALED_INT_TEST
}
