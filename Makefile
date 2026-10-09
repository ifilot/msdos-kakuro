PYTHON ?= python3
.DEFAULT_GOAL := build
PUZZLE ?= 001
GENERATED = src/DIGITDAT.H src/PROFDAT.H src/CATALOG.H src/DOCDATA.H assets/KAKURO.DAT assets/PUZZLES.DAT assets/SOUND.DAT src/SNDDATA.H

.PHONY: build run fonts test-fonts test-video test-puzzle test-start test-menu test-perf test-assets release test-documents test-archive test-sound smoke verify-toolchain
src/DIGITDAT.H: assets/fonts/digits.json tools/generate_fonts.py
	$(PYTHON) tools/generate_fonts.py

assets/splash/START.VGA assets/splash/START.PAL &: assets/splash/courtyard.bin assets/splash/courtyard.pal tools/generate_splash.py
	$(PYTHON) tools/generate_splash.py

src/PROFDAT.H: assets/fonts/internet/source/profont22.bdf tools/generate_profont.py
	$(PYTHON) tools/generate_profont.py

build: $(GENERATED)
	$(PYTHON) tools/dos.py build

fonts: build
	$(PYTHON) tools/dos.py run --fonts

test-fonts: $(GENERATED)
	$(PYTHON) tools/dos.py test-fonts

run: build
	$(PYTHON) tools/dos.py run --puzzle "$(PUZZLE)"

test-puzzle: $(GENERATED)
	$(PYTHON) tools/dos.py test-puzzle

test-video: $(GENERATED)
	$(PYTHON) tools/dos.py test

test-start: $(GENERATED)
	$(PYTHON) tools/dos.py test-start

smoke:
	$(PYTHON) buildenv/smoke.py

verify-toolchain:
	cd buildenv && sha256sum --check SHA256SUMS

test-menu: $(GENERATED)
	$(PYTHON) tools/dos.py test-menu

src/CATALOG.H: tools/generate_catalog.py $(wildcard assets/puzzles/*.puz)
	$(PYTHON) tools/generate_catalog.py

assets/KAKURO.DAT: tools/pack_assets.py assets/splash/START.VGA assets/splash/START.PAL assets/background/BOARD.VGA assets/menu/MENU.VGA
	$(PYTHON) tools/pack_assets.py

test-perf: $(GENERATED)
	DOS_CYCLES=3000 $(PYTHON) tools/dos.py test-perf

release: build
	$(PYTHON) tools/package_release.py

test-assets: $(GENERATED)
	$(PYTHON) tests/test_asset_pack.py
	$(PYTHON) tools/dos.py test-assets

src/DOCDATA.H: tools/generate_documents.py assets/documents/HELP.TXT
	$(PYTHON) tools/generate_documents.py

assets/PUZZLES.DAT: tools/pack_puzzles.py tools/pack_assets.py tools/generate_catalog.py $(wildcard assets/puzzles/*.puz)
	$(PYTHON) tools/pack_puzzles.py

test-documents: $(GENERATED)
	$(PYTHON) tools/dos.py test-documents

test-archive: $(GENERATED)
	$(PYTHON) tests/test_puzzle_pack.py
	$(PYTHON) tools/dos.py test-archive

assets/SOUND.DAT src/SNDDATA.H &: tools/pack_sound.py $(wildcard assets/sound/fm/*) $(wildcard assets/sound/midi/*)
	$(PYTHON) tools/pack_sound.py

test-sound: $(GENERATED)
	$(PYTHON) tests/test_sound.py
	$(PYTHON) tools/dos.py test-sound
