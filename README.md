# Self-executable WAD file containing the DOOM.EXE binary

This is a silly idea, sparked by a tweet and encouraged by the
one and only polyglot file god:
[Ange Albertini](https://twitter.com/angealbertini)

## errr ok... what do I need to try this?

 * The shareware Doom (go find doom19s.zip), unzipped below this folder
 * Dosbox (standard package or debug build)
 * faucc 16-bit compiler (standard package in Deb/Ubuntu)
 * gcc 32-bit compiler and build-essentials tooling (make)
 * Lunacy.

## How do I try this?

 * You should be able to run `make test` and it all just works..

## How does this work?

 * There are three stages involved:
   * Packing new lumps (unpacker, DOOM.EXE) into the WAD:
     This is done by `wadinject`, an un-surprising command line C program,
     which emits the new WAD as an MS-DOS .COM program.
   * Unpacking DOOM.EXE from the self-executable WAD/.COM:
     This is done by a 16bit COM program stub that forms part of the WAD
     itself (and where the polyglot magic happens).
   * Renaming the .COM back to .WAD (or DOOM doesn't work), then launching
     the unpacked DOOM.EXE (now D.EXE).

## Are there binaries?

Yep - check the [releases](/phlash/wadexe/releases) folder...
