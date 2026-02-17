Version 3.13:

- Disabled DPI support via project manifest; scaling is now handled in code
- Added two new files to the project: dpi_utils.cpp and dpi_utils.h
- Implemented calculation of the interface scaling factor based on window, monitor, or system DPI (depending on available APIs)
- Added runtime DPI change handling (WM_DPICHANGED, starting from Windows 10)
- Updated WM_GETMINMAXINFO and WM_SIZE message handling to account for current scaling
- Rewritten code for positioning and scaling interface elements (buttons, controls, lists, fonts)
- Scaling now correctly applies to menus and standard dialog windows
- DPI-based interface scaling is supported on systems starting from Windows XP

Version 3.12:

- Removed the "Backup" folder (left over from converting the project from an old type to a new one)
- Removed the "Gl" folder (old "*.h" files no longer used in the project)
- Removed cursor file "cursor1.cur" and its references in the project (from resource file "1.rc")
- Disabled "Use of MFC" in project properties ("Use standard Windows libraries")
- For "Release" configuration, set "Whole Program Optimization" to "Use Link-Time Code Generation" (previously "No Optimization")
- For "Debug" and "Release" configurations, set "Manifest Tools" → "Input and Output" → "DPI Awareness" to "High DPI Support" (to render window elements pixel-perfect without scaling)
- For "Debug" configuration (for "Release" it was already set), configured "Configuration Properties" → "C++" → "Code Generation" → "Runtime Library" to "(/MTd)" (for "Release" it is "(/MT)") — to avoid DLL dependencies and ensure Windows XP compatibility
- Removed files "sound_rec.vcproj", "sound_rec.vcproj.SEM-HOME..", "sound_rec.vcproj.SEM-NOTEBOOK.SEM.user" (left over from old project, no longer needed)
- Removed "afxres.h" file (not needed with MFC disabled)
- Edited resource file "1.rc" — removed leftover junk code from old project and MFC support, replaced include "afxres.h" with <windows.h>
- Created a subfolder "res" in the project folder and moved all "*.ico" files there
- Removed "sound_rec.ncb" and "sound_rec.suo" files from the root folder (where "sound_rec.sln" is located), they were left over from the old project, no longer needed

- Added a checkbox "Show Event Log" to toggle visibility of the event log
- Changed the size of the event log list
- Moved and resized the event log clear button (now at the bottom of the list instead of on the side)
- Added action for pressing the event log clear button + SHIFT key (toggle clipping display)

- Code optimization