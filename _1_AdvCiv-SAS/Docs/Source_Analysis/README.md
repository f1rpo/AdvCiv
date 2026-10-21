# Source Analysis

These tracked development records preserve resumable LLM-assisted source investigations. The folder is excluded from player release archives and generated historical commit patches, but its current files remain included in Git clones and AdvCiv-SAS light-source handoffs.

## Historical source archaeology

[`historical_source_archaeology.txt`](/_1_AdvCiv-SAS/Docs/Source_Analysis/historical_source_archaeology.txt) records the completed commit-oriented BtS, K-Mod, AdvCiv and AdvCiv-SAS lineage sweep and subsequent repair status.

## Current-tree audit albums

[`cpp_file_audit_album.txt`](/_1_AdvCiv-SAS/Docs/Source_Analysis/cpp_file_audit_album.txt) reviews current C++ files and cross-file contracts. Future Python, XML or other file-oriented albums belong beside it.

## Focused reviews

[`global_define_name_length_review.md`](/_1_AdvCiv-SAS/Docs/Source_Analysis/global_define_name_length_review.md) compares current SAS GlobalDefine key lengths with inherited/reference XML, separates the hard 255-byte loader boundary from optional shorter naming policy, and records the lookup/performance/save-compatibility evidence for any later rename work.

Keep canonical filenames stable across checkpoints. Preserve each audit's completed results, false positives and exact continuation cursor; reconcile provisional KI numbers against current Known Issues before promotion.
