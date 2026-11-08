# Build and publish

This project builds the plugin DLL locally. Steam Workshop upload is done in Kenshi's Forgotten Construction Set (FCS); there is no SteamCMD upload command in this repository.

## Requirements

- Python 3
- Visual Studio Build Tools with the x64 C++ tools
- Kenshi and FCS installed through Steam
- The existing Workshop item for Elemental Enchantment (Steam ID `3832978749`)

The commands below assume PowerShell is open at the repository root and that Kenshi is installed at `E:\SteamLibrary\steamapps\common\Kenshi`. Adjust `$gameMod` if the Steam library is elsewhere.

## Optional API reference

Install Doxygen, then run `doxygen .\Doxyfile` from the repository root. Generated HTML goes to `docs\api\` and is ignored by Git.

## Build and stage

```powershell
# Only needed after changing MCM translations:
python .\source\generate_mcm_strings.py

# Only needed after changing UI translations or game terms:
python .\source\generate_localization.py

# Build the normal plugin DLL:
python .\build.py
if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed; do not stage an older DLL.' }
python .\tools\check_release_exports.py
if ($LASTEXITCODE -ne 0) { throw 'Plugin export or dependency check failed.' }

# Put that DLL into the project package first:
Copy-Item -LiteralPath '.\build\ElementalEnchantmentTest.dll' `
    -Destination '.\mod\Elemental Enchantment\plugin\ElementalEnchantmentTest.dll' -Force

# Copy the tracked Workshop package into Kenshi's local mods folder.
$gameMod = 'E:\SteamLibrary\steamapps\common\Kenshi\mods\Elemental Enchantment'
robocopy '.\mod\Elemental Enchantment' $gameMod /E
if ($LASTEXITCODE -ge 8) { throw "Package copy failed (robocopy exit $LASTEXITCODE)." }

# Confirm the staged plugin matches the new build:
$builtHash = (Get-FileHash '.\build\ElementalEnchantmentTest.dll').Hash
$packageHash = (Get-FileHash '.\mod\Elemental Enchantment\plugin\ElementalEnchantmentTest.dll').Hash
$stagedHash = (Get-FileHash "$gameMod\plugin\ElementalEnchantmentTest.dll").Hash
if ($builtHash -ne $packageHash -or $builtHash -ne $stagedHash) {
    throw 'Package or staged DLL does not match the build.'
}
```

All three SHA256 values should match. `robocopy /E` copies the project package into Kenshi without deleting extra files already in the local mod folder; inspect that folder for stale files before testing or publishing.

The DLL dependency check covers static Windows DLL imports only. It does not verify runtime-loaded components such as RE_Kenshi or their versions; those still need in-game verification.

To run the engine-independent native checks, build a separate test DLL and run the check runner:

```powershell
python .\build.py --test
if ($LASTEXITCODE -ne 0) { throw 'Native test build failed.' }
python .\tools\run_native_checks.py
if ($LASTEXITCODE -ne 0) { throw 'Native checks failed.' }
```

The test DLL is separate from the normal plugin and should not be copied into the Workshop package. The runner also checks particle systems and mod-owned material/texture references. To check references to built-in Kenshi assets as well, run:

```powershell
python .\tools\check_particle_resources.py --game-data 'E:\SteamLibrary\steamapps\common\Kenshi\data'
```

These checks do not replace in-game verification. `--frost-dev` and `--ice-experimental` build development variants; do not publish those as the normal plugin.

## Review and push source

```powershell
git diff --check
git status --short
git diff --stat
git add .clang-format .gitignore Doxyfile README.md build.py docs source tools mod
git commit -m "Describe the change"
git push origin main
```

Review `git status` before `git add` and commit only the intended source and package changes. Build output under `build/` is local and should not be committed.

## Upload through FCS

Complete the owner's in-game testing before updating the public Workshop item. A successful build or native check is not approval to publish. Wait for the owner to confirm that testing is complete and authorize the upload.

1. Start `forgotten construction set.exe` from the Kenshi install folder.
2. Open the local `Elemental Enchantment` mod and its Mod Info window.
3. Confirm the title, description, change notes, visibility, and Steam ID `3832978749`. Keeping the existing Steam ID updates the current Workshop item.
4. Click **Upload** and wait for FCS to finish.
5. Open the Workshop item and check its update time and files. If the game is running, restart it before checking the subscribed copy.

FCS controls the Workshop upload step. GitHub `git push` updates the source repository only; it does not publish to Steam.

## Release order

1. Update source and any particle/package files under `mod/Elemental Enchantment/`.
2. Build the normal DLL and copy the package to the Kenshi `mods` folder.
3. Check the copied DLL hashes, inspect the package, then commit and push the reviewed files.
4. Test the staged mod in game and wait for the owner's confirmation and authorization to publish.
5. Upload the same tested local mod through FCS and verify the Workshop page.
