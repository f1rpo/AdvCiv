Draft of balance/historicity changes for the AdvCiv mod. Dec2018
**************************************************************************

tech-tree.pdf: The proposed tech tree
stat-changes.pdf: All other stat changes (units, buildings ...)
Rationales in footnotes

Those specific changes should be accompanied by the following general rule changes not fully covered by the PDFs:

1. Combat system overhaul:
* Greatly reduce the number of collateral damage targets, the collateral damage limit and the damage limit of siege units (see stat-changes.pdf). Exempt some units from damage limits; e.g. let air units destroy ships.
* Only four units, chosen at random, are available for defense in each stack; the best defender is chosen from that subset. After each attack, another random defender becomes available. Prototype implementation:
https://github.com/f1rpo/AdvCiv/compare/master...f1rpo:defender-randomization
* Limited capacity of cities and Forts: No defensive bonus while stack size exceeds capacity.
* Limited number of attacks per turn against a single tile.
* Limited number of combat rounds, so that combat can end in a draw (like air combat).
* Limited free healing; when land units are badly damaged, some need to be disbanded in order to heal the others. Or some other cost on healing. Perhaps unlimited free healing for the AI, as a player handicap.
See also: https://forums.civfanatics.com/threads/advanced-civ.614217/page-5#post-15157996

2. Lower the production bonuses from traits and resources to 80%. Let production modifiers and special commerce modifiers apply to Processes (Wealth, Research, Culture), but halve both modifiers. Reduce the production-to-commerce conversion rate of Processes to 80%.

3. Better scaling of Great Person effects (e.g. Discover ability, Trade Mission) with the game progress - currently too powerful in the early game and midgame. Revise the progression of birth thresholds. Settled Great People grant a free building (e.g. a Bank from a Merchant), but have a limited lifetime. Hurry-wonder mission for Great Prophet (religious wonders).

4. Much lower corporation yields overall; the yields per city decrease each time the corporation expands. No HQ required in order to train Executives, but increased maintenance (and/or some other handicap) when there is no HQ or when war or a civic makes it inaccessible.

5. Make resources less crucial for health and happiness as this disadvantages small civs too much by the mid/late game: A few more resources go obsolete (see tech-tree.pdf) and perhaps cap the total health bonus of buildings like Granary at 2 (not spelled out in stat-changes.pdf). Grant happiness from city culture rate (with diminishing returns) regardless of whether the culture slider is used; have the slider position only matter for Colosseum and Theater.
(Already implemented: The AI is willing to sell even non-surplus resources at reasonable prices when it doesn't need those resources for city growth.)

6. Civs running a state religion different from that of the Apostolic Palace (AP) can't be AP members. The AP owner picks all proposals, no election of the AP resident. UN and AP voting populations are weighted by city nationality (i.e. relative city tile culture).

7. Increase the cost of drafting and balance it by adding or removing food to/from the city's food store in addition to population loss. And stricter checks and balances. Remove the whip ability of Slavery (see stat-changes.pdf).

8. Tech trading: Imparting a tech costs an amount of research points. Probably to be paid after the trade, blocking further research and tech trades in the meantime.

9. Tech diffusion based on culture in foreign city tiles (and other factors).

10. Make nuke damage to units highly randomized, usually destroying some units outright and leaving others unscathed. Units can't move/attack into a tile on the same turn that it was nuked.

11. Declarations of war take effect only at the end of a turn, i.e. the attacked side gets to make the first move. Or perhaps merely a rule change to ensure that a defending fleet gets a chance to destroy transports before they're able to unload (cf. change id 162 in the AdvCiv manual).

12. Ravages of war: (Civ 4 Reimagined and Dawn of Civilization already do some of this.)
* Razing a city takes one attack per population; each attack reduces the population by one and has a chance of destroying a building.
* Reduced city defense causes anger; bombardment causes war weariness and has a small chance of destroying a building. Each attack against a city defender has a chance of destroying a building. Upon entering the city, few buildings or none are destroyed (unless razed).
* Reduce capture gold; perhaps only grant it when razing.

13. City garrisons need to have sufficient (defensive) combat strength in order to quell "fear for safety" anger. (Already implemented as change 500b, but currently disabled through an XML switch.)

14. Changes to finances:
* Fold inflation into increased costs for units, civics and cities; i.e. pay for having a lot of stuff, not for the game turn number.
* Add a small gold cost to air missions and airlifting.
* Stronger impact of upkeep class on civic upkeep costs.
* Overhaul of trade route profit modifiers (rewarding trade between coastal cities, even if on the same continent) and the matching of partner cities.

15. A slight waste effect when a commerce slider is in an extreme position (akin to Alpha Centauri) or a small cost for adjusting a slider.