@echo off
echo !!!
echo !!! Remember to update the version numbers in three places: changelog, C3X.h, README
echo !!!
7z.exe a NEW_RELEASE.zip Art lend tcc Text C3X.h changelog.txt civ_prog_objects.csv Civ3Conquests.h default.c3x_config.ini ep.c injected_code.c common.c INSTALL.bat README.txt RUN.bat "Trade Net X/TradeNetX.dll" "Trade Net X/TradeNetX.cpp" "Trade Net X/Civ3Conquests.hpp" "Trade Net X/BUILD.bat" "Trade Net X/TRADE NET X INFO.txt"
pause
