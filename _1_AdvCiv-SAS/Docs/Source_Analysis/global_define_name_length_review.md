# GlobalDefine name-length review

Reviewed on 2026-10-21 by ChatGPT-5.6-Sol after the KI#1001 DefineTextVal capacity mitigation. This is a focused source/data review of `DefineName` length; it does not rename defines, change gameplay, add a stricter style CI limit, or resume the C++ File Audit Album.

## Scope and conclusion

This review follows a separate question raised while fixing KI#1001: SAS define names are often much longer than inherited Firaxis/K-Mod/AdvCiv names, so should they be shortened for safety, maintenance or performance?

The current evidence supports separating three different concerns:

- **Loader safety:** `CvXMLLoadUtility::ReadGlobalDefines` still reads each `<DefineName>` through `char szName[256]`. The existing KI#1001 CI therefore correctly treats 255 UTF-8 bytes as the maximum repository-safe content length, leaving one byte for the C-string terminator.
- **Variable-system capacity:** after loading, the name is stored as a `std::string` key in `stdext::hash_map<std::string, FVariable*>`. No smaller semantic key-length limit is visible in the DLL source.
- **Naming policy:** current SAS names are much longer than inherited names, but none approaches the 255-byte loader boundary. Any lower limit would therefore be a readability/headroom convention, not an established Civ4 correctness requirement.

The current longest shipped name is 114 bytes. The longest name in the current inherited XML files shipped by AdvCiv-SAS is 42 bytes, while the stock BtS XML represented by Civ4CE reaches 46 bytes.

Targeted public searches did not find a documented Civ4 `DefineName` failure threshold below the loader's visible 256-byte scratch buffer. Absence of a forum report is not proof of safety; the source-derived 255-byte repository limit remains the stronger contract.

## Relationship to the audit album and KI#1001

This is not a newly promoted audit finding.

C031-WIP555 / F680, later KI#1001, found a concrete shipped `DefineTextVal` violation: `SAS_MAP_SCRIPT_NAMES_LAND_HEAVY` had reached 297 bytes while the inherited text branch used `char szVal[256]`. That was an active data/loader mismatch and was mitigated separately.

The same album pass also noticed `char szName[256]`. Its then-current data census measured the largest SAS `DefineName` at 126 bytes and explicitly classified define-name capacity as a preventive hardening surface rather than part of F680. The Queue-052 final raw-buffer census again found no second current capacity root.

The current tree has since fallen from that 126-byte historical maximum to 114 bytes. This review therefore extends the preventive question without changing the completed album record or claiming another KI.

## Loader and lookup mechanics

The inherited GlobalDefines reader uses a fixed temporary for the key:

```cpp
char szName[256];
if (GetChildXmlVal(szName))
    GC.getDefinesVarSystem()->SetValue(szName, value);
```

The visible FXML raw-string interface receives a `char*` but no destination capacity. As with KI#1001's text values, the closed EXE does not expose what it would do with an overlong name. A 255-byte content ceiling is therefore the conservative lossless boundary for this specific `char[256]` destination.

After loading, `FVariableSystem` does not retain a 256-byte key array. It declares:

```cpp
typedef stdext::hash_map<std::string, FVariable*> FVariableHash;
```

and lookups use `m_mapVariableMap.find(szVariable)`. String values are dynamically allocated too. This supports treating 256 as an XML-loader scratch-buffer constraint rather than a general `FVariableSystem` naming limit.

The public Civ IV Wiki describes GlobalDefines as `DefineName` paired with typed value fields, without documenting a separate short-name limit. A CivFanatics DLL explanation likewise describes `getDefineINT` as looking up XML-loaded values in a hash table with the strings as keys.

Those public references are useful corroboration, but the local SDK source remains the stronger evidence for the exact implementation used by SAS.

## Current AdvCiv-SAS census

The current mod folder contains 977 defines across the nine root GlobalDefines/PythonCallbackDefines files checked by the loader-safety CI.

| Current file | Defines | Longest `DefineName` bytes | Longest name |
| --- | ---: | ---: | --- |
| `GlobalDefines_advciv_sas.xml` | 667 | 114 | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_HEAVY_INVESTMENT_MAX_TURNS_NORMAL_GAMESPEED` |
| `GlobalDefines_advc.xml` | 134 | 42 | `WE_LOVE_THE_KING_POPULATION_MIN_POPULATION` |
| `PythonCallbackDefines.xml` | 62 | 38 | `USE_CAN_FOUND_CITIES_ON_WATER_CALLBACK` |
| `GlobalDefinesAlt.xml` | 40 | 41 | `WARLORD_EXTRA_EXPERIENCE_PER_UNIT_PERCENT` |
| `AI_Variables_GlobalDefines.xml` | 38 | 36 | `UWAI_WEIGHT_THIRD_PARTY_INTERVENTION` |
| `LeadFromBehind_GlobalDefines.xml` | 10 | 23 | `LFB_DEFENSIVEADJUSTMENT` |
| `GlobalDefines_devel.xml` | 9 | 30 | `PER_PLAYER_MESSAGE_CONTROL_LOG` |
| `BBAI_Game_Options_GlobalDefines.xml` | 9 | 32 | `BBAI_VICTORY_STRATEGY_DOMINATION` |
| `TechDiffusion_GlobalDefines.xml` | 8 | 37 | `TECH_COST_FIRST_KNOWN_PREREQ_MODIFIER` |

The eight non-SAS files total 310 defines and top out at 42 bytes. The large difference therefore comes from SAS's intentionally descriptive naming convention rather than a gradual inherited trend toward long keys.

## External inherited/reference census

The local `sources_refs` archives provide additional comparison points outside the current mod folder. Counts below reflect the GlobalDefines/PythonCallbackDefines files actually present in each supplied reference archive; mod archives can omit unchanged stock files, so the counts are not intended as total engine-wide define counts.

| Reference | Defines scanned | Longest `DefineName` bytes | Longest name |
| --- | ---: | ---: | --- |
| AdvCiv 1.14 | 312 | 42 | `WE_LOVE_THE_KING_POPULATION_MIN_POPULATION` |
| K-Mod 1.46 | 111 | 41 | `WARLORD_EXTRA_EXPERIENCE_PER_UNIT_PERCENT` |
| Civ4CE BtS stock XML | 430 | 46 | `INITIAL_FREE_MILITARY_UNITS_POPULATION_PERCENT` |
| Civ4CE Warlords stock XML | 311 | 46 | `INITIAL_FREE_MILITARY_UNITS_POPULATION_PERCENT` |
| Civ4CE Civ4 stock XML | 295 | 46 | `INITIAL_FREE_MILITARY_UNITS_POPULATION_PERCENT` |
| Taurus 1.04 | 34 | 30 | `GLOBAL_WARMING_UNHEALTH_WEIGHT` |

These references reinforce two points: inherited naming is substantially shorter than current SAS naming, but the observed inherited maxima do not themselves establish an engine limit. In particular, using 42 or 46 bytes as a hard SAS rule would be a style decision rather than a source-supported safety requirement.

## Name-length distributions and possible style ceilings

2026-10-21 census by GPT-6.1-Sol: the 977 shipped names are unique and all ASCII, so byte and character lengths coincide. The tables separate the 667 `SAS_` names from the 310 inherited names shipped in the mod; the external reference archives above are outside these distributions.

Each band spans five bytes. Share is the percentage in that band; the cumulative column gives the percentage at or below its upper bound. Each example is a real name from that band, with its exact length. Empty bands use the closest name from the same group, explicitly labelled as outside the band.

This mirrors SAS's filename-length review approach: compare the distribution in five-unit bands alongside real examples to choose a useful desired maximum, while keeping that naming preference distinct from technical capacity limits.

### SAS names

| Name length (bytes) | Names in band | Share | At/below band maximum | Example | Actual length |
| --- | ---: | ---: | ---: | --- | ---: |
| 1-5 | 0 | 0.00% | 0.00% | `SAS_UI_FONT_BODY` (nearest; outside band) | 16 |
| 6-10 | 0 | 0.00% | 0.00% | `SAS_UI_FONT_BODY` (nearest; outside band) | 16 |
| 11-15 | 0 | 0.00% | 0.00% | `SAS_UI_FONT_BODY` (nearest; outside band) | 16 |
| 16-20 | 8 | 1.20% | 1.20% | `SAS_BBAI_LOG_ENABLE` | 19 |
| 21-25 | 16 | 2.40% | 3.60% | `SAS_BBAI_CITY_LOG_LEVEL` | 23 |
| 26-30 | 22 | 3.30% | 6.90% | `SAS_LAUNCH_GUARD_TEST_STRING` | 28 |
| 31-35 | 49 | 7.35% | 14.24% | `SAS_AI_DO_CIVICS_SWITCH_MIN_SLACK` | 33 |
| 36-40 | 75 | 11.24% | 25.49% | `SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL` | 38 |
| 41-45 | 100 | 14.99% | 40.48% | `SAS_AI_ATTACK_CITY_SKIP_UPGRADE_WAIT_ENABLE` | 43 |
| 46-50 | 93 | 13.94% | 54.42% | `SAS_AI_ATTACK_CITY_REMOTE_CAPTURED_CORE_DISTANCE` | 48 |
| 51-55 | 73 | 10.94% | 65.37% | `SAS_AI_CHOOSE_UNIT_NO_EXCESS_SIEGES_ALL_MIN_EXTRA_CAP` | 53 |
| 56-60 | 64 | 9.60% | 74.96% | `SAS_AI_CIVIC_VALUE_UNLIMITED_SPECIALIST_YIELD_HAMMER_VALUE` | 58 |
| 61-65 | 46 | 6.90% | 81.86% | `SAS_AI_CHOOSE_PRODUCTION_FIRST_SETTLER_PREFERRED_MIN_POPULATION` | 63 |
| 66-70 | 32 | 4.80% | 86.66% | `SAS_AI_BUILDING_VALUE_DOMAIN_SEA_FREE_EXPERIENCE_THROUGHPUT_OPTIMIZE` | 68 |
| 71-75 | 46 | 6.90% | 93.55% | `SAS_AI_BUILDING_VALUE_DOMAIN_LAND_PRODUCTION_MODIFIER_THROUGHPUT_OPTIMIZE` | 73 |
| 76-80 | 19 | 2.85% | 96.40% | `SAS_AI_CHOOSE_PRODUCTION_MIN_OFFENSIVE_PRODUCTION_CAPACITY_RENAISSANCE_PERCENT` | 78 |
| 81-85 | 5 | 0.75% | 97.15% | `SAS_AI_BUILDING_VALUE_MILITARY_NATIONAL_WONDER_UNIT_PRODUCTION_SHARE_FLOOR_PERCENT` | 82 |
| 86-90 | 6 | 0.90% | 98.05% | `SAS_AI_CHOOSE_UNIT_PEACEFUL_LAND_MILITARY_SATURATION_MAX_INDEPENDENT_RIVAL_TEAMS_IN_AREA` | 88 |
| 91-95 | 5 | 0.75% | 98.80% | `SAS_WORKER_AI_PHASE0_PRODUCTIVE_FEATURE_CHOP_MIN_PRODUCTION_TURNS_NORMAL_GAMESPEED_WORTH_X100` | 93 |
| 96-100 | 5 | 0.75% | 99.55% | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_MAX_TURNS_NORMAL_GAMESPEED` | 97 |
| 101-105 | 1 | 0.15% | 99.70% | `SAS_AI_TECH_VALUE_PRODUCTIVE_FEATURE_REMOVE_MIN_CHOP_BASE_PRODUCTION_TURNS_NORMAL_GAMESPEED_EQUIVALENT` | 102 |
| 106-110 | 1 | 0.15% | 99.85% | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_HEAVY_INVESTMENT_MIN_COMPLETION_PERCENT` | 110 |
| 111-115 | 1 | 0.15% | 100.00% | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_HEAVY_INVESTMENT_MAX_TURNS_NORMAL_GAMESPEED` | 114 |

### Inherited names shipped in SAS

| Name length (bytes) | Names in band | Share | At/below band maximum | Example | Actual length |
| --- | ---: | ---: | ---: | --- | ---: |
| 1-5 | 0 | 0.00% | 0.00% | `LFB_ENABLE` (nearest; outside band) | 10 |
| 6-10 | 2 | 0.65% | 0.65% | `LFB_ENABLE` | 10 |
| 11-15 | 20 | 6.45% | 7.10% | `LOG_MAP_STATS` | 13 |
| 16-20 | 44 | 14.19% | 21.29% | `CAMERA_LOWER_PITCH` | 18 |
| 21-25 | 91 | 29.35% | 50.65% | `CAMERA_MAX_SCROLL_SPEED` | 23 |
| 26-30 | 85 | 27.42% | 78.06% | `ALERT_ON_NO_LONGER_WAR_TRADE` | 28 |
| 31-35 | 48 | 15.48% | 93.55% | `BBAI_SKIP_BOMBARD_MIN_STACK_RATIO` | 33 |
| 36-40 | 17 | 5.48% | 99.03% | `REVOLT_DEFENSE_STATE_RELIGION_MODIFIER` | 38 |
| 41-45 | 3 | 0.97% | 100.00% | `WE_LOVE_THE_KING_POPULATION_MIN_POPULATION` | 42 |

The SAS median is 49 bytes; its nearest-rank 95th percentile is 77 bytes and its maximum is 114. An 80-byte ceiling accommodates 96.40% of current SAS names and leaves 24 above it; every inherited name already fits, with a maximum of 42 bytes.

### Possible style ceilings

The four names above 96 bytes are:

| Bytes | Current name |
| ---: | --- |
| 114 | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_HEAVY_INVESTMENT_MAX_TURNS_NORMAL_GAMESPEED` |
| 110 | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_HEAVY_INVESTMENT_MIN_COMPLETION_PERCENT` |
| 102 | `SAS_AI_TECH_VALUE_PRODUCTIVE_FEATURE_REMOVE_MIN_CHOP_BASE_PRODUCTION_TURNS_NORMAL_GAMESPEED_EQUIVALENT` |
| 97 | `SAS_AI_CHOOSE_PRODUCTION_SPACE3_MINIMUM_SPACESHIP_COMPONENT_CONTINUITY_MAX_TURNS_NORMAL_GAMESPEED` |

A possible later style rule around 96 bytes would therefore target only four current outliers while leaving 159 bytes of loader headroom. A lower rule such as 80 bytes would be a broader naming cleanup affecting 24 current SAS keys. Neither number is an engine requirement, and this review deliberately does not choose or enforce one.

The inherited 42-46-byte range is useful as a comparison baseline, but adopting it directly would rename hundreds of otherwise understandable SAS settings. That would create substantial churn for little demonstrated benefit.

## Performance considerations

Shorter names can reduce key storage and the amount of string data involved in hashing/comparison, so there is a small technical basis for expecting shorter keys to be cheaper in isolation. The actual lookup cost is not determined by length alone: hash collisions and where strings first differ also matter.

More importantly, AdvCiv-SAS already avoids many repeated string lookups. AdvCiv's centralized cached integer defines use enum-indexed access, and SAS commonly uses function-local `static const` values for fixed process-lifetime settings. In those cases the string lookup is paid during cache initialization or first use rather than on every gameplay read.

No define-name benchmark was performed for this review. Any shortening should therefore be justified primarily by readability, reduced typo surface and extra headroom below the inherited loader boundary, not advertised as a gameplay performance optimization.

## Save and integration considerations

No current caller was found that serializes the GlobalDefines `FVariableSystem` through its generic `Read`/`Write` methods. Define names therefore are not part of the current SAS save schema merely because `FVariableSystem` supports generic stream serialization.

A pure rename that preserves the value and behavior should not require save conversion. It does require every XML/C++/Python/documentation reference to move together, and a native define literal requires a matching rebuilt DLL/XML pair. The existing DLL define-change guard is the appropriate integration protection for such changes.

Behavioral redesigns should remain separate. In particular, the later map-heaviness idea—treating map-script identity as indicative while also evaluating each player's actual geography—changes AI/gameplay interpretation and is not part of a define-name cleanup.

## Possible follow-up workflow

A possible low-risk continuation would be:

1. Keep the existing 255-byte loader-safety CI as the hard correctness boundary.
2. If a lower style ceiling is desired, test it against one representative long define first and review whether the shorter name remains clear.
3. Rename coherent families or functional groups in separate reviewable batches rather than mechanically abbreviating every key at once.
4. Maintain an old-to-new mapping while renames are in progress so historical commits, logs and documentation remain searchable.

If a lower style ceiling is eventually adopted, append the chosen rationale and conversion mapping here rather than rewriting the historical measurements above.

## Evidence and references

- Current SAS loader/storage: `CvGameCoreDLL/CvXMLLoadUtilitySet.cpp`, `CvGameCoreDLL/FVariableSystem.h`, `CvGameCoreDLL/FVariableSystem.inl` and `CvGameCoreDLL/CvGlobals.cpp`.
- Current SAS data and CI: `Assets/XML/*GlobalDefines*.xml`, `Assets/XML/PythonCallbackDefines.xml` and `.github/workflows/build/xml/global_define_lengths.py`.
- Album context: `cpp_file_audit_album.txt`, C031-WIP555/WIP556, F680/KI#1001.
- Local external references: `sources_refs/AdvCiv-1.14.zip`, `Civ4-K-Mod-master-1.46.zip`, `Civ4CE-main.zip` and `Taurus-1.04.zip`.
- [Civ IV Wiki: GlobalDefines](https://civ4.wikidot.com/xml:globaldefines) documents the public XML structure.
- [CivFanatics: How to change city growth threshold?](https://forums.civfanatics.com/threads/how-to-change-city-growth-threshold.619005/) includes Nightinggale's explanation that GlobalDefines are cached in a hash table with string lookup keys.
